# T81-AIOPT-001 — Fixed-Catalog Ternary Pattern Filtering

**Status:** PROPOSED / RECONSTRUCTED PROTOCOL — not an accepted historical specification  
**Version:** 0.1.0 (2026-11-01)  
**Milestone:** M1, independent correctness only  
**Repository scope:** isolated research experiment; no runtime, ISA, Axion, CanonFS, or governance-contract changes

## 1. Provenance and authority

The previously discussed T81-AIOPT-001 protocol could not be located in the default-branch code index, PR search, branch-name search, or commit-message search as of 2026-11-01. This document **reconstructs a prospective protocol** from the known research objective; it does not purport to reproduce an unavailable prior document, its exact corpus, or historical approvals. Any recovered original protocol must be compared explicitly before superseding this draft.

M1 is **not** evidence of ternary speedup, adaptive representation, AI optimization, inference acceleration, or hardware efficiency.

## 2. Question and boundaries

**Question:** Can independently implemented scalar, byte-oriented, and SWAR-oriented evaluators produce identical, exact results for a fixed catalog of ternary pattern predicates on a bounded input domain?

M1 must not select a faster representation, tune thresholds, run timed benchmarks, use learned models, or claim adaptive representation. Future milestones may compare catalog selection policies only after an independently accepted correctness gate.

## 3. Mathematical semantics

Let the alphabet be `T = {-1, 0, +1}`. Let the input domain be `X = T^4` (81 distinct four-trit vectors). A catalog entry is a pattern `p ∈ T^4`; no wildcards in M1.

Define exact matching:

`match(x, p) = 1` iff `x_i = p_i` for all `i ∈ {0,1,2,3}`; otherwise `0`.

The output of catalog filtering is an ordered list of matching catalog indices in ascending index order. The result must be independent of implementation, iteration history, host endianness, or signed-char behavior.

**Catalog C40:** enumerate `T^4` lexicographically with `-1 < 0 < +1`; retain the first 40 patterns. Index 0 corresponds to `(-1,-1,-1,-1)`. Index 39 must be derived by enumeration, not hard-coded. Catalog order and indices are immutable within this version. This deliberately artificial catalog tests correctness; it is not a representative workload distribution.

## 4. Representations

- **Independent scalar oracle:** operate on four semantic integer values; compare values individually without calling optimized implementation helpers or sharing a match predicate.
- **Byte adapter:** encode each trit as one unsigned byte using `-1 → 0`, `0 → 1`, `+1 → 2`; reject all other byte values. Compare four encoded positions.
- **SWAR adapter:** pack each trit into two bits, least-significant pair first, using the same codes. For four trits, use an unsigned 8-bit payload. Code `3` in any two-bit position is invalid and must be rejected before evaluation. Decode and encode through separately testable functions. Do not use undefined shifts, signed overflow, or host-dependent byte reinterpretation.

No representation may silently normalize an invalid trit, accept a malformed payload, or return a partial match list on error.

## 5. API contract

Each adapter must provide an operation equivalent to `filter(input, catalog) -> Result<ordered_indices, error>`.

Errors must distinguish at least:
- `INVALID_TRIT` for semantic input outside `T`;
- `INVALID_ENCODING` for byte or SWAR codes outside `{0,1,2}`;
- `INVALID_LENGTH` for input vectors of length other than four;
- `INVALID_CATALOG` for catalog size/order/content that differs from C40.

The oracle's semantic result is authoritative for valid input; invalid-input handling must be independently asserted against the declared API contract. Exact error names may be implemented in the experiment language but must remain stable within the milestone.

## 6. Test and evidence requirements

**Exhaustive valid domain:** all 81 inputs against all 40 catalog entries (3,240 individual predicate decisions), plus the complete ordered filter output for each of the 81 inputs. Check both adapters against the independent oracle.

**Round trips:** all 81 valid vectors must round-trip through byte and SWAR encoding. Check uniqueness of encodings and deterministic output.

**Malformed cases:** test every invalid two-bit code position in the SWAR payload, representative invalid byte values at each position, wrong vector lengths, wrong catalog size, wrong catalog order, and invalid semantic trit values. Confirm fail-closed behavior.

**Boundary cases:** first and last valid vectors, catalog members and nonmembers, and vectors whose encoded payload includes zero bytes or high-order nonzero bit pairs.

**Determinism:** run the same corpus twice in fresh processes where practical, compare exact machine-readable results and a cryptographic digest of canonical serialized results.

**Corpus:** preserve any earlier reported 22-record / 1,890-case corpus as an **unverified historical target**, not an M1 requirement or an executed result. Do not fabricate those cases or imply they were recovered. The exhaustive 81 × 40 grid is the authoritative minimum for this reconstructed v0.1 protocol.

## 7. Required artifacts

Create an isolated experiment directory, suggested `experiments/aiopt_001_fixed_catalog/`, containing:
1. This protocol or a relative link to its accepted location;
2. A separately written semantic oracle;
3. Byte and SWAR adapters with explicit validation;
4. A deterministic test runner and instructions for supported hosts;
5. A machine-readable results summary including exact counts, failures, environment, source commit, and digest;
6. A limitations and disposition report.

No test counts, benchmark outcomes, or acceptance status may be asserted before actual execution.

## 8. M1 acceptance gate

M1 **PASSES** only when:
- all exhaustive valid-domain oracle comparisons agree;
- all declared invalid inputs fail closed with the expected error category;
- round-trip and uniqueness checks pass;
- deterministic repeatability is demonstrated;
- an independent reviewer can reproduce the results from pinned source and documented commands.

M1 **FAILS** on any observed semantic disagreement, invalid acceptance, or nondeterministic result. M1 is **BLOCKED** if required implementation, tooling, or independent review is unavailable.

A passing M1 authorizes only consideration of a separately specified M2 measurement protocol; it does not automatically authorize measurements, learned selection, hardware claims, or changes to production T81 components.

## 9. Deferred questions

Future milestones must separately define (a) selection objectives and budgets, (b) conventional binary or non-ternary comparators, (c) workload distribution and sampling, (d) cost of encoding, decoding, metadata and switching, (e) statistical protocol, and (f) preregistered success and stopping thresholds. Fixed-catalog selection is not equivalent to general adaptive representation.

## 10. Change control

Any changes to alphabet, vector length, catalog ordering, encodings, invalid-input policy, or acceptance criteria require a versioned amendment before testing. Preserve negative results. Do not retrofit the protocol to outcomes.
