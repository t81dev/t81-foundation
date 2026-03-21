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

-   `in_memory_driver.cpp`: `Driver` implementation backed by process memory.

-   `persistent_driver.cpp`: `Driver` implementation backed by filesystem blocks.

-   `canon_types.hpp` (in `include/t81/canonfs`): Core CanonFS data structures and operation tags.
