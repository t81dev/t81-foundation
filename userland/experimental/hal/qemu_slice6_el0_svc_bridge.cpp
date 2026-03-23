// userland/experimental/hal/qemu_slice6_el0_svc_bridge.cpp
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

// KernelCallKind numeric values (Phase 6–8 subset, from kernel_abi.hpp):
//   GetThreadIdentity = 10 (Yield=0 … SpawnThreadFromEntryDescriptor=9)
//   ExitThread        = 12
//   SendMessage       = 13  (Phase 8 IPC)
//   ReceiveMessage    = 14  (Phase 8 IPC)
static constexpr uint32_t kKindGetThreadIdentity = 10u;
static constexpr uint32_t kKindSendMessage        = 13u;
static constexpr uint32_t kKindReceiveMessage     = 14u;

// Minimum response size covering all fields through caller_tid at [44:48].
static constexpr uint64_t kMinRspBytes = 48u;

// Minimum request size covering magic (4B) + version/bytes (4B) + kind (4B).
static constexpr uint64_t kMinReqBytes = 12u;

// Phase 5 TVA validator — defined in qemu_slice6_el0_mmu.cpp.
extern "C" int el0_tva_valid(uint64_t va, uint64_t size) noexcept;

// ── Per-thread identity tracker (Phase 7) ────────────────────────────────────
//
// el0_svc_set_current_tid() is called by canon_exec_load_and_run() /
// canon_ipc_load_and_run() before each ERET to EL0 so that GetThreadIdentity
// returns the correct tid (1=init, 2=process A, 3=process B).
static uint32_t s_current_el0_tid = 1u;

extern "C" void el0_svc_set_current_tid(uint32_t tid) noexcept {
    s_current_el0_tid = tid;
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
                return;

            case kKindSendMessage: {
                // Phase 8 IPC — store sender identity in the single-slot
                // mailbox.  req[12:16] carries ipc_dst (target tid); we only
                // need it for validation; the sender_tid is s_current_el0_tid.
                // Status Ok is already written in the common header above.
                s_ipc_mailbox.sender_tid = s_current_el0_tid;
                s_ipc_mailbox.ready      = true;
                s_ipc_mailbox.delivered  = false;
                return;
            }

            case kKindReceiveMessage: {
                // Phase 8 IPC — if a message is pending, deliver it by writing
                // sender_tid at spawned_tid offset [36:40] in the response.
                // If no message is ready, return InvalidRequest.
                if (s_ipc_mailbox.ready) {
                    write_u32(rsp, 36, s_ipc_mailbox.sender_tid);
                    s_ipc_mailbox.delivered = true;
                } else {
                    write_u32(rsp, 8, kStatusInvalidRequest);
                }
                return;
            }

            default:
                // Unsupported kind in the Phase 6–8 freestanding subset.
                // Write an InvalidRequest status and return — no EL1 state is
                // modified.
                write_u32(rsp, 8, kStatusInvalidRequest);
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
