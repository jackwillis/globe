// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Compile-time feature probe for the Retro68 toolchain. It must compile without exceptions or
// RTTI. Each section is guarded by a feature-test macro so a failure names the missing feature
// instead of producing a wall of errors; tools/probe-retro68.sh prints which sections were built.
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#if defined(__has_include)
#if __has_include(<variant>)
#include <variant>
#define PROBE_HAS_VARIANT 1
#endif
#if __has_include(<span>)
#include <span>
#define PROBE_HAS_SPAN 1
#endif
#endif

// Report values, readable from the compiler's #pragma message output.
#define PROBE_STR2(x) #x
#define PROBE_STR(x) PROBE_STR2(x)
#pragma message("probe: __cplusplus=" PROBE_STR(__cplusplus))
#pragma message("probe: __GNUC__=" PROBE_STR(__GNUC__) "." PROBE_STR(__GNUC_MINOR__))
#ifdef PROBE_HAS_VARIANT
#pragma message("probe: <variant> available")
#else
#pragma message("probe: <variant> MISSING")
#endif
#ifdef PROBE_HAS_SPAN
#pragma message("probe: <span> available")
#else
#pragma message("probe: <span> MISSING")
#endif
#ifdef __cpp_exceptions
#pragma message("probe: exceptions ENABLED")
#else
#pragma message("probe: exceptions disabled")
#endif
#ifdef __cpp_rtti
#pragma message("probe: rtti ENABLED")
#else
#pragma message("probe: rtti disabled")
#endif

namespace {

// constexpr tables and palette generation (design doc section 8).
constexpr std::array<std::uint16_t, 8> make_gray_ramp() {
    std::array<std::uint16_t, 8> t{};
    for (std::size_t i = 0; i < t.size(); ++i) {
        const auto v = static_cast<std::uint8_t>(i * 255 / (t.size() - 1));
        t[i] = static_cast<std::uint16_t>(((v & 0xF8U) << 8) | ((v & 0xFCU) << 3) | (v >> 3));
    }
    return t;
}
constexpr auto kRamp = make_gray_ramp();
static_assert(kRamp[0] == 0x0000 && kRamp[7] == 0xFFFF, "constexpr table generation");

// std::optional without exceptions.
std::optional<int> half(int v) {
    if (v % 2 != 0) {
        return std::nullopt;
    }
    return v / 2;
}

#ifdef PROBE_HAS_VARIANT
struct MsgTick {
    std::uint32_t ms;
};
struct MsgKey {
    int code;
};
using Msg = std::variant<MsgTick, MsgKey>;

int dispatch(const Msg& m) {
    if (const auto* tick = std::get_if<MsgTick>(&m)) {
        return static_cast<int>(tick->ms);
    }
    return std::get<MsgKey>(m).code; // never throws: only reached when the alternative is MsgKey
}
#endif

#ifdef PROBE_HAS_SPAN
int sum(std::span<const int> v) {
    int s = 0;
    for (const int x : v) {
        s += x;
    }
    return s;
}
#endif

struct Pod {
    int a;
    int b;
};
static_assert(std::is_trivially_copyable_v<Pod>, "type traits");
static_assert(std::is_same_v<decltype(std::make_pair(1, 2)), std::pair<int, int>>, "utility");

} // namespace

int main() {
    int result = half(8).value_or(-1);
#ifdef PROBE_HAS_VARIANT
    result += dispatch(MsgTick{1}) + dispatch(MsgKey{2});
#endif
#ifdef PROBE_HAS_SPAN
    const int values[] = {1, 2, 3};
    result += sum(values);
#endif
    return result == 0 ? 1 : 0;
}
