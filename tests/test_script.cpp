// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "classic_present.hpp"
#include "classic_wait.hpp"
#include "script.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <string>
static unsigned allocations = 0;
void* operator new(std::size_t n) {
    ++allocations;
    if (auto* p = std::malloc(n ? n : 1))
        return p;
    std::abort();
}
void* operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete[](void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}
using namespace globe;
struct Sink {
    char data[32768]{};
    std::size_t size = 0;
    unsigned records = 0, fail_at = 0;
    static bool write(void* p, const char* s, std::size_t n) {
        auto& out = *static_cast<Sink*>(p);
        if (++out.records == out.fail_at || n > sizeof out.data - out.size - 1)
            return false;
        CHECK(n && s[n - 1] == '\n');
        std::memcpy(out.data + out.size, s, n);
        out.size += n;
        out.data[out.size] = 0;
        return true;
    }
    bool has(const char* s) const { return std::strstr(data, s) != nullptr; }
};
static ScriptEnvironment env{"test-version",
                             "test-build",
                             "test-host",
                             "test\"\\\nscript",
                             "test-edition",
                             "linux-host-test",
                             "simulated observations",
                             true};
static void dispatch(void* p, const Msg& msg) {
    update(*static_cast<Model*>(p), msg);
}
static void tick(ScriptRunner& r, Model& m, std::uint64_t time = 1, bool settled = false,
                 std::uint64_t rev = 0) {
    r.step(m, {time, m.generation, rev, settled}, dispatch, &m);
}
int main(int argc, char** argv) {
    ScriptCommand c;
    for (const auto* s :
         {"get borders", "set borders false", "set borders true", "assert borders false",
          "wait settled 10000", "\t get\tborders  ", "wait settled 60000"})
        CHECK(parse_script_line(s, c) == ScriptParse::Command);
    for (const auto* s : {"", " \t ", " # comment"})
        CHECK(parse_script_line(s, c) == ScriptParse::Ignored);
    for (const auto* s : {"assert borderz false", "get borders extra", "set borders FALSE", "get",
                          "wait settled -1", "wait settled 0", "wait settled 60001",
                          "wait settled 999999999999999999999", "wait settled 1.2",
                          "wait settled 1x", "what", "get borders\r", "set borders false #no"})
        CHECK(parse_script_line(s, c) == ScriptParse::Error);
    CHECK(parse_script_line(std::string(201, ' '), c) == ScriptParse::Error);
    CHECK(parse_script_line(std::string_view("get borders\0", 12), c) == ScriptParse::Error);
    auto settled_model = initial_model(64, 64);
    settled_model.stage = QualityStage::Refine;
    settled_model.refine_active = false;
    CHECK(script_presentation_settled(settled_model, {true}));
    CHECK(!script_presentation_settled(settled_model, {}));
    for (unsigned flag = 0; flag < 7; ++flag) {
        ScriptPresentation p{true};
        switch (flag) {
        case 0:
            p.job_active = true;
            break;
        case 1:
            p.redraw = true;
            break;
        case 2:
            p.dirty = true;
            break;
        case 3:
            p.quick = true;
            break;
        case 4:
            p.progress_bar = true;
            break;
        case 5:
            p.pending = true;
            break;
        case 6:
            p.update_event = true;
            break;
        }
        CHECK(!script_presentation_settled(settled_model, p));
    }
    settled_model.refine_active = true;
    CHECK(!script_presentation_settled(settled_model, {true}));
    settled_model.refine_active = false;
    settled_model.dragging = true;
    settled_model.drag_active = true;
    CHECK(!script_presentation_settled(settled_model, {true}));
    settled_model.dragging = false;
    settled_model.stage = QualityStage::Moving;
    CHECK(!script_presentation_settled(settled_model, {true}));
    Model m = initial_model(64, 64);
    m.layers.borders = true;
    Sink sink;
    ScriptRunner r;
    unsigned before = allocations;
    CHECK(r.start(
        "# comment\rget borders\r\nset borders false\nwait settled 10000\rassert borders false",
        env, m, Sink::write, &sink));
    tick(r, m);
    CHECK(r.commands() == 0);
    tick(r, m);
    CHECK(sink.has("\"value\":true"));
    tick(r, m);
    CHECK(!m.layers.borders);
    CHECK(sink.has("\"phase\":\"applied\",\"changed\":true"));
    tick(r, m, 2);
    CHECK(r.waiting());
    tick(r, m, 3);
    CHECK(r.waiting());
    CHECK(r.commands() == 3);
    tick(r, m, 4, true);
    CHECK(!r.waiting());
    tick(r, m);
    tick(r, m);
    CHECK(r.status() == ScriptStatus::Pass && r.commands() == 4 && r.errors() == 0);
    CHECK(sink.has("\"seq\":1,\"line\":2"));
    CHECK(sink.has("\"seq\":4,\"line\":5"));
    CHECK(sink.has("\"condition\":\"presentation-settled\""));
    CHECK(sink.has("\"observed\":false"));
    CHECK(allocations == before);
    if (argc > 2) {
        std::string path = std::string(argv[2]) + "/script-test.jsonl";
        auto* file = std::fopen(path.c_str(), "wb");
        CHECK(file);
        CHECK(std::fwrite(sink.data, 1, sink.size, file) == sink.size);
        CHECK(!std::fclose(file));
    }
    sink = {};
    r.start("set borders false\nget borders", env, m, Sink::write, &sink);
    tick(r, m);
    CHECK(sink.has("\"changed\":false"));
    m.layers.borders = true;
    tick(r, m);
    CHECK(sink.has("\"value\":true"));
    tick(r, m);
    sink = {};
    r.start("assert borders false\nset borders false", env, m, Sink::write, &sink);
    tick(r, m);
    CHECK(r.status() == ScriptStatus::Fail && r.commands() == 1 && r.assertions_failed() == 1 &&
          m.layers.borders);
    sink = {};
    r.start("assert borderz false\nset borders false", env, m, Sink::write, &sink);
    tick(r, m);
    CHECK(r.status() == ScriptStatus::Error && r.commands() == 1 && m.layers.borders);
    CHECK(sink.has("malformed-input"));
    CHECK(sink.has("\"type\":\"end\""));
    for (unsigned mode = 0; mode < 4; ++mode) {
        sink = {};
        r.start("wait settled 10\nset borders false", env, m, Sink::write, &sink);
        tick(r, m, 100);
        CHECK(r.waiting() && r.deadline() == 110);
        if (mode == 0) {
            tick(r, m, 110, true);
            CHECK(sink.has("timeout"));
        }
        if (mode == 1) {
            ++m.generation;
            tick(r, m, 101, true);
            CHECK(sink.has("superseded"));
        }
        if (mode == 2) {
            tick(r, m, 101, true, 1);
            CHECK(sink.has("superseded"));
        }
        if (mode == 3) {
            r.cancel();
            CHECK(r.status() == ScriptStatus::Cancelled);
            CHECK(sink.has("cancelled"));
        }
        CHECK(!r.active() && r.commands() == 1 && m.layers.borders);
    }
    sink = {};
    r.start("wait settled 10", env, m, Sink::write, &sink);
    tick(r, m, 100);
    m.layers.borders = false;
    tick(r, m, 101, true);
    CHECK(sink.has("superseded"));
    sink = {};
    r.start("get borders", env, m, Sink::write, &sink);
    r.step(m, {1, m.generation, 0, false}, nullptr, nullptr);
    tick(r, m);
    CHECK(r.status() == ScriptStatus::Pass);
    sink = {};
    r.start("set borders true", env, m, Sink::write, &sink);
    r.step(m, {1, m.generation, 0, false}, nullptr, nullptr);
    CHECK(sink.has("unsupported-operation"));
    std::string huge(kScriptBytes + 1, ' ');
    sink = {};
    const auto oversized_allocations = allocations;
    CHECK(!r.start(huge, env, m, Sink::write, &sink));
    CHECK(allocations == oversized_allocations);
    CHECK(r.status() == ScriptStatus::Error && r.errors() == 1 && r.commands() == 0);
    CHECK(sink.records == 3);
    CHECK(std::strncmp(sink.data, "{\"type\":\"run\"", 13) == 0);
    CHECK(sink.has("\"seq\":0,\"line\":0"));
    CHECK(sink.has("\"code\":\"resource-limit\""));
    CHECK(sink.has("{\"type\":\"end\",\"status\":\"error\",\"commands\":0,\"assertions_failed\":0,"
                   "\"errors\":1}"));
    // Failure at each of run/error/end must latch I/O failure and never produce a valid end.
    for (unsigned fail = 1; fail <= 3; ++fail) {
        sink = {};
        sink.fail_at = fail;
        CHECK(!r.start(huge, env, m, Sink::write, &sink));
        CHECK(r.report_failed() && r.status() == ScriptStatus::Error);
        CHECK(r.errors() == (fail == 1 ? 1U : 2U) && r.commands() == 0);
        CHECK(std::string_view(r.error_code()) == "io-failure");
        CHECK(sink.records == fail && !sink.has("\"type\":\"end\""));
        CHECK((fail == 3) == sink.has("resource-limit"));
    }
    sink = {};
    r.start(std::string_view("get borders\0", 12), env, m, Sink::write, &sink);
    tick(r, m);
    CHECK(r.status() == ScriptStatus::Error);
    // A refused record ends execution. No subsequent valid end can turn a partial file into pass.
    for (unsigned fail = 1; fail <= 3; ++fail) {
        sink = {};
        sink.fail_at = fail;
        r.start("get borders", env, m, Sink::write, &sink);
        tick(r, m);
        tick(r, m);
        CHECK(r.report_failed() && r.status() == ScriptStatus::Error && r.errors() == 1);
        CHECK(!sink.has("\"type\":\"end\""));
    }
    // Regression: simulate the real outer loop with no mouse/key events and no render work.
    // The previous policy slept until deadline and timed out before its next settled observation.
    const auto idle_wait = [&](bool old_policy, bool initially_settled) {
        ScriptRunner idle;
        CHECK(idle.start("wait settled 10000", env, m, nullptr, nullptr));
        Model view = m;
        view.stage = QualityStage::Refine;
        view.refine_active = false;
        view.dragging = false;
        ClassicPresentation presentation;
        if (!initially_settled) {
            presentation.request(view.generation);
            CHECK(presentation.begin(view.generation, 65));
        }
        std::uint64_t now = 100;
        tick(idle, view, now);
        unsigned observations = 0;
        while (idle.active() && observations < 10) {
            const auto wake = old_policy ? idle.deadline() : idle.wake_at(now, 0);
            const auto ticks = classic_wait_ticks(now, wake, false);
            CHECK(ticks > 0); // waiting alone must yield, never spin with zero sleep
            now += (static_cast<std::uint64_t>(ticks) * 1000 + 59) / 60;
            // Native submission of 65 rows needs three 32-row bands; no completion until all
            // are acknowledged. This exercises the same queue/predicate as the Classic adapter.
            if (presentation.active(view.generation))
                presentation.submitted(view.generation);
            ScriptPresentation state{true};
            state.pending = presentation.pending(view.generation);
            const bool settled = script_presentation_settled(view, state);
            tick(idle, view, now, settled);
            if (state.pending && !old_policy)
                CHECK(idle.waiting());
            ++observations;
        }
        CHECK(!idle.active());
        CHECK(observations <= (initially_settled || old_policy ? 2U : 4U));
        return idle.status();
    };
    CHECK(idle_wait(false, true) == ScriptStatus::Pass);
    CHECK(idle_wait(false, false) == ScriptStatus::Pass); // pending work finishes on a later turn
    CHECK(idle_wait(true, true) == ScriptStatus::Error);  // falsifies reviewed-head policy
    sink = {};
    r.start("wait settled 10", env, m, Sink::write, &sink);
    tick(r, m, 100);
    CHECK(r.wake_at(100, 0) == 110);   // short timeout wins over observation interval
    CHECK(r.wake_at(100, 105) == 105); // preserve earlier real application work
    CHECK(r.wake_at(110, 0) == 110);
    tick(r, m, 110, true);
    CHECK(r.status() == ScriptStatus::Error && sink.has("timeout"));
    CHECK(r.wake_at(110, 0) == 0); // terminal runner stops observation wakes
    // Disabled output still runs and summarizes without retaining history.
    CHECK(r.start("get borders\nset borders true\nassert borders true", env, m, nullptr, nullptr));
    for (unsigned i = 0; i < 4; ++i)
        tick(r, m);
    CHECK(r.status() == ScriptStatus::Pass && r.commands() == 3 && !r.report_failed());
    auto huge_env = env;
    std::string huge_name(kJsonRecord, 'n');
    huge_env.script = huge_name;
    sink = {};
    CHECK(!r.start("get borders", huge_env, m, Sink::write, &sink));
    CHECK(r.report_failed() && r.status() == ScriptStatus::Error && sink.records == 0);
    CHECK(std::string_view(r.error_code()) == "report-overflow-or-encoding");
    sink = {};
    before = allocations;
    JsonRecord json;
    json.literal("{\"s\":");
    json.string("\"\\\b\f\n\r\t\x01 café");
    json.literal(",\"n\":");
    json.number(std::numeric_limits<std::uint64_t>::max());
    json.literal("}");
    CHECK(json.finish(Sink::write, &sink));
    CHECK(allocations == before);
    CHECK(sink.has("\\u0001"));
    CHECK(sink.has("18446744073709551615"));
    CHECK(sink.has("café"));
    for (const auto* bad : {"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80"}) {
        JsonRecord j;
        j.string(bad);
        CHECK(!j.finish(Sink::write, &sink));
    }
    std::string oversized(kJsonRecord, 'x');
    JsonRecord overflow;
    overflow.string(oversized);
    const auto records = sink.records;
    CHECK(!overflow.finish(Sink::write, &sink));
    CHECK(records == sink.records);
    char name[20];
    const auto n = script_macroman_utf8("caf\x8e", name, sizeof name);
    CHECK(std::string_view(name, n) == "café");
    CHECK(!script_macroman_utf8("caf\x8e", name, 4));
    return TEST_MAIN_RESULT();
}
