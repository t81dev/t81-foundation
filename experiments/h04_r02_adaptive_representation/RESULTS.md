# H04-R02 unified research conclusion

The original H04-R02 inference is invalidated by accounting, comparator and
validation defects. Corrected H04-R02 provides conditional symbolic cost results
and a concrete reversible packing demonstration. H04 remains unresolved.

- **Mathematically established:** exact trit ranges, code capacity, finite byte
  lengths and crossover equations in [MATHEMATICS.md](MATHEMATICS.md).
- **Modeled:** optimized AT/AB winners depend on r, mode boundaries, metadata,
  transition and conversion assumptions. The legacy boundary trace retains an
  AT window; the 10-bit control narrows it to r<5/3, which includes 1.60.
  Equal metadata cannot erase a payload advantage. Constant-large adaptation
  loses to fixed precision at the default parameters. The historical 96% claim
  describes one ratio-1 trace and is not a workload-independent attribution.
- **Implemented and tested:** C++ prefix DP, Python suffix oracle, independent
  rational enumeration, block sharing, reoptimized crossover and reversible
  Base-243 / packed 10-bit codecs. See [CORRECTED_RESULTS.md](CORRECTED_RESULTS.md)
  and [VERIFICATION.md](VERIFICATION.md).
- **Measured:** actual encoded byte lengths. No new CPU performance, bandwidth,
  hardware area or energy benchmark supports an execution advantage or loss.
  Timing ratios from other experiments cannot be transplanted into this codec.
- **Unresolved:** physical ternary devices, alternative arithmetic, stronger
  binary codecs, causal adaptation, real conversion/selection costs, and
  independent cross-platform/laboratory reproduction. H04-R01's source and
  tests are recovered here; reexecuting its own verifier is not independent
  validation of all of its mathematical assumptions.

Track R continues with falsifiable experiments; Track F maintains reliable
runtime/evidence apparatus; Track M records corrections and provenance. This
packet neither closes ternary software research nor establishes ternary-specific
computational superiority. Canonical H01–H08 remain unchanged.

The discrepancy resolutions are in [DISCREPANCY_LEDGER.md](DISCREPANCY_LEDGER.md).
Prior invalid claims and negative results remain exact, explicitly superseded
snapshots under [history/](history/README.md).

## Next experiment: matched codec and access control

Test whether Base-243's saving over fixed 10-bit fields survives an equally
optimized binary block code for the same 729-state domain and a declared access
contract. Predeclare block sizes 1,5,64,1024; equal count framing, adaptive tags
if used, 1/8/64-byte alignment, and sequential versus random-index reads.
Include packed 10-bit, Base-243 and integer block coding of base-729 digits;
use the same input distribution and exact round trips for all arms. Report
actual bytes separately from encode/decode latency distributions and bandwidth.
Pin source/toolchain, seeds and timing procedure before measuring.

Falsifier for a radix-exclusive storage claim: a binary block code matches or
beats Base-243 under the same contract (it can encode the same byte language).
Falsifier for a practical codec win: the predefined byte-saving and end-to-end
latency/bandwidth targets fail after framing, access and conversion. Do not
reinterpret storage-only success as faster computation. No VM/ISA expansion is
needed. A native-device cost experiment is a separate H01/H04 question.
