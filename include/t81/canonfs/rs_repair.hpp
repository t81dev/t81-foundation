#pragma once

#include <cstddef>
#include <vector>
#include "t81/canonfs/gf3_9.hpp"

namespace t81::canonfs {

class ReedSolomonRepair {
public:
  static constexpr int kDataShards = 3;
  static constexpr int kParityShards = 2;
  static constexpr int kTotalShards = kDataShards + kParityShards;

  /**
   * @brief Simple Vandermonde-based encoding for GF(3^9).
   * Generates kParityShards, where each parity element uses 2 bytes to represent the up-to-19682 GF(3^9) elements.
   */
  static std::vector<std::vector<std::byte>> encode(const std::vector<std::byte>& data) {
    size_t total_size = data.size();
    size_t shard_size = (total_size + kDataShards - 1) / kDataShards;

    std::vector<std::vector<std::byte>> shards(kTotalShards);
    for (int i = 0; i < kDataShards; ++i) shards[i].resize(shard_size, std::byte{0});
    for (int i = kDataShards; i < kTotalShards; ++i) shards[i].resize(shard_size * 2, std::byte{0});

    // Split data into kDataShards
    for (size_t i = 0; i < total_size; ++i) {
      shards[i / shard_size][i % shard_size] = data[i];
    }

    // Generate parity shards
    for (size_t byte_idx = 0; byte_idx < shard_size; ++byte_idx) {
      for (int i = 0; i < kParityShards; ++i) {
        GF3_9::value_type sum = 0;
        GF3_9::value_type x = static_cast<GF3_9::value_type>(kDataShards + i + 1);
        GF3_9::value_type x_pow = 1;
        for (int j = 0; j < kDataShards; ++j) {
          GF3_9::value_type val_j = static_cast<uint8_t>(shards[j][byte_idx]);
          sum = GF3_9::add(sum, GF3_9::mul(x_pow, val_j));
          x_pow = GF3_9::mul(x_pow, x);
        }
        shards[kDataShards + i][byte_idx * 2] = static_cast<std::byte>(sum & 0xFF);
        shards[kDataShards + i][byte_idx * 2 + 1] = static_cast<std::byte>((sum >> 8) & 0xFF);
      }
    }
    return shards;
  }

  /**
   * @brief Simple Vandermonde-based reconstruction for GF(3^9).
   */
  static std::vector<std::vector<std::byte>> repair(
      const std::vector<std::vector<std::byte>>& shards, const std::vector<bool>& available) {
    // Find indices of available shards
    std::vector<int> avail_indices;
    for (int i = 0; i < kTotalShards; ++i) {
      if (available[i]) avail_indices.push_back(i);
    }

    if (avail_indices.size() < kDataShards) return {};  // Not enough shards

    // Need exactly kDataShards for the matrix inversion
    std::vector<int> used_indices(avail_indices.begin(), avail_indices.begin() + kDataShards);

    // Matrix A: describes how each available shard was derived from the data (coefficients).
    std::vector<std::vector<GF3_9::value_type>> matrix(kDataShards,
                                                       std::vector<GF3_9::value_type>(kDataShards));
    for (int i = 0; i < kDataShards; ++i) {
      int idx = used_indices[i];
      if (idx < kDataShards) {
        // Data shards are just the raw coefficients.
        for (int j = 0; j < kDataShards; ++j) {
          matrix[i][j] = (j == idx) ? 1 : 0;
        }
      } else {
        // Parity shards are polynomial evaluations at X = idx + 1
        GF3_9::value_type x = static_cast<GF3_9::value_type>(idx + 1);
        GF3_9::value_type x_pow = 1;
        for (int j = 0; j < kDataShards; ++j) {
          matrix[i][j] = x_pow;
          x_pow = GF3_9::mul(x_pow, x);
        }
      }
    }

    // Invert matrix A (Gaussian elimination)
    std::vector<std::vector<GF3_9::value_type>> inv(kDataShards,
                                                    std::vector<GF3_9::value_type>(kDataShards, 0));
    for (int i = 0; i < kDataShards; ++i) inv[i][i] = 1;

    for (int i = 0; i < kDataShards; ++i) {
      GF3_9::value_type pivot = matrix[i][i];
      if (pivot == 0) {
        // Find non-zero pivot
        for (int k = i + 1; k < kDataShards; ++k) {
          if (matrix[k][i] != 0) {
            std::swap(matrix[i], matrix[k]);
            std::swap(inv[i], inv[k]);
            pivot = matrix[i][i];
            break;
          }
        }
      }
      GF3_9::value_type inv_pivot = GF3_9::inv(pivot);
      for (int j = 0; j < kDataShards; ++j) {
        matrix[i][j] = GF3_9::mul(matrix[i][j], inv_pivot);
        inv[i][j] = GF3_9::mul(inv[i][j], inv_pivot);
      }
      for (int k = 0; k < kDataShards; ++k) {
        if (k == i) continue;
        GF3_9::value_type factor = matrix[k][i];
        for (int j = 0; j < kDataShards; ++j) {
          matrix[k][j] = GF3_9::sub(matrix[k][j], GF3_9::mul(matrix[i][j], factor));
          inv[k][j] = GF3_9::sub(inv[k][j], GF3_9::mul(inv[i][j], factor));
        }
      }
    }

    // Determine data shard size.
    size_t shard_size = 0;
    for (int i = 0; i < kTotalShards; ++i) {
      if (available[i]) {
        if (i < kDataShards) {
          shard_size = shards[i].size(); 
          break;
        } else {
          shard_size = shards[i].size() / 2; 
          break;
        }
      }
    }

    // Reconstruct data shards
    std::vector<std::vector<std::byte>> recovered(kDataShards, std::vector<std::byte>(shard_size));

    for (size_t byte_idx = 0; byte_idx < shard_size; ++byte_idx) {
      for (int i = 0; i < kDataShards; ++i) {
        GF3_9::value_type sum = 0;
        for (int j = 0; j < kDataShards; ++j) {
          int original_idx = used_indices[j];
          GF3_9::value_type val_j;
          if (original_idx < kDataShards) {
            val_j = static_cast<uint8_t>(shards[original_idx][byte_idx]);
          } else {
            val_j = static_cast<uint8_t>(shards[original_idx][byte_idx * 2]) |
                    (static_cast<uint16_t>(shards[original_idx][byte_idx * 2 + 1]) << 8);
          }
          sum = GF3_9::add(sum, GF3_9::mul(inv[i][j], val_j));
        }
        recovered[i][byte_idx] = static_cast<std::byte>(sum & 0xFF);
      }
    }

    return recovered;
  }
};

}  // namespace t81::canonfs
