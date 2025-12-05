# Runtime / VM Observability Manual

This manual describes the runtime internals you can observe to understand Axion’s determinism guarantees. It covers the HanoiVM state, `State::axion_log`, the segment instrumentation (stack/heap/tensor/meta), guard/enum payload events, and how regressions expose faults so you can audit behavior without deep debugging.

## 1. HanoiVM state & Axion hooks

- The VM keeps a `t81::vm::State` containing registers, stack/heap allocators, tensors, and Axion data (see `include/t81/vm/state.hpp` & `include/t81/axion/context.hpp`).  
- Axion instrumentation records each event via `record_axion_event` (e.g., when `EnumIsVariant` runs), storing it in `State::axion_log`. Each entry includes opcode/tag/`verdict.reason`, and guards like `AxRead`/`AxSet` capture segment names/addresses.
- `State::axion_log` is accessible through the REPL `:trace` command, `t81 run --axion-log`, and helper programs like `axion_policy_runner`.

## 2. Segment visibility

Each stack, heap, tensor, and meta transition emits a deterministic string (RFC-0013):

| Event | Example reason |
| --- | --- |
| Stack alloc | `stack frame allocated stack addr=243 size=16` |
| Stack free | `stack frame freed stack addr=243 size=16` |
| Heap alloc | `heap block allocated heap addr=512 size=64` |
| Heap free | `heap block freed heap addr=512 size=64` |
| Tensor slot | `tensor slot allocated tensor addr=3` |
| Meta slot | `meta slot axion event segment=meta addr=1283` |
| AxRead guard | `AxRead guard segment=stack addr=5` |
| AxSet guard | `AxSet guard segment=heap addr=42` |
| Bounds fault | `bounds fault segment=stack addr=999 action=stack frame allocate` |

The new `canonfs_axion_trace_test` ensures CanonFS writes/logs emit the same meta slot strings plus `action=Write`/`action=Read`, giving policy authors a deterministically reproducible snippet. This test now targets the disk-backed driver, so the Axion trace proves that bytes first triggered the canonical meta slot event before landing in `objects/<hash>.blk`.

Regressions such as `axion_segment_trace_test` and `vm_memory_test` assert those strings exist and print them (CI artifact). The manual `scripts/capture-axion-trace.sh` builds `axion_policy_runner` to produce the same log file.

## 3. Guard & enum payload instrumentation

- `EnumIsVariant` emits `enum guard enum=<name> variant=<name> [payload=<type>] match=pass|fail`.
- `EnumUnwrapPayload` emits `enum payload enum=<name> variant=<name> payload=<type>`.
- These events carry variant ids decoded from `t81::enum_meta`; if metadata is missing the strings omit the names but still include `variant=<id>`.
- `match guard` strings feed `(require-match-guard ...)` policies; the policy engine rejects if the log lacks the exact string before the privileged opcode executes.

## 4. Loop & Axion policy metadata

- `format_loop_metadata` adds entries per RFC-0009, such as `loop hint file=... line=... column=... bound=... annotated true`.
- `t81 compile --verbose` prints these loop strings before Axion validates loop guards when a policy uses `(require-loop-hint ...)`.
- The REPL `:rules` command shows the parsed policy invoked by the running program, so you can verify the required loop IDs match your source annotations.

## 5. Observing runtime artifacts

1. **REPL `:trace`** – dumps `State::axion_log`. Use `:trace chart` to view segment timing and `:trace filter guard` to highlight guard events.  
2. **`t81 run --axion-log`** – automatically saves `build/artifacts/axion-<run>.log` with all Axion events.  
3. **`axion_policy_runner`** – writes the log required by RFC-0013/0009 (stack, heap, tensor, meta, AxRead/AxSet).  
4. **`tests/cpp/axion_enum_guard_test.cpp` and `axion_match_metadata_test.cpp`** – show how metadata flows through the VM and how Axion logs can be validated inside regressions.

## 6. Fault tracing

- When a policy requirement fails (`Trap::SecurityFault`), the VM dumps the Axion log with the last few `verdict.reason` strings so you know which required string was missing.  
- `vm_memory_test` intentionally causes bounds faults to ensure `bounds fault segment=...` strings appear in `State::axion_log`, capturing them for CI releases.  
- `t81 compile` with `--trace-guards` shows guard instrumentation even if policies do not require it, mirroring `axion_policy_trace`.

## 7. Linking observability to RFCs

- RFC-0013 defines the segment traces (stack/heap/tensor/meta) you can observe.  
- RFC-0019 dictates the enum/match metadata strings and how they map to `(require-match-guard ...)` policies.  
- RFC-0009 ensures the policy engine enforces the strings before privileged instructions run.  
- `spec/axion-kernel.md` §1.5‑1.10 and §2.1 describe how Axion uses the traces for DTS/VS; keep those sections handy as you audit `State::axion_log`.

## 8. Release artifact checklist

When packaging a release, include:  
1. The Axion log (`build/artifacts/axion_policy_runner.log`, `axion_policy_trace` output).  
2. The `ctest -R axion_segment_trace_test` snippet showing segment and bounds fault strings.  
3. The `t81 compile --verbose` transcript verifying guard strings exist (matches `policy/guards.axion`).  
These artifacts complement the manuals already referenced in `docs/guides/cli-user-manual.md`, `axion-trace` references, and the policy manual.
