# Two-Agent Offline Receipt Handoff (`RFC-00D4` Companion Profile)

Self-contained runnable example and test suite demonstrating **Agent A -> Agent B** offline cryptographic handoff using `t81 mcp serve`, immutable `axion_root.apl`, `t81 receipt export`, and offline `SHA3-512` + RFC 8032 Ed25519 verification.

## Run the Demo

```bash
python3 examples/agent-handoff/handoff_demo.py
```

## Run the Automated Verification Suite

Covers valid signed handoff (`PASS`) plus fail-closed rejection (`FAIL-CLOSED`) on:
1. Missing Ed25519 signature (`ERR_MISSING_SIGNATURE`)
2. 1-byte tampered receipt payload (`ERR_MANIFEST_DIGEST_MISMATCH` / `ERR_RECEIPT_VERIFY_FAILED`)
3. Mismatched `expected_policy_sha3_512` (`ERR_POLICY_DIGEST_MISMATCH`)

```bash
python3 -m unittest examples/agent-handoff/test_handoff.py -v
```

See [`docs/rfcs/RFC-00D4-agent-receipt-handoff.md`](../../docs/rfcs/RFC-00D4-agent-receipt-handoff.md) for the protocol specification.
