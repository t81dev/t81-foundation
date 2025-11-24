# t81/axion — Minimal Axion C++ Façade (Stub)

A tiny, dependency-free C++ layer that allows code to compile and unit tests to
exercise “Axion-like” calls while the real backend remains in `legacy/`.

## Files

- `api.hpp`
  - `DeviceKind`, `Device`
  - `Request` / `Response`
  - `Context::run(const Request&)` (supports `"dot"` on two vectors)

## Example

```cpp
#include <t81/axion/api.hpp>
#include <t81/tensor.hpp>

t81::T729Tensor a({3}); a.data() = {1,2,3};
t81::T729Tensor b({3}); b.data() = {4,5,6};

t81::axion::Context ctx({t81::axion::DeviceKind::CPU, 0, "cpu0"});
t81::axion::Request req; req.op = "dot"; req.inputs = {a,b};
t81::axion::Response r = ctx.run(req);
// r.ok == true, r.outputs[0] is {1} with value 32.0f
```

## Notes

- The stub only implements `"dot"` for two 1-D tensors.
- Extend by adding new `req.op` handlers inside `Context::run`.
- Keep the API stable so downstream callers can link against either the stub
  or a real backend without code changes.
