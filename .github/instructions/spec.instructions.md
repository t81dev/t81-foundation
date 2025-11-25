# Copilot Instructions for `spec/` Directory

These instructions apply when editing files under the `spec/` directory.

1. **Treat spec files as normative**

   - They define how T81 must behave; code should follow them, not the other way around.
   - Avoid speculative implementation details that are not yet agreed.

2. **Write clearly and precisely**

   - Prefer unambiguous language, using definitions, numbered lists, and explicit MUST/SHOULD/MAY where appropriate.
   - Keep terminology consistent across spec files (e.g., “TISC”, “T81VM”, “Axion kernel”).

3. **Maintain structure**

   - Each spec should have: Scope, Definitions, Invariants, Semantics, and Extensibility sections where appropriate.
   - Cross-link to other spec documents (e.g., from VM spec to data-types spec).

4. **Couple changes with implementation**

   - When suggesting spec changes that affect behavior, also suggest corresponding changes in:
     - `include/t81/` (API),
     - `src/` (implementation),
     - `tests/cpp/` (tests).

5. **Never silently weaken constraints**

   - Do not remove safety or determinism requirements without calling this out in comments or commit messages.
