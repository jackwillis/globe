// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace globe {
// Small ASCII UI strings only; caller owns fixed output and receives false on overflow.
bool script_filename(std::string_view macroman_name, std::uint32_t mac_seconds, char*, std::size_t);
bool script_os_version(std::uint32_t system_bcd, char*, std::size_t);
bool script_progress_title(unsigned commands, char*, std::size_t);
struct ScriptSummary {
    const char* status;
    unsigned commands, assertions_failed, errors;
    const char* code;
    bool saving, report_error;
};
bool script_summary(ScriptSummary, char*, std::size_t);
} // namespace globe
