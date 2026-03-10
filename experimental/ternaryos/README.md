# experimental/ternaryos

**Status:** Experimental — non-DCP, not governance-gated.
**Progress:** [PROGRESS.md](PROGRESS.md) ← start here
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
**RFC-00B0 (HAL):** [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)
**RFC-00B1 (MMU):** [docs/rfcs/RFC-00B1-ternary-mmu.md](../../docs/rfcs/RFC-00B1-ternary-mmu.md)
**RFC-00B2 (Drivers):** [docs/rfcs/RFC-00B2-device-drivers.md](../../docs/rfcs/RFC-00B2-device-drivers.md)

Prototype implementation of TernOS — a ternary-native OS kernel for the T81VM
runtime. Phases 1 through 3 are complete, and Phase 4 device-driver work is in
progress.

## Structure

```
hal/
  hal.hpp              HAL public interface (MemoryRegion, HardwareInterrupt,
                       BootContext, hal_main)
  hal_main.cpp         Ethics-first boot (Θ₁–Θ₉ via Axion) → T81VM handoff
  interrupt_table.cpp  Shadow binary interrupt dispatch table
  hosted_stub.cpp      Hosted (macOS/Linux) simulation — stand-in for UEFI stub
  virtualbox_platform.hpp/.cpp  First-target VirtualBox VM profile scaffold
  virtualbox_guest_devices.hpp/.cpp  VirtualBox profile-to-device binding helpers

mmu/
  tva.hpp              Ternary Virtual Address: base-3 uint64_t, VPN + offset,
                       kPageSize=59049, kMaxTva=3^30-1, trit utilities
  ternary_page_alloc.hpp/.cpp  Physical page allocator (balanced-ternary PageState)
  page_table.hpp/.cpp  Flat VPN→physical page map; mmu_map/translate/unmap

sched/
  tisc_context.hpp     TiscContext: full TISC thread snapshot for pre-emption
  context_switch.hpp/.cpp  context_save / context_restore / context_yield
  run_queue.hpp/.cpp   81-slot deterministic run queue
  scheduler.hpp/.cpp   Round-robin scheduler over TISC contexts

ipc/
  canon_message.hpp/.cpp  CanonRef-safe FIFO message bus

dev/
  block_device.hpp     CanonBlock-aligned block device interface
  hosted_block_dev.hpp/.cpp  File-backed hosted block device
  virtualbox_ahci_dev.hpp/.cpp  VirtualBox-first AHCI adapter scaffold
  virtualbox_e1000_dev.hpp/.cpp  VirtualBox-first E1000 adapter scaffold
  canon_store.hpp/.cpp Content-addressed CanonBlock store + reboot rebuild
  framebuffer.hpp/.cpp 81x27 ternary framebuffer with ASCII dump
  ttf.hpp/.cpp         Minimal ASCII ↔ balanced-ternary text codec + renderer
  net_packet.hpp       Ternary Ethernet packet wrapper + binary frame codec

tests/
  hal_boot_test.cpp          Phase 1 — 71 assertions
  ternary_page_alloc_test.cpp Phase 1 — 28 assertions
  context_switch_test.cpp    Phase 1 — 43 assertions
  mmu_test.cpp               Phase 2 — 47 assertions
  scheduler_test.cpp         Phase 3 — 120 assertions
  ipc_test.cpp               Phase 3 — 73 assertions
  device_driver_test.cpp     Phase 4 — 138 assertions
```

## Build & Test

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -R ternaryos -V
# Expected: 517/517 assertions, 7/7 tests pass
```

## Demo

For a short presentation-oriented walkthrough:

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build --target t81_ternaryos_demo
./build/t81_ternaryos_demo
```

The demo shows a VirtualBox-first hosted simulation path:

- the HAL boots a first-target VirtualBox guest profile
- the guest profile binds its first storage and network devices through AHCI and E1000 wrappers
- CanonStore persists a CanonBlock across a simulated reboot through that binding
- TTF renders ASCII text into the ternary framebuffer.
- TernaryEthernetPacket round-trips through the VirtualBox E1000 scaffold.

## Promotion Path

Each layer graduates from `experimental/ternaryos/` to the mainline when its
RFC acceptance criteria are met (see `PROGRESS.md` promotion checklist).
Promoted sources move to `include/t81/`, `src/`, `runtime/` and become CI-gated.
