# experimental/ternaryos

**Status:** Experimental — non-DCP, not governance-gated.
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
**RFC:** [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)

Prototype implementation of TernOS Phase 1: the Hardware Abstraction Layer (HAL)
that bridges binary host hardware to the T81VM ternary runtime.

## Structure

```
hal/
  hal.hpp              HAL public interface (MemoryRegion, HardwareInterrupt,
                       BootContext, hal_main)
  hal_main.cpp         Ethics-first boot (Θ₁–Θ₉ via Axion) → T81VM handoff
  interrupt_table.cpp  Shadow binary interrupt dispatch table
  hosted_stub.cpp      Hosted (macOS/Linux) simulation — stand-in for the
                       UEFI PE stub; used for unit testing without UEFI toolchain
tests/
  hal_boot_test.cpp    Unit tests: BootContext construction, ethics gate,
                       interrupt registration
```

## Build

Enable with `-DT81_ENABLE_TERNARYOS=ON` (requires `T81_BUILD_TESTS` for the test target).

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build --target t81_ternaryos_hal_boot_test
ctest --test-dir build -R ternaryos
```

## Promotion Path

When Phase 1 acceptance criteria are met (see RFC-00B0 §7), the HAL sources
will be promoted to `include/t81/hal/` and `src/hal/` and become CI-gated.
