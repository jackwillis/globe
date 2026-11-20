// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "script.hpp"
#include "globe/shown_frame.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
namespace globe {
namespace {
bool space(char c) {
    return c == ' ' || c == '\t';
}
// Validate one Unicode scalar encoded in UTF-8. Overlong, surrogate and >U+10FFFF are errors.
std::size_t utf8(std::string_view s) {
    const auto c = static_cast<unsigned char>(s[0]);
    if (c < 128)
        return 1;
    unsigned n = c >= 0xc2 && c <= 0xdf   ? 2
                 : c >= 0xe0 && c <= 0xef ? 3
                 : c >= 0xf0 && c <= 0xf4 ? 4
                                          : 0;
    if (!n || s.size() < n)
        return 0;
    std::uint32_t scalar = c & (0x7fU >> n);
    for (unsigned i = 1; i < n; ++i) {
        const auto b = static_cast<unsigned char>(s[i]);
        if ((b & 0xc0) != 0x80)
            return 0;
        scalar = (scalar << 6) | (b & 63);
    }
    if ((n == 2 && scalar < 128) || (n == 3 && scalar < 2048) || (n == 4 && scalar < 65536) ||
        scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
        return 0;
    return n;
}
} // namespace
ScriptParse parse_script_line(std::string_view line, ScriptCommand& command) {
    if (line.size() > kScriptLine)
        return ScriptParse::Error;
    while (!line.empty() && space(line.front()))
        line.remove_prefix(1);
    if (line.empty() || line.front() == '#')
        return ScriptParse::Ignored;
    std::string_view tokens[4];
    unsigned count = 0;
    while (!line.empty()) {
        if (count == 4)
            return ScriptParse::Error;
        std::size_t n = 0;
        while (n < line.size() && !space(line[n])) {
            if (static_cast<unsigned char>(line[n]) < 32 ||
                static_cast<unsigned char>(line[n]) > 126)
                return ScriptParse::Error;
            ++n;
        }
        tokens[count++] = line.substr(0, n);
        line.remove_prefix(n);
        while (!line.empty() && space(line.front()))
            line.remove_prefix(1);
    }
    if (count == 2 && tokens[0] == "get" && tokens[1] == "borders") {
        command = {ScriptVerb::Get};
        return ScriptParse::Command;
    }
    if (count != 3)
        return ScriptParse::Error;
    if ((tokens[0] == "set" || tokens[0] == "assert") && tokens[1] == "borders" &&
        (tokens[2] == "true" || tokens[2] == "false")) {
        command = {tokens[0] == "set" ? ScriptVerb::Set : ScriptVerb::Assert, tokens[2] == "true"};
        return ScriptParse::Command;
    }
    if (tokens[0] == "wait" && tokens[1] == "settled") {
        std::uint32_t value = 0;
        for (char c : tokens[2]) {
            if (c < '0' || c > '9' || value > 60000 / 10)
                return ScriptParse::Error;
            value = value * 10 + static_cast<unsigned>(c - '0');
        }
        if (!value || value > 60000)
            return ScriptParse::Error;
        command = {ScriptVerb::Wait, false, value};
        return ScriptParse::Command;
    }
    return ScriptParse::Error;
}
void JsonRecord::literal(std::string_view s) {
    if (!valid_ || s.size() > sizeof bytes_ - size_) {
        valid_ = false;
        return;
    }
    std::memcpy(bytes_ + size_, s.data(), s.size());
    size_ += s.size();
}
void JsonRecord::string(std::string_view s) {
    literal("\"");
    while (!s.empty() && valid_) {
        const auto c = static_cast<unsigned char>(s.front());
        if (c < 32) {
            constexpr char digits[] = "0123456789abcdef";
            char escape[] = {'\\', 'u', '0', '0', digits[c >> 4], digits[c & 15]};
            literal({escape, 6});
            s.remove_prefix(1);
        } else if (c == '"' || c == '\\') {
            char escape[] = {'\\', static_cast<char>(c)};
            literal({escape, 2});
            s.remove_prefix(1);
        } else {
            const auto n = utf8(s);
            if (!n) {
                valid_ = false;
                return;
            }
            literal(s.substr(0, n));
            s.remove_prefix(n);
        }
    }
    literal("\"");
}
void JsonRecord::unsigned_number(std::uint64_t v) {
    char digits[20];
    unsigned n = 0;
    do {
        digits[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        literal({&digits[--n], 1});
}
bool JsonRecord::finish(ScriptWrite write, void* out) {
    literal("\n");
    return valid_ && (!write || write(out, bytes_, size_));
}
std::size_t script_macroman_utf8(std::string_view s, char* out, std::size_t capacity) {
    // Unicode mapping for Mac OS Roman; no locale or conversion heap is needed.
    static constexpr std::uint16_t high[128] = {
        0xc4,   0xc5,   0xc7,   0xc9,   0xd1,   0xd6,   0xdc,   0xe1,   0xe0,   0xe2,   0xe4,
        0xe3,   0xe5,   0xe7,   0xe9,   0xe8,   0xea,   0xeb,   0xed,   0xec,   0xee,   0xef,
        0xf1,   0xf3,   0xf2,   0xf4,   0xf6,   0xf5,   0xfa,   0xf9,   0xfb,   0xfc,   0x2020,
        0xb0,   0xa2,   0xa3,   0xa7,   0x2022, 0xb6,   0xdf,   0xae,   0xa9,   0x2122, 0xb4,
        0xa8,   0x2260, 0xc6,   0xd8,   0x221e, 0xb1,   0x2264, 0x2265, 0xa5,   0xb5,   0x2202,
        0x2211, 0x220f, 0x3c0,  0x222b, 0xaa,   0xba,   0x3a9,  0xe6,   0xf8,   0xbf,   0xa1,
        0xac,   0x221a, 0x192,  0x2248, 0x2206, 0xab,   0xbb,   0x2026, 0xa0,   0xc0,   0xc3,
        0xd5,   0x152,  0x153,  0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0xf7,   0x25ca,
        0xff,   0x178,  0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02, 0x2021, 0xb7,   0x201a,
        0x201e, 0x2030, 0xc2,   0xca,   0xc1,   0xcb,   0xc8,   0xcd,   0xce,   0xcf,   0xcc,
        0xd3,   0xd4,   0xf8ff, 0xd2,   0xda,   0xdb,   0xd9,   0x131,  0x2c6,  0x2dc,  0xaf,
        0x2d8,  0x2d9,  0x2da,  0xb8,   0x2dd,  0x2db,  0x2c7,
    };
    std::size_t used = 0;
    for (unsigned char c : s) {
        const std::uint32_t u = c < 128 ? c : high[c - 128];
        const unsigned n = u < 128 ? 1 : u < 2048 ? 2 : 3;
        if (n > capacity - used)
            return 0;
        if (n == 1)
            out[used++] = static_cast<char>(u);
        else {
            if (n == 3)
                out[used++] = static_cast<char>(0xe0 | (u >> 12));
            out[used++] =
                static_cast<char>((n == 2 ? 0xc0 : 0x80) | ((u >> 6) & (n == 2 ? 31 : 63)));
            out[used++] = static_cast<char>(0x80 | (u & 63));
        }
    }
    return used;
}
bool script_presentation_settled(const Model& model, ScriptPresentation p) {
    return p.foreground && frame_settled(model, p.job_active) && !model.refine_active &&
           !presentation_held(model) && !p.redraw && !p.dirty && !p.quick && !p.progress_bar &&
           !p.pending && !p.update_event;
}
const char* script_status_name(ScriptStatus s) {
    switch (s) {
    case ScriptStatus::Pass:
        return "pass";
    case ScriptStatus::Fail:
        return "fail";
    case ScriptStatus::Cancelled:
        return "cancelled";
    case ScriptStatus::Running:
        return "running";
    case ScriptStatus::Idle:
        return "idle";
    case ScriptStatus::Error:
        return "error";
    }
    return "error";
}
void ScriptRunner::emit(JsonRecord& record) {
    if (!report_failed_ && !record.finish(write_, destination_)) {
        report_failed_ = true;
        error_ = record.valid() ? "io-failure" : "report-overflow-or-encoding";
        ++errors_;
        status_ = ScriptStatus::Error;
        waiting_ = false;
    }
}
bool ScriptRunner::start(std::string_view source, const ScriptEnvironment& env, const Model& model,
                         ScriptWrite write, void* out) {
    if (active())
        return false;
    *this = ScriptRunner{};
    source_ = source;
    write_ = write;
    destination_ = out;
    status_ = ScriptStatus::Running;
    { // End the large run-record lifetime before error reporting constructs another record.
        JsonRecord r;
        r.literal("{\"type\":\"run\",\"schema\":\"pe-script/1\",\"app_version\":");
        r.string(env.version);
        r.literal(",\"app_build\":");
        r.string(env.build);
        r.literal(",\"os\":");
        r.string(env.os);
        r.literal(",\"platform\":");
        r.string(env.platform);
        r.literal(",\"environment\":");
        r.string(env.environment);
        r.literal(",\"script\":");
        r.string(env.script);
        r.literal(
            ",\"script_checksum\":null,\"checksum_status\":\"unavailable\",\"save_requested\":");
        r.boolean(env.save_requested);
        r.literal(",\"initial_borders\":");
        r.boolean(model.layers.borders);
        r.literal(",\"edition\":");
        r.string(env.edition);
        r.literal("}");
        emit(r);
    }
    if (active() && source.size() > kScriptBytes)
        error("resource-limit");
    return active();
}
void ScriptRunner::result(const char* type, const char* phase, const char* evidence, bool value,
                          bool changed) {
    JsonRecord r;
    r.literal("{\"type\":\"command\",\"seq\":");
    r.number(commands_);
    r.literal(",\"line\":");
    r.number(line_);
    r.literal(",\"input\":");
    r.string(input_);
    r.literal(",\"result\":{\"type\":");
    r.string(type);
    const std::string_view t = type;
    if (t == "value") {
        r.literal(",\"value\":");
        r.boolean(value);
    }
    if (t == "pass" || t == "fail") {
        r.literal(",\"observed\":");
        r.boolean(value);
    }
    if (phase) {
        r.literal(",\"phase\":");
        r.string(phase);
    }
    if (phase && std::string_view(phase) == "applied") {
        r.literal(",\"changed\":");
        r.boolean(changed);
    }
    if (phase && std::string_view(phase) == "completed")
        r.literal(",\"condition\":\"presentation-settled\"");
    if (t == "error") {
        r.literal(",\"code\":");
        r.string(error_);
    }
    r.literal("},\"evidence\":");
    r.string(evidence);
    r.literal("}");
    emit(r);
}
void ScriptRunner::end(ScriptStatus s) {
    waiting_ = false;
    status_ = s;
    JsonRecord r;
    r.literal("{\"type\":\"end\",\"status\":");
    r.string(script_status_name(s));
    r.literal(",\"commands\":");
    r.number(commands_);
    r.literal(",\"assertions_failed\":");
    r.number(failures_);
    r.literal(",\"errors\":");
    r.number(errors_);
    r.literal("}");
    emit(r);
}
void ScriptRunner::error(const char* code) {
    error_ = code;
    ++errors_;
    result("error", nullptr, "application");
    if (!report_failed_)
        end(ScriptStatus::Error);
}
void ScriptRunner::cancel() {
    if (!active())
        return;
    if (waiting_) {
        error_ = "cancelled";
        result("error", nullptr, "application");
    }
    if (!report_failed_)
        end(ScriptStatus::Cancelled);
}
std::uint64_t ScriptRunner::wake_at(std::uint64_t now, std::uint64_t application_wake) const {
    if (!active() || !waiting_)
        return application_wake;
    // Subtraction avoids overflow near the injected clock's limit. Never poll beyond timeout.
    const auto observation =
        now >= deadline_ ? deadline_ : now + std::min<std::uint64_t>(16, deadline_ - now);
    return application_wake ? std::min(application_wake, observation) : observation;
}
void ScriptRunner::step(const Model& model, ScriptObservation o,
                        void (*dispatch)(void*, const Msg&), void* app) {
    if (!active())
        return;
    if (waiting_) {
        if (o.generation != generation_ || o.revision != revision_ ||
            model.layers.borders != borders_)
            error("superseded");
        else if (o.now_ms >= deadline_)
            error("timeout");
        else if (o.presentation_settled) {
            waiting_ = false;
            result("ok", "completed", "application");
        }
        return;
    }
    if (offset_ == source_.size()) {
        end(ScriptStatus::Pass);
        return;
    }
    // At most one source line per turn, including comments and blanks.
    const auto start = offset_;
    while (offset_ < source_.size() && source_[offset_] != '\r' && source_[offset_] != '\n')
        ++offset_;
    auto line = source_.substr(start, offset_ - start);
    if (offset_ < source_.size()) {
        const char c = source_[offset_++];
        if (c == '\r' && offset_ < source_.size() && source_[offset_] == '\n')
            ++offset_;
    }
    ++line_;
    ScriptCommand command;
    const auto parsed = parse_script_line(line, command);
    if (parsed == ScriptParse::Ignored)
        return;
    ++commands_;
    // Invalid non-ASCII source cannot be misreported as UTF-8. Preserve its location and code.
    bool ascii = line.size() <= kScriptLine;
    for (unsigned char c : line)
        if (c > 127 || c == 0)
            ascii = false;
    input_ = ascii ? line : std::string_view("[invalid or oversized source line]");
    if (parsed == ScriptParse::Error) {
        error(line.size() > kScriptLine ? "resource-limit" : "malformed-input");
        return;
    }
    switch (command.verb) {
    case ScriptVerb::Get:
        result("value", nullptr, "model", model.layers.borders);
        break;
    case ScriptVerb::Set: {
        const bool changed = model.layers.borders != command.value;
        if (changed) {
            if (!dispatch) {
                error("unsupported-operation");
                return;
            }
            dispatch(app, CommandIssued{o.now_ms, Command::ToggleBorders});
        }
        if (model.layers.borders != command.value) {
            error("state-not-applied");
            return;
        }
        result("ok", "applied", "model", false, changed);
        break;
    }
    case ScriptVerb::Assert:
        if (model.layers.borders == command.value)
            result("pass", nullptr, "model", model.layers.borders);
        else {
            ++failures_;
            result("fail", nullptr, "model", model.layers.borders);
            if (!report_failed_)
                end(ScriptStatus::Fail);
        }
        break;
    case ScriptVerb::Wait:
        if (o.now_ms > std::numeric_limits<std::uint64_t>::max() - command.timeout_ms) {
            error("invalid-clock");
            return;
        }
        generation_ = o.generation;
        revision_ = o.revision;
        borders_ = model.layers.borders;
        deadline_ = o.now_ms + command.timeout_ms;
        waiting_ = true;
        break;
    }
}
} // namespace globe
