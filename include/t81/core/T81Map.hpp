/**
 * @file T81Map.hpp
 * @brief Defines the T81Map class, a ternary-optimized associative map.
 *
 * This file provides the T81Map<K, V> class, a hash map designed for high
 * performance in the T81 ecosystem. It is particularly optimized for
 * `T81Symbol` keys, for which it can use a perfect hash function. For other
 * key types, it falls back to a generic hash. The map uses a power-of-3
 * growth strategy and ternary-inspired probing to ensure good load
 * distribution and cache performance.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <compare>
#include <type_traits>
#include <functional>
#include <stdexcept>

namespace t81 {

// ======================================================================
// T81Map<K,V> – Map with optimized path for T81Symbol keys
// ======================================================================
template <typename K, typename V>
class T81Map {
    // Internal bucket: one key + one value, plus occupancy bit
    struct Bucket {
        K    key{};
        V    value{};
        bool occupied = false;
        auto operator<=>(const Bucket&) const noexcept = default;
    };

    std::vector<Bucket> buckets_;
    std::size_t size_ = 0;  // number of occupied entries

    // Load factor: 0.729 ≈ 3⁻¹ (heuristic target)
    static constexpr double MAX_LOAD_FACTOR = 0.729;

    // ------------------------------------------------------------------
    // Hash helpers
    // ------------------------------------------------------------------
    [[nodiscard]] static std::size_t symbol_hash(const T81Symbol& sym,
                                                 std::size_t bucket_count) noexcept {
        if (bucket_count == 0) return 0;
        std::uint64_t h = sym.hash();      // already well-distributed
        return static_cast<std::size_t>(h % bucket_count);
    }

    template <typename T>
    [[nodiscard]] static std::size_t generic_hash(const T& key,
                                                  std::size_t bucket_count) noexcept {
        if (bucket_count == 0) return 0;
        std::hash<T> hasher;
        std::uint64_t h = static_cast<std::uint64_t>(hasher(key));
        return static_cast<std::size_t>(h % bucket_count);
    }

    [[nodiscard]] static std::size_t hash_for_key(const K& key,
                                                  std::size_t bucket_count) noexcept {
        if constexpr (std::is_same_v<K, T81Symbol>) {
            return symbol_hash(key, bucket_count);
        } else {
            return generic_hash(key, bucket_count);
        }
    }

    // Ternary-inspired probing: odd step sizes to reduce clustering
    [[nodiscard]] static std::size_t probe_step(std::size_t attempt) noexcept {
        // 1, 3, 5, 7, ... (simple, works with any bucket_count)
        return 1 + 2 * attempt;
    }

    // ------------------------------------------------------------------
    // Rehashing
    // ------------------------------------------------------------------
    void rehash_if_needed() {
        if (buckets_.empty()) {
            buckets_.resize(27); // 3^3 initial size
            return;
        }

        const double lf = static_cast<double>(size_) / static_cast<double>(buckets_.size());
        if (lf < MAX_LOAD_FACTOR) return;

        const std::size_t old_count = buckets_.size();
        std::size_t new_count = old_count ? old_count * 3 : 27; // grow by factor of 3

        std::vector<Bucket> new_buckets(new_count);
        for (const auto& bucket : buckets_) {
            if (!bucket.occupied) continue;

            std::size_t idx = hash_for_key(bucket.key, new_count);
            std::size_t attempt = 0;
            while (new_buckets[idx].occupied) {
                idx = (idx + probe_step(attempt++)) % new_count;
            }
            new_buckets[idx] = bucket;
        }

        buckets_ = std::move(new_buckets);
    }

    // ------------------------------------------------------------------
    // Internal lookup: index of key if present (const)
    // ------------------------------------------------------------------
    [[nodiscard]] std::optional<std::size_t> find_index(const K& key) const noexcept {
        if (buckets_.empty()) return std::nullopt;

        const std::size_t n = buckets_.size();
        std::size_t idx = hash_for_key(key, n);
        std::size_t attempt = 0;

        // Probe until we find the key or hit an empty bucket
        while (buckets_[idx].occupied) {
            if (buckets_[idx].key == key) {
                return idx;
            }
            idx = (idx + probe_step(attempt++)) % n;
        }

        return std::nullopt;
    }

    // Non-const helper that reuses the const version
    [[nodiscard]] std::optional<std::size_t> find_index_nonconst(const K& key) noexcept {
        return const_cast<const T81Map*>(this)->find_index(key);
    }

    // ------------------------------------------------------------------
    // Erase implementation for open addressing
    // ------------------------------------------------------------------
    void erase_index(std::size_t idx) noexcept {
        if (buckets_.empty() || !buckets_[idx].occupied) return;

        const std::size_t n = buckets_.size();

        // Remove the element at idx
        buckets_[idx].occupied = false;
        --size_;

        // Reinsert the cluster of elements that might be displaced
        std::size_t j = (idx + 1) % n;
        while (buckets_[j].occupied) {
            Bucket tmp = std::move(buckets_[j]);
            buckets_[j].occupied = false;
            --size_;

            // Reinsert tmp into the same bucket array
            std::size_t new_idx = hash_for_key(tmp.key, n);
            std::size_t attempt = 0;
            while (buckets_[new_idx].occupied) {
                new_idx = (new_idx + probe_step(attempt++)) % n;
            }
            buckets_[new_idx] = std::move(tmp);
            ++size_;

            j = (j + 1) % n;
        }
    }

public:
    using key_type    = K;
    using mapped_type = V;
    using value_type  = std::pair<const K, V>;
    using size_type   = std::size_t;

    //===================================================================
    // Construction
    //===================================================================
    T81Map() {
        buckets_.resize(27); // start at 3^3
    }

    //===================================================================
    // Element access
    //===================================================================
    [[nodiscard]] V& operator[](const K& key) {
        rehash_if_needed();

        const std::size_t n = buckets_.size();
        std::size_t idx = hash_for_key(key, n);
        std::size_t attempt = 0;

        while (buckets_[idx].occupied) {
            if (buckets_[idx].key == key) {
                return buckets_[idx].value;
            }
            idx = (idx + probe_step(attempt++)) % n;
        }

        // Insert new
        buckets_[idx].key      = key;
        buckets_[idx].value    = V{};
        buckets_[idx].occupied = true;
        ++size_;

        rehash_if_needed();
        return buckets_[idx].value;
    }

    [[nodiscard]] const V& at(const K& key) const {
        auto idx = find_index(key);
        if (!idx) {
            throw std::out_of_range("T81Map::at – key not found");
        }
        return buckets_[*idx].value;
    }

    [[nodiscard]] V& at(const K& key) {
        auto idx = find_index_nonconst(key);
        if (!idx) {
            throw std::out_of_range("T81Map::at – key not found");
        }
        return buckets_[*idx].value;
    }

    //===================================================================
    // Lookup
    //===================================================================
    [[nodiscard]] bool contains(const K& key) const noexcept {
        return find_index(key).has_value();
    }

    [[nodiscard]] std::optional<V> get(const K& key) const noexcept {
        auto idx = find_index(key);
        if (!idx) return std::nullopt;
        return buckets_[*idx].value;
    }

    //===================================================================
    // Modifiers
    //===================================================================
    size_type erase(const K& key) noexcept {
        auto idx = find_index_nonconst(key);
        if (!idx) return 0;
        erase_index(*idx);
        return 1;
    }

    void clear() noexcept {
        buckets_.clear();
        buckets_.resize(27);
        size_ = 0;
    }

    //===================================================================
    // Iterators (read-only view)
    //===================================================================
    struct const_iterator {
        const T81Map* map = nullptr;
        std::size_t index = 0;

        [[nodiscard]] bool operator==(const const_iterator& o) const noexcept = default;

        const_iterator& operator++() noexcept {
            const std::size_t n = map->buckets_.size();
            do {
                ++index;
            } while (index < n && !map->buckets_[index].occupied);
            return *this;
        }

        [[nodiscard]] value_type operator*() const noexcept {
            const auto& b = map->buckets_[index];
            return value_type{b.key, b.value};
        }
    };

    [[nodiscard]] const_iterator begin() const noexcept {
        const_iterator it{this, 0};
        const std::size_t n = buckets_.size();
        while (it.index < n && !buckets_[it.index].occupied) {
            ++it.index;
        }
        return it;
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator{this, buckets_.size()};
    }

    //===================================================================
    // Size & Capacity
    //===================================================================
    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    //===================================================================
    // Comparison (structural)
    //===================================================================
    [[nodiscard]] auto operator<=>(const T81Map&) const noexcept = default;
};

// ======================================================================
// Canonical maps that do *not* depend on T81List
// ======================================================================

// string → token ID
using VocabMap = T81Map<T81String, std::uint32_t>;

// If you want SymbolTable later, define it in a separate header that
// includes T81List.hpp, e.g.:
//   using SymbolTable = T81Map<T81Symbol, T81List<T81Symbol>>;

} // namespace t81
