# MCP serve path confinement and WeightsLoad fail-closed

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [MCP serve path confinement and WeightsLoad fail-closed](#mcp-serve-path-confinement-and-weightsload-fail-closed)
  - [`t81 mcp serve`](#`t81-mcp-serve`)
  - [Governed `WeightsLoad`](#governed-`weightsload`)

<!-- T81-TOC:END -->


This note covers caller-visible changes in `t81 mcp serve` and governed `WeightsLoad`. Ungoverned VMs, frozen opcode numbers, and frozen spec text are unchanged.

## `t81 mcp serve`

File tools resolve `..` and symlinks, then accept a path only when the result is inside a `--root` directory. With no `--root`, the working directory is the only root. Repeat `--root` to allow more than one tree. Relative paths resolve under the first root. Absolute paths may land in any configured root.

Refused:

- a path whose canonical result leaves every root, including `..`, an absolute path, or a symlink that points out
- a sibling directory that only shares a string prefix with a root
- an embedded NUL

Percent-encoded dots and slashes (`%2e`, `%2f`) stay literal name characters. A file whose name is literally `%2e%2e` and that lives inside a root is still readable. Encoding is not decoded into traversal.

Reads stop at 64 MiB.

`t81.run_subgraph` and `t81.tx_commit` require `policy_path`. A commit that omits the policy does not discard the transaction backup. `t81.tx_abort` still restores it.

```text
t81 mcp serve --root /srv/agent-workspace
```

Callers that passed absolute paths outside the working directory now pass those directories as `--root`.

## Governed `WeightsLoad`

A VM with no policy still uses `AllowAllEngine`. `WeightsLoad` on that path is unchanged.

When a `PolicyEngine` is active, `WeightsLoad` is a `SecurityFault` unless the tensor name is an exact entry of `allowed-tensor-hashes` or `allowed-ternary-model-hashes`. These do not admit the load:

- an empty allowlist
- an empty tensor name
- a different spelling or case
- a model checksum that is not itself the tensor name

`allowed-ternary-model-hashes` still gates later model-backed ops (`WLOAD`, `TMatMul`, and the other ternary-native consumers) by model checksum. A policy that previously listed only the checksum must also list each tensor name:

```text
(policy
  (tier 2)
  (allowed-tensor-hashes ["mat_a" "mat_b"])
  (allowed-ternary-model-hashes ["sha3-512:<model checksum>"]))
```

The name entry admits `WeightsLoad`. The checksum entry still admits or denies the later op. Listing the checksum alone used to let `WeightsLoad` proceed. It no longer does.
