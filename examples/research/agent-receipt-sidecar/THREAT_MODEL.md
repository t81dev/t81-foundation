# Threat Model: T81 Agent Receipt & Policy Sidecar

**Scope:** [`t81_receipt_sidecar.py`](t81_receipt_sidecar.py) (`RFC-00D4` `1.0.0-draft` + Axion `.apl` + CanonFS `fsck --verify-hashes`).

## What This Sidecar Protects Against
1. **Unauthorized high-consequence tool execution under full context compromise:** Even if a calling LLM agent is completely prompt-injected, `t81.gate_and_seal` enforces `.apl` admission rules before side effects in `fail-closed` mode.
2. **Caller-initiated mode downgrade:** Enforcement mode (`observe-only` < `warn` < `fail-closed`) is owned by policy (`(enforcement-mode "...")`) or sidecar configuration (`T81_ENFORCEMENT_MODE`). A calling agent can tighten mode per-call, never downgrade it.
3. **Forged or tampered handoff receipts:** `t81.verify_receipt` verifies canonical `meaning + evidence + authority` bytes against an Ed25519 signature; 1-byte mutations fail closed.
4. **Shadow-receipt smuggling:** In `observe-only` or `warn` modes, denied actions still record their true `authority.verdict` (`DENY` or `WARN`) inside the signed receipt—never `ALLOW`—so a downstream `fail-closed` verifier rejects them.
5. **Mid-flight policy revocation (TOCTOU):** `t81.verify_receipt` compares `authority.policy_hash` and `model_weights_digest` against the Executor's current trusted `.apl` policy.
6. **Silent artifact corruption:** `t81.canonfs_fsck` (`t81 canonfs fsck --verify-hashes`) detects 1-byte mutations in sealed CanonFS `.blk` objects.

## What This Sidecar Does NOT Protect Against
1. **Exfiltration or abuse through already-allowed channels:** If a policy permits an action or tool, the sidecar does not inspect or prevent covert exfiltration inside allowed payloads.
2. **Deceptive agent reasoning:** An agent can still produce misleading natural-language explanations alongside a valid receipt.
3. **Sidecar host or signing-key compromise:** If an attacker compromises the sidecar process itself or steals the Ed25519 private key, forged receipts can be signed. (Hardware-backed key isolation / KMS integration is out of scope for this prototype.)
4. **Unverified external execution:** Fast-path `t81.verify_receipt` checks policy, signature, and CanonFS integrity in ~10 ms; proving that a specific model computation produced an output requires deterministic replay (e.g., `t81 ai task assess-fixed` or `t81 determinism certify`).
