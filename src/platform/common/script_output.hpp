// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
namespace globe {
struct ScriptCloseResult {
    bool close_failed = false, flush_failed = false;
};
// A close is attempted exactly once, then that explicitly chosen volume is flushed, even if
// close failed. Invalidate the reference first: subsequent cleanup must never close it again.
inline ScriptCloseResult script_close_output(short& reference, short volume,
                                             bool (*close)(void*, short),
                                             bool (*flush)(void*, short), void* context) {
    if (reference == -1)
        return {};
    const short file = reference;
    reference = -1;
    const bool closed = close(context, file);
    const bool flushed = flush(context, volume);
    return {!closed, !flushed};
}
} // namespace globe
