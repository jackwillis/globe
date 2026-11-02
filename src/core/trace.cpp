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

struct KeyEntry {
    const char* name;
    Key key;
};

constexpr KeyEntry kKeys[] = {
    {"left", Key::Left},
    {"right", Key::Right},
    {"up", Key::Up},
    {"down", Key::Down},
    {"zoom_in", Key::ZoomIn},
    {"zoom_out", Key::ZoomOut},
    {"reset", Key::Reset},
    {"toggle_graticule", Key::ToggleGraticule},
    {"toggle_borders", Key::ToggleBorders},
    {"toggle_coastlines", Key::ToggleCoastlines},
    {"quit", Key::Quit},
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

} // namespace

const char* key_name(Key key) {
    for (const KeyEntry& e : kKeys) {
        if (e.key == key) {
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
            for (const KeyEntry& e : kKeys) {
                if (std::strcmp(tok[2], e.name) == 0) {
                    return Msg{KeyPress{t, e.key}};
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
        return Msg{Expose{}};
    }
    return std::nullopt;
}

std::size_t format_message(const Msg& msg, char* buf, std::size_t capacity) {
    if (buf == nullptr || capacity == 0) {
        return 0;
    }
    int len = 0;
    if (const auto* down = std::get_if<PointerDown>(&msg)) {
        len = std::snprintf(buf, capacity, "down %llu %.17g %.17g",
                            static_cast<unsigned long long>(down->time_ms), down->x, down->y);
    } else if (const auto* move = std::get_if<PointerMove>(&msg)) {
        len = std::snprintf(buf, capacity, "move %llu %.17g %.17g",
                            static_cast<unsigned long long>(move->time_ms), move->x, move->y);
    } else if (const auto* up = std::get_if<PointerUp>(&msg)) {
        len = std::snprintf(buf, capacity, "up %llu", static_cast<unsigned long long>(up->time_ms));
    } else if (const auto* wheel = std::get_if<Wheel>(&msg)) {
        len = std::snprintf(buf, capacity, "wheel %llu %d",
                            static_cast<unsigned long long>(wheel->time_ms), wheel->steps);
    } else if (const auto* key = std::get_if<KeyPress>(&msg)) {
        len = std::snprintf(buf, capacity, "key %llu %s",
                            static_cast<unsigned long long>(key->time_ms), key_name(key->key));
    } else if (const auto* resize = std::get_if<Resize>(&msg)) {
        len = std::snprintf(buf, capacity, "resize %d %d", resize->width, resize->height);
    } else {
        len = std::snprintf(buf, capacity, "expose");
    }
    if (len < 0 || static_cast<std::size_t>(len) >= capacity) {
        buf[0] = '\0';
        return 0;
    }
    return static_cast<std::size_t>(len);
}

} // namespace globe
