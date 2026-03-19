// experimental/ternaryos/hal/hosted_stub.cpp
//
// Hosted (macOS / Linux) simulation of the UEFI PE boot stub.
//
// On a real target this file is replaced by a UEFI application entry point
// (EFI_MAIN) that queries EFI_MEMORY_DESCRIPTOR for the memory map and calls
// hal_main(). Here we simulate that with a synthetic memory map derived from
// the host process address space — sufficient for unit testing and local
// development without the gnu-efi / EDK2 toolchain.
//
// Entry point: ternaryos_hosted_boot()
//   Called by hal_boot_test.cpp and can be invoked standalone.

#include "hal.hpp"

#include <cstdint>
#include <iostream>

namespace t81::ternaryos::hal {

namespace {

// Simulate a modest memory map: two regions that a real UEFI firmware
// might report (conventional memory + MMIO-reserved area).
std::vector<MemoryRegion> build_simulated_memory_map() {
  return {
    // Region 0: 128 MiB conventional RAM — writable, non-executable.
    MemoryRegion{
      .base_phys  = 0x0000'0000'0010'0000ULL,  // 1 MiB base (skip legacy BIOS area)
      .size_bytes = 128ULL * 1024 * 1024,
      .writable   = true,
      .executable = false,
    },
    // Region 1: 4 MiB kernel image mapping — writable + executable.
    MemoryRegion{
      .base_phys  = 0x0000'0000'0800'0000ULL,  // 128 MiB base
      .size_bytes = 4ULL * 1024 * 1024,
      .writable   = true,
      .executable = true,
    },
  };
}

}  // namespace

/**
 * @brief Hosted simulation of the UEFI boot stub.
 *
 * Builds a synthetic BootContext and calls hal_main() exactly as the real
 * UEFI EFI_MAIN stub will, minus the firmware API calls.
 *
 * @param ethics_required  Pass false only in unit tests that want to skip
 *                         the Θ₁–Θ₉ evaluation (e.g. testing bad memory maps).
 * @return Return code from hal_main (0 = success).
 */
int ternaryos_hosted_boot(bool ethics_required) {
  BootContext ctx;
  ctx.memory_map           = build_simulated_memory_map();
  ctx.kernel_load_address  = 0x0000'0000'0800'0000ULL;
  ctx.stack_top            = 0x0000'0000'07FF'F000ULL;
  ctx.ethics_boot_required = ethics_required;
  ctx.platform_id          = "hosted-simulation";

  // Register a minimal timer interrupt handler so the dispatch table
  // is exercised even in the hosted boot path.
  register_interrupt_handler(InterruptSource::Timer, [](const HardwareInterrupt& irq) {
    hal_log("timer IRQ received — payload=" + std::to_string(irq.payload) +
            " ts=" + std::to_string(irq.timestamp_ns) + "ns");
  });

  return hal_main(ctx);
}

}  // namespace t81::ternaryos::hal
