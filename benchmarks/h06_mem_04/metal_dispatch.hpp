#pragma once

#include <cstdint>

namespace t81::bench::h06_mem_04 {

// Returns true when compiled with T81_HAS_METAL=1 and an Apple Metal GPU device is present.
[[nodiscard]] bool metal_backend_available() noexcept;

// Execute bit-exact ternary matrix-batch matmul on Apple Metal GPU using coalesced
// multi-batch 2D threadgroups and shared unified-memory buffers (zero leaked resources).
// Returns false if Metal is unavailable or queue_depth > max_queue_depth (triggering
// deterministic caller fallback to cpu_simd_mt).
[[nodiscard]] bool metal_execute_ternary_matmul(const std::int8_t* weights,
                                                const std::int8_t* activations,
                                                std::int64_t* output, std::uint32_t batch,
                                                std::uint32_t out_dim, std::uint32_t k_dim,
                                                std::uint32_t queue_depth,
                                                std::uint32_t max_queue_depth) noexcept;

}  // namespace t81::bench::h06_mem_04
