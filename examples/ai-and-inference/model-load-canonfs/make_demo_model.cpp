#include "t81/weights.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: t81_make_demo_model <out.t81w>\n";
    return 2;
  }

  t81::weights::NativeModel model;

  t81::weights::NativeTensor mat_a;
  mat_a.shape = {2, 2};
  mat_a.trits = 4;
  mat_a.data = {40};
  model["mat_a"] = mat_a;

  t81::weights::NativeTensor mat_b;
  mat_b.shape = {2, 2};
  mat_b.trits = 4;
  mat_b.data = {67};
  model["mat_b"] = mat_b;

  const std::filesystem::path output = argv[1];
  t81::weights::save_t81w(model, output);

  const auto loaded = t81::weights::load_t81w(output);
  std::cout << "wrote=" << output.string() << "\n";
  std::cout << "sha3-512=" << loaded.checksum << "\n";
  return 0;
}
