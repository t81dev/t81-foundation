# Fresh-agent onboarding example

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Fresh-agent onboarding example](#fresh-agent-onboarding-example)
  - [Discover an unresolved question](#discover-an-unresolved-question)
  - [Challenge a different claim](#challenge-a-different-claim)
  - [H08: fresh-client discovery with no prior conversation](#h08-fresh-client-discovery-with-no-prior-conversation)

<!-- T81-TOC:END -->


This is a reproducible example for a new, independent agent. It is authored by
the implementation agent, not a claim that a second model or human has performed
an independent review. No previous conversation, model identity, API key or
experiment execution is required. Use the environment/setup in the Commons README.

## Discover an unresolved question

1. Call `list_research_tracks`. Track R names H01–H06 and points to the continuity
   charter; Track M contains H08, not Investigation08's integration hypothesis.
2. Call `list_hypotheses`, then `get_hypothesis` with `hypothesis_id: H04`.
   The canonical claim asks whether an explicitly defined adaptive ternary policy
   beats fixed ternary and an equally capable adaptive binary policy. Its
   historical result says the original Hanoi tier semantics remain under-specified.
3. Call `get_evidence` for `h04-refinement`, `h04-r02`, `h04-limitations` and
   `h04-handoff`. The first specifies matched four-arm model controls; R02 reports
   a negative result. The handoff's broad software conclusion exceeds its stated
   symbolic-model limitations. Preserve the negative; do not assume a hardware
   result or erase the unresolved original question.
4. Call `list_open_research_questions`. `Q-H04` asks for equal domains, packing,
   control and transition costs; `D-H04-SCOPE` identifies the overgeneralization.
   The hypothesis's `discrepancy_history` retains `D-607-IDS` as superseded
   by #608; it records why #607's throughput definition was never canonical H04.
5. Call `get_reproduction_instructions` with `hypothesis_id: H04`. This returns
   design/review instructions, with execution unavailable. A recipe is not approval.
6. Inspect [h04-proposal.json](examples/h04-proposal.json). It records the exact
   main baseline, linked negative/model evidence, independent oracle requirements,
   matched AB control, falsifier, measurement plan, resources and security limits.
   Authorization is `not_requested`; it makes no finding or execution claim.

For example, retrieve the canonical record and validate the prepared proposal:

```sh
python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H04"}}'
python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_evidence","arguments":{"evidence_id":"h04-limitations"}}'
python -B tools/research_commons/commons.py --validate-contribution docs/research/commons/examples/h04-proposal.json
```

Expected structured outcomes: canonical H04 and dated result, the limitations
file with its pinned identity, and `valid_proposal_shape` with
`execution: unavailable`. This validates discovery and proposal shape only.
The new reviewer should replace attribution with their actual identity/role,
record prior context honestly, and independently check the formulas before
requesting authorization for a bounded experiment. A proposal supported solely
by generated summaries does not meet that bar.

## Challenge a different claim

Retrieve H02, then `hanoi-negative`: recorded addition failures are 422/547 and
multiplication failures 140/201 under positional semantics. Retrieval is not a
new reproduction. Retrieve `e1-readme` for the separately merged E1 footprint
negative: complete TISC 1,240 bytes versus Wasm 157 bytes at the recorded entry
contract. Neither result settles all ternary devices or designs.


## H08: fresh-client discovery with no prior conversation

Start a new client process with only the Commons README and repository checkout.
Use `list_research_tracks` to find H08 under Track M, then `get_hypothesis` with
`hypothesis_id: H08`. Its verbatim canonical record has evidence tiers C/E,
`confidence: Researcher intent authoritative; comparative benefit untested.`,
and earlier prompt provenance unavailable. The recorded result explicitly says
this was not a controlled AI-productivity study. Read the assumptions: commit
authorship does not prove prompt provenance, and repeated model summaries are
not independent verification.

Retrieve `register` to check the record independently of this narrative. Retrieve
`initiative` (historical PR607), `initiative-608` (merged but disputed H07/H08
summary) and `initiative-610-proposed` (UNMERGED proposed correction). The returned
roles and uncertainty distinguish them. Superseded history does not erase the
occurrence of PR607 errors. The remaining PR608 discrepancy and the dependency
on PR610 are explicit; the proposed correction is not represented as main.

Prepare [h08-proposal.json](examples/h08-proposal.json): human-only,
single-assistant and adversarial-assistant arms with matched tasks, information
and resource budgets; blind adjudicators independent of generators; a held-out
confirmatory corpus, preregistered effect threshold and stopping rules; raw
failures, uncertainty and actual provenance. It records `not_requested` human
authorization and claims no methodological benefit or execution.

```sh
python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H08"}}'
python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_evidence","arguments":{"evidence_id":"register"}}'
python -B tools/research_commons/commons.py --validate-contribution docs/research/commons/examples/h08-proposal.json
```

The contract test runs discovery/retrieval/validation in fresh CLI processes
without conversation context and prepares the proposal in scratch. This checks
client onboarding behavior, not the judgment of a second model, authentic reviewer
independence, or scientific benefit. No independent-model review is claimed.
