// experimental/ternaryos/hal/hal_main.cpp
//
// Ethics-first HAL boot sequence (RFC-00B0 §4.4).
// Mirrors the pattern in experimental/hanoi/in_memory_kernel.cpp::boot().

#include "hal.hpp"

#include <iostream>

#include "../kernel/kernel_main.hpp"
#include "t81/axion/ethics.hpp"

namespace t81::ternaryos::hal {

// ─── hal_log ─────────────────────────────────────────────────────────────────

void hal_log(const std::string& msg) {
  std::cout << "[hal] " << msg << "\n";
}

// ─── I/O stubs (Phase 1: hosted / bare-metal stubs) ─────────────────────────

uint8_t io_read8(uint16_t /*port*/) { return 0; }
uint16_t io_read16(uint16_t /*port*/) { return 0; }
void io_write8(uint16_t /*port*/, uint8_t /*val*/) {}
void io_write16(uint16_t /*port*/, uint16_t /*val*/) {}

// ─── hal_main ────────────────────────────────────────────────────────────────

int hal_main(const BootContext& ctx) {
  hal_log("boot start — platform: " +
          (ctx.platform_id.empty() ? "(unspecified)" : ctx.platform_id));

  // ── Step 1: Validate BootContext ──────────────────────────────────────────
  if (ctx.memory_map.empty()) {
    hal_log("FATAL: BootContext has no memory regions.");
    return 1;
  }

  bool has_writable = false;
  uint64_t total_ternary_pages = 0;
  for (const auto& r : ctx.memory_map) {
    if (r.writable) has_writable = true;
    total_ternary_pages += r.ternary_page_count();
  }

  if (!has_writable) {
    hal_log("FATAL: BootContext has no writable memory region.");
    return 1;
  }

  hal_log("memory map: " + std::to_string(ctx.memory_map.size()) + " region(s), " +
          std::to_string(total_ternary_pages) + " ternary pages (3^10-tryte granularity).");

  // ── Step 2: Ethics-first boot (Θ₁–Θ₉) ────────────────────────────────────
  if (ctx.ethics_boot_required) {
    hal_log("ethics gate: evaluating Θ₁–Θ₉ ...");

    t81::axion::SyscallContext syscall_ctx;
    syscall_ctx.caller  = "ternaryos::hal_main";
    syscall_ctx.syscall = "boot";

    for (int i = 1; i <= t81::axion::kEthicsPrincipleCount; ++i) {
      auto principle = static_cast<t81::axion::EthicsPrinciple>(i);
      auto verdict   = t81::axion::check_ethics(principle, syscall_ctx);

      if (verdict.kind == t81::axion::VerdictKind::Deny) {
        hal_log("FATAL: ethics gate DENIED by " +
                std::string(t81::axion::to_string(principle)) +
                " — " + verdict.reason);
        return 1;
      }

      hal_log("  " + std::string(t81::axion::to_string(principle)) + " → " +
              (verdict.kind == t81::axion::VerdictKind::Allow ? "ALLOW" : "WARN/DEFER"));
    }

    hal_log("ethics gate: all nine principles passed.");
  } else {
    hal_log("ethics gate: SKIPPED (ethics_boot_required=false — test mode only).");
  }

  // ── Step 3: Hand off to Axion kernel runtime ──────────────────────────────
  hal_log("Axion kernel handoff: entering kernel-owned runtime.");
  hal_log("  kernel_load_address = 0x" +
          [&]() {
            char buf[17];
            std::snprintf(buf, sizeof(buf), "%016llx",
                          static_cast<unsigned long long>(ctx.kernel_load_address));
            return std::string(buf);
          }());
  hal_log("  stack_top           = 0x" +
          [&]() {
            char buf[17];
            std::snprintf(buf, sizeof(buf), "%016llx",
                          static_cast<unsigned long long>(ctx.stack_top));
            return std::string(buf);
          }());
  const int kernel_rc = t81::ternaryos::kernel::axion_kernel_main(ctx);
  if (kernel_rc != 0) {
    hal_log("FATAL: Axion kernel runtime rejected boot context.");
    return kernel_rc;
  }
  hal_log("Axion kernel handoff: runtime bootstrap complete.");
  return kernel_rc;
}

}  // namespace t81::ternaryos::hal
