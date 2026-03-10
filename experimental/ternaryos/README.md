# experimental/ternaryos

**Status:** Experimental — non-DCP, not governance-gated.
**Progress:** [PROGRESS.md](PROGRESS.md) ← start here
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
**RFC-00B0 (HAL):** [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)
**RFC-00B1 (MMU):** [docs/rfcs/RFC-00B1-ternary-mmu.md](../../docs/rfcs/RFC-00B1-ternary-mmu.md)

Prototype implementation of TernOS — a ternary-native OS kernel for the T81VM
runtime. Phases 1 and 2 are complete. Phase 3 (pre-emptive scheduler) is next.

## Structure

```
hal/
  hal.hpp              HAL public interface (MemoryRegion, HardwareInterrupt,
                       BootContext, hal_main)
  hal_main.cpp         Ethics-first boot (Θ₁–Θ₉ via Axion) → T81VM handoff
  interrupt_table.cpp  Shadow binary interrupt dispatch table
  hosted_stub.cpp      Hosted (macOS/Linux) simulation — stand-in for UEFI stub

mmu/
  tva.hpp              Ternary Virtual Address: base-3 uint64_t, VPN + offset,
                       kPageSize=59049, kMaxTva=3^30-1, trit utilities
  ternary_page_alloc.hpp/.cpp  Physical page allocator (balanced-ternary PageState)
  page_table.hpp/.cpp  Flat VPN→physical page map; mmu_map/translate/unmap

sched/
  tisc_context.hpp     TiscContext: full TISC thread snapshot for pre-emption
  context_switch.hpp/.cpp  context_save / context_restore / context_yield

tests/
  hal_boot_test.cpp          Phase 1 — 9 assertions
  ternary_page_alloc_test.cpp Phase 1 — 28 assertions
  context_switch_test.cpp    Phase 1 — 43 assertions
  mmu_test.cpp               Phase 2 — 47 assertions
```

## Build & Test

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -R ternaryos -V
# Expected: 127/127 assertions, 4/4 tests pass
```

## Promotion Path

Each layer graduates from `experimental/ternaryos/` to the mainline when its
RFC acceptance criteria are met (see `PROGRESS.md` promotion checklist).
Promoted sources move to `include/t81/`, `src/`, `runtime/` and become CI-gated.
