# RFC-00B9 User Environment Evidence (2026-03-18)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [RFC-00B9 User Environment Evidence (2026-03-18)](#rfc-00b9-user-environment-evidence-2026-03-18)

<!-- T81-TOC:END -->


Build mode:
- `Release`
- `-DT81_ENABLE_TERNARYOS=ON`

Verified targets:
- `t81_ternaryos_user_env_test`
- `t81_ternaryos_shell_session_test`

Commands:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DT81_ENABLE_TERNARYOS=ON`
- `cmake --build build --target t81_ternaryos_user_env_test t81_ternaryos_shell_session_test -j4`
- `ctest --test-dir build -R 't81_ternaryos_(user_env_test|shell_session_test)' --output-on-failure`

Observed result:
- `2/2` tests passed

Acceptance coverage:
- `t81_ternaryos_user_env_test` maps directly to RFC-00B9 AC-1 through AC-15:
  - boot ordering and boot hash
  - integrity failure path
  - monotonic session allocation
  - CanonFS session records
  - login deny path
  - logout drain
  - shell prompt and builtins
  - command history
  - `ShellExec` policy gate
  - TTY handoff for `studio` / `agent`
  - `ServiceSpawn` policy gate
  - unknown-capability refusal
  - shared REPL state
- `t81_ternaryos_shell_session_test` adds shell-session durability and typed command evidence.

Conclusion:
- RFC-00B9 acceptance criteria are met in the opt-in TernaryOS build.
- Remaining work is promotion hardening for default-on integration and broader runtime adoption.
