Here’s `tests/cpp/README.md`:

````md
# C++ Tests

This folder contains small, dependency-light unit tests for the new C++ API.

## Building

### CMake
```bash
cmake -S . -B build -DT81_BUILD_TESTS=ON
cmake --build build
````

### Make (shim)

```bash
make tests
make run-tests
```

### Bazel

```bash
bazel test //:t81_*_test
```

## Test List

* `bigint_roundtrip.cpp` — exercises `T81BigInt` add/mul and JSON vectors (`tests/harness/canonical/bigint.json`).
* `fraction_roundtrip.cpp` — exercises `T81Fraction` add/mul and (optional) reduced expectations (`fraction.json`).
* `tensor_transpose_test.cpp` — tests `ops::transpose`.
* `tensor_slice_test.cpp` — tests `ops::slice2d`.
* `tensor_reshape_test.cpp` — tests `ops::reshape` (with `-1` inference).
* `tensor_loader_test.cpp` — roundtrip text IO for tensors.
* `canonfs_io_test.cpp` — 99-byte wire encode/decode for `CanonRef`.
* `ir_encoding_test.cpp` — 32-byte instruction encode/decode roundtrip.
* `hash_stub_test.cpp` — base-81 stub and CanonHash81 stub checks.

> Note: `bigint_roundtrip.cpp` and `fraction_roundtrip.cpp` expect `nlohmann/json` header.
> You can vendor it under `third_party/` and add `-Ithird_party` to your include path.

```
```
