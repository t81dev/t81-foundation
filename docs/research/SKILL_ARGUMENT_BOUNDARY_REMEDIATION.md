# Single skill entry point: literal policy metadata arguments

This correction addresses one representative entry point's exposure to OP-SEC-01, OP-COMP-01/02, and OP-SEC-03 from [draft audit PR #612](https://github.com/t81dev/t81-foundation/pull/612). It replaces the delegation in `skills/policy-checker/validate-policy.sh` with a fixed argv adapter. It does not repair the other skill entry points, CLI production code, Axion semantics, canonical hypotheses, RFCs, or governance activation. [Draft PR #613](https://github.com/t81dev/t81-foundation/pull/613) remains separate and unchanged; this metadata operation does not use its execution-policy path.

## Inspection, selection, and trace

Read the audit and its complete 69-command compatibility matrix at `215fb5cba7d452f1c94a714446235619e92e7abe`, and inspected all 12 main wrappers and 54 delegators. Their original paths, dispatch strings, mutation statements, and SHA-256 identities are retained in [inspection.json](evidence/skill_policy_boundary/inspection.json). The audit describes the negative baseline; its historical conclusions are not replaced by this correction.

The selected entry advertised `validate-policy <policy_file> [--strict]`. It actually executed `check.sh`, which parsed two positionals, created `T81_CANONFS_ROOT`, selected `axion check` from its own basename, and evaluated a command string. Neither its intended `axion validate-policy` route nor `--strict` exists. The audit separately exercised the real `policy validate` operation successfully. Correcting one delegator to that fixed operation avoids changing the unsafe shared dispatcher or its other callers.

The corrected contract is [SKILL.md](../../skills/policy-checker/SKILL.md). The Bash adapter accepts exactly one readable regular `.apl` or `.axionb` file, optional boolean `--json` once, and `--` followed by exactly one path. Unsupported flags, values, suffixes, extra positionals, and operations exit 2 before CLI invocation. Missing CLI exits 127. Relative paths gain `./` to prevent native option parsing; absolute paths remain literal. It executes only the array `t81 policy validate "$policy_file" --json` and propagates the native exit status and output.

Native trace: `main` → shared `parse_args` → `run_policy` → `run_policy_validate` → text `parse_policy` or binary `Policy::deserialize` → JSON metadata. That handler reads and parses the policy; it does not create a VM, invoke the Axion authorization engine, import objects, write provenance, create output directories, or fetch data. A zero-instruction-budget policy is valid data here, not authorization to execute a program. Parser rejection returns 1. No execution-policy option is forwarded or implied.

Input paths may be anywhere the invoking OS user can read, and symlinks to regular files are accepted. There is no workspace containment or race protection. Output is stdout/stderr only; shell redirection by the caller can write files. The wrapper has no filesystem mutation operations. Source inspection and sampled before/after filesystem observations support a read-only application path, not a process-level no-write guarantee.

## Before/after compatibility matrix

| Case | Original selected wrapper / inherited dispatcher | Corrected selected wrapper |
| --- | --- | --- |
| One valid policy input | Exit 1, demands operation and resource; no validation metadata | Exit 0, native `t81.policy-validate.v1`, `valid: true` |
| Malformed/empty text or malformed compiled policy | Exit 1 at unrelated argument precondition | Native exit 1, `valid: false`, parse/deserialization error |
| Valid compiled policy | No intended operation reached | Exit 0 after deserialization |
| Spaces, literal quotes, `$HOME`, parentheses, command-substitution/backtick syntax, raw metacharacters | Advertised single-path form fails before backend | Original path preserved as one native positional argument (relative `./` prefix documented) |
| `import` plus substitution-shaped resource (malformed two-positional call) | Fake backend exits 0; wrong `axion check` dispatch, store and `printf` canary created; native backend also creates canary before rejecting route | Exit 2; no backend output, store creation, or canary |
| Unquoted substitution-shaped operation or quote-breakout resource | Inherited evaluation creates harmless canary and store; see exact outputs in before receipt | Exit 2 before backend; no persistent filesystem change |
| Unsupported flags, output/store requests, `--strict`, boolean values, extra positionals | Incompatible parsing, wrong route, or usage error; not the advertised syntax validation | Exit 2; explicit rejection |
| Missing/directory/broken-symlink/FIFO/unreadable input | Single-path form fails at unrelated precondition | Exit 2 without backend; FIFO rejection does not block |
| Missing backend | Single-path form fails before availability check | Exit 127 |
| Native status propagation | Not the intended path; string evaluation can change final status | Fixed exec propagates backend status; fake exit 23 retained |

Before and after use identical self-authored fixtures and test code. The two-positional canary probes are explicitly malformed calls, not claims that the original advertised one-path form reached evaluation. All payloads use harmless `printf` inside disposable scratch directories. Literal argument tests include quote characters inside filenames and raw metacharacters passed by argv. The wrapper cannot prevent substitution performed by a caller's shell before invocation; integrations must pass argv directly or quote their shell inputs correctly.

## Identities, reproduction, and actual results

- Base: `903ea2fe653df0d636e0543f9788dd8442738045`.
- Implementation/test source: `fae432ba22723f16b2f5f9eb11a91a7046b2fb7c`.
- Audit record: `215fb5cba7d452f1c94a714446235619e92e7abe`; original code baseline: `fabc7aa90f70ead227ec257a1ae6833fa16ced40`.
- Retained audit CLI binary: SHA-256 `d6a88ee8b445d09a1c93592c7b75323a590839b17d9427873a208050b5e38c0d`. It is reused explicitly, not described as a newly built or independently supplied binary. Relevant handler/parser/serialization files and original skill scripts are unchanged between the audit baseline and this branch's base; exact checks are retained with verification evidence.

Local environment: macOS 26.5.2 ARM64, Apple Bash 3.2.57, Python 3.14.3. Tests use isolated directories, controlled HOME/store roots, scrubbed shell startup/function hooks, and a PATH containing either a fixed argument-capture executable or the named real CLI. Trusted inherited host environment remains outside the wrapper's security boundary. Test fixtures include a source policy compiled to `.axionb` by a separately recorded setup command; that harness setup writes its scratch fixture and is not attributed to the read-only wrapper.

From `/private/tmp/t81-skill-boundary`:

```sh
python3 tests/python/test_skill_policy_boundary.py --bash /bin/bash \
  --wrapper /private/tmp/t81-operational-audit/skills/policy-checker/validate-policy.sh \
  --t81-bin /private/tmp/t81-operational-build/t81 --receipt /private/tmp/t81-skill-before.json
python3 tests/python/test_skill_policy_boundary.py --bash /bin/bash \
  --t81-bin /private/tmp/t81-operational-build/t81 --receipt /private/tmp/t81-skill-after.json
python3 tests/python/test_skill_policy_boundary.py --bash /bin/bash \
  --receipt /private/tmp/t81-skill-fake-after.json
```

The [before receipt](evidence/skill_policy_boundary/before.json) exits 1: 86 process receipts, 85 failing wrapper cases and 141 failed checks. The [after receipt](evidence/skill_policy_boundary/after.json) exits 0: one fixture compilation, 40 fake-backend calls, 44 real CLI calls, and one missing-backend call; zero failed checks and no observed persistent wrapper filesystem changes. Fake-only execution exits 0 with 41 calls and explicitly reports that the real CLI was not run. Fake success proves argument/dispatch behavior only. Real success proves the sampled native metadata operation, not authorization enforcement, policy soundness, or scientific replication.

Receipts retain exact argv, working directories, environment overrides, exit codes, stdout/stderr, filesystem changes, fixture text, compiled fixture hash, wrapper/test/binary hashes, and Bash/environment identification. They observe persistent path/content/mode changes under scratch, not transient writes, read access times, same-byte overwrites, or all host syscalls.

A fresh Release CLI build at the implementation pin succeeded (AppleClang 21.0.0.21000101, CMake 4.3.0, Ninja 1.13.2, offline local Asio 1.30.2). Its SHA-256 is identical to the retained audit binary above. [fresh-build.json](evidence/skill_policy_boundary/fresh-build.json) repeats the full matrix against that freshly built path: exit 0, 86 process receipts, no failures. Registered `t81_skill_policy_boundary_test` passes through CTest (1/1). [verification.json](evidence/skill_policy_boundary/verification.json) retains exact configure/build/CTest commands and outputs, compiler identification, source hashes, all 66 Bash syntax checks (pass), and byte-identity checks for the 65 unchanged wrappers.

CLI documentation parity passes (24 topics), as do root structure, active documentation links, overclaim guardrails, and determinism claims checks. Documentation structure exits 1 for the existing five missing `book/book-{en,cn,es,pt,ru}` directories. The skill-creator quick validator exits 1 on both original and corrected definitions because the existing OpenClaw identifier `t81_policy_checker` uses underscores; that identifier is preserved rather than renamed as part of this boundary fix. Neither failure is counted as a pass or repaired by changing unrelated contracts. The full repository CTest suite was not run.

The focused hosted workflow exercises fake-backend argument preservation on Linux Bash and macOS Bash independently of the repository's broader build. The UNIX CTest runs both fake and real tests when the CLI is built. Linux real CLI, native Windows, WSL, alternate shells, and OpenClaw gateway behavior have not been exercised locally. Hosted results will be reported separately with their commit pins; no queued CI is counted as a pass.

## Threat model and remaining unsafe interfaces

Protects: the selected adapter's own parsing/dispatch boundary against attacker-controlled argument text becoming shell code, word splitting, globbing, unintended CLI options, silent operation substitution, and wrapper-created store/output directories. Negative tests check persistent canary/store creation and unexpected backend calls. The accepted operation is fixed and cannot execute arbitrary host commands or programs.

Does not protect: caller-side shell expansion; malicious PATH/CLI/shell/startup environment; native parser vulnerabilities; arbitrary host file access permitted to the user; symlink/TOCTOU races; oversized input/resource exhaustion; output redirection; other skill entry points. Syntax validation is not policy authorization, a signature check, or a security certificate. Any future external-agent broker still needs explicit file permissions, resource controls, trusted process launch, and acceptance review.

All 65 other original shell entry points remain outside the approved exposure boundary. Their exact paths are explicitly enumerated as `remaining_unsafe_wrapper_paths` in [inspection.json](evidence/skill_policy_boundary/inspection.json). This includes all 12 unchanged `eval` main wrappers: `ai-tasks/assess.sh`, `bundle-creator/create-bundle.sh`, `bundle-validator/validate.sh`, `canonfs-export/export.sh`, `canonfs-import/import.sh`, `governance-demo/demo-governance.sh`, `model-loader/load-hf.sh`, `policy-checker/check.sh`, `t81-compiler/build.sh`, `t81-debugger/debug.sh`, `t81vm-runner/run.sh`, and `tensor-ops/tensor-math.sh`, plus their 53 remaining delegators. There is no package-wide approval, generic dispatcher, or permission to expose the skills directory.

Recommendation: generalize the fixed-operation argv pattern only one operation at a time, after tracing real handlers, side effects, native option parsing, authorization, errors, and platform behavior. Do not mechanically translate command strings to arrays while retaining nonexistent routes or lost-mode dispatch. Mutating operations need a separately reviewed authorization and output-path contract. Human review must assess this adapter's narrow contract, raw receipts, CI outcomes, and remaining limits before acceptance. Do not merge automatically.
