#include "t81/weights.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::vector<std::byte> read_all_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  assert(in.good());
  std::vector<std::byte> bytes;
  char ch = 0;
  while (in.get(ch)) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

void write_le64(std::ofstream& out, uint64_t value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

fs::path make_temp_path(const std::string& stem, const std::string& ext) {
  return fs::temp_directory_path() / (stem + ext);
}

void write_llama_profile_safetensors(const fs::path& path) {
  std::vector<float> values;
  values.reserve(256);
  for (size_t i = 0; i < 256; ++i) {
    values.push_back((i % 5 == 0) ? -1.75f : ((i % 7 == 0) ? 1.75f : 0.0f));
  }

  const std::string header =
      "{\"__metadata__\":{\"architecture\":\"llama\",\"llama.block_count\":32,"
      "\"llama.embedding_length\":4096,\"llama.attention.head_count\":32},"
      "\"model.layers.0.self_attn.q_proj.weight\":{\"dtype\":\"F32\",\"shape\":[16,16],"
      "\"data_offsets\":[0],\"data_lengths\":[" +
      std::to_string(values.size() * sizeof(float)) + "]}}";

  std::ofstream out(path, std::ios::binary);
  assert(out.good());
  write_le64(out, header.size());
  out.write(header.data(), static_cast<std::streamsize>(header.size()));
  out.write(reinterpret_cast<const char*>(values.data()),
            static_cast<std::streamsize>(values.size() * sizeof(float)));
}

}  // namespace

int main() {
  const fs::path t81w_a = make_temp_path("t81-native-metrics-a", ".t81w");
  const fs::path t81w_b = make_temp_path("t81-native-metrics-b", ".t81w");
  const fs::path source_sf = make_temp_path("t81-native-metrics-source", ".safetensors");
  const fs::path converted_a = make_temp_path("t81-native-metrics-converted-a", ".t81w");
  const fs::path converted_b = make_temp_path("t81-native-metrics-converted-b", ".t81w");

  t81::weights::NativeTensor tensor;
  tensor.shape = {128, 192};
  tensor.trits = 128 * 192;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.data.reserve((tensor.trits + 47) / 48);
  for (uint64_t i = 0; i < (tensor.trits + 47) / 48; ++i) {
    tensor.data.push_back((i % 2 == 0) ? 0x123456789abcdef0ULL : 0x0fedcba987654321ULL);
  }

  t81::weights::NativeModel model;
  model.emplace("dense.weight", tensor);

  t81::weights::save_t81w(model, t81w_a);
  t81::weights::save_t81w(model, t81w_b);

  const auto bytes_a = read_all_bytes(t81w_a);
  const auto bytes_b = read_all_bytes(t81w_b);
  assert(bytes_a == bytes_b);

  const auto loaded = t81::weights::load_t81w(t81w_a);
  assert(loaded.native.contains("dense.weight"));
  assert(loaded.bits_per_trit > 0.0);
  assert(loaded.bits_per_trit < 1.6);
  assert(loaded.sparsity >= 0.0);
  assert(loaded.sparsity <= 1.0);

  write_llama_profile_safetensors(source_sf);
  const auto imported_a = t81::weights::load_safetensors(source_sf);
  const auto imported_b = t81::weights::load_safetensors(source_sf);
  assert(imported_a.format == "SafeTensors(arch-profile=llama-dense-v1)");
  assert(imported_b.format == "SafeTensors(arch-profile=llama-dense-v1)");

  t81::weights::save_t81w(imported_a.native, converted_a);
  t81::weights::save_t81w(imported_b.native, converted_b);

  const auto converted_bytes_a = read_all_bytes(converted_a);
  const auto converted_bytes_b = read_all_bytes(converted_b);
  assert(converted_bytes_a == converted_bytes_b);

  const auto converted_loaded = t81::weights::load_t81w(converted_a);
  assert(converted_loaded.bits_per_trit > 0.0);
  assert(converted_loaded.bits_per_trit < 1.8);
  assert(converted_loaded.sparsity >= 0.0);
  assert(converted_loaded.sparsity <= 1.0);

  fs::remove(t81w_a);
  fs::remove(t81w_b);
  fs::remove(source_sf);
  fs::remove(converted_a);
  fs::remove(converted_b);
  return 0;
}
