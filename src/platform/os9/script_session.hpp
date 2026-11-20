// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "script.hpp"
#include <Memory.h>
namespace globe {
// No allocation or filesystem access until the user chooses Run Script.
class ScriptSession {
  public:
    ~ScriptSession();
    void choose(const Model&);
    void finish();
    ScriptRunner runner;

  private:
    static bool write(void*, const char*, std::size_t);
    void release();
    Ptr source_ = nullptr;
    short output_ = -1;
    bool saving_ = false, disk_error_ = false;
};
} // namespace globe
