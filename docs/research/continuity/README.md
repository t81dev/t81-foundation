# T81 research continuity packet — 2026-10-25

This packet continues the user-supplied [Master Continuity Charter v1.0](MASTER_CONTINUITY_CHARTER_v1.0.md). It preserves the primary ternary/alternative-representation question while keeping governance and CI as supporting, separately evaluated work. It does not alter the public runtime contracts or certify any implementation.

Start with [the source-grounded lineage](LINEAGE.md), the [versioned hypothesis register](HYPOTHESIS_REGISTER_v1.json), and the [selected next discriminator](H04_RADIX_ADAPTATION_DISCRIMINATOR.md). The [session handoff](SESSION_HANDOFF.md) records the next single action and stopping criterion.

## Findings established in this session

- Foundation's historical Git tree contains 41 blobs identical to source in the current Hanoi import. `t81recursion.cweb` is byte-identical at Foundation commit `fe00396831c8bb249fde06a04186fa02f6018caf` (2025-11-22). Its archive was removed from the working tree on 2026-03-06 but remains in history. Hanoi's public Git repository currently has one import commit on 2026-10-25. This establishes shared source, not the original invention date or the direction of every private transfer.
- The standalone T81Lang repository is a documented extraction from Foundation; its initial parser blob exactly matches a Foundation revision from the preceding day. The standalone VM instead documents selective adaptation and local reimplementation. The lineage is not a simple chain of successively complete implementations.
- A pinned, isolated CWEB arithmetic probe found **422/547 addition failures** and **140/201 multiplication failures** under balanced positional semantics, excluding overflow cases. Negation passed all 27 inputs. `-1 + 0` emits a digit `2`; `3 * 1` emits zero. These are failures against the balanced positional-integer contract used by the probe. Componentwise multiplication may instead belong to a vector algebra; no disproof of ternary arithmetic or future hardware follows.
- The original adaptive-tier hypothesis lacks common domains and transition semantics. Current scheduler and promotion modules use different fields, encodings and triggers; the modern Foundation cognitive tiers are another contract. No new scheduler was built.
- The ideal asymptotic storage threshold is insufficient for finite cases: for 256 states, six ternary elements at cost 1.5 each cost 9, versus eight binary elements at cost 1 each. This is a conditional model result, not physical measurement.

Under the ideal fixed-capacity model, let `n_b = ceil(log_b M)`. Ternary storage is cheaper exactly when `C3/C2 < n_2/n_3`; the asymptotic limit is `log2(3)`. This independently derived comparison assumes the same cost units and no sensing, ECC, control or conversion overhead. It is neither a universal latency threshold nor a manufactured-device result. Joint/block packing changes the capacity being compared and must be handled for both baselines.

## Reproduce the bounded findings

Use Python 3, a C11 compiler, and a clone containing Hanoi commit `59205d76db4aca3da7c0bdc521924b8f9bc52cc3`:

```sh
python3 docs/research/continuity/probes/reproduce_hanoi_arithmetic.py \
  --hanoi-root /path/to/Hanoi --output-dir /tmp/t81-hanoi-probe
python3 docs/research/continuity/probes/storage_boundary.py
python3 docs/research/continuity/probes/verify_lineage.py \
  --foundation-root /path/to/t81-foundation --hanoi-root /path/to/Hanoi \
  --lang-root /path/to/t81-lang
```

The first command extracts function bodies verbatim from the pinned Git object and compiles only those functions plus an independent bounded integer oracle. It does not tangle CWEB, load kernel modules, run a VM, test a GPU, or validate a complete Hanoi build. Exit zero means the expected historical counterexamples were collected; the JSON explicitly records `candidate_passes: false`.

The full Hanoi root `make -n all` independently fails with a missing `hvm_loader.c` rule. No full-build success is claimed; `ctangle` was unavailable. Sources and histories were inspected without running installation/migration scripts or rewriting the archives.

Evidence includes source pins, source/blob identities, exact extracted bodies, compiler command/version, result counts, the dry-run failure, an exact finite-state calculation, and current PR snapshots. A/B/C/D/E labels distinguish reproduced observations, source inspection, reports, derivations and proposals. The referenced 34-page PDF, original paper and two historical transcripts were not available under the supplied names; their contents were not inferred or quoted.

PRs #565 and #566 remain open/draft at their existing heads. Their CodeQL checks now report success, but their required quality gates report failure. No merge, closure, new PR, CI repair or branch-protection change was performed in this continuation. Protocol 001 remains NOT READY and was not modified.

## Version history

- 2026-10-25, packet/register 1.0.0: preserve charter bytes; pin six repositories; distinguish shared legacy source, frontend extraction and VM adaptation; reproduce isolated arithmetic counterexamples; select a radix/adaptation discriminator. Prior research documents remain historical records and were not rewritten.
