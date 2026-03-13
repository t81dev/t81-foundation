#pragma once

#include "kernel_abi.hpp"

#include "t81/canonfs/canon_types.hpp"

#include <optional>

namespace t81::ternaryos::kernel {

inline constexpr std::size_t kKernelExecutableLabelLimit = 96;

std::optional<t81::canonfs::CanonBlock> axion_kernel_encode_executable_block(
    const KernelThreadSpawnDescriptor& descriptor) noexcept;

std::optional<KernelThreadSpawnDescriptor> axion_kernel_decode_executable_block(
    const t81::canonfs::CanonBlock& block) noexcept;

}  // namespace t81::ternaryos::kernel
