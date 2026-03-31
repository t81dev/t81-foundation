(policy
  (name "GovernedCognitionAlpha")
  (tier 4)
  (max-instructions 10000000000)
  (max-recursion-depth 729)
  (max-stack 1024)
  (max-tensor-memory 8GB)
  (require-self-model-integrity true)
  (allowed-tensor-hashes [
    "sha3:4158a421be4b663b969a3028d72fa19a"
    "sha3:B12C09AF..."
  ])
  (capabilities [
    "net-outbound"
    "fs-read-only"
    "tier-4-reflection"
    "entropy-source"
  ])
  (require-reflection-cycle 10)
  (max-entropy-leakage 1000)
  (log-level deterministic-trace)
)
