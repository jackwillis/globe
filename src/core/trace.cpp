// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/trace.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace globe {

namespace {

constexpr std::size_t kMaxLine = 200;
constexpr std::size_t kMaxTokens = 6;

struct CommandEntry {
    const char* name;
    Command command;
};

constexpr CommandEntry kCommands[] = {
    {"left", Command::Left},
    {"right", Command::Right},
    {"up", Command::Up},
    {"down", Command::Down},
    {"zoom_in", Command::ZoomIn},
    {"zoom_out", Command::ZoomOut},
    {"reset", Command::Reset},
    {"toggle_graticule", Command::ToggleGraticule},
    {"toggle_borders", Command::ToggleBorders},
    {"toggle_coastlines", Command::ToggleCoastlines},
    {"align_north", Command::AlignNorth},
    {"toggle_drag_mode", Command::ToggleDragMode},
    {"quit", Command::Quit},
};

// Splits a NUL-terminated copy of the line in place; returns the token count, or kMaxTokens + 1
// if there are too many.
std::size_t tokenize(char* s, char* tokens[kMaxTokens]) {
    std::size_t n = 0;
    while (*s != '\0') {
        while (*s == ' ' || *s == '\t' || *s == '\r') {
            *s++ = '\0';
        }
        if (*s == '\0') {
            break;
        }
        if (n == kMaxTokens) {
            return kMaxTokens + 1;
        }
        tokens[n++] = s;
        while (*s != '\0' && *s != ' ' && *s != '\t' && *s != '\r') {
            ++s;
        }
    }
    return n;
}

bool parse_u64(const char* s, std::uint64_t& out) {
    if (*s < '0' || *s > '9') {
        return false;
    }
    char* end = nullptr;
    const unsigned long long v = std::strtoull(s, &end, 10);
    if (*end != '\0' || v > (1ULL << 53)) {
        return false;
    }
    out = v;
    return true;
}

bool parse_int(const char* s, int lo, int hi, int& out) {
    char* end = nullptr;
    if (*s == '\0') {
        return false;
    }
    const long v = std::strtol(s, &end, 10);
    if (*end != '\0' || v < lo || v > hi) {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}

bool parse_double(const char* s, double& out) {
    char* end = nullptr;
    if (*s == '\0') {
        return false;
    }
    const double v = std::strtod(s, &end);
    if (*end != '\0' || !std::isfinite(v)) {
        return false;
    }
    out = v;
    return true;
}

// Retro68's printf has no %ll (see docs/toolchain.md), so 64-bit values are written by hand.
// Doubles still use "%.17g" through snprintf; that conversion is plain C89.
class Writer {
  public:
    Writer(char* buf, std::size_t cap) : buf_(buf), cap_(cap) {}

    void text(const char* s) {
        const std::size_t n = std::strlen(s);
        if (!ok_ || len_ + n >= cap_) {
            ok_ = false;
            return;
        }
        std::memcpy(buf_ + len_, s, n);
        len_ += n;
    }

    void u64(std::uint64_t v) {
        char digits[24];
        std::size_t n = 0;
        do {
            digits[n++] = static_cast<char>('0' + static_cast<int>(v % 10U));
            v /= 10U;
        } while (v != 0);
        char out[24];
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = digits[n - 1 - i];
        }
        out[n] = '\0';
        text(out);
    }

    void integer(int v) { formatted("%d", v); }
    void real(double v) { formatted("%.17g", v); }

    // Terminates the buffer; returns the length, or 0 if anything did not fit.
    std::size_t finish() {
        if (!ok_ || len_ >= cap_) {
            buf_[0] = '\0';
            return 0;
        }
        buf_[len_] = '\0';
        return len_;
    }

  private:
    template <typename T> void formatted(const char* fmt, T v) {
        char tmp[40];
        const int n = std::snprintf(tmp, sizeof tmp, fmt, v);
        if (n < 0 || static_cast<std::size_t>(n) >= sizeof tmp) {
            ok_ = false;
            return;
        }
        text(tmp);
    }

    char* buf_;
    std::size_t cap_;
    std::size_t len_ = 0;
    bool ok_ = true;
};

} // namespace

const char* command_name(Command command) {
    for (const CommandEntry& e : kCommands) {
        if (e.command == command) {
            return e.name;
        }
    }
    return "?";
}

std::optional<Msg> parse_message(std::string_view line) {
    if (line.size() > kMaxLine) {
        return std::nullopt;
    }
    char buf[kMaxLine + 1];
    std::memcpy(buf, line.data(), line.size());
    buf[line.size()] = '\0';
    if (std::memchr(buf, '\0', line.size()) != nullptr) {
        return std::nullopt; // embedded NUL
    }
    char* tok[kMaxTokens];
    const std::size_t n = tokenize(buf, tok);
    if (n == 0 || n > kMaxTokens || tok[0][0] == '#') {
        return std::nullopt;
    }
    const std::string_view verb = tok[0];
    std::uint64_t t = 0;
    if ((verb == "down" || verb == "move") && n == 4) {
        double x = 0.0;
        double y = 0.0;
        if (parse_u64(tok[1], t) && parse_double(tok[2], x) && parse_double(tok[3], y)) {
            if (verb == "down") {
                return Msg{PointerDown{t, x, y}};
            }
            return Msg{PointerMove{t, x, y}};
        }
    } else if (verb == "up" && n == 2) {
        if (parse_u64(tok[1], t)) {
            return Msg{PointerUp{t}};
        }
    } else if (verb == "wheel" && n == 3) {
        int steps = 0;
        if (parse_u64(tok[1], t) && parse_int(tok[2], -1000000, 1000000, steps)) {
            return Msg{Wheel{t, steps}};
        }
    } else if (verb == "key" && n == 3) {
        if (parse_u64(tok[1], t)) {
            for (const CommandEntry& e : kCommands) {
                if (std::strcmp(tok[2], e.name) == 0) {
                    return Msg{CommandIssued{t, e.command}};
                }
            }
        }
    } else if (verb == "resize" && n == 3) {
        int w = 0;
        int h = 0;
        if (parse_int(tok[1], 1, kMaxViewportDimension, w) &&
            parse_int(tok[2], 1, kMaxViewportDimension, h)) {
            return Msg{Resize{w, h}};
        }
    } else if (verb == "expose" && n == 1) {
        return Msg{Repaint{}};
    }
    return std::nullopt;
}

std::size_t format_message(const Msg& msg, char* buf, std::size_t capacity) {
    if (buf == nullptr || capacity == 0) {
        return 0;
    }
    Writer w(buf, capacity);
    if (const auto* down = std::get_if<PointerDown>(&msg)) {
        w.text("down ");
        w.u64(down->time_ms);
        w.text(" ");
        w.real(down->x);
        w.text(" ");
        w.real(down->y);
    } else if (const auto* move = std::get_if<PointerMove>(&msg)) {
        w.text("move ");
        w.u64(move->time_ms);
        w.text(" ");
        w.real(move->x);
        w.text(" ");
        w.real(move->y);
    } else if (const auto* up = std::get_if<PointerUp>(&msg)) {
        w.text("up ");
        w.u64(up->time_ms);
    } else if (const auto* wheel = std::get_if<Wheel>(&msg)) {
        w.text("wheel ");
        w.u64(wheel->time_ms);
        w.text(" ");
        w.integer(wheel->steps);
    } else if (const auto* key = std::get_if<CommandIssued>(&msg)) {
        w.text("key ");
        w.u64(key->time_ms);
        w.text(" ");
        w.text(command_name(key->command));
    } else if (const auto* resize = std::get_if<Resize>(&msg)) {
        w.text("resize ");
        w.integer(resize->width);
        w.text(" ");
        w.integer(resize->height);
    } else {
        w.text("expose");
    }
    return w.finish();
}

} // namespace globe
