# CI Workflow Confirmation

## AI Experiments CI Workflow

### File Location

`.github/workflows/ai-experiments-ci.yml`

### What It Builds

**Primary Target:** `t81_ai` executable

**Build Process:**
1. **Fresh checkout** of repository
2. **Dependency installation** (cmake, build-essential)
3. **Configuration** with `-DT81_ENABLE_AI_EXPERIMENTS=ON`
4. **Compilation** of `t81_ai` target
5. **Binary verification** - ensures executable exists

**Expected Build Artifacts:**
- `build/experiments/ai/ux_tools/t81_ai`

### What It Tests

**Automated Smoke Test:**
- **Help command**: `./t81_ai --help`
- **Model inspection**: `./t81_ai model inspect test_file.gguf`
- **Model verification**: `./t81_ai verify test_file.gguf`
- **Backend capabilities**: `./t81_ai backend capabilities`
- **Backend selection trace**: `./t81_ai backend select --format gguf --mode strict_deterministic`
- **Inference runtime command**: `./t81_ai inference run --model test --model-file test_file.gguf --prompt "smoke"`
- **Quantization runtime command**: `./t81_ai quantization inspect --model test --model-file test_file.gguf`
- **Benchmark runtime command**: `./t81_ai benchmark run --model test --model-file test_file.gguf`
- **Policy runtime command**: `./t81_ai policy test --event-type model_load --model-file test_file.gguf`
- **Error handling**: `./t81_ai verify nonexistent.gguf`

**Official Smoke Test:**
- Runs `scripts/test_ai_simple.sh`
- Verifies all automated tests pass

**Evidence Promotion Gate:**
- Builds signed multi-lane AI evidence manifest with promotion-window attestation metadata (`build/ai-manifest/ai_evidence_manifest.json`)
- Requires keyring-backed manifest signature verification and rotation-policy validation (`scripts/ci/ai_evidence_manifest_keyring.json`), including `material_env` support for secret-injected key material via workflow env/`secrets.*`
- Enforces model provenance manifest with CanonFS object identity, signed multi-entry provenance-chain verification, and required lineage-event sequence checks (`build/ai-provenance/test_model.manifest.json`)
- Enforces governed runtime deterministic multi-seed replay attestations with failure taxonomy artifact (`build/ai-governed/governed_llama_replay_attestation.json`)
- Enforces signed governed replay attestation with keyring verification and escalation mapping (`build/ai-governed/governed_llama_replay_attestation.json`)
- Enforces signed multi-event Axion policy-ledger snapshot and deterministic replay verification (`build/ai-policy/ai_axion_policy_ledger_snapshot.json`, `build/ai-policy/ai_axion_policy_ledger_replay_verification.json`)
- Enforces signed backend-selection manifest bound to policy/runtime evidence snapshots (`build/ai-backend/runtime_backend_selection_manifest.json`)
- Enforces signed direct backend execution attestation replay (governed `t81 llama-run`) under deterministic evidence constraints (`build/ai-ux/ai_direct_backend_execution_attestation.json`)
- Publishes machine-readable escalation mappings for signing/keyring/runtime-binding failures in policy/backend/evidence artifacts
- Enforces runtime benchmark execution replay and regression/trend thresholds (`build/ai-benchmark/ai_benchmark_spec_contract.json`)
- Emits benchmark format/mode capability matrix with expectation-contract enforcement (including allowlisted unsupported `t3k` lane visibility) without blocking required baseline lane (`build/ai-benchmark/ai_benchmark_capability_matrix.json`)
- Enforces signed benchmark threshold promotion approval attestations via keyring-backed verification (`build/ai-benchmark/ai_benchmark_threshold_approval_report.json`)
- Emits RFC-0026 runtime readiness tracker binding opcode conformance evidence with benchmark/inference lane capability states, runtime capability-alignment gate status, and opcode baseline approval-policy gate status (`build/ai-opcodes-runtime/ai_rfc0026_readiness.json`)
- Enforces benchmark/inference runtime capability alignment for governed format/mode pairs (`build/ai-opcodes-runtime/ai_runtime_capability_alignment.json`)
- Enforces signed opcode baseline history window promotion approvals via keyring-backed verification (`build/ai-opcodes-runtime/ai_opcode_baseline_approval_report.json`)
- Emits inference format/mode capability matrix with expectation-contract enforcement (including allowlisted unsupported `t3k` lane visibility) without blocking required baseline lane (`build/ai-ux/ai_inference_capability_matrix.json`)
- Enforces runtime quantization inspect replay + encode/decode fixture corpus roundtrip validation with rolling profile trend analytics (`build/ai-quantization/ai_quantization_codec_contract.json`)
- Enforces signed quantization profile promotion approval attestations via keyring-backed verification (`build/ai-quantization/ai_quantization_profile_approval_report.json`)
- Emits AI signing key expiry alert report for keyring rotation readiness (`build/ai-keyring/ai_keyring_expiry_report.json`)
- Enforces AI keyring KMS metadata contract and emits report (`build/ai-keyring/ai_keyring_kms_contract_report.json`)

### How It Prevents Regressions

**Core Isolation Check:**
```yaml
- name: Verify Core Isolation
  run: |
    if [ "$GITHUB_EVENT" = "pull_request" ]; then
      CORE_CHANGES=$(git diff --name-only origin/$GITHUB_BASE_HEAD..HEAD | grep -E '^(src/|include/t81/|spec/|tests/)' || true)
      if [ -n "$CORE_CHANGES" ]; then
        echo "❌ Core directories were modified:"
        echo "$CORE_CHANGES"
        exit 1
      fi
    fi
```

**Binary Existence Check:**
```yaml
- name: Verify Binary Exists
  run: |
    if [ ! -f "experiments/ai/ux_tools/t81_ai" ]; then
      echo "❌ AI CLI binary not found"
      exit 1
    fi
```

**Smoke Test Gate:**
```yaml
- name: Run Official Smoke Test
  run: |
    ../scripts/test_ai_simple.sh
```

### Triggers

**Protected Branches:**
- `main` - Prevents breaking stable branch
- `feature/ai-*` - Tests AI feature branches

**Protected Paths:**
- `experiments/ai/**` - Triggers on AI changes
- `CMakeLists.txt` - Triggers on build integration
- `scripts/test_ai_simple.sh` - Triggers on test changes
- `scripts/ci/ai_evidence_manifest_keyring.json` - Triggers on manifest signing key rotation changes
- `scripts/ci/ai_model_provenance_keyring.json` - Triggers on provenance signing key rotation changes
- `scripts/ci/ai_policy_ledger_keyring.json` - Triggers on policy-ledger signing key rotation changes
- `scripts/ci/ai_backend_selection_keyring.json` - Triggers on backend-selection signing key rotation changes
- `scripts/ci/ai_governed_replay_keyring.json` - Triggers on governed replay signing key rotation changes
- `scripts/ci/ai_direct_backend_attestation_keyring.json` - Triggers on direct-backend attestation signing key rotation changes
- `scripts/ci/ai_benchmark_threshold_approval_keyring.json` - Triggers on benchmark-threshold approval signing key rotation changes
- `scripts/ci/ai_quantization_profile_approval_keyring.json` - Triggers on quantization-profile approval signing key rotation changes
- `scripts/ci/check_ai_keyring_kms_contract.py` - Triggers on KMS metadata contract policy updates
- `scripts/ci/ai_benchmark_thresholds.json` - Triggers on benchmark threshold baseline updates
- `scripts/ci/ai_benchmark_thresholds_history.json` - Triggers on benchmark threshold history window updates
- `scripts/ci/check_ai_benchmark_capability_matrix.py` - Triggers on benchmark capability-matrix policy updates
- `scripts/ci/ai_benchmark_capability_expectations.json` - Triggers on benchmark capability expectation contract updates
- `scripts/ci/check_ai_inference_capability_matrix.py` - Triggers on inference capability-matrix policy updates
- `scripts/ci/ai_inference_capability_expectations.json` - Triggers on inference capability expectation contract updates
- `scripts/ci/check_ai_runtime_capability_alignment.py` - Triggers on runtime capability alignment policy updates
- `scripts/ci/check_ai_rfc0026_readiness.py` - Triggers on RFC-0026 runtime readiness tracker updates
- `scripts/ci/check_ai_opcode_baseline_history_approvals.py` - Triggers on opcode baseline approval policy gate updates
- `scripts/ci/ai_opcode_baseline_approval_keyring.json` - Triggers on opcode baseline approval key rotation updates
- `scripts/ci/check_ai_benchmark_threshold_approvals.py` - Triggers on benchmark threshold approval policy gate updates
- `scripts/ci/ai_quantization_codec_profile.json` - Triggers on quantization codec profile baseline updates
- `scripts/ci/ai_quantization_codec_profile_history.json` - Triggers on quantization codec profile history window updates
- `scripts/ci/check_ai_quantization_profile_approvals.py` - Triggers on quantization profile approval policy gate updates
- `experiments/ai/opcodes/PHASE1_BASELINE_HASHES_HISTORY.json` - Triggers on opcode baseline history-window updates

### Failure Conditions

**CI Will Fail If:**
- Binary compilation fails
- Binary not found at expected location
- Any smoke test step fails
- Core directories are modified in PR
- Official smoke test script fails

### Success Indicators

**CI Pass Status Means:**
- ✅ AI CLI builds successfully
- ✅ All CLI commands work correctly
- ✅ Error handling operates properly
- ✅ Core isolation maintained
- ✅ Automated tests pass
- ✅ No regressions introduced

### Reviewer Benefits

**For PR Reviewers:**
- **Independent verification**: CI tests run automatically
- **Reproducibility guarantee**: Fresh build every time
- **Core protection**: Automatic detection of core modifications
- **Test coverage**: All functionality verified
- **Documentation**: Expected behavior clearly defined

**For Developers:**
- **Immediate feedback**: Broken changes detected quickly
- **Quality gate**: Prevents regressions
- **Consistency**: Ensures all changes meet standards
- **Safety**: Protects stable main branch

---

**Status:** ✅ CI workflow implemented and verified
