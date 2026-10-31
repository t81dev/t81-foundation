# Notebook CI Strategy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Notebook CI Strategy](#notebook-ci-strategy)
  - [Goal](#goal)
  - [Strategy](#strategy)
    - [1. Automated Execution](#1-automated-execution)
- [Example CI command](#example-ci-command)
    - [2. Output Verification](#2-output-verification)
    - [3. Environment Setup](#3-environment-setup)
    - [4. Failure Handling](#4-failure-handling)

<!-- T81-TOC:END -->


## Goal

Ensure that all notebooks in `/notebooks` remain executable and produce deterministic outputs consistent with the underlying C++ implementation.

## Strategy

### 1. Automated Execution

In CI, we can use `nbconvert` or `papermill` to execute the notebooks headlessly.

```bash
# Example CI command
jupyter nbconvert --to notebook --execute --ExecutePreprocessor.timeout=600 --inplace notebooks/*.ipynb
```

### 2. Output Verification

Because T81 emphasizes determinism, the outputs of cells (especially those printing hashes or traces) should be stable. We can verify this by:

*   **Hash Anchors**: Key cells should output a hash of their result (e.g., `Sequence Hash: a1b2...`).
*   **Diffing**: Compare the executed notebook against the committed version. Any diff in output cells indicates a regression or a breach of determinism.

### 3. Environment Setup

The CI runner must:

1.  Build the project with `t81_python` enabled.
    ```bash
    cmake -S . -B build -DT81_BUILD_TESTS=OFF
    cmake --build build --target t81_python
    ```
2.  Set `PYTHONPATH`.
    ```bash
    export PYTHONPATH=$PYTHONPATH:$(pwd)/build
    ```
3.  Install dependencies.
    ```bash
    pip install jupyterlab
    ```

### 4. Failure Handling

If a notebook fails to execute (raises an exception) or produces different output, the CI build should fail. This treats documentation as code, preventing drift.
