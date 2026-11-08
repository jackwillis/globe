// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Standalone host allocation probe. Not linked into Globe or its build configuration.
#include "globe/atlas.hpp"
#include "globe/countries.hpp"
#include "globe/elevation.hpp"
#include "globe/geometry.hpp"
#include "globe/landclasses.hpp"
#include "globe/places.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <new>
#include <vector>

namespace {
bool tracking = false;
std::size_t live = 0, peak = 0, allocations = 0;
struct alignas(std::max_align_t) Header {
    std::size_t size;
    bool counted;
};
void* allocate(std::size_t size) noexcept {
    auto* header = static_cast<Header*>(std::malloc(sizeof(Header) + size));
    if (!header)
        return nullptr;
    *header = {size, tracking};
    if (tracking) {
        live += size;
        peak = std::max(peak, live);
        ++allocations;
    }
    return header + 1;
}
void release(void* p) noexcept {
    if (!p)
        return;
    auto* header = static_cast<Header*>(p) - 1;
    if (header->counted)
        live -= header->size;
    std::free(header);
}
void begin() {
    if (live)
        std::abort();
    peak = allocations = 0;
    tracking = true;
}
void result(const char* layer, std::size_t reported = 0) {
    tracking = false;
    std::printf("{\"layer\":\"%s\",\"heap_payload_live_bytes\":%zu,\"heap_payload_peak_bytes\":%zu,"
                "\"allocations\":%zu,\"runtime_reported_bytes\":%zu}\n",
                layer, live, peak, allocations, reported);
}
std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        std::abort();
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
} // namespace
void* operator new(std::size_t n) {
    auto* p = allocate(n);
    if (!p)
        std::abort();
    return p;
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
    std::printf("{\"sizeof_Vec3\":%zu,\"sizeof_CellBands\":%zu,\"sizeof_PolylineCap\":%zu,\"sizeof_"
                "TerrainScene\":%zu,\"sizeof_pointer\":%zu}\n",
                sizeof(globe::Vec3), sizeof(globe::CellBands), sizeof(globe::PolylineCap),
                sizeof(globe::TerrainScene), sizeof(void*));
    const auto atlas = read("data/atlas.gatl"), countries = read("data/countries.gpol"),
               places = read("data/cities.gplc"), classes = read("data/landclasses-1deg.ggrd");
    {
        begin();
        auto data = globe::Atlas::parse(atlas.data(), atlas.size());
        if (!data)
            return 1;
        auto scene = globe::Scene::create(*data);
        if (!scene)
            return 1;
        result("atlas_scene", scene->bytes());
    }
    {
        begin();
        auto data = globe::Countries::parse(countries.data(), countries.size());
        if (!data)
            return 1;
        auto scene = globe::CountryScene::create(*data);
        if (!scene)
            return 1;
        result("country_scene");
    }
    {
        begin();
        auto data = globe::Places::parse(places.data(), places.size());
        if (!data)
            return 1;
        auto scene = globe::PlaceScene::create(*data);
        if (!scene)
            return 1;
        result("place_scene");
    }
    {
        begin();
        auto data = globe::LandClasses::decode(classes.data(), classes.size());
        if (!data)
            return 1;
        result("landclasses", data->bytes());
    }
    for (const auto* path : {"data/elevation-1deg.gelv", "data/elevation-0.5deg.gelv"}) {
        auto bytes = read(path);
        begin();
        auto data = globe::Elevation::parse(bytes.data(), bytes.size());
        if (!data)
            return 1;
        auto scene = globe::TerrainScene::create(*data);
        if (!scene)
            return 1;
        result(path, scene->bytes());
    }
    if (live)
        return 2;
}
