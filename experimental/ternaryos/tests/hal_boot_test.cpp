// experimental/ternaryos/tests/hal_boot_test.cpp
//
// Unit tests for TernOS Phase 1 HAL boot sequence (RFC-00B0 §7).
//
// Acceptance criteria tested here:
//   [AC-1] hal_main returns 0 on a valid BootContext with ethics enabled.
//   [AC-2] hal_main returns non-zero when memory_map is empty.
//   [AC-3] hal_main returns non-zero when no writable region exists.
//   [AC-4] hal_main returns 0 when ethics_boot_required=false (test bypass).
//   [AC-5] Interrupt handler registration and dispatch are functional.
//   [AC-6] fire_simulated_interrupt reaches the registered handler.
//   [AC-7] ternaryos_hosted_boot(true) succeeds end-to-end.

#include "../hal/hal.hpp"
#include "../hal/virtualbox_guest_devices.hpp"
#include "../hal/virtualbox_platform.hpp"
#include "../dev/hosted_block_dev.hpp"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <string>

// Declared in hosted_stub.cpp
namespace t81::ternaryos::hal {
int  ternaryos_hosted_boot(bool ethics_required);
void fire_simulated_interrupt(InterruptSource source, uint64_t payload);
}

using namespace t81::ternaryos::hal;

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
  return cond;
}

// ─── Helper: minimal valid BootContext ───────────────────────────────────────

static BootContext make_valid_ctx(bool ethics = false) {
  BootContext ctx;
  ctx.memory_map.push_back(MemoryRegion{
    .base_phys  = 0x100000ULL,
    .size_bytes = 64ULL * 1024 * 1024,
    .writable   = true,
    .executable = false,
  });
  ctx.kernel_load_address  = 0x100000ULL;
  ctx.stack_top            = 0x200000ULL;
  ctx.ethics_boot_required = ethics;
  ctx.platform_id          = "test";
  return ctx;
}

// ─── Test cases ──────────────────────────────────────────────────────────────

static void test_valid_context_no_ethics() {
  std::printf("\n[AC-4] Valid context, ethics disabled\n");
  auto ctx = make_valid_ctx(/*ethics=*/false);
  int  rc  = hal_main(ctx);
  check(rc == 0, "hal_main returns 0");
}

static void test_valid_context_with_ethics() {
  std::printf("\n[AC-1] Valid context, ethics enabled\n");
  auto ctx = make_valid_ctx(/*ethics=*/true);
  int  rc  = hal_main(ctx);
  check(rc == 0, "hal_main returns 0 with ethics gate");
}

static void test_empty_memory_map() {
  std::printf("\n[AC-2] Empty memory map\n");
  BootContext ctx;
  ctx.ethics_boot_required = false;
  ctx.platform_id          = "test";
  int rc = hal_main(ctx);
  check(rc != 0, "hal_main returns non-zero for empty memory map");
}

static void test_no_writable_region() {
  std::printf("\n[AC-3] No writable region\n");
  BootContext ctx;
  ctx.memory_map.push_back(MemoryRegion{
    .base_phys  = 0x100000ULL,
    .size_bytes = 64ULL * 1024 * 1024,
    .writable   = false,   // read-only
    .executable = true,
  });
  ctx.ethics_boot_required = false;
  ctx.platform_id          = "test";
  int rc = hal_main(ctx);
  check(rc != 0, "hal_main returns non-zero for no writable region");
}

static void test_interrupt_registration_and_dispatch() {
  std::printf("\n[AC-5 / AC-6] Interrupt registration and dispatch\n");

  std::atomic<int>      call_count{0};
  std::atomic<uint64_t> last_payload{0};

  register_interrupt_handler(InterruptSource::Storage, [&](const HardwareInterrupt& irq) {
    ++call_count;
    last_payload.store(irq.payload);
  });

  fire_simulated_interrupt(InterruptSource::Storage, 0xDEAD'BEEF);
  fire_simulated_interrupt(InterruptSource::Storage, 0xCAFE'BABE);

  check(call_count.load() == 2,         "handler called twice");
  check(last_payload.load() == 0xCAFE'BABE, "last payload matches");
}

static void test_unknown_interrupt_fallback() {
  std::printf("\n[AC-5] Unknown interrupt falls back to Unknown handler\n");

  std::atomic<bool> unknown_fired{false};

  register_interrupt_handler(InterruptSource::Unknown, [&](const HardwareInterrupt&) {
    unknown_fired = true;
  });

  // Keyboard is not explicitly registered — should fall back to Unknown.
  fire_simulated_interrupt(InterruptSource::Keyboard, 0x41 /*'A'*/);

  check(unknown_fired.load(), "Unknown handler fired as fallback");
}

static void test_ternary_page_count() {
  std::printf("\n[AC-1] MemoryRegion::ternary_page_count()\n");
  MemoryRegion r{.base_phys = 0, .size_bytes = 59049 * 4, .writable = true, .executable = false};
  check(r.ternary_page_count() == 4, "4 ternary pages in 4 * 59049 bytes");
}

static void test_hosted_boot_end_to_end() {
  std::printf("\n[AC-7] ternaryos_hosted_boot end-to-end\n");
  int rc = ternaryos_hosted_boot(/*ethics_required=*/true);
  check(rc == 0, "hosted_boot with ethics returns 0");
}

static void test_virtualbox_profile_defaults() {
  std::printf("\n[AC-8] VirtualBox profile defaults are accepted\n");
  VBoxProfile profile;
  auto err = validate_virtualbox_profile(profile);
  check(!err.has_value(), "default VirtualBox profile is valid");
  check(virtualbox_profile_summary(profile) == "VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC",
        "default profile summary matches first-target VM profile");
}

static void test_virtualbox_profile_rejects_nvme_without_opt_in() {
  std::printf("\n[AC-9] VirtualBox profile rejects NVMe without explicit opt-in\n");
  VBoxProfile profile;
  profile.storage = VBoxStorage::Nvme;
  auto err = validate_virtualbox_profile(profile);
  check(err.has_value(), "NVMe-first profile rejected by default");
}

static void test_virtualbox_boot_context() {
  std::printf("\n[AC-10] VirtualBox BootContext scaffold\n");
  VBoxBootSpec spec;
  spec.ram_bytes = 96ULL * 1024 * 1024;
  auto ctx = make_virtualbox_boot_context(spec);

  check(ctx.platform_id.find("virtualbox-x86_64:") == 0,
        "platform_id is tagged as virtualbox-x86_64");
  check(ctx.memory_map.size() == 3, "VirtualBox scaffold creates 3 memory regions");
  check(ctx.memory_map[0].writable, "RAM region is writable");
  check(ctx.memory_map[1].executable, "kernel image region is executable");
  check(!ctx.memory_map[2].writable, "MMIO region is reserved/read-only");

  int rc = hal_main(ctx);
  check(rc == 0, "hal_main accepts VirtualBox scaffold BootContext");
}

static void test_virtualbox_device_map_defaults() {
  std::printf("\n[AC-11] VirtualBox device map for first-target profile\n");
  VBoxProfile profile;
  auto devices = virtualbox_device_map(profile);

  check(devices.size() == 5, "first-target profile exposes 5 primary devices");
  check(devices[0].name == std::string("ioapic"), "device[0] is ioapic");
  check(devices[1].name == std::string("hpet"),   "device[1] is hpet");
  check(devices[2].name == std::string("ahci"),   "device[2] is ahci");
  check(devices[3].name == std::string("e1000") || devices[3].name == std::string("vmsvga"),
        "remaining devices include e1000/vmsvga");

  bool saw_ahci = false;
  bool saw_e1000 = false;
  bool saw_vmsvga = false;
  for (const auto& dev : devices) {
    if (std::string(dev.name) == "ahci") {
      saw_ahci = true;
      check(dev.bus == VBoxBusKind::Mmio, "ahci uses MMIO");
      check(dev.irq == 19, "ahci IRQ == 19");
    } else if (std::string(dev.name) == "e1000") {
      saw_e1000 = true;
      check(dev.bus == VBoxBusKind::Mmio, "e1000 uses MMIO");
      check(dev.irq == 11, "e1000 IRQ == 11");
    } else if (std::string(dev.name) == "vmsvga") {
      saw_vmsvga = true;
      check(dev.bus == VBoxBusKind::Mmio, "vmsvga uses MMIO");
      check(dev.irq == 16, "vmsvga IRQ == 16");
    }
  }
  check(saw_ahci, "device map includes ahci");
  check(saw_e1000, "device map includes e1000");
  check(saw_vmsvga, "device map includes vmsvga");
}

static void test_virtualbox_timer_tick_scaffold() {
  std::printf("\n[AC-12] VirtualBox timer tick scaffold\n");

  VBoxProfile profile;
  auto tick = make_virtualbox_timer_tick(profile, /*tick_index=*/7, /*period_ns=*/1000000);
  check(tick.has_value(), "timer tick can be constructed for default profile");
  if (tick) {
    check(tick->tick_index == 7, "tick index preserved");
    check(tick->period_ns == 1000000, "tick period preserved");
    check(tick->hpet_counter == 7000000, "HPET counter derived from tick index");
    check(tick->ioapic_irq == 2, "timer tick uses IOAPIC IRQ 2");
  }

  std::atomic<int> timer_calls{0};
  std::atomic<uint64_t> last_payload{0};
  std::atomic<uint64_t> last_ts{0};

  register_interrupt_handler(InterruptSource::Timer, [&](const HardwareInterrupt& irq) {
    ++timer_calls;
    last_payload = irq.payload;
    last_ts = irq.timestamp_ns;
  });

  check(dispatch_virtualbox_timer_tick(profile, /*tick_index=*/3, /*period_ns=*/500000),
        "dispatch_virtualbox_timer_tick returns true");
  check(timer_calls.load() == 1, "timer handler invoked once");
  check(last_payload.load() == 3, "timer IRQ payload carries tick index");
  check(last_ts.load() == 1500000, "timer IRQ timestamp matches HPET counter");

  VBoxProfile bad = profile;
  bad.timer = static_cast<VBoxTimerModel>(0);
  check(!dispatch_virtualbox_timer_tick(bad, 1, 1000),
        "dispatch rejects unsupported timer model");
  check(!make_virtualbox_timer_tick(profile, 1, 0).has_value(),
        "timer tick rejects zero period");
}

static void test_virtualbox_storage_binding() {
  std::printf("\n[AC-13] VirtualBox storage binding\n");

  VBoxProfile profile;
  t81::ternaryos::dev::HostedBlockDev backing(12, "backing-store");

  auto binding_err = validate_virtualbox_storage_binding(profile);
  check(!binding_err.has_value(), "AHCI-first profile has a valid storage binding");

  auto binding = create_virtualbox_storage_binding(profile, backing);
  check(binding.has_value(), "create_virtualbox_storage_binding returns a device");
  if (binding) {
    check(binding->binding_name == "virtualbox-ahci", "binding name identifies AHCI path");
    auto info = binding->device->info();
    check(info.device_id == "vbox-ahci0", "bound device id matches AHCI adapter");
    check(info.total_blocks == 12, "bound device exposes backing block count");

    t81::ternaryos::dev::BlockData wr{};
    wr.fill(0xA1);
    t81::ternaryos::dev::BlockData rd{};
    check(binding->device->write_block(4, wr), "bound device write succeeds");
    check(binding->device->read_block(4, rd), "bound device read succeeds");
    check(rd == wr, "bound device round-trips through backing storage");
  }

  VBoxProfile nvme = profile;
  nvme.storage = VBoxStorage::Nvme;
  check(validate_virtualbox_storage_binding(nvme).has_value(),
        "NVMe-first profile rejected until a VirtualBox NVMe adapter exists");
  check(!create_virtualbox_storage_binding(nvme, backing).has_value(),
        "NVMe binding is not created yet");
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== TernOS HAL boot tests (RFC-00B0 §7) ===\n");

  test_valid_context_no_ethics();
  test_valid_context_with_ethics();
  test_empty_memory_map();
  test_no_writable_region();
  test_interrupt_registration_and_dispatch();
  test_unknown_interrupt_fallback();
  test_ternary_page_count();
  test_hosted_boot_end_to_end();
  test_virtualbox_profile_defaults();
  test_virtualbox_profile_rejects_nvme_without_opt_in();
  test_virtualbox_boot_context();
  test_virtualbox_device_map_defaults();
  test_virtualbox_timer_tick_scaffold();
  test_virtualbox_storage_binding();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
