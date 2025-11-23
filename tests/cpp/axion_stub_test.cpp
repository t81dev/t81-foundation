#include <cassert>
#include <iostream>
#include "t81/axion/api.hpp"
#include "t81/tensor.hpp"

int main() {
  using namespace t81;

  // Build two vectors for dot
  T729Tensor a({3}); a.data() = {1, 2, 3};
  T729Tensor b({3}); b.data() = {4, 5, 6};

  axion::Context ctx(axion::Device{axion::DeviceKind::CPU, 0, "cpu0"});
  axion::Request req;
  req.op = "dot";
  req.inputs = {a, b};

  axion::Response resp = ctx.run(req);
  assert(resp.ok);
  assert(resp.outputs.size() == 1);
  assert(resp.outputs[0].rank() == 1);
  assert(resp.outputs[0].shape()[0] == 1);

  float dot = resp.outputs[0].data()[0];
  // 1*4 + 2*5 + 3*6 = 32
  assert(dot == 32.0f);

  // Negative test: wrong arity
  axion::Request bad;
  bad.op = "dot";
  bad.inputs = {a};
  auto r2 = ctx.run(bad);
  assert(!r2.ok);

  std::cout << "axion_stub ok\n";
  return 0;
}
