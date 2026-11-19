# Ternary AI Primitives (`DualRail (P, N)` Memory, Proof-Carrying Diffs, & Block-Zero Skip)

Demonstrates three novel AI primitives enabled directly by balanced ternary's **orthogonal `(P, N)` bitplanes** (`0.25 B/trit`) and native **`0` ("don't care")** state:

1. **Zero-Float Verifiable Agent Memory (Ternary RAG):**
   - `10,000 × 4,096`-dim semantic retrieval at **`16×` smaller RAM** than FP32 (`10 MB` vs `160 MB`), **`~11.9×` faster** than NEON FP32 cosine, and **`+18.7` Recall@10 points higher** than 1-bit binary sign hashing (`70.0%` vs `51.3%`) because balanced ternary's `0` trit masks weak background noise instead of forcing noisy dimensions to `±1`.
   - Emits a deterministic `SHA3-512` commitment over the exact integer top-10 retrieval set.
2. **Proof-Carrying Ternary Model Diffs (`Axion` Flip-Budget Gate):**
   - Measures exact weight-flip distance between a `CanonFS` base weight matrix `W_base` and a hot-patched matrix `W_new` in 3 bitwise ops per 64 weights: `popcount((P_new ^ P_base) | (N_new ^ N_base))`.
   - Applies sparse XOR patches losslessly (`SHA3-512` verified) and enforces a fail-closed flip-budget policy (admits `0.5%` patch, rejects `2.0%` patch when `max_flip_pct = 1.0%`).
3. **Block-Structured Activation-Sparsity Zero-Skipping (`1×4096×4096`):**
   - Pre-extracts active 128-trit activation blocks once per token (`O(32)`) and streams only active weight blocks branchlessly across `N = 4096` rows (`0.99×` at `0%` zero, `1.77×` at `50%`, `5.13×` at `87.5%` / 8-expert MoE, `10.09×` at `93.8%` / 16-expert MoE).
   - *Caveat:* Applies to block-structured sparsity (MoE / grouped channels), not uniform unstructured random sparsity where every 128-trit block contains a non-zero element.
4. **Zero-Dequantization Saturated Ternary Adapter Composition + `SBDR1` Wire Format (`t81.adapter_apply` & `bench_policy_and_adapters.cpp`):**
   - Composes a ternary adapter `Δ ∈ {-1, 0, +1}` directly onto packed Dual-Rail weights via exact 6-op bitwise saturated addition `out_p = (wp & ~dn) | (dp & ~wn)`, `out_n = (wn & ~dp) | (dn & ~wp)` while counting exact flips in the same pass (`5.35×` vs NEON `int8_t` at 1M weights, `3.94×` at 16M DRAM-bound; `64–77 B weights/s` on Apple M2).
   - Supports canonical **`SBDR1` (Sparse-Block Dual-Rail v1)** wire format (`"SBDR1\0\0\0"` + `total_words` + 1-bit-per-block active mask + only non-zero `(pos, neg)` word pairs): beats `Base-243` (`1.60 bits/trit`) in both size and speed above `25%` block-zero sparsity, reaching **`0.12 bits/trit`** (`13.3×` smaller than `Base-243`, `16.7×` smaller than dense Dual-Rail) and `10×–29×` faster decode at `95%` sparsity, with direct zero-decompression apply in `t81.adapter_apply`.
5. **Kleene 3-Valued Policy Composition & Tri-State Revocation Filter (`bench_policy_and_adapters.cpp`):**
   - Vectorized Kleene strong conjunction (`ALLOW=+1`, `UNKNOWN=0`, `DENY=-1` where `DENY` overrides `UNKNOWN`) plus single-pass conflict mask (`5.92×` vs NEON `int8_t` at 1M rules, `~87 B rules/s` on Apple M2).
   - 256 KB Dual-Rail Tri-State Bloom filter (`0/5000` revoked false negatives, fail-closed, `~7.2 ns/query` = `138 M checks/s`).
   - *Parked negative / mixed findings:*
     - **Ternary KV-Cache Attention:** Standalone 1-stage ternary `Q·K^T` at `D=128` suffers integer tie collisions (`18.2%–40.6%` Recall@10); 2-stage ternary top-64 pre-filter → FP32 rerank reaches `85.6%` vs `57.0%` binary on sparse needles, parked pending real LLM KV traces.
     - **Ternary Hyperdimensional Computing (VSA, `D=10,240`):** Dual-Rail cleanup is `3.23×` faster than NEON `int8_t`, but against a fair 50/50 tie-broken 1-bit Binary VSA baseline, dense random capacity is tied (`150/150` exact, `242/250` at `K=250`) while binary uses `2×` less RAM; parked unless wildcard-`0` partial queries are needed.

## Quick Start

```bash
./examples/research/ternary-ai-primitives/run_primitives.sh
```
