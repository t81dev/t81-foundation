#include "kernel_main.hpp"

#include <cstdlib>
#include <filesystem>

namespace t81::ternaryos::kernel {

namespace {

constexpr std::string_view kVBoxPlatformPrefix = "virtualbox-x86_64:";

CapabilityRecordId next_capability_record_id(KernelRuntimeState& state) {
  return state.next_capability_record_id++;
}

std::vector<KernelCapabilityRecord> default_process_group_capabilities(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id) {
  return {
      KernelCapabilityRecord{
          .record_id = next_capability_record_id(state),
          .kind = KernelCapabilityKind::Yield,
          .kernel_seeded = true,
      },
      KernelCapabilityRecord{
          .record_id = next_capability_record_id(state),
          .kind = KernelCapabilityKind::ThreadSpawn,
          .kernel_seeded = true,
      },
      KernelCapabilityRecord{
          .record_id = next_capability_record_id(state),
          .kind = KernelCapabilityKind::IpcSend,
          .kernel_seeded = true,
      },
      KernelCapabilityRecord{
          .record_id = next_capability_record_id(state),
          .kind = KernelCapabilityKind::IpcReceive,
          .kernel_seeded = true,
      },
      KernelCapabilityRecord{
          .record_id = next_capability_record_id(state),
          .kind = KernelCapabilityKind::FaultObserve,
          .process_group_scope = process_group_id,
          .kernel_seeded = true,
      },
      KernelCapabilityRecord{
          .record_id = next_capability_record_id(state),
          .kind = KernelCapabilityKind::FaultAcknowledge,
          .process_group_scope = process_group_id,
          .kernel_seeded = true,
      },
  };
}

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

std::unique_ptr<t81::canonfs::Driver> bootstrap_published_executable_canonfs() {
  const char* raw = std::getenv("T81_CANONFS_ROOT");
  if (!raw || raw[0] == '\0') {
    return nullptr;
  }
  std::filesystem::path canon_root(raw);
  std::error_code ec;
  std::filesystem::create_directories(canon_root, ec);
  return t81::canonfs::make_persistent_driver(canon_root);
}

KernelRuntimeState::ProcessGroupState* create_process_group(KernelRuntimeState& state) {
  const ProcessGroupId id = state.next_process_group_id++;
  auto [it, inserted] = state.process_groups.emplace(
      id,
      KernelRuntimeState::ProcessGroupState{
          .id = id,
          .capabilities = default_process_group_capabilities(state, id),
      });
  return inserted ? &it->second : nullptr;
}

KernelRuntimeState::SupervisorState* create_supervisor(KernelRuntimeState& state) {
  const SupervisorId id = state.next_supervisor_id++;
  auto [it, inserted] =
      state.supervisors.emplace(id, KernelRuntimeState::SupervisorState{.id = id});
  return inserted ? &it->second : nullptr;
}

KernelRuntimeState::AddressSpaceState* create_address_space(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id) {
  const AddressSpaceId id = state.next_address_space_id++;
  auto [it, inserted] = state.address_spaces.emplace(
      id,
      KernelRuntimeState::AddressSpaceState{
          .id = id,
          .process_group_id = process_group_id,
      });
  return inserted ? &it->second : nullptr;
}

KernelRuntimeState::ProcessGroupState* assign_thread_to_group(
    KernelRuntimeState& state, sched::Tid tid, ProcessGroupId process_group_id) {
  auto* thread_state = state.find_thread_runtime_mut(tid);
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!thread_state || !group_state) {
    return nullptr;
  }
  thread_state->process_group_id = process_group_id;
  group_state->member_tids.push_back(tid);
  return group_state;
}

KernelRuntimeState::SupervisorState* assign_group_to_supervisor(
    KernelRuntimeState& state, ProcessGroupId process_group_id, SupervisorId supervisor_id) {
  auto* supervisor_state = state.find_supervisor_mut(supervisor_id);
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!supervisor_state || !group_state) {
    return nullptr;
  }
  supervisor_state->managed_groups.push_back(process_group_id);
  state.process_group_supervisors[process_group_id] = supervisor_id;
  return supervisor_state;
}

KernelRuntimeState::AddressSpaceState* assign_group_to_address_space(
    KernelRuntimeState& state,
    ProcessGroupId process_group_id,
    AddressSpaceId address_space_id) {
  auto* address_space = state.find_address_space_mut(address_space_id);
  auto* group_state = state.find_process_group_mut(process_group_id);
  if (!address_space || !group_state) {
    return nullptr;
  }
  address_space->process_group_id = process_group_id;
  state.process_group_address_spaces[process_group_id] = address_space_id;
  return address_space;
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
  state.published_executable_canonfs = bootstrap_published_executable_canonfs();
  state.thread_runtime.emplace(
      KernelRuntimeState::kKernelTid,
      KernelRuntimeState::ThreadRuntimeState{
          .tid = KernelRuntimeState::kKernelTid,
          .process_group_id = KernelRuntimeState::kKernelProcessGroup,
      });
  state.process_groups.emplace(
      KernelRuntimeState::kKernelProcessGroup,
      KernelRuntimeState::ProcessGroupState{
          .id = KernelRuntimeState::kKernelProcessGroup,
          .member_tids = {KernelRuntimeState::kKernelTid},
          .capabilities = default_process_group_capabilities(
              state,
              KernelRuntimeState::kKernelProcessGroup),
      });
  state.address_spaces.emplace(
      KernelRuntimeState::kKernelAddressSpace,
      KernelRuntimeState::AddressSpaceState{
          .id = KernelRuntimeState::kKernelAddressSpace,
          .process_group_id = KernelRuntimeState::kKernelProcessGroup,
          .kernel_owned = true,  // RFC-00B1 §3.1: kernel AS may map the full TVA space
      });
  state.supervisors.emplace(
      KernelRuntimeState::kKernelSupervisor,
      KernelRuntimeState::SupervisorState{
          .id = KernelRuntimeState::kKernelSupervisor,
          .managed_groups = {KernelRuntimeState::kKernelProcessGroup},
      });
  state.process_group_supervisors.emplace(KernelRuntimeState::kKernelProcessGroup,
                                          KernelRuntimeState::kKernelSupervisor);
  state.process_group_address_spaces.emplace(KernelRuntimeState::kKernelProcessGroup,
                                             KernelRuntimeState::kKernelAddressSpace);
  state.device_arbitration = bootstrap_device_arbitration(ctx.platform_id);
  return state;
}

std::optional<sched::Tid> axion_kernel_spawn_thread(
    KernelRuntimeState& state,
    sched::TiscContext ctx) noexcept {
  auto* group = create_process_group(state);
  auto* supervisor = create_supervisor(state);
  auto* address_space = group ? create_address_space(state, group->id) : nullptr;
  if (!group || !supervisor || !address_space) {
    if (group) {
      state.process_groups.erase(group->id);
    }
    if (supervisor) {
      state.supervisors.erase(supervisor->id);
    }
    if (address_space) {
      state.address_spaces.erase(address_space->id);
    }
    return std::nullopt;
  }
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (tid.has_value()) {
    state.ipc_bus.register_thread(*tid);
    state.thread_runtime.emplace(*tid,
                                 KernelRuntimeState::ThreadRuntimeState{
                                     .tid = *tid,
                                     .process_group_id = group->id,
                                 });
    assign_thread_to_group(state, *tid, group->id);
    assign_group_to_supervisor(state, group->id, supervisor->id);
    assign_group_to_address_space(state, group->id, address_space->id);
  } else {
    state.process_groups.erase(group->id);
    state.supervisors.erase(supervisor->id);
    state.address_spaces.erase(address_space->id);
  }
  return tid;
}

std::optional<sched::Tid> axion_kernel_spawn_thread_in_group(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    ProcessGroupId process_group_id) noexcept {
  if (!state.find_process_group(process_group_id)) {
    return std::nullopt;
  }
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (tid.has_value()) {
    state.ipc_bus.register_thread(*tid);
    state.thread_runtime.emplace(*tid,
                                 KernelRuntimeState::ThreadRuntimeState{
                                     .tid = *tid,
                                     .process_group_id = process_group_id,
                                 });
    assign_thread_to_group(state, *tid, process_group_id);
  }
  return tid;
}

std::optional<sched::Tid> axion_kernel_spawn_thread_under_supervisor(
    KernelRuntimeState& state,
    sched::TiscContext ctx,
    SupervisorId supervisor_id) noexcept {
  if (!state.find_supervisor(supervisor_id)) {
    return std::nullopt;
  }
  auto* group = create_process_group(state);
  auto* address_space = group ? create_address_space(state, group->id) : nullptr;
  if (!group || !address_space) {
    if (group) {
      state.process_groups.erase(group->id);
    }
    return std::nullopt;
  }
  auto tid = state.scheduler.spawn(std::move(ctx));
  if (!tid.has_value()) {
    state.process_groups.erase(group->id);
    state.address_spaces.erase(address_space->id);
    return std::nullopt;
  }
  state.ipc_bus.register_thread(*tid);
  state.thread_runtime.emplace(*tid,
                               KernelRuntimeState::ThreadRuntimeState{
                                   .tid = *tid,
                                   .process_group_id = group->id,
                               });
  assign_thread_to_group(state, *tid, group->id);
  if (!assign_group_to_supervisor(state, group->id, supervisor_id)) {
    state.thread_runtime.erase(*tid);
    state.process_groups.erase(group->id);
    state.address_spaces.erase(address_space->id);
    return std::nullopt;
  }
  if (!assign_group_to_address_space(state, group->id, address_space->id)) {
    state.thread_runtime.erase(*tid);
    state.process_groups.erase(group->id);
    state.address_spaces.erase(address_space->id);
    return std::nullopt;
  }
  return tid;
}

}  // namespace t81::ternaryos::kernel
