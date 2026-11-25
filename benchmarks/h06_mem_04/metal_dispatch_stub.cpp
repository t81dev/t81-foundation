#include "metal_dispatch.hpp"

namespace t81::bench::h06_mem_04 {

bool metal_backend_available() noexcept { return false; }

bool metal_execute_ternary_matmul(const std::int8_t*, const std::int8_t*, std::int64_t*,
                                  std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
                                  std::uint32_t) noexcept {
  return false;
}

}  // namespace t81::bench::h06_mem_04
