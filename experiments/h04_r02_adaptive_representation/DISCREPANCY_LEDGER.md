# Consolidated discrepancy and resolution ledger

Baseline #569 `01c85019`; audited #570 `8a49ba1d`, #571 `b2918551`.
Exact prior reports are retained under `history/`; these resolutions supersede
those reports. Classification D applies to the original inference, not all H04.

| ID | Category | Confirmed issue and disposition | Verification |
| --- | --- | --- | --- |
| DEF-01 | Mathematical defect | Both baseline implementations scale binary control metadata by r. Correct to B*m independent of r. #570 fix accepted. | C++ ratio sweeps and rational enumeration |
| DEF-02 | Mathematical defect | Published ratios do not match either consistent historical scaled-header equations or corrected equal-header equations. #570 fixed the ratio-1 path expression; integration solves the reoptimized envelope. | Emitted root versus exact max over paths |
| DEF-03 | Comparator fairness defect | Six trits and eight bits have different tier boundaries and capacity. 10 bits cover six trits but still have unequal native capacity. Add actual 10-bit control; avoid calling it exact capacity equality. | Boundary executable: 1250 vs 1210 at r=1.60 |
| DEF-04 | Implementation defect | Baseline C++ greedy selection contradicts declared DP. #570 DP fix accepted in principle; replace quadratic path copying and finite 1e18 sentinel with backpointers and infinity. | Exhaustive high-transition and 1e20-cost cases |
| DEF-05 | Documentation inconsistency | int64 clipping was described as the mathematical 41-trit range. Domain restriction is legitimate; full mathematical maximum is 18236498188585393201. | Exact Python integer derivation, int64 endpoints |
| DEF-06 | Documentation inconsistency | Claims that def47c68 does not exist are false locally. Local refs and object contain source/test/data packet. Preserve pinned archive and reexecute its verifier/tests. | H04-R01 logs and archive hash |
| DEF-07 | Unsupported inference | 96% adaptation attribution generalized from constant-small. Constant-large AB loses 250 U to FB; (-250)/(6400-4350)=-12.1951%. | Actual default output; scope conclusion to workload |
| DEF-08 | Measurement limitation | Symbol slots were extrapolated to CPU, hardware or universal software impossibility. Withdraw. | No performance counter/benchmark in H04-R02 |
| INT-01 | Mathematical defect | #571 says 11.6 exceeds 12 and associates 5/3 with an 8-bit baseline. Equal headers preserve 0.4 bits/value; 8/6=4/3, 10/6=5/3. | Concrete stream sizes; executable controls |
| INT-02 | Unsupported inference | #570 says Base-243 plus decode latency loses storage and execution even at 5/3 crossover. Decode latency alone does not change byte count, and no such codec was benchmarked. | Actual reversible codec; separately state absent timings |
| INT-03 | Implementation defect | One header per block previously allowed unrecorded intra-block tier changes. Enforce a shared mode for each block in both optimizers. | Partial-block enumeration and sequence inspection |
| INT-04 | Implementation defect | Frozen ratio-1 schedule is not a general optimized crossover. Reoptimize per ratio. | Schedule-changing [0,88574,0], tau=7 |
| INT-05 | Implementation defect | #570 unittest parses --bin-path in setUpClass, after unittest has rejected it. Remove custom args before unittest; require supplied binary, no repository walking/stale fallback. | Direct Python invocation and CTest |
| INT-06 | Implementation defect | 1e18 sentinel can leave predecessor=-1 for valid large costs; block_size=0 permits modulo zero. Validate parameters and use infinity. | Invalid-input and 1e20-cost executable tests |
| INT-07 | Documentation inconsistency | Methodology/oracle docs still scale headers; original oracle CTest only prints output, not a comparison. Correct contracts and add executable comparison. | Audit CTest, independent enumeration |
| INT-08 | Implementation defect | Experiment lacked an explicit C++ standard requirement, relying on extensions on this macOS compiler. Require cxx_std_17 on this target only. Small phased traces also risk division by zero; clamp phase length. | Warning-free target rebuild and source portability review |

## PR integration recommendation

Integrate the independently tested branch instead of merging #570 and #571
unchanged. #570 contributes valid metadata/DP/output-testing corrections and
preservation intent; its historical-nonexistence and impossibility conclusions
are rejected. #571 contributes a valid packing question and narrower intent;
its arithmetic and claimed empirical execution conclusion are rejected.
Root README additions are omitted: the existing root already links the registry,
and this research does not require changing the public project framing.

After review and required CI checks, merge this integration PR, then close #570
and #571 as superseded with links to the integrated evidence. No PR is merged
or closed by this work. Their complete commits remain available by pinned SHA;
exact report snapshots preserve the unsuccessful reasoning, not just summaries.
Existing main-wide red checks remain a separate maintenance issue; local research
passes are not a declaration that the full repository or all platforms are green.
