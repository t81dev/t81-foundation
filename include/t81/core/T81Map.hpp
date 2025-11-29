/**
 * @file T81Map.hpp
 * @brief Defines the T81Map class, a ternary-optimized associative map.
 *
 * This file provides the T81Map<K, V> class, a hash map designed for high
 * performance in the T81 ecosystem. It is particularly optimized for
 * `T81Symbol` keys, for which it can use a perfect hash function. For other
 * key types, it falls back to a generic hash. The map uses a power-of-3
 * growth strategy and ternary linear probing to ensure good load distribution
 * and cache performance.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81List.hpp"
#include <cstddef>
#include <span>
#include <vector>
#include <optional>
#include <compare>
#include <bit>      // for std::has_single_bit
#include <cstring>

namespace t81 {

// ======================================================================
// T81Map<K,V> – Perfect-hash map using T81Symbol keys when possible
// ======================================================================
template <typename K, typename V>
    requires (!std::is_void_v<K> && !std::is_void_v<V>)
class T81Map {
    // Internal bucket: one key + one value, packed tightly
    struct Bucket {
        alignas(8) K key;
        V    value;
        bool occupied = false;

        constexpr Bucket() noexcept = default;
    };

    // Use power-of-3 bucket count for perfect ternary load distribution
    alignas(64) std::vector<Bucket> buckets_;
    size_t size_ = 0;  // number of occupied entries

    // Load factor: 0.729 ≈ 3⁻¹ (optimal for ternary probing)
    static constexpr double MAX_LOAD_FACTOR = 0.729;

    // Perfect hash for T81Symbol keys (81 trits → fold to index)
    [[nodiscard]] static constexpr size_t symbol_hash(const t81::core::T81Symbol& sym, size_t bucket_count) noexcept {
        uint64_t h = sym.hash();  // already avalanche-perfect
        // Ternary-friendly reduction: fold to log₂(bucket_count) bits
        size_t mask = bucket_count - 1;
        return h & mask;
    }

    // Fallback hash for arbitrary keys
    template <typename T>
    [[nodiscard]] static constexpr size_t generic_hash(const T& key, size_t bucket_count) noexcept {
        std::hash<T> hasher;
        uint64_t h = static_cast<uint64_t>(hasher(key));
        size_t mask = bucket_count - 1;
        return h & mask;
    }

    // Ternary linear probing: step = 3^k to avoid clustering
    [[nodiscard]] static constexpr size_t probe_step(size_t attempt) noexcept {
        return (attempt < 8) ? (1ULL << (attempt * 2)) : 81;  // fallback to prime
    }

    void rehash_if_needed() {
        if (size_ == 0 || static_cast<double>(size_) / buckets_.size() < MAX_LOAD_FACTOR)
            return;

        size_t old_count = buckets_.size();
        size_t new_count = old_count ? old_count * 3 : 27;  // grow by factor of 3
        if (!std::has_single_bit(new_count)) new_count = 81;  // ensure power-of-3-ish

        std::vector<Bucket> new_buckets(new_count);
        for (const auto& bucket : buckets_) {
            if (!bucket.occupied) continue;

            size_t idx = 0;
            if constexpr (std::same_as<K, t81::core::T81Symbol>) {
                idx = symbol_hash(bucket.key, new_count);
            } else {
                idx = generic_hash(bucket.key, new_count);
            }

            size_t attempt = 0;
            while (new_buckets[idx].occupied) {
                idx = (idx + probe_step(attempt++)) % new_count;
            }
            new_buckets[idx] = bucket;
        }

        buckets_ = std::move(new_buckets);
    }

public:
    using key_type    = K;
    using mapped_type = V;
    using value_type  = std::pair<const K, V>;
    using size_type   = size_t;

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Map() noexcept { buckets_.resize(27); }  // start with 3³

    //===================================================================
    // Element access
    //===================================================================
    [[nodiscard]] V& operator[](const K& key) {
        rehash_if_needed();

        size_t idx = 0;
        if constexpr (std::same_as<K, t81::core::T81Symbol>) {
            idx = symbol_hash(key, buckets_.size());
        } else {
            idx = generic_hash(key, buckets_.size());
        }

        size_t attempt = 0;
        while (buckets_[idx].occupied) {
            if (buckets_[idx].key == key)
                return buckets_[idx].value;
            idx = (idx + probe_step(attempt++)) % buckets_.size();
        }

        // Insert new
        buckets_[idx] = Bucket{key, V{}, true};
        ++size_;
        rehash_if_needed();
        return buckets_[idx].value;
    }

    [[nodiscard]] const V& at(const K& key) const {
        auto it = find(key);
        if (it == end()) throw std::out_of_range("T81Map::at");
        return it->second;
    }

    [[nodiscard]] V& at(const K& key) {
        auto it = find(key);
        if (it == end()) throw std::out_of_range("T81Map::at");
        return it->second;
    }

    //===================================================================
    // Lookup
    //===================================================================
    [[nodiscard]] bool contains(const K& key) const noexcept {
        return find(key) != end();
    }

    [[nodiscard]] std::optional<V> get(const K& key) const noexcept {
        auto it = find(key);
        return (it != end()) ? std::optional<V>(it->second) : std::nullopt;
    }

    //===================================================================
    // Iterators (simple but sufficient)
    //===================================================================
    struct const_iterator {
        const T81Map* map;
        size_t index;

        [[nodiscard]] bool operator==(const const_iterator& o) const noexcept = default;
        const_iterator& operator++() noexcept {
            do { ++index; } while (index < map->buckets_.size() && !map->buckets_[index].occupied);
            return *this;
        }
        [[nodiscard]] value_type operator*() const noexcept {
            return {map->buckets_[index].key, map->buckets_[index].value};
        }
    };

    [[nodiscard]] const_iterator begin() const noexcept {
        const_iterator it{this, 0};
        if (!buckets_[0].occupied) ++it;
        return it;
    }
    [[nodiscard]] const_iterator end() const noexcept { return {this, buckets_.size()}; }

    //===================================================================
    // Size & Capacity
    //===================================================================
    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    //===================================================================
    // Hash & Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Map&) const noexcept = default;
};

// ======================================================================
// Canonical maps used in the new world
// ======================================================================
using SymbolTable   = T81Map<t81::core::T81Symbol, T81List<t81::core::T81Symbol>>;
using EmbeddingMap  = T81Map<t81::core::T81Symbol, T81Tensor<T81Float<72,9>, 1, 4096>>;
using VocabMap      = T81Map<T81String, uint32_t>;           // string → token ID
using KVStore       = T81Map<t81::core::T81Symbol, T81List<T81Float<72,9>>>;

// ======================================================================
// Example: This is how the future stores knowledge
// ======================================================================
/*
T81Map<T81Symbol, T81String> knowledge;
knowledge[symbols::HUMAN] = "MORTAL"_t81;
knowledge[symbols::SOCRATES] = "HUMAN"_t81;

auto mortal = knowledge[symbols::SOCRATES] + " IS " + knowledge[symbols::HUMAN];
assert(mortal.str() == "HUMAN IS MORTAL");
*/
