# Changelog

All notable changes to the T81 Foundation should be documented in this file. Follow the [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) style and add entries under the appropriate headings.

## [Unreleased]

- add a high-rank tensor demo and a graph demo to the `examples/` directory plus documentation so the suite of data-type demos now illustrates tensor indexing and adjacency matrix handling
- expand `docs/guides/data-types-overview.md`, `docs/guides/demo-gallery.md`, `docs/index.md`, and `README.md` with descriptions and CLI commands for the new demos to keep the guides and gallery synchronized
- ensure `scripts/run-demos.sh` compiles and runs the full demo suite (match → quaternion → high-rank tensor → graph) and document that automation with cross-references
- run the required build/test workflow (`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, `cmake --build build --parallel`, `ctest --test-dir build --output-on-failure`) and document completion of the mandated release checks
