// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/app.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
namespace globe {
constexpr std::size_t kScriptBytes = 16384, kScriptLine = 200, kJsonRecord = 4096;
enum class ScriptVerb : std::uint8_t { Get, Set, Wait, Assert };
enum class ScriptParse : std::uint8_t { Command, Ignored, Error };
struct ScriptCommand {
    ScriptVerb verb = ScriptVerb::Get;
    bool value = false;
    std::uint32_t timeout_ms = 0;
};
ScriptParse parse_script_line(std::string_view, ScriptCommand&);
// Complete-record sink: false latches I/O failure. No allocations in serialization.
using ScriptWrite = bool (*)(void*, const char*, std::size_t);
class JsonRecord {
  public:
    void literal(std::string_view);
    void string(std::string_view); // strict UTF-8, including control escaping
    // V1 emits unsigned integer JSON numbers only. Floating-point inputs (including NaN/Inf)
    // are rejected at compile time, so libc float formatting/locale is not linked into Classic.
    template <class T> void number(T value) {
        static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
        unsigned_number(value);
    }
    void boolean(bool b) { literal(b ? "true" : "false"); }
    bool finish(ScriptWrite, void*); // appends LF; overflow/encoding errors never reach sink
    bool valid() const { return valid_; }

  private:
    void unsigned_number(std::uint64_t);
    char bytes_[kJsonRecord];
    std::size_t size_ = 0;
    bool valid_ = true;
};
// Converts a Classic MacRoman filename to UTF-8. Returns 0 on insufficient capacity.
std::size_t script_macroman_utf8(std::string_view, char*, std::size_t);
struct ScriptEnvironment {
    std::string_view version, build, os, script, edition;
    std::string_view platform = "unavailable", environment = "unavailable";
    bool save_requested = false;
};
struct ScriptObservation {
    std::uint64_t now_ms = 0, generation = 0, revision = 0;
    bool presentation_settled = false;
};
// Native pending work is supplied by the adapter; an empty render queue alone is insufficient.
struct ScriptPresentation {
    bool foreground = false, job_active = false, redraw = false, dirty = false;
    bool quick = false, progress_bar = false, pending = false, update_event = false;
};
bool script_presentation_settled(const Model&, ScriptPresentation);
enum class ScriptStatus : std::uint8_t { Idle, Running, Pass, Fail, Error, Cancelled };
const char* script_status_name(ScriptStatus);
// Borrowed source must remain unchanged/alive until terminal status. Call step once per turn.
// dispatch must synchronously route the Msg through the application's normal apply/Effects path.
class ScriptRunner {
  public:
    bool start(std::string_view source, const ScriptEnvironment&, const Model&, ScriptWrite, void*);
    void step(const Model&, ScriptObservation, void (*dispatch)(void*, const Msg&), void*);
    void cancel();
    bool active() const { return status_ == ScriptStatus::Running; }
    bool waiting() const { return waiting_; }
    ScriptStatus status() const { return status_; }
    unsigned commands() const { return commands_; }
    unsigned assertions_failed() const { return failures_; }
    unsigned errors() const { return errors_; }
    bool report_failed() const { return report_failed_; }
    const char* error_code() const { return error_; }
    std::uint64_t deadline() const { return deadline_; }
    // While waiting, request a completion observation every 16 ms (rounded by native waits).
    // Returns the earlier application wake, observation wake or timeout; no wake when idle.
    std::uint64_t wake_at(std::uint64_t now_ms, std::uint64_t application_wake) const;

  private:
    void emit(JsonRecord&);
    void result(const char*, const char*, const char*, bool = false, bool = false);
    void end(ScriptStatus);
    void error(const char*);
    std::string_view source_, input_;
    std::size_t offset_ = 0;
    unsigned line_ = 0, commands_ = 0, failures_ = 0, errors_ = 0;
    ScriptStatus status_ = ScriptStatus::Idle;
    bool waiting_ = false, report_failed_ = false;
    std::uint64_t deadline_ = 0, generation_ = 0, revision_ = 0;
    bool borders_ = false;
    const char* error_ = "none";
    ScriptWrite write_ = nullptr;
    void* destination_ = nullptr;
};
} // namespace globe
