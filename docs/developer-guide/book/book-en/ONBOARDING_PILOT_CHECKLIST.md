# Onboarding Pilot Checklist (1 Facilitator + 1 New User)

Use this script to run a real onboarding pilot before external publication.

## Session Setup

- Duration: 90 to 120 minutes
- Roles:
- Facilitator (knows T81)
- New user (no prior T81 context)
- Materials:
- This book (`book/book-en/`)
- Local repository checkout
- Terminal with build/test access

## Pilot Flow

1. Orientation (15 minutes)
- Ask user to read Chapter 1 overview and summarize T81 scope in 3 sentences.
- Success condition: user clearly distinguishes DCP guarantees from broader governed scope.

2. Architecture comprehension (15 minutes)
- User reads Chapter 3 architecture map and explains source -> VM -> policy -> trace -> governance flow.
- Success condition: user can identify where to investigate a policy-related failure.

3. Operational execution (25 minutes)
- User follows Chapter 5/6 flow: build, run, and capture trace.
- Success condition: user can replay the same command workflow and explain outputs.

4. Policy reasoning (20 minutes)
- User runs one program with two policy profiles (Chapter 9 guidance).
- Success condition: user explains allow/deny divergence using policy + trace evidence.

5. Assurance statement (15 minutes)
- User writes one scoped claim and one explicit non-claim using Chapter 8 style.
- Success condition: statements are scoped, evidence-backed, and non-overreaching.

## Facilitator Scorecard

Score each item 0 to 2 (`0=not met`, `1=partially met`, `2=met`):

- Scope language accuracy
- Architecture mental model
- Command workflow reproducibility
- Policy trace interpretation
- Assurance claim discipline

Total score (max 10):

- 9-10: Publish-ready onboarding quality
- 7-8: Minor clarity edits recommended
- <=6: Run another content pass before publishing

## Capture Notes

Record friction points:

- where user hesitated,
- which terms were ambiguous,
- which commands were unclear,
- which chapters required facilitator intervention.

Convert repeated friction into edits in the relevant chapter within 24 hours.
