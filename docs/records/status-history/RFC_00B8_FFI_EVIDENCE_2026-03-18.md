# RFC-00B8 FFI Evidence (2026-03-18)

Host:
- Apple M2 / macOS ARM64
- build directory: `build`

Verified surfaces:
- RFC-0036 language/frontend acceptance:
  - `./build/t81lang_foreign_ffi_test`
- ISA emission / symbol preservation:
  - `./build/t81_isa_binary_emitter_test`
- VM bridge execution:
  - `./build/vm_ffi_bridge_test`
  - `ctest --test-dir build -R 'vm_ffi_bridge_test' --output-on-failure`

Bridge evidence covered by `vm_ffi_bridge_test`:
- encoded function-name symbol preservation into `FFICall`
- register-backed `FFIRegister`
- success and failure traps through the VM opcode path
- audit-trail emission on successful FFI calls
- fail-closed quarantine behavior
- scalar integer calls:
  - `() -> uint64_t`
  - `(int64_t) -> int64_t`
  - `(int64_t, int64_t) -> int64_t`
- string transport:
  - `(string) -> int64_t`
  - `() -> string`
- float transport:
  - `(double) -> double`
  - `() -> double`
- bytes transport with embedded NULs:
  - `(bytes) -> int64_t`
  - `() -> bytes`
- ordered heterogeneous arguments:
  - `(int64_t, string) -> int64_t`
- initial structured ABI:
  - `() -> string[]`
  - `(string[]) -> int64_t`

Real external-library evidence:
- system library no-arg integer call:
  - `getpid` (or platform equivalent)
- system library unary integer call:
  - `llabs` (or platform equivalent fallback)
- system library unary string call:
  - `strlen` / `lstrlenA`
- system library unary double call:
  - `fabs` on non-Windows hosts

Conclusion:
- RFC-00B8 acceptance criteria are met in-repo.
- Remaining work is stable-promotion hardening, not acceptance blocking:
  - broader structured schemas
  - more ecosystem bindings
  - explicit sandbox/isolation decision for quarantined FFI
