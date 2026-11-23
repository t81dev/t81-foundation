# Canonical File System (CanonFS)

This directory contains the source code for the Canonical File System (CanonFS), a content-addressed file system designed for deterministic, verifiable, and permanent storage.

## Core Principles

CanonFS is built on a few core principles:

-   **Content-Addressing:** All files and directories are identified by a cryptographic hash of their contents (a `CanonicalId`), rather than by a mutable path. This means that if the content changes, the identifier changes, ensuring data integrity and eliminating ambiguity.

-   **Immutability:** Data written to CanonFS is immutable. There are no "overwrite" or "delete" operations in the traditional sense. Instead, new versions of files are created, and old versions remain accessible as long as they are referenced.

-   **Verifiability:** Because all data is content-addressed, the integrity of a file or an entire directory tree can be verified at any time by re-calculating the hashes.

-   **Deduplication:** Content-addressing naturally leads to data deduplication. If two files have the same content, they will have the same hash and will therefore only be stored once.

## Current Status

CanonFS has both:

- `in_memory_driver.cpp` for deterministic test/prototyping paths.
- `persistent_driver.cpp` for disk-backed storage under `objects/`, `caps/`, and `parity/`.

Both drivers enforce content addressing on write and perform hash re-verification on read by default. Read verification can be disabled for diagnostics via `T81_CANONFS_READ_VERIFY=0`.

## Components

### Core CanonFS Storage Drivers & RFC-00D1 Interchange (Stable / Active Lane)

- `in_memory_driver.cpp`: `Driver` implementation backed by process memory.
- `persistent_driver.cpp`: `Driver` implementation backed by filesystem blocks under `objects/`, `caps/`, and `parity/`.
- `block_backed_driver.cpp`: Block-device-backed `Driver` adapter.
- `canonfs_interchange.cpp`, `canonfs_interchange_engine.cpp`, `canonfs_interchange_ops.cpp`, `canonfs_json_renderer.cpp`, `canonfs_enhanced_errors.cpp`: Draft RFC-00D1 foreign filesystem import/export interchange, provenance/result JSON rendering, and structured error diagnostics.
- `canon_types.hpp` (in `include/t81/canonfs`): Core CanonFS data structures and operation tags.

### Experimental Storage Optimizers (Non-Core / Research-Only)

- `auto_optimizer.cpp`, `canonical_optimization_decision.cpp`, `deep_learning_optimizer.cpp`, `ternary_optimization.cpp`: Experimental storage-layout prototypes; not part of the accepted RFC-00D1 interchange contract. (Unreferenced CLI/monitor/optimizer prototypes `auto_optimization_cli.cpp`, `canonfs_performance_monitor.cpp`, `ml_optimization_cli.cpp`, `ml_optimizer.cpp`, `performance_analyzer.cpp`, and `performance_cli.cpp` were archived under `archive/legacy_orphans/fs/`.)
