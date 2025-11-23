
# Examples

Small, self-contained samples for the C++ API.

## Build & Run (CMake)
```bash
cmake -S . -B build -DT81_BUILD_EXAMPLES=ON
cmake --build build
./build/t81_demo
./build/t81_tensor_ops
./build/t81_ir_roundtrip
````

## Files

* `demo.cpp` — basic BigInt + Tensor dot example.
* `tensor_ops.cpp` — transpose, slice, reshape on a small matrix.
* `ir_roundtrip.cpp` — encode/decode a tiny IR program and print it.
