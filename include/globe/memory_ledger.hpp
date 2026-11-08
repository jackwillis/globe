// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace globe {

// Requested dynamic allocation payload only. Inline/static storage, allocator overhead and
// process RSS have different meanings and must be reported separately. Single event-loop owner.
enum class MemoryCategory : std::uint8_t {
    Framebuffer,
    Scene,
    LineWorkspace,
    TerrainCache,
    ClassCache,
    OwnedPack,
    JobWorkspace,
    Count
};
constexpr std::size_t kMemoryCategoryCount = static_cast<std::size_t>(MemoryCategory::Count);

struct MemoryCounter {
    std::uint64_t current = 0;
    std::uint64_t peak = 0;
    std::uint64_t successful_allocations = 0;
    std::uint64_t failed_allocations = 0;
};
struct MemorySnapshot {
    std::array<MemoryCounter, kMemoryCategoryCount> categories{};
    std::uint64_t total_current = 0;
    std::uint64_t total_peak = 0;
    bool accounting_error = false;
};

class MemoryLedger {
  public:
    MemoryLedger() = default;
    // Charges borrow this ledger; its address and lifetime must remain stable.
    MemoryLedger(const MemoryLedger&) = delete;
    MemoryLedger& operator=(const MemoryLedger&) = delete;
    MemoryLedger(MemoryLedger&&) = delete;
    MemoryLedger& operator=(MemoryLedger&&) = delete;

    // Report only a successful allocation, never a reservation or attempted allocation.
    // On an invalid category/overflow, preserve all numeric fields and flag the measurement.
    // Successful zero-byte allocations count as events without increasing payload.
    bool acquired(MemoryCategory category, std::uint64_t bytes) noexcept {
        auto* counter = get(category);
        if (counter == nullptr)
            return false;
        constexpr auto max = std::numeric_limits<std::uint64_t>::max();
        if (bytes > max - counter->current || bytes > max - state_.total_current ||
            counter->successful_allocations == max)
            return error();
        counter->current += bytes;
        ++counter->successful_allocations;
        if (counter->current > counter->peak)
            counter->peak = counter->current;
        state_.total_current += bytes;
        if (state_.total_current > state_.total_peak)
            state_.total_peak = state_.total_current;
        return true;
    }

    // Size/category must match the owning allocation. A zero-byte release is a valid no-op.
    bool released(MemoryCategory category, std::uint64_t bytes) noexcept {
        auto* counter = get(category);
        if (counter == nullptr)
            return false;
        if (bytes > counter->current || bytes > state_.total_current)
            return error();
        counter->current -= bytes;
        state_.total_current -= bytes;
        return true;
    }

    bool allocation_failed(MemoryCategory category) noexcept {
        auto* counter = get(category);
        if (counter == nullptr)
            return false;
        if (counter->failed_allocations == std::numeric_limits<std::uint64_t>::max())
            return error();
        ++counter->failed_allocations;
        return true;
    }

    MemorySnapshot snapshot() const noexcept { return state_; }
    // Start a new peak window with existing live payload as the baseline. Event counts and
    // sticky accounting errors survive, so resetting a peak cannot hide a bad measurement.
    void reset_peaks() noexcept {
        for (auto& counter : state_.categories)
            counter.peak = counter.current;
        state_.total_peak = state_.total_current;
    }

  private:
    bool error() noexcept {
        state_.accounting_error = true;
        return false;
    }
    MemoryCounter* get(MemoryCategory category) noexcept {
        const auto index = static_cast<std::size_t>(category);
        if (index >= kMemoryCategoryCount) {
            error();
            return nullptr;
        }
        return &state_.categories[index];
    }
    MemorySnapshot state_{};
};

// Accounting ownership, not allocation ownership: this ticket never allocates or frees payload.
// Construct it only after the corresponding allocation succeeds. Destroy/release at the same
// point that payload is freed. The ledger must outlive every ticket. A failed accounting charge
// leaves the ticket empty and flags the ledger; it never changes an allocation's success/failure.
class MemoryCharge {
  public:
    MemoryCharge() = default;
    MemoryCharge(MemoryLedger& ledger, MemoryCategory category, std::uint64_t bytes) noexcept {
        if (ledger.acquired(category, bytes)) {
            ledger_ = &ledger;
            category_ = category;
            bytes_ = bytes;
        }
    }
    MemoryCharge(const MemoryCharge&) = delete;
    MemoryCharge& operator=(const MemoryCharge&) = delete;
    MemoryCharge(MemoryCharge&& other) noexcept { take(other); }
    MemoryCharge& operator=(MemoryCharge&& other) noexcept {
        if (this != &other) {
            release();
            take(other);
        }
        return *this;
    }
    ~MemoryCharge() { release(); }

    explicit operator bool() const noexcept { return ledger_ != nullptr; }
    std::uint64_t bytes() const noexcept { return bytes_; }
    void release() noexcept {
        if (ledger_ != nullptr) {
            ledger_->released(category_, bytes_);
            ledger_ = nullptr;
            bytes_ = 0;
        }
    }

  private:
    void take(MemoryCharge& other) noexcept {
        ledger_ = std::exchange(other.ledger_, nullptr);
        category_ = other.category_;
        bytes_ = std::exchange(other.bytes_, 0);
    }
    MemoryLedger* ledger_ = nullptr;
    MemoryCategory category_ = MemoryCategory::Framebuffer;
    std::uint64_t bytes_ = 0;
};

} // namespace globe
