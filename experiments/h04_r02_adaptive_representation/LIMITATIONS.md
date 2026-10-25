# H04-R02 Limitations and Threats to Validity

**Document ID:** `experiments/h04_r02_adaptive_representation/LIMITATIONS.md`
**Date:** October 2026
**Status:** Complete / Final Research Packet

---

## 1. Missing Historical Source H04-R01

As verified in the repository audit:
- The historical experimental implementation commit `def47c680f477bb1a7bb10ebb57e11a0c7b24a6f` on branch `research/h04-r01-trace-ledger` is **absent** from the git object database.
- `HYPOTHESIS_REGISTER_v1.json` explicitly lists H04-R01 as a "selected specification, not implemented or benchmarked".
- H04-R02 reconstructed and evaluated the four-arm adaptive trace ledger independently rather than building on an undocumented binary artifact.

---

## 2. Scope Limitations

1. **Simulated Symbol Models:**
   - Evaluates algebraic slot counts, metadata bits, and transition cost parameters. It does not measure actual silicon area, gate delays, or physical power consumption on 3-state hardware devices.

2. **Fixed Mode Granularity:**
   - Tier limits are fixed to 4 discrete precision modes (8/16/32/64 bits vs 6/11/21/41 trits). Continuous or arbitrary bit/trit length representations are not evaluated.

3. **Arithmetic Operation Tax:**
   - Focuses on representation and trace switching storage ledger costs. It does not measure the execution throughput penalty of emulating software ternary ALUs on 64-bit binary hardware (which was separately evaluated in `experiments/radix_comparison/` and shown to be $10.6\times$ slower).
