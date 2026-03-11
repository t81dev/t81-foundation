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
#include "../hal/hal_c_abi.h"
#include "../hal/virtualbox_guest_devices.hpp"
#include "../hal/virtualbox_platform.hpp"
#include "../kernel/kernel_main.hpp"
#include "../dev/hosted_block_dev.hpp"
#include "../mmu/page_table.hpp"

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
using namespace t81::ternaryos::kernel;

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

static void test_hal_c_abi_bridge() {
  std::printf("\n[AC-7b] hal_main C ABI bridge\n");

  const TernaryOsMemoryRegion memory_map[] = {
      {
          .base_phys = 0x0000000000100000ULL,
          .size_bytes = 64ULL * 1024ULL * 1024ULL,
          .writable = true,
          .executable = false,
      },
      {
          .base_phys = 0x0000000008000000ULL,
          .size_bytes = 4ULL * 1024ULL * 1024ULL,
          .writable = true,
          .executable = true,
      },
  };
  const TernaryOsBootContext ctx{
      .memory_map = memory_map,
      .memory_map_len = 2,
      .kernel_load_address = 0x0000000008000000ULL,
      .stack_top = 0x0000000007FFF000ULL,
      .ethics_boot_required = true,
      .platform_id = "c-abi-test",
  };

  check(ternaryos_hal_main_c(&ctx) == 0, "ternaryos_hal_main_c accepts a valid C boot context");
  check(ternaryos_hal_main_c(nullptr) != 0, "ternaryos_hal_main_c rejects null context");
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

static void test_virtualbox_guest_bootstrap() {
  std::printf("\n[AC-14] VirtualBox guest bootstrap\n");

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  t81::ternaryos::dev::HostedBlockDev backing(18, "guest-bootstrap-backing");
  auto guest = bootstrap_virtualbox_guest(spec, backing);
  check(guest.has_value(), "bootstrap_virtualbox_guest succeeds for first-target profile");
  if (guest) {
    check(guest->profile_summary == "VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC",
          "guest bootstrap preserves profile summary");
    check(guest->boot_context.platform_id.find("virtualbox-x86_64:") == 0,
          "guest bootstrap produces a VirtualBox boot context");
    check(guest->device_map.size() == 5, "guest bootstrap carries the VirtualBox device map");
    check(guest->storage.binding_name == "virtualbox-ahci",
          "guest bootstrap binds AHCI storage");
    check(guest->storage.device->info().total_blocks == 18,
          "guest bootstrap storage exposes backing block count");
    check(guest->network.binding_name == "virtualbox-e1000",
          "guest bootstrap binds E1000 networking");
    check(guest->network.device->device_id() == "vbox-e1000",
          "guest bootstrap network device id matches E1000 adapter");
    check(guest->display.binding_name == "virtualbox-vmsvga",
          "guest bootstrap binds VMSVGA display");
    check(guest->display.device->device_id() == "vbox-vmsvga0",
          "guest bootstrap display device id matches VMSVGA adapter");
    check(hal_main(guest->boot_context) == 0,
          "guest bootstrap BootContext is accepted by hal_main");
  }
}

static void test_virtualbox_network_binding() {
  std::printf("\n[AC-15] VirtualBox network binding\n");

  VBoxProfile profile;
  auto binding_err = validate_virtualbox_network_binding(profile);
  check(!binding_err.has_value(), "E1000-first profile has a valid network binding");

  auto binding = create_virtualbox_network_binding(profile);
  check(binding.has_value(), "create_virtualbox_network_binding returns a device");
  if (binding) {
    check(binding->binding_name == "virtualbox-e1000", "binding name identifies E1000 path");
    check(binding->device->device_id() == "vbox-e1000", "bound network device id matches E1000 adapter");
    auto pkt = t81::ternaryos::dev::TernaryEthernetPacket::build(
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
        binding->device->e1000_info().mac,
        0x0081,
        {1, 0, -1});
    check(pkt.has_value(), "network binding can build a ternary packet");
    auto frame = pkt ? binding->device->send_packet(*pkt) : std::nullopt;
    check(frame.has_value(), "bound network device can serialize a packet");
    check(binding->device->tx_frames() == 1, "bound network device records one TX frame");
  }

  VBoxProfile pcnet = profile;
  pcnet.network = VBoxNetwork::PcNet;
  check(validate_virtualbox_network_binding(pcnet).has_value(),
        "PCNet profile rejected until a VirtualBox PCNet adapter exists");
  check(!create_virtualbox_network_binding(pcnet).has_value(),
        "PCNet binding is not created yet");
}

static void test_virtualbox_display_binding() {
  std::printf("\n[AC-16] VirtualBox display binding\n");

  VBoxProfile profile;
  auto binding_err = validate_virtualbox_display_binding(profile);
  check(!binding_err.has_value(), "VMSVGA-first profile has a valid display binding");

  auto binding = create_virtualbox_display_binding(profile);
  check(binding.has_value(), "create_virtualbox_display_binding returns a device");
  if (binding) {
    check(binding->binding_name == "virtualbox-vmsvga", "binding name identifies VMSVGA path");
    check(binding->device->device_id() == "vbox-vmsvga0",
          "bound display device id matches VMSVGA adapter");
    auto& fb = binding->device->framebuffer();
    check(fb.set_pixel(0, 0, t81::ternaryos::dev::TritPixel{1}),
          "bound display framebuffer accepts writes");
    check(binding->device->present(), "bound display device can present a frame");
    check(binding->device->present_count() == 1, "bound display device records one present");
  }

  VBoxProfile vga = profile;
  vga.display = VBoxDisplay::Vga;
  check(validate_virtualbox_display_binding(vga).has_value(),
        "VGA profile rejected until a VirtualBox VGA adapter exists");
  check(!create_virtualbox_display_binding(vga).has_value(),
        "VGA binding is not created yet");
}

static void test_kernel_runtime_bootstrap() {
  std::printf("\n[AC-17] Axion kernel-owned runtime bootstrap\n");

  auto ctx = make_valid_ctx(/*ethics=*/true);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap accepts valid BootContext");
  if (state) {
    check(state->platform_id == "test", "kernel runtime preserves platform_id");
    check(state->memory_region_count == 1, "kernel runtime counts memory regions");
    check(state->total_ternary_pages == ctx.memory_map[0].ternary_page_count(),
          "kernel runtime reports ternary page count");
    check(state->has_writable_memory, "kernel runtime records writable memory");
    check(state->allocator.total_pages() == ctx.memory_map[0].ternary_page_count(),
          "kernel runtime initializes allocator from BootContext");
    check(state->page_table.empty(), "kernel runtime starts with an empty page table");
    check(state->scheduler.thread_count() == 0, "kernel runtime starts with an empty scheduler");
    check(state->ipc_bus.is_registered(KernelRuntimeState::kKernelTid),
          "kernel runtime registers the kernel IPC inbox");
    check(state->ipc_bus.pending(KernelRuntimeState::kKernelTid) == 0,
          "kernel IPC inbox starts empty");
    check(!state->has_device_arbitration(),
          "generic hosted test context does not install device arbitration");
    check(state->fault_count() == 0, "kernel runtime starts with an empty fault log");
  }
  check(axion_kernel_main(ctx) == 0, "axion_kernel_main returns 0 for valid context");
}

static void test_kernel_fault_reporting() {
  std::printf("\n[AC-18] Axion kernel fault reporting consumes MMU faults\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for fault-reporting setup");
  if (!state) {
    return;
  }

  const auto mapped_tva = mmu::tva_from_vpn_offset(2, 7);
  check(mmu::mmu_map(state->page_table, state->allocator, mapped_tva, /*owner_pid=*/1,
                     {.readable = true, .writable = false, .executable = false}),
        "readonly mapping succeeds for kernel fault reporting");

  const auto read_ok =
      axion_kernel_check_access(*state, mapped_tva, mmu::MmuAccessMode::Read);
  check(read_ok.phys_addr.has_value(), "kernel access report returns physical address for valid read");
  check(!read_ok.fault.has_value(), "kernel access report omits fault on valid read");
  check(state->fault_count() == 0, "valid read does not grow the kernel fault log");

  const auto write_fault =
      axion_kernel_check_access(*state, mapped_tva, mmu::MmuAccessMode::Write);
  check(!write_fault.phys_addr.has_value(), "kernel access report suppresses physical address on denied write");
  check(write_fault.fault.has_value(), "kernel access report emits a fault record on denied write");
  if (write_fault.fault) {
    check(write_fault.fault->platform_id == "test", "fault record preserves platform id");
    check(write_fault.fault->tva == mapped_tva, "fault record preserves TVA");
    check(write_fault.fault->access_mode == mmu::MmuAccessMode::Write,
          "fault record preserves access mode");
    check(write_fault.fault->fault == mmu::MmuFault::PermissionDenied,
          "fault record classifies permission denial");
  }
  check(state->fault_count() == 1, "denied write is recorded in the kernel fault log");

  const auto unmapped_tva = mmu::tva_from_vpn_offset(9, 0);
  const auto unmapped_fault =
      axion_kernel_check_access(*state, unmapped_tva, mmu::MmuAccessMode::Read);
  check(unmapped_fault.fault.has_value(), "kernel access report emits fault for unmapped access");
  if (unmapped_fault.fault) {
    check(unmapped_fault.fault->fault == mmu::MmuFault::Unmapped,
          "fault record classifies unmapped access");
  }
  check(state->fault_count() == 2, "unmapped access is recorded in the kernel fault log");

  const auto invalid_fault =
      axion_kernel_check_access(*state, mmu::kMaxTva + 1,
                                mmu::MmuAccessMode::Execute);
  check(invalid_fault.fault.has_value(), "kernel access report emits fault for invalid TVA");
  if (invalid_fault.fault) {
    check(invalid_fault.fault->fault == mmu::MmuFault::InvalidTva,
          "fault record classifies invalid TVA");
  }
  check(state->fault_count() == 3, "invalid TVA is recorded in the kernel fault log");
  check(state->fault_log.back().fault == mmu::MmuFault::InvalidTva,
        "kernel fault log preserves the last classified fault");
}

static void test_kernel_device_arbitration() {
  std::printf("\n[AC-19] Axion kernel runtime installs minimal device arbitration\n");

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;
  auto ctx = make_virtualbox_boot_context(spec);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for VirtualBox device arbitration");
  if (!state) {
    return;
  }

  check(state->has_device_arbitration(),
        "VirtualBox runtime installs device arbitration state");
  if (!state->device_arbitration) {
    return;
  }

  check(state->device_arbitration->profile_summary == "VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC",
        "device arbitration preserves the first-target profile summary");
  check(state->device_arbitration->devices.size() == 5,
        "device arbitration tracks the five primary VirtualBox devices");
  check(state->device_arbitration->has_storage,
        "device arbitration exposes storage ownership");
  check(state->device_arbitration->has_network,
        "device arbitration exposes network ownership");
  check(state->device_arbitration->has_display,
        "device arbitration exposes display ownership");
  check(state->device_arbitration->devices[2].name == "ahci",
        "device arbitration preserves AHCI ordering");
  check(state->device_arbitration->devices[2].irq == 19,
        "device arbitration preserves AHCI IRQ");
  check(state->device_arbitration->devices[3].name == "e1000",
        "device arbitration preserves E1000 ordering");
  check(state->device_arbitration->devices[4].name == "vmsvga",
        "device arbitration preserves VMSVGA ordering");
}

static void test_kernel_runtime_scheduler_and_ipc() {
  std::printf("\n[AC-20] Axion runtime drives scheduler and IPC through owned state\n");

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for runtime scheduler/ipc test");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext thread_a;
  thread_a.label = "A";
  thread_a.registers[0] = 10;

  t81::ternaryos::sched::TiscContext thread_b;
  thread_b.label = "B";
  thread_b.registers[0] = 20;

  auto tid_a = axion_kernel_spawn_thread(*state, thread_a);
  auto tid_b = axion_kernel_spawn_thread(*state, thread_b);
  check(tid_a.has_value(), "kernel runtime spawns thread A");
  check(tid_b.has_value(), "kernel runtime spawns thread B");
  if (!tid_a || !tid_b) {
    return;
  }

  check(state->scheduler.thread_count() == 2, "runtime-owned scheduler tracks two threads");
  check(state->ipc_bus.is_registered(*tid_a), "runtime-owned IPC registers thread A inbox");
  check(state->ipc_bus.is_registered(*tid_b), "runtime-owned IPC registers thread B inbox");

  check(axion_kernel_tick(*state), "first kernel tick dispatches the first runnable thread");
  check(state->scheduler.current_tid() == *tid_a, "runtime tick makes thread A current");
  check(state->cpu_context.registers[0] == 10, "runtime CPU context restores thread A register state");

  check(axion_kernel_tick(*state), "second kernel tick switches to thread B");
  check(state->scheduler.current_tid() == *tid_b, "runtime tick advances to thread B");
  check(state->cpu_context.registers[0] == 20, "runtime CPU context restores thread B register state");

  t81::ternaryos::ipc::CanonMessage msg;
  msg.sender = *tid_a;
  msg.ref.hash.h.bytes.fill(0x2A);
  msg.payload = 81;
  msg.tag = "kernel-msg";

  check(axion_kernel_ipc_send(*state, *tid_b, msg), "runtime-owned IPC sends CanonRef message");
  check(state->ipc_bus.pending(*tid_b) == 1, "thread B inbox sees one pending message");

  auto recv = axion_kernel_ipc_recv(*state, *tid_b);
  check(recv.has_value(), "runtime-owned IPC receives the queued message");
  if (recv) {
    check(recv->sender == *tid_a, "received message preserves sender Tid");
    check(recv->payload == 81, "received message preserves payload");
    check(recv->tag == "kernel-msg", "received message preserves tag");
  }
  check(state->ipc_bus.pending(*tid_b) == 0, "thread B inbox drains after receive");
  check(state->counters.scheduler_ticks == 2, "runtime-owned scheduler counts ticks");
  check(state->counters.scheduler_switches == 2, "runtime-owned scheduler counts switches");
  check(state->counters.ipc_messages_sent == 1, "runtime-owned IPC counts sent messages");
  check(state->counters.ipc_messages_received == 1, "runtime-owned IPC counts received messages");
}

static void test_kernel_loop_and_active_device_arbitration() {
  std::printf("\n[AC-21] Axion kernel loop and active device arbitration\n");

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;
  auto ctx = make_virtualbox_boot_context(spec);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for loop/arbitration test");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext thread_a;
  thread_a.label = "loop-a";
  thread_a.registers[0] = 31;
  t81::ternaryos::sched::TiscContext thread_b;
  thread_b.label = "loop-b";
  thread_b.registers[0] = 47;

  auto tid_a = axion_kernel_spawn_thread(*state, thread_a);
  auto tid_b = axion_kernel_spawn_thread(*state, thread_b);
  check(tid_a.has_value(), "loop runtime spawns thread A");
  check(tid_b.has_value(), "loop runtime spawns thread B");
  if (!tid_a || !tid_b) {
    return;
  }

  check(axion_kernel_step(*state), "first kernel step dispatches a thread");
  check(state->counters.loop_iterations == 1, "kernel step counts loop iterations");
  check(state->scheduler.current_tid() == *tid_a, "first kernel step selects thread A");
  check(axion_kernel_step(*state), "second kernel step advances the runtime");
  check(state->counters.loop_iterations == 2, "second kernel step increments loop iterations");
  check(state->scheduler.current_tid() == *tid_b, "second kernel step selects thread B");

  check(axion_kernel_claim_device(*state, "ahci", *tid_a),
        "first thread claims AHCI device");
  check(!axion_kernel_claim_device(*state, "ahci", *tid_b),
        "second thread cannot steal claimed AHCI device");
  check(axion_kernel_claim_device(*state, "ahci", *tid_a),
        "same owner can idempotently re-claim AHCI device");
  check(axion_kernel_release_device(*state, "ahci", *tid_b) == false,
        "non-owner cannot release AHCI device");
  check(axion_kernel_release_device(*state, "ahci", *tid_a),
        "owner releases AHCI device");
  check(axion_kernel_claim_device(*state, "ahci", *tid_b),
        "second thread can claim AHCI after release");

  bool saw_ahci_owner_b = false;
  if (state->device_arbitration) {
    for (const auto& device : state->device_arbitration->devices) {
      if (device.name == "ahci" && device.owner_tid == *tid_b) {
        saw_ahci_owner_b = true;
      }
    }
  }
  check(saw_ahci_owner_b, "device arbitration records active AHCI owner");
}

static void test_kernel_loop_fault_delivery() {
  std::printf("\n[AC-22] Axion kernel loop delivers recorded faults deterministically\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for loop fault delivery");
  if (!state) {
    return;
  }

  auto first_fault =
      axion_kernel_check_access(*state, mmu::tva_from_vpn_offset(11, 0), mmu::MmuAccessMode::Read);
  auto second_fault =
      axion_kernel_check_access(*state, mmu::kMaxTva + 1, mmu::MmuAccessMode::Execute);
  check(first_fault.fault.has_value(), "unmapped access records first pending fault");
  check(second_fault.fault.has_value(), "invalid TVA records second pending fault");
  check(state->fault_count() == 2, "fault log records both faults");
  check(state->pending_fault_count() == 2, "pending fault queue tracks both faults");
  check(state->counters.faults_recorded == 2, "runtime counts recorded faults");

  check(!axion_kernel_step(*state), "kernel step can deliver pending fault without runnable threads");
  check(state->counters.loop_iterations == 1, "fault-delivery step increments loop count");
  check(state->pending_fault_count() == 1, "first loop step drains one pending fault");
  check(state->counters.faults_delivered == 1, "runtime counts delivered faults");
  check(state->last_delivered_fault.has_value(), "loop step exposes delivered fault");
  if (state->last_delivered_fault) {
    check(state->last_delivered_fault->fault == mmu::MmuFault::Unmapped,
          "first delivered fault preserves insertion order");
  }

  check(!axion_kernel_step(*state), "second loop step delivers next pending fault");
  check(state->counters.loop_iterations == 2, "second fault-delivery step increments loop count");
  check(state->pending_fault_count() == 0, "second loop step drains remaining pending fault");
  check(state->counters.faults_delivered == 2, "runtime counts both delivered faults");
  check(state->last_delivered_fault.has_value(), "second delivered fault is exposed");
  if (state->last_delivered_fault) {
    check(state->last_delivered_fault->fault == mmu::MmuFault::InvalidTva,
          "second delivered fault preserves insertion order");
  }

  check(!axion_kernel_step(*state), "idle loop step runs with no pending faults");
  check(!state->last_delivered_fault.has_value(), "idle loop step clears delivered-fault slot");
}

static void test_kernel_fault_process_boundary() {
  std::printf("\n[AC-23] Axion kernel loop routes faults into thread runtime state\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for thread fault routing");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext thread_a;
  thread_a.label = "fault-a";
  thread_a.registers[0] = 91;
  t81::ternaryos::sched::TiscContext thread_b;
  thread_b.label = "fault-b";
  thread_b.registers[0] = 123;

  auto tid_a = axion_kernel_spawn_thread(*state, thread_a);
  auto tid_b = axion_kernel_spawn_thread(*state, thread_b);
  check(tid_a.has_value(), "fault-routing runtime spawns thread A");
  check(tid_b.has_value(), "fault-routing runtime spawns thread B");
  if (!tid_a || !tid_b) {
    return;
  }

  check(axion_kernel_step(*state), "first kernel step dispatches thread A");
  check(state->scheduler.current_tid() == *tid_a, "thread A is current before access fault");

  const auto fault_report = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(27, 0), mmu::MmuAccessMode::Read);
  check(fault_report.fault.has_value(), "unmapped read records a fault for the running thread");
  if (fault_report.fault) {
    check(fault_report.fault->subject_tid == *tid_a,
          "fault record tags the current running thread");
  }
  check(state->pending_fault_count() == 1, "fault-routing runtime queues the pending fault");

  check(axion_kernel_step(*state), "second kernel step delivers fault and continues runtime");
  check(state->pending_fault_count() == 0, "delivered fault drains from pending queue");
  check(state->counters.faults_routed_to_threads == 1,
        "runtime counts faults routed to thread boundary");
  check(state->counters.thread_quarantines == 1,
        "runtime counts quarantined faulting threads");
  check(state->scheduler.current_tid() == *tid_b,
        "scheduler advances to thread B after quarantining thread A");

  const auto* runtime_a = state->find_thread_runtime(*tid_a);
  const auto* runtime_b = state->find_thread_runtime(*tid_b);
  check(runtime_a != nullptr, "thread A runtime state exists");
  check(runtime_b != nullptr, "thread B runtime state exists");
  if (runtime_a) {
    check(runtime_a->quarantined, "faulting thread is quarantined");
    check(runtime_a->fault_inbox.size() == 1, "faulting thread inbox receives delivered fault");
    if (!runtime_a->fault_inbox.empty()) {
      check(runtime_a->fault_inbox.front().fault == mmu::MmuFault::Unmapped,
            "thread inbox preserves delivered fault classification");
      check(runtime_a->fault_inbox.front().subject_tid == *tid_a,
            "thread inbox preserves delivered fault subject");
    }
  }
  if (runtime_b) {
    check(!runtime_b->quarantined, "non-faulting thread remains runnable");
    check(runtime_b->fault_inbox.empty(), "non-faulting thread inbox remains empty");
  }
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
  test_hal_c_abi_bridge();
  test_virtualbox_profile_defaults();
  test_virtualbox_profile_rejects_nvme_without_opt_in();
  test_virtualbox_boot_context();
  test_virtualbox_device_map_defaults();
  test_virtualbox_timer_tick_scaffold();
  test_virtualbox_storage_binding();
  test_virtualbox_guest_bootstrap();
  test_virtualbox_network_binding();
  test_virtualbox_display_binding();
  test_kernel_runtime_bootstrap();
  test_kernel_fault_reporting();
  test_kernel_device_arbitration();
  test_kernel_runtime_scheduler_and_ipc();
  test_kernel_loop_and_active_device_arbitration();
  test_kernel_loop_fault_delivery();
  test_kernel_fault_process_boundary();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
