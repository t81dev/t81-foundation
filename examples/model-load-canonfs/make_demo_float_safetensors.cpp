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
    std::cerr << "usage: t81_make_demo_float_safetensors <out.safetensors>\n";
    return 2;
  }

  const std::filesystem::path output = argv[1];
  const std::vector<float> values = {-2.0f, -0.1f, 0.2f, 3.0f};
  const std::string header =
      "{\"__metadata__\":{\"architecture\":\"llama\",\"llama.block_count\":1,"
      "\"llama.embedding_length\":2,\"llama.attention.head_count\":1},"
      "\"model.layers.0.self_attn.q_proj.weight\":{\"dtype\":\"F32\",\"shape\":[2,2],"
      "\"data_offsets\":[0],\"data_lengths\":[" + std::to_string(values.size() * sizeof(float)) + "]}}";

  std::ofstream out(output, std::ios::binary);
  if (!out) {
    std::cerr << "unable to create " << output.string() << "\n";
    return 1;
  }

  write_le64(out, static_cast<uint64_t>(header.size()));
  out.write(header.data(), static_cast<std::streamsize>(header.size()));
  out.write(reinterpret_cast<const char*>(values.data()),
            static_cast<std::streamsize>(values.size() * sizeof(float)));
  out.close();

  std::cout << "wrote=" << output.string() << "\n";
  std::cout << "arch=llama\n";
  std::cout << "tensor=model.layers.0.self_attn.q_proj.weight\n";
  return 0;
}
