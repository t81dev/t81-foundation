# Two-Node Replay Consensus via CanonFS

Demonstrates bit-identical state consensus between two independent nodes (`node_a` and `node_b`) without requiring network determinism (`RFC-00D0` boundary).

## Protocol

1. **Node A** compiles `exact_rational_lab.t81` to a `.tisc` artifact, imports it into `CanonFS_A`, verifies `CanonFS_A` with `canonfs fsck --verify-hashes`, exports the `.tisc` object over an untrusted transport directory, and records its `t81 determinism certify` receipt.
2. **Node B** imports the transported `.tisc` artifact into an independent `CanonFS_B`, verifies `CanonFS_B` with `canonfs fsck --verify-hashes`, extracts the canonical object via `canonfs get`, and runs `t81 determinism certify`.
3. **Consensus Check:** Both nodes assert identical `CanonRef` (`sha3-256:...`), `program_sha3_512`, `run1_output_sha3_512`, and `run1_trace_sha3_512`.

## Evidence Boundary

This example demonstrates **deterministic replay equivalence across independent CanonFS stores**. It does not implement `RFC-00D0` networking or claim network timing determinism.

## Run

```bash
chmod +x examples/research/replay-consensus/run_consensus.sh
./examples/research/replay-consensus/run_consensus.sh
```
