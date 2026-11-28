# RFC-00D4 Companion Profile: Offline Two-Agent Receipt Handoff ("For AI, By AI")

**Status:** Draft Companion Profile (built on accepted [`spec/rfcs/RFC-00D4-accountable-decision-bundle.md`](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md))  
**Baseline:** `T81 Foundation v2.3.0` (`DpeMcpCoordinator`, immutable `axion_root.apl`, `CanonFS`, `t81 receipt export`, RFC 8032 / Wycheproof Ed25519 `verify_dag`)  
**Public C++ API / ISA Impact:** None (`include/t81/**/*.hpp`, `PUBLIC_API_LOCK.json`, and frozen `TISC v1.9.0-Stable` unchanged)

---

## 1. Purpose

When **Agent A** executes a bounded side-effecting step (code edit, artifact transform, or model ingest) and delegates downstream work to **Agent B**, prose summaries and commit messages do not establish cryptographic provenance or policy compliance.

This profile defines an **offline, machine-verifiable two-agent receipt handoff** using only implemented `v2.3.0` primitives:

1. **Agent A (Producer):** Executes inside an ephemeral `t81 mcp serve --root <workspace>` jail governed by an immutable (`0444`) `axion_root.apl` policy, commits via `t81.tx_begin` / `t81.tx_commit`, and exports an atomic forensic bundle via `t81 receipt export` alongside a `handoff_envelope.json` pinning `expected_policy_sha3_512` and `state_c_ref`.
2. **Agent B (Consumer / Verifier):** Operates **100% offline** with no live MCP server or network access. Before accepting Agent A's output, Agent B verifies:
   - `manifest.sha3-512` digest integrity over `bundle.json` and `receipt_raw.json`,
   - Presence and validity of the RFC 8032 / Wycheproof Ed25519 signature over the exact canonical signing payload (`t81.verify_dag`),
   - Exact equality between the receipt's recorded `policy_sha3_512` and Agent B's pinned `expected_policy_sha3_512`, and
   - Exact equality between the committed artifact bytes and `state_c_ref`.

## 2. Fail-Closed Error Contract

Agent B **MUST** reject the handoff (exit non-zero, refuse downstream execution) on any violation:

| Condition | Fail-Closed Code |
|---|---|
| Missing `ed25519_signature_b64` or `public_key_pem` | `ERR_MISSING_SIGNATURE` |
| `manifest.sha3-512` mismatch on `bundle.json` or `receipt_raw.json` | `ERR_MANIFEST_DIGEST_MISMATCH` |
| Invalid or tampered Ed25519 signature / payload | `ERR_RECEIPT_VERIFY_FAILED` |
| Receipt `policy_sha3_512 != expected_policy_sha3_512` | `ERR_POLICY_DIGEST_MISMATCH` |
| Artifact file bytes `SHA3-512 != state_c_ref` | `ERR_ARTIFACT_DIGEST_MISMATCH` |

## 3. Handoff Envelope Schema (`t81.agent-handoff-envelope.v1`)

```json
{
  "schema": "t81.agent-handoff-envelope.v1",
  "producer_agent_id": "agent-a",
  "consumer_agent_id": "agent-b",
  "expected_policy_sha3_512": "sha3-512:<128-hex>",
  "state_a_ref": "sha3-512:<128-hex>",
  "state_c_ref": "sha3-512:<128-hex>",
  "receipt_canonfs_ref": "<64-hex>",
  "bundle_dir": "forensic_bundle",
  "artifact_file": "artifact.json"
}
```

## 4. Reference Implementation & Verification

See [`examples/agent-handoff/`](../../examples/agent-handoff/README.md):
- `handoff_demo.py`: end-to-end Agent A producer + Agent B offline verifier.
- `test_handoff.py`: automated tests proving valid signed handoff (`PASS`) and fail-closed rejection on missing signature, tampered payload, and mismatched policy digest (`FAIL-CLOSED`).

## 5. Scope Boundary

Content identity and signature validity establish cryptographic integrity and policy binding under the producer's key; they do **not** establish factual truth of AI-generated prose or replace human governance for merges, releases, or Protocol 001 activation.
