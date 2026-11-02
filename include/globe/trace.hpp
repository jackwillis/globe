// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// One-line text form of a Msg, for recorded sessions and scripted benchmark input.
//
//   down TIME X Y | move TIME X Y | up TIME | wheel TIME STEPS | key TIME NAME
//   resize W H | expose
//
// Blank lines and lines starting with '#' are comments. Numbers are written with enough digits to
// round-trip exactly. The legacy `key` verb represents CommandIssued messages.
#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "globe/app.hpp"

namespace globe {

// Parses one line. Returns nullopt for comments, blank lines and anything malformed (unknown
// verb, missing or extra fields, non-finite or out-of-range numbers). Lines longer than 200
// bytes are rejected.
std::optional<Msg> parse_message(std::string_view line);

// Writes the line form (without newline) and a terminating NUL into buf. Returns the length, or
// 0 if capacity is too small.
std::size_t format_message(const Msg& msg, char* buf, std::size_t capacity);

const char* command_name(Command key);

} // namespace globe
