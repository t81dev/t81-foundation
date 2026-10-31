# Multilingual Governance Policy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Multilingual Governance Policy](#multilingual-governance-policy)
  - [1. Canonical Source of Truth](#1-canonical-source-of-truth)
  - [2. Directory Structure](#2-directory-structure)
  - [3. Translation Synchronization Workflow](#3-translation-synchronization-workflow)
    - [3.1. The "Diff-Propagation" Process](#31-the-"diff-propagation"-process)
    - [3.2. Version Tagging](#32-version-tagging)
  - [4. Automation & CI Enforcement](#4-automation-&-ci-enforcement)
    - [4.1. Structural Integrity Check](#41-structural-integrity-check)
    - [4.2. Staleness Detection](#42-staleness-detection)
  - [5. Contribution Guidelines for Translators](#5-contribution-guidelines-for-translators)
  - [6. Adding a New Language](#6-adding-a-new-language)

<!-- T81-TOC:END -->


## 1. Canonical Source of Truth

**English (`en`) is the single source of truth for all T81 Foundation documentation and specifications.**

*   All architectural decisions, specification changes, and governance policies must be authored and merged in English first.
*   Translations are considered **derivative works**. In the event of a conflict or ambiguity, the English version prevails.

## 2. Directory Structure

To prevent structural drift, all language-specific documentation must mirror the English directory structure exactly.

```
/book
  /book-en (Canonical)
  /book-cn (Chinese)
  /book-es (Spanish)
  /book-pt (Portuguese)
  /book-ru (Russian)
```

Root-level READMEs must follow this naming convention:
*   `README.md` (English/Canonical)
*   `README.es.md`
*   `README.pt-BR.md`
*   `README.ru.md`
*   `README.zh-CN.md`

## 3. Translation Synchronization Workflow

### 3.1. The "Diff-Propagation" Process
When the canonical English documentation is updated:
1.  The commit hash of the update is recorded.
2.  A tracking issue is automatically generated (or manually created) for each supported language, referencing the diff.
3.  Translators apply the semantic changes to the target language files.

### 3.2. Version Tagging
Each translated file should include a header metadata block indicating the English version it corresponds to:

```markdown
---
source_commit: <git-sha-of-english-version>
translation_status: [up-to-date | pending | outdated]
translator: @username
---
```

## 4. Automation & CI Enforcement

### 4.1. Structural Integrity Check
A CI job will run on every PR to ensure that:
*   For every file in `docs/developer-guide/book/book-en/`, a corresponding file exists in `docs/developer-guide/book/book-*/` (or is explicitly ignored via `.t81ignore`).
*   No "orphan" files exist in translation directories that do not have an English counterpart.

### 4.2. Staleness Detection
The CI system will flag translations that are significantly behind the canonical version (e.g., > 10 commits or > 30 days) as `outdated` in the generated documentation site.

## 5. Contribution Guidelines for Translators

*   **Do not change the structure**: Do not add new sections or reorder existing ones.
*   **Code blocks**: Do not translate code comments unless they are purely explanatory. Variable names and keywords must remain in English/TISC.
*   **Glossary**: Use the official T81 Multilingual Glossary (to be created) for technical terms.

## 6. Adding a New Language

To propose a new language:
1.  Submit an RFC.
2.  Commit to maintaining the translation for at least 6 months.
3.  Demonstrate a complete translation of `README.md` and `docs/developer-guide/book/book-en/chapter-01`.
