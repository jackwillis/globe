// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "script_session.hpp"
#include "globe/version.hpp"
#include "script_output.hpp"
#include "script_text.hpp"
#include <Dialogs.h>
#include <Files.h>
#include <Gestalt.h>
#include <StandardFile.h>
#include <cstring>
#ifndef GLOBE_SCRIPT_BUILD
#define GLOBE_SCRIPT_BUILD "unavailable"
#endif
namespace globe {
namespace {
void pascal_text(unsigned char* out, const char* text) {
    const auto n = std::strlen(text);
    out[0] = static_cast<unsigned char>(n > 255 ? 255 : n);
    std::memcpy(out + 1, text, out[0]);
}
void message(const char* text) {
    unsigned char p[256];
    pascal_text(p, text);
    const unsigned char empty[] = {0};
    ParamText(p, empty, empty, empty);
    NoteAlert(132, nullptr);
}
} // namespace
ScriptSession::~ScriptSession() {
    if (runner.active())
        runner.cancel();
    release();
}
void ScriptSession::release() {
    const auto result = script_close_output(
        output_, output_volume_, [](void*, short file) { return FSClose(file) == noErr; },
        [](void*, short volume) { return FlushVol(nullptr, volume) == noErr; }, nullptr);
    if (result.close_failed || result.flush_failed) {
        if (!disk_error_)
            disk_error_code_ = result.close_failed ? "close-failure" : "flush-failure";
        disk_error_ = true;
    }
    if (source_) {
        DisposePtr(source_);
        source_ = nullptr;
    }
}
bool ScriptSession::write(void* context, const char* bytes, std::size_t count) {
    auto& s = *static_cast<ScriptSession*>(context);
    long actual = static_cast<long>(count);
    const auto err = FSWrite(s.output_, &actual, bytes);
    if (err != noErr || actual != static_cast<long>(count)) {
        s.disk_error_ = true;
        s.disk_error_code_ = "write-failure";
    }
    return !s.disk_error_;
}
void ScriptSession::choose(const Model& model) {
    if (runner.active())
        return;
    StandardFileReply selected{};
    StandardGetFile(nullptr, -1, nullptr, &selected);
    if (!selected.sfGood)
        return;
    if (selected.sfScript != 0) { // Roman script code; other filename encodings need an adapter.
        message("Unsupported script filename encoding. V1 supports MacRoman filenames. Nothing was "
                "run.");
        return;
    }
    unsigned char name[256];
    std::memcpy(name, selected.sfFile.name, static_cast<std::size_t>(selected.sfFile.name[0]) + 1);
    const unsigned char empty[] = {0};
    ParamText(name, empty, empty, empty);
    // Three explicit choices: Cancel, Run, Run and Save. All modal UI precedes execution.
    const auto choice = CautionAlert(130, nullptr);
    if (choice != 1 && choice != 3)
        return;
    saving_ = choice == 3;
    disk_error_ = false;
    disk_error_code_ = "none";
    short input = -1;
    long length = 0;
    auto err = FSpOpenDF(&selected.sfFile, fsRdPerm, &input);
    if (err == noErr)
        err = GetEOF(input, &length);
    if (err == noErr && (length < 0 || length > static_cast<long>(kScriptBytes)))
        err = paramErr;
    if (err == noErr) {
        source_ = NewPtr(static_cast<Size>(length ? length : 1));
        if (!source_)
            err = memFullErr;
    }
    long actual = length;
    if (err == noErr && length) {
        err = FSRead(input, &actual, source_);
        if (actual != length)
            err = eofErr;
    }
    if (input != -1 && FSClose(input) != noErr)
        err = ioErr;
    if (err != noErr) {
        release();
        message("Cannot read script (limit: 16 KiB). Nothing was run.");
        return;
    }
    if (saving_) {
        // HFS: <=31 bytes. Fixed ASCII stem plus 8 hex Mac-epoch seconds, no locale dependence.
        // The chosen dialog name/destination is authoritative; creation is exclusive below.
        unsigned long stamp = 0;
        GetDateTime(&stamp);
        char suggestion[32];
        script_filename({reinterpret_cast<const char*>(name + 1), name[0]}, stamp, suggestion,
                        sizeof suggestion);
        unsigned char proposed[32], prompt[64];
        pascal_text(proposed, suggestion);
        pascal_text(prompt, "Save script output as:");
        StandardFileReply destination{};
        StandardPutFile(prompt, proposed, &destination);
        if (!destination.sfGood) {
            release();
            return;
        }
        // Never truncate/overwrite: even a confirmed replacement is refused in v1. Also protects
        // the source script and distribution files. User can choose another name on a new run.
        err = FSpCreate(&destination.sfFile, 'PdlE', 'TEXT', destination.sfScript);
        if (err != noErr) {
            release();
            message(err == dupFNErr ? "Output already exists. Nothing was overwritten or run. "
                                      "Choose a different filename."
                                    : "Cannot create output. Choose a new name on a writable disk. "
                                      "Nothing was run.");
            return;
        }
        output_volume_ = destination.sfFile.vRefNum;
        err = FSpOpenDF(&destination.sfFile, fsWrPerm, &output_);
        if (err != noErr) {
            output_ = -1;
            release();
            message("Cannot open output. Nothing was run; the empty file may remain.");
            return;
        }
    }
    char utf8_name[768];
    const auto name_size = script_macroman_utf8({reinterpret_cast<const char*>(name + 1), name[0]},
                                                utf8_name, sizeof utf8_name);
    long system = 0;
    char os[48];
    if (Gestalt(gestaltSystemVersion, &system) == noErr)
        script_os_version(static_cast<std::uint32_t>(system), os, sizeof os);
    else
        std::strcpy(os, "Mac OS version unavailable");
    const ScriptEnvironment env{kVersion,
                                GLOBE_SCRIPT_BUILD,
                                os,
                                {utf8_name, name_size},
                                "embedded floppy; edition identifier unavailable",
                                "classic-mac-powerpc",
                                "normal application; native/emulated host unavailable",
                                saving_};
    runner.start({source_, static_cast<std::size_t>(length)}, env, model, saving_ ? write : nullptr,
                 this);
    if (!runner.active())
        finish();
}
void ScriptSession::finish() {
    if (!source_)
        return;
    release(); // include close errors in the UI's saved claim
    char summary[256];
    const bool report_error = disk_error_ || runner.report_failed();
    const unsigned errors = runner.errors() + (disk_error_ && !runner.report_failed() ? 1U : 0U);
    script_summary({report_error ? "error" : script_status_name(runner.status()), runner.commands(),
                    runner.assertions_failed(), errors,
                    disk_error_ ? disk_error_code_ : runner.error_code(), saving_, report_error},
                   summary, sizeof summary);
    message(summary);
}
} // namespace globe
