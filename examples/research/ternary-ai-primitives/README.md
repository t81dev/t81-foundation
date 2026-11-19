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

## Quick Start

```bash
./examples/research/ternary-ai-primitives/run_primitives.sh
```
