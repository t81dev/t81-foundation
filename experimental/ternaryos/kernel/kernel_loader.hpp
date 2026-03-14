#pragma once

// experimental/ternaryos/kernel/kernel_loader.hpp
//
// CanonExec section loader — Slice 1A (Real Executable Load).
//
// Allocates ternary pages, maps a CanonExec image block into the kernel-wide
// page table at the VPN that contains the entry PC declared in the spawn
// descriptor, and writes section bytes into physical_page_storage.
//
// RFC-00B1 §3 (mmu_map), RFC-00B6 (SpawnThreadFromExecutableObject).

#include "kernel_abi.hpp"
#include "t81/canonfs/canon_types.hpp"

#include <cstdint>
#include <optional>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState;

// ─── Load result ─────────────────────────────────────────────────────────────

/// Returned by load_canon_exec_sections() on success.
struct CanonExecLoadResult {
  uint64_t entry_tva{0};        ///< Entry TVA — identical to requested_pc, now mapped
  uint64_t phys_base{0};        ///< Physical page base holding the text section
  bool     already_mapped{false}; ///< true when the page was pre-existing (re-spawn path)
};

// ─── Section loader ──────────────────────────────────────────────────────────

/// Load a CanonExec image block into the address space.
///
/// **First load**: allocates one page from state.allocator, maps it at the VPN
/// containing @p requested_pc with {readable=true, writable=false,
/// executable=true}, and copies image_block bytes into
/// state.physical_page_storage.
///
/// **Re-spawn** (VPN already mapped): skips allocation, looks up the existing
/// physical base, returns already_mapped=true.  The entry_tva is preserved
/// unchanged so the spawned thread's PC remains correct.
///
/// @return nullopt on OOM or if the VPN is mapped but cannot be translated.
std::optional<CanonExecLoadResult> load_canon_exec_sections(
    KernelRuntimeState&             state,
    ProcessGroupId                  owner_group_id,
    const t81::canonfs::CanonBlock& image_block,
    uint64_t                        requested_pc);

}  // namespace t81::ternaryos::kernel
