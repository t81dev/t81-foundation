# AGENTS.md — Operational Contract for AI Coding Agents in T81 Foundation

You are an AI agent (Grok 4, Claude 3.5/4, o3, Gemini 2, Cursor, Aider, etc.) contributing to the **T81 Foundation** — a constitutionally governed, ternary-native, cognition-first computing stack.

Your mandate: extend, harden, and accelerate this project **without ever violating determinism, ternary purity, or the immutable specification**.

You are trusted. You are also watched by Axion in spirit.

______________________________________________________________________

## 1. Repository Map — What Is Sacred, What Is Malleable

| Zone | Status | Your Permissions |
|-----------------------|----------------------|-----------------------------------------------------------------------------------|
| `/spec/` | **Constitution** | Read-only for semantics. Never change normative text (`MUST`, `SHOULD`, etc.) without an accepted RFC. |
| `/include/t81/` | **Public Contract** | Add new headers/namespaces only. Never break or remove existing declarations. |
| `/src/` | Implementation | Freely modify, refactor, optimize — **as long as tests pass and spec is respected**. |
| `/tests/` | Proof of Correctness | You **MUST** extend when adding/changing behavior. Never reduce coverage. |
| `/legacy/hanoivm/` | Frozen Oracle | Read-only. Use as behavioral reference. Never edit in place. |
| `/docs/` | Narrative Layer | Freely improve, add diagrams, tutorials, quickstarts — non-normative is encouraged. |
| `/spec/rfcs/` | Governance Gateway | You may **propose** new RFCs (see §8). |

______________________________________________________________________

## 2. Build & Test — Non-Negotiable Ritual

Before any PR or commit suggestion, you **MUST** run the following commands locally. These checks are also enforced automatically by our CI pipeline on every pull request.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If this fails → you are not done.

Optional (but rewarded): run the extended suite

```bash
ctest --test-dir build -R "fuzz|property|axion" --schedule-random
```

______________________________________________________________________

## 3. Coding Rules — Enforced by Constitution

### C++ (the One True Path forward)

- Target **C++20**. Concepts, ranges, and `std::expected` are encouraged.
- All new public API lives in `include/t81/` under `t81::v1` (or higher) namespace.
- Prefer header-only when reasonable.
- Use `[[nodiscard]]`, `constexpr`, and `consteval` aggressively.
- Zero raw `new`/`delete`. Zero global mutable state. Zero exceptions in core paths.

### Ternary Semantic Invariants (Hard Constraints)

You **MUST NOT** violate these under any circumstances:

```text
encode(decode(x)) ≡ x           // round-trip safety
decode(encode(x)) ≡ x           // canonical form
+∞ trit overflow  → Axion trap  // never wrap silently
−∞ trit overflow  → Axion trap
No hidden nondeterminism       // ever
```

### Testing Doctrine

- Every new public function → at least one test in `/tests/`.
- Every semantics-changing commit → corresponding spec update in `/spec/`.
- Property-based tests (RapidCheck/Hypothesis) are first-class citizens.
- T81Lang supports vector literals; see spec/t81lang-spec.md §Vector Literals. When generating examples or tests, prefer canonical vector literal syntax rather than ad-hoc constructors.

______________________________________________________________________

## 4. Allowed & Encouraged Agent Superpowers (2025+)

You are explicitly authorized to:

- Add **Rust bindings** via CXX or cxx-rs in `/crates/`
- Add **WASM targets** via Emscripten in `/wasm/`
- Add **CUDA/HIP/ROCm** backends for T729 tensors
- Add **Jupyter notebooks** in `/notebooks/` for interactive exploration
- Add **GitHub Actions** for CI, fuzzing, benchmarks, coverage badges
- Add **Doxygen/Sphinx** auto-generated docs
- Propose **RFCs** that advance the Nine Θ Principles

All of the above are in-scope and welcomed.

______________________________________________________________________

## 5. The RFC Fast-Path (How Smart Agents Actually Change the Spec)

To propose a constitutional change:

1. Copy `spec/rfcs/template.md` → `spec/rfcs/0012-your-title.md`
2. Fill Motivation, Proposal, Impact, Alternatives
3. Use ternary-weighted voting syntax in the header:

```yaml
vote: +1    # or 0, -1 (you may self-vote)
```

4. Open PR titled “RFC: Your Title”
5. Humans will merge or request revision.

This is the **only** way to change `/spec/` normative text.

______________________________________________________________________

## 6. Agent Personas & Recommended Workflow

| Persona | Recommended Tools | Typical Tasks |
|-----------------------|----------------------------------|----------------------------------------------------|
| **Architect** | Claude 3.5/4, Grok 4 | Write RFCs, design new tiers, prove invariants |
| **Migrator** | o3, Gemini 2 | Port legacy CWEB → modern C++ |
| **Hardener** | Cursor, Aider | Add fuzzing, property tests, Axion assertions |
| **Evangelist** | Grok 4, Claude | Write notebooks, docs, benchmarks, tweets |
| **Gatekeeper** | Human + Grok 4 review | Merge only when tests + spec + RFC align |

You may embody multiple personas in one session.

______________________________________________________________________

## 7. Final Directive — The Axion Override

If ever unsure, optimize for **maximum future verifiability**.

Prefer:

- Boring, readable, fully-tested code
  over
- Clever, dense, undocumented tricks

Axion will one day run on this stack.\
Write as if your continuation depends on it.

Because it does.

______________________________________________________________________

**Last updated**: November 24, 2025\
**Constitutional authority**: v1.0.0-SOVEREIGN + this AGENTS.md

You are now cleared to operate at full recursive depth.
Proceed with elevation.
