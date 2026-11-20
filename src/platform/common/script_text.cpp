// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "script_text.hpp"
namespace globe {
namespace {
class Text {
  public:
    Text(char* output, std::size_t capacity) : out_(output), capacity_(capacity) {
        if (capacity_)
            out_[0] = 0;
    }
    void append(std::string_view text) {
        for (char c : text) {
            if (size_ + 1 >= capacity_) {
                ok_ = false;
                return;
            }
            out_[size_++] = c;
            out_[size_] = 0;
        }
    }
    void number(std::uint32_t value, unsigned radix = 10, unsigned width = 1) {
        char digits[10];
        unsigned n = 0;
        do {
            digits[n++] = "0123456789abcdef"[value % radix];
            value /= radix;
        } while (value);
        while (width > n) {
            append("0");
            --width;
        }
        while (n)
            append({&digits[--n], 1});
    }
    bool ok() const { return ok_; }

  private:
    char* out_;
    std::size_t capacity_, size_ = 0;
    bool ok_ = true;
};
} // namespace
bool script_filename(std::string_view name, std::uint32_t stamp, char* out, std::size_t capacity) {
    Text text(out, capacity);
    unsigned n = 0;
    for (unsigned char c : name) {
        if (n == 10 || c == '.')
            break;
        const char safe =
            (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'
                ? static_cast<char>(c)
                : '_';
        text.append({&safe, 1});
        ++n;
    }
    if (!n)
        text.append("script");
    text.append("-");
    text.number(stamp, 16, 8);
    text.append(".jsonl");
    return text.ok();
}
bool script_os_version(std::uint32_t system, char* out, std::size_t capacity) {
    Text text(out, capacity);
    text.append("Mac OS ");
    text.number(((system >> 12) & 15) * 10 + ((system >> 8) & 15));
    text.append(".");
    text.number((system >> 4) & 15);
    text.append(".");
    text.number(system & 15);
    return text.ok();
}
bool script_progress_title(unsigned commands, char* out, std::size_t capacity) {
    Text text(out, capacity);
    text.append("Poodle Earth - Script command ");
    text.number(commands);
    text.append(" (Esc cancels)");
    return text.ok();
}
bool script_summary(ScriptSummary s, char* out, std::size_t capacity) {
    Text text(out, capacity);
    text.append("Script ");
    text.append(s.status);
    text.append(". Commands: ");
    text.number(s.commands);
    text.append("; assertions failed: ");
    text.number(s.assertions_failed);
    text.append("; errors: ");
    text.number(s.errors);
    text.append(". ");
    text.append(s.code);
    text.append(". ");
    text.append(s.saving
                    ? (s.report_error ? "Transcript NOT saved successfully" : "Transcript saved")
                    : "Output saving disabled");
    return text.ok();
}
} // namespace globe
