#include "kernel_loader.hpp"

#include "kernel_runtime_state.hpp"
#include "../mmu/page_table.hpp"
#include "../mmu/tva.hpp"

#include <algorithm>
#include <cstddef>

namespace t81::ternaryos::kernel {

std::optional<CanonExecLoadResult> load_canon_exec_sections(
    KernelRuntimeState&             state,
    ProcessGroupId                  owner_group_id,
    const t81::canonfs::CanonBlock& image_block,
    uint64_t                        requested_pc) {

  using namespace t81::ternaryos::mmu;

  const uint64_t vpn          = tva_vpn(requested_pc);
  const uint64_t page_base_tva = tva_from_vpn_offset(vpn, 0);
  const uint32_t owner_pid    = static_cast<uint32_t>(owner_group_id);

  // ── Attempt a fresh page mapping ─────────────────────────────────────────
  //
  // mmu_map() returns false if the VPN is already mapped or the allocator is
  // OOM.  Distinguish the two cases by attempting a translate after failure.

  const bool mapped = mmu_map(
      state.page_table,
      state.allocator,
      page_base_tva,
      owner_pid,
      PagePermissions{.readable = true, .writable = false, .executable = true});

  if (!mapped) {
    // VPN already present — this is a re-spawn of the same executable.
    // Confirm the mapping is valid before proceeding.
    const auto existing_phys = mmu_translate(state.page_table, page_base_tva);
    if (!existing_phys.has_value()) {
      // OOM: mmu_map failed and no prior mapping exists.
      return std::nullopt;
    }
    return CanonExecLoadResult{
        .entry_tva    = requested_pc,
        .phys_base    = *existing_phys,
        .already_mapped = true,
    };
  }

  // ── Write section bytes into physical_page_storage ───────────────────────

  const auto phys_opt = mmu_translate(state.page_table, page_base_tva);
  if (!phys_opt.has_value()) {
    // Guard: should not occur after a successful mmu_map.
    return std::nullopt;
  }
  const uint64_t phys_base = *phys_opt;

  // Initialise the full page with zero bytes, then overlay the block data.
  auto& page_data = state.physical_page_storage[phys_base];
  page_data.assign(kPageSize, std::byte{0});

  const uint64_t  page_offset = tva_offset(requested_pc);
  const auto      block_bytes = image_block.to_bytes();
  const std::size_t copy_len  = std::min(
      block_bytes.size(),
      static_cast<std::size_t>(kPageSize - page_offset));

  std::copy(
      block_bytes.begin(),
      block_bytes.begin() + static_cast<std::ptrdiff_t>(copy_len),
      page_data.begin()   + static_cast<std::ptrdiff_t>(page_offset));

  return CanonExecLoadResult{
      .entry_tva    = requested_pc,
      .phys_base    = phys_base,
      .already_mapped = false,
  };
}

}  // namespace t81::ternaryos::kernel
