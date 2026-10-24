# Reproducibility Specification & Evidence Manifest Format

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/REPRODUCIBILITY.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Approved Reproducibility Contract

---

## 1. Purpose

This document specifies the compact evidence manifest schema, environment metadata recording standards, and exact reproduction commands required to ensure that any independent engineer can re-run and verify Research Protocol 001 without external assistance or non-public context.

---

## 2. Evidence Preservation Strategy

To balance complete scientific auditability with repository cleanliness:

1. **Large Artifact Isolation:** Generated SQLite databases, temporary Wasm binaries, and raw execution logs MUST NOT be committed directly to git repositories.
2. **Compact Manifest Recording:** Every experimental run produces a single, compact canonical JSON evidence manifest (`manifest.json` under $10$ KB) recorded in the evidence directory.
3. **Detached Blob Referencing:** Heavy evidence artifacts (trace logs, payload blobs) are content-addressed via SHA256 / CanonHash-81 digests and stored in a designated local cache or object store (`tmpfs` or external artifact server).

---

## 3. Canonical Evidence Manifest Schema (`manifest.json`)

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Protocol001EvidenceManifest",
  "type": "object",
  "required": [
    "protocol_id",
    "experiment_timestamp_utc",
    "source_commit_sha",
    "system_arm",
    "environment",
    "inputs_digest",
    "outputs_digest",
    "metrics_summary",
    "reproduction_commands"
  ],
  "properties": {
    "protocol_id": { "type": "string", "enum": ["RESEARCH_PROTOCOL_001"] },
    "experiment_timestamp_utc": { "type": "string", "format": "date-time" },
    "source_commit_sha": { "type": "string", "pattern": "^[0-9a-f]{40}$" },
    "system_arm": {
      "type": "string",
      "enum": ["arm_i_conventional_python", "arm_ii_t81_substrate", "arm_iii_wasmtime_baseline"]
    },
    "environment": {
      "type": "object",
      "required": ["operating_system", "architecture", "cpu_model", "compiler_version", "dependency_versions"],
      "properties": {
        "operating_system": { "type": "string" },
        "architecture": { "type": "string", "enum": ["x86_64", "arm64"] },
        "cpu_model": { "type": "string" },
        "compiler_version": { "type": "string" },
        "dependency_versions": {
          "type": "object",
          "properties": {
            "wasmtime": { "type": "string" },
            "opa": { "type": "string" },
            "sqlite": { "type": "string" },
            "python": { "type": "string" },
            "cryptography": { "type": "string" }
          }
        }
      }
    },
    "inputs_digest": { "type": "string", "description": "SHA256 digest of corpus input files" },
    "outputs_digest": { "type": "string", "description": "SHA256 digest of canonical execution results" },
    "metrics_summary": {
      "type": "object",
      "required": ["sample_count", "latency_mean_ms", "latency_median_ms", "unauthorized_action_rate", "verification_pass_rate"],
      "properties": {
        "sample_count": { "type": "integer", "minimum": 30 },
        "latency_mean_ms": { "type": "number" },
        "latency_median_ms": { "type": "number" },
        "latency_stdev_ms": { "type": "number" },
        "unauthorized_action_rate": { "type": "number" },
        "verification_pass_rate": { "type": "number" }
      }
    },
    "test_failures_and_skipped": {
      "type": "array",
      "items": { "type": "string" }
    },
    "reproduction_commands": {
      "type": "array",
      "items": { "type": "string" }
    }
  }
}
```

---

## 4. Exact Reproduction Commands

An independent evaluator MUST be able to re-execute the experiment on a clean host system using the following exact command sequence:

### Step 1: Environment & Dependency Verification
```bash
# Verify system dependencies and Python 3.12 environment with cryptography PyPI package
python3 -m pip install cryptography
python3 -c "import cryptography; print(cryptography.__version__)"
cmake --version
ninja --version
```

### Step 2: Build Reference Candidates
```bash
# Build T81 Substrate (Arm ii) in Release mode
cmake -S . -B build_research -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_PROVENANCE_EXPERIMENT=ON \
  -DT81_STRICT_DETERMINISTIC_FLOAT=ON
cmake --build build_research --parallel

# Verify clean build output
ctest --test-dir build_research --output-on-failure -R "^deterministic_provenance_experiment$"
```

### Step 3: Execute Benchmark Protocol Harness
```bash
# Execute Research Protocol 001 benchmark harness (30 iterations per test case)
python3 experiments/research_001/run.py \
  --build-dir build_research \
  --output /tmp/research_001_results \
  --repetitions 30
```

### Step 4: Execute Independent Detached Verifier
```bash
# Verify exported evidence bundles using standalone verifier (no producer imports)
python3 experiments/research_001/verify.py \
  /tmp/research_001_results/manifest.json \
  --store /tmp/research_001_results/store
```
