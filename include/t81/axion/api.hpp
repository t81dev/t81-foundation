#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "t81/tensor.hpp"
#include "t81/bigint.hpp"
#include "t81/fraction.hpp"

namespace t81::axion {

// Minimal C++ façade for Axion integration (stub).
// Replace internals with real backend bindings when available.

enum class DeviceKind : uint8_t { CPU = 0, GPU = 1 };
struct Device {
  DeviceKind kind{DeviceKind::CPU};
  int        index{0};             // GPU index if kind==GPU
  std::string name{};              // optional descriptive label
};

struct Request {
  std::string op;                  // e.g., "dot", "mul", "conv2d"
  std::vector<T729Tensor> inputs;  // tensor inputs
  std::vector<float>      scalars; // optional scalar params
  std::string             meta;    // free-form JSON/meta
};

struct Response {
  bool ok{false};
  std::vector<T729Tensor> outputs;
  std::string error;               // non-empty if ok == false
};

// Context for submitting requests. In a real implementation this would
// own handles to threads/streams/queues.
class Context {
public:
  Context() = default;
  explicit Context(Device dev) : dev_(std::move(dev)) {}

  const Device& device() const { return dev_; }

  // Synchronous stub execution. A real backend would dispatch to GPU/driver.
  Response run(const Request& req) const {
    if (req.op == "dot") {
      if (req.inputs.size() != 2) return fail_("dot expects 2 inputs");
      const auto& a = req.inputs[0];
      const auto& b = req.inputs[1];
      try {
        auto out = T729Tensor::contract_dot(a, b);
        return Response{true, {std::move(out)}, {}};
      } catch (const std::exception& e) {
        return Response{false, {}, e.what()};
      }
    }
    // Unknown op → error
    return fail_("unsupported op: " + req.op);
  }

private:
  static Response fail_(std::string msg) { return Response{false, {}, std::move(msg)}; }
  Device dev_{};
};

} // namespace t81::axion
