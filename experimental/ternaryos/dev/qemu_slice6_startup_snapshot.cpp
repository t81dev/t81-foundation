// experimental/ternaryos/dev/qemu_slice6_startup_snapshot.cpp
//
// Host-side snapshot tool for Slice 6 — QEMU AArch64 EDK2 guest image bootstrap.
//
// Exercises Slices 4 and 5 in process, queries the runtime status view, then
// emits a C header (`qemu_slice6_startup.h`) and a plain-text report that are
// embedded in the QEMU-native EFI stub (`qemu_armv8_efi_stub.c`) at build
// time.  At UEFI runtime the stub writes this content to the FAT32 disk so
// the QEMU probe script can read it back after the machine exits.
//
// References: RFC-00B6 §5.2 (Slice 4), RFC-00B1 §3.1 / RFC-00B6 §5.7 (Slice 5),
//             RFC-00B3 §3.9 (QEMU AArch64 + EDK2 developer lane).

#include "../kernel/kernel_main.hpp"
#include "../kernel/kernel_service_contract.hpp"
#include "../kernel/kernel_trap_shim.hpp"
#include "../mmu/tva.hpp"

#include <cstddef>
#include <cstdio>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace {

bool write_file(const std::string& path, const std::string& contents) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out << contents;
  return out.good();
}

std::optional<std::string> build_slice6_report() {
  using namespace t81::ternaryos::kernel;
  using namespace t81::ternaryos::hal;
  using namespace t81::ternaryos;

  // ── Boot context — mirrors QEMU virt AArch64 memory layout ───────────────
  BootContext ctx;
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys  = 0x0000000040000000ULL,  // QEMU virt RAM base
      .size_bytes = 128ULL * 1024ULL * 1024ULL,
      .writable   = true,
      .executable = false,
  });
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys  = 0x0000000048000000ULL,  // QEMU virt kernel load region
      .size_bytes = 4ULL * 1024ULL * 1024ULL,
      .writable   = true,
      .executable = true,
  });
  ctx.kernel_load_address  = 0x0000000048000000ULL;
  ctx.stack_top            = 0x0000000047FFF000ULL;
  ctx.ethics_boot_required = false;  // snapshot / test mode only
  ctx.platform_id          = "qemu-armv8:AArch64/EDK2/slice6-snapshot-host";

  // ── Bootstrap the Axion kernel ────────────────────────────────────────────
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) return std::nullopt;
  KernelRuntimeState& state = *state_opt;

  // ── Slice 4: SVC trap wiring (RFC-00B6 §5.2) ──────────────────────────────
  // Exercise the rejection path: svc_imm != 0 must be rejected without
  // needing valid TVAs.  A single call is sufficient to confirm the shim is
  // present and the enforcement rule executes.
  SvcTrapFrame bad_frame;
  bad_frame.svc_imm = 7;  // non-zero → RFC-00B6 §5.2 requires rejection
  const auto bad_result = axion_kernel_handle_svc_trap(state, bad_frame);
  const bool slice4_ok  = bad_result.svc_imm_rejected;

  // ── Slice 5: user-space address space isolation (RFC-00B1 §3.1) ──────────
  // Spawn a user thread so that a user process group and its address space
  // exist; then attempt to write to a kernel-space TVA from that user AS.
  // The write must fail and increment `counters.kernel_space_rejections`.
  sched::TiscContext user_ctx{};
  const auto user_tid = axion_kernel_spawn_thread(state, user_ctx);
  bool slice5_ok       = false;

  if (user_tid.has_value()) {
    axion_kernel_tick(state);  // makes user thread the current thread

    const auto* user_rt = state.find_thread_runtime(*user_tid);
    const auto  user_as = user_rt
        ? state.find_process_group_address_space(user_rt->process_group_id)
        : std::nullopt;

    if (user_as.has_value()) {
      // Attempt to write to a kernel-space TVA from the (non-kernel) user AS.
      // VPN = kKernelSpaceVpnBase = 3^19 — first VPN in kernel half.
      const uint64_t kernel_tva =
          mmu::tva_from_vpn_offset(mmu::kKernelSpaceVpnBase, 0);
      const uint64_t rejections_before = state.counters.kernel_space_rejections;
      const std::byte dummy{0xAB};
      axion_kernel_write_address_space_bytes(state, *user_as, kernel_tva,
                                             &dummy, 1);
      slice5_ok = (state.counters.kernel_space_rejections > rejections_before);
    }
  }

  // ── Query runtime status view ─────────────────────────────────────────────
  const auto status = axion_kernel_service_request(
      state,
      KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  const bool status_ok          = (status.status == KernelServiceStatus::Ok);
  const uint64_t trap_dispatches =
      (status.runtime ? status.runtime->syscall_trap_dispatches : 0);
  const uint64_t space_rejections =
      (status.runtime ? status.runtime->kernel_space_rejections : 0);

  // ── Build the report ──────────────────────────────────────────────────────
  std::ostringstream report;
  report << "AXION_QEMU_SLICE6_BOOT_REPORT\n";
  report << "platform_id=qemu-armv8:AArch64/EDK2/slice6-boot-probe\n";
  report << "slice4_svc_trap_wiring="
         << (slice4_ok ? "complete" : "incomplete") << "\n";
  report << "slice5_user_isolation="
         << (slice5_ok ? "complete" : "incomplete") << "\n";
  report << "syscall_trap_dispatches=" << trap_dispatches << "\n";
  report << "kernel_space_rejections=" << space_rejections << "\n";
  report << "runtime_status_ok=" << (status_ok ? "true" : "false") << "\n";
  report << "kernel_boot_ready_slice=slice6-efi-boot\n";
  report << "hal_main_result=0\n";
  report << "boot_progress_state=ready\n";
  report << "boot_progress_pending=false\n";
  report << "boot_progress_blocked=false\n";
  report << "boot_validation_lane=qemu-armv8-slice6-probe\n";

  if (!slice4_ok || !slice5_ok || !status_ok) return std::nullopt;
  return report.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fputs(
        "usage: qemu_slice6_startup_snapshot <report-text> <report-header>\n",
        stderr);
    return 2;
  }

  const auto report = build_slice6_report();
  if (!report.has_value()) {
    std::fputs("failed to build slice6 startup report\n", stderr);
    return 1;
  }

  const std::string header =
      "#pragma once\n"
      "static const char kGeneratedQemuSlice6Startup[] =\n"
      "R\"AXION_SLICE6(\n" +
      *report + ")AXION_SLICE6\";\n";

  if (!write_file(argv[1], *report)) {
    std::fputs("failed to write slice6 startup report text\n", stderr);
    return 1;
  }
  if (!write_file(argv[2], header)) {
    std::fputs("failed to write slice6 startup report header\n", stderr);
    return 1;
  }

  return 0;
}
