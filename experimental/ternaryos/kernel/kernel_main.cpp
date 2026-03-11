#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

namespace {

constexpr std::string_view kVBoxPlatformPrefix = "virtualbox-x86_64:";

std::optional<KernelDeviceArbitrationState> bootstrap_device_arbitration(
    const std::string& platform_id) {
  if (!platform_id.starts_with(kVBoxPlatformPrefix)) {
    return std::nullopt;
  }

  const hal::VBoxProfile profile{};
  const auto profile_summary = hal::virtualbox_profile_summary(profile);
  const std::string expected_platform_id =
      std::string(kVBoxPlatformPrefix) + profile_summary;
  if (platform_id != expected_platform_id) {
    return std::nullopt;
  }

  KernelDeviceArbitrationState state;
  state.profile_summary = profile_summary;
  for (const auto& dev : hal::virtualbox_device_map(profile)) {
    state.devices.push_back(KernelDeviceRecord{
        .name = dev.name,
        .bus = dev.bus,
        .base = dev.base,
        .span_bytes = dev.span_bytes,
        .irq = dev.irq,
    });
    state.has_storage = state.has_storage || state.devices.back().name == "ahci";
    state.has_network = state.has_network || state.devices.back().name == "e1000";
    state.has_display = state.has_display || state.devices.back().name == "vmsvga";
  }
  return state;
}

void record_fault(KernelRuntimeState& state,
                  uint64_t tva,
                  mmu::MmuAccessMode mode,
                  mmu::MmuFault fault) {
  if (state.fault_log.size() >= KernelRuntimeState::kMaxFaultLog) {
    state.fault_log.pop_front();
  }
  state.fault_log.push_back(KernelFaultRecord{
      .platform_id = state.platform_id,
      .tva = tva,
      .access_mode = mode,
      .fault = fault,
  });
}

KernelDeviceRecord* find_device(KernelRuntimeState& state, std::string_view device_name) {
  if (!state.device_arbitration) {
    return nullptr;
  }
  for (auto& device : state.device_arbitration->devices) {
    if (device.name == device_name) {
      return &device;
    }
  }
  return nullptr;
}

}  // namespace

std::optional<KernelRuntimeState> axion_kernel_bootstrap(
    const hal::BootContext& ctx) noexcept {
  if (ctx.memory_map.empty()) {
    return std::nullopt;
  }

  std::size_t memory_region_count = ctx.memory_map.size();
  uint64_t total_ternary_pages = 0;
  bool has_writable_memory = false;

  for (const auto& region : ctx.memory_map) {
    total_ternary_pages += region.ternary_page_count();
    has_writable_memory = has_writable_memory || region.writable;
  }

  if (!has_writable_memory) {
    return std::nullopt;
  }

  KernelRuntimeState state{
      ctx.platform_id,
      memory_region_count,
      total_ternary_pages,
      has_writable_memory,
      mmu::TernaryPageAllocator(ctx.memory_map),
  };
  state.ipc_bus.register_thread(KernelRuntimeState::kKernelTid);
  state.device_arbitration = bootstrap_device_arbitration(ctx.platform_id);
  return state;
}

KernelAccessReport axion_kernel_check_access(
    KernelRuntimeState& state,
    uint64_t tva,
    mmu::MmuAccessMode mode) noexcept {
  const auto result = mmu::mmu_translate_checked(state.page_table, tva, mode);
  if (result.fault == mmu::MmuFault::None) {
    return {.phys_addr = result.phys_addr, .fault = std::nullopt};
  }

  record_fault(state, tva, mode, result.fault);
  return {
      .phys_addr = std::nullopt,
      .fault = KernelFaultRecord{
          .platform_id = state.platform_id,
          .tva = tva,
          .access_mode = mode,
          .fault = result.fault,
      },
  };
}

std::optional<sched::Tid> axion_kernel_spawn_thread(
    KernelRuntimeState& state,
    sched::TiscContext ctx) noexcept {
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (tid.has_value()) {
    state.ipc_bus.register_thread(*tid);
  }
  return tid;
}

bool axion_kernel_tick(KernelRuntimeState& state) noexcept {
  ++state.counters.scheduler_ticks;
  const bool switched = state.scheduler.tick(state.cpu_context);
  if (switched) {
    ++state.counters.scheduler_switches;
  }
  return switched;
}

bool axion_kernel_step(KernelRuntimeState& state) noexcept {
  ++state.counters.loop_iterations;
  return axion_kernel_tick(state);
}

bool axion_kernel_ipc_send(KernelRuntimeState& state,
                           sched::Tid dst,
                           ipc::CanonMessage msg) noexcept {
  const bool sent = state.ipc_bus.ipc_send(dst, std::move(msg));
  if (sent) {
    ++state.counters.ipc_messages_sent;
  }
  return sent;
}

std::optional<ipc::CanonMessage> axion_kernel_ipc_recv(
    KernelRuntimeState& state,
    sched::Tid tid) noexcept {
  auto msg = state.ipc_bus.ipc_recv(tid);
  if (msg.has_value()) {
    ++state.counters.ipc_messages_received;
  }
  return msg;
}

bool axion_kernel_claim_device(KernelRuntimeState& state,
                               std::string_view device_name,
                               sched::Tid owner) noexcept {
  auto* device = find_device(state, device_name);
  if (!device) {
    return false;
  }
  if (device->owner_tid.has_value() && *device->owner_tid != owner) {
    return false;
  }
  device->owner_tid = owner;
  return true;
}

bool axion_kernel_release_device(KernelRuntimeState& state,
                                 std::string_view device_name,
                                 sched::Tid owner) noexcept {
  auto* device = find_device(state, device_name);
  if (!device || !device->owner_tid.has_value() || *device->owner_tid != owner) {
    return false;
  }
  device->owner_tid.reset();
  return true;
}

int axion_kernel_main(const hal::BootContext& ctx) noexcept {
  return axion_kernel_bootstrap(ctx).has_value() ? 0 : 1;
}

}  // namespace t81::ternaryos::kernel
