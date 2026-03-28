#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void write_le64(std::ofstream& out, uint64_t value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: t81_make_demo_safetensors <out.safetensors>\n";
    return 2;
  }

  const std::filesystem::path output = argv[1];
  const std::vector<int8_t> mat_a = {-1, 0, 1, -1};
  const std::vector<int8_t> mat_b = {1, 0, -1, 1};

  const uint64_t mat_a_offset = 0;
  const uint64_t mat_b_offset = static_cast<uint64_t>(mat_a.size());
  const std::string header =
      "{\"__metadata__\":{\"format\":\"native-ternary-i8\"},"
      "\"mat_a\":{\"dtype\":\"I8\",\"shape\":[2,2],\"data_offsets\":["
      + std::to_string(mat_a_offset) + "],\"data_lengths\":[" + std::to_string(mat_a.size()) + "]},"
      "\"mat_b\":{\"dtype\":\"I8\",\"shape\":[2,2],\"data_offsets\":["
      + std::to_string(mat_b_offset) + "],\"data_lengths\":[" + std::to_string(mat_b.size()) + "]}}";

  std::ofstream out(output, std::ios::binary);
  if (!out) {
    std::cerr << "unable to create " << output.string() << "\n";
    return 1;
  }

  write_le64(out, static_cast<uint64_t>(header.size()));
  out.write(header.data(), static_cast<std::streamsize>(header.size()));
  out.write(reinterpret_cast<const char*>(mat_a.data()), static_cast<std::streamsize>(mat_a.size()));
  out.write(reinterpret_cast<const char*>(mat_b.data()), static_cast<std::streamsize>(mat_b.size()));
  out.close();

  std::cout << "wrote=" << output.string() << "\n";
  std::cout << "tensors=mat_a,mat_b\n";
  return 0;
}
