# Source- and commit-grounded lineage

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Source- and commit-grounded lineage](#source--and-commit-grounded-lineage)
  - [Intellectual origin versus code evidence](#intellectual-origin-versus-code-evidence)
  - [Recovered sequence](#recovered-sequence)
  - [Direct inheritance that can be checked](#direct-inheritance-that-can-be-checked)
  - [What changed in the meaning of “tier” and “Axion”](#what-changed-in-the-meaning-of-“tier”-and-“axion”)
  - [Reproduced arithmetic contradiction](#reproduced-arithmetic-contradiction)
  - [Unclosed provenance gaps](#unclosed-provenance-gaps)

<!-- T81-TOC:END -->


Scope: six repositories pinned in [source-pins.json](evidence/source-pins.json). Commit metadata is a repository record, not independent proof of when an idea was invented. “Earliest” below means earliest identified in the available inspected history. Source identity proves shared content, not authorship or whole-system semantic equivalence.

## Intellectual origin versus code evidence

The researcher describes Knuth, discrete mathematics, recursion and CWEB as the starting point. That intent is preserved in the supplied charter. The original mathematics paper and historical 34-page PDF were not supplied in this workspace. Their detailed technical claims remain tier C; no publication chronology or paper quotation is manufactured here.

## Recovered sequence

| Record | Pinned evidence | What it establishes |
| --- | --- | --- |
| Foundation repository and early TISC/VM specifications | Initial `049d1d929d3558deac4b68a1bcc2cfdeb6128359`, 2025-11-21; TISC spec `bf6dd85834a010044f59766a6aca8c40ac2ba0b9`; VM spec `eed6107cbee1cb2e059be6763c59281fe003fa6a` | Modern repository/spec history is older than the currently published Hanoi Git import. Specifications alone do not establish execution correctness. |
| Hanoi-family CWEB archived inside Foundation | `fe00396831c8bb249fde06a04186fa02f6018caf`, 2025-11-22, `legacy/hanoivm/src/hanoivm_core/` | Concrete earlier source for recursion, promotion and arithmetic hypotheses. Archive removal `03112f6ce583905f13bfc88a6e07db7ea3a5a1ca`, 2026-03-06, did not erase the objects. |
| Numeric and compiler implementation surfaces | Foundation parser history includes `ee351c9436b972ac868a84ddb68982cb27f7a669`, 2025-11-24; tensor header `9dcda5bd4e5f6021097e2bfc967b41bcc7033827`, 2025-11-22; Axion policy implementation `649e235c60c1b269449d5a8419e6ced902a755b2`, 2025-12-04 | Actual code paths developed alongside specifications. These dates do not establish that all named components worked together. |
| t81lib | Root `87e480c5fae5082aa5807bb0bd28d836b9c9d478`, 2025-12-09; inspected head `995ea70796259a8f9c5f265a6cb3084143e2b949` | Separate numerical/quantization work. Current README speed claims are not reproduced measurements in this packet. |
| Foundation to t81-lang extraction | Split initial `dc9ec92044876651aa2531b2879413285b388961`, 2026-02-08; parser blob `b0a3812386063a2aa60d6a0b7e4643330ec936b0` matches Foundation `77c77098ad821ecd2c8452c3e7ab4f5eefae1760`, 2026-02-07 | Documented migration direction corroborated by byte identity, not merely shared terminology. |
| Standalone t81-vm | Bootstrap `68b32caef7f07e5f7d030b9b0ba63d0736c2b208`, 2026-02-08; head `4e2bba639d05249722db847563ae37991289ea52` | A separate runtime scaffold/adaptation. Its migration plan explicitly rejects wholesale copying of the Foundation VM and the legacy Hanoi archive. Full parity is not inferred. |
| Hardware baseline | RTL introduced `3ea4bccf0dfa867cf8186ff886c9d716bb7b96dc`, 2026-02-08; inspected head `834521d4e6b37cf8c871b1ac64af449ff6493b7d` | Executable-looking digital RTL defines signed two-bit trit signals and saturating operations. No physical three-state element cost or fabricated-device advantage is measured here. |
| Modern Foundation tiers | `2f93753f387864d8d09e0edadd29381032680264`, 2026-02-26, tier-limit module; inspected Foundation head `f6d4d680709c5c6c98b305628e04346f5ef61620` | Tier0..Tier6 resource/recursion limits are distinguishable from Hanoi's 81/243/729 scheduler labels. Common numbers do not establish an identical model. |
| Current public Hanoi import | `59205d76db4aca3da7c0bdc521924b8f9bc52cc3`, 2026-10-25, sole reachable commit | Makes the precursor source available now; does not date its original development. Forty-one source blobs occur in Foundation history. |

## Direct inheritance that can be checked

Foundation historical `legacy/hanoivm/src/hanoivm_core/t81recursion.cweb` and current Hanoi `src/t81recursion.cweb` share Git blob `d9a0d3301ebc5820b7997db5d09965414ad65883` and SHA-256 `8261d418037a5e9d9b95ca48c4c9071fdbac156c263059fe6b4d29c3c1db4834`. [The full 41-blob inventory](evidence/shared-hanoi-foundation-blobs.json) records exact historical paths. Arithmetic, scheduler and promotion files are not claimed to be byte-identical merely because their names match.

Foundation parser at `77c77098…` and t81-lang initial parser at `dc9ec920…` share blob `b0a3812386063a2aa60d6a0b7e4643330ec936b0` and SHA-256 `ba61bb2cf6a8816a42db61bbac3a8ed5cf6b59591a20fd94fc37c717ad9258e6`. The split's `scripts/migrate-from-foundation.sh:4-8,35-40` explicitly copies frontend assets from Foundation. Its migration TSV records paths, not source commit IDs; the pinned blob comparison closes that gap for this parser revision only.

The standalone VM's `docs/foundation-migration.md:27-31,60-72` calls for copying the interface pattern, locally reimplementing components, and not migrating the legacy CWEB tree. This supports conceptual/contract lineage, not a proof that its VM body is inherited or equivalent. Its `src/vm/loader.cpp:11-16` parses a tier policy and its `src/vm/vm.cpp:894` has a tier-zero deny predicate; these are not interchangeable evidence for Foundation's full Axion engine or historical AxionAI.

## What changed in the meaning of “tier” and “Axion”

At Hanoi head, `src/tier_scheduler.cweb:37-44` selects 729 for entropy/depth over 100 or an `AI` intent substring; otherwise 243 over 40, else 81. Line 56 writes `ctx->tier_mode`. `src/hvm_promotion.cweb:40-60,99-121` instead defines modes 0/1/2, promotes at call depth over 10 and on tensor-op detection, and writes `ctx->mode`. The shown VM context in `src/hanoivm_vm.cweb:41-48` has `mode`, not `tier_mode`; referenced integration headers are not present under the expected paths. The VM and promotion files also assign different numeric constants to some promotion opcodes. This is inspected inconsistency/under-specification, not an executed integration failure.

`src/hanoivm_vm.cweb:90,181,231` repeats operation-table definitions and includes null handlers. Combined with the failed root dry run, this prevents treating the source collection as a validated integrated VM. No null handler was executed in this session.

Foundation `vm/tier_limits.cpp:29-47` maps cognitive tiers to recursion limits 0,1,10,81,243,729,2187. That is a resource policy, not a declaration of radix width. The `T729Tensor` name does not impose a 729-element tensor; its shape is dynamic. Historical AxionAI entropy/NLP/kernel-control intent and modern `kernel/axion/policy_engine.cpp` authorization contracts share a research theme, but their behavioral equivalence is unproved.

## Reproduced arithmetic contradiction

The current CWEB's `TernaryNum` comment advertises digits -1/0/1. Verbatim functions at lines 82,100,108 were extracted and compiled with Apple Clang 21, C11, O2 and strict warnings. Independent interpretation is `a + 3b + 9c`. Only exactly representable outputs in [-13,13] count; overflow cases are excluded.

Addition fails 422 of 547 cases; multiplication fails 140 of 201; negation fails zero of 27. Addition normalizes into 0/1/2 rather than the advertised balanced domain. Multiplication is componentwise and lacks positional cross-products: (0,1,0) times (1,0,0), representing 3 times 1, returns zero. The multiplication comment explicitly says componentwise: such an operation can be valid for a vector algebra, but cannot be substituted for positional integer multiplication. The reported multiplication failures are against the stated integer oracle, not against every possible three-state algebra. The earlier Foundation arithmetic archive also documents componentwise multiplication, but its addition body differs; current probe counts are not attributed to that earlier revision.

See [results](evidence/hanoi-arithmetic-results.json), [exact extracted C](evidence/isolated_hanoi_arithmetic.c), and [reproduction script](probes/reproduce_hanoi_arithmetic.py). The isolated oracle is not a full CWEB build, benchmark, physical-device experiment or proof about modern Foundation arithmetic.

## Unclosed provenance gaps

Private pre-publication chronology, original Knuth notes, the named PDF/transcripts and the original LLM mathematics paper remain unavailable. No private-history inference is made from filesystem dates, archive dates or README prose. Complete source inheritance into modern Foundation VM/Axion, whole-system Hanoi reachability and an operational common tier-domain contract remain unproved. These gaps are recorded rather than filled with a more elaborate architecture.
