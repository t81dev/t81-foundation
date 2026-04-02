# Financial Services Vertical Demo Walkthrough

## Purpose

This walkthrough turns the current bounded `classify-fixed` composition into a
financial-services demo about controlled execution and audit-ready output.

Use it as a live demo script, not as a product roadmap.

## Demo Promise

This walkthrough shows four things only:

1. an unapproved artifact path can be denied before execution
2. an approved bounded task can run
3. the output is a canonical object chain, not a log-only result
4. the same run can be repeated with the same canonical object identity

## Audience Translation

Say this at the start:

> This is not a benchmark demo. It is a proof that we can control what runs,
> prove why it was allowed, and preserve what it produced as an auditable
> object.

## Demo Shape

Keep the walkthrough in this order:

1. denied artifact path
2. approved bounded classification path
3. object readback
4. deterministic rerun

Do not add extra chains or side stories.

## Step 1 — Show a Denied Path First

From the repo root:

```bash
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"

build/t81 canonfs import \
  examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w \
  --canonfs-root "$canon_root" \
  --policy examples/storage-and-canonfs/canonfs-interchange/v1/policy-deny-all.apl \
  --json
```

What to say:

> Before anything executes, the system can deny the artifact path.
> The important point is not just that it failed. The important point is that
> it failed with structured policy evidence before the path was allowed to
> proceed.

What to point at:

- `status: "error"`
- `policy_result: "denied"`
- `errors[0].kind: "policy-failure"`
- `errors[0].reason: "policy_denied"`

## Step 2 — Run the Approved Bounded Classification Path

Use the existing rule-selection chain:

```bash
bash examples/ai-and-inference/model-load-canonfs/run_classify_fixed_rule_selection.sh
```

Optional fixed input:

```bash
INPUT_TEXT="greet hello" \
  bash examples/ai-and-inference/model-load-canonfs/run_classify_fixed_rule_selection.sh
```

What to say:

> Now we switch to an approved bounded task. The task does not end at a text
> answer. It ends at a canonical stored bundle that downstream systems can use.

## Step 3 — Call Out the Object Chain Explicitly

During the script output, point at these objects in order:

- result artifact
- provenance artifact
- downstream rule-selection record
- final bundle artifact

Say this explicitly:

> The first three are intermediate. The bundle is the top-level persisted
> object for the completed decision.

For a financial-services audience, translate the chain this way:

- result artifact:
  bounded model-assisted classification result
- provenance artifact:
  evidence of how the result was produced
- downstream record:
  typed decision record connecting the result to a selected rule-set identity
- final bundle:
  the auditor-facing canonical object

## Step 4 — State the Control Value

Use this line:

> Most systems give you logs and traces. This system gives you a canonical
> object whose identity is tied to what ran and why it was allowed.

## Step 5 — Prove Deterministic Identity

Run the portable proof path:

```bash
./build/t81_ai_task_classify_fixed_composition_test ./build/t81
```

What to say:

> This confirms that for identical task, model, policy, and input, the result,
> provenance, downstream record, and canonical bundle keep the same
> content-addressed identity.

If you want to keep the live demo purely CLI-facing, rerun the same example
script twice and point out the repeated refs. The composition test remains the
stronger proof surface.

## Short Spoken Track

Use this script nearly verbatim:

1. "First, I’m going to show a denied path. The point is that approval happens
   before execution, not after."
2. "Now I’ll run an approved bounded classification flow."
3. "Notice that the output is not just text. It becomes a result artifact, a
   provenance artifact, a typed decision record, and then a final bundle."
4. "That final bundle is the object a downstream control or audit system would
   consume."
5. "Now I’ll show that the same inputs reproduce the same canonical object
   identity."

## What Not To Say

Do not say:

- "This replaces model risk infrastructure."
- "This is a general AI platform."
- "This is a complete operating system."
- "This solves all AI governance."
- "This is about faster model execution."

## What The Audience Should Leave With

They should be able to repeat:

- nothing runs unless it is admitted
- the result is a stored object, not just an output line
- the object is tied to provenance and policy
- the same run can be reproduced with the same identity

## Follow-On Artifact

If the live demo works, the next artifact should be a short recorded run using
this exact sequence and no broader claims.
