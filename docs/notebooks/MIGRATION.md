# Notebook Migration Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Notebook Migration Guide](#notebook-migration-guide)
  - [Overview](#overview)
  - [Legacy Notebooks](#legacy-notebooks)
  - [New Track Structure](#new-track-structure)
  - [Migration Steps for Developers](#migration-steps-for-developers)
  - [FAQ](#faq)

<!-- T81-TOC:END -->


## Overview

The `/notebooks` directory has been restructured into a comprehensive interactive laboratory organized into five tracks. This new structure aligns with the T81 specification and provides a rigorous, executable reference manual.

## Legacy Notebooks

The previous notebooks have been moved to `notebooks/legacy/`:

*   `00_t81_orientation.ipynb` -> `notebooks/legacy/00_t81_orientation.ipynb`
*   `01_balanced_ternary_playground.ipynb` -> `notebooks/legacy/01_balanced_ternary_playground.ipynb`
*   `02_t81vm_trace_walkthrough.ipynb` -> `notebooks/legacy/02_t81vm_trace_walkthrough.ipynb`

These files are preserved for historical reference but are no longer maintained as the primary entry point.

## New Track Structure

Users should now start with **Track I** and progress sequentially.

1.  **Track I**: Core Numeric Substrate (BigInt, Float, Ternary)
2.  **Track II**: Data Structures (Tensors, Containers)
3.  **Track III**: VM Execution (TISC, Traps)
4.  **Track IV**: Governance (Axion, CanonFS)
5.  **Track V**: Advanced Features (Reflection, Agents)

## Migration Steps for Developers

1.  **Update References**: Point any documentation or internal wikis to `notebooks/README.md`.
2.  **Build Bindings**: Ensure you build the `t81_python` target (`cmake --build build --target t81_python`) to run the new notebooks.
3.  **Verify Environment**: Check that your `PYTHONPATH` includes the build directory.

## FAQ

**Q: Where is the balanced ternary playground?**
A: See `00_Numeric_Foundations.ipynb`.

**Q: Where is the VM trace demo?**
A: See `06_TISC_and_VM_Execution.ipynb`.
