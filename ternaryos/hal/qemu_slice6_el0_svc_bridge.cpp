// ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp
//
// Phase 6 — Freestanding KernelCall SVC bridge (RFC-00BC §"Phase 6").
//
// Implements a freestanding subset of the Axion kernel call wire protocol,
// reachable from EL0 through SVC #1 (KernelCall).  This bridge is the entry
// point into the governed kernel call path from EL0 userland; it routes
// based on the `kind` field in the wire request block.
//
// Wire block compact subset (byte offsets are byte-identical to the hosted
// KernelCallWireRequestBlock / KernelCallWireResponseBlock from
// kernel_abi_wire.hpp — only the minimum fields needed for Phase 6 are used):
//
//   Request (minimum 12 bytes readable, compact Phase 6 block = 16 bytes):
//     [0:4]   magic   = 0x4B415152  (KAQR in big-endian, little-endian uint32)
//     [4:6]   version = 1           (uint16_t)
//     [6:8]   bytes   ≥ 12          (uint16_t — actual caller buffer size)
//     [8:12]  kind    = KernelCallKind (uint32_t; see kernel_abi.hpp enum)
//
//   Response (minimum 48 bytes writable, covers through caller_tid):
//     [0:4]   magic     = 0x4B415250  (KARP)
//     [4:6]   version   = 1
//     [6:8]   bytes     = rsp_size
//     [8:12]  status    = 0 (Ok) / 1 (InvalidRequest)
//     [12:16] rejection = 0
//     [16:24] flags     = 0
//     [24:35] bool fields × 11  (action_performed … service_blocked, reserved0)
//     [35:36] alignment pad     (implicit, before first uint32_t field)
//     [36:40] spawned_tid = 0
//     [40:44] queried_tid = 0
//     [44:48] caller_tid  = 1   (for GetThreadIdentity)
//
// RFC-0048 governance classification: governed non-DCP transport layer.
// The dispatcher is a straight-through shim; no side effects are inserted
// between SVC entry and the dispatched handler.  All observable T81 state
// changes happen inside the dispatched operation, not in this file.
//
// DPE touch point 1 (RFC-DPE-0002 §4): GetThreadIdentity does not require
// PagerService capability.  SubmitEpoch, which does, is not dispatched here.
//
// DPE touch point 2 (RFC-DPE-0008): the dispatch path does not bypass
// record_audit_event(); Phase 6 kernel calls that have no hosted KernelRuntimeState
// emit no audit events (the freestanding bridge has no audit trail).  This is
// acceptable for the EL0 init probe (tid=1 identity query); audit wiring is
// a Phase 7+ concern when a bootstrapped KernelRuntimeState is available.
//
// Compilation constraints (same as other slice6 freestanding TUs):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib  --target=aarch64-pc-windows-msvc

#include <stdint.h>

// ── AArch64 trap frame layout ─────────────────────────────────────────────────
//
// Must match AArch64TrapFrameSimple in qemu_slice6_bridge_irq.cpp and the
// save sequence in axion_svc_entry (aarch64_exception_vectors.S):
//   x[0..30] at offsets 0..240,  sp_el0 at 248,
//   elr_el1 at 256,  spsr_el1 at 264,  esr_el1 at 272.

struct FsBridgeTrapFrame {
    uint64_t x[31];     // x0..x30         offsets   0..247
    uint64_t sp_el0;    // EL0 stack ptr   offset  248
    uint64_t elr_el1;   // EL1 exception link register  offset 256
    uint64_t spsr_el1;  // saved program status         offset 264
    uint64_t esr_el1;   // exception syndrome           offset 272
};

// ── Wire protocol constants ───────────────────────────────────────────────────

static constexpr uint32_t kReqMagic              = 0x4B415152u;  // KAQR
static constexpr uint32_t kRspMagic              = 0x4B415250u;  // KARP
static constexpr uint32_t kStatusOk              = 0u;
static constexpr uint32_t kStatusInvalidRequest  = 1u;

// KernelCallKind numeric values (Phase 6–9 subset, from kernel_abi.hpp):
//   GetThreadIdentity   = 10
//   ExitThread          = 12
//   SendMessage         = 13  (Phase 8 IPC; Phase 9 waker)
//   ReceiveMessage      = 14  (Phase 8 IPC)
//   BlockOnIpcReceive   = 42  (Phase 9 cooperative scheduler park, RFC-00BE)
//   WaitForDevice       = 43  (Phase 9 cooperative scheduler park, RFC-00BE)
static constexpr uint32_t kKindGetThreadIdentity  = 10u;
static constexpr uint32_t kKindSendMessage         = 13u;
static constexpr uint32_t kKindReceiveMessage      = 14u;
static constexpr uint32_t kKindBlockOnIpcReceive   = 42u;
static constexpr uint32_t kKindWaitForDevice       = 43u;

// Minimum response size covering all fields through caller_tid at [44:48].
static constexpr uint64_t kMinRspBytes = 48u;

// Minimum request size covering magic (4B) + version/bytes (4B) + kind (4B).
static constexpr uint64_t kMinReqBytes = 12u;

// Phase 5 TVA validator — defined in qemu_slice6_el0_mmu.cpp.
extern "C" int el0_tva_valid(uint64_t va, uint64_t size) noexcept;

// EL1 return PC — written by run_proc_entry() in canon_exec_loader.cpp before
// the first ERET; ExitThread uses it when no Runnable threads remain.
extern "C" uint64_t g_axion_el1_return_pc;

// RFC-00C2: device-wait loop PC — set to fs_sched_device_wait_loop() by
// canon_irq_wake_load_and_run() before a scheduler session that uses
// IRQ-driven WaitForDevice waking.  Zero means use g_axion_el1_return_pc
// (Phase 11 EL1-direct wake path).
extern "C" uint64_t g_axion_el1_device_wait_pc = 0u;

// ── Per-thread identity tracker (Phase 7) ────────────────────────────────────
//
// Declared here (before the scheduler block) because both el0_svc_set_current_tid
// and fs_sched_exit_thread write to it.
static uint32_t s_current_el0_tid = 1u;

extern "C" void el0_svc_set_current_tid(uint32_t tid) noexcept {
    s_current_el0_tid = tid;
}

// ── RFC-00BE cooperative scheduler ───────────────────────────────────────────
//
// Fixed-capacity thread table.  All state is module-private except the exported
// APIs used by canon_exec_loader.cpp and qemu_slice6_bridge_irq.cpp.

enum class FsSchedState : uint8_t {
    Unused            = 0,
    Runnable          = 1,
    Running           = 2,
    BlockedIpcReceive = 3,
    BlockedDeviceWait = 4,
    Exited            = 5,
};

// RFC-00C6: sentinel meaning "use shared L3 (no per-thread isolation)".
static constexpr uint32_t kNoThreadL3 = ~0u;

struct FsSchedThread {
    uint32_t     tid;             // 0 = slot unused
    FsSchedState state;
    uint64_t     resume_elr;      // ELR_EL1: entry PA or resume PC
    uint64_t     resume_sp_el0;   // SP_EL0 at registration or block time
    uint64_t     resume_spsr;     // SPSR_EL1 (0x3C0 for EL0t + DAIF masked)
    uint64_t     ipc_rsp_tva;     // EL0 VA of response buffer (saved at block)
    uint64_t     ipc_rsp_size;    // response buffer size in bytes
    uint32_t     device_id;       // RFC-00C4: INTID to match on wake; 0 = any
    uint32_t     l3_slot;         // RFC-00C6: per-thread L3 slot; kNoThreadL3 = shared
};

// RFC-00C6: per-thread L3 MMU API (qemu_slice6_el0_mmu.cpp).
extern "C" void el0_mmu_install_thread_l3(uint32_t slot) noexcept;
extern "C" void el0_mmu_install_shared_l3() noexcept;

static constexpr int kMaxSchedThreads = 8;
static FsSchedThread s_sched[kMaxSchedThreads];
static uint32_t      s_sched_running_tid  = 0u;
static bool          s_sched_ipc_delivered = false;

// ── Scheduler helper functions ────────────────────────────────────────────────

static FsSchedThread* fs_find_running() noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].state == FsSchedState::Running) return &s_sched[i];
    }
    return nullptr;
}

static FsSchedThread* fs_find_next_runnable() noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].state == FsSchedState::Runnable) return &s_sched[i];
    }
    return nullptr;
}

static FsSchedThread* fs_find_blocked_ipc(uint32_t tid) noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].tid == tid &&
            s_sched[i].state == FsSchedState::BlockedIpcReceive)
            return &s_sched[i];
    }
    return nullptr;
}

// ── Scheduler public API ──────────────────────────────────────────────────────

extern "C" void fs_sched_register(uint32_t tid, uint64_t elr,
                                   uint64_t sp_el0, uint64_t spsr) noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].state == FsSchedState::Unused) {
            s_sched[i].tid           = tid;
            s_sched[i].state         = FsSchedState::Runnable;
            s_sched[i].resume_elr    = elr;
            s_sched[i].resume_sp_el0 = sp_el0;
            s_sched[i].resume_spsr   = spsr;
            s_sched[i].ipc_rsp_tva   = 0u;
            s_sched[i].ipc_rsp_size  = 0u;
            return;
        }
    }
}

extern "C" void fs_sched_mark_running(uint32_t tid) noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].tid == tid &&
            s_sched[i].state == FsSchedState::Runnable) {
            s_sched[i].state  = FsSchedState::Running;
            s_sched_running_tid = tid;
            return;
        }
    }
}

extern "C" void fs_obs_reset() noexcept;  // forward decl; defined below
extern "C" void fs_gov_reset() noexcept;  // forward decl; defined below

extern "C" void fs_sched_reset() noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        s_sched[i]          = FsSchedThread{};
        s_sched[i].l3_slot  = kNoThreadL3;  // RFC-00C6: default shared L3
    }
    s_sched_running_tid   = 0u;
    s_sched_ipc_delivered = false;
    fs_obs_reset();  // RFC-00BF: clear ring at start of each scheduler session
    fs_gov_reset();  // RFC-00C3: clear governance ring
}

// RFC-00C6: assign a per-thread L3 slot to a registered thread.
// Call after fs_sched_register() and before the first run_proc_entry().
extern "C" void fs_sched_set_thread_l3(uint32_t tid, uint32_t l3_slot) noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].tid == tid) {
            s_sched[i].l3_slot = l3_slot;
            return;
        }
    }
}

extern "C" bool fs_sched_ipc_delivered() noexcept {
    return s_sched_ipc_delivered;
}

// RFC-00C0: transition a BlockedDeviceWait thread to Runnable.
// Called from EL1 C code (outside any SVC handler) when the device event fires.
extern "C" void fs_sched_wake_device(uint32_t tid) noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].tid == tid &&
            s_sched[i].state == FsSchedState::BlockedDeviceWait) {
            s_sched[i].state = FsSchedState::Runnable;
            return;
        }
    }
}

// RFC-00C0: return the saved resume context for the given tid.
// Used by EL1 to construct a run_proc_entry() call after waking a device thread.
// RFC-00C3/C4: forward declaration — gov ring defined after obs ring below.
// Third param is device_id (RFC-00C4); 0 = not device-specific.
static void fs_gov_record(uint32_t tid, uint32_t event,
                           uint32_t device_id) noexcept;

// RFC-00C2/C4: wake BlockedDeviceWait threads matching intid on a timer tick.
// Called from axion_irq_handler_aarch64() with the fired INTID — async-signal-safe.
// RFC-00C4: threads with device_id==0 (any) or device_id==intid are woken;
//           others remain parked waiting for their specific INTID.
// RFC-00C3: records kGovTimerDeviceWake for each woken thread.
extern "C" void fs_sched_timer_device_wake(uint32_t intid) noexcept {
    for (uint32_t i = 0u; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].state == FsSchedState::BlockedDeviceWait) {
            const uint32_t did = s_sched[i].device_id;
            if (did == 0u || did == intid) {
                s_sched[i].state = FsSchedState::Runnable;
                fs_gov_record(s_sched[i].tid, 1u /*kGovTimerDeviceWake*/, intid);
            }
        }
    }
}

extern "C" bool fs_sched_get_resume(uint32_t tid,
                                     uint64_t* out_elr,
                                     uint64_t* out_sp) noexcept {
    for (int i = 0; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].tid == tid) {
            *out_elr = s_sched[i].resume_elr;
            *out_sp  = s_sched[i].resume_sp_el0;
            return true;
        }
    }
    return false;
}

// Called from ExitThread (SVC #2) handler in qemu_slice6_bridge_irq.cpp.
// Marks the current running thread as Exited and either:
//   • redirects the trap frame to the next Runnable thread (context switch), or
//   • redirects to g_axion_el1_return_pc to return to EL1.
// When the scheduler is not active (s_sched_running_tid == 0) this function
// behaves identically to the Phase 6–8 ExitThread handler.
extern "C" void fs_sched_exit_thread(void* frame_ptr) noexcept {
    auto* f = static_cast<FsBridgeTrapFrame*>(frame_ptr);

    // Mark current running thread Exited (if any).
    if (s_sched_running_tid != 0u) {
        FsSchedThread* cur = fs_find_running();
        if (cur) cur->state = FsSchedState::Exited;
        s_sched_running_tid = 0u;
    }

    // Find next Runnable thread.
    FsSchedThread* next = fs_find_next_runnable();
    if (next) {
        next->state           = FsSchedState::Running;
        s_sched_running_tid   = next->tid;
        s_current_el0_tid     = next->tid;
        // RFC-00C6: install next thread's private L3 before ERET.
        if (next->l3_slot != kNoThreadL3)
            el0_mmu_install_thread_l3(next->l3_slot);
        f->elr_el1            = next->resume_elr;
        f->sp_el0             = next->resume_sp_el0;
        f->spsr_el1           = next->resume_spsr;
    } else {
        // All threads done — restore shared L3 and return to EL1.
        el0_mmu_install_shared_l3();  // RFC-00C6
        f->elr_el1  = g_axion_el1_return_pc;
        f->spsr_el1 = 0x5u;  // EL1h, DAIF all unmasked
    }
}

// ── RFC-00BF: KernelCall observability ring ───────────────────────────────────
//
// 32-slot fixed-capacity ring.  Each slot holds one FsObsRecord (24 bytes).
// Written on every successful SVC #1 dispatch; never modified after write.
// s_obs_seq is the monotonic counter; slot = s_obs_seq % kObsRingCap.

struct FsObsRecord {
    uint32_t seq_id;     // record index (low 32 bits of s_obs_seq)
    uint32_t tid;        // caller tid at dispatch time
    uint32_t kind;       // KernelCallKind ordinal (frozen, RFC-00BD)
    uint32_t status;     // 0 = Ok, 1 = InvalidRequest
    uint32_t peer_tid;   // SendMessage: ipc_dst; else 0
    uint32_t _reserved;  // zeroed
};

static constexpr uint32_t kObsRingCap = 32u;
static FsObsRecord s_obs_ring[kObsRingCap];
static uint64_t    s_obs_seq = 0u;

static void fs_obs_record(uint32_t tid, uint32_t kind,
                           uint32_t status, uint32_t peer_tid) noexcept {
    const uint32_t slot = static_cast<uint32_t>(s_obs_seq % kObsRingCap);
    s_obs_ring[slot] = {
        static_cast<uint32_t>(s_obs_seq), tid, kind, status, peer_tid, 0u
    };
    ++s_obs_seq;
}

extern "C" void fs_obs_reset() noexcept {
    for (uint32_t i = 0u; i < kObsRingCap; ++i)
        s_obs_ring[i] = FsObsRecord{};
    s_obs_seq = 0u;
}

extern "C" uint64_t fs_obs_count() noexcept { return s_obs_seq; }

// Return true iff a record with status=Ok exists matching tid, kind, and
// (if peer_tid != 0) peer_tid.  Searches the live set (min(s_obs_seq, 32)).
extern "C" bool fs_obs_find(uint32_t tid, uint32_t kind,
                              uint32_t peer_tid) noexcept {
    const uint32_t count =
        s_obs_seq < kObsRingCap
            ? static_cast<uint32_t>(s_obs_seq)
            : kObsRingCap;
    for (uint32_t i = 0u; i < count; ++i) {
        const FsObsRecord& r = s_obs_ring[i];
        if (r.tid    == tid  &&
            r.kind   == kind &&
            r.status == 0u   &&
            (peer_tid == 0u || r.peer_tid == peer_tid))
            return true;
    }
    return false;
}

// ── RFC-00C3: Axion governance audit ring ─────────────────────────────────────
//
// 16-slot fixed-capacity ring recording async scheduler transitions.
// Complements the obs ring (SVC-driven events) with IRQ-driven events.
// obs_seq_at cross-references each gov event against the obs ring timeline.
//
// Event constants:
//   kGovTimerDeviceWake    = 1  — timer ISR: BlockedDeviceWait → Runnable
//   kGovAsyncContextSwitch = 2  — device_wait_loop: ERET to EL0 thread

static constexpr uint32_t kGovTimerDeviceWake    = 1u;
static constexpr uint32_t kGovAsyncContextSwitch = 2u;
static constexpr uint32_t kGovRingCap            = 16u;

struct FsGovRecord {
    uint32_t seq_id;      // monotonic gov counter (low 32 bits of s_gov_seq)
    uint32_t tid;         // thread being woken / switched to
    uint32_t event;       // kGovTimerDeviceWake or kGovAsyncContextSwitch
    uint32_t obs_seq_at;  // s_obs_seq value at time of event (logical timestamp)
    uint32_t device_id;   // RFC-00C4: INTID that triggered wake; 0 = N/A
    uint32_t _reserved;
};

static FsGovRecord s_gov_ring[kGovRingCap];
static uint64_t    s_gov_seq = 0u;

static void fs_gov_record(uint32_t tid, uint32_t event,
                           uint32_t device_id) noexcept {
    const uint32_t obs_at = static_cast<uint32_t>(s_obs_seq);
    FsGovRecord& slot = s_gov_ring[s_gov_seq % kGovRingCap];
    slot.seq_id     = static_cast<uint32_t>(s_gov_seq);
    slot.tid        = tid;
    slot.event      = event;
    slot.obs_seq_at = obs_at;
    slot.device_id  = device_id;   // RFC-00C4
    slot._reserved  = 0u;
    ++s_gov_seq;
}

extern "C" void fs_gov_reset() noexcept {
    for (uint32_t i = 0u; i < kGovRingCap; ++i) s_gov_ring[i] = FsGovRecord{};
    s_gov_seq = 0u;
}

extern "C" uint64_t fs_gov_count() noexcept { return s_gov_seq; }

extern "C" bool fs_gov_find(uint32_t tid, uint32_t event) noexcept {
    const uint32_t count = s_gov_seq < kGovRingCap
                               ? static_cast<uint32_t>(s_gov_seq)
                               : kGovRingCap;
    for (uint32_t i = 0u; i < count; ++i) {
        if (s_gov_ring[i].tid == tid && s_gov_ring[i].event == event)
            return true;
    }
    return false;
}

// RFC-00C4: find a gov record matching tid, event, AND device_id exactly.
extern "C" bool fs_gov_find_device(uint32_t tid, uint32_t event,
                                    uint32_t device_id) noexcept {
    const uint32_t count = s_gov_seq < kGovRingCap
                               ? static_cast<uint32_t>(s_gov_seq)
                               : kGovRingCap;
    for (uint32_t i = 0u; i < count; ++i) {
        if (s_gov_ring[i].tid       == tid       &&
            s_gov_ring[i].event     == event     &&
            s_gov_ring[i].device_id == device_id)
            return true;
    }
    return false;
}

// ── RFC-00C2: EL1 device-wait idle loop ──────────────────────────────────────
//
// Entered via ERET from the WaitForDevice SVC handler when no Runnable threads
// remain and g_axion_el1_device_wait_pc is set to this function's address.
// SPSR on entry = 0x5 (EL1h, DAIF = 0 = IRQs enabled).
//
// Spins with wfi until the GICv3 timer ISR calls fs_sched_timer_device_wake(),
// which transitions BlockedDeviceWait → Runnable.  After wfi returns the loop
// detects the Runnable thread and ERets to it at EL0.
extern "C" __attribute__((noinline)) void fs_sched_device_wait_loop() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    for (;;) {
        __asm__ volatile("wfi" ::: "memory");
        FsSchedThread* next = fs_find_next_runnable();
        if (next) {
            next->state         = FsSchedState::Running;
            s_sched_running_tid = next->tid;
            s_current_el0_tid   = next->tid;
            // RFC-00C3: record async context switch before ERET.
            fs_gov_record(next->tid, kGovAsyncContextSwitch, 0u);
            // RFC-00C6: install per-thread L3 before ERET (no-op if kNoThreadL3).
            if (next->l3_slot != kNoThreadL3)
                el0_mmu_install_thread_l3(next->l3_slot);
            const uint64_t elr = next->resume_elr;
            const uint64_t sp  = next->resume_sp_el0;
            __asm__ volatile(
                "msr elr_el1,  %[elr]\n\t"
                "msr sp_el0,   %[sp]\n\t"
                "mov x8, #0x3c0\n\t"
                "msr spsr_el1, x8\n\t"
                "isb\n\t"
                "eret\n\t"
                :
                : [elr] "r"(elr), [sp] "r"(sp)
                : "x8", "memory");
            __builtin_unreachable();
        }
    }
#endif
}

// ── Phase 8 IPC mailbox ───────────────────────────────────────────────────────
//
// Sequential single-slot mailbox — no concurrency; A and B run sequentially.
// SendMessage (kind=13): stores sender_tid, sets ready=true.
// ReceiveMessage (kind=14): if ready, writes sender_tid into response at
//   spawned_tid offset [36:40], sets delivered=true.
// canon_ipc_delivered(): polled by EL1 after B exits to confirm delivery.
// canon_ipc_reset(): called by EL1 before the IPC sequence begins.

struct IpcMailbox {
    uint32_t sender_tid;
    bool     ready;
    bool     delivered;
};
static IpcMailbox s_ipc_mailbox{0u, false, false};

extern "C" bool canon_ipc_delivered() noexcept {
    return s_ipc_mailbox.delivered;
}

extern "C" void canon_ipc_reset() noexcept {
    s_ipc_mailbox = IpcMailbox{0u, false, false};
}

// ── Response write helpers (byte-offset based, no struct alignment assumptions)

static inline void write_u16(uint8_t* b, uint32_t off, uint16_t v) noexcept {
    __builtin_memcpy(b + off, &v, 2);
}

static inline void write_u32(uint8_t* b, uint32_t off, uint32_t v) noexcept {
    __builtin_memcpy(b + off, &v, 4);
}

// Write a minimal InvalidRequest response into rsp[0..rsp_size).
// Caller must ensure rsp points to a valid writable range.
static void write_invalid_response(uint8_t* rsp, uint64_t rsp_size) noexcept {
    if (rsp_size < 12u) return;  // not enough room for even a minimal header
    const uint64_t cap = (rsp_size > 0xFFFFu) ? 0xFFFFu : rsp_size;
    __builtin_memset(rsp, 0, static_cast<__SIZE_TYPE__>(cap));
    write_u32(rsp,  0, kRspMagic);
    write_u16(rsp,  4, 1u);                              // version
    write_u16(rsp,  6, static_cast<uint16_t>(cap));      // bytes
    write_u32(rsp,  8, kStatusInvalidRequest);
}

// ── KernelCall SVC #1 dispatcher ─────────────────────────────────────────────
//
// Called from axion_kernel_handle_svc_trap_aarch64() in bridge_irq.cpp when
// ESR_EL1[15:0] == 1 (SVC #1 = KernelCall).
//
// ABI: x0=req_tva, x1=req_size, x2=rsp_tva, x3=rsp_size.
// Both TVAs must be within the EL0-mapped VA range (Phase 5 isolation).

extern "C" void el0_svc_kernel_call_dispatch(void* frame_ptr) noexcept {
    auto* f = static_cast<FsBridgeTrapFrame*>(frame_ptr);

    const uint64_t req_tva  = f->x[0];
    const uint64_t req_size = f->x[1];
    const uint64_t rsp_tva  = f->x[2];
    const uint64_t rsp_size = f->x[3];

    // Effective sizes: enforce protocol minimums so the TVA check below covers
    // at least the fields we need to read/write.
    const uint64_t eff_req = (req_size < kMinReqBytes) ? kMinReqBytes : req_size;
    const uint64_t eff_rsp = (rsp_size < kMinRspBytes) ? kMinRspBytes : rsp_size;

    // Phase 5 TVA validation — both pointers must lie entirely within the
    // EL0-mapped VA range (code page or stack page).  Reject any pointer
    // that could reach EL1 kernel address space.
    if (!el0_tva_valid(req_tva, eff_req)) goto deny;
    if (!el0_tva_valid(rsp_tva, eff_rsp)) goto deny;

    {
        const auto* req = reinterpret_cast<const uint8_t*>(
            static_cast<uintptr_t>(req_tva));
        auto* rsp = reinterpret_cast<uint8_t*>(
            static_cast<uintptr_t>(rsp_tva));

        // Validate request magic.  Any block that does not begin with KAQR is
        // silently rejected — no partial state is written to the response.
        uint32_t magic;
        __builtin_memcpy(&magic, req + 0, 4);
        if (magic != kReqMagic) goto deny;

        // Read the call kind from offset 8.
        uint32_t kind;
        __builtin_memcpy(&kind, req + 8, 4);

        // Zero the response buffer and write the common header fields.
        __builtin_memset(rsp, 0, static_cast<__SIZE_TYPE__>(rsp_size));
        write_u32(rsp,  0, kRspMagic);
        write_u16(rsp,  4, 1u);   // version
        write_u16(rsp,  6, static_cast<uint16_t>(
                               rsp_size < 0xFFFFu ? rsp_size : 0xFFFFu));
        write_u32(rsp,  8, kStatusOk);
        write_u32(rsp, 12, 0u);   // rejection = None

        switch (kind) {
            case kKindGetThreadIdentity:
                // Return the freestanding kernel thread identity (tid = 1).
                // caller_tid lives at byte offset 44 in the response block
                // (after header[8] + status[4] + rejection[4] + flags[8] +
                // 11 bool bytes + 1 alignment pad byte + spawned_tid[4] +
                // queried_tid[4] = 44).
                write_u32(rsp, 44, 1u);
                fs_obs_record(s_current_el0_tid, kKindGetThreadIdentity, kStatusOk, 0u);
                return;

            case kKindSendMessage: {
                // Phase 8 / Phase 9 IPC — store sender identity.
                // req[12:16] carries ipc_dst (target tid, uint32).
                uint32_t ipc_dst = 0u;
                if (req_size >= 16u) __builtin_memcpy(&ipc_dst, req + 12, 4);

                // Phase 9 (RFC-00BE): if a scheduler thread is blocking on
                // IpcReceive for this dst tid, deliver directly to its saved
                // response buffer (write sender_tid at offset 36).
                FsSchedThread* target = fs_find_blocked_ipc(ipc_dst);
                if (target && target->ipc_rsp_tva != 0u &&
                    target->ipc_rsp_size >= 40u) {
                    auto* trsp = reinterpret_cast<uint8_t*>(
                        static_cast<uintptr_t>(target->ipc_rsp_tva));
                    write_u32(trsp, 36, s_current_el0_tid);
                    target->state         = FsSchedState::Runnable;
                    s_sched_ipc_delivered = true;
                }

                // Phase 8 legacy mailbox (always updated for backward compat).
                s_ipc_mailbox.sender_tid = s_current_el0_tid;
                s_ipc_mailbox.ready      = true;
                s_ipc_mailbox.delivered  = false;
                fs_obs_record(s_current_el0_tid, kKindSendMessage, kStatusOk, ipc_dst);
                return;
            }

            case kKindReceiveMessage: {
                // Phase 8 IPC — if a message is pending, deliver it by writing
                // sender_tid at spawned_tid offset [36:40] in the response.
                // If no message is ready, return InvalidRequest.
                uint32_t recv_status;
                if (s_ipc_mailbox.ready) {
                    write_u32(rsp, 36, s_ipc_mailbox.sender_tid);
                    s_ipc_mailbox.delivered = true;
                    recv_status = kStatusOk;
                } else {
                    write_u32(rsp, 8, kStatusInvalidRequest);
                    recv_status = kStatusInvalidRequest;
                }
                fs_obs_record(s_current_el0_tid, kKindReceiveMessage, recv_status, 0u);
                return;
            }

            case kKindBlockOnIpcReceive: {
                // Phase 9 (RFC-00BE): park the calling thread and switch to
                // the next Runnable thread.  The common header (status=Ok) was
                // already written above; the waker will fill spawned_tid[36:40]
                // when it delivers.
                FsSchedThread* cur = fs_find_running();
                if (!cur) { write_u32(rsp, 8, kStatusInvalidRequest); return; }

                cur->resume_elr    = f->elr_el1;   // PC after svc #1
                cur->resume_sp_el0 = f->sp_el0;
                cur->resume_spsr   = f->spsr_el1;
                cur->ipc_rsp_tva   = rsp_tva;
                cur->ipc_rsp_size  = rsp_size;
                cur->state         = FsSchedState::BlockedIpcReceive;
                // Record before frame redirect: s_current_el0_tid is still the caller.
                fs_obs_record(s_current_el0_tid, kKindBlockOnIpcReceive, kStatusOk, 0u);

                FsSchedThread* next = fs_find_next_runnable();
                if (next) {
                    next->state         = FsSchedState::Running;
                    s_sched_running_tid = next->tid;
                    s_current_el0_tid   = next->tid;
                    f->elr_el1  = next->resume_elr;
                    f->sp_el0   = next->resume_sp_el0;
                    f->spsr_el1 = next->resume_spsr;
                } else {
                    s_sched_running_tid = 0u;
                    f->elr_el1  = g_axion_el1_return_pc;
                    f->spsr_el1 = 0x5u;  // EL1h
                }
                return;
            }

            case kKindWaitForDevice: {
                // Phase 9 (RFC-00BE): park the calling thread on a device event.
                // RFC-00C4: read device_id from req[12:16] if present (bytes≥16);
                // 0 means wake on any device (backward compatible with 12-byte req).
                FsSchedThread* cur = fs_find_running();
                if (!cur) { write_u32(rsp, 8, kStatusInvalidRequest); return; }

                uint32_t req_device_id = 0u;
                if (req_size >= 16u) __builtin_memcpy(&req_device_id, req + 12, 4);

                cur->resume_elr    = f->elr_el1;
                cur->resume_sp_el0 = f->sp_el0;
                cur->resume_spsr   = f->spsr_el1;
                cur->device_id     = req_device_id;  // RFC-00C4
                cur->state         = FsSchedState::BlockedDeviceWait;
                fs_obs_record(s_current_el0_tid, kKindWaitForDevice, kStatusOk, 0u);

                FsSchedThread* next = fs_find_next_runnable();
                if (next) {
                    next->state         = FsSchedState::Running;
                    s_sched_running_tid = next->tid;
                    s_current_el0_tid   = next->tid;
                    // RFC-00C6: install next thread's private L3 before ERET.
                    if (next->l3_slot != kNoThreadL3)
                        el0_mmu_install_thread_l3(next->l3_slot);
                    f->elr_el1  = next->resume_elr;
                    f->sp_el0   = next->resume_sp_el0;
                    f->spsr_el1 = next->resume_spsr;
                } else {
                    s_sched_running_tid = 0u;
                    // RFC-00C2: if the device-wait loop is installed, redirect
                    // ERET there (EL1 wfi loop awaiting timer IRQ wake).
                    // Otherwise fall back to immediate EL1 return (Phase 11).
                    f->elr_el1  = (g_axion_el1_device_wait_pc != 0u)
                                      ? g_axion_el1_device_wait_pc
                                      : g_axion_el1_return_pc;
                    f->spsr_el1 = 0x5u;  // EL1h, DAIF unmasked (IRQs enabled)
                }
                return;
            }

            default:
                // Unsupported kind in the Phase 6–9 freestanding subset.
                write_u32(rsp, 8, kStatusInvalidRequest);
                fs_obs_record(s_current_el0_tid, kind, kStatusInvalidRequest, 0u);
                return;
        }
    }

deny:
    // Write an InvalidRequest response if the response TVA itself is EL0-valid.
    // If rsp_tva is also invalid we cannot write anything — silently drop.
    if (rsp_size >= 12u && el0_tva_valid(rsp_tva, 12u)) {
        write_invalid_response(
            reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(rsp_tva)),
            rsp_size);
    }
}
