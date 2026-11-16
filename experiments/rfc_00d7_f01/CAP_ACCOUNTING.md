# Static domain-cap audit after independent review

These are closed-form source-accounting bounds, **not experimental results**.
No case, order generator, candidate, oracle or control was invoked. The reviewed
baseline is `71997805e156e64f3cb4d3b58bd4493ca8b71a32`. Protocol commit
`c8ae2200bc37064282ffc4aaa683ed21d94d567c` and its SHA-256
`0c7e7da921d3af3c6c314ec4a8bf23478f9e05695b28b808721a45475106975c`
are unchanged. `CAP_ACCOUNTING.json` is machine-readable preparation metadata.

## State meaning and B correction

Counted units retain their previous meaning: materialized input/partial-order
states, candidate/reference output slots, instrumented transitions, result
states, controls and repeated observations. An interrupted pre-reservation is
not claimed as a completed state. Infrastructure instructions, byte buffers and
schema-validator internal instructions are not newly relabeled contract states.
Every result row is now charged when constructed, including rows later hidden
because a subclaim already has a witness. Evidence bytes are separate accounting.

B now directly generates valid topological orders by shuffling the three-event
chain `invoke < commit < response` with the four-event chain
`acquire < check < publish < release`. There are `C(7,3)=35` complete orders.
The full recursion tree materializes `C(9,4)-1=125` prefixes, including the empty
prefix, and charges all 125. The old filtered-permutation generator materialized
5,040 permutations before the 16 contextual expansions. No invalid permutation
is now constructed; this does not exempt a constructed state from charging.
The old event-rank order is preserved. No O1 or fence mutual-exclusion rule is
invented: all and only the same two-chain orders remain in the domain.

Direct generation alone is insufficient: 560 cases times seven candidate event
transitions would still leave too little room for comparisons and inputs. B
therefore reuses candidate transitions ONLY for identical semantic inputs
`(order, initial_revoked, applicable)`. The candidate consumes exactly these three
fields. A static AST check binds that read set to the memo key. There are at
most `35*2*2=140` actual candidate invocations, each with seven charged transitions.
Each newly materialized memo entry is additionally charged. There is no oracle
memoization, no cross-control cache and no candidate/oracle shared transition
routine. The source of both models remains unchanged.

All `35*16=560` labeled input cases remain: primary/fallback, epoch/selective,
target/peer and initially-valid/revoked. Every case still reserves its own two
candidate/reference output observations and result rows, including cache hits.
Repeated controls and their transitions are still charged on all four rotations.
A cache hit does not revisit or reconstruct intermediate candidate states.
This is computation reuse, not deduplication of repeated case visits.

B's conservative full bound is:

```text
88 controls + 125 order prefixes + 560 inputs + 1,120 output slots
+ 140 memo entries + 980 actual transitions + 560 B1 rows + 210 B2 rows
= 3,783 <= 4,096
```

The 210-row bound allows B2 on every order for all six applicable, initially
valid contextual combinations, even though some orders will not satisfy its
condition. State fit makes no runtime or memory-performance claim.

## All-area accounting

Full-path numbers assume no first-witness stop and no suppression of comparisons
after a solved subclaim. They include all control rotations and intermediate
states; actual bounded runs could stop earlier, but none has been performed.

| Area | Input domain | Rotated controls / charged states | Full-path charged states | Original cap | Consequence |
|---|---:|---:|---:|---:|---|
| A | 114 | 8 / 3,088 | 21,326 | 4,096 | Full traversal cannot fit; first-witness or inconclusive only |
| B | 35 orders × 16 = 560 | 8 / 88 | at most 3,783 | 4,096 | Full state bound fits; runtime unverified |
| C | 1,555 histories + 6 trace inputs; 347 yield | 8 / 80 | 11,150 | 8,192 | Full traversal cannot fit |
| D | 9,421 raw; 9,357 yield | 16 / 8,168 | at least 17,589; at most 147,616 | 16,384 | Even controls plus generation exceed cap |
| E | 6,156 crash + 144 two-phase + 9 bit-flip inputs | 16 / 72 | 60,327 | 16,384 | Full traversal cannot fit |
| F | 288 | 8 / 32 | 1,760 | 256 | Incompatible full domain; F explicitly blocked |

A has 12, 24, 36 and 42 cases for K=1,2,4,256. Their oracle prefix advances sum
to 24+84+252+17,422; input/output/result overhead is 4*114, plus 3,088 controls.

C has `1+6+36+216+1296=1555` raw histories and 5,910 validity-check transitions.
Exactly four next commands are legal at each valid prefix: two revocations and
one dispatch-or-cancel choice per request. Hence `1+4+16+64+256=341` valid
histories, with total valid lengths 1,252. Include six trace inputs, their
72 comparison/result states and 80 controls: `1555+5910+6+3*341+2*1252+72+80`.

D generates `4*3^7=8748` memory inputs, 576 step tuples (512 valid), one named
step case and 96 capacity inputs. Controls cost `4*(2*12+974+1044)=8168`.
Its upper bound is `8168+8748*14+576+512*29+976+96*6`. The lower bound counts
only controls and generation. The current memory-first loop can consume the
remaining allowance before later subdomains even if memory comparisons stop;
that is incomplete coverage, never a favorable result. This correction does
not reorder or shrink that declared domain.

E's six crash coordinates have total action counts 1+2+3+4+5+6=21 for each
prefix/sync pair. Its bound is `72+6156*6+1026*21+144*12+9*5`.

F retains `9*(4*2 + 2 + 11*2)=288` inputs: nine length/pattern combinations,
eight binding variants, two byte-mutation controls and 22 missing/null controls.
Every input has six charged units including two result rows; rotated mandatory
controls add 32. Early structural witnesses cannot be called complete 288-case
coverage. No cases or required controls have been deleted. `run_area` returns
`BLOCKED` before F constructs controls or enumerates inputs. The separate
unapplied amendment proposal describes a possible resolution.

## Evidence, real resources and stopping

The ledger reserves at most 128 bytes per row. Each mandatory control record
now has an explicit 4,096-byte bound, within its area's existing 4 MiB allowance.
All raw witnesses together remain limited to 512 KiB per area. Upper evidence
reservations, in bytes, are A 571,648; B 655,616; C 602,240; D 1,787,520;
E 2,203,776; F 630,784. Their sum plus 64 KiB runner metadata and 64 KiB
supervisor metadata is 6,582,656 bytes, below 25 MiB. This is a reservation bound,
not generated evidence or a measured footprint. A failed reservation stops
inconclusively before writing; it never licenses evidence omission and a pass.

The unchanged hard state caps sum to 49,408; wall/CPU ceilings remain 60/50
seconds, aggregate memory 256 MiB, at most one active 128 MiB runner, and total
artifacts 25 MiB. No runtime completion, kernel enforcement or memory peak has
been measured. All source/oracle independence requirements remain in force.

A/C/D/E may produce first witnesses or incomplete outcomes under the protocol's
stopping rules; their caps cannot support an unconditional exhaustive-negative
claim. F's full-coverage incompatibility requires a protocol decision. Readiness
is **independent source re-review**, not experiment authorization or execution
qualification. Preserve this negative preparation evidence.
