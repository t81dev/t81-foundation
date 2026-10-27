---
name: t81_policy_checker
description: Parse one local Axion policy and return native validation metadata through validate-policy.sh. Other policy-checker entry points remain unsafe and unsupported for agent exposure.
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 policy syntax validation

Use only `validate-policy.sh` for syntax/metadata inspection of an explicitly
selected local policy. Its fixed invocation is `t81 policy validate <path> --json`.
It parses policy data; it does not evaluate an operation's authorization, install
an execution policy, execute programs, or establish policy correctness.

```sh
/bin/bash skills/policy-checker/validate-policy.sh [--json] [--] <file.apl|file.axionb>
```

Supply exactly one existing readable regular file. `.apl` text and `.axionb`
compiled policies are supported. JSON output is always enabled; `--json` is an
optional boolean flag accepted once. Options must precede `--`; exactly one path
must follow that separator. Use `--` for filenames beginning with `-`. Other flags,
option values, extra positionals, and operations are rejected, including the old
unimplemented `--strict`. Native parse/deserialization errors propagate unchanged.
Wrapper usage/path errors exit 2; absent CLI exits 127; native validation exits 0
for parse success and 1 for parse failure. A deny-all policy can be syntactically
valid: parse success is not permission to execute anything.

Paths containing spaces, quotes, dollar signs, parentheses, backticks, and shell
metacharacters are literal argv data. Relative paths gain `./` before forwarding
so the native shared parser cannot treat them as flags. Quote paths when calling
from a shell; the wrapper cannot undo expansion performed by the caller's shell.
Agent integrations must pass argv directly rather than composing shell programs.

Input paths may refer to any readable regular file with a supported suffix,
including symlinks to regular files. There is no workspace containment, symlink
race protection, size/time limit, or host sandbox. A future integration must apply
its own file allowlist and resource limits. Treat the local CLI on trusted `PATH`,
the shell, inherited environment, and host as trusted dependencies.

The wrapper creates no directories or files and offers no output/store/network
flags. The selected native path reads and parses the policy and writes metadata
to stdout and diagnostics to stderr; it does not import objects or write
provenance. Caller redirection can write output files. This is a read-only
application path under those dependencies, not an OS-level no-write guarantee.

`check.sh`, `simulate.sh`, `test-rule.sh`, and `list-policies.sh` remain unchanged,
unsafe, and outside this reviewed boundary. Do not dispatch to them or treat their
historical advertised operations as implemented APIs. The other skill packages
are also outside this correction. See the
[boundary report](../../docs/research/SKILL_ARGUMENT_BOUNDARY_REMEDIATION.md) for
source pins, before/after results, threat model, and unresolved findings.
