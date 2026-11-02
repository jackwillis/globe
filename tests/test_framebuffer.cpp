// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/framebuffer.hpp"

using namespace globe;

int main() {
    static_assert(rgb565(255, 255, 255) == 0xFFFF, "white");
    static_assert(rgb565(0, 0, 0) == 0x0000, "black");
    static_assert(rgb565(255, 0, 0) == 0xF800, "red");
    static_assert(rgb565(0, 255, 0) == 0x07E0, "green");
    static_assert(rgb565(0, 0, 255) == 0x001F, "blue");

    CHECK(!Framebuffer::create(0, 10));
    CHECK(!Framebuffer::create(10, 0));
    CHECK(!Framebuffer::create(-1, 10));
    CHECK(!Framebuffer::create(Framebuffer::kMaxDimension + 1, 10));
    CHECK(!Framebuffer::create(10, Framebuffer::kMaxDimension + 1));

    auto created = Framebuffer::create(8, 4);
    CHECK(created.has_value());
    if (!created) {
        return 1;
    }
    Framebuffer fb = std::move(*created);
    CHECK(fb.width() == 8 && fb.height() == 4);
    CHECK(fb.byte_size() == 8U * 4U * 2U);
    // Newly created buffers are zeroed rather than uninitialized.
    CHECK(fb.pixel(0, 0) == 0 && fb.pixel(7, 3) == 0);

    fb.clear(0x1234);
    CHECK(fb.pixel(0, 0) == 0x1234 && fb.pixel(7, 3) == 0x1234);

    fb.set_pixel(3, 2, 0xFFFF);
    CHECK(fb.pixel(3, 2) == 0xFFFF);
    CHECK(fb.data()[2 * fb.stride() + 3] == 0xFFFF);
    CHECK(fb.pixel(2, 2) == 0x1234);

    // Out-of-range drawing is ignored and reads return 0; nothing is written outside the buffer.
    fb.set_pixel(-1, 0, 1);
    fb.set_pixel(0, -1, 1);
    fb.set_pixel(8, 0, 1);
    fb.set_pixel(0, 4, 1);
    CHECK(fb.pixel(-1, 0) == 0 && fb.pixel(8, 0) == 0 && fb.pixel(0, 4) == 0);
    int changed = 0;
    for (int i = 0; i < 8 * 4; ++i) {
        changed += fb.data()[i] != 0x1234 ? 1 : 0;
    }
    CHECK(changed == 1); // only the (3,2) pixel

    fb.clear(0);
    fb.fill_span(1, 6, 2, 0xAAAA); // reversed order
    CHECK(fb.pixel(1, 1) == 0 && fb.pixel(2, 1) == 0xAAAA && fb.pixel(6, 1) == 0xAAAA &&
          fb.pixel(7, 1) == 0);
    fb.fill_span(2, -50, 100, 0xBBBB); // clipped to the whole row
    CHECK(fb.pixel(0, 2) == 0xBBBB && fb.pixel(7, 2) == 0xBBBB);
    fb.fill_span(-1, 0, 7, 0xCCCC);
    fb.fill_span(4, 0, 7, 0xCCCC);
    fb.fill_span(0, 20, 30, 0xCCCC);   // entirely right of the buffer
    fb.fill_span(0, -30, -20, 0xCCCC); // entirely left of the buffer
    CHECK(fb.pixel(0, 0) == 0 && fb.pixel(7, 0) == 0 && fb.pixel(0, 3) == 0);

    return TEST_MAIN_RESULT();
}
