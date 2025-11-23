#include <iostream>
#include "t81/axion/api.hpp"
#include "t81/tensor.hpp"

int main() {
  using namespace t81;

  // Two 3D vectors
  T729Tensor a({3}); a.data() = {1, 2, 3};
  T729Tensor b({3}); b.data() = {4, 5, 6};

  axion::Context ctx({axion::DeviceKind::CPU, 0, "cpu0"});

  axion::Request req;
  req.op = "dot";
  req.inputs = {a, b};

  auto resp = ctx.run(req);
  if (!resp.ok) {
    std::cerr << "axion error: " << resp.error << "\n";
    return 1;
  }

  std::cout << "dot=" << resp.outputs[0].data()[0] << "\n"; // 32
  return 0;
}
