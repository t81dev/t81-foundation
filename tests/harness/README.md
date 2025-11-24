# T81 Test Harness

This directory contains the official testing infrastructure for T81 implementations.

## Goals

- Validate deterministic execution
- Validate TISC instruction semantics
- Validate VM memory model
- Validate canonical data types
- Validate Axion privileged boundaries
- Validate shape/tensor safety
- Validate recursion and reasoning limits

## Running

./run_all.sh

This:

1. runs each test vector
1. captures deterministic traces
1. re-runs the same program
1. validates 100% identical trace output
1. performs canonicalization checks
1. validates fault behavior

For CI use, simply call `run_all.sh` from GitHub Actions.

## Adding Tests

Place `.t81` bytecode files or `.t81lang` source files inside `test_vectors/`.
