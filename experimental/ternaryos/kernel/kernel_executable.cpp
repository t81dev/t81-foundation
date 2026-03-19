#include "kernel_executable.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace t81::ternaryos::kernel {

namespace {

inline constexpr std::size_t kTypeOffset = 0;
inline constexpr std::size_t kVersionOffset = 1;
inline constexpr std::size_t kFlagsOffset = 2;
inline constexpr std::size_t kLabelLengthOffset = 3;
inline constexpr std::size_t kPcOffset = 4;
inline constexpr std::size_t kSpOffset = 12;
inline constexpr std::size_t kRegister0Offset = 20;
inline constexpr std::size_t kLabelOffset = 28;
inline constexpr uint8_t kExecutableVersion = 1;
inline constexpr uint8_t kFlagHalted = 1u << 0;
inline constexpr uint8_t kFlagActive = 1u << 1;

void encode_u64(uint8_t* out, uint64_t value) noexcept {
  for (std::size_t i = 0; i < sizeof(uint64_t); ++i) {
    out[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xffu);
  }
}

uint64_t decode_u64(const uint8_t* in) noexcept {
  uint64_t value = 0;
  for (std::size_t i = 0; i < sizeof(uint64_t); ++i) {
    value |= static_cast<uint64_t>(in[i]) << (i * 8);
  }
  return value;
}

bool has_zero_padding(const t81::canonfs::CanonBlock& block,
                      std::size_t begin) noexcept {
  return std::all_of(block.trytes.begin() + static_cast<std::ptrdiff_t>(begin),
                     block.trytes.end(),
                     [](uint8_t byte) { return byte == 0; });
}

}  // namespace

std::optional<t81::canonfs::CanonBlock> axion_kernel_encode_executable_block(
    const KernelThreadSpawnDescriptor& descriptor) noexcept {
  if (descriptor.label.size() > kKernelExecutableLabelLimit) {
    return std::nullopt;
  }

  t81::canonfs::CanonBlock block{};
  block.trytes[kTypeOffset] =
      static_cast<uint8_t>(t81::canonfs::ObjectType::CanonExec);
  block.trytes[kVersionOffset] = kExecutableVersion;
  block.trytes[kFlagsOffset] =
      static_cast<uint8_t>((descriptor.halted ? kFlagHalted : 0u) |
                           (descriptor.active ? kFlagActive : 0u));
  block.trytes[kLabelLengthOffset] =
      static_cast<uint8_t>(descriptor.label.size());
  encode_u64(block.trytes.data() + kPcOffset,
             static_cast<uint64_t>(descriptor.pc));
  encode_u64(block.trytes.data() + kSpOffset,
             static_cast<uint64_t>(descriptor.sp));

  static_assert(sizeof(std::int64_t) == sizeof(uint64_t));
  uint64_t register_bits = 0;
  std::memcpy(&register_bits, &descriptor.register0, sizeof(register_bits));
  encode_u64(block.trytes.data() + kRegister0Offset, register_bits);

  std::copy(descriptor.label.begin(),
            descriptor.label.end(),
            block.trytes.begin() + static_cast<std::ptrdiff_t>(kLabelOffset));
  return block;
}

std::optional<KernelThreadSpawnDescriptor> axion_kernel_decode_executable_block(
    const t81::canonfs::CanonBlock& block) noexcept {
  if (block.trytes[kTypeOffset] !=
          static_cast<uint8_t>(t81::canonfs::ObjectType::CanonExec) ||
      block.trytes[kVersionOffset] != kExecutableVersion) {
    return std::nullopt;
  }

  const auto label_size = static_cast<std::size_t>(block.trytes[kLabelLengthOffset]);
  if (label_size > kKernelExecutableLabelLimit ||
      kLabelOffset + label_size > t81::canonfs::CanonBlock::kTryteCount) {
    return std::nullopt;
  }
  if (!has_zero_padding(block, kLabelOffset + label_size)) {
    return std::nullopt;
  }

  std::int64_t register0 = 0;
  const auto register_bits = decode_u64(block.trytes.data() + kRegister0Offset);
  std::memcpy(&register0, &register_bits, sizeof(register0));

  KernelThreadSpawnDescriptor descriptor{
      .pc = static_cast<std::size_t>(decode_u64(block.trytes.data() + kPcOffset)),
      .sp = static_cast<std::size_t>(decode_u64(block.trytes.data() + kSpOffset)),
      .register0 = register0,
      .halted = (block.trytes[kFlagsOffset] & kFlagHalted) != 0,
      .active = (block.trytes[kFlagsOffset] & kFlagActive) != 0,
      .label = std::string(block.trytes.begin() +
                               static_cast<std::ptrdiff_t>(kLabelOffset),
                           block.trytes.begin() +
                               static_cast<std::ptrdiff_t>(kLabelOffset + label_size)),
  };
  return descriptor;
}

}  // namespace t81::ternaryos::kernel
