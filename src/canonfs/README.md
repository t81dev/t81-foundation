# Canonical File System (CanonFS)

This directory contains the source code for the Canonical File System (CanonFS), a content-addressed file system designed for deterministic, verifiable, and permanent storage.

## Core Principles

CanonFS is built on a few core principles:

-   **Content-Addressing:** All files and directories are identified by a cryptographic hash of their contents (a `CanonicalId`), rather than by a mutable path. This means that if the content changes, the identifier changes, ensuring data integrity and eliminating ambiguity.

-   **Immutability:** Data written to CanonFS is immutable. There are no "overwrite" or "delete" operations in the traditional sense. Instead, new versions of files are created, and old versions remain accessible as long as they are referenced.

-   **Verifiability:** Because all data is content-addressed, the integrity of a file or an entire directory tree can be verified at any time by re-calculating the hashes.

-   **Deduplication:** Content-addressing naturally leads to data deduplication. If two files have the same content, they will have the same hash and will therefore only be stored once.

## Current Status

The CanonFS implementation is currently in an early, **experimental** stage. The public API is defined in `include/t81/canonfs`, and a basic in-memory driver is provided for testing, but a persistent, on-disk storage layer has not yet been implemented.

## Components

-   `in_memory_driver.cpp`: An implementation of the `ICanonDriver` interface (defined in `include/t81/canonfs/canon_driver.hpp`) that stores all data in memory. This is useful for testing and prototyping but is not suitable for production use.

-   `canon_types.hpp` (in `include/t81/canonfs`): Defines the core data structures of CanonFS, such as `CanonId` and directory entry types.
