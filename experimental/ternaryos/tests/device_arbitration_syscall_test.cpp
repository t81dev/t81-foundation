// experimental/ternaryos/tests/device_arbitration_syscall_test.cpp
//
// RFC-00B6 §5.3.6 — ClaimDevice / ReleaseDevice / QueryDevice KernelCallKind
// ABI tests.  Closes the open question noted in RFC-00B6 §9 acceptance notes.
//
// Acceptance criteria covered:
//   [AC-22d-01]  ClaimDevice on an unclaimed device succeeds; device_claimed = true.
//   [AC-22d-02]  ClaimDevice on a device already owned by a different thread
//                returns Conflict / DeviceAlreadyClaimed.
//   [AC-22d-03]  Same owner may idempotently re-claim (ClaimDevice is
//                monotonic for the current owner).
//   [AC-22d-04]  ReleaseDevice by the owner succeeds; device_released = true.
//   [AC-22d-05]  ReleaseDevice by a non-owner returns CapabilityDenied /
//                DeviceNotOwned.
//   [AC-22d-06]  QueryDevice returns device_is_claimed = false for unclaimed
//                device and device_is_claimed = true with the correct owner_tid
//                after a successful claim.
//   [AC-22d-07]  ClaimDevice / ReleaseDevice / QueryDevice all return
//                InvalidRequest / MissingDeviceName when device_name is absent.
//   [AC-22d-08]  ClaimDevice / ReleaseDevice / QueryDevice all return
//                NotFound / DeviceNotFound for an unknown device name.

#include "../kernel/kernel_abi.hpp"
#include "../kernel/kernel_main.hpp"
#include "../hal/hal.hpp"
#include "../hal/virtualbox_platform.hpp"
#include "../sched/tisc_context.hpp"

#include <cassert>
#include <cstdio>

using namespace t81::ternaryos::kernel;
using namespace t81::ternaryos::hal;
using namespace t81::ternaryos::sched;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Build a VirtualBox-backed BootContext so the kernel bootstraps device
/// arbitration (devices: ahci, e1000, vmsvga, etc.).
static BootContext make_vbox_boot_ctx() {
  VBoxBootSpec spec;
  spec.ram_bytes             = 64ULL * 1024 * 1024;  // 64 MiB
  spec.ethics_boot_required  = false;
  return make_virtualbox_boot_context(spec);
}

// ── [AC-22d-07] Missing device_name → MissingDeviceName ──────────────────────

static void test_missing_device_name(KernelRuntimeState& state) {
  std::printf("\n[AC-22d-07] Missing device_name → MissingDeviceName for all three calls\n");

  for (auto kind : {KernelCallKind::ClaimDevice,
                    KernelCallKind::ReleaseDevice,
                    KernelCallKind::QueryDevice}) {
    KernelCallRequest req;
    req.kind = kind;
    // device_name intentionally absent
    const auto result = axion_kernel_call(state, req);
    check(result.status    == KernelCallStatus::InvalidRequest,
          "[AC-22d-07] status == InvalidRequest");
    check(result.rejection == KernelCallRejection::MissingDeviceName,
          "[AC-22d-07] rejection == MissingDeviceName");
  }
}

// ── [AC-22d-08] Unknown device name → DeviceNotFound ─────────────────────────

static void test_unknown_device_name(KernelRuntimeState& state) {
  std::printf("\n[AC-22d-08] Unknown device name → DeviceNotFound\n");

  for (auto kind : {KernelCallKind::ClaimDevice,
                    KernelCallKind::ReleaseDevice,
                    KernelCallKind::QueryDevice}) {
    KernelCallRequest req;
    req.kind        = kind;
    req.device_name = "no-such-device";
    const auto result = axion_kernel_call(state, req);
    check(result.status    == KernelCallStatus::NotFound,
          "[AC-22d-08] status == NotFound");
    check(result.rejection == KernelCallRejection::DeviceNotFound,
          "[AC-22d-08] rejection == DeviceNotFound");
  }
}

// ── [AC-22d-06] QueryDevice on unclaimed device ───────────────────────────────

static void test_query_unclaimed_device(KernelRuntimeState& state) {
  std::printf("\n[AC-22d-06a] QueryDevice on unclaimed device → device_is_claimed = false\n");

  KernelCallRequest req;
  req.kind        = KernelCallKind::QueryDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status == KernelCallStatus::Ok,
        "[AC-22d-06a] QueryDevice status == Ok");
  check(!result.device_is_claimed,
        "[AC-22d-06a] device_is_claimed == false for unclaimed ahci");
  check(!result.device_owner_tid.has_value(),
        "[AC-22d-06a] device_owner_tid absent for unclaimed ahci");
}

// ── [AC-22d-01] ClaimDevice succeeds ─────────────────────────────────────────

static void test_claim_device_ok(KernelRuntimeState& state) {
  std::printf("\n[AC-22d-01] ClaimDevice on unclaimed device succeeds\n");

  KernelCallRequest req;
  req.kind        = KernelCallKind::ClaimDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status == KernelCallStatus::Ok,
        "[AC-22d-01] status == Ok");
  check(result.rejection == KernelCallRejection::None,
        "[AC-22d-01] rejection == None");
  check(result.action_performed,
        "[AC-22d-01] action_performed == true");
  check(result.device_claimed,
        "[AC-22d-01] device_claimed == true");
}

// ── [AC-22d-06b] QueryDevice after claim shows owner ─────────────────────────

static void test_query_claimed_device(KernelRuntimeState& state,
                                      Tid expected_owner) {
  std::printf("\n[AC-22d-06b] QueryDevice after claim reports owner tid\n");

  KernelCallRequest req;
  req.kind        = KernelCallKind::QueryDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status == KernelCallStatus::Ok,
        "[AC-22d-06b] QueryDevice status == Ok");
  check(result.device_is_claimed,
        "[AC-22d-06b] device_is_claimed == true after claim");
  check(result.device_owner_tid.has_value(),
        "[AC-22d-06b] device_owner_tid is present");
  check(result.device_owner_tid.value_or(Tid{0}) == expected_owner,
        "[AC-22d-06b] device_owner_tid matches caller tid");
}

// ── [AC-22d-03] Idempotent re-claim by same owner ────────────────────────────

static void test_idempotent_reclaim(KernelRuntimeState& state) {
  std::printf("\n[AC-22d-03] Same owner can idempotently re-claim the device\n");

  KernelCallRequest req;
  req.kind        = KernelCallKind::ClaimDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status == KernelCallStatus::Ok,
        "[AC-22d-03] re-claim status == Ok");
  check(result.device_claimed,
        "[AC-22d-03] device_claimed == true on re-claim");
}

// ── [AC-22d-04] ReleaseDevice by owner succeeds ──────────────────────────────

static void test_release_device_ok(KernelRuntimeState& state) {
  std::printf("\n[AC-22d-04] ReleaseDevice by owner succeeds\n");

  KernelCallRequest req;
  req.kind        = KernelCallKind::ReleaseDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status == KernelCallStatus::Ok,
        "[AC-22d-04] status == Ok");
  check(result.rejection == KernelCallRejection::None,
        "[AC-22d-04] rejection == None");
  check(result.action_performed,
        "[AC-22d-04] action_performed == true");
  check(result.device_released,
        "[AC-22d-04] device_released == true");
}

// ── [AC-22d-02] Claim conflict — different owner ──────────────────────────────
// This test spawns a second thread, has it claim ahci, then verifies
// the first thread (the kernel tick caller) cannot steal the claim.

static void test_claim_conflict(KernelRuntimeState& state,
                                Tid first_owner) {
  std::printf("\n[AC-22d-02] ClaimDevice by different owner → Conflict / DeviceAlreadyClaimed\n");

  // Spawn a second thread and make it claim ahci by mutating device state
  // directly — since axion_kernel_call() always uses the current scheduler
  // thread as owner, the simplest way to set up a "different owner" is to
  // write the owner_tid directly into the registry and then verify the call
  // from the current thread (first_owner) sees a conflict.
  //
  // Find ahci in the registry and assign a fictitious owner tid.
  const Tid other_owner = first_owner + 100;  // guaranteed different
  if (state.device_arbitration) {
    for (auto& d : state.device_arbitration->devices) {
      if (d.name == "ahci") {
        d.owner_tid = other_owner;
        break;
      }
    }
  }

  KernelCallRequest req;
  req.kind        = KernelCallKind::ClaimDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status    == KernelCallStatus::Conflict,
        "[AC-22d-02] status == Conflict");
  check(result.rejection == KernelCallRejection::DeviceAlreadyClaimed,
        "[AC-22d-02] rejection == DeviceAlreadyClaimed");
  check(!result.device_claimed,
        "[AC-22d-02] device_claimed == false on conflict");

  // Restore: clear the fictitious owner so subsequent tests are clean.
  if (state.device_arbitration) {
    for (auto& d : state.device_arbitration->devices) {
      if (d.name == "ahci") { d.owner_tid.reset(); break; }
    }
  }
}

// ── [AC-22d-05] ReleaseDevice by non-owner ───────────────────────────────────

static void test_release_not_owned(KernelRuntimeState& state,
                                   Tid current_caller) {
  std::printf("\n[AC-22d-05] ReleaseDevice by non-owner → CapabilityDenied / DeviceNotOwned\n");

  // Assign a different owner to ahci so the current caller doesn't own it.
  const Tid other_owner = current_caller + 200;
  if (state.device_arbitration) {
    for (auto& d : state.device_arbitration->devices) {
      if (d.name == "ahci") { d.owner_tid = other_owner; break; }
    }
  }

  KernelCallRequest req;
  req.kind        = KernelCallKind::ReleaseDevice;
  req.device_name = "ahci";
  const auto result = axion_kernel_call(state, req);
  check(result.status    == KernelCallStatus::CapabilityDenied,
        "[AC-22d-05] status == CapabilityDenied");
  check(result.rejection == KernelCallRejection::DeviceNotOwned,
        "[AC-22d-05] rejection == DeviceNotOwned");
  check(!result.device_released,
        "[AC-22d-05] device_released == false on non-owner release");

  // Restore.
  if (state.device_arbitration) {
    for (auto& d : state.device_arbitration->devices) {
      if (d.name == "ahci") { d.owner_tid.reset(); break; }
    }
  }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== ClaimDevice/ReleaseDevice/QueryDevice KernelCallKind ABI tests (RFC-00B6 §5.3.6) ===\n");

  const auto ctx = make_vbox_boot_ctx();
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed\n");
    return 1;
  }
  auto& state = *state_opt;

  if (!state.device_arbitration.has_value()) {
    std::printf("FATAL: device_arbitration not bootstrapped for VirtualBox platform\n");
    return 1;
  }

  // One tick establishes the kernel thread (kKernelTid) as the current
  // caller context for axion_kernel_call().
  axion_kernel_tick(state);

  // Determine the caller tid (the current scheduler thread after tick).
  const Tid caller_tid = state.scheduler.current_tid();

  // Run tests in a logical sequence: query → claim → re-claim → query → release.
  test_missing_device_name(state);
  test_unknown_device_name(state);
  test_query_unclaimed_device(state);
  test_claim_device_ok(state);
  test_query_claimed_device(state, caller_tid);
  test_idempotent_reclaim(state);
  test_release_device_ok(state);
  test_claim_conflict(state, caller_tid);
  test_release_not_owned(state, caller_tid);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
