# T81 C API (Compatibility Layer)

Thin, stable C façade that wraps the modern C++ headers for legacy consumers.

## Headers

```c
#include "src/c_api/t81_c_api.h"
```

## ABI Handles

Opaque pointers are used to keep the ABI stable:

```c
typedef struct t81_bigint_s* t81_bigint;
```

## Functions

```c
// Construct from canonical base-243 string (digits 0..242 separated by '.')
t81_bigint t81_bigint_from_ascii(const char* s);

// Convert to string (caller must free() the returned buffer)
char*      t81_bigint_to_string(t81_bigint h);

// Release
void       t81_bigint_free(t81_bigint h);
```

## Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "src/c_api/t81_c_api.h"

int main() {
  t81_bigint a = t81_bigint_from_ascii("1.42.7"); // base-243 digits (MSB-first)
  t81_bigint b = t81_bigint_from_ascii("0.0.1");
  char* sa = t81_bigint_to_string(a);
  char* sb = t81_bigint_to_string(b);
  printf("A=%s\nB=%s\n", sa, sb);
  free(sa); free(sb);
  t81_bigint_free(a); t81_bigint_free(b);
  return 0;
}
```

## Build

- **CMake**: included via target `t81` (headers) and `t81_io` (IO .cpp).
- **Bazel**: `//:t81` provides headers and required sources.

> For new functionality, add C wrappers here that delegate to C++ types under `include/t81/`.

```
```
