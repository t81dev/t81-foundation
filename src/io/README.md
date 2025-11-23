# t81 IO Utilities

Small C++ helpers that require compilation (live under `src/io/`) and pair with
header APIs in `include/t81/io/`.

## Tensor Text IO

**Headers**
```cpp
#include <t81/io/tensor_loader.hpp>
````

**Implementation**

* `src/io/tensor_loader.cpp`

**Format**

```
RANK D1 ... DR
v0 v1 v2 ... v{D1*...*DR-1}
```

Values are read/written in row-major order.

**APIs**

```cpp
t81::T729Tensor t81::io::load_tensor_txt(std::istream& in);
t81::T729Tensor t81::io::load_tensor_txt_file(const std::string& path);

void t81::io::save_tensor_txt(std::ostream& out, const t81::T729Tensor& t);
void t81::io::save_tensor_txt_file(const std::string& path, const t81::T729Tensor& t);
```

## Build

### CMake

The `t81_io` target compiles IO sources and links against the header-only `t81`:

```cmake
add_library(t81_io STATIC
  src/io/tensor_loader.cpp
)
target_link_libraries(t81_io PUBLIC t81)
```

### Bazel

`//:t81` already includes `src/io/tensor_loader.cpp` in its `srcs`.

## Tests

* `tests/cpp/tensor_loader_test.cpp` — roundtrip save/load in memory.

```
```
