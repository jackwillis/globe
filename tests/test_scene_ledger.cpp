// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/render.hpp"

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
    // Build source bytes and all comparison buffers outside the measured allocation window.
    const auto bytes =
        atlas_util::make_atlas({{1, {{30000, 16000}, {35000, 18000}, {40000, 16000}}}}, "test");
    const auto other_bytes = atlas_util::make_atlas(
        {{1, {{1000, 16000}, {2000, 17000}}}, {2, {{3000, 15000}, {4000, 16000}}}}, "other");
    const auto other_atlas = Atlas::parse(other_bytes.data(), other_bytes.size());
    CHECK(other_atlas);
    const auto atlas = Atlas::parse(bytes.data(), bytes.size());
    CHECK(atlas);
    const auto reference = Scene::create(*atlas);
    auto expected = Framebuffer::create(80, 60);
    auto actual = Framebuffer::create(80, 60);
    CHECK(reference && expected && actual);
    const auto payload = reference->bytes();
    const auto caps_bytes = atlas->polyline_count() * sizeof(PolylineCap);
    const auto vectors_bytes = payload - caps_bytes;
    const auto camera_result = make_camera({{0, 0}, 0, 25, 80, 60});
    CHECK(camera_result);
    const auto camera = *camera_result;
    render_globe(*expected, camera, *reference, Palette{}, Layers{});
    constexpr auto scene_category = MemoryCategory::Scene;
    {
        MemoryLedger ledger;
        begin();
        {
            auto scene = Scene::create(*atlas, &ledger);
            CHECK(scene && attempts == 2);
            CHECK(actual_live == payload && actual_peak == payload);
            CHECK(ledger.snapshot().total_current == actual_live);
            CHECK(category(ledger.snapshot(), scene_category).successful_allocations == 2);
            for (unsigned i = 0; i < 10; ++i) {
                render_globe(*actual, camera, *scene, Palette{}, Layers{});
                (void)ledger.snapshot();
            }
            CHECK(std::memcmp(actual->data(), expected->data(), actual->byte_size()) == 0);
            RefinementJob job(camera, *scene, Palette{}, Layers{}, RenderQuality{}, 1);
            const auto after_start = attempts;
            while (job.active())
                job.step(*actual, 1, 7);
            CHECK(attempts == after_start && after_start == 2);
            CHECK(std::memcmp(actual->data(), expected->data(), actual->byte_size()) == 0);
            RefinementJob cancelled(camera, *scene, Palette{}, Layers{}, RenderQuality{}, 2);
            cancelled.step(*actual, 2, 1);
            cancelled.cancel();
            CHECK(!cancelled.active() && attempts == 2);
            CHECK(ledger.snapshot().total_current == payload); // cancellation doesn't own scene
        }
        CHECK(actual_live == 0 && ledger.snapshot().total_current == 0);
        CHECK(ledger.snapshot().total_peak == actual_peak && !ledger.snapshot().accounting_error);
        tracking = false;
    }
    for (unsigned fail : {1U, 2U}) {
        MemoryLedger ledger;
        begin(fail);
        CHECK(!Scene::create(*atlas, &ledger));
        const auto state = ledger.snapshot();
        CHECK(attempts == fail && actual_live == 0 && state.total_current == 0);
        CHECK(state.total_peak == actual_peak);
        CHECK(actual_peak == (fail == 1 ? 0 : vectors_bytes));
        CHECK(category(state, scene_category).successful_allocations == fail - 1);
        CHECK(category(state, scene_category).failed_allocations == 1);
        CHECK(!state.accounting_error);
        tracking = false;
        begin(fail);
        CHECK(!Scene::create(*atlas));
        CHECK(attempts == fail && actual_live == 0);
        tracking = false;
    }
    {
        MemoryLedger ledger;
        begin();
        {
            auto retained = Scene::create(*atlas, &ledger);
            CHECK(retained);
            fail_attempt = attempts + 2;
            CHECK(!Scene::create(*atlas, &ledger));
            CHECK(ledger.snapshot().total_current == actual_live && actual_live == payload);
            CHECK(ledger.snapshot().total_peak == actual_peak &&
                  actual_peak == payload + vectors_bytes);
            render_globe(*actual, camera, *retained, Palette{}, Layers{});
            CHECK(std::memcmp(actual->data(), expected->data(), actual->byte_size()) == 0);
            fail_attempt = 0;
            auto replacement = Scene::create(*atlas, &ledger);
            CHECK(replacement && actual_live == 2 * payload);
            Scene moved(std::move(*replacement));
            const auto before = attempts;
            *retained = std::move(moved);
            auto* self = &*retained;
            *retained = std::move(*self);
            CHECK(attempts == before && actual_live == payload);
            CHECK(ledger.snapshot().total_current == actual_live);
            CHECK(ledger.snapshot().total_peak == actual_peak && actual_peak == 2 * payload);
            CHECK(category(ledger.snapshot(), scene_category).successful_allocations == 5);
            ledger.reset_peaks();
            CHECK(ledger.snapshot().total_peak == payload);
        }
        CHECK(actual_live == 0 && ledger.snapshot().total_current == 0);
        CHECK(!ledger.snapshot().accounting_error);
        tracking = false;
    }
    {
        MemoryLedger left, right;
        begin();
        {
            auto a = Scene::create(*atlas, &left);
            auto b = Scene::create(*other_atlas, &right);
            CHECK(a && b);
            const auto* original_vertices = a->vertices();
            *b = std::move(*a);
            CHECK(b->vertices() == original_vertices);
            CHECK(b->atlas().point_count() == atlas->point_count());
            CHECK(b->atlas().polyline_count() == atlas->polyline_count());
            render_globe(*actual, camera, *b, Palette{}, Layers{});
            CHECK(std::memcmp(actual->data(), expected->data(), actual->byte_size()) == 0);
            CHECK(left.snapshot().total_current == actual_live && actual_live == payload);
            CHECK(right.snapshot().total_current == 0);
        }
        CHECK(actual_live == 0 && left.snapshot().total_current == 0);
        CHECK(!left.snapshot().accounting_error && !right.snapshot().accounting_error);
        tracking = false;
    }
    std::printf("scene ledger: payload=%zu, vectors=%zu, caps=%zu; failures, replacement, moves, "
                "pixels and allocation-free render/steps checked; sizeof scene=%zu\n",
                payload, vectors_bytes, caps_bytes, sizeof(Scene));
    return TEST_MAIN_RESULT();
}
