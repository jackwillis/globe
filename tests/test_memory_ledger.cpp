// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/memory_ledger.hpp"

#include <cstdlib>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace {
std::uint64_t allocation_calls = 0;
void* allocate(std::size_t n) {
    ++allocation_calls;
    auto* p = std::malloc(n == 0 ? 1 : n);
    if (p == nullptr)
        std::abort();
    return p;
}
const globe::MemoryCounter& counter(const globe::MemorySnapshot& s, globe::MemoryCategory c) {
    return s.categories[static_cast<std::size_t>(c)];
}
} // namespace

// Independent allocation hooks observe the whole tested operation sequence, including ticket
// moves/destruction and snapshot copies. The counter is exercised explicitly before measurement.
void* operator new(std::size_t n) {
    return allocate(n);
}
void* operator new[](std::size_t n) {
    return allocate(n);
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

int main() {
    using namespace globe;
    static_assert(!std::is_copy_constructible_v<MemoryLedger>);
    static_assert(!std::is_move_constructible_v<MemoryLedger>);
    static_assert(!std::is_copy_constructible_v<MemoryCharge>);
    static_assert(std::is_nothrow_move_constructible_v<MemoryCharge>);
    static_assert(std::is_nothrow_move_assignable_v<MemoryCharge>);
    static_assert(noexcept(std::declval<MemoryLedger&>().snapshot()));
    constexpr auto framebuffer = MemoryCategory::Framebuffer;
    constexpr auto terrain = MemoryCategory::TerrainCache;
    constexpr auto line = MemoryCategory::LineWorkspace;
    auto* probe = ::operator new(8);
    ::operator delete(probe);
    CHECK(allocation_calls > 0);
    const auto before = allocation_calls;

    {
        MemoryLedger ledger;
        auto state = ledger.snapshot();
        CHECK(state.total_current == 0 && state.total_peak == 0 && !state.accounting_error);
        CHECK(ledger.acquired(framebuffer, 100));
        CHECK(ledger.released(framebuffer, 100));
        CHECK(ledger.acquired(terrain, 200));
        state = ledger.snapshot();
        CHECK(counter(state, framebuffer).peak == 100);
        CHECK(counter(state, terrain).peak == 200);
        CHECK(state.total_peak == 200); // simultaneous peak, not sum of category peaks (300)
        CHECK(ledger.acquired(framebuffer, 75));
        CHECK(ledger.snapshot().total_peak == 275);
        CHECK(ledger.released(terrain, 200));
        ledger.reset_peaks();
        state = ledger.snapshot();
        CHECK(state.total_current == 75 && state.total_peak == 75);
        CHECK(counter(state, terrain).peak == 0);
        CHECK(counter(state, framebuffer).peak == 75);
        CHECK(counter(state, framebuffer).successful_allocations == 2);
        CHECK(ledger.released(framebuffer, 75));
        CHECK(ledger.snapshot().total_current == 0);
    }
    {
        MemoryLedger ledger;
        // A partially constructed resource has one successful allocation followed by a failure.
        {
            MemoryCharge pixels(ledger, framebuffer, 640);
            CHECK(pixels && pixels.bytes() == 640);
            CHECK(ledger.allocation_failed(framebuffer));
            CHECK(counter(ledger.snapshot(), framebuffer).failed_allocations == 1);
        }
        CHECK(ledger.snapshot().total_current == 0);
        CHECK(ledger.snapshot().total_peak == 640);
        CHECK(counter(ledger.snapshot(), framebuffer).successful_allocations == 1);
    }
    {
        MemoryLedger ledger;
        CHECK(ledger.acquired(line, 0));
        CHECK(ledger.released(line, 0));
        {
            MemoryCharge zero(ledger, line, 0);
            CHECK(zero && zero.bytes() == 0);
        }
        CHECK(ledger.snapshot().total_peak == 0);
        CHECK(counter(ledger.snapshot(), line).successful_allocations == 2);
        CHECK(!ledger.snapshot().accounting_error);
    }
    {
        MemoryLedger ledger;
        MemoryCharge a(ledger, framebuffer, 100);
        MemoryCharge b(std::move(a));
        CHECK(!a && a.bytes() == 0 && b && b.bytes() == 100);
        CHECK(counter(ledger.snapshot(), framebuffer).successful_allocations == 1);
        MemoryCharge c(ledger, terrain, 200);
        CHECK(ledger.snapshot().total_current == 300);
        c = std::move(b); // release old terrain charge, then transfer framebuffer accounting
        CHECK(!b && c && c.bytes() == 100);
        CHECK(ledger.snapshot().total_current == 100);
        CHECK(counter(ledger.snapshot(), terrain).current == 0);
        auto* same_ticket = &c;
        c = std::move(*same_ticket); // self-move through the public assignment API
        CHECK(c && ledger.snapshot().total_current == 100);
        c.release();
        c.release(); // idempotent; no underflow and no second release
        CHECK(!c && c.bytes() == 0);
        CHECK(ledger.snapshot().total_current == 0 && !ledger.snapshot().accounting_error);
    }
    {
        MemoryLedger left, right;
        MemoryCharge a(left, framebuffer, 10);
        MemoryCharge b(right, terrain, 20);
        b = std::move(a);
        CHECK(right.snapshot().total_current == 0);
        CHECK(left.snapshot().total_current == 10);
        b.release();
        CHECK(left.snapshot().total_current == 0);
    }
    {
        MemoryLedger ledger;
        CHECK(!ledger.released(framebuffer, 1));
        CHECK(ledger.snapshot().accounting_error);
        CHECK(ledger.snapshot().total_current == 0 && ledger.snapshot().total_peak == 0);
        CHECK(ledger.acquired(framebuffer, 5));
        CHECK(!ledger.released(framebuffer, 6));
        CHECK(
            !ledger.released(terrain, 1)); // wrong category cannot borrow another category's bytes
        CHECK(ledger.snapshot().total_current == 5);
        ledger.reset_peaks();
        CHECK(ledger.snapshot().accounting_error); // resets cannot hide invalid accounting
        CHECK(ledger.released(framebuffer, 5));
    }
    {
        MemoryLedger ledger;
        const auto invalid = static_cast<MemoryCategory>(255);
        CHECK(!ledger.acquired(MemoryCategory::Count, 1));
        CHECK(!ledger.released(invalid, 0));
        CHECK(!ledger.allocation_failed(invalid));
        CHECK(ledger.snapshot().accounting_error);
        CHECK(ledger.snapshot().total_current == 0);
        MemoryCharge rejected(ledger, invalid, 1);
        CHECK(!rejected && rejected.bytes() == 0);
    }
    {
        MemoryLedger ledger;
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        CHECK(ledger.acquired(framebuffer, maximum));
        const auto original = ledger.snapshot();
        CHECK(!ledger.acquired(framebuffer, 1)); // category payload overflow
        CHECK(!ledger.acquired(terrain, 1));     // total payload overflow across categories
        MemoryCharge rejected(ledger, terrain, 1);
        CHECK(!rejected);
        const auto state = ledger.snapshot();
        CHECK(state.accounting_error && state.total_current == maximum &&
              state.total_peak == maximum);
        CHECK(counter(state, framebuffer).successful_allocations ==
              counter(original, framebuffer).successful_allocations);
        CHECK(counter(state, terrain).successful_allocations == 0);
        CHECK(counter(state, terrain).current == 0);
        CHECK(ledger.released(framebuffer, maximum));
        CHECK(ledger.snapshot().total_current == 0);
    }
    {
        // A separate small reference model exercises interleaved categories and peak windows.
        MemoryLedger ledger;
        std::array<std::uint64_t, kMemoryCategoryCount> live{}, peaks{}, successes{}, failures{};
        std::uint64_t total_peak = 0;
        std::uint32_t random = 0x7183U;
        for (unsigned turn = 0; turn < 10000; ++turn) {
            random = random * 1664525U + 1013904223U;
            const auto index = (random >> 8) % kMemoryCategoryCount;
            const auto category = static_cast<MemoryCategory>(index);
            const auto bytes = static_cast<std::uint64_t>(random % 127U);
            switch ((random >> 24) % 4) {
            case 0:
                CHECK(ledger.acquired(category, bytes));
                live[index] += bytes;
                ++successes[index];
                break;
            case 1: {
                const auto released = bytes > live[index] ? live[index] : bytes;
                CHECK(ledger.released(category, released));
                live[index] -= released;
                break;
            }
            case 2:
                CHECK(ledger.allocation_failed(category));
                ++failures[index];
                break;
            default:
                ledger.reset_peaks();
                peaks = live;
                total_peak = 0;
                for (auto value : live)
                    total_peak += value;
                break;
            }
            std::uint64_t total = 0;
            for (std::size_t i = 0; i < live.size(); ++i) {
                total += live[i];
                if (live[i] > peaks[i])
                    peaks[i] = live[i];
            }
            if (total > total_peak)
                total_peak = total;
            const auto state = ledger.snapshot();
            CHECK(!state.accounting_error);
            CHECK(state.total_current == total && state.total_peak == total_peak);
            for (std::size_t i = 0; i < live.size(); ++i) {
                CHECK(state.categories[i].current == live[i]);
                CHECK(state.categories[i].peak == peaks[i]);
                CHECK(state.categories[i].successful_allocations == successes[i]);
                CHECK(state.categories[i].failed_allocations == failures[i]);
            }
        }
    }
    CHECK(allocation_calls == before);
    std::printf("memory_ledger: no allocation calls during tested operations; sizeof ledger=%zu, "
                "charge=%zu, pointer=%zu\n",
                sizeof(MemoryLedger), sizeof(MemoryCharge), sizeof(void*));
    return TEST_MAIN_RESULT();
}
