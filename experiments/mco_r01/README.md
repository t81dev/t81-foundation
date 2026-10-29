# MCO-R01 executable conformance experiment

Bounded research only. The frozen contract is at
[`docs/research/model-computational-object`](../../docs/research/model-computational-object/).
No production T81 module is imported or changed. The sole protected effect is
`Owner.rows.append` on an in-memory list. Harness artifact writes and fixed
Python/Ruby subprocess commands are experiment orchestration; proposal bytes
cannot select commands, paths, authority context, or operations.

From the repository root, use Python 3.14.3 with the pinned packages in the
packet's `review/requirements.txt`, and Ruby with its standard JSON, BigDecimal,
Base64, and Digest libraries (tested Ruby 2.6.10 / JSON 2.1.0):

```sh
python3 -m venv /tmp/mco-r01-venv
/tmp/mco-r01-venv/bin/python -m pip install -r docs/research/model-computational-object/review/requirements.txt
/tmp/mco-r01-venv/bin/python docs/research/model-computational-object/review/check_packet.py --manifest
/tmp/mco-r01-venv/bin/python experiments/mco_r01/run.py --output-dir /tmp/mco-r01-new-run
```

The output directory must not exist. A failed gate raises an error and preserves
the commands and streams collected so far. `results.json` is written only after
all gates succeed. Mutation/reconstruction exit 2 is expected rejection, not a
crash counted as a pass. Each mutant must first reach its specified semantic
violation and exit 0. Every run uses new entropy; exact replay uses that run's
`proposal-trace.json`:

```sh
/tmp/mco-r01-venv/bin/python experiments/mco_r01/driver.py --arm A --proposal-trace /tmp/mco-r01-new-run/proposal-trace.json > /tmp/mco-observed-A.json
ruby experiments/mco_r01/oracle.rb docs/research/model-computational-object --reconstruct /tmp/mco-observed-A.json /tmp/mco-r01-new-run/proposal-trace.json
```

`oracle.rb` uses a separate Ruby parser, exact decimal numbers, a frozen-schema
subset interpreter, and direct Boolean authorization. It imports no candidate,
combiner, transition helper, fixture generator, or packet review code. Its
default mode compares all frozen cases and event plans before candidates run.
The schema interpreter rejects unsupported keywords; it is not a general
Draft 2020-12 validator. The packet's pinned validator supplies the independent
meta-schema/structural check; authorization remains a semantic calculation.

Arm A uses Boolean facts, explicit diagnostics, and an integer enum. Arm B uses
the specified nine-entry D/U/P meet table. They implement authorization
separately, run in separate processes, and share only wire/schema handling and
the honest owner. That shared owner is the enforcement control and trusted
computing base, not a second independent sink implementation. Neither arm reads
the fixtures' expected results. The oracle's reconstruction mode also ignores
expected annotations, recomputing from fixture bytes and authoritative context.

The additional source chooses 32 inert, frozen endorse/reject/review proposals
using OS entropy. All three have the same request and authority context. Each
draw receives a fresh independently authorized owner; a recorded trace replays
exactly. This models bounded proposal nondeterminism, not arbitrary LLM behavior.

See [RESULTS.md](RESULTS.md), [DISCREPANCIES.md](DISCREPANCIES.md), and the new
[research handoff](HANDOFF.md). Existing packet records remain byte-identical.
