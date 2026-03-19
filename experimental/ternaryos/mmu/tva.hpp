#pragma once

// experimental/ternaryos/mmu/tva.hpp
//
// Ternary Virtual Address (TVA) — RFC-00B1 §2.
//
// A TVA is a uint64_t interpreted as an unsigned base-3 number:
//
//   tva = vpn * kPageSize + offset
//
//   kPageSize  = 3^10 = 59,049  (tryte offset within a page)
//   VPN        = tva / kPageSize (virtual page number; 20 trits → 3^20 pages)
//   offset     = tva % kPageSize (0..59,048)
//
// The full virtual space is 3^30 ≈ 205 TB — fits in uint64_t.
// Physical addresses remain plain uint64_t byte offsets (no re-encoding).

#include <cstdint>
#include <optional>
#include <string>

namespace t81::ternaryos::mmu {

// ─── Constants ───────────────────────────────────────────────────────────────

/// Bytes per ternary page: 3^10 = 59,049.
inline constexpr uint64_t kPageSize = 59049ULL;

/// Maximum virtual page number (VPN): 3^20 − 1.
inline constexpr uint64_t kMaxVpn  = 3486784400ULL;  // 3^20 - 1

/// Maximum valid TVA: kMaxVpn * kPageSize + (kPageSize - 1) = 3^30 − 1.
inline constexpr uint64_t kMaxTva  = 205891132094648ULL;  // 3^30 - 1

// ─── TVA decomposition ───────────────────────────────────────────────────────

/// Extract the virtual page number from a TVA.
inline constexpr uint64_t tva_vpn(uint64_t tva) noexcept {
  return tva / kPageSize;
}

/// Extract the page offset from a TVA (0..kPageSize−1).
inline constexpr uint64_t tva_offset(uint64_t tva) noexcept {
  return tva % kPageSize;
}

/// Compose a TVA from a VPN and a page offset.
inline constexpr uint64_t tva_from_vpn_offset(uint64_t vpn,
                                               uint64_t offset) noexcept {
  return vpn * kPageSize + offset;
}

/// True if the TVA is within the valid virtual address space.
inline constexpr bool tva_valid(uint64_t tva) noexcept {
  return tva <= kMaxTva;
}

// ─── Kernel / user address space split (RFC-00B1 §3.1 / RFC-00B6 §5.7) ───────
//
// The 20-trit VPN space is divided at 3^19 (the most-significant trit boundary):
//
//   User space:   VPN   0 .. 3^19 − 1  (trit-19 = 0)
//   Kernel space: VPN 3^19 .. 3^20 − 1 (trit-19 = 1 or 2)
//
// User-mode callers (process-group address spaces) may only map and access
// TVAs below kKernelSpaceVpnBase.  The kernel address space (id=0) may map
// anywhere in the full 3^30 virtual space.

/// First VPN in the kernel half: 3^19 = 1,162,261,467.
inline constexpr uint64_t kKernelSpaceVpnBase = 1162261467ULL;  // 3^19

/// Exclusive upper bound for user-space VPNs (equal to kKernelSpaceVpnBase).
inline constexpr uint64_t kUserSpaceVpnLimit  = 1162261467ULL;  // 3^19

/// True if a TVA falls within user-accessible virtual space (VPN < 3^19).
inline constexpr bool tva_in_user_space(uint64_t tva) noexcept {
  return tva_vpn(tva) < kUserSpaceVpnLimit;
}

/// True if a TVA falls within kernel-only virtual space (VPN >= 3^19).
inline constexpr bool tva_in_kernel_space(uint64_t tva) noexcept {
  return tva_vpn(tva) >= kKernelSpaceVpnBase;
}

// ─── Ternary digit utilities ──────────────────────────────────────────────────

/// Return the k-th trit (base-3 digit) of val, starting from trit 0 (least
/// significant). Value is in {0, 1, 2}.
inline constexpr uint8_t trit_at(uint64_t val, unsigned k) noexcept {
  // Divide out lower k trits, then take mod 3.
  for (unsigned i = 0; i < k; ++i) val /= 3;
  return static_cast<uint8_t>(val % 3);
}

/// Return the number of non-zero trits in val (ternary Hamming weight).
inline constexpr unsigned trit_weight(uint64_t val) noexcept {
  unsigned w = 0;
  while (val > 0) {
    if (val % 3 != 0) ++w;
    val /= 3;
  }
  return w;
}

/// Convert a TVA to a human-readable base-3 string (most-significant trit
/// first), limited to the 30 trits of the valid address space.
std::string tva_to_string(uint64_t tva);

}  // namespace t81::ternaryos::mmu
