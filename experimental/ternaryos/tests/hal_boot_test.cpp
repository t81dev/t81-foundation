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
    check(state->process_group_count() == 1, "kernel runtime starts with one kernel process group");
    check(state->address_space_count() == 1,
          "kernel runtime starts with one kernel address space");
    const auto* kernel_runtime = state->find_thread_runtime(KernelRuntimeState::kKernelTid);
    check(kernel_runtime != nullptr, "kernel thread runtime state exists");
    if (kernel_runtime) {
      check(kernel_runtime->process_group_id == KernelRuntimeState::kKernelProcessGroup,
            "kernel thread belongs to the kernel process group");
    }
    check(state->find_process_group_address_space(KernelRuntimeState::kKernelProcessGroup) ==
              KernelRuntimeState::kKernelAddressSpace,
          "kernel process group resolves to the kernel address space");
    check(!state->has_device_arbitration(),
          "generic hosted test context does not install device arbitration");
    check(state->fault_count() == 0, "kernel runtime starts with an empty fault log");
    check(state->audit_count() == 0, "kernel runtime starts with an empty audit log");
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
  const auto* runtime_a = state->find_thread_runtime(*tid_a);
  const auto* runtime_b = state->find_thread_runtime(*tid_b);
  check(runtime_a != nullptr, "thread A runtime state exists");
  check(runtime_b != nullptr, "thread B runtime state exists");
  if (runtime_a && runtime_b) {
    check(runtime_a->process_group_id != runtime_b->process_group_id,
          "threads spawned independently receive distinct process groups");
    check(state->find_process_group(runtime_a->process_group_id) != nullptr,
          "thread A process group exists");
    check(state->find_process_group(runtime_b->process_group_id) != nullptr,
          "thread B process group exists");
    const auto address_space_a =
        state->find_process_group_address_space(runtime_a->process_group_id);
    const auto address_space_b =
        state->find_process_group_address_space(runtime_b->process_group_id);
    check(address_space_a.has_value(), "thread A process group resolves to an address space");
    check(address_space_b.has_value(), "thread B process group resolves to an address space");
    if (address_space_a && address_space_b) {
      check(*address_space_a != *address_space_b,
            "independent process groups receive distinct address spaces");
    }
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

static void test_kernel_interrupt_event_delivery() {
  std::printf("\n[AC-22a] Axion kernel records and delivers interrupt events deterministically\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for interrupt delivery");
  if (!state) {
    return;
  }

  check(axion_kernel_record_interrupt(
            *state,
            HardwareInterrupt{
                .source = InterruptSource::Timer,
                .timestamp_ns = 100,
                .payload = 11,
            }),
        "kernel records first interrupt event");
  check(axion_kernel_record_interrupt(
            *state,
            HardwareInterrupt{
                .source = InterruptSource::Storage,
                .timestamp_ns = 200,
                .payload = 22,
            }),
        "kernel records second interrupt event");
  check(state->pending_interrupt_count() == 2,
        "pending interrupt queue tracks both recorded interrupts");
  check(state->pending_interrupt_high_watermark == 2,
        "interrupt queue retains pending high-water mark");
  check(state->counters.interrupts_recorded == 2,
        "runtime counts recorded interrupts");
  check(state->counters.interrupt_sources_recorded.timer == 1,
        "runtime counts one recorded timer interrupt");
  check(state->counters.interrupt_sources_recorded.storage == 1,
        "runtime counts one recorded storage interrupt");
  check(state->last_recorded_interrupt.has_value(),
        "interrupt intake retains latest recorded interrupt");
  if (state->last_recorded_interrupt) {
    check(state->last_recorded_interrupt->source == InterruptSource::Storage,
          "interrupt intake retains the latest recorded source");
    check(state->last_recorded_interrupt->sequence == 2,
          "interrupt intake retains the latest recorded sequence");
  }

  check(axion_kernel_step(*state), "interrupt step delivers the first pending interrupt");
  check(state->pending_interrupt_count() == 1,
        "interrupt step drains one pending interrupt");
  check(state->counters.interrupts_delivered == 1,
        "runtime counts first delivered interrupt");
  check(state->counters.interrupt_sources_delivered.timer == 1,
        "runtime counts one delivered timer interrupt");
  check(state->last_delivered_interrupt.has_value(),
        "interrupt step exposes the delivered interrupt");
  if (state->last_delivered_interrupt) {
    check(state->last_delivered_interrupt->source == InterruptSource::Timer,
          "first delivered interrupt preserves FIFO source order");
    check(state->last_delivered_interrupt->payload == 11,
          "first delivered interrupt preserves payload");
    check(state->last_delivered_interrupt->sequence == 1,
          "first delivered interrupt preserves intake sequence");
  }

  auto fault =
      axion_kernel_check_access(*state, mmu::tva_from_vpn_offset(23, 0), mmu::MmuAccessMode::Read);
  check(fault.fault.has_value(), "unmapped access records a fault alongside pending interrupts");
  check(state->pending_fault_count() == 1, "fault queue records the interrupt-priority test fault");
  check(state->pending_interrupt_count() == 1,
        "interrupt queue remains pending before fault-priority step");

  check(!axion_kernel_step(*state), "fault step keeps fault priority over pending interrupts");
  check(state->pending_fault_count() == 0, "fault-priority step drains the pending fault first");
  check(state->pending_interrupt_count() == 1,
        "fault-priority step leaves the pending interrupt queued");
  check(!state->last_delivered_interrupt.has_value(),
        "fault-priority step clears stale delivered-interrupt state");
  check(state->counters.interrupts_delivered == 1,
        "fault-priority step does not advance interrupt deliveries");

  check(axion_kernel_step(*state), "next interrupt step delivers the remaining pending interrupt");
  check(state->pending_interrupt_count() == 0,
        "second interrupt step drains the remaining pending interrupt");
  check(state->counters.interrupts_delivered == 2,
        "runtime counts both delivered interrupts");
  check(state->counters.interrupt_sources_delivered.storage == 1,
        "runtime counts one delivered storage interrupt");
  check(state->last_delivered_interrupt.has_value(),
        "second interrupt step exposes the remaining delivered interrupt");
  if (state->last_delivered_interrupt) {
    check(state->last_delivered_interrupt->source == InterruptSource::Storage,
          "second delivered interrupt preserves FIFO source order");
    check(state->last_delivered_interrupt->payload == 22,
          "second delivered interrupt preserves payload");
    check(state->last_delivered_interrupt->sequence == 2,
          "second delivered interrupt preserves intake sequence");
  }

  auto runtime_status = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_status.status == KernelServiceStatus::Ok,
        "runtime status succeeds after interrupt delivery");
  check(runtime_status.runtime.has_value(),
        "runtime status returns interrupt delivery details");
  if (runtime_status.runtime) {
    check(runtime_status.runtime->pending_interrupt_count == 0,
          "runtime status reports no pending interrupts after delivery");
    check(runtime_status.runtime->pending_interrupt_high_watermark == 2,
          "runtime status retains interrupt queue high-water mark");
    check(runtime_status.runtime->interrupts_recorded == 2,
          "runtime status reports recorded interrupt count");
    check(runtime_status.runtime->interrupts_delivered == 2,
          "runtime status reports delivered interrupt count");
    check(runtime_status.runtime->interrupt_sources_recorded.timer == 1,
          "runtime status reports recorded timer interrupt count");
    check(runtime_status.runtime->interrupt_sources_recorded.storage == 1,
          "runtime status reports recorded storage interrupt count");
    check(runtime_status.runtime->interrupt_sources_delivered.timer == 1,
          "runtime status reports delivered timer interrupt count");
    check(runtime_status.runtime->interrupt_sources_delivered.storage == 1,
          "runtime status reports delivered storage interrupt count");
    check(!runtime_status.runtime->next_pending_interrupt.has_value(),
          "runtime status clears next pending interrupt after delivery");
    check(runtime_status.runtime->last_recorded_interrupt.has_value(),
          "runtime status retains latest recorded interrupt");
    check(runtime_status.runtime->last_delivered_interrupt.has_value(),
          "runtime status exposes the latest delivered interrupt");
    if (runtime_status.runtime->last_recorded_interrupt) {
      check(runtime_status.runtime->last_recorded_interrupt->source ==
                InterruptSource::Storage,
            "runtime status retains the latest recorded interrupt source");
    }
    if (runtime_status.runtime->last_delivered_interrupt) {
      check(runtime_status.runtime->last_delivered_interrupt->source ==
                InterruptSource::Storage,
            "runtime status retains the latest delivered interrupt source");
    }
  }

  auto fault_summary = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_summary.status == KernelServiceStatus::Ok,
        "fault summary succeeds after interrupt delivery");
  check(fault_summary.fault_summary.has_value(),
        "fault summary returns interrupt delivery details");
  if (fault_summary.fault_summary) {
    check(fault_summary.fault_summary->pending_interrupts == 0,
          "fault summary reports no pending interrupts after delivery");
    check(fault_summary.fault_summary->pending_interrupt_high_watermark == 2,
          "fault summary retains interrupt queue high-water mark");
    check(fault_summary.fault_summary->interrupts_recorded == 2,
          "fault summary reports recorded interrupt count");
    check(fault_summary.fault_summary->interrupts_delivered == 2,
          "fault summary reports delivered interrupt count");
    check(fault_summary.fault_summary->interrupt_sources_recorded.timer == 1,
          "fault summary reports recorded timer interrupt count");
    check(fault_summary.fault_summary->interrupt_sources_recorded.storage == 1,
          "fault summary reports recorded storage interrupt count");
    check(fault_summary.fault_summary->interrupt_sources_delivered.timer == 1,
          "fault summary reports delivered timer interrupt count");
    check(fault_summary.fault_summary->interrupt_sources_delivered.storage == 1,
          "fault summary reports delivered storage interrupt count");
    check(!fault_summary.fault_summary->next_pending_interrupt.has_value(),
          "fault summary clears next pending interrupt after delivery");
    check(fault_summary.fault_summary->last_recorded_interrupt.has_value(),
          "fault summary retains latest recorded interrupt");
    check(fault_summary.fault_summary->last_delivered_interrupt.has_value(),
          "fault summary exposes the latest delivered interrupt");
  }

  auto audit_summary = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::AuditSummary});
  check(audit_summary.status == KernelServiceStatus::Ok,
        "audit summary succeeds after interrupt delivery");
  check(audit_summary.audit_summary.has_value(),
        "audit summary returns interrupt audit details");
  if (audit_summary.audit_summary) {
    check(audit_summary.audit_summary->interrupt_deliveries == 2,
          "audit summary reports interrupt delivery count");
    check(audit_summary.audit_summary->interrupt_sources_recorded.timer == 1,
          "audit summary reports recorded timer interrupt count");
    check(audit_summary.audit_summary->interrupt_sources_recorded.storage == 1,
          "audit summary reports recorded storage interrupt count");
    check(audit_summary.audit_summary->interrupt_sources_delivered.timer == 1,
          "audit summary reports delivered timer interrupt count");
    check(audit_summary.audit_summary->interrupt_sources_delivered.storage == 1,
          "audit summary reports delivered storage interrupt count");
    check(audit_summary.audit_summary->last_recorded_interrupt.has_value(),
          "audit summary retains the latest recorded interrupt");
    check(audit_summary.audit_summary->last_delivered_interrupt.has_value(),
          "audit summary exposes the latest delivered interrupt");
    check(!audit_summary.audit_summary->recent_events.empty(),
          "audit summary retains recent interrupt audit events");
    if (!audit_summary.audit_summary->recent_events.empty()) {
      check(audit_summary.audit_summary->recent_events.back().kind ==
                KernelAuditEventKind::InterruptDelivered,
            "audit summary records interrupt delivery as the latest audit event");
    }
  }

  auto queued_state = axion_kernel_bootstrap(ctx);
  check(queued_state.has_value(), "kernel bootstrap succeeds for queued interrupt visibility");
  if (queued_state) {
    check(axion_kernel_record_interrupt(
              *queued_state,
              HardwareInterrupt{
                  .source = InterruptSource::Keyboard,
                  .timestamp_ns = 300,
                  .payload = 33,
              }),
          "queued interrupt visibility records first interrupt");
    check(axion_kernel_record_interrupt(
              *queued_state,
              HardwareInterrupt{
                  .source = InterruptSource::Network,
                  .timestamp_ns = 400,
                  .payload = 44,
              }),
          "queued interrupt visibility records second interrupt");
    auto queued_runtime = axion_kernel_service_request(
        *queued_state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
    check(queued_runtime.status == KernelServiceStatus::Ok,
          "runtime status succeeds with queued interrupts");
    check(queued_runtime.runtime.has_value(),
          "runtime status returns queued interrupt visibility");
    if (queued_runtime.runtime) {
      check(queued_runtime.runtime->pending_interrupt_count == 2,
            "runtime status reports queued interrupt count before delivery");
      check(queued_runtime.runtime->interrupt_sources_recorded.keyboard == 1,
            "runtime status reports recorded keyboard interrupt count before delivery");
      check(queued_runtime.runtime->interrupt_sources_recorded.network == 1,
            "runtime status reports recorded network interrupt count before delivery");
      check(queued_runtime.runtime->interrupt_sources_delivered.keyboard == 0,
            "runtime status reports zero delivered keyboard interrupts before delivery");
      check(queued_runtime.runtime->next_pending_interrupt.has_value(),
            "runtime status exposes next pending interrupt before delivery");
      if (queued_runtime.runtime->next_pending_interrupt) {
        check(queued_runtime.runtime->next_pending_interrupt->source ==
                  InterruptSource::Keyboard,
              "runtime status preserves FIFO head interrupt source");
        check(queued_runtime.runtime->next_pending_interrupt->sequence == 1,
              "runtime status preserves FIFO head interrupt sequence");
      }
      check(queued_runtime.runtime->last_recorded_interrupt.has_value(),
            "runtime status retains latest recorded interrupt before delivery");
      if (queued_runtime.runtime->last_recorded_interrupt) {
        check(queued_runtime.runtime->last_recorded_interrupt->source ==
                  InterruptSource::Network,
              "runtime status preserves latest recorded interrupt before delivery");
      }
    }
  }
}

static void test_kernel_pager_fault_state() {
  std::printf("\n[AC-22b] Axion kernel classifies pager-needed versus policy faults\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for pager fault-state test");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext pager_thread;
  pager_thread.label = "pager-fault";
  pager_thread.registers[0] = 211;
  t81::ternaryos::sched::TiscContext policy_thread;
  policy_thread.label = "policy-fault";
  policy_thread.registers[0] = 212;

  auto pager_tid = axion_kernel_spawn_thread(*state, pager_thread);
  auto policy_tid = axion_kernel_spawn_thread(*state, policy_thread);
  check(pager_tid.has_value(), "pager-fault thread spawns successfully");
  check(policy_tid.has_value(), "policy-fault thread spawns successfully");
  if (!pager_tid || !policy_tid) {
    return;
  }

  const auto* pager_runtime = state->find_thread_runtime(*pager_tid);
  const auto* policy_runtime = state->find_thread_runtime(*policy_tid);
  check(pager_runtime != nullptr, "pager-fault runtime state exists");
  check(policy_runtime != nullptr, "policy-fault runtime state exists");
  if (!pager_runtime || !policy_runtime) {
    return;
  }

  const auto pager_group_id = pager_runtime->process_group_id;
  const auto policy_group_id = policy_runtime->process_group_id;
  const auto pager_address_space_id =
      state->find_process_group_address_space(pager_group_id);
  const auto policy_address_space_id =
      state->find_process_group_address_space(policy_group_id);
  check(pager_address_space_id.has_value(),
        "pager-fault group resolves to an address space");
  check(policy_address_space_id.has_value(),
        "policy-fault group resolves to an address space");
  if (!pager_address_space_id || !policy_address_space_id) {
    return;
  }

  const auto readonly_tva = mmu::tva_from_vpn_offset(62, 0);
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     readonly_tva,
                     *policy_address_space_id,
                     {.readable = true, .writable = false, .executable = false}),
        "policy-fault address space accepts a readonly mapping");

  check(axion_kernel_step(*state), "pager fault-state step dispatches pager thread");
  check(state->scheduler.current_tid() == *pager_tid,
        "pager-fault thread becomes current first");
  auto pager_fault = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(61, 0), mmu::MmuAccessMode::Read);
  check(pager_fault.fault.has_value(), "unmapped read records a pager-eligible fault");
  check(axion_kernel_step(*state), "pager fault-state step delivers pager-needed fault");
  check(state->scheduler.current_tid() == *policy_tid,
        "policy thread becomes current after pager thread quarantine");

  auto policy_fault =
      axion_kernel_check_access(*state, readonly_tva, mmu::MmuAccessMode::Write);
  check(policy_fault.fault.has_value(), "readonly write records a policy fault");
  check(axion_kernel_step(*state), "pager fault-state step delivers policy fault");

  auto runtime_status = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_status.status == KernelServiceStatus::Ok,
        "runtime status succeeds after mixed fault classification");
  check(runtime_status.runtime.has_value(),
        "runtime status returns pager fault-state view");
  if (runtime_status.runtime) {
    check(runtime_status.runtime->pager_needed_address_space_count == 1,
          "runtime status reports one pager-needed address space");
    check(runtime_status.runtime->pending_pager_handoff_count == 1,
          "runtime status reports one pending pager handoff before dispatch");
    check(runtime_status.runtime->pager_worker_inbox_count == 0,
          "runtime status starts with empty pager worker inbox");
    check(!runtime_status.runtime->pager_worker_busy,
          "runtime status starts with idle pager worker");
    check(runtime_status.runtime->pager_eligible_faults == 1,
          "runtime status counts one pager-eligible fault");
    check(runtime_status.runtime->policy_faults == 1,
          "runtime status counts one policy fault");
    check(runtime_status.runtime->pager_handoffs_dispatched == 0,
          "runtime status starts with zero dispatched pager handoffs");
  }

  auto pager_group_status = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = pager_group_id,
      });
  check(pager_group_status.status == KernelServiceStatus::FaultedGroup,
        "pager-needed process group remains faulted until acknowledgement");
  check(pager_group_status.process_group.has_value(),
        "pager-needed process group view is returned");
  if (pager_group_status.process_group) {
    check(pager_group_status.process_group->address_space_id == pager_address_space_id,
          "pager-needed process group preserves backing address space");
    check(pager_group_status.process_group->pager_needed,
          "pager-needed process group view reports pager-needed state");
    check(pager_group_status.process_group->pager_handoff_pending,
          "pager-needed process group view reports pending pager handoff");
    check(pager_group_status.process_group->pending_pager_fault_count == 1,
          "pager-needed process group view counts one pending pager fault");
    check(pager_group_status.process_group->pager_faults == 1,
          "pager-needed process group view counts one pager fault");
    check(pager_group_status.process_group->pager_handoffs == 0,
          "pager-needed process group view starts with zero dispatched handoffs");
    check(pager_group_status.process_group->last_pager_fault.has_value(),
          "pager-needed process group view exposes the last pager fault");
    if (pager_group_status.process_group->last_pager_fault) {
      check(pager_group_status.process_group->last_pager_fault->fault ==
                mmu::MmuFault::Unmapped,
            "pager-needed process group view preserves unmapped classification");
    }
  }

  auto policy_group_status = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = policy_group_id,
      });
  check(policy_group_status.status == KernelServiceStatus::FaultedGroup,
        "policy-fault process group also enters the fault boundary");
  check(policy_group_status.process_group.has_value(),
        "policy-fault process group view is returned");
  if (policy_group_status.process_group) {
    check(!policy_group_status.process_group->pager_needed,
          "policy-fault process group does not report pager-needed state");
    check(!policy_group_status.process_group->pager_handoff_pending,
          "policy-fault process group reports no pager handoff pending");
    check(policy_group_status.process_group->pending_pager_fault_count == 0,
          "policy-fault process group reports zero pending pager faults");
    check(policy_group_status.process_group->pager_faults == 0,
          "policy-fault process group reports zero pager faults");
    check(!policy_group_status.process_group->last_pager_fault.has_value(),
          "policy-fault process group exposes no pager fault record");
  }

  auto fault_summary = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_summary.status == KernelServiceStatus::Ok,
        "fault summary succeeds after mixed fault classification");
  check(fault_summary.fault_summary.has_value(),
        "fault summary returns mixed fault-state detail");
  if (fault_summary.fault_summary) {
    check(fault_summary.fault_summary->pager_eligible_faults == 1,
          "fault summary counts one pager-eligible fault");
    check(fault_summary.fault_summary->policy_faults == 1,
          "fault summary counts one policy fault");
    check(fault_summary.fault_summary->pager_needed_address_spaces == 1,
          "fault summary reports one pager-needed address space");
    check(fault_summary.fault_summary->pending_pager_handoffs == 1,
          "fault summary reports one pending pager handoff before dispatch");
    check(fault_summary.fault_summary->pager_handoffs_dispatched == 0,
          "fault summary starts with zero dispatched pager handoffs");
    check(fault_summary.fault_summary->last_pager_address_space_id ==
              pager_address_space_id,
          "fault summary tracks the latest pager-needed address space");
    check(fault_summary.fault_summary->last_pager_fault.has_value(),
          "fault summary exposes the latest pager-needed fault");
    if (fault_summary.fault_summary->last_pager_fault) {
      check(fault_summary.fault_summary->last_pager_fault->fault ==
                mmu::MmuFault::Unmapped,
            "fault summary preserves unmapped pager-needed classification");
    }
    check(fault_summary.fault_summary->last_delivered_fault.has_value(),
          "fault summary still exposes the latest delivered fault");
    if (fault_summary.fault_summary->last_delivered_fault) {
      check(fault_summary.fault_summary->last_delivered_fault->fault ==
                mmu::MmuFault::PermissionDenied,
            "fault summary keeps latest delivered policy fault separate from pager state");
    }
    check(!fault_summary.fault_summary->last_pager_handoff.has_value(),
          "fault summary starts without a dispatched pager handoff record");
  }

  (void)axion_kernel_step(*state);
  check(true, "later kernel step dispatches one pending internal pager handoff");

  auto runtime_status_after_handoff = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_status_after_handoff.status == KernelServiceStatus::Ok,
        "runtime status succeeds after pager handoff dispatch");
  check(runtime_status_after_handoff.runtime.has_value(),
        "runtime status returns pager handoff detail");
  if (runtime_status_after_handoff.runtime) {
    check(runtime_status_after_handoff.runtime->pager_needed_address_space_count == 1,
          "runtime status retains pager-needed address space after handoff");
    check(runtime_status_after_handoff.runtime->pending_pager_handoff_count == 0,
          "runtime status clears pending pager handoff after dispatch");
    check(runtime_status_after_handoff.runtime->pager_worker_inbox_count == 1,
          "runtime status queues one handoff into pager worker inbox");
    check(!runtime_status_after_handoff.runtime->pager_worker_busy,
          "runtime status keeps pager worker idle until the next worker step");
    check(runtime_status_after_handoff.runtime->pager_handoffs_dispatched == 1,
          "runtime status counts one dispatched pager handoff");
    check(runtime_status_after_handoff.runtime->pager_resolutions == 0,
          "runtime status starts with zero pager resolutions after handoff");
    check(runtime_status_after_handoff.runtime->pager_faults_coalesced == 0,
          "runtime status starts with zero coalesced pager faults after first handoff");
    check(runtime_status_after_handoff.runtime->pager_worker_handoffs_received == 1,
          "runtime status counts one handoff received by pager worker");
  }

  auto pager_group_after_handoff = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = pager_group_id,
      });
  check(pager_group_after_handoff.process_group.has_value(),
        "pager-needed process group view remains available after handoff");
  if (pager_group_after_handoff.process_group) {
    check(!pager_group_after_handoff.process_group->pager_handoff_pending,
          "pager-needed process group clears pending handoff after dispatch");
    check(pager_group_after_handoff.process_group->pager_worker_owned,
          "pager-needed process group reports worker-owned pager state after handoff");
    check(pager_group_after_handoff.process_group->pager_handoffs == 1,
          "pager-needed process group counts one dispatched handoff");
    check(pager_group_after_handoff.process_group->pager_needed,
          "pager-needed process group remains pager-needed after handoff");
    check(pager_group_after_handoff.process_group->pager_resolutions == 0,
          "pager-needed process group starts with zero resolutions after handoff");
    check(pager_group_after_handoff.process_group->pager_faults_coalesced == 0,
          "pager-needed process group starts with zero coalesced faults after handoff");
  }

  auto fault_summary_after_handoff = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_summary_after_handoff.status == KernelServiceStatus::Ok,
        "fault summary succeeds after pager handoff dispatch");
  check(fault_summary_after_handoff.fault_summary.has_value(),
        "fault summary returns pager handoff state");
  if (fault_summary_after_handoff.fault_summary) {
    check(fault_summary_after_handoff.fault_summary->pending_pager_handoffs == 0,
          "fault summary clears pending pager handoff count after dispatch");
    check(fault_summary_after_handoff.fault_summary->pager_handoffs_dispatched == 1,
          "fault summary counts one dispatched pager handoff");
    check(fault_summary_after_handoff.fault_summary->last_pager_handoff.has_value(),
          "fault summary exposes the latest dispatched pager handoff");
    if (fault_summary_after_handoff.fault_summary->last_pager_handoff) {
      check(fault_summary_after_handoff.fault_summary->last_pager_handoff->address_space_id ==
                *pager_address_space_id,
            "fault summary tracks the handed-off pager address space");
      check(fault_summary_after_handoff.fault_summary->last_pager_handoff->fault.fault ==
                mmu::MmuFault::Unmapped,
            "fault summary preserves unmapped classification in handoff record");
    }
    check(!fault_summary_after_handoff.fault_summary->last_pager_resolution.has_value(),
          "fault summary starts without a pager resolution record");
  }

  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = mmu::tva_from_vpn_offset(61, 0),
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *pager_tid,
  });
  check(true, "same unresolved address space can fault again while worker owns it");
  (void)axion_kernel_step(*state);

  auto runtime_status_after_coalesce = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_status_after_coalesce.status == KernelServiceStatus::Ok,
        "runtime status succeeds after coalesced pager fault");
  check(runtime_status_after_coalesce.runtime.has_value(),
        "runtime status exposes coalesced pager fault state");
  if (runtime_status_after_coalesce.runtime) {
    check(runtime_status_after_coalesce.runtime->pending_pager_handoff_count == 0,
          "runtime status keeps pending pager handoff count clear after coalescing");
    check(runtime_status_after_coalesce.runtime->pager_worker_inbox_count == 1,
          "runtime status retains one queued worker item during coalescing");
    check(runtime_status_after_coalesce.runtime->pager_handoffs_dispatched == 1,
          "runtime status does not dispatch a duplicate pager handoff");
    check(runtime_status_after_coalesce.runtime->pager_faults_coalesced == 1,
          "runtime status counts one coalesced pager fault");
  }

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     mmu::tva_from_vpn_offset(61, 0),
                     *pager_address_space_id),
        "pager-needed address space accepts a mapping for the missing page");
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  auto runtime_status_after_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_status_after_resolution.status == KernelServiceStatus::Ok,
        "runtime status succeeds after pager resolution");
  check(runtime_status_after_resolution.runtime.has_value(),
        "runtime status returns pager resolution detail");
  if (runtime_status_after_resolution.runtime) {
    check(runtime_status_after_resolution.runtime->pager_needed_address_space_count == 0,
          "runtime status clears pager-needed address-space count after resolution");
    check(runtime_status_after_resolution.runtime->pager_handoffs_dispatched == 1,
          "runtime status retains dispatched pager handoff count after resolution");
    check(runtime_status_after_resolution.runtime->pager_resolutions == 1,
          "runtime status counts one pager resolution");
    check(runtime_status_after_resolution.runtime->pager_worker_inbox_count == 0,
          "runtime status keeps pager worker inbox empty after resolution");
    check(!runtime_status_after_resolution.runtime->pager_worker_busy,
          "runtime status reports idle pager worker after resolution");
    check(runtime_status_after_resolution.runtime->pager_worker_handoffs_received == 1,
          "runtime status retains pager worker handoff count after resolution");
    check(runtime_status_after_resolution.runtime->pager_worker_resolutions_completed == 1,
          "runtime status counts one completed pager worker resolution");
    check(runtime_status_after_resolution.runtime->pager_faults_coalesced == 1,
          "runtime status retains coalesced pager fault count after resolution");
  }

  auto pager_group_after_resolution = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = pager_group_id,
      });
  check(pager_group_after_resolution.process_group.has_value(),
        "pager-needed process group view remains available after resolution");
  if (pager_group_after_resolution.process_group) {
    check(!pager_group_after_resolution.process_group->pager_needed,
          "process group clears pager-needed state after resolution");
    check(!pager_group_after_resolution.process_group->pager_handoff_pending,
          "process group keeps handoff pending cleared after resolution");
    check(!pager_group_after_resolution.process_group->pager_worker_owned,
          "process group clears worker-owned pager state after resolution");
    check(pager_group_after_resolution.process_group->pager_handoffs == 1,
          "process group retains dispatched handoff count after resolution");
    check(pager_group_after_resolution.process_group->pager_resolutions == 1,
          "process group counts one pager resolution");
    check(pager_group_after_resolution.process_group->pager_faults_coalesced == 1,
          "process group retains one coalesced pager fault after resolution");
  }

  auto fault_summary_after_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_summary_after_resolution.status == KernelServiceStatus::Ok,
        "fault summary succeeds after pager resolution");
  check(fault_summary_after_resolution.fault_summary.has_value(),
        "fault summary returns pager resolution state");
  if (fault_summary_after_resolution.fault_summary) {
    check(fault_summary_after_resolution.fault_summary->pager_needed_address_spaces == 0,
          "fault summary clears pager-needed address-space count after resolution");
    check(fault_summary_after_resolution.fault_summary->pending_pager_handoffs == 0,
          "fault summary keeps pending handoff count clear after resolution");
    check(fault_summary_after_resolution.fault_summary->pager_handoffs_dispatched == 1,
          "fault summary retains dispatched handoff count after resolution");
    check(fault_summary_after_resolution.fault_summary->pager_resolutions == 1,
          "fault summary counts one pager resolution");
    check(fault_summary_after_resolution.fault_summary->last_pager_resolution.has_value(),
          "fault summary exposes the latest pager resolution");
    if (fault_summary_after_resolution.fault_summary->last_pager_resolution) {
      check(fault_summary_after_resolution.fault_summary->last_pager_resolution->address_space_id ==
                *pager_address_space_id,
            "fault summary tracks the resolved address space");
      check(fault_summary_after_resolution.fault_summary->last_pager_resolution->fault.fault ==
                mmu::MmuFault::Unmapped,
            "fault summary preserves the resolved fault classification");
    }
    check(!fault_summary_after_resolution.fault_summary->pager_worker_busy,
          "fault summary reports idle pager worker after resolution");
    check(fault_summary_after_resolution.fault_summary->pager_worker_handoffs_received == 1,
          "fault summary counts one pager worker handoff after first resolution");
    check(fault_summary_after_resolution.fault_summary->pager_worker_resolutions_completed == 1,
          "fault summary counts one pager worker resolution after first resolution");
    check(fault_summary_after_resolution.fault_summary->pager_faults_coalesced == 1,
          "fault summary counts one coalesced pager fault after first resolution");
  }

  const auto repeat_tva = mmu::tva_from_vpn_offset(63, 0);
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = repeat_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *pager_tid,
  });
  check(true, "same address space can enter a second pager-needed cycle");
  (void)axion_kernel_step(*state);
  check(true, "second pager cycle fault is delivered");
  auto runtime_status_repeat_fault = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_status_repeat_fault.runtime.has_value(),
        "runtime status exposes repeat pager-needed cycle");
  if (runtime_status_repeat_fault.runtime) {
    check(runtime_status_repeat_fault.runtime->pager_needed_address_space_count == 1,
          "runtime status reports one pager-needed address space in repeat cycle");
    check(runtime_status_repeat_fault.runtime->pending_pager_handoff_count == 1,
          "runtime status reports pending handoff in repeat cycle");
  }
  (void)axion_kernel_step(*state);
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     repeat_tva,
                     *pager_address_space_id),
        "same address space accepts a second missing-page mapping");
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  auto fault_summary_after_repeat = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_summary_after_repeat.fault_summary.has_value(),
        "fault summary exposes repeated pager worker cycle");
  if (fault_summary_after_repeat.fault_summary) {
    check(fault_summary_after_repeat.fault_summary->pager_handoffs_dispatched == 2,
          "fault summary counts two dispatched pager handoffs after repeat cycle");
    check(fault_summary_after_repeat.fault_summary->pager_resolutions == 2,
          "fault summary counts two pager resolutions after repeat cycle");
    check(fault_summary_after_repeat.fault_summary->pager_worker_handoffs_received == 2,
          "fault summary counts two pager worker handoffs after repeat cycle");
    check(fault_summary_after_repeat.fault_summary->pager_worker_resolutions_completed == 2,
          "fault summary counts two pager worker resolutions after repeat cycle");
  }
}

static void test_kernel_pager_worker_backlog() {
  std::printf("\n[AC-22c] Axion kernel tracks deterministic pager backlog under load\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for pager backlog test");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext first_thread;
  first_thread.label = "pager-backlog-a";
  first_thread.registers[0] = 221;
  t81::ternaryos::sched::TiscContext second_thread;
  second_thread.label = "pager-backlog-b";
  second_thread.registers[0] = 222;

  auto first_tid = axion_kernel_spawn_thread(*state, first_thread);
  auto second_tid = axion_kernel_spawn_thread(*state, second_thread);
  check(first_tid.has_value(), "first backlog thread spawns successfully");
  check(second_tid.has_value(), "second backlog thread spawns successfully");
  if (!first_tid || !second_tid) {
    return;
  }

  const auto* first_runtime = state->find_thread_runtime(*first_tid);
  const auto* second_runtime = state->find_thread_runtime(*second_tid);
  check(first_runtime != nullptr, "first backlog runtime state exists");
  check(second_runtime != nullptr, "second backlog runtime state exists");
  if (!first_runtime || !second_runtime) {
    return;
  }

  const auto first_group_id = first_runtime->process_group_id;
  const auto second_group_id = second_runtime->process_group_id;
  const auto first_address_space_id =
      state->find_process_group_address_space(first_group_id);
  const auto second_address_space_id =
      state->find_process_group_address_space(second_group_id);
  check(first_address_space_id.has_value(),
        "first backlog process group resolves to an address space");
  check(second_address_space_id.has_value(),
        "second backlog process group resolves to an address space");
  if (!first_address_space_id || !second_address_space_id) {
    return;
  }

  const auto first_tva = mmu::tva_from_vpn_offset(70, 0);
  const auto second_tva = mmu::tva_from_vpn_offset(71, 0);
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = first_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *first_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = second_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *second_tid,
  });

  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  auto runtime_after_delivery = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_delivery.runtime.has_value(),
        "runtime status exposes pager backlog after fault delivery");
  if (runtime_after_delivery.runtime) {
    check(runtime_after_delivery.runtime->pager_needed_address_space_count == 2,
          "runtime status reports two pager-needed address spaces under backlog");
    check(runtime_after_delivery.runtime->pending_pager_handoff_count == 2,
          "runtime status reports two pending pager handoffs under backlog");
    check(runtime_after_delivery.runtime->pending_pager_handoff_high_watermark == 2,
          "runtime status records pending handoff high watermark under backlog");
    check(runtime_after_delivery.runtime->pager_worker_inbox_count == 0,
          "runtime status keeps worker inbox empty before dispatching backlog");
    check(runtime_after_delivery.runtime->pager_worker_activations == 0,
          "runtime status starts with zero pager worker activations under backlog");
  }

  (void)axion_kernel_step(*state);
  auto fault_after_first_dispatch = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_first_dispatch.fault_summary.has_value(),
        "fault summary exposes first pager backlog dispatch");
  if (fault_after_first_dispatch.fault_summary) {
    check(fault_after_first_dispatch.fault_summary->pending_pager_handoffs == 1,
          "fault summary drains one pending handoff on first backlog dispatch");
    check(fault_after_first_dispatch.fault_summary->pager_worker_inbox_count == 1,
          "fault summary queues one work item after first backlog dispatch");
    check(fault_after_first_dispatch.fault_summary->pager_worker_inbox_high_watermark == 1,
          "fault summary records one-item worker inbox watermark initially");
    check(fault_after_first_dispatch.fault_summary
              ->pager_worker_next_queued_address_space_id ==
              first_address_space_id,
          "fault summary tracks the first queued pager-worker address space");
    check(fault_after_first_dispatch.fault_summary
              ->pager_worker_next_queued_handoff_sequence == 1,
          "fault summary tracks the first queued pager-worker handoff ordinal");
    check(fault_after_first_dispatch.fault_summary
              ->pager_worker_last_received_address_space_id ==
              first_address_space_id,
          "fault summary tracks the first received pager-worker address space");
    check(fault_after_first_dispatch.fault_summary
              ->pager_worker_last_received_handoff_sequence == 1,
          "fault summary tracks the first received pager-worker handoff ordinal");
    check(fault_after_first_dispatch.fault_summary->last_pager_handoff.has_value(),
          "fault summary exposes first backlog handoff");
    if (fault_after_first_dispatch.fault_summary->last_pager_handoff) {
      check(fault_after_first_dispatch.fault_summary->last_pager_handoff->address_space_id ==
                *first_address_space_id,
            "fault summary dispatches the first queued address space first");
    }
  }

  (void)axion_kernel_step(*state);
  auto runtime_after_second_dispatch = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_second_dispatch.runtime.has_value(),
        "runtime status exposes second pager backlog dispatch");
  if (runtime_after_second_dispatch.runtime) {
    check(runtime_after_second_dispatch.runtime->pending_pager_handoff_count == 0,
          "runtime status drains pending handoffs after second backlog dispatch");
    check(runtime_after_second_dispatch.runtime->pending_pager_handoff_high_watermark == 2,
          "runtime status retains pending handoff high watermark after dispatch");
    check(runtime_after_second_dispatch.runtime->pager_worker_inbox_count == 2,
          "runtime status exposes two queued worker items under backlog");
    check(runtime_after_second_dispatch.runtime->pager_worker_inbox_high_watermark == 2,
          "runtime status records worker inbox high watermark under backlog");
    check(runtime_after_second_dispatch.runtime
              ->pager_worker_next_queued_address_space_id ==
              first_address_space_id,
          "runtime status tracks the next queued pager-worker address before activation");
    check(runtime_after_second_dispatch.runtime
              ->pager_worker_next_queued_handoff_sequence == 1,
          "runtime status tracks the next queued pager-worker handoff ordinal before activation");
    check(runtime_after_second_dispatch.runtime->pager_handoffs_dispatched == 2,
          "runtime status counts two dispatched pager handoffs under backlog");
    check(runtime_after_second_dispatch.runtime->pager_worker_handoffs_received == 2,
          "runtime status counts two handoffs received by the worker under backlog");
    check(runtime_after_second_dispatch.runtime
              ->pager_worker_last_received_address_space_id ==
              second_address_space_id,
          "runtime status tracks the last received pager-worker address under backlog");
    check(runtime_after_second_dispatch.runtime
              ->pager_worker_last_received_handoff_sequence == 2,
          "runtime status tracks the last received pager-worker handoff ordinal");
  }

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     second_tva,
                     *second_address_space_id),
        "second backlog address space accepts its mapping before activation");
  (void)axion_kernel_step(*state);
  auto runtime_after_first_activation = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_first_activation.runtime.has_value(),
        "runtime status exposes first worker activation");
  if (runtime_after_first_activation.runtime) {
    check(!runtime_after_first_activation.runtime->pager_worker_busy,
          "runtime status returns worker to idle after ready-bypass resolution");
    check(!runtime_after_first_activation.runtime->pager_worker_active_address_space_id.has_value(),
          "runtime status exposes no active worker address after ready-bypass resolution");
    check(!runtime_after_first_activation.runtime->pager_worker_active_handoff_sequence.has_value(),
          "runtime status exposes no active handoff after ready-bypass resolution");
    check(runtime_after_first_activation.runtime->pager_worker_inbox_count == 1,
          "runtime status leaves one queued stalled item after ready-bypass resolution");
    check(runtime_after_first_activation.runtime
              ->pager_worker_next_queued_address_space_id ==
              first_address_space_id,
          "runtime status keeps the stalled original item at the queue head after ready bypass");
    check(runtime_after_first_activation.runtime
              ->pager_worker_next_queued_handoff_sequence == 1,
          "runtime status keeps the stalled original handoff ordinal at the queue head after ready bypass");
    check(runtime_after_first_activation.runtime->pager_worker_activations == 1,
          "runtime status counts one worker activation after ready bypass");
    check(runtime_after_first_activation.runtime->pager_worker_ready_bypass_activations == 1,
          "runtime status counts one ready-bypass activation");
    check(runtime_after_first_activation.runtime
              ->pager_worker_last_ready_bypass_blocked_address_space_id ==
              first_address_space_id,
          "runtime status tracks the blocked FIFO head used for ready bypass");
    check(runtime_after_first_activation.runtime
              ->pager_worker_last_ready_bypass_promoted_address_space_id ==
              second_address_space_id,
          "runtime status tracks the promoted ready queued address space");
    check(runtime_after_first_activation.runtime->pager_worker_last_ready_bypass_cycle == 1,
          "runtime status tracks the ready-bypass ordinal");
    check(runtime_after_first_activation.runtime->pager_worker_last_activated_address_space_id ==
              second_address_space_id,
          "runtime status tracks the promoted ready address space as the first activation");
    check(runtime_after_first_activation.runtime->pager_worker_last_activation_cycle == 1,
          "runtime status tracks the first activation ordinal");
    check(runtime_after_first_activation.runtime->pager_worker_stall_cycles == 0,
          "runtime status avoids a stall when ready bypass can select mapped work first");
    check(runtime_after_first_activation.runtime->pager_worker_backlog_blocked_cycles == 0,
          "runtime status avoids backlog-blocked accounting under ready bypass");
    check(runtime_after_first_activation.runtime->pager_worker_ready_backlog_cycles == 0,
          "runtime status avoids ready-backlog stall accounting under ready bypass");
    check(runtime_after_first_activation.runtime->pager_worker_ready_backlog_count == 0,
          "runtime status reports no live ready backlog after ready-bypass resolution");
    check(runtime_after_first_activation.runtime->pager_worker_ready_backlog_high_watermark == 0,
          "runtime status leaves ready-backlog watermark clear when bypass avoids the stall");
    check(!runtime_after_first_activation.runtime->pager_worker_last_stalled_address_space_id.has_value(),
          "runtime status records no stalled active address space under ready bypass");
    check(!runtime_after_first_activation.runtime->pager_worker_last_stall_cycle.has_value(),
          "runtime status records no stall ordinal under ready bypass");
    check(!runtime_after_first_activation.runtime
                ->pager_worker_last_ready_backlog_address_space_id.has_value(),
          "runtime status records no blocked ready queued address under ready bypass");
    check(!runtime_after_first_activation.runtime->pager_worker_last_ready_backlog_cycle.has_value(),
          "runtime status records no blocked-ready stall ordinal under ready bypass");
    check(!runtime_after_first_activation.runtime->pager_worker_last_ready_backlog_count.has_value(),
          "runtime status records no blocked-ready depth under ready bypass");
    check(runtime_after_first_activation.runtime->pager_resolutions == 1,
          "runtime status resolves the promoted ready address space immediately under ready bypass");
  }

  auto fault_after_first_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_first_resolution.fault_summary.has_value(),
        "fault summary exposes first ready-bypass resolution");
  if (fault_after_first_resolution.fault_summary) {
    check(fault_after_first_resolution.fault_summary->pager_resolutions == 1,
          "fault summary counts the first ready-bypass resolution");
    check(fault_after_first_resolution.fault_summary
              ->pager_worker_last_completed_address_space_id ==
              second_address_space_id,
          "fault summary tracks the promoted ready address space as the first completion");
    check(fault_after_first_resolution.fault_summary
              ->pager_worker_last_completed_resolution_sequence == 1,
          "fault summary tracks the first completed pager-worker resolution ordinal");
    check(fault_after_first_resolution.fault_summary->pager_worker_ready_bypass_activations == 1,
          "fault summary counts one ready-bypass activation");
    check(fault_after_first_resolution.fault_summary
              ->pager_worker_last_ready_bypass_blocked_address_space_id ==
              first_address_space_id,
          "fault summary tracks the blocked FIFO head used for ready bypass");
    check(fault_after_first_resolution.fault_summary
              ->pager_worker_last_ready_bypass_promoted_address_space_id ==
              second_address_space_id,
          "fault summary tracks the promoted ready address space");
    check(fault_after_first_resolution.fault_summary->pager_worker_last_ready_bypass_cycle == 1,
          "fault summary tracks the ready-bypass ordinal");
    check(fault_after_first_resolution.fault_summary->last_pager_resolution.has_value(),
          "fault summary exposes the first ready-bypass resolution record");
    if (fault_after_first_resolution.fault_summary->last_pager_resolution) {
      check(fault_after_first_resolution.fault_summary->last_pager_resolution->address_space_id ==
                *second_address_space_id,
            "fault summary resolves the promoted ready address space first");
    }
  }

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     first_tva,
                     *first_address_space_id),
        "first backlog address space accepts its mapping before final resolution");

  (void)axion_kernel_step(*state);
  auto runtime_after_second_activation = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_second_activation.runtime.has_value(),
        "runtime status exposes second worker activation/resolution step");
  if (runtime_after_second_activation.runtime) {
    check(runtime_after_second_activation.runtime->pager_resolutions == 2,
          "runtime status resolves the remaining stalled address space on its activation step");
    check(!runtime_after_second_activation.runtime->pager_worker_busy,
          "runtime status returns worker to idle after second activation step");
    check(!runtime_after_second_activation.runtime->pager_worker_active_address_space_id.has_value(),
          "runtime status clears active worker address after second activation step");
    check(!runtime_after_second_activation.runtime->pager_worker_active_handoff_sequence.has_value(),
          "runtime status clears active worker handoff ordinal after second activation step");
    check(!runtime_after_second_activation.runtime->pager_worker_next_queued_address_space_id.has_value(),
          "runtime status clears next queued pager-worker address after second activation step");
    check(!runtime_after_second_activation.runtime->pager_worker_next_queued_handoff_sequence.has_value(),
          "runtime status clears next queued pager-worker handoff ordinal after second activation step");
    check(runtime_after_second_activation.runtime->pager_worker_inbox_count == 0,
          "runtime status drains the worker inbox by the second activation");
    check(runtime_after_second_activation.runtime->pager_worker_activations == 2,
          "runtime status counts two worker activations after backlog drain");
    check(runtime_after_second_activation.runtime->pager_worker_ready_bypass_activations == 1,
          "runtime status retains one ready-bypass activation after backlog drain");
    check(runtime_after_second_activation.runtime->pager_worker_last_activated_address_space_id ==
              first_address_space_id,
          "runtime status tracks the remaining stalled address space as the final activation");
    check(runtime_after_second_activation.runtime->pager_worker_last_activation_cycle == 2,
          "runtime status tracks the second activation ordinal");
  }
  auto runtime_after_second_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_second_resolution.runtime.has_value(),
        "runtime status exposes second backlog resolution");
  if (runtime_after_second_resolution.runtime) {
    check(runtime_after_second_resolution.runtime->pager_resolutions == 2,
          "runtime status counts two pager resolutions after backlog drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_busy,
          "runtime status reports idle worker after backlog drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_active_handoff_sequence.has_value(),
          "runtime status retains no active handoff ordinal after backlog drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_next_queued_address_space_id.has_value(),
          "runtime status retains no queued pager-worker address after backlog drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_next_queued_handoff_sequence.has_value(),
          "runtime status retains no queued pager-worker handoff ordinal after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_inbox_count == 0,
          "runtime status leaves no queued work after backlog drain");
    check(runtime_after_second_resolution.runtime->pending_pager_handoff_high_watermark == 2,
          "runtime status retains pending handoff watermark after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_inbox_high_watermark == 2,
          "runtime status retains inbox watermark after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_activations == 2,
          "runtime status retains worker activation count after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_ready_bypass_activations == 1,
          "runtime status retains one ready-bypass activation after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_last_activated_address_space_id ==
              first_address_space_id,
          "runtime status retains the last activated address space after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_last_activation_cycle == 2,
          "runtime status retains the last activation ordinal after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_stall_cycles == 0,
          "runtime status retains zero worker stall cycles after ready-bypass drain");
    check(runtime_after_second_resolution.runtime->pager_worker_backlog_blocked_cycles == 0,
          "runtime status retains zero backlog-blocked cycles after ready-bypass drain");
    check(runtime_after_second_resolution.runtime->pager_worker_ready_backlog_cycles == 0,
          "runtime status retains zero ready-backlog stall cycles after ready-bypass drain");
    check(runtime_after_second_resolution.runtime->pager_worker_ready_backlog_count == 0,
          "runtime status clears current ready-backlog depth after backlog drain");
    check(runtime_after_second_resolution.runtime->pager_worker_ready_backlog_high_watermark == 0,
          "runtime status retains zero ready-backlog high watermark after ready-bypass drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_last_stalled_address_space_id.has_value(),
          "runtime status retains no stalled active address after ready-bypass drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_last_stall_cycle.has_value(),
          "runtime status retains no stall ordinal after ready-bypass drain");
    check(!runtime_after_second_resolution.runtime
                ->pager_worker_last_ready_backlog_address_space_id.has_value(),
          "runtime status retains no blocked ready queued address after ready-bypass drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_last_ready_backlog_cycle.has_value(),
          "runtime status retains no blocked-ready stall ordinal after ready-bypass drain");
    check(!runtime_after_second_resolution.runtime->pager_worker_last_ready_backlog_count.has_value(),
          "runtime status retains no blocked-ready depth after ready-bypass drain");
    check(runtime_after_second_resolution.runtime->pager_worker_resolutions_completed == 2,
          "runtime status counts two completed worker resolutions after backlog drain");
    check(runtime_after_second_resolution.runtime
              ->pager_worker_last_received_address_space_id ==
              second_address_space_id,
          "runtime status retains the last received pager-worker address after backlog drain");
    check(runtime_after_second_resolution.runtime
              ->pager_worker_last_received_handoff_sequence == 2,
          "runtime status retains the last received pager-worker handoff ordinal");
    check(runtime_after_second_resolution.runtime
              ->pager_worker_last_completed_address_space_id ==
              first_address_space_id,
          "runtime status retains the last completed pager-worker address after backlog drain");
    check(runtime_after_second_resolution.runtime
              ->pager_worker_last_completed_resolution_sequence == 2,
          "runtime status retains the last completed pager-worker resolution ordinal");
  }

  auto fault_after_second_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_second_resolution.fault_summary.has_value(),
        "fault summary exposes final backlog resolution");
  if (fault_after_second_resolution.fault_summary) {
    check(fault_after_second_resolution.fault_summary->last_pager_resolution.has_value(),
          "fault summary exposes the second backlog resolution record");
    if (fault_after_second_resolution.fault_summary->last_pager_resolution) {
      check(fault_after_second_resolution.fault_summary->last_pager_resolution->address_space_id ==
                *first_address_space_id,
            "fault summary resolves the originally blocked address space second");
    }
    check(fault_after_second_resolution.fault_summary->pending_pager_handoff_high_watermark == 2,
          "fault summary retains pending handoff watermark after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_inbox_high_watermark == 2,
          "fault summary retains inbox watermark after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_activations == 2,
          "fault summary counts two worker activations after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_ready_bypass_activations == 1,
          "fault summary retains one ready-bypass activation after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_last_activated_address_space_id ==
              first_address_space_id,
          "fault summary retains the last activated address space after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_last_activation_cycle == 2,
          "fault summary retains the last activation ordinal after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_stall_cycles == 0,
          "fault summary counts zero worker stall cycles after ready-bypass drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_backlog_blocked_cycles == 0,
          "fault summary counts zero backlog-blocked cycles after ready-bypass drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_ready_backlog_cycles == 0,
          "fault summary counts zero ready-backlog stall cycles after ready-bypass drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_ready_backlog_count == 0,
          "fault summary clears current ready-backlog depth after backlog drain");
    check(fault_after_second_resolution.fault_summary->pager_worker_ready_backlog_high_watermark == 0,
          "fault summary retains zero ready-backlog high watermark after ready-bypass drain");
    check(!fault_after_second_resolution.fault_summary->pager_worker_active_handoff_sequence.has_value(),
          "fault summary retains no active handoff ordinal after backlog drain");
    check(!fault_after_second_resolution.fault_summary
                ->pager_worker_next_queued_address_space_id.has_value(),
          "fault summary retains no queued pager-worker address after backlog drain");
    check(!fault_after_second_resolution.fault_summary
                ->pager_worker_next_queued_handoff_sequence.has_value(),
          "fault summary retains no queued pager-worker handoff ordinal after backlog drain");
    check(!fault_after_second_resolution.fault_summary
                ->pager_worker_last_stalled_address_space_id.has_value(),
          "fault summary retains no stalled active address after ready-bypass drain");
    check(!fault_after_second_resolution.fault_summary->pager_worker_last_stall_cycle.has_value(),
          "fault summary retains no stall ordinal after ready-bypass drain");
    check(!fault_after_second_resolution.fault_summary
                ->pager_worker_last_ready_backlog_address_space_id.has_value(),
          "fault summary retains no blocked ready queued address after ready-bypass drain");
    check(!fault_after_second_resolution.fault_summary
                ->pager_worker_last_ready_backlog_cycle.has_value(),
          "fault summary retains no blocked-ready stall ordinal after ready-bypass drain");
    check(!fault_after_second_resolution.fault_summary
                ->pager_worker_last_ready_backlog_count.has_value(),
          "fault summary retains no blocked-ready depth after ready-bypass drain");
    check(fault_after_second_resolution.fault_summary
              ->pager_worker_last_received_address_space_id ==
              second_address_space_id,
          "fault summary retains the last received pager-worker address after backlog drain");
    check(fault_after_second_resolution.fault_summary
              ->pager_worker_last_received_handoff_sequence == 2,
          "fault summary retains the last received pager-worker handoff ordinal");
    check(fault_after_second_resolution.fault_summary
              ->pager_worker_last_completed_address_space_id ==
              first_address_space_id,
          "fault summary retains the last completed pager-worker address after backlog drain");
    check(fault_after_second_resolution.fault_summary
              ->pager_worker_last_completed_resolution_sequence == 2,
          "fault summary retains the last completed pager-worker resolution ordinal");
  }
}

static void test_kernel_pager_worker_ready_bypass_cap() {
  std::printf(
      "\n[AC-22d] Axion pager worker caps repeated ready bypass for one blocked head\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for ready-bypass cap");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext first_thread;
  first_thread.label = "pager-cap-a";
  first_thread.registers[0] = 111;

  t81::ternaryos::sched::TiscContext second_thread;
  second_thread.label = "pager-cap-b";
  second_thread.registers[0] = 222;

  t81::ternaryos::sched::TiscContext third_thread;
  third_thread.label = "pager-cap-c";
  third_thread.registers[0] = 333;

  auto first_tid = axion_kernel_spawn_thread(*state, first_thread);
  auto second_tid = axion_kernel_spawn_thread(*state, second_thread);
  auto third_tid = axion_kernel_spawn_thread(*state, third_thread);
  check(first_tid.has_value(), "first cap thread spawns successfully");
  check(second_tid.has_value(), "second cap thread spawns successfully");
  check(third_tid.has_value(), "third cap thread spawns successfully");
  if (!first_tid || !second_tid || !third_tid) {
    return;
  }

  const auto* first_runtime = state->find_thread_runtime(*first_tid);
  const auto* second_runtime = state->find_thread_runtime(*second_tid);
  const auto* third_runtime = state->find_thread_runtime(*third_tid);
  check(first_runtime != nullptr, "first cap runtime state exists");
  check(second_runtime != nullptr, "second cap runtime state exists");
  check(third_runtime != nullptr, "third cap runtime state exists");
  if (!first_runtime || !second_runtime || !third_runtime) {
    return;
  }

  const auto first_address_space_id =
      state->find_process_group_address_space(first_runtime->process_group_id);
  const auto second_address_space_id =
      state->find_process_group_address_space(second_runtime->process_group_id);
  const auto third_address_space_id =
      state->find_process_group_address_space(third_runtime->process_group_id);
  check(first_address_space_id.has_value(),
        "first cap process group resolves to an address space");
  check(second_address_space_id.has_value(),
        "second cap process group resolves to an address space");
  check(third_address_space_id.has_value(),
        "third cap process group resolves to an address space");
  if (!first_address_space_id || !second_address_space_id ||
      !third_address_space_id) {
    return;
  }

  const auto first_tva = mmu::tva_from_vpn_offset(80, 0);
  const auto second_tva = mmu::tva_from_vpn_offset(81, 0);
  const auto third_tva = mmu::tva_from_vpn_offset(82, 0);
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = first_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *first_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = second_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *second_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = third_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *third_tid,
  });

  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     second_tva,
                     *second_address_space_id),
        "second cap address space accepts its mapping before first activation");
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     third_tva,
                     *third_address_space_id),
        "third cap address space accepts its mapping before first activation");

  (void)axion_kernel_step(*state);
  auto runtime_after_first_bypass = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_first_bypass.runtime.has_value(),
        "runtime status exposes first bounded ready bypass");
  if (runtime_after_first_bypass.runtime) {
    check(runtime_after_first_bypass.runtime->pager_resolutions == 1,
          "runtime status resolves one promoted ready item on first bypass");
    check(runtime_after_first_bypass.runtime->pager_worker_ready_bypass_activations == 1,
          "runtime status counts one ready-bypass activation before cap");
    check(runtime_after_first_bypass.runtime->pager_worker_ready_bypass_deferrals == 0,
          "runtime status starts with zero ready-bypass deferrals");
    check(runtime_after_first_bypass.runtime
              ->pager_worker_last_ready_bypass_blocked_address_space_id ==
              first_address_space_id,
          "runtime status tracks the blocked head used for the first bypass");
    check(runtime_after_first_bypass.runtime
              ->pager_worker_last_ready_bypass_promoted_address_space_id ==
              second_address_space_id,
          "runtime status tracks the promoted ready item used for the first bypass");
    check(runtime_after_first_bypass.runtime->pager_worker_inbox_count == 2,
          "runtime status retains the blocked head and third item after first bypass");
    check(runtime_after_first_bypass.runtime
              ->pager_worker_next_queued_address_space_id ==
              first_address_space_id,
          "runtime status keeps the blocked head at the queue front after first bypass");
  }

  (void)axion_kernel_step(*state);
  auto runtime_after_capped_activation = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_capped_activation.runtime.has_value(),
        "runtime status exposes capped deferral after first bypass");
  if (runtime_after_capped_activation.runtime) {
    check(!runtime_after_capped_activation.runtime->pager_worker_busy,
          "runtime status keeps the worker idle once repeated bypass is capped");
    check(!runtime_after_capped_activation.runtime->pager_worker_active_address_space_id.has_value(),
          "runtime status clears active work when the cap defers activation");
    check(!runtime_after_capped_activation.runtime->pager_worker_active_handoff_sequence.has_value(),
          "runtime status clears active handoff provenance when the cap parks the worker");
    check(runtime_after_capped_activation.runtime->pager_worker_inbox_count == 2,
          "runtime status leaves both the blocked head and third item queued under the cap");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_next_queued_address_space_id ==
              first_address_space_id,
          "runtime status keeps the blocked head at the queue front when the cap parks the worker");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_next_queued_handoff_sequence == 1,
          "runtime status preserves the blocked head handoff ordinal at the queue front under cap");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_ready_backlog_count == 0,
          "runtime status reports no ready-behind-active backlog while the worker remains parked");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_parked_ready_count == 1,
          "runtime status reports one ready item behind the parked blocked head");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_parked_ready_high_watermark == 1,
          "runtime status raises parked-ready backlog watermark on first parked cycle");
    check(runtime_after_capped_activation.runtime->pager_resolutions == 1,
          "runtime status does not resolve additional work while the blocked head remains parked");
    check(runtime_after_capped_activation.runtime->pager_worker_activations == 1,
          "runtime status does not add a second activation while the blocked head remains parked");
    check(runtime_after_capped_activation.runtime->pager_worker_handoffs_received == 3,
          "runtime status retains three received handoffs while the worker is parked");
    check(runtime_after_capped_activation.runtime->pager_worker_last_received_address_space_id ==
              third_address_space_id,
          "runtime status retains the third item as the latest received handoff while parked");
    check(runtime_after_capped_activation.runtime->pager_worker_last_received_handoff_sequence == 3,
          "runtime status retains the third handoff ordinal while parked");
    check(runtime_after_capped_activation.runtime->pager_worker_ready_bypass_activations == 1,
          "runtime status does not permit a second ready bypass for the same head");
    check(runtime_after_capped_activation.runtime->pager_worker_ready_bypass_deferrals == 1,
          "runtime status counts one ready-bypass deferral after the cap fires");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_last_ready_bypass_deferred_blocked_address_space_id ==
              first_address_space_id,
          "runtime status tracks the blocked head for the deferred bypass");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_last_ready_bypass_deferred_ready_address_space_id ==
              third_address_space_id,
          "runtime status tracks the still-ready queued item deferred by the cap");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_last_ready_bypass_deferred_cycle == 1,
          "runtime status tracks the first ready-bypass deferral ordinal");
    check(runtime_after_capped_activation.runtime->pager_worker_parked_cycles == 1,
          "runtime status counts one parked worker cycle after the cap fires");
    check(runtime_after_capped_activation.runtime->pager_worker_parked_resumptions == 0,
          "runtime status does not count a parked resumption before the blocked head becomes ready");
    check(runtime_after_capped_activation.runtime->pager_worker_parked_resolved_heads == 0,
          "runtime status does not count a parked-head resolution before the blocked head resumes");
    check(!runtime_after_capped_activation.runtime->pager_worker_last_parked_resumed_ready_count.has_value(),
          "runtime status has no resumed ready-backlog count before the blocked head resumes");
    check(!runtime_after_capped_activation.runtime
               ->pager_worker_last_parked_resolved_address_space_id.has_value(),
          "runtime status has no parked-head resolution address before the blocked head resumes");
    check(!runtime_after_capped_activation.runtime
               ->pager_worker_last_parked_resolved_remaining_inbox_count.has_value(),
          "runtime status has no parked-head remaining inbox count before the blocked head resumes");
    check(!runtime_after_capped_activation.runtime
               ->pager_worker_last_parked_resumed_handoff_sequence.has_value(),
          "runtime status has no resumed blocked-head handoff ordinal before the blocked head resumes");
    check(!runtime_after_capped_activation.runtime
               ->pager_worker_last_parked_resumed_ready_address_space_id.has_value(),
          "runtime status has no resumed trailing ready item before the blocked head resumes");
    check(!runtime_after_capped_activation.runtime
               ->pager_worker_last_parked_resumed_ready_handoff_sequence.has_value(),
          "runtime status has no resumed trailing ready ordinal before the blocked head resumes");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_last_parked_blocked_address_space_id ==
              first_address_space_id,
          "runtime status tracks the blocked head for the parked cycle");
    check(runtime_after_capped_activation.runtime
              ->pager_worker_last_parked_ready_address_space_id ==
              third_address_space_id,
          "runtime status tracks the ready queued item behind the parked head");
    check(runtime_after_capped_activation.runtime->pager_worker_last_parked_cycle == 1,
          "runtime status tracks the first parked worker cycle ordinal");
    check(runtime_after_capped_activation.runtime->pager_worker_last_parked_ready_count == 1,
          "runtime status tracks one ready queued item behind the parked head");
    check(runtime_after_capped_activation.runtime->pager_worker_stall_cycles == 0,
          "runtime status does not add a stall when the cap parks the worker");
    check(runtime_after_capped_activation.runtime->pager_worker_backlog_blocked_cycles == 0,
          "runtime status does not add a backlog-blocked cycle when the worker stays parked");
    check(runtime_after_capped_activation.runtime->pager_worker_ready_backlog_cycles == 0,
          "runtime status does not add a ready-behind-active cycle when no work is active");
  }

  auto fault_after_capped_deferral = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_capped_deferral.fault_summary.has_value(),
        "fault summary exposes parked worker state after capped deferral");
  if (fault_after_capped_deferral.fault_summary) {
    check(!fault_after_capped_deferral.fault_summary->pager_worker_busy,
          "fault summary reports idle worker while the blocked head is parked");
    check(fault_after_capped_deferral.fault_summary->pager_worker_inbox_count == 2,
          "fault summary retains both queued items while the blocked head is parked");
    check(fault_after_capped_deferral.fault_summary
              ->pager_worker_next_queued_address_space_id ==
              first_address_space_id,
          "fault summary keeps the blocked head at the queue front while parked");
    check(fault_after_capped_deferral.fault_summary
              ->pager_worker_ready_bypass_deferrals == 1,
          "fault summary counts one ready-bypass deferral after the cap parks the worker");
    check(fault_after_capped_deferral.fault_summary->pager_worker_parked_cycles == 1,
          "fault summary counts one parked worker cycle after the cap parks the worker");
    check(fault_after_capped_deferral.fault_summary->pager_worker_parked_resumptions == 0,
          "fault summary does not count a parked resumption while the head is still parked");
    check(fault_after_capped_deferral.fault_summary->pager_worker_parked_resolved_heads == 0,
          "fault summary does not count a parked-head resolution while the head is still parked");
    check(!fault_after_capped_deferral.fault_summary->pager_worker_last_parked_resumed_ready_count.has_value(),
          "fault summary has no resumed ready-backlog count while the head is still parked");
    check(!fault_after_capped_deferral.fault_summary
               ->pager_worker_last_parked_resolved_address_space_id.has_value(),
          "fault summary has no parked-head resolution address while the head is still parked");
    check(!fault_after_capped_deferral.fault_summary
               ->pager_worker_last_parked_resolved_remaining_inbox_count.has_value(),
          "fault summary has no parked-head remaining inbox count while the head is still parked");
    check(!fault_after_capped_deferral.fault_summary
               ->pager_worker_last_parked_resumed_handoff_sequence.has_value(),
          "fault summary has no resumed blocked-head handoff ordinal while the head is still parked");
    check(!fault_after_capped_deferral.fault_summary
               ->pager_worker_last_parked_resumed_ready_address_space_id.has_value(),
          "fault summary has no resumed trailing ready item while the head is still parked");
    check(!fault_after_capped_deferral.fault_summary
               ->pager_worker_last_parked_resumed_ready_handoff_sequence.has_value(),
          "fault summary has no resumed trailing ready ordinal while the head is still parked");
    check(fault_after_capped_deferral.fault_summary->pager_worker_parked_ready_count == 1,
          "fault summary reports one ready item behind the parked blocked head");
    check(fault_after_capped_deferral.fault_summary->pager_worker_parked_ready_high_watermark == 1,
          "fault summary raises parked-ready backlog watermark on first parked cycle");
    check(fault_after_capped_deferral.fault_summary->pager_worker_stall_cycles == 0,
          "fault summary does not count a stall while the worker stays parked");
    check(fault_after_capped_deferral.fault_summary->pager_worker_backlog_blocked_cycles == 0,
          "fault summary does not count blocked backlog while no work is active");
    check(fault_after_capped_deferral.fault_summary->pager_worker_ready_backlog_cycles == 0,
          "fault summary does not count ready-behind-active backlog while parked");
  }

  (void)axion_kernel_step(*state);
  auto runtime_after_second_park = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_second_park.runtime.has_value(),
        "runtime status exposes repeated parked deferral while the head stays unresolved");
  if (runtime_after_second_park.runtime) {
    check(!runtime_after_second_park.runtime->pager_worker_busy,
          "runtime status keeps the worker idle on a repeated parked cycle");
    check(runtime_after_second_park.runtime->pager_worker_inbox_count == 2,
          "runtime status keeps both queued items during a repeated parked cycle");
    check(runtime_after_second_park.runtime->pager_worker_ready_bypass_deferrals == 1,
          "runtime status keeps one ready-bypass deferral for the same parked episode");
    check(runtime_after_second_park.runtime->pager_worker_parked_cycles == 2,
          "runtime status counts a second parked worker cycle");
    check(runtime_after_second_park.runtime->pager_worker_parked_resumptions == 0,
          "runtime status still reports no parked resumption on repeated parked cycles");
    check(runtime_after_second_park.runtime->pager_worker_parked_resolved_heads == 0,
          "runtime status still reports no parked-head resolution on repeated parked cycles");
    check(!runtime_after_second_park.runtime->pager_worker_last_parked_resumed_ready_count.has_value(),
          "runtime status still has no resumed ready-backlog count on repeated parked cycles");
    check(!runtime_after_second_park.runtime
               ->pager_worker_last_parked_resolved_address_space_id.has_value(),
          "runtime status still has no parked-head resolution address on repeated parked cycles");
    check(!runtime_after_second_park.runtime
               ->pager_worker_last_parked_resolved_remaining_inbox_count.has_value(),
          "runtime status still has no parked-head remaining inbox count on repeated parked cycles");
    check(!runtime_after_second_park.runtime
               ->pager_worker_last_parked_resumed_handoff_sequence.has_value(),
          "runtime status still has no resumed blocked-head handoff ordinal on repeated parked cycles");
    check(!runtime_after_second_park.runtime
               ->pager_worker_last_parked_resumed_ready_address_space_id.has_value(),
          "runtime status still has no resumed trailing ready item on repeated parked cycles");
    check(!runtime_after_second_park.runtime
               ->pager_worker_last_parked_resumed_ready_handoff_sequence.has_value(),
          "runtime status still has no resumed trailing ready ordinal on repeated parked cycles");
    check(runtime_after_second_park.runtime->pager_worker_parked_ready_count == 1,
          "runtime status keeps one ready item behind the blocked head on repeated parked cycles");
    check(runtime_after_second_park.runtime->pager_worker_parked_ready_high_watermark == 1,
          "runtime status retains parked-ready backlog watermark across repeated parked cycles");
    check(runtime_after_second_park.runtime->pager_worker_last_parked_blocked_address_space_id ==
              first_address_space_id,
          "runtime status preserves the blocked head across repeated parked cycles");
    check(runtime_after_second_park.runtime->pager_worker_last_parked_ready_address_space_id ==
              third_address_space_id,
          "runtime status preserves the ready queued item across repeated parked cycles");
    check(runtime_after_second_park.runtime->pager_worker_last_parked_cycle == 2,
          "runtime status advances the parked worker cycle ordinal");
    check(runtime_after_second_park.runtime->pager_worker_last_parked_ready_count == 1,
          "runtime status keeps one ready queued item behind the parked head");
  }

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     first_tva,
                     *first_address_space_id),
        "first cap address space accepts its mapping before draining the blocked head");

  (void)axion_kernel_step(*state);
  auto runtime_after_head_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_head_resolution.runtime.has_value(),
        "runtime status exposes blocked-head resolution after parked cap");
  if (runtime_after_head_resolution.runtime) {
    check(runtime_after_head_resolution.runtime->pager_resolutions == 2,
          "runtime status resolves the blocked head second after the parked cap");
    check(!runtime_after_head_resolution.runtime->pager_worker_busy,
          "runtime status returns worker to idle after blocked-head resolution");
    check(runtime_after_head_resolution.runtime->pager_worker_inbox_count == 1,
          "runtime status keeps the third item queued after blocked-head resolution");
    check(runtime_after_head_resolution.runtime->pager_worker_parked_resumptions == 1,
          "runtime status counts one parked resumption when the blocked head becomes ready");
    check(runtime_after_head_resolution.runtime->pager_worker_parked_resolved_heads == 1,
          "runtime status counts one parked-head resolution after the resumed head drains");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resumed_address_space_id ==
              first_address_space_id,
          "runtime status tracks the blocked head resumed from parked state");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resumed_handoff_sequence == 1,
          "runtime status tracks the resumed blocked-head handoff ordinal");
    check(runtime_after_head_resolution.runtime->pager_worker_last_parked_resumption_cycle == 1,
          "runtime status tracks the first parked resumption ordinal");
    check(runtime_after_head_resolution.runtime->pager_worker_last_parked_resumed_ready_count == 1,
          "runtime status tracks one ready item still queued behind the resumed head");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resumed_ready_address_space_id ==
              third_address_space_id,
          "runtime status tracks the trailing ready item still queued behind the resumed head");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resumed_ready_handoff_sequence == 3,
          "runtime status tracks the trailing ready item handoff ordinal at parked resumption");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resolved_address_space_id ==
              first_address_space_id,
          "runtime status tracks the blocked head as the parked-resolved address");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resolved_handoff_sequence == 1,
          "runtime status tracks the resumed blocked-head handoff ordinal at parked resolution");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resolved_resolution_sequence == 2,
          "runtime status tracks the blocked head resolution ordinal after parked resumption");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resolved_remaining_inbox_count == 1,
          "runtime status tracks one queued item remaining after the parked head resolves");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resolved_remaining_address_space_id ==
              third_address_space_id,
          "runtime status tracks the queued item remaining after the parked head resolves");
    check(runtime_after_head_resolution.runtime
              ->pager_worker_last_parked_resolved_remaining_handoff_sequence == 3,
          "runtime status tracks the remaining queued item handoff ordinal after parked resolution");
    check(runtime_after_head_resolution.runtime->pager_worker_parked_resolution_follow_on_activations == 0,
          "runtime status does not record a parked-resolution follow-on activation until the next step");
  }

  (void)axion_kernel_step(*state);
  auto runtime_after_follow_on_activation = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_follow_on_activation.runtime.has_value(),
        "runtime status exposes parked-resolution follow-on activation");
  if (runtime_after_follow_on_activation.runtime) {
    check(runtime_after_follow_on_activation.runtime->pager_worker_parked_resolution_follow_on_activations == 1,
          "runtime status counts one parked-resolution follow-on activation");
    check(runtime_after_follow_on_activation.runtime
              ->pager_worker_last_parked_resolution_follow_on_address_space_id ==
              third_address_space_id,
          "runtime status tracks the queued item activated after parked resolution");
    check(runtime_after_follow_on_activation.runtime
              ->pager_worker_last_parked_resolution_follow_on_handoff_sequence == 3,
          "runtime status tracks the queued item handoff ordinal after parked resolution");
    check(runtime_after_follow_on_activation.runtime
              ->pager_worker_last_parked_resolution_follow_on_activation_cycle == 3,
          "runtime status tracks the follow-on activation ordinal after parked resolution");
    check(runtime_after_follow_on_activation.runtime->pager_worker_parked_resolution_follow_on_resolutions == 1,
          "runtime status counts one parked-resolution follow-on resolution once the queued item drains");
    check(runtime_after_follow_on_activation.runtime
              ->pager_worker_last_parked_resolution_follow_on_resolved_address_space_id ==
              third_address_space_id,
          "runtime status tracks the queued item as the parked-resolution follow-on resolved address");
    check(runtime_after_follow_on_activation.runtime
              ->pager_worker_last_parked_resolution_follow_on_resolved_handoff_sequence == 3,
          "runtime status tracks the queued item handoff ordinal at follow-on resolution");
    check(runtime_after_follow_on_activation.runtime
              ->pager_worker_last_parked_resolution_follow_on_resolution_sequence == 3,
          "runtime status tracks the follow-on resolution ordinal");
  }

  (void)axion_kernel_step(*state);
  auto fault_after_final_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_final_resolution.fault_summary.has_value(),
        "fault summary exposes final capped-backlog resolution");
  if (fault_after_final_resolution.fault_summary) {
    check(fault_after_final_resolution.fault_summary->pager_resolutions == 3,
          "fault summary counts all three pager resolutions under the capped policy");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_resolution_follow_on_activations == 1,
          "fault summary retains one parked-resolution follow-on activation");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_resolution_follow_on_resolutions == 1,
          "fault summary retains one parked-resolution follow-on resolution");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolution_follow_on_address_space_id ==
              third_address_space_id,
          "fault summary retains the queued item activated after parked resolution");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolution_follow_on_handoff_sequence == 3,
          "fault summary retains the queued item handoff ordinal after parked resolution");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolution_follow_on_activation_cycle == 3,
          "fault summary retains the follow-on activation ordinal after parked resolution");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolution_follow_on_resolved_address_space_id ==
              third_address_space_id,
          "fault summary retains the queued item as the parked-resolution follow-on resolved address");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolution_follow_on_resolved_handoff_sequence == 3,
          "fault summary retains the queued item handoff ordinal at follow-on resolution");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolution_follow_on_resolution_sequence == 3,
          "fault summary retains the follow-on resolution ordinal");
    check(fault_after_final_resolution.fault_summary->pager_worker_ready_bypass_activations == 1,
          "fault summary retains one ready-bypass activation under the capped policy");
    check(fault_after_final_resolution.fault_summary->pager_worker_ready_bypass_deferrals == 1,
          "fault summary retains one ready-bypass deferral for the parked capped episode");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_cycles == 2,
          "fault summary retains two parked worker cycles under the parked capped policy");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_resumptions == 1,
          "fault summary retains one parked resumption after the blocked head drains");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_resolved_heads == 1,
          "fault summary retains one parked-head resolution after the blocked head drains");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_ready_count == 0,
          "fault summary clears live parked-ready backlog after the parked head drains");
    check(fault_after_final_resolution.fault_summary->pager_worker_parked_ready_high_watermark == 1,
          "fault summary retains parked-ready backlog watermark after drain");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_ready_bypass_blocked_address_space_id ==
              first_address_space_id,
          "fault summary retains the blocked head used for the ready bypass");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_ready_bypass_promoted_address_space_id ==
              second_address_space_id,
          "fault summary retains the promoted ready item used for the bypass");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_ready_bypass_deferred_blocked_address_space_id ==
              first_address_space_id,
          "fault summary retains the blocked head used for the deferral");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_ready_bypass_deferred_ready_address_space_id ==
              third_address_space_id,
          "fault summary retains the deferred third ready item");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_parked_blocked_address_space_id ==
              first_address_space_id,
          "fault summary retains the blocked head for the latest parked cycle");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_parked_ready_address_space_id ==
              third_address_space_id,
          "fault summary retains the ready item for the latest parked cycle");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_parked_cycle == 2,
          "fault summary retains the latest parked cycle ordinal");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_parked_ready_count == 1,
          "fault summary retains one ready item behind the parked head");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resumed_address_space_id ==
              first_address_space_id,
          "fault summary retains the blocked head resumed from parked state");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resumed_handoff_sequence == 1,
          "fault summary retains the resumed blocked-head handoff ordinal");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_parked_resumption_cycle == 1,
          "fault summary retains the parked resumption ordinal");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_parked_resumed_ready_count == 1,
          "fault summary retains one ready item queued behind the resumed head");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resumed_ready_address_space_id ==
              third_address_space_id,
          "fault summary retains the trailing ready item behind the resumed head");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resumed_ready_handoff_sequence == 3,
          "fault summary retains the trailing ready item handoff ordinal at parked resumption");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolved_address_space_id ==
              first_address_space_id,
          "fault summary retains the blocked head as the parked-resolved address");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolved_handoff_sequence == 1,
          "fault summary retains the resumed blocked-head handoff ordinal at parked resolution");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolved_resolution_sequence == 2,
          "fault summary retains the blocked head resolution ordinal after parked resumption");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolved_remaining_inbox_count == 1,
          "fault summary retains one queued item remaining after the parked head resolves");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolved_remaining_address_space_id ==
              third_address_space_id,
          "fault summary retains the queued item remaining after the parked head resolves");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_parked_resolved_remaining_handoff_sequence == 3,
          "fault summary retains the remaining queued item handoff ordinal after parked resolution");
    check(fault_after_final_resolution.fault_summary->pager_worker_last_activated_address_space_id ==
              third_address_space_id,
          "fault summary retains the third item as the final activation after the head drains");
    check(fault_after_final_resolution.fault_summary
              ->pager_worker_last_completed_address_space_id ==
              third_address_space_id,
          "fault summary retains the third item as the final completion after the head drains");
    check(fault_after_final_resolution.fault_summary->pager_worker_stall_cycles == 0,
          "fault summary retains zero stalls under the parked capped policy");
    check(fault_after_final_resolution.fault_summary->pager_worker_backlog_blocked_cycles == 0,
          "fault summary retains zero backlog-blocked cycles under the parked capped policy");
    check(fault_after_final_resolution.fault_summary->pager_worker_ready_backlog_cycles == 0,
          "fault summary retains zero ready-backlog cycles under the parked capped policy");
  }
}

static void test_kernel_pager_worker_terminal_parked_head() {
  std::printf(
      "\n[AC-22e] Axion pager worker terminalizes an unresolved parked head\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for parked terminal policy");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext first_thread;
  first_thread.label = "pager-terminal-a";
  first_thread.registers[0] = 111;

  t81::ternaryos::sched::TiscContext second_thread;
  second_thread.label = "pager-terminal-b";
  second_thread.registers[0] = 222;

  t81::ternaryos::sched::TiscContext third_thread;
  third_thread.label = "pager-terminal-c";
  third_thread.registers[0] = 333;

  auto first_tid = axion_kernel_spawn_thread(*state, first_thread);
  auto second_tid = axion_kernel_spawn_thread(*state, second_thread);
  auto third_tid = axion_kernel_spawn_thread(*state, third_thread);
  check(first_tid.has_value(), "first terminal thread spawns successfully");
  check(second_tid.has_value(), "second terminal thread spawns successfully");
  check(third_tid.has_value(), "third terminal thread spawns successfully");
  if (!first_tid || !second_tid || !third_tid) {
    return;
  }

  const auto* first_runtime = state->find_thread_runtime(*first_tid);
  const auto* second_runtime = state->find_thread_runtime(*second_tid);
  const auto* third_runtime = state->find_thread_runtime(*third_tid);
  check(first_runtime != nullptr, "first terminal runtime state exists");
  check(second_runtime != nullptr, "second terminal runtime state exists");
  check(third_runtime != nullptr, "third terminal runtime state exists");
  if (!first_runtime || !second_runtime || !third_runtime) {
    return;
  }

  const auto first_address_space_id =
      state->find_process_group_address_space(first_runtime->process_group_id);
  const auto second_address_space_id =
      state->find_process_group_address_space(second_runtime->process_group_id);
  const auto third_address_space_id =
      state->find_process_group_address_space(third_runtime->process_group_id);
  check(first_address_space_id.has_value(),
        "first terminal process group resolves to an address space");
  check(second_address_space_id.has_value(),
        "second terminal process group resolves to an address space");
  check(third_address_space_id.has_value(),
        "third terminal process group resolves to an address space");
  if (!first_address_space_id || !second_address_space_id ||
      !third_address_space_id) {
    return;
  }

  const auto first_tva = mmu::tva_from_vpn_offset(90, 0);
  const auto second_tva = mmu::tva_from_vpn_offset(91, 0);
  const auto third_tva = mmu::tva_from_vpn_offset(92, 0);
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = first_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *first_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = second_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *second_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = third_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *third_tid,
  });

  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     second_tva,
                     *second_address_space_id),
        "second terminal address space accepts its mapping before bypass");
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     third_tva,
                     *third_address_space_id),
        "third terminal address space accepts its mapping before terminal policy");

  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  auto runtime_after_terminal = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_terminal.runtime.has_value(),
        "runtime status exposes terminal parked-head policy");
  if (runtime_after_terminal.runtime) {
    check(!runtime_after_terminal.runtime->pager_worker_busy,
          "runtime status keeps the worker idle after terminalizing the blocked head");
    check(runtime_after_terminal.runtime->pager_resolutions == 1,
          "runtime status resolves only the promoted ready item before terminalizing the blocked head");
    check(runtime_after_terminal.runtime->pager_needed_address_space_count == 2,
          "runtime status keeps the terminal head and trailing ready item pager-needed before final drain");
    check(runtime_after_terminal.runtime->pager_terminal_address_space_count == 1,
          "runtime status counts one terminal pager address space");
    check(runtime_after_terminal.runtime->pager_worker_inbox_count == 1,
          "runtime status removes the terminal head from the pager-worker inbox");
    check(runtime_after_terminal.runtime->pager_worker_next_queued_address_space_id ==
              third_address_space_id,
          "runtime status leaves the trailing ready item at the queue front after terminalizing the head");
    check(runtime_after_terminal.runtime->pager_worker_next_queued_handoff_sequence == 3,
          "runtime status preserves the trailing ready item handoff ordinal after terminalizing the head");
    check(runtime_after_terminal.runtime->pager_worker_parked_cycles == 3,
          "runtime status counts three parked cycles before the terminal rule fires");
    check(runtime_after_terminal.runtime->pager_worker_ready_bypass_deferrals == 1,
          "runtime status keeps one deferral episode before terminalizing the head");
    check(runtime_after_terminal.runtime->pager_worker_terminal_failures == 1,
          "runtime status counts one terminal parked-head failure");
    check(runtime_after_terminal.runtime->pager_worker_last_terminal_address_space_id ==
              first_address_space_id,
          "runtime status tracks the terminalized blocked head");
    check(runtime_after_terminal.runtime->pager_worker_last_terminal_handoff_sequence == 1,
          "runtime status tracks the blocked-head handoff ordinal at terminalization");
    check(runtime_after_terminal.runtime->pager_worker_last_terminal_cycle == 1,
          "runtime status tracks the first terminal failure ordinal");
  }

  auto fault_after_terminal = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_terminal.fault_summary.has_value(),
        "fault summary exposes terminal parked-head policy");
  if (fault_after_terminal.fault_summary) {
    check(fault_after_terminal.fault_summary->pager_terminal_address_spaces == 1,
          "fault summary counts one terminal pager address space");
    check(fault_after_terminal.fault_summary->pager_worker_inbox_count == 1,
          "fault summary removes the terminal head from the worker inbox");
    check(fault_after_terminal.fault_summary->pager_worker_terminal_failures == 1,
          "fault summary counts one terminal parked-head failure");
    check(fault_after_terminal.fault_summary->pager_worker_last_terminal_address_space_id ==
              first_address_space_id,
          "fault summary retains the terminalized blocked head");
    check(fault_after_terminal.fault_summary->pager_worker_last_terminal_handoff_sequence ==
              1,
          "fault summary retains the blocked-head handoff ordinal at terminalization");
    check(fault_after_terminal.fault_summary->pager_worker_last_terminal_cycle == 1,
          "fault summary retains the terminal failure ordinal");
  }

  check(axion_kernel_set_address_space_boot_critical(
            *state, *first_address_space_id, true),
        "kernel can mark a terminalized head boot-critical for reporting");
  auto runtime_after_boot_block = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_boot_block.runtime.has_value(),
        "runtime status exposes boot-blocked terminal head state");
  if (runtime_after_boot_block.runtime) {
    check(runtime_after_boot_block.runtime->boot_critical_address_space_count == 1,
          "runtime status counts one boot-critical address space in blocked state");
    check(runtime_after_boot_block.runtime->boot_critical_pager_needed_count == 0,
          "runtime status reports no pending boot progress once the head is terminal");
    check(runtime_after_boot_block.runtime->boot_critical_terminal_count == 1,
          "runtime status reports one boot-critical terminal head");
    check(!runtime_after_boot_block.runtime->boot_progress_pending,
          "runtime status clears boot progress pending when only terminal boot-critical state remains");
    check(runtime_after_boot_block.runtime->boot_progress_blocked,
          "runtime status reports boot progress blocked for a terminal boot-critical head");
  }

  auto fault_after_boot_block = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_boot_block.fault_summary.has_value(),
        "fault summary exposes boot-blocked terminal head state");
  if (fault_after_boot_block.fault_summary) {
    check(fault_after_boot_block.fault_summary->boot_critical_address_spaces == 1,
          "fault summary counts one boot-critical address space in blocked state");
    check(fault_after_boot_block.fault_summary
              ->boot_critical_pager_needed_address_spaces == 0,
          "fault summary reports no pending boot progress once the head is terminal");
    check(fault_after_boot_block.fault_summary
              ->boot_critical_terminal_address_spaces == 1,
          "fault summary reports one boot-critical terminal head");
    check(!fault_after_boot_block.fault_summary->boot_progress_pending,
          "fault summary clears boot progress pending when only terminal boot-critical state remains");
    check(fault_after_boot_block.fault_summary->boot_progress_blocked,
          "fault summary reports boot progress blocked for a terminal boot-critical head");
  }

  (void)axion_kernel_step(*state);
  auto runtime_after_trailing_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_trailing_resolution.runtime.has_value(),
        "runtime status exposes trailing ready work after terminalizing the head");
  if (runtime_after_trailing_resolution.runtime) {
    check(runtime_after_trailing_resolution.runtime->pager_resolutions == 2,
          "runtime status resolves the trailing ready item after terminalizing the blocked head");
    check(runtime_after_trailing_resolution.runtime->pager_terminal_address_space_count == 1,
          "runtime status retains the terminal address space after the trailing item drains");
    check(runtime_after_trailing_resolution.runtime->pager_needed_address_space_count == 1,
          "runtime status leaves only the terminal address space pager-needed after the trailing item drains");
    check(runtime_after_trailing_resolution.runtime->pager_worker_inbox_count == 0,
          "runtime status drains the worker inbox after the trailing item resolves");
  }

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     first_tva,
                     *first_address_space_id),
        "terminalized head still accepts a later mapping");
  (void)axion_kernel_step(*state);
  auto runtime_after_terminal_mapping = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_terminal_mapping.runtime.has_value(),
        "runtime status retains terminal state after the blocked head is mapped later");
  if (runtime_after_terminal_mapping.runtime) {
    check(runtime_after_terminal_mapping.runtime->pager_resolutions == 2,
          "runtime status does not auto-resolve a terminalized head after a later mapping");
    check(runtime_after_terminal_mapping.runtime->pager_terminal_address_space_count == 1,
          "runtime status retains one terminal address space after a later mapping");
    check(runtime_after_terminal_mapping.runtime->pager_worker_terminal_failures == 1,
          "runtime status retains one terminal failure after a later mapping");
    check(runtime_after_terminal_mapping.runtime->pager_worker_inbox_count == 0,
          "runtime status keeps the worker inbox empty after the terminalized head is mapped later");
  }
}

static void test_kernel_boot_critical_pager_resolution() {
  std::printf(
      "\n[AC-22f] Axion kernel auto-resolves boot-critical pager work\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for boot-critical pager policy");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext first_thread;
  first_thread.label = "pager-boot-critical-a";
  first_thread.registers[0] = 111;

  t81::ternaryos::sched::TiscContext second_thread;
  second_thread.label = "pager-boot-critical-b";
  second_thread.registers[0] = 222;

  t81::ternaryos::sched::TiscContext third_thread;
  third_thread.label = "pager-boot-critical-c";
  third_thread.registers[0] = 333;

  auto first_tid = axion_kernel_spawn_thread(*state, first_thread);
  auto second_tid = axion_kernel_spawn_thread(*state, second_thread);
  auto third_tid = axion_kernel_spawn_thread(*state, third_thread);
  check(first_tid.has_value(), "first boot-critical thread spawns successfully");
  check(second_tid.has_value(), "second boot-critical thread spawns successfully");
  check(third_tid.has_value(), "third boot-critical thread spawns successfully");
  if (!first_tid || !second_tid || !third_tid) {
    return;
  }

  const auto* first_runtime = state->find_thread_runtime(*first_tid);
  const auto* second_runtime = state->find_thread_runtime(*second_tid);
  const auto* third_runtime = state->find_thread_runtime(*third_tid);
  check(first_runtime != nullptr, "first boot-critical runtime state exists");
  check(second_runtime != nullptr, "second boot-critical runtime state exists");
  check(third_runtime != nullptr, "third boot-critical runtime state exists");
  if (!first_runtime || !second_runtime || !third_runtime) {
    return;
  }

  const auto first_address_space_id =
      state->find_process_group_address_space(first_runtime->process_group_id);
  const auto second_address_space_id =
      state->find_process_group_address_space(second_runtime->process_group_id);
  const auto third_address_space_id =
      state->find_process_group_address_space(third_runtime->process_group_id);
  check(first_address_space_id.has_value(),
        "first boot-critical process group resolves to an address space");
  check(second_address_space_id.has_value(),
        "second boot-critical process group resolves to an address space");
  check(third_address_space_id.has_value(),
        "third boot-critical process group resolves to an address space");
  if (!first_address_space_id || !second_address_space_id ||
      !third_address_space_id) {
    return;
  }

  check(axion_kernel_set_address_space_boot_critical(
            *state, *first_address_space_id, true),
        "kernel marks the blocked head address space boot-critical");

  const auto first_tva = mmu::tva_from_vpn_offset(93, 0);
  const auto second_tva = mmu::tva_from_vpn_offset(94, 0);
  const auto third_tva = mmu::tva_from_vpn_offset(95, 0);
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = first_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *first_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = second_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *second_tid,
  });
  state->pending_faults.push_back(KernelFaultRecord{
      .platform_id = state->platform_id,
      .tva = third_tva,
      .access_mode = mmu::MmuAccessMode::Read,
      .fault = mmu::MmuFault::Unmapped,
      .subject_tid = *third_tid,
  });

  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);
  (void)axion_kernel_step(*state);

  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     second_tva,
                     *second_address_space_id),
        "second boot-critical address space accepts its mapping before worker activation");
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     third_tva,
                     *third_address_space_id),
        "third boot-critical address space accepts its mapping before worker activation");

  auto runtime_before_boot_critical_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_before_boot_critical_resolution.runtime.has_value(),
        "runtime status exposes pending boot-critical progress before auto-resolution");
  if (runtime_before_boot_critical_resolution.runtime) {
    check(runtime_before_boot_critical_resolution.runtime
              ->boot_critical_address_space_count == 1,
          "runtime status counts one boot-critical address space before auto-resolution");
    check(runtime_before_boot_critical_resolution.runtime
              ->boot_critical_pager_needed_count == 1,
          "runtime status reports one pending boot-critical address space before auto-resolution");
    check(runtime_before_boot_critical_resolution.runtime
              ->boot_critical_terminal_count == 0,
          "runtime status reports no boot-critical terminal state before auto-resolution");
    check(runtime_before_boot_critical_resolution.runtime->boot_progress_pending,
          "runtime status reports boot progress pending before auto-resolution");
    check(!runtime_before_boot_critical_resolution.runtime->boot_progress_blocked,
          "runtime status keeps boot progress unblocked before auto-resolution");
  }

  (void)axion_kernel_step(*state);
  auto runtime_after_boot_critical_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_after_boot_critical_resolution.runtime.has_value(),
        "runtime status exposes boot-critical auto-resolution");
  if (runtime_after_boot_critical_resolution.runtime) {
    check(runtime_after_boot_critical_resolution.runtime
              ->boot_critical_address_space_count == 1,
          "runtime status counts one boot-critical address space");
    check(runtime_after_boot_critical_resolution.runtime
              ->boot_critical_pager_needed_count == 0,
          "runtime status clears pending boot progress after boot-critical resolution");
    check(runtime_after_boot_critical_resolution.runtime
              ->boot_critical_terminal_count == 0,
          "runtime status reports no boot-critical terminal state after boot-critical resolution");
    check(!runtime_after_boot_critical_resolution.runtime->boot_progress_pending,
          "runtime status clears boot progress pending after boot-critical resolution");
    check(!runtime_after_boot_critical_resolution.runtime->boot_progress_blocked,
          "runtime status keeps boot progress unblocked after boot-critical resolution");
    check(runtime_after_boot_critical_resolution.runtime->pager_resolutions == 1,
          "runtime status resolves the boot-critical blocked head first");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_ready_bypass_activations == 0,
          "runtime status does not ready-bypass the boot-critical blocked head");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_terminal_failures == 0,
          "runtime status does not terminalize the boot-critical blocked head");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_boot_critical_resolutions == 1,
          "runtime status counts one boot-critical pager resolution");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_last_boot_critical_address_space_id ==
              first_address_space_id,
          "runtime status tracks the boot-critical resolved address space");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_last_boot_critical_handoff_sequence == 1,
          "runtime status tracks the boot-critical handoff ordinal");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_last_boot_critical_resolution_sequence == 1,
          "runtime status tracks the boot-critical resolution ordinal");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_last_completed_address_space_id ==
              first_address_space_id,
          "runtime status completes the boot-critical blocked head first");
    check(runtime_after_boot_critical_resolution.runtime
              ->pager_worker_inbox_count == 2,
          "runtime status leaves the remaining queued work after boot-critical resolution");
  }

  auto fault_after_boot_critical_resolution = axion_kernel_service_request(
      *state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_after_boot_critical_resolution.fault_summary.has_value(),
        "fault summary exposes boot-critical auto-resolution");
  if (fault_after_boot_critical_resolution.fault_summary) {
    check(fault_after_boot_critical_resolution.fault_summary
              ->boot_critical_address_spaces == 1,
          "fault summary counts one boot-critical address space");
    check(fault_after_boot_critical_resolution.fault_summary
              ->boot_critical_pager_needed_address_spaces == 0,
          "fault summary clears pending boot progress after boot-critical resolution");
    check(fault_after_boot_critical_resolution.fault_summary
              ->boot_critical_terminal_address_spaces == 0,
          "fault summary reports no boot-critical terminal state after boot-critical resolution");
    check(!fault_after_boot_critical_resolution.fault_summary->boot_progress_pending,
          "fault summary clears boot progress pending after boot-critical resolution");
    check(!fault_after_boot_critical_resolution.fault_summary->boot_progress_blocked,
          "fault summary keeps boot progress unblocked after boot-critical resolution");
    check(fault_after_boot_critical_resolution.fault_summary
              ->pager_worker_boot_critical_resolutions == 1,
          "fault summary counts one boot-critical pager resolution");
    check(fault_after_boot_critical_resolution.fault_summary
              ->pager_worker_last_boot_critical_address_space_id ==
              first_address_space_id,
          "fault summary tracks the boot-critical resolved address space");
    check(fault_after_boot_critical_resolution.fault_summary
              ->pager_worker_last_boot_critical_handoff_sequence == 1,
          "fault summary tracks the boot-critical handoff ordinal");
    check(fault_after_boot_critical_resolution.fault_summary
              ->pager_worker_last_boot_critical_resolution_sequence == 1,
          "fault summary tracks the boot-critical resolution ordinal");
  }
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

static void test_kernel_fault_acknowledgement_and_recovery() {
  std::printf("\n[AC-24] Axion kernel acknowledges and recovers quarantined threads\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for fault acknowledgement");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext thread_a;
  thread_a.label = "recover-a";
  thread_a.registers[0] = 301;
  t81::ternaryos::sched::TiscContext thread_b;
  thread_b.label = "recover-b";
  thread_b.registers[0] = 302;

  auto tid_a = axion_kernel_spawn_thread(*state, thread_a);
  auto tid_b = axion_kernel_spawn_thread(*state, thread_b);
  check(tid_a.has_value(), "recovery runtime spawns thread A");
  check(tid_b.has_value(), "recovery runtime spawns thread B");
  if (!tid_a || !tid_b) {
    return;
  }

  check(axion_kernel_step(*state), "first recovery step dispatches thread A");
  check(state->scheduler.current_tid() == *tid_a, "thread A is current before recovery fault");
  auto fault_report = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(33, 0), mmu::MmuAccessMode::Read);
  check(fault_report.fault.has_value(), "recovery path records a thread fault");
  check(axion_kernel_step(*state), "second recovery step delivers and quarantines thread A");

  const auto* runtime_a = state->find_thread_runtime(*tid_a);
  check(runtime_a != nullptr, "recovery path exposes thread A runtime state");
  if (!runtime_a) {
    return;
  }
  check(runtime_a->quarantined, "thread A is quarantined before acknowledgement");
  check(runtime_a->fault_inbox.size() == 1, "thread A inbox has one pending fault");
  check(state->scheduler.current_tid() == *tid_b,
        "thread B is current while thread A is quarantined");

  check(!axion_kernel_ack_thread_fault(*state, *tid_b),
        "non-faulting thread cannot acknowledge an empty fault inbox");
  check(axion_kernel_ack_thread_fault(*state, *tid_a),
        "faulting thread acknowledgement succeeds");
  check(state->counters.thread_fault_acknowledgements == 1,
        "runtime counts fault acknowledgements");
  check(state->counters.thread_fault_recoveries == 0,
        "thread acknowledgement alone does not recover the quarantined thread");

  runtime_a = state->find_thread_runtime(*tid_a);
  check(runtime_a->fault_inbox.empty(), "acknowledgement drains thread A fault inbox");
  check(runtime_a->quarantined,
        "thread A remains quarantined until its process group is acknowledged");
  check(axion_kernel_ack_process_group_fault(*state, runtime_a->process_group_id),
        "faulted process group acknowledgement succeeds after thread inbox drains");
  check(state->counters.process_group_acknowledgements == 1,
        "runtime counts process-group acknowledgements in the recovery path");
  check(state->counters.thread_fault_recoveries == 1,
        "group acknowledgement triggers deterministic thread recovery");

  runtime_a = state->find_thread_runtime(*tid_a);
  check(!runtime_a->quarantined, "thread A leaves quarantine after group acknowledgement");

  check(axion_kernel_step(*state), "post-acknowledgement step continues runtime");
  check(state->scheduler.current_tid() == *tid_a,
        "thread A becomes runnable again after acknowledgement");
}

static void test_kernel_process_group_fault_gate() {
  std::printf("\n[AC-25] Axion kernel process-group fault gate requires explicit group acknowledgement\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for process-group fault gate");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext leader;
  leader.label = "group-leader";
  leader.registers[0] = 401;
  auto leader_tid = axion_kernel_spawn_thread(*state, leader);
  check(leader_tid.has_value(), "group leader spawns successfully");
  if (!leader_tid) {
    return;
  }

  const auto* leader_runtime = state->find_thread_runtime(*leader_tid);
  check(leader_runtime != nullptr, "leader runtime state exists");
  if (!leader_runtime) {
    return;
  }
  const auto group_id = leader_runtime->process_group_id;

  t81::ternaryos::sched::TiscContext sibling;
  sibling.label = "group-sibling";
  sibling.registers[0] = 402;
  auto sibling_tid = axion_kernel_spawn_thread_in_group(*state, sibling, group_id);
  check(sibling_tid.has_value(), "sibling thread spawns into existing process group");

  t81::ternaryos::sched::TiscContext outsider;
  outsider.label = "other-group";
  outsider.registers[0] = 403;
  auto outsider_tid = axion_kernel_spawn_thread(*state, outsider);
  check(outsider_tid.has_value(), "outsider thread spawns with its own process group");
  if (!sibling_tid || !outsider_tid) {
    return;
  }

  const auto* group_before_fault = state->find_process_group(group_id);
  check(group_before_fault != nullptr, "shared process group exists");
  if (group_before_fault) {
    check(group_before_fault->member_tids.size() == 2,
          "shared process group tracks deterministic membership");
  }

  check(axion_kernel_step(*state), "first group step dispatches the leader");
  check(state->scheduler.current_tid() == *leader_tid, "leader is current before group fault");

  auto fault_report = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(39, 0), mmu::MmuAccessMode::Read);
  check(fault_report.fault.has_value(), "group fault records for running leader");
  check(axion_kernel_step(*state), "second group step delivers the fault");

  leader_runtime = state->find_thread_runtime(*leader_tid);
  const auto* sibling_runtime = state->find_thread_runtime(*sibling_tid);
  const auto* outsider_runtime = state->find_thread_runtime(*outsider_tid);
  const auto* faulted_group = state->find_process_group(group_id);
  check(leader_runtime != nullptr, "leader runtime state is still present");
  check(sibling_runtime != nullptr, "sibling runtime state is still present");
  check(outsider_runtime != nullptr, "outsider runtime state is still present");
  check(faulted_group != nullptr, "faulted process group state exists");
  if (!leader_runtime || !sibling_runtime || !outsider_runtime || !faulted_group) {
    return;
  }

  check(leader_runtime->quarantined, "faulting thread is quarantined");
  check(!sibling_runtime->quarantined, "same-group sibling is not auto-quarantined");
  check(!outsider_runtime->quarantined, "other-group thread remains runnable");
  check(faulted_group->faulted, "faulted process group enters fault state");
  check(faulted_group->blocked, "faulted process group enters blocked state");
  check(faulted_group->acknowledgement_pending,
        "faulted process group requires explicit acknowledgement");
  check(faulted_group->pending_fault_count == 1,
        "faulted process group counts one pending fault");
  check(state->counters.process_group_fault_entries == 1,
        "runtime counts process-group fault entries");

  check(!axion_kernel_ack_thread_fault(*state, *outsider_tid),
        "other-group thread cannot acknowledge an empty fault inbox");
  check(axion_kernel_ack_thread_fault(*state, *leader_tid),
        "faulting thread acknowledgement drains its thread inbox");
  leader_runtime = state->find_thread_runtime(*leader_tid);
  check(leader_runtime->quarantined,
        "thread remains quarantined until its group is acknowledged");
  check(!axion_kernel_ack_process_group_fault(*state, outsider_runtime->process_group_id),
        "unfaulted process group cannot be acknowledged");
  check(axion_kernel_ack_process_group_fault(*state, group_id),
        "faulted process group acknowledgement succeeds");

  const auto* recovered_group = state->find_process_group(group_id);
  check(recovered_group != nullptr, "recovered process group state exists");
  if (recovered_group) {
    check(!recovered_group->faulted, "group fault state clears after acknowledgement");
    check(!recovered_group->blocked, "group blocked state clears after acknowledgement");
    check(!recovered_group->acknowledgement_pending,
          "group acknowledgement gate clears after acknowledgement");
    check(recovered_group->pending_fault_count == 0,
          "group pending fault count drains after thread and group acknowledgement");
  }

  leader_runtime = state->find_thread_runtime(*leader_tid);
  check(leader_runtime != nullptr, "leader runtime state remains visible after recovery");
  if (!leader_runtime) {
    return;
  }
  check(!leader_runtime->quarantined, "leader leaves quarantine after group acknowledgement");
  check(state->counters.process_group_acknowledgements == 1,
        "runtime counts process-group acknowledgements");
  check(state->counters.process_group_recoveries == 1,
        "runtime counts process-group-mediated thread recoveries");

  check(axion_kernel_step(*state), "post-group-ack step continues the runtime");
  check(state->scheduler.current_tid() == *sibling_tid ||
            state->scheduler.current_tid() == *leader_tid ||
            state->scheduler.current_tid() == *outsider_tid,
        "same-group runtime remains schedulable after recovery");
}

static void test_kernel_process_group_audit_log() {
  std::printf("\n[AC-26] Axion kernel emits deterministic process-group audit events\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for process-group audit log");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext thread;
  thread.label = "audit-thread";
  thread.registers[0] = 501;
  auto tid = axion_kernel_spawn_thread(*state, thread);
  check(tid.has_value(), "audit thread spawns successfully");
  if (!tid) {
    return;
  }

  const auto* runtime = state->find_thread_runtime(*tid);
  check(runtime != nullptr, "audit thread runtime state exists");
  if (!runtime) {
    return;
  }
  const auto group_id = runtime->process_group_id;

  check(axion_kernel_step(*state), "audit step dispatches the faulting thread");
  auto fault_report = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(45, 0), mmu::MmuAccessMode::Read);
  check(fault_report.fault.has_value(), "audit path records a fault");
  check(axion_kernel_step(*state), "audit step delivers the fault");
  check(axion_kernel_ack_thread_fault(*state, *tid),
        "thread acknowledgement succeeds for audit test");
  check(axion_kernel_ack_process_group_fault(*state, group_id),
        "group acknowledgement succeeds for audit test");

  check(state->audit_count() >= 6, "audit log records the expected governance events");
  check(state->counters.audit_events_recorded >= 6,
        "runtime counts recorded audit events");
  check(state->last_audit_event.has_value(), "runtime exposes the latest audit event");

  std::vector<KernelAuditEventKind> observed;
  for (const auto& entry : state->audit_log) {
    if (entry.subject_tid == *tid || entry.process_group_id == group_id) {
      observed.push_back(entry.kind);
    }
  }
  check(observed.size() >= 6, "audit log exposes at least six relevant events");
  if (observed.size() >= 7) {
    check(observed[0] == KernelAuditEventKind::FaultDelivered,
          "audit event[0] is fault delivered");
    check(observed[1] == KernelAuditEventKind::ProcessGroupFaultEntered,
          "audit event[1] is process-group fault entry");
    check(observed[2] == KernelAuditEventKind::SupervisorFaultNotified,
          "audit event[2] is supervisor fault notification");
    check(observed[3] == KernelAuditEventKind::ThreadQuarantined,
          "audit event[3] is thread quarantined");
    check(observed[4] == KernelAuditEventKind::ThreadFaultAcknowledged,
          "audit event[4] is thread acknowledgement");
    check(observed[5] == KernelAuditEventKind::ProcessGroupAcknowledged,
          "audit event[5] is process-group acknowledgement");
    check(observed[6] == KernelAuditEventKind::ThreadRecovered,
          "audit event[6] is thread recovery");
  }

  uint64_t last_sequence = 0;
  for (const auto& entry : state->audit_log) {
    check(entry.sequence > last_sequence, "audit event sequences are strictly increasing");
    last_sequence = entry.sequence;
  }
}

static void test_kernel_supervisor_fault_boundary() {
  std::printf("\n[AC-27] Axion kernel assigns deterministic supervisor ownership to process groups\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for supervisor boundary");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext leader;
  leader.label = "supervisor-leader";
  leader.registers[0] = 601;
  auto leader_tid = axion_kernel_spawn_thread(*state, leader);
  check(leader_tid.has_value(), "leader thread spawns successfully");
  if (!leader_tid) {
    return;
  }

  const auto* leader_runtime = state->find_thread_runtime(*leader_tid);
  check(leader_runtime != nullptr, "leader runtime exists");
  if (!leader_runtime) {
    return;
  }
  const auto group_a = leader_runtime->process_group_id;

  t81::ternaryos::sched::TiscContext sibling;
  sibling.label = "supervisor-sibling";
  sibling.registers[0] = 602;
  auto sibling_tid = axion_kernel_spawn_thread_in_group(*state, sibling, group_a);
  check(sibling_tid.has_value(), "sibling thread spawns into leader group");

  t81::ternaryos::sched::TiscContext outsider;
  outsider.label = "supervisor-outsider";
  outsider.registers[0] = 603;
  auto outsider_tid = axion_kernel_spawn_thread(*state, outsider);
  check(outsider_tid.has_value(), "outsider thread spawns successfully");
  if (!sibling_tid || !outsider_tid) {
    return;
  }

  const auto* outsider_runtime = state->find_thread_runtime(*outsider_tid);
  check(outsider_runtime != nullptr, "outsider runtime exists");
  if (!outsider_runtime) {
    return;
  }
  const auto group_b = outsider_runtime->process_group_id;

  auto supervisor_a = state->find_process_group_supervisor(group_a);
  auto supervisor_b = state->find_process_group_supervisor(group_b);
  check(supervisor_a.has_value(), "leader group resolves to a supervisor");
  check(supervisor_b.has_value(), "outsider group resolves to a supervisor");
  if (!supervisor_a || !supervisor_b) {
    return;
  }
  check(*supervisor_a != *supervisor_b, "distinct process groups get distinct supervisors");
  check(state->supervisor_count() == 3,
        "kernel runtime tracks kernel supervisor plus two user supervisors");

  const auto* supervisor_a_state = state->find_supervisor(*supervisor_a);
  const auto* supervisor_b_state = state->find_supervisor(*supervisor_b);
  check(supervisor_a_state != nullptr, "supervisor A state exists");
  check(supervisor_b_state != nullptr, "supervisor B state exists");
  if (!supervisor_a_state || !supervisor_b_state) {
    return;
  }
  check(supervisor_a_state->managed_groups.size() == 1,
        "supervisor A manages exactly one process group");
  check(supervisor_b_state->managed_groups.size() == 1,
        "supervisor B manages exactly one process group");

  check(axion_kernel_step(*state), "supervisor boundary step dispatches leader");
  check(state->scheduler.current_tid() == *leader_tid, "leader runs before supervisor fault");

  auto fault_report = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(51, 0), mmu::MmuAccessMode::Read);
  check(fault_report.fault.has_value(), "supervisor boundary records leader fault");
  check(axion_kernel_step(*state), "supervisor boundary delivers the leader fault");

  supervisor_a_state = state->find_supervisor(*supervisor_a);
  supervisor_b_state = state->find_supervisor(*supervisor_b);
  check(supervisor_a_state->pending_groups.size() == 1,
        "faulted group is queued exactly once for its supervisor");
  check(supervisor_a_state->pending_groups.front() == group_a,
        "supervisor pending queue records the owning process group");
  check(supervisor_b_state->pending_groups.empty(),
        "unrelated supervisor remains unaffected");
  check(state->counters.supervisor_fault_notifications == 1,
        "runtime counts one supervisor fault notification");
}

static void test_kernel_supervisor_acknowledgement_flow() {
  std::printf("\n[AC-28] Axion kernel requires supervisor acknowledgement for group recovery\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for supervisor acknowledgement");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext worker;
  worker.label = "supervisor-worker";
  worker.registers[0] = 611;
  auto worker_tid = axion_kernel_spawn_thread(*state, worker);
  check(worker_tid.has_value(), "worker thread spawns successfully");
  if (!worker_tid) {
    return;
  }

  const auto* worker_runtime = state->find_thread_runtime(*worker_tid);
  check(worker_runtime != nullptr, "worker runtime exists");
  if (!worker_runtime) {
    return;
  }
  const auto group_id = worker_runtime->process_group_id;
  auto supervisor_id = state->find_process_group_supervisor(group_id);
  check(supervisor_id.has_value(), "worker group resolves to a supervisor");
  if (!supervisor_id) {
    return;
  }

  check(axion_kernel_step(*state), "supervisor ack step dispatches the worker");
  auto fault_report = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(57, 0), mmu::MmuAccessMode::Read);
  check(fault_report.fault.has_value(), "supervisor ack path records a fault");
  check(axion_kernel_step(*state), "supervisor ack step delivers the fault");

  check(!axion_kernel_ack_supervisor_group_fault(*state,
                                                 KernelRuntimeState::kKernelSupervisor,
                                                 group_id),
        "wrong supervisor cannot acknowledge a foreign process group");
  check(!axion_kernel_ack_supervisor_group_fault(*state, *supervisor_id, group_id),
        "supervisor acknowledgement is gated until thread inbox drains");
  check(axion_kernel_ack_thread_fault(*state, *worker_tid),
        "thread acknowledgement drains the worker fault inbox");

  worker_runtime = state->find_thread_runtime(*worker_tid);
  check(worker_runtime->quarantined,
        "worker remains quarantined until supervisor acknowledges the group");
  check(axion_kernel_ack_supervisor_group_fault(*state, *supervisor_id, group_id),
        "owning supervisor acknowledges the faulted group");

  const auto* supervisor_state = state->find_supervisor(*supervisor_id);
  const auto* group_state = state->find_process_group(group_id);
  worker_runtime = state->find_thread_runtime(*worker_tid);
  check(supervisor_state != nullptr, "supervisor state remains visible");
  check(group_state != nullptr, "group state remains visible");
  if (!supervisor_state || !group_state || !worker_runtime) {
    return;
  }

  check(supervisor_state->pending_groups.empty(),
        "supervisor pending queue drains after acknowledgement");
  check(supervisor_state->acknowledgements == 1,
        "supervisor acknowledgement counter increments");
  check(state->counters.supervisor_acknowledgements == 1,
        "runtime counts supervisor acknowledgements");
  check(!group_state->faulted, "group fault state clears after supervisor acknowledgement");
  check(!worker_runtime->quarantined, "worker recovers after supervisor-mediated acknowledgement");

  std::vector<KernelAuditEventKind> observed;
  for (const auto& entry : state->audit_log) {
    if (entry.subject_tid == *worker_tid || entry.process_group_id == group_id) {
      observed.push_back(entry.kind);
    }
  }
  check(observed.size() >= 8, "audit log records supervisor-facing events");
  if (observed.size() >= 8) {
    check(observed[0] == KernelAuditEventKind::FaultDelivered,
          "audit event[0] is fault delivered");
    check(observed[1] == KernelAuditEventKind::ProcessGroupFaultEntered,
          "audit event[1] is process-group fault entry");
    check(observed[2] == KernelAuditEventKind::SupervisorFaultNotified,
          "audit event[2] is supervisor fault notification");
    check(observed[3] == KernelAuditEventKind::ThreadQuarantined,
          "audit event[3] is thread quarantined");
    check(observed[4] == KernelAuditEventKind::ThreadFaultAcknowledged,
          "audit event[4] is thread acknowledgement");
    check(observed[5] == KernelAuditEventKind::SupervisorGroupAcknowledged,
          "audit event[5] is supervisor acknowledgement");
    check(observed[6] == KernelAuditEventKind::ProcessGroupAcknowledged,
          "audit event[6] is process-group acknowledgement");
    check(observed[7] == KernelAuditEventKind::ThreadRecovered,
          "audit event[7] is thread recovery");
  }
}

static void test_kernel_service_runtime_views() {
  std::printf("\n[AC-29] Axion kernel service contract exposes deterministic runtime views\n");

  auto hosted_ctx = make_valid_ctx(/*ethics=*/false);
  auto hosted_state = axion_kernel_bootstrap(hosted_ctx);
  check(hosted_state.has_value(), "kernel bootstrap succeeds for service runtime views");
  if (!hosted_state) {
    return;
  }

  auto runtime_result = axion_kernel_service_request(
      *hosted_state, KernelServiceRequest{.kind = KernelServiceRequestKind::RuntimeStatus});
  check(runtime_result.status == KernelServiceStatus::Ok,
        "runtime status request succeeds");
  check(runtime_result.rejection == KernelServiceRequestRejection::None,
        "successful runtime status clears request rejection");
  check(runtime_result.runtime.has_value(), "runtime status view is returned");
  if (runtime_result.runtime) {
    check(runtime_result.runtime->memory_region_count == hosted_ctx.memory_map.size(),
          "runtime status reports deterministic memory-map count");
    check(runtime_result.runtime->platform_id == hosted_ctx.platform_id,
          "runtime status reports platform id");
    check(runtime_result.runtime->address_space_count == 1,
          "runtime status starts with one kernel address space");
    check(runtime_result.runtime->mapped_pages == 0,
          "runtime status starts with zero mapped pages");
    check(runtime_result.runtime->managed_service_count == 0,
          "runtime status starts with zero managed services");
    check(runtime_result.runtime->service_lifecycle_transitions == 0,
          "runtime status starts with zero service lifecycle transitions");
    check(!runtime_result.runtime->last_service_transition_kind.has_value(),
          "runtime status starts without service transition metadata");
  }

  auto hosted_device_result = axion_kernel_service_request(
      *hosted_state, KernelServiceRequest{.kind = KernelServiceRequestKind::DeviceSummary});
  check(hosted_device_result.status == KernelServiceStatus::NoDeviceArbitration,
        "hosted generic context reports no device arbitration");
  check(hosted_device_result.rejection ==
            KernelServiceRequestRejection::MissingDeviceArbitration,
        "hosted generic device summary reports MissingDeviceArbitration rejection");

  auto vbox_ctx = make_virtualbox_boot_context(VBoxBootSpec{});
  auto vbox_state = axion_kernel_bootstrap(vbox_ctx);
  check(vbox_state.has_value(), "kernel bootstrap succeeds for VirtualBox service runtime views");
  if (!vbox_state) {
    return;
  }

  auto device_result = axion_kernel_service_request(
      *vbox_state, KernelServiceRequest{.kind = KernelServiceRequestKind::DeviceSummary});
  check(device_result.status == KernelServiceStatus::Ok,
        "VirtualBox device summary request succeeds");
  check(device_result.rejection == KernelServiceRequestRejection::None,
        "successful device summary clears request rejection");
  check(device_result.device_summary.has_value(), "device summary view is returned");
  if (device_result.device_summary) {
    check(device_result.device_summary->has_device_arbitration,
          "device summary reports arbitration present");
    check(device_result.device_summary->device_count == 5,
          "device summary reports five supported devices");
    check(device_result.device_summary->has_storage,
          "device summary reports storage");
    check(device_result.device_summary->has_network,
          "device summary reports network");
    check(device_result.device_summary->has_display,
          "device summary reports display");
    check(device_result.device_summary->claimed_device_count == 0,
          "device summary starts with zero claimed devices");
    check(device_result.device_summary->service_lifecycle_transitions == 0,
          "device summary starts with zero service lifecycle transitions");
    check(!device_result.device_summary->last_service_transition_kind.has_value(),
          "device summary starts without service transition metadata");
  }

  auto fault_result = axion_kernel_service_request(
      *vbox_state, KernelServiceRequest{.kind = KernelServiceRequestKind::FaultSummary});
  check(fault_result.status == KernelServiceStatus::Ok,
        "fault summary request succeeds");
  check(fault_result.rejection == KernelServiceRequestRejection::None,
        "successful fault summary clears request rejection");
  check(fault_result.fault_summary.has_value(), "fault summary view is returned");
  if (fault_result.fault_summary) {
    check(fault_result.fault_summary->recorded_faults == 0,
          "fault summary starts with zero recorded faults");
    check(fault_result.fault_summary->pending_faults == 0,
          "fault summary starts with zero pending faults");
    check(fault_result.fault_summary->delivered_faults == 0,
          "fault summary starts with zero delivered faults");
    check(fault_result.fault_summary->routed_thread_faults == 0,
          "fault summary starts with zero routed thread faults");
    check(fault_result.fault_summary->service_lifecycle_transitions == 0,
          "fault summary starts with zero service lifecycle transitions");
    check(!fault_result.fault_summary->last_audit_event.has_value(),
          "fault summary starts without audit events");
    check(!fault_result.fault_summary->last_service_transition_kind.has_value(),
          "fault summary starts without service transition metadata");
  }

  auto audit_result = axion_kernel_service_request(
      *vbox_state, KernelServiceRequest{.kind = KernelServiceRequestKind::AuditSummary});
  check(audit_result.status == KernelServiceStatus::Ok,
        "audit summary request succeeds");
  check(audit_result.rejection == KernelServiceRequestRejection::None,
        "successful audit summary clears request rejection");
  check(audit_result.audit_summary.has_value(), "audit summary view is returned");
  if (audit_result.audit_summary) {
    check(audit_result.audit_summary->audit_events == 0,
          "audit summary starts with zero audit events");
    check(audit_result.audit_summary->service_lifecycle_transitions == 0,
          "audit summary starts with zero service lifecycle transitions");
    check(!audit_result.audit_summary->last_service_transition_kind.has_value(),
          "audit summary starts without service transition metadata");
    check(audit_result.audit_summary->recent_events.empty(),
          "audit summary starts with zero recent events");
  }
}

static void test_kernel_service_group_fault_visibility() {
  std::printf("\n[AC-30] Axion kernel service contract distinguishes healthy and faulted groups\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for service fault visibility");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext faulted_ctx;
  faulted_ctx.label = "service-faulted";
  faulted_ctx.registers[0] = 701;
  auto faulted_tid = axion_kernel_spawn_thread(*state, faulted_ctx);
  check(faulted_tid.has_value(), "faulted thread spawns");

  t81::ternaryos::sched::TiscContext healthy_ctx;
  healthy_ctx.label = "service-healthy";
  healthy_ctx.registers[0] = 702;
  auto healthy_tid = axion_kernel_spawn_thread(*state, healthy_ctx);
  check(healthy_tid.has_value(), "healthy thread spawns");
  if (!faulted_tid || !healthy_tid) {
    return;
  }

  const auto* faulted_runtime = state->find_thread_runtime(*faulted_tid);
  const auto* healthy_runtime = state->find_thread_runtime(*healthy_tid);
  check(faulted_runtime != nullptr, "faulted runtime exists");
  check(healthy_runtime != nullptr, "healthy runtime exists");
  if (!faulted_runtime || !healthy_runtime) {
    return;
  }
  const auto faulted_group_id = faulted_runtime->process_group_id;
  const auto healthy_group_id = healthy_runtime->process_group_id;
  const auto faulted_address_space_id =
      state->find_process_group_address_space(faulted_group_id);
  check(faulted_address_space_id.has_value(),
        "faulted group resolves to an address space");
  if (!faulted_address_space_id) {
    return;
  }
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     mmu::tva_from_vpn_offset(92, 0),
                     *faulted_address_space_id),
        "faulted group address space accepts an owned mapping");
  auto healthy_before = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = healthy_group_id,
      });
  check(healthy_before.status == KernelServiceStatus::Ok,
        "healthy group request is OK before any fault");
  check(healthy_before.process_group.has_value(), "healthy group view is returned");
  if (healthy_before.process_group) {
    check(!healthy_before.process_group->faulted, "healthy group starts not faulted");
  }

  check(axion_kernel_step(*state), "service fault visibility dispatches faulted thread");
  check(state->scheduler.current_tid() == *faulted_tid, "faulted thread runs first");
  auto access_result = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(63, 0), mmu::MmuAccessMode::Read);
  check(access_result.fault.has_value(), "faulted thread records MMU fault");
  check(axion_kernel_step(*state), "service fault visibility delivers the fault");

  auto faulted_group = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = faulted_group_id,
      });
  check(faulted_group.status == KernelServiceStatus::FaultedGroup,
        "faulted group request returns FaultedGroup status");
  check(faulted_group.rejection == KernelServiceRequestRejection::None,
        "faulted subject group status still returns a concrete group view");
  check(faulted_group.process_group.has_value(), "faulted group view is returned");
  if (faulted_group.process_group) {
    check(faulted_group.process_group->address_space_id == faulted_address_space_id,
          "faulted group view reports address-space ownership");
    check(faulted_group.process_group->owned_page_count == 1,
          "faulted group view reports one owned mapped page");
    check(faulted_group.process_group->member_count == 1,
          "faulted group view reports one member");
    check(faulted_group.process_group->quarantined_thread_count == 1,
          "faulted group view reports one quarantined thread");
    check(faulted_group.process_group->faulted, "faulted group view reports faulted");
    check(faulted_group.process_group->blocked, "faulted group view reports blocked");
    check(faulted_group.process_group->acknowledgement_pending,
          "faulted group view reports acknowledgement pending");
    check(faulted_group.process_group->pending_fault_count == 1,
          "faulted group view reports one pending fault");
    check(faulted_group.process_group->audit_events >= 2,
          "faulted group view reports audit activity");
    check(faulted_group.process_group->fault_entries == 1,
          "faulted group view reports one fault entry");
    check(faulted_group.process_group->supervisor_id.has_value(),
          "faulted group view reports supervisor ownership");
  }

  auto healthy_after = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = healthy_group_id,
      });
  check(healthy_after.status == KernelServiceStatus::Ok,
        "unaffected group request remains OK");
  check(healthy_after.process_group.has_value(), "healthy group view remains available");
  if (healthy_after.process_group) {
    check(healthy_after.process_group->owned_page_count == 0,
          "unaffected group keeps zero owned mapped pages");
    check(!healthy_after.process_group->faulted, "unaffected group remains healthy");
    check(healthy_after.process_group->quarantined_thread_count == 0,
          "unaffected group has no quarantined threads");
  }

  auto supervisor_id = state->find_process_group_supervisor(faulted_group_id);
  check(supervisor_id.has_value(), "faulted group resolves to supervisor");
  if (!supervisor_id) {
    return;
  }
  auto supervisor_result = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorStatus,
          .supervisor_id = *supervisor_id,
      });
  check(supervisor_result.status == KernelServiceStatus::Ok,
        "supervisor status request succeeds");
  check(supervisor_result.rejection == KernelServiceRequestRejection::None,
        "successful supervisor status clears request rejection");
  check(supervisor_result.supervisor.has_value(), "supervisor status view is returned");
  if (supervisor_result.supervisor) {
    check(supervisor_result.supervisor->managed_group_count == 1,
          "supervisor view reports one managed group");
    check(supervisor_result.supervisor->managed_faulted_group_count == 1,
          "supervisor view reports one faulted group");
    check(supervisor_result.supervisor->pending_group_count == 1,
          "supervisor view reports one pending group");
    check(supervisor_result.supervisor->fault_notifications == 1,
          "supervisor view reports one fault notification");
    check(supervisor_result.supervisor->last_pending_group == faulted_group_id,
          "supervisor view reports the faulted group as last pending");
  }

  auto denied_runtime = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::RuntimeStatus,
          .requesting_process_group_id = faulted_group_id,
      });
  check(denied_runtime.status == KernelServiceStatus::FaultedGroup,
        "faulted group cannot request runtime status");
  check(denied_runtime.rejection ==
            KernelServiceRequestRejection::FaultedRequestingGroup,
        "faulted runtime request reports FaultedRequestingGroup rejection");

  auto denied_faults = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::FaultSummary,
          .requesting_process_group_id = faulted_group_id,
      });
  check(denied_faults.status == KernelServiceStatus::FaultedGroup,
        "faulted group cannot request fault summary");
  check(denied_faults.rejection ==
            KernelServiceRequestRejection::FaultedRequestingGroup,
        "faulted fault-summary request reports FaultedRequestingGroup rejection");

  auto denied_audit = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::AuditSummary,
          .requesting_process_group_id = faulted_group_id,
      });
  check(denied_audit.status == KernelServiceStatus::FaultedGroup,
        "faulted group cannot request audit summary");
  check(denied_audit.rejection ==
            KernelServiceRequestRejection::FaultedRequestingGroup,
        "faulted audit request reports FaultedRequestingGroup rejection");

  auto healthy_runtime_result = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::RuntimeStatus,
          .requesting_process_group_id = healthy_group_id,
      });
  check(healthy_runtime_result.status == KernelServiceStatus::Ok,
        "healthy group can request runtime status");
  check(healthy_runtime_result.rejection == KernelServiceRequestRejection::None,
        "healthy runtime request clears request rejection");

  auto healthy_faults = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::FaultSummary,
          .requesting_process_group_id = healthy_group_id,
      });
  check(healthy_faults.status == KernelServiceStatus::Ok,
        "healthy group can request fault summary");
  check(healthy_faults.rejection == KernelServiceRequestRejection::None,
        "healthy fault-summary request clears request rejection");
  check(healthy_faults.fault_summary.has_value(),
        "healthy group receives fault summary view");
  if (healthy_faults.fault_summary) {
    check(healthy_faults.fault_summary->recorded_faults == 1,
          "fault summary reports one recorded fault");
    check(healthy_faults.fault_summary->pending_faults == 0,
          "fault summary reports no pending faults after delivery");
    check(healthy_faults.fault_summary->audit_events >= 4,
          "fault summary reports audit count");
    check(healthy_faults.fault_summary->last_audit_event.has_value(),
          "fault summary reports last audit event");
    if (healthy_faults.fault_summary->last_audit_event) {
      check(healthy_faults.fault_summary->last_audit_event->kind ==
                KernelAuditEventKind::ThreadQuarantined,
            "fault summary last audit event is thread quarantine");
    }
    check(healthy_faults.fault_summary->service_lifecycle_transitions == 0,
          "fault summary keeps service lifecycle transitions at zero in a pure fault flow");
    check(!healthy_faults.fault_summary->last_service_transition_sequence.has_value(),
          "fault summary has no service transition metadata in a pure fault flow");
  }

  auto missing_group = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = 999999,
      });
  check(missing_group.status == KernelServiceStatus::NotFound,
        "missing process group returns NotFound");
  check(missing_group.rejection == KernelServiceRequestRejection::MissingProcessGroup,
        "missing process group reports MissingProcessGroup rejection");

  auto missing_requester = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::RuntimeStatus,
          .requesting_process_group_id = 999999,
      });
  check(missing_requester.status == KernelServiceStatus::NotFound,
        "missing requesting process group returns NotFound");
  check(missing_requester.rejection ==
            KernelServiceRequestRejection::MissingRequestingGroup,
        "missing requesting process group reports MissingRequestingGroup rejection");

  auto missing_supervisor_request = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorStatus,
      });
  check(missing_supervisor_request.status == KernelServiceStatus::InvalidRequest,
        "missing supervisor request returns InvalidRequest");
  check(missing_supervisor_request.rejection ==
            KernelServiceRequestRejection::MissingSupervisor,
        "missing supervisor request reports MissingSupervisor rejection");
}

static void test_kernel_service_fault_ack_action() {
  std::printf("\n[AC-31] Axion kernel service contract supports supervisor fault acknowledgement\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for service fault acknowledgement");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext worker_ctx;
  worker_ctx.label = "service-ack-worker";
  worker_ctx.registers[0] = 801;
  auto worker_tid = axion_kernel_spawn_thread(*state, worker_ctx);
  check(worker_tid.has_value(), "worker thread spawns");
  if (!worker_tid) {
    return;
  }

  const auto* worker_runtime = state->find_thread_runtime(*worker_tid);
  check(worker_runtime != nullptr, "worker runtime exists");
  if (!worker_runtime) {
    return;
  }
  const auto group_id = worker_runtime->process_group_id;
  auto supervisor_id = state->find_process_group_supervisor(group_id);
  check(supervisor_id.has_value(), "worker process group resolves to a supervisor");
  if (!supervisor_id) {
    return;
  }

  check(axion_kernel_step(*state), "service action dispatches worker thread");
  check(state->scheduler.current_tid() == *worker_tid, "worker thread runs first");
  auto access_result = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(71, 0), mmu::MmuAccessMode::Read);
  check(access_result.fault.has_value(), "worker thread records MMU fault");
  check(axion_kernel_step(*state), "service action delivers the fault");

  auto premature_action = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::AcknowledgeSupervisorFaultGroup,
          .supervisor_id = *supervisor_id,
          .process_group_id = group_id,
      });
  check(premature_action.status == KernelServiceStatus::InvalidRequest,
        "service action rejects supervisor acknowledgement before thread inbox drain");
  check(premature_action.rejection ==
            KernelServiceActionRejection::SupervisorGatePendingThreadFault,
        "premature supervisor acknowledgement reports pending-thread-fault rejection");

  check(axion_kernel_ack_thread_fault(*state, *worker_tid),
        "thread-local acknowledgement drains the worker inbox");

  auto missing_supervisor = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::AcknowledgeSupervisorFaultGroup,
          .supervisor_id = 999999,
          .process_group_id = group_id,
      });
  check(missing_supervisor.status == KernelServiceStatus::NotFound,
        "service action returns NotFound for missing supervisor");
  check(missing_supervisor.rejection ==
            KernelServiceActionRejection::MissingSupervisor,
        "missing supervisor reports MissingSupervisor rejection");

  auto missing_requester = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::AcknowledgeSupervisorFaultGroup,
          .requesting_process_group_id = 999999,
          .supervisor_id = *supervisor_id,
          .process_group_id = group_id,
      });
  check(missing_requester.status == KernelServiceStatus::NotFound,
        "service action returns NotFound for missing requesting group");
  check(missing_requester.rejection ==
            KernelServiceActionRejection::MissingRequestingGroup,
        "missing requester reports MissingRequestingGroup rejection");

  auto ack_action = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::AcknowledgeSupervisorFaultGroup,
          .supervisor_id = *supervisor_id,
          .process_group_id = group_id,
      });
  check(ack_action.status == KernelServiceStatus::Ok,
        "service action acknowledges the supervisor fault gate");
  check(ack_action.rejection == KernelServiceActionRejection::None,
        "successful supervisor acknowledgement clears rejection reason");
  check(ack_action.action_performed, "service action reports work performed");
  check(ack_action.process_group.has_value(), "service action returns updated group view");
  check(ack_action.supervisor.has_value(), "service action returns updated supervisor view");
  check(ack_action.fault_summary.has_value(), "service action returns updated fault summary");

  if (ack_action.process_group) {
    check(!ack_action.process_group->faulted, "updated group view is no longer faulted");
    check(!ack_action.process_group->blocked, "updated group view is no longer blocked");
    check(!ack_action.process_group->acknowledgement_pending,
          "updated group view clears acknowledgement pending");
    check(ack_action.process_group->recoveries == 1,
          "updated group view reports one recovery");
  }

  if (ack_action.supervisor) {
    check(ack_action.supervisor->pending_group_count == 0,
          "updated supervisor view clears pending groups");
    check(ack_action.supervisor->acknowledgements == 1,
          "updated supervisor view reports one acknowledgement");
  }

  if (ack_action.fault_summary) {
    check(ack_action.fault_summary->last_audit_event.has_value(),
          "updated fault summary reports a last audit event");
    if (ack_action.fault_summary->last_audit_event) {
      check(ack_action.fault_summary->last_audit_event->kind ==
                KernelAuditEventKind::ThreadRecovered,
            "service action drives recovery as the last audit event");
    }
  }

  auto post_action_group = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ProcessGroupStatus,
          .process_group_id = group_id,
      });
  check(post_action_group.status == KernelServiceStatus::Ok,
        "group request returns Ok after service acknowledgement");
}

static void test_kernel_service_supervisor_recovery_status() {
  std::printf("\n[AC-32] Axion kernel service contract exposes supervisor recovery status\n");

  namespace mmu = t81::ternaryos::mmu;

  auto ctx = make_valid_ctx(/*ethics=*/false);
  auto state = axion_kernel_bootstrap(ctx);
  check(state.has_value(), "kernel bootstrap succeeds for supervisor recovery status");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext worker_ctx;
  worker_ctx.label = "supervisor-recovery-worker";
  worker_ctx.registers[0] = 901;
  auto worker_tid = axion_kernel_spawn_thread(*state, worker_ctx);
  check(worker_tid.has_value(), "worker thread spawns for recovery status");
  if (!worker_tid) {
    return;
  }

  const auto* worker_runtime = state->find_thread_runtime(*worker_tid);
  check(worker_runtime != nullptr, "worker runtime exists for recovery status");
  if (!worker_runtime) {
    return;
  }
  const auto group_id = worker_runtime->process_group_id;
  auto supervisor_id = state->find_process_group_supervisor(group_id);
  check(supervisor_id.has_value(), "worker group resolves to supervisor");
  if (!supervisor_id) {
    return;
  }

  check(axion_kernel_step(*state), "recovery status dispatches worker thread");
  check(state->scheduler.current_tid() == *worker_tid, "worker runs before fault");
  auto access_result = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(181, 0), mmu::MmuAccessMode::Read);
  check(access_result.fault.has_value(), "worker records MMU fault for recovery status");
  check(axion_kernel_step(*state), "recovery status delivers fault to runtime");

  auto pre_ack = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorRecoveryStatus,
          .supervisor_id = *supervisor_id,
      });
  check(pre_ack.status == KernelServiceStatus::Ok,
        "supervisor recovery status succeeds before acknowledgement");
  check(pre_ack.rejection == KernelServiceRequestRejection::None,
        "successful supervisor recovery request clears request rejection");
  check(pre_ack.supervisor_recovery.has_value(),
        "supervisor recovery status view is returned");
  if (pre_ack.supervisor_recovery) {
    check(pre_ack.supervisor_recovery->pending_group_count == 1,
          "recovery status shows one pending group before acknowledgement");
    check(pre_ack.supervisor_recovery->managed_service_count == 0,
          "recovery status starts with zero managed services in a pure fault flow");
    check(pre_ack.supervisor_recovery->service_lifecycle_transitions == 0,
          "recovery status starts with zero service lifecycle transitions in a pure fault flow");
    check(pre_ack.supervisor_recovery->pending_group_ids.size() == 1,
          "recovery status exposes one pending group id");
    if (!pre_ack.supervisor_recovery->pending_group_ids.empty()) {
      check(pre_ack.supervisor_recovery->pending_group_ids.front() == group_id,
            "recovery status reports the faulted group id");
    }
    check(pre_ack.supervisor_recovery->acknowledgements == 0,
          "recovery status shows zero acknowledgements before recovery");
    check(pre_ack.supervisor_recovery->recovered_groups == 0,
          "recovery status shows zero recovered groups before recovery");
    check(!pre_ack.supervisor_recovery->last_acknowledged_group.has_value(),
          "recovery status has no last acknowledged group before recovery");
    check(!pre_ack.supervisor_recovery->last_recovered_group.has_value(),
          "recovery status has no last recovered group before recovery");
    check(!pre_ack.supervisor_recovery->last_service_transition_kind.has_value(),
          "recovery status has no last service transition before any service lifecycle work");
  }

  check(axion_kernel_ack_thread_fault(*state, *worker_tid),
        "thread-local acknowledgement drains inbox before supervisor recovery");

  auto ack_action = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::AcknowledgeSupervisorFaultGroup,
          .supervisor_id = *supervisor_id,
          .process_group_id = group_id,
      });
  check(ack_action.status == KernelServiceStatus::Ok,
        "supervisor recovery action succeeds after thread acknowledgement");
  check(ack_action.supervisor_recovery.has_value(),
        "supervisor recovery action returns updated recovery view");
  if (ack_action.supervisor_recovery) {
    check(ack_action.supervisor_recovery->pending_group_count == 0,
          "updated recovery view clears pending groups");
    check(ack_action.supervisor_recovery->managed_service_count == 0,
          "updated recovery view keeps managed service count at zero in a pure fault flow");
    check(ack_action.supervisor_recovery->acknowledgements == 1,
          "updated recovery view reports one acknowledgement");
    check(ack_action.supervisor_recovery->recovered_groups == 1,
          "updated recovery view reports one recovered group");
    check(ack_action.supervisor_recovery->last_acknowledged_group.has_value() &&
              *ack_action.supervisor_recovery->last_acknowledged_group == group_id,
          "updated recovery view tracks the last acknowledged group");
    check(ack_action.supervisor_recovery->last_recovered_group.has_value() &&
              *ack_action.supervisor_recovery->last_recovered_group == group_id,
          "updated recovery view tracks the last recovered group");
  }

  auto post_ack = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorRecoveryStatus,
          .supervisor_id = *supervisor_id,
      });
  check(post_ack.status == KernelServiceStatus::Ok,
        "supervisor recovery status succeeds after recovery");
  check(post_ack.rejection == KernelServiceRequestRejection::None,
        "post-recovery supervisor request clears request rejection");
  check(post_ack.supervisor_recovery.has_value(),
        "post-recovery supervisor recovery status view is returned");
  if (post_ack.supervisor_recovery) {
    check(post_ack.supervisor_recovery->pending_group_count == 0,
          "post-recovery status keeps pending group count at zero");
    check(post_ack.supervisor_recovery->managed_service_count == 0,
          "post-recovery status keeps managed service count at zero in a pure fault flow");
    check(post_ack.supervisor_recovery->recovered_groups == 1,
          "post-recovery status keeps recovered group count at one");
    check(post_ack.supervisor_recovery->last_acknowledged_group.has_value() &&
              *post_ack.supervisor_recovery->last_acknowledged_group == group_id,
          "post-recovery status preserves last acknowledged group");
    check(post_ack.supervisor_recovery->last_recovered_group.has_value() &&
              *post_ack.supervisor_recovery->last_recovered_group == group_id,
          "post-recovery status preserves last recovered group");
    check(!post_ack.supervisor_recovery->last_service_transition_sequence.has_value(),
          "post-recovery status has no service transition sequence in a pure fault flow");
  }
}

static void test_kernel_service_device_actions() {
  std::printf("\n[AC-33] Axion kernel service contract supports deterministic device actions\n");

  namespace mmu = t81::ternaryos::mmu;

  auto vbox_ctx = make_virtualbox_boot_context(VBoxBootSpec{});
  auto state = axion_kernel_bootstrap(vbox_ctx);
  check(state.has_value(), "kernel bootstrap succeeds for device actions");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext owner_ctx;
  owner_ctx.label = "device-owner";
  owner_ctx.registers[0] = 1001;
  auto owner_tid = axion_kernel_spawn_thread(*state, owner_ctx);
  check(owner_tid.has_value(), "owner thread spawns for device actions");
  if (!owner_tid) {
    return;
  }

  t81::ternaryos::sched::TiscContext contender_ctx;
  contender_ctx.label = "device-contender";
  contender_ctx.registers[0] = 1002;
  auto contender_tid = axion_kernel_spawn_thread(*state, contender_ctx);
  check(contender_tid.has_value(), "contender thread spawns for device actions");
  if (!contender_tid) {
    return;
  }

  const auto* owner_runtime = state->find_thread_runtime(*owner_tid);
  const auto* contender_runtime = state->find_thread_runtime(*contender_tid);
  check(owner_runtime != nullptr, "owner runtime exists");
  check(contender_runtime != nullptr, "contender runtime exists");
  if (!owner_runtime || !contender_runtime) {
    return;
  }
  const auto owner_group = owner_runtime->process_group_id;
  const auto contender_group = contender_runtime->process_group_id;

  auto preclaim = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ClaimDevice,
          .requesting_process_group_id = owner_group,
          .device_name = std::string{"ahci"},
      });
  check(preclaim.status == KernelServiceStatus::Ok,
        "healthy owner group can claim a device");
  check(preclaim.rejection == KernelServiceActionRejection::None,
        "successful claim clears rejection reason");
  check(preclaim.action_performed, "claim action reports work performed");
  check(preclaim.device_summary.has_value(), "claim action returns device summary");
  if (preclaim.device_summary) {
    check(preclaim.device_summary->claimed_device_count == 1,
          "device summary reports one claimed device after claim");
  }

  auto conflicting_claim = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ClaimDevice,
          .requesting_process_group_id = contender_group,
          .device_name = std::string{"ahci"},
      });
  check(conflicting_claim.status == KernelServiceStatus::InvalidRequest,
        "another healthy group cannot steal a claimed device");
  check(conflicting_claim.rejection == KernelServiceActionRejection::DeviceConflict,
        "conflicting claim reports DeviceConflict rejection");

  check(axion_kernel_step(*state), "device action path dispatches owner thread");
  check(axion_kernel_step(*state), "device action path dispatches contender thread");
  check(state->scheduler.current_tid() == *contender_tid,
        "contender thread becomes current before fault");
  auto fault_result = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(211, 0), mmu::MmuAccessMode::Read);
  check(fault_result.fault.has_value(), "contender thread records MMU fault");
  check(axion_kernel_step(*state), "device action path delivers contender fault");

  auto faulted_claim = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ClaimDevice,
          .requesting_process_group_id = contender_group,
          .device_name = std::string{"e1000"},
      });
  check(faulted_claim.status == KernelServiceStatus::FaultedGroup,
        "faulted group cannot claim a device through the service boundary");
  check(faulted_claim.rejection ==
            KernelServiceActionRejection::FaultedRequestingGroup,
        "faulted claim reports FaultedRequestingGroup rejection");

  auto bad_release = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ReleaseDevice,
          .requesting_process_group_id = contender_group,
          .device_name = std::string{"ahci"},
      });
  check(bad_release.status == KernelServiceStatus::FaultedGroup,
        "faulted group cannot release a device through the service boundary");
  check(bad_release.rejection ==
            KernelServiceActionRejection::FaultedRequestingGroup,
        "faulted release reports FaultedRequestingGroup rejection");

  auto release = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ReleaseDevice,
          .requesting_process_group_id = owner_group,
          .device_name = std::string{"ahci"},
      });
  check(release.status == KernelServiceStatus::Ok,
        "owning healthy group can release a device");
  check(release.rejection == KernelServiceActionRejection::None,
        "successful release clears rejection reason");
  check(release.action_performed, "release action reports work performed");
  check(release.device_summary.has_value(), "release action returns device summary");
  if (release.device_summary) {
    check(release.device_summary->claimed_device_count == 0,
          "device summary reports zero claimed devices after release");
  }

  auto hosted_state = axion_kernel_bootstrap(make_valid_ctx(/*ethics=*/false));
  check(hosted_state.has_value(),
        "generic hosted runtime bootstraps without device arbitration");
  if (!hosted_state.has_value()) {
    return;
  }
  t81::ternaryos::sched::TiscContext hosted_ctx;
  hosted_ctx.label = "hosted-requester";
  hosted_ctx.registers[0] = 701;
  const auto hosted_thread = axion_kernel_spawn_thread(*hosted_state, hosted_ctx);
  check(hosted_thread.has_value(),
        "generic hosted runtime can spawn a requester group for arbitration checks");
  if (!hosted_thread.has_value()) {
    return;
  }
  const auto* hosted_runtime = hosted_state->find_thread_runtime(*hosted_thread);
  check(hosted_runtime != nullptr,
        "generic hosted runtime exposes requester thread runtime state");
  if (hosted_runtime == nullptr) {
    return;
  }
  auto missing_arbitration = axion_kernel_service_action(
      *hosted_state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ClaimDevice,
          .requesting_process_group_id = hosted_runtime->process_group_id,
          .device_name = std::string{"ahci"},
      });
  check(missing_arbitration.status == KernelServiceStatus::NoDeviceArbitration,
        "hosted generic runtime reports missing device arbitration");
  check(missing_arbitration.rejection ==
            KernelServiceActionRejection::MissingDeviceArbitration,
        "missing device arbitration reports explicit rejection");
}

static void test_kernel_service_diagnostic_details() {
  std::printf("\n[AC-34] Axion kernel service contract exposes stable audit and device detail views\n");

  namespace mmu = t81::ternaryos::mmu;

  auto vbox_ctx = make_virtualbox_boot_context(VBoxBootSpec{});
  auto state = axion_kernel_bootstrap(vbox_ctx);
  check(state.has_value(), "kernel bootstrap succeeds for service diagnostics");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext owner_ctx;
  owner_ctx.label = "diag-owner";
  owner_ctx.registers[0] = 1101;
  auto owner_tid = axion_kernel_spawn_thread(*state, owner_ctx);
  check(owner_tid.has_value(), "diagnostic owner thread spawns");
  if (!owner_tid) {
    return;
  }

  t81::ternaryos::sched::TiscContext faulted_ctx;
  faulted_ctx.label = "diag-faulted";
  faulted_ctx.registers[0] = 1102;
  auto faulted_tid = axion_kernel_spawn_thread(*state, faulted_ctx);
  check(faulted_tid.has_value(), "diagnostic faulted thread spawns");
  if (!faulted_tid) {
    return;
  }

  const auto* owner_runtime = state->find_thread_runtime(*owner_tid);
  const auto* faulted_runtime = state->find_thread_runtime(*faulted_tid);
  check(owner_runtime != nullptr, "diagnostic owner runtime exists");
  check(faulted_runtime != nullptr, "diagnostic faulted runtime exists");
  if (!owner_runtime || !faulted_runtime) {
    return;
  }

  auto claim = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ClaimDevice,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .device_name = std::string{"ahci"},
      });
  check(claim.status == KernelServiceStatus::Ok,
        "diagnostic owner can claim ahci");

  check(axion_kernel_step(*state), "diagnostic loop dispatches owner");
  check(axion_kernel_step(*state), "diagnostic loop dispatches faulted thread");
  check(state->scheduler.current_tid() == *faulted_tid,
        "diagnostic faulted thread becomes current");
  auto fault = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(251, 0), mmu::MmuAccessMode::Read);
  check(fault.fault.has_value(), "diagnostic fault is recorded");
  check(axion_kernel_step(*state), "diagnostic loop delivers fault");

  auto audit = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::AuditSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(audit.status == KernelServiceStatus::Ok,
        "healthy group can request audit summary");
  check(audit.rejection == KernelServiceRequestRejection::None,
        "successful audit detail clears request rejection");
  check(audit.audit_summary.has_value(), "audit summary detail is returned");
  if (audit.audit_summary) {
    check(audit.audit_summary->fault_deliveries == 1,
          "audit summary reports one fault delivery");
    check(audit.audit_summary->thread_quarantines == 1,
          "audit summary reports one thread quarantine");
    check(audit.audit_summary->process_group_fault_entries == 1,
          "audit summary reports one process-group fault entry");
    check(audit.audit_summary->supervisor_notifications == 1,
          "audit summary reports one supervisor notification");
    check(audit.audit_summary->recent_events.size() >= 4,
          "audit summary exposes recent audit events");
    if (audit.audit_summary->recent_events.size() >= 4) {
      check(audit.audit_summary->recent_events[0].kind ==
                KernelAuditEventKind::FaultDelivered,
            "audit summary starts with fault delivery");
      check(audit.audit_summary->recent_events[1].kind ==
                KernelAuditEventKind::ProcessGroupFaultEntered,
            "audit summary reports process-group fault entry");
      check(audit.audit_summary->recent_events[2].kind ==
                KernelAuditEventKind::SupervisorFaultNotified,
            "audit summary reports supervisor notification");
      check(audit.audit_summary->recent_events[3].kind ==
                KernelAuditEventKind::ThreadQuarantined,
            "audit summary reports thread quarantine");
    }
  }

  auto devices = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::DeviceSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(devices.status == KernelServiceStatus::Ok,
        "healthy group can request detailed device summary");
  check(devices.rejection == KernelServiceRequestRejection::None,
        "successful device detail clears request rejection");
  check(devices.device_summary.has_value(), "device summary detail is returned");
  if (devices.device_summary) {
    check(devices.device_summary->devices.size() == 5,
          "device summary detail exposes five devices");
    bool saw_claimed_ahci = false;
    bool saw_unclaimed_e1000 = false;
    for (const auto& device : devices.device_summary->devices) {
      if (device.name == "ahci") {
        saw_claimed_ahci = true;
        check(device.claimed, "device summary marks ahci claimed");
        check(device.owner_tid == owner_tid, "device summary preserves ahci owner tid");
        check(device.irq == 19, "device summary preserves ahci irq");
      } else if (device.name == "e1000") {
        saw_unclaimed_e1000 = true;
        check(!device.claimed, "device summary keeps e1000 unclaimed");
        check(!device.owner_tid.has_value(),
              "device summary keeps unclaimed e1000 owner empty");
      }
    }
    check(saw_claimed_ahci, "device summary includes claimed ahci");
    check(saw_unclaimed_e1000, "device summary includes unclaimed e1000");
  }
}

static void test_kernel_service_runtime_layer() {
  std::printf("\n[AC-35] Axion kernel service runtime layer\n");

  namespace mmu = t81::ternaryos::mmu;

  auto state = axion_kernel_bootstrap(make_virtualbox_boot_context(VBoxBootSpec{}));
  check(state.has_value(), "kernel bootstrap succeeds for service runtime layer");
  if (!state) {
    return;
  }

  t81::ternaryos::sched::TiscContext owner_ctx;
  owner_ctx.label = "service-owner";
  owner_ctx.registers[0] = 1201;
  auto owner_tid = axion_kernel_spawn_thread(*state, owner_ctx);
  check(owner_tid.has_value(), "service owner thread spawns");
  if (!owner_tid) {
    return;
  }

  const auto* owner_runtime = state->find_thread_runtime(*owner_tid);
  check(owner_runtime != nullptr, "service owner runtime exists");
  if (!owner_runtime) {
    return;
  }

  const auto supervisor_id =
      state->find_process_group_supervisor(owner_runtime->process_group_id);
  check(supervisor_id.has_value(), "service owner group resolves to a supervisor");
  if (!supervisor_id) {
    return;
  }
  const auto owner_address_space_id =
      state->find_process_group_address_space(owner_runtime->process_group_id);
  check(owner_address_space_id.has_value(),
        "service owner group resolves to an address space");
  if (!owner_address_space_id) {
    return;
  }
  check(mmu::mmu_map(state->page_table,
                     state->allocator,
                     mmu::tva_from_vpn_offset(104, 0),
                     *owner_address_space_id),
        "service owner address space accepts an owned mapping");

  auto register_service = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::RegisterService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_name = std::string{"svc.alpha"},
      });
  check(register_service.status == KernelServiceStatus::Ok,
        "healthy group can register a service");
  check(register_service.rejection == KernelServiceActionRejection::None,
        "successful service registration clears rejection");
  check(register_service.action_performed,
        "service registration reports work performed");
  check(register_service.service.has_value(),
        "service registration returns service status");
  check(register_service.supervisor_services.has_value(),
        "service registration returns supervisor inventory");

  std::optional<ServiceId> service_id{};
  if (register_service.service) {
    service_id = register_service.service->id;
    check(register_service.service->name == "svc.alpha",
          "registered service preserves name");
    check(register_service.service->supervisor_id == *supervisor_id,
          "registered service preserves supervisor");
    check(register_service.service->process_group_id == owner_runtime->process_group_id,
          "registered service preserves backing process group");
    check(register_service.service->address_space_id == owner_address_space_id,
          "registered service preserves backing address space");
    check(register_service.service->owned_page_count == 1,
          "registered service reports one owned mapped page");
    check(register_service.service->registered,
          "registered service reports registered state");
    check(!register_service.service->blocked,
          "registered service starts unblocked");
  }
  if (register_service.supervisor_services) {
    check(register_service.supervisor_services->service_count == 1,
          "supervisor inventory reports one service after registration");
    check(register_service.supervisor_services->service_ids.size() == 1,
          "supervisor inventory exposes one service id");
    check(register_service.supervisor_services->services.size() == 1,
          "supervisor inventory exposes one service entry");
    check(register_service.supervisor_services->services.front().address_space_id ==
              owner_address_space_id,
          "supervisor inventory entry preserves the backing address space");
    check(register_service.supervisor_services->services.front().owned_page_count == 1,
          "supervisor inventory entry reports one owned mapped page");
    check(register_service.supervisor_services->services.front().state_transitions == 0,
          "supervisor inventory entry starts with zero state mutations after registration");
    check(register_service.supervisor_services->services.front().last_transition_kind ==
              KernelAuditEventKind::ServiceRegistered,
          "supervisor inventory entry tracks registration as the latest lifecycle event");
    check(register_service.supervisor_services->services.front()
              .last_transition_sequence.has_value(),
          "supervisor inventory entry exposes the registration audit sequence");
    check(register_service.supervisor_services->blocked_service_count == 0,
          "supervisor inventory starts with zero blocked services");
    check(register_service.supervisor_services->service_lifecycle_transitions == 1,
          "supervisor inventory tracks one lifecycle transition after registration");
    check(register_service.supervisor_services->last_service_transition_id == service_id,
          "supervisor inventory tracks the last transitioned service after registration");
    check(register_service.supervisor_services->last_service_transition_kind ==
              KernelAuditEventKind::ServiceRegistered,
          "supervisor inventory tracks registration as the last lifecycle event");
    check(register_service.supervisor_services->total_service_requests == 0,
          "supervisor inventory starts with zero service requests");
  }
  auto supervisor_status_after_register = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .supervisor_id = *supervisor_id,
      });
  check(supervisor_status_after_register.status == KernelServiceStatus::Ok,
        "healthy group can request supervisor status after service registration");
  check(supervisor_status_after_register.rejection == KernelServiceRequestRejection::None,
        "supervisor status after registration clears rejection");
  check(supervisor_status_after_register.supervisor.has_value(),
        "supervisor status after registration returns a supervisor view");
  if (supervisor_status_after_register.supervisor) {
    check(supervisor_status_after_register.supervisor->managed_address_space_count == 1,
          "supervisor status reports one managed address space after registration");
    check(supervisor_status_after_register.supervisor->managed_mapped_page_count == 1,
          "supervisor status reports one managed mapped page after registration");
    check(supervisor_status_after_register.supervisor->managed_service_count == 1,
          "supervisor status reports one managed service after registration");
    check(supervisor_status_after_register.supervisor->blocked_service_count == 0,
          "supervisor status starts with zero blocked services");
    check(supervisor_status_after_register.supervisor->service_lifecycle_transitions == 1,
          "supervisor status tracks one lifecycle transition after registration");
    check(supervisor_status_after_register.supervisor->last_service_transition_id == service_id,
          "supervisor status tracks the last transitioned service after registration");
    check(supervisor_status_after_register.supervisor->last_service_transition_kind ==
              KernelAuditEventKind::ServiceRegistered,
          "supervisor status tracks registration as the latest lifecycle event");
  }
  if (!service_id) {
    return;
  }

  auto duplicate_service = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::RegisterService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_name = std::string{"svc.beta"},
      });
  check(duplicate_service.status == KernelServiceStatus::InvalidRequest,
        "duplicate service registration is rejected");
  check(duplicate_service.rejection == KernelServiceActionRejection::DuplicateService,
        "duplicate service registration reports duplicate service rejection");

  auto service_view = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ServiceStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(service_view.status == KernelServiceStatus::Ok,
        "healthy group can request service status");
  check(service_view.rejection == KernelServiceRequestRejection::None,
        "healthy service request clears rejection");
  check(service_view.service.has_value(), "service status request returns service view");
  if (service_view.service) {
    check(service_view.service->address_space_id == owner_address_space_id,
          "service view preserves backing address-space ownership");
    check(service_view.service->owned_page_count == 1,
          "service view reports one owned mapped page");
    check(!service_view.service->pager_needed,
          "healthy service view reports no pager-needed state");
    check(!service_view.service->pager_handoff_pending,
          "healthy service view reports no pending pager handoff");
    check(!service_view.service->pager_worker_owned,
          "healthy service view reports no worker-owned pager state");
    check(service_view.service->pending_pager_fault_count == 0,
          "healthy service view reports zero pending pager faults");
    check(service_view.service->pager_handoffs == 0,
          "healthy service view reports zero pager handoffs");
    check(service_view.service->pager_resolutions == 0,
          "healthy service view reports zero pager resolutions");
    check(service_view.service->pager_faults_coalesced == 0,
          "healthy service view reports zero coalesced pager faults");
    check(service_view.service->primary_tid == *owner_tid,
          "service view exposes primary group thread");
    check(!service_view.service->faulted_group,
          "healthy service view reports non-faulted group");
    check(!service_view.service->suspended,
          "healthy service view reports non-suspended lifecycle state");
    check(service_view.service->quarantined_thread_count == 0,
          "healthy service view reports zero quarantined threads");
    check(service_view.service->pending_fault_count == 0,
          "healthy service view reports zero pending group faults");
    check(service_view.service->requests >= 1,
          "service view tracks request count");
    check(service_view.service->rejected_requests == 0,
          "healthy service request does not increment rejected count");
    check(service_view.service->last_transition_kind ==
              KernelAuditEventKind::ServiceRegistered,
          "freshly registered service view tracks registration as the latest lifecycle event");
    check(service_view.service->last_transition_sequence.has_value(),
          "freshly registered service view exposes the registration audit sequence");
  }

  auto suspend_service = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::SuspendService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(suspend_service.status == KernelServiceStatus::Ok,
        "healthy owner group can suspend its service");
  check(suspend_service.rejection == KernelServiceActionRejection::None,
        "successful suspend clears rejection");
  check(suspend_service.action_performed,
        "service suspend reports work performed");
  check(suspend_service.service.has_value(),
        "service suspend returns service status");
  check(suspend_service.supervisor_services.has_value(),
        "service suspend returns supervisor inventory");
  if (suspend_service.service) {
    check(suspend_service.service->suspended,
          "suspended service reports suspended lifecycle state");
    check(suspend_service.service->state_transitions >= 1,
          "service suspend increments lifecycle transitions");
  }
  if (suspend_service.supervisor_services) {
    check(suspend_service.supervisor_services->suspended_service_count == 1,
          "supervisor inventory reports one suspended service");
    check(suspend_service.supervisor_services->services.front().suspended,
          "supervisor inventory entry reports suspended service");
    check(suspend_service.supervisor_services->services.front().state_transitions == 1,
          "supervisor inventory entry increments state mutations after suspend");
    check(suspend_service.supervisor_services->services.front().last_transition_kind ==
              KernelAuditEventKind::ServiceSuspended,
          "supervisor inventory entry tracks suspend as the latest lifecycle event");
    check(suspend_service.supervisor_services->service_lifecycle_transitions == 2,
          "supervisor inventory increments lifecycle transition count after suspend");
    check(suspend_service.supervisor_services->last_service_transition_kind ==
              KernelAuditEventKind::ServiceSuspended,
          "supervisor inventory tracks suspend as the last lifecycle event");
  }

  auto duplicate_suspend = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::SuspendService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(duplicate_suspend.status == KernelServiceStatus::InvalidRequest,
        "duplicate suspend is rejected");
  check(duplicate_suspend.rejection ==
            KernelServiceActionRejection::ServiceAlreadySuspended,
        "duplicate suspend reports already-suspended rejection");

  auto resume_service = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ResumeService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(resume_service.status == KernelServiceStatus::Ok,
        "healthy owner group can resume its service");
  check(resume_service.rejection == KernelServiceActionRejection::None,
        "successful resume clears rejection");
  check(resume_service.action_performed,
        "service resume reports work performed");
  if (resume_service.service) {
    check(!resume_service.service->suspended,
          "resumed service clears suspended lifecycle state");
  }
  if (resume_service.supervisor_services) {
    check(resume_service.supervisor_services->suspended_service_count == 0,
          "supervisor inventory clears suspended service count after resume");
    check(resume_service.supervisor_services->service_lifecycle_transitions == 3,
          "supervisor inventory increments lifecycle transition count after resume");
    check(resume_service.supervisor_services->last_service_transition_kind ==
              KernelAuditEventKind::ServiceResumed,
          "supervisor inventory tracks resume as the last lifecycle event");
  }

  auto duplicate_resume = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ResumeService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(duplicate_resume.status == KernelServiceStatus::InvalidRequest,
        "duplicate resume is rejected");
  check(duplicate_resume.rejection ==
            KernelServiceActionRejection::ServiceNotSuspended,
        "duplicate resume reports not-suspended rejection");

  t81::ternaryos::sched::TiscContext supervisor_peer_ctx;
  supervisor_peer_ctx.label = "service-supervisor-peer";
  supervisor_peer_ctx.registers[0] = 1203;
  auto supervisor_peer_tid = axion_kernel_spawn_thread_under_supervisor(
      *state, supervisor_peer_ctx, *supervisor_id);
  check(supervisor_peer_tid.has_value(),
        "supervisor peer thread spawns under existing supervisor");
  if (!supervisor_peer_tid) {
    return;
  }

  const auto* supervisor_peer_runtime = state->find_thread_runtime(*supervisor_peer_tid);
  check(supervisor_peer_runtime != nullptr, "supervisor peer runtime exists");
  if (!supervisor_peer_runtime) {
    return;
  }
  check(supervisor_peer_runtime->process_group_id != owner_runtime->process_group_id,
        "supervisor peer owns a distinct process group");
  auto peer_supervisor_id =
      state->find_process_group_supervisor(supervisor_peer_runtime->process_group_id);
  check(peer_supervisor_id.has_value(), "supervisor peer group resolves to a supervisor");
  if (!peer_supervisor_id) {
    return;
  }
  check(*peer_supervisor_id == *supervisor_id,
        "supervisor peer group shares the same supervisor");

  auto supervisor_suspend = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::SuspendService,
          .requesting_process_group_id = supervisor_peer_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(supervisor_suspend.status == KernelServiceStatus::Ok,
        "same-supervisor peer group can suspend a managed service");
  check(supervisor_suspend.rejection == KernelServiceActionRejection::None,
        "same-supervisor suspend clears rejection");
  if (supervisor_suspend.service) {
    check(supervisor_suspend.service->suspended,
          "same-supervisor suspend updates managed service state");
  }

  t81::ternaryos::sched::TiscContext foreign_ctx;
  foreign_ctx.label = "service-foreign-peer";
  foreign_ctx.registers[0] = 1204;
  auto foreign_tid = axion_kernel_spawn_thread(*state, foreign_ctx);
  check(foreign_tid.has_value(), "foreign supervisor thread spawns");
  if (!foreign_tid) {
    return;
  }
  const auto* foreign_runtime = state->find_thread_runtime(*foreign_tid);
  check(foreign_runtime != nullptr, "foreign runtime exists");
  if (!foreign_runtime) {
    return;
  }

  auto foreign_resume = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ResumeService,
          .requesting_process_group_id = foreign_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(foreign_resume.status == KernelServiceStatus::InvalidRequest,
        "foreign supervisor group cannot resume another supervisor's service");
  check(foreign_resume.rejection ==
            KernelServiceActionRejection::ServiceSupervisorMismatch,
        "foreign supervisor resume reports supervisor mismatch");

  auto supervisor_resume = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::ResumeService,
          .requesting_process_group_id = supervisor_peer_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(supervisor_resume.status == KernelServiceStatus::Ok,
        "same-supervisor peer group can resume a managed service");
  check(supervisor_resume.rejection == KernelServiceActionRejection::None,
        "same-supervisor resume clears rejection");
  if (supervisor_resume.service) {
    check(!supervisor_resume.service->suspended,
          "same-supervisor resume clears managed service suspension");
  }

  auto mark_unhealthy = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::MarkServiceUnhealthy,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(mark_unhealthy.status == KernelServiceStatus::Ok,
        "healthy owner group can mark its service unhealthy");
  check(mark_unhealthy.rejection == KernelServiceActionRejection::None,
        "mark-unhealthy clears rejection");
  if (mark_unhealthy.service) {
    check(mark_unhealthy.service->unhealthy,
          "service view exposes unhealthy lifecycle state");
    check(!mark_unhealthy.service->faulted_group,
          "service unhealthy transition does not imply a faulted group");
  }
  if (mark_unhealthy.supervisor_services) {
    check(mark_unhealthy.supervisor_services->unhealthy_service_count == 1,
          "supervisor inventory reports one unhealthy service");
    check(mark_unhealthy.supervisor_services->services.front().unhealthy,
          "supervisor inventory entry reports unhealthy service");
    check(mark_unhealthy.supervisor_services->services.front().last_transition_kind ==
              KernelAuditEventKind::ServiceMarkedUnhealthy,
          "supervisor inventory entry tracks unhealthy as the latest lifecycle event");
    check(mark_unhealthy.supervisor_services->service_lifecycle_transitions == 6,
          "supervisor inventory aggregates lifecycle transitions across same-supervisor control");
    check(mark_unhealthy.supervisor_services->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedUnhealthy,
          "supervisor inventory tracks unhealthy transition as the latest event");
  }

  auto unhealthy_request = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ServiceStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(unhealthy_request.status == KernelServiceStatus::ServiceUnavailable,
        "unhealthy service request is rejected as unavailable");
  check(unhealthy_request.rejection == KernelServiceRequestRejection::UnhealthyService,
        "unhealthy service request reports explicit unhealthy rejection");

  auto duplicate_unhealthy = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::MarkServiceUnhealthy,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(duplicate_unhealthy.status == KernelServiceStatus::InvalidRequest,
        "duplicate unhealthy transition is rejected");
  check(duplicate_unhealthy.rejection ==
            KernelServiceActionRejection::ServiceAlreadyUnhealthy,
        "duplicate unhealthy transition reports already-unhealthy rejection");

  auto foreign_health = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::MarkServiceHealthy,
          .requesting_process_group_id = foreign_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(foreign_health.status == KernelServiceStatus::InvalidRequest,
        "foreign supervisor group cannot heal another supervisor's service");
  check(foreign_health.rejection ==
            KernelServiceActionRejection::ServiceSupervisorMismatch,
        "foreign health transition reports supervisor mismatch");

  auto peer_health = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::MarkServiceHealthy,
          .requesting_process_group_id = supervisor_peer_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(peer_health.status == KernelServiceStatus::Ok,
        "same-supervisor peer group can heal a managed service");
  check(peer_health.rejection == KernelServiceActionRejection::None,
        "same-supervisor heal clears rejection");
  if (peer_health.service) {
    check(!peer_health.service->unhealthy,
          "same-supervisor heal clears unhealthy lifecycle state");
    check(peer_health.service->last_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "service view tracks heal as the latest lifecycle event");
    check(peer_health.service->last_transition_sequence.has_value(),
          "service view exposes the latest lifecycle audit sequence after heal");
  }
  if (peer_health.supervisor_services) {
    check(peer_health.supervisor_services->unhealthy_service_count == 0,
          "supervisor inventory clears unhealthy count after heal");
    check(peer_health.supervisor_services->services.front().last_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "supervisor inventory entry tracks heal as the latest lifecycle event");
    check(peer_health.supervisor_services->services.front().last_transition_sequence.has_value(),
          "supervisor inventory entry exposes the latest lifecycle audit sequence");
    check(peer_health.supervisor_services->service_lifecycle_transitions == 7,
          "supervisor inventory increments lifecycle transitions after heal");
    check(peer_health.supervisor_services->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "supervisor inventory tracks heal as the latest lifecycle event");
    check(peer_health.supervisor_services->last_service_transition_sequence.has_value(),
          "supervisor inventory exposes the latest lifecycle audit sequence");
  }
  auto supervisor_status_after_heal = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .supervisor_id = *supervisor_id,
      });
  check(supervisor_status_after_heal.status == KernelServiceStatus::Ok,
        "healthy group can request supervisor status after service heal");
  check(supervisor_status_after_heal.rejection == KernelServiceRequestRejection::None,
        "supervisor status after heal clears rejection");
  check(supervisor_status_after_heal.supervisor.has_value(),
        "supervisor status after heal returns a supervisor view");
  if (supervisor_status_after_heal.supervisor) {
    check(supervisor_status_after_heal.supervisor->managed_service_count == 1,
          "supervisor status retains one managed service after heal");
    check(supervisor_status_after_heal.supervisor->suspended_service_count == 0,
          "supervisor status reports zero suspended services after heal");
    check(supervisor_status_after_heal.supervisor->unhealthy_service_count == 0,
          "supervisor status reports zero unhealthy services after heal");
    check(supervisor_status_after_heal.supervisor->service_lifecycle_transitions == 7,
          "supervisor status aggregates lifecycle transitions across managed service control");
    check(supervisor_status_after_heal.supervisor->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "supervisor status tracks heal as the latest lifecycle event");
    check(supervisor_status_after_heal.supervisor->last_service_transition_sequence.has_value(),
          "supervisor status exposes the latest lifecycle audit sequence");
  }

  auto recovery_status_after_heal = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorRecoveryStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .supervisor_id = *supervisor_id,
      });
  check(recovery_status_after_heal.status == KernelServiceStatus::Ok,
        "healthy group can request supervisor recovery status after service heal");
  check(recovery_status_after_heal.rejection == KernelServiceRequestRejection::None,
        "supervisor recovery status after heal clears rejection");
  check(recovery_status_after_heal.supervisor_recovery.has_value(),
        "supervisor recovery status after heal returns a recovery view");
  if (recovery_status_after_heal.supervisor_recovery) {
    check(recovery_status_after_heal.supervisor_recovery->managed_service_count == 1,
          "recovery status retains one managed service after heal");
    check(recovery_status_after_heal.supervisor_recovery->blocked_service_count == 0,
          "recovery status reports zero blocked services after heal");
    check(recovery_status_after_heal.supervisor_recovery->unhealthy_service_count == 0,
          "recovery status reports zero unhealthy services after heal");
    check(recovery_status_after_heal.supervisor_recovery->service_lifecycle_transitions == 7,
          "recovery status aggregates service lifecycle transitions after heal");
    check(recovery_status_after_heal.supervisor_recovery->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "recovery status tracks heal as the latest service lifecycle event");
    check(recovery_status_after_heal.supervisor_recovery->last_service_transition_sequence.has_value(),
          "recovery status exposes the latest service lifecycle audit sequence after heal");
  }

  auto fault_summary_after_heal = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::FaultSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(fault_summary_after_heal.status == KernelServiceStatus::Ok,
        "healthy group can request fault summary after service heal");
  check(fault_summary_after_heal.rejection == KernelServiceRequestRejection::None,
        "fault summary after service heal clears rejection");
  check(fault_summary_after_heal.fault_summary.has_value(),
        "fault summary after service heal returns a fault view");
  if (fault_summary_after_heal.fault_summary) {
    check(fault_summary_after_heal.fault_summary->service_lifecycle_transitions == 7,
          "fault summary aggregates service lifecycle transitions after heal");
    check(fault_summary_after_heal.fault_summary->last_service_transition_id == service_id,
          "fault summary tracks the last transitioned service after heal");
    check(fault_summary_after_heal.fault_summary->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "fault summary tracks heal as the latest service lifecycle event");
    check(fault_summary_after_heal.fault_summary->last_service_transition_sequence.has_value(),
          "fault summary exposes the latest service lifecycle audit sequence after heal");
  }

  auto runtime_status_after_heal = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::RuntimeStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(runtime_status_after_heal.status == KernelServiceStatus::Ok,
        "healthy group can request runtime status after service heal");
  check(runtime_status_after_heal.rejection == KernelServiceRequestRejection::None,
        "runtime status after heal clears rejection");
  check(runtime_status_after_heal.runtime.has_value(),
        "runtime status after heal returns a runtime view");
  if (runtime_status_after_heal.runtime) {
    check(runtime_status_after_heal.runtime->managed_service_count == 1,
          "runtime status reports one managed service after heal");
    check(runtime_status_after_heal.runtime->blocked_service_count == 0,
          "runtime status reports zero blocked services after heal");
    check(runtime_status_after_heal.runtime->suspended_service_count == 0,
          "runtime status reports zero suspended services after heal");
    check(runtime_status_after_heal.runtime->unhealthy_service_count == 0,
          "runtime status reports zero unhealthy services after heal");
    check(runtime_status_after_heal.runtime->service_lifecycle_transitions == 7,
          "runtime status aggregates service lifecycle transitions after heal");
    check(runtime_status_after_heal.runtime->last_service_transition_id == service_id,
          "runtime status tracks the last transitioned service after heal");
    check(runtime_status_after_heal.runtime->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "runtime status tracks heal as the latest service lifecycle event");
    check(runtime_status_after_heal.runtime->last_service_transition_sequence.has_value(),
          "runtime status exposes the latest service lifecycle audit sequence after heal");
  }

  auto device_summary_after_heal = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::DeviceSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(device_summary_after_heal.status == KernelServiceStatus::Ok,
        "healthy group can request device summary after service heal");
  check(device_summary_after_heal.rejection == KernelServiceRequestRejection::None,
        "device summary after heal clears rejection");
  check(device_summary_after_heal.device_summary.has_value(),
        "device summary after heal returns a device view");
  if (device_summary_after_heal.device_summary) {
    check(device_summary_after_heal.device_summary->service_lifecycle_transitions == 7,
          "device summary aggregates service lifecycle transitions after heal");
    check(device_summary_after_heal.device_summary->last_service_transition_id == service_id,
          "device summary tracks the last transitioned service after heal");
    check(device_summary_after_heal.device_summary->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "device summary tracks heal as the latest service lifecycle event");
    check(device_summary_after_heal.device_summary->last_service_transition_sequence.has_value(),
          "device summary exposes the latest service lifecycle audit sequence after heal");
  }

  auto audit_summary = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::AuditSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(audit_summary.status == KernelServiceStatus::Ok,
        "healthy group can request audit summary after service lifecycle actions");
  check(audit_summary.rejection == KernelServiceRequestRejection::None,
        "audit summary request clears rejection");
  check(audit_summary.audit_summary.has_value(),
        "audit summary request returns audit detail");
  if (audit_summary.audit_summary) {
    std::vector<KernelAuditEventKind> service_lifecycle_events;
    for (const auto& entry : audit_summary.audit_summary->recent_events) {
      switch (entry.kind) {
        case KernelAuditEventKind::ServiceRegistered:
        case KernelAuditEventKind::ServiceUnregistered:
        case KernelAuditEventKind::ServiceSuspended:
        case KernelAuditEventKind::ServiceResumed:
        case KernelAuditEventKind::ServiceMarkedUnhealthy:
        case KernelAuditEventKind::ServiceMarkedHealthy:
          service_lifecycle_events.push_back(entry.kind);
          break;
        default:
          break;
      }
    }
    check(service_lifecycle_events.size() == 7,
          "audit summary captures seven successful service lifecycle transitions before unregister");
    check(audit_summary.audit_summary->service_lifecycle_transitions == 7,
          "audit summary aggregates service lifecycle transitions before unregister");
    check(audit_summary.audit_summary->last_service_transition_id == service_id,
          "audit summary tracks the last transitioned service before unregister");
    check(audit_summary.audit_summary->last_service_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "audit summary tracks heal as the latest service lifecycle event before unregister");
    check(audit_summary.audit_summary->last_service_transition_sequence.has_value(),
          "audit summary exposes the latest service lifecycle audit sequence before unregister");
    if (service_lifecycle_events.size() == 7) {
      check(service_lifecycle_events[0] == KernelAuditEventKind::ServiceRegistered,
            "audit summary records service registration first");
      check(service_lifecycle_events[1] == KernelAuditEventKind::ServiceSuspended,
            "audit summary records owner-initiated service suspension");
      check(service_lifecycle_events[2] == KernelAuditEventKind::ServiceResumed,
            "audit summary records owner-initiated service resume");
      check(service_lifecycle_events[3] == KernelAuditEventKind::ServiceSuspended,
            "audit summary records same-supervisor suspension");
      check(service_lifecycle_events[4] == KernelAuditEventKind::ServiceResumed,
            "audit summary records same-supervisor resume");
      check(service_lifecycle_events[5] ==
                KernelAuditEventKind::ServiceMarkedUnhealthy,
            "audit summary records unhealthy transition");
      check(service_lifecycle_events[6] == KernelAuditEventKind::ServiceMarkedHealthy,
            "audit summary records healthy transition");
    }
  }

  auto duplicate_healthy = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::MarkServiceHealthy,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(duplicate_healthy.status == KernelServiceStatus::InvalidRequest,
        "duplicate healthy transition is rejected");
  check(duplicate_healthy.rejection ==
            KernelServiceActionRejection::ServiceAlreadyHealthy,
        "duplicate healthy transition reports already-healthy rejection");

  t81::ternaryos::sched::TiscContext fault_ctx;
  fault_ctx.label = "service-faulted";
  fault_ctx.registers[0] = 1202;
  auto fault_tid = axion_kernel_spawn_thread_in_group(
      *state, fault_ctx, owner_runtime->process_group_id);
  check(fault_tid.has_value(), "service fault thread spawns in same group");
  if (!fault_tid) {
    return;
  }

  check(axion_kernel_step(*state), "service runtime dispatches owner thread");
  check(axion_kernel_step(*state), "service runtime dispatches supervisor peer thread");
  check(axion_kernel_step(*state), "service runtime dispatches foreign peer thread");
  check(axion_kernel_step(*state), "service runtime dispatches sibling fault thread");
  check(state->scheduler.current_tid() == *fault_tid,
        "service runtime fault thread becomes current");
  auto fault = axion_kernel_check_access(
      *state, mmu::tva_from_vpn_offset(401, 0), mmu::MmuAccessMode::Read);
  check(fault.fault.has_value(), "service runtime records sibling fault");
  check(axion_kernel_step(*state), "service runtime delivers sibling fault");

  auto blocked_service = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ServiceStatus,
          .service_id = *service_id,
      });
  check(blocked_service.status == KernelServiceStatus::FaultedGroup,
        "blocked service request is rejected deterministically");
  check(blocked_service.rejection == KernelServiceRequestRejection::FaultedRequestingGroup,
        "blocked service request reports faulted-group rejection");

  auto supervisor_inventory = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorServiceInventory,
          .supervisor_id = *supervisor_id,
      });
  check(supervisor_inventory.status == KernelServiceStatus::Ok,
        "supervisor inventory request succeeds");
  check(supervisor_inventory.rejection == KernelServiceRequestRejection::None,
        "supervisor inventory request clears rejection");
  check(supervisor_inventory.supervisor_services.has_value(),
        "supervisor inventory returns service view");
  if (supervisor_inventory.supervisor_services) {
    check(supervisor_inventory.supervisor_services->service_count == 1,
          "supervisor inventory retains one service while blocked");
    check(supervisor_inventory.supervisor_services->service_ids.size() == 1,
          "supervisor inventory retains service id while blocked");
    check(supervisor_inventory.supervisor_services->services.size() == 1,
          "supervisor inventory retains service entry while blocked");
    check(supervisor_inventory.supervisor_services->blocked_service_count == 1,
          "supervisor inventory reports one blocked service");
    check(supervisor_inventory.supervisor_services->suspended_service_count == 0,
          "supervisor inventory does not conflate blocked and suspended services");
    check(supervisor_inventory.supervisor_services->total_service_requests >= 1,
          "supervisor inventory aggregates service requests");
    check(supervisor_inventory.supervisor_services->total_service_rejections >= 1,
          "supervisor inventory aggregates rejected service requests");
    check(supervisor_inventory.supervisor_services->services.front().blocked,
          "supervisor inventory entry reports blocked service");
    check(supervisor_inventory.supervisor_services->services.front().pager_needed,
          "blocked supervisor inventory entry reports pager-needed state");
    check(supervisor_inventory.supervisor_services->services.front().pager_handoff_pending,
          "blocked supervisor inventory entry reports pending pager handoff");
    check(!supervisor_inventory.supervisor_services->services.front().pager_worker_owned,
          "blocked supervisor inventory entry is not yet worker-owned before handoff dispatch");
    check(supervisor_inventory.supervisor_services->services.front().pending_pager_fault_count == 1,
          "blocked supervisor inventory entry counts one pending pager fault");
    check(supervisor_inventory.supervisor_services->services.front().pager_faults == 1,
          "blocked supervisor inventory entry counts one pager fault");
    check(supervisor_inventory.supervisor_services->services.front().pager_handoffs == 0,
          "blocked supervisor inventory entry has not dispatched a pager handoff yet");
    check(supervisor_inventory.supervisor_services->services.front().pager_faults_coalesced == 0,
          "blocked supervisor inventory entry has not coalesced pager faults yet");
    check(supervisor_inventory.supervisor_services->services.front().last_pager_fault.has_value(),
          "blocked supervisor inventory entry exposes the pager fault record");
    check(supervisor_inventory.supervisor_services->services.front().last_transition_kind ==
              KernelAuditEventKind::ServiceMarkedHealthy,
          "blocked supervisor inventory entry retains the latest lifecycle event");
    check(supervisor_inventory.supervisor_services->services.front()
              .last_transition_sequence.has_value(),
          "blocked supervisor inventory entry retains the latest lifecycle audit sequence");
  }

  auto bad_unregister = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::UnregisterService,
          .service_id = *service_id,
      });
  check(bad_unregister.status == KernelServiceStatus::InvalidRequest,
        "missing-group unregister path is rejected");
  check(bad_unregister.rejection == KernelServiceActionRejection::MissingRequestingGroup,
        "missing-group unregister reports missing requesting group");

  check(axion_kernel_ack_thread_fault(*state, *fault_tid),
        "service runtime fault thread acknowledgement drains inbox");
  check(axion_kernel_ack_supervisor_group_fault(*state, *supervisor_id,
                                                owner_runtime->process_group_id),
        "service runtime supervisor acknowledgement completes recovery gate");

  auto unregister_service = axion_kernel_service_action(
      *state,
      KernelServiceAction{
          .kind = KernelServiceActionKind::UnregisterService,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(unregister_service.status == KernelServiceStatus::Ok,
        "healthy owner group can unregister its service");
  check(unregister_service.rejection == KernelServiceActionRejection::None,
        "successful unregister clears rejection");
  check(unregister_service.action_performed,
        "service unregister reports work performed");
  check(unregister_service.service.has_value(),
        "service unregister returns final service status");
  check(unregister_service.supervisor_services.has_value(),
        "service unregister returns final supervisor inventory");
  if (unregister_service.service) {
    check(!unregister_service.service->registered,
          "unregistered service reports unregistered state");
    check(!unregister_service.service->blocked,
          "unregistered service is no longer blocked");
    check(!unregister_service.service->suspended,
          "unregistered service is no longer suspended");
    check(unregister_service.service->state_transitions >= 2,
          "unregistered service increments lifecycle transitions");
    check(unregister_service.service->last_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "service view tracks unregister as the latest lifecycle event");
    check(unregister_service.service->last_transition_sequence.has_value(),
          "service view retains the latest lifecycle audit sequence after unregister");
  }
  if (unregister_service.supervisor_services) {
    check(unregister_service.supervisor_services->service_count == 0,
          "supervisor inventory empties after unregister");
    check(unregister_service.supervisor_services->suspended_service_count == 0,
          "supervisor inventory has no suspended services after unregister");
    check(unregister_service.supervisor_services->services.empty(),
          "supervisor inventory removes service entries after unregister");
    check(unregister_service.supervisor_services->service_lifecycle_transitions == 8,
          "supervisor inventory retains total lifecycle transition count after unregister");
    check(unregister_service.supervisor_services->last_service_transition_id == service_id,
          "supervisor inventory retains the last transitioned service after unregister");
    check(unregister_service.supervisor_services->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "supervisor inventory tracks unregister as the latest lifecycle event");
    check(unregister_service.supervisor_services->last_service_transition_sequence.has_value(),
          "supervisor inventory retains the latest lifecycle audit sequence after unregister");
  }
  auto supervisor_status_after_unregister = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .supervisor_id = *supervisor_id,
      });
  check(supervisor_status_after_unregister.status == KernelServiceStatus::Ok,
        "healthy group can request supervisor status after unregister");
  check(supervisor_status_after_unregister.rejection == KernelServiceRequestRejection::None,
        "supervisor status after unregister clears rejection");
  check(supervisor_status_after_unregister.supervisor.has_value(),
        "supervisor status after unregister returns a supervisor view");
  if (supervisor_status_after_unregister.supervisor) {
    check(supervisor_status_after_unregister.supervisor->managed_service_count == 0,
          "supervisor status reports zero managed services after unregister");
    check(supervisor_status_after_unregister.supervisor->service_lifecycle_transitions == 8,
          "supervisor status retains lifecycle transition count after unregister");
    check(supervisor_status_after_unregister.supervisor->last_service_transition_id == service_id,
          "supervisor status retains the last transitioned service after unregister");
    check(supervisor_status_after_unregister.supervisor->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "supervisor status tracks unregister as the latest lifecycle event");
    check(supervisor_status_after_unregister.supervisor->last_service_transition_sequence.has_value(),
          "supervisor status retains the latest lifecycle audit sequence after unregister");
  }
  auto recovery_status_after_unregister = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::SupervisorRecoveryStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .supervisor_id = *supervisor_id,
      });
  check(recovery_status_after_unregister.status == KernelServiceStatus::Ok,
        "healthy group can request supervisor recovery status after unregister");
  check(recovery_status_after_unregister.rejection == KernelServiceRequestRejection::None,
        "supervisor recovery status after unregister clears rejection");
  check(recovery_status_after_unregister.supervisor_recovery.has_value(),
        "supervisor recovery status after unregister returns a recovery view");
  if (recovery_status_after_unregister.supervisor_recovery) {
    check(recovery_status_after_unregister.supervisor_recovery->managed_service_count == 0,
          "recovery status reports zero managed services after unregister");
    check(recovery_status_after_unregister.supervisor_recovery->service_lifecycle_transitions == 8,
          "recovery status retains lifecycle transition count after unregister");
    check(recovery_status_after_unregister.supervisor_recovery->last_service_transition_id ==
              service_id,
          "recovery status retains the last transitioned service after unregister");
    check(recovery_status_after_unregister.supervisor_recovery->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "recovery status tracks unregister as the latest service lifecycle event");
    check(recovery_status_after_unregister.supervisor_recovery->last_service_transition_sequence
              .has_value(),
          "recovery status retains the latest service lifecycle audit sequence after unregister");
  }
  auto fault_summary_after_unregister = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::FaultSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(fault_summary_after_unregister.status == KernelServiceStatus::Ok,
        "healthy group can request fault summary after unregister");
  check(fault_summary_after_unregister.rejection == KernelServiceRequestRejection::None,
        "fault summary after unregister clears rejection");
  check(fault_summary_after_unregister.fault_summary.has_value(),
        "fault summary after unregister returns a fault view");
  if (fault_summary_after_unregister.fault_summary) {
    check(fault_summary_after_unregister.fault_summary->service_lifecycle_transitions == 8,
          "fault summary retains lifecycle transition count after unregister");
    check(fault_summary_after_unregister.fault_summary->last_service_transition_id == service_id,
          "fault summary retains the last transitioned service after unregister");
    check(fault_summary_after_unregister.fault_summary->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "fault summary tracks unregister as the latest service lifecycle event");
    check(fault_summary_after_unregister.fault_summary->last_service_transition_sequence.has_value(),
          "fault summary retains the latest service lifecycle audit sequence after unregister");
  }
  auto runtime_status_after_unregister = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::RuntimeStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(runtime_status_after_unregister.status == KernelServiceStatus::Ok,
        "healthy group can request runtime status after unregister");
  check(runtime_status_after_unregister.rejection == KernelServiceRequestRejection::None,
        "runtime status after unregister clears rejection");
  check(runtime_status_after_unregister.runtime.has_value(),
        "runtime status after unregister returns a runtime view");
  if (runtime_status_after_unregister.runtime) {
    check(runtime_status_after_unregister.runtime->managed_service_count == 0,
          "runtime status reports zero managed services after unregister");
    check(runtime_status_after_unregister.runtime->service_lifecycle_transitions == 8,
          "runtime status retains lifecycle transition count after unregister");
    check(runtime_status_after_unregister.runtime->last_service_transition_id == service_id,
          "runtime status retains the last transitioned service after unregister");
    check(runtime_status_after_unregister.runtime->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "runtime status tracks unregister as the latest service lifecycle event");
    check(runtime_status_after_unregister.runtime->last_service_transition_sequence.has_value(),
          "runtime status retains the latest service lifecycle audit sequence after unregister");
  }
  auto audit_summary_after_unregister = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::AuditSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(audit_summary_after_unregister.status == KernelServiceStatus::Ok,
        "healthy group can request audit summary after unregister");
  check(audit_summary_after_unregister.rejection == KernelServiceRequestRejection::None,
        "audit summary after unregister clears rejection");
  check(audit_summary_after_unregister.audit_summary.has_value(),
        "audit summary after unregister returns an audit view");
  if (audit_summary_after_unregister.audit_summary) {
    check(audit_summary_after_unregister.audit_summary->service_lifecycle_transitions == 8,
          "audit summary retains lifecycle transition count after unregister");
    check(audit_summary_after_unregister.audit_summary->last_service_transition_id == service_id,
          "audit summary retains the last transitioned service after unregister");
    check(audit_summary_after_unregister.audit_summary->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "audit summary tracks unregister as the latest service lifecycle event");
    check(audit_summary_after_unregister.audit_summary->last_service_transition_sequence
              .has_value(),
          "audit summary retains the latest service lifecycle audit sequence after unregister");
  }
  auto device_summary_after_unregister = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::DeviceSummary,
          .requesting_process_group_id = owner_runtime->process_group_id,
      });
  check(device_summary_after_unregister.status == KernelServiceStatus::Ok,
        "healthy group can request device summary after unregister");
  check(device_summary_after_unregister.rejection == KernelServiceRequestRejection::None,
        "device summary after unregister clears rejection");
  check(device_summary_after_unregister.device_summary.has_value(),
        "device summary after unregister returns a device view");
  if (device_summary_after_unregister.device_summary) {
    check(device_summary_after_unregister.device_summary->service_lifecycle_transitions == 8,
          "device summary retains lifecycle transition count after unregister");
    check(device_summary_after_unregister.device_summary->last_service_transition_id == service_id,
          "device summary retains the last transitioned service after unregister");
    check(device_summary_after_unregister.device_summary->last_service_transition_kind ==
              KernelAuditEventKind::ServiceUnregistered,
          "device summary tracks unregister as the latest service lifecycle event");
    check(device_summary_after_unregister.device_summary->last_service_transition_sequence
              .has_value(),
          "device summary retains the latest service lifecycle audit sequence after unregister");
  }
  check(!state->audit_log.empty(), "service lifecycle actions produce audit records");
  if (!state->audit_log.empty()) {
    check(state->audit_log.back().kind == KernelAuditEventKind::ServiceUnregistered,
          "service unregister records the final lifecycle audit event");
  }

  auto missing_service_view = axion_kernel_service_request(
      *state,
      KernelServiceRequest{
          .kind = KernelServiceRequestKind::ServiceStatus,
          .requesting_process_group_id = owner_runtime->process_group_id,
          .service_id = *service_id,
      });
  check(missing_service_view.status == KernelServiceStatus::NotFound,
        "service status rejects unregistered service");
  check(missing_service_view.rejection == KernelServiceRequestRejection::MissingService,
        "service status reports missing service after unregister");
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
  test_kernel_interrupt_event_delivery();
  test_kernel_pager_fault_state();
  test_kernel_pager_worker_backlog();
  test_kernel_pager_worker_ready_bypass_cap();
  test_kernel_pager_worker_terminal_parked_head();
  test_kernel_boot_critical_pager_resolution();
  test_kernel_fault_process_boundary();
  test_kernel_fault_acknowledgement_and_recovery();
  test_kernel_process_group_fault_gate();
  test_kernel_process_group_audit_log();
  test_kernel_supervisor_fault_boundary();
  test_kernel_supervisor_acknowledgement_flow();
  test_kernel_service_runtime_views();
  test_kernel_service_group_fault_visibility();
  test_kernel_service_fault_ack_action();
  test_kernel_service_supervisor_recovery_status();
  test_kernel_service_device_actions();
  test_kernel_service_diagnostic_details();
  test_kernel_service_runtime_layer();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
