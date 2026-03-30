# Stale Claims Register

| Document Path | Quoted or Summarized Claim | Classification | Evidence | Recommended Correction |
|---|---|---|---|---|
| `docs/architecture/OVERVIEW.md` | `"TISC ISA" -> core/isa/; "VM Interpreter" -> core/vm/` | CONTRADICTED | `ls -d isa vm` shows these are at repo root, not under `core/`. | Update mapping instructions and paths in the table to use `isa/`, `vm/`, `include/t81/types/` etc. |
| `docs/architecture/OVERVIEW.md` | `Data types -> core/types/` | CONTRADICTED | No `core/` or `types/` at root. Types are likely in `include/t81/types/` or `include/t81/data_types/` or `lang/`. | Check `include/` and adjust path to `include/t81/types/`. |
| `docs/architecture/OVERVIEW.md` | `VM interpreter -> core/vm/` | CONTRADICTED | Actual path is `vm/`. | Change to `vm/`. |
| `docs/architecture/OVERVIEW.md` | `CanonFS -> src/canonfs/ + include/t81/canonfs/` | CONTRADICTED | Actual path is `fs/` + `include/t81/canonfs/` | Change `src/canonfs/` to `fs/`. |
| `scripts/governance/check_docs_structure.py` | `book/book-en` in BOOK_DIRS | STALE | Directory `book` is actually inside `docs/book` | Prefix with `docs/` |
