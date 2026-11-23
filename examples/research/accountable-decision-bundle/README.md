# RFC-00D4 Accountable Decision Bundle Experiment

This experiment turns [RFC-00D4](../../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) into an executable prototype on top of T81's bounded AI OS-object family (`assess-fixed`), Axion `.apl` policy gating, CanonFS content-addressed storage, and detached Ed25519 signatures.

## What It Demonstrates

1. **All 6 Accountable Computation Primitives:** Maps `t81 ai task assess-fixed` outputs, `.apl` policy hashes, `t81_reference_vm` method metadata, and an Ed25519 signature over canonical JSON (`meaning + evidence + authority`) into an `RFC-00D4` (`1.0.0-draft`) bundle.
2. **Offline Two-Node Verification:** An independent verifier (`adb_tool.py verify`) checks schema, policy hash, Ed25519 signature, and deterministic `assess-fixed` replay against an isolated CanonFS root.
3. **Fail-Closed Adversarial Guarantees:**
   - Mid-flight policy revocation rejects both the bundle and VM execution (`SecurityFault`).
   - 1-byte bundle tampering fails Ed25519 verification.
   - 1-byte CanonFS `.blk` tampering fails `t81 canonfs fsck --verify-hashes`.

## Run

```bash
bash examples/research/accountable-decision-bundle/run_adb_experiment.sh
```

## Explicit Scope & Limits

- `RFC-00D4` remains `1.0.0-draft`; signing uses `openssl pkeyutl` (Ed25519) in the prototype harness rather than a built-in `crypto/` key-management subsystem.
- Replay uses the bounded `assess-fixed` reference task, not general-purpose LLM inference.
