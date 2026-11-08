// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/framebuffer.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

namespace {
struct Allocation {
    void* pointer = nullptr;
    std::size_t bytes = 0;
};
std::array<Allocation, 16> live_allocations{};
bool tracking = false;
unsigned attempts = 0, fail_attempt = 0;
std::uint64_t actual_live = 0, actual_peak = 0;

void* allocate(std::size_t bytes) noexcept {
    if (tracking && ++attempts == fail_attempt)
        return nullptr;
    void* pointer = std::malloc(bytes == 0 ? 1 : bytes);
    if (pointer == nullptr || !tracking)
        return pointer;
    for (auto& allocation : live_allocations) {
        if (allocation.pointer == nullptr) {
            allocation = {pointer, bytes};
            actual_live += bytes;
            if (actual_live > actual_peak)
                actual_peak = actual_live;
            return pointer;
        }
    }
    std::abort(); // fixture capacity exceeded; do not lose allocation evidence
}
void release(void* pointer) noexcept {
    if (pointer == nullptr)
        return;
    for (auto& allocation : live_allocations) {
        if (allocation.pointer == pointer) {
            actual_live -= allocation.bytes;
            allocation = {};
            break;
        }
    }
    std::free(pointer);
}
void begin(unsigned fail = 0) {
    if (actual_live != 0)
        std::abort();
    attempts = 0;
    actual_peak = 0;
    fail_attempt = fail;
    tracking = true;
}
const globe::MemoryCounter& category(const globe::MemorySnapshot& s, globe::MemoryCategory c) {
    return s.categories[static_cast<std::size_t>(c)];
}
void paint(globe::Framebuffer& fb) {
    fb.clear(42);
    fb.coverage(globe::Framebuffer::Coverage::Mark);
    fb.fill_span(1, 2, 8, 123);
    fb.set_pixel(4, 2, 321);
    fb.coverage(globe::Framebuffer::Coverage::Uncovered);
    fb.fill_span(1, 0, 16, 77);
    fb.set_pixel(4, 2, 555); // marked pixel must survive
    fb.coverage(globe::Framebuffer::Coverage::Off);
    fb.clip_rows(2, 4);
    fb.fill_span(3, -1, 50, 456);
    fb.clip_rows(0, fb.height());
}
} // namespace

void* operator new(std::size_t n) {
    void* pointer = allocate(n);
    if (pointer == nullptr)
        std::abort();
    return pointer;
}
void* operator new[](std::size_t n) {
    return ::operator new(n);
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
    return allocate(n);
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
    return allocate(n);
}
void operator delete(void* p) noexcept {
    release(p);
}
void operator delete[](void* p) noexcept {
    release(p);
}
void operator delete(void* p, std::size_t) noexcept {
    release(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    release(p);
}
void operator delete(void* p, const std::nothrow_t&) noexcept {
    release(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept {
    release(p);
}

int main() {
    using namespace globe;
    constexpr int width = 17, height = 9;
    constexpr std::uint64_t pixels = width * height * sizeof(Rgb565);
    constexpr std::uint64_t coverage = ((width + 7) / 8) * height;
    constexpr auto primary = MemoryCategory::Framebuffer;
    constexpr auto lines = MemoryCategory::LineWorkspace;
    auto uninstrumented = Framebuffer::create(width, height);
    CHECK(uninstrumented.has_value());
    paint(*uninstrumented);
    {
        MemoryLedger ledger;
        begin();
        {
            auto fb = Framebuffer::create(width, height, &ledger);
            CHECK(fb.has_value());
            CHECK(attempts == 2 && actual_live == pixels + coverage);
            const auto state = ledger.snapshot();
            CHECK(state.total_current == actual_live && state.total_peak == actual_peak);
            CHECK(category(state, primary).successful_allocations == 2);
            CHECK(category(state, primary).failed_allocations == 0);
            paint(*fb);
            CHECK(std::memcmp(fb->data(), uninstrumented->data(), fb->byte_size()) == 0);
            for (int i = 0; i < 10000; ++i) {
                fb->set_pixel(i % width, i % height, static_cast<Rgb565>(i));
                fb->fill_span(i % height, 1, width - 2, 37);
                (void)ledger.snapshot();
            }
            CHECK(attempts == 2); // actual drawing plus measurement adds no allocations
        }
        CHECK(actual_live == 0 && ledger.snapshot().total_current == 0);
        CHECK(!ledger.snapshot().accounting_error);
        tracking = false;
    }
    for (unsigned fail : {1U, 2U}) {
        MemoryLedger ledger;
        begin(fail);
        auto fb = Framebuffer::create(width, height, &ledger);
        CHECK(!fb);
        const auto state = ledger.snapshot();
        CHECK(attempts == fail);
        CHECK(actual_live == 0 && state.total_current == 0);
        CHECK(state.total_peak == actual_peak);
        CHECK(state.total_peak == (fail == 1 ? 0 : pixels));
        CHECK(category(state, primary).successful_allocations == fail - 1);
        CHECK(category(state, primary).failed_allocations == 1);
        CHECK(!state.accounting_error);
        tracking = false;
    }
    for (unsigned fail : {1U, 2U}) {
        begin(fail);
        CHECK(!Framebuffer::create(width,
                                   height)); // legacy callers still fail safely without a ledger
        CHECK(attempts == fail && actual_live == 0);
        tracking = false;
    }
    {
        MemoryLedger ledger;
        begin();
        CHECK(!Framebuffer::create(0, height, &ledger));
        CHECK(!Framebuffer::create(width, Framebuffer::kMaxDimension + 1, &ledger));
        CHECK(attempts == 0 && actual_live == 0);
        CHECK(category(ledger.snapshot(), primary).failed_allocations == 0);
        tracking = false;
    }
    {
        MemoryLedger ledger;
        begin();
        {
            auto primary_fb = Framebuffer::create(width, height, &ledger);
            auto line_fb = Framebuffer::create(3, 2, &ledger, lines);
            CHECK(primary_fb && line_fb);
            const auto peak = ledger.snapshot().total_peak;
            CHECK(peak == actual_peak);
            CHECK(category(ledger.snapshot(), lines).current ==
                  line_fb->byte_size() + line_fb->coverage_byte_size());
            Framebuffer moved(std::move(*primary_fb));
            CHECK(attempts == 4 && ledger.snapshot().total_current == actual_live);
            CHECK(category(ledger.snapshot(), primary).successful_allocations == 2);
            *line_fb = std::move(moved); // old line pixels/coverage freed; primary charge transfers
            CHECK(category(ledger.snapshot(), lines).current == 0);
            CHECK(ledger.snapshot().total_current == actual_live);
            CHECK(ledger.snapshot().total_current == pixels + coverage);
            CHECK(ledger.snapshot().total_peak == peak);
            ledger.reset_peaks();
            CHECK(ledger.snapshot().total_peak == pixels + coverage);
        }
        CHECK(actual_live == 0 && ledger.snapshot().total_current == 0);
        CHECK(!ledger.snapshot().accounting_error);
        tracking = false;
    }
    {
        MemoryLedger ledger;
        begin();
        auto retained = Framebuffer::create(width, height, &ledger);
        CHECK(retained);
        paint(*retained);
        fail_attempt = attempts + 2; // old usable buffer coexists with a partially built candidate
        auto candidate = Framebuffer::create(width, height, &ledger);
        CHECK(!candidate);
        CHECK(actual_live == pixels + coverage);
        CHECK(ledger.snapshot().total_current == actual_live);
        CHECK(ledger.snapshot().total_peak == actual_peak);
        CHECK(actual_peak == 2 * pixels + coverage);
        CHECK(category(ledger.snapshot(), primary).successful_allocations == 3);
        CHECK(category(ledger.snapshot(), primary).failed_allocations == 1);
        CHECK(std::memcmp(retained->data(), uninstrumented->data(), retained->byte_size()) == 0);
        retained.reset();
        CHECK(actual_live == 0 && ledger.snapshot().total_current == 0);
        tracking = false;
    }
    {
        MemoryLedger left, right;
        begin();
        {
            auto a = Framebuffer::create(width, height, &left);
            auto b = Framebuffer::create(3, 2, &right, lines);
            CHECK(a && b);
            *b = std::move(*a);
            CHECK(right.snapshot().total_current == 0);
            CHECK(left.snapshot().total_current == actual_live);
            CHECK(actual_live == pixels + coverage);
        }
        CHECK(actual_live == 0 && left.snapshot().total_current == 0);
        CHECK(!left.snapshot().accounting_error && !right.snapshot().accounting_error);
        tracking = false;
    }
    {
        MemoryLedger ledger;
        begin();
        auto fb = Framebuffer::create(width, height, &ledger, MemoryCategory::Count);
        CHECK(fb.has_value()); // accounting failure must not change resource allocation success
        CHECK(ledger.snapshot().accounting_error);
        CHECK(ledger.snapshot().total_current == 0);
        fb.reset();
        CHECK(actual_live == 0);
        tracking = false;
    }
    std::printf("framebuffer ledger: exact allocation payload, failure cleanup, moves, unchanged "
                "pixels; sizeof framebuffer=%zu\n",
                sizeof(Framebuffer));
    return TEST_MAIN_RESULT();
}
