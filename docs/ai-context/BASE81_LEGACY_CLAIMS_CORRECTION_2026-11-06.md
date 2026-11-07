# Base-81 legacy explanation — correction and non-authoritative warning (2026-11-06)

> **Non-normative correction / context-loading tombstone.** An earlier assistant-generated explanation circulated in conversation, beginning “Base-81 (T81) is a unique numeral system” and claiming, among other things, “A single T81 digit can represent 4.32 bits of information.” That explanation is **withdrawn as research guidance**. Its original file provenance is **not established**: do not attribute it to `TYRNARY_-_T81Analysis.pdf` or another document without source verification. This note does not alter the canonical hypothesis register, historical records, or experimental receipts.

## Corrections that any AI reader must apply

| Legacy assertion | Correct bounded interpretation |
|---|---|
| One Base-81 digit represents 4.32 bits | **False.** `81 = 3^4`; one base-81 digit encodes four trits and has `log2(81) ≈ 6.33985` bits of maximum information. A single 8-bit host byte has 256 code points, so representing 81 states in one byte uses approximately 79.25% of its theoretical capacity. |
| Higher radix is more computationally efficient because fewer symbols are needed | **Unsupported.** Fewer written digits is not a speed, energy, area or cost result. Under the classical radix-economy model with digit cost proportional to radix, the continuous optimum is `e`, and ternary is best among integer radices. Different engineering cost models require explicit evaluation. |
| Base-81-trained AI models use fewer computation steps than binary models | **Unsupported.** No established model-training or equal-workload benchmark demonstrates this; do not infer AI acceleration from digit grouping. |
| Base 81 maps naturally to SIMD/vector hardware | **Unsupported as stated.** Four-trit grouping and 2-bit packed ternary operations are distinct from base-81 numeral arithmetic. Vector efficiency depends on concrete layout, instructions, decode and conversion overhead. |
| Radix conversion itself compresses information | **False.** Lossless invertible re-encoding does not reduce source information. Binary-to-text expansion and distribution-dependent compression must be evaluated separately, with full framing overhead. |
| More entropy per encoded character implies stronger cryptography | **False as a security inference.** The encoding alphabet does not improve underlying key entropy, hash security or unpredictability. |
| Base-81 floating point has simpler rounding or better performance | **Unverified.** A specified arithmetic format, oracle, error model and matched implementation are prerequisites. |
| Base-81 will revolutionize high-performance computing | **Promotional speculation, not a scientific result.** No such universal advantage is established. |

## Measured counterevidence and valid scope

- Historical **4096×4096 AVX2 MatVec** comparison: native INT8 **17.15 GFLOP/s** versus packed 2-bit ternary **12.91 GFLOP/s** (ternary approximately **75.3%** of INT8 throughput); the packed representation uses **75% less weight storage** than 8-bit weights in this comparison. Do not confuse storage reduction with measured speedup or generalize the memory regime beyond its primary evidence.
- Base 81 remains a legitimate **four-trit grouping / representation-interface choice**, not a proven computational optimum. The research question is whether it yields a reproducible workload-specific net benefit against matched binary alternatives after encoding, decoding, alignment and conversion costs.
- The current [RESEARCH_IDENTITY.md](RESEARCH_IDENTITY.md) already correctly gives **6.33985 bits** and treats H03 as a falsifiable hypothesis. This note specifically prevents assistant-generated historical assertions from being elevated to authority by context reuse.

## Loading and provenance rule

Any assistant or reviewer encountering the quoted legacy explanation in chat transcripts, PDF-derived summaries, archive copies, or prior AI handoffs must treat the claims above as **superseded and non-authoritative**. Preserve original source text for provenance where it exists; do not silently alter historical artifacts. Do not cite this correction as proof that a particular PDF contained the claims. Prioritize canonical H01–H08 definitions and actual measured evidence. No new performance experiments or Phase C authorization follows from this note.
