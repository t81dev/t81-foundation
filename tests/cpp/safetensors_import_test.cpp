#include "t81/weights.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_path(const std::string& stem) {
  return fs::temp_directory_path() / (stem + ".safetensors");
}

void write_le64(std::ofstream& out, uint64_t value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void write_safetensors_file(const fs::path& path,
                            const std::string& metadata_json,
                            const std::vector<int8_t>& values,
                            const std::string& tensor_name = "weight") {
  constexpr uint64_t kDataOffset = 256;
  const std::string header =
      "{\"__metadata__\":" + metadata_json +
      ",\"" + tensor_name + "\":{\"dtype\":\"I8\",\"shape\":[2,2],\"data_offsets\":[" +
      std::to_string(kDataOffset) +
      "],\"data_lengths\":[" + std::to_string(values.size()) + "]}}";
  assert(8 + header.size() <= kDataOffset);

  std::ofstream out(path, std::ios::binary);
  assert(out.good());
  write_le64(out, header.size());
  out.write(header.data(), static_cast<std::streamsize>(header.size()));

  std::vector<char> padding(static_cast<size_t>(kDataOffset - 8 - header.size()), '\0');
  out.write(padding.data(), static_cast<std::streamsize>(padding.size()));
  out.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(values.size()));
}

void write_f32_safetensors_file(const fs::path& path,
                                const std::string& metadata_json,
                                const std::vector<float>& values,
                                const std::string& tensor_name = "weight",
                                const std::string& shape_json = "[2,2]") {
  const std::string header =
      "{\"__metadata__\":" + metadata_json +
      ",\"" + tensor_name + "\":{\"dtype\":\"F32\",\"shape\":" + shape_json + ",\"data_offsets\":[0],\"data_lengths\":[" +
      std::to_string(values.size() * sizeof(float)) + "]}}";

  std::ofstream out(path, std::ios::binary);
  assert(out.good());
  write_le64(out, header.size());
  out.write(header.data(), static_cast<std::streamsize>(header.size()));
  out.write(reinterpret_cast<const char*>(values.data()),
            static_cast<std::streamsize>(values.size() * sizeof(float)));
}

struct F32TensorFixture {
  std::string name;
  std::string shape_json = "[2,2]";
  std::vector<float> values;
};

void write_multi_f32_safetensors_file(const fs::path& path,
                                      const std::string& metadata_json,
                                      const std::vector<F32TensorFixture>& tensors) {
  std::string header = "{\"__metadata__\":" + metadata_json;
  uint64_t data_offset = 0;
  for (size_t i = 0; i < tensors.size(); ++i) {
    const auto& tensor = tensors[i];
    header += ",\"" + tensor.name + "\":{\"dtype\":\"F32\",\"shape\":" + tensor.shape_json +
              ",\"data_offsets\":[" + std::to_string(data_offset) + "],\"data_lengths\":["
              + std::to_string(tensor.values.size() * sizeof(float)) + "]}";
    data_offset += static_cast<uint64_t>(tensor.values.size() * sizeof(float));
    if (i + 1 == tensors.size()) {
      header += "}";
    }
  }
  header += "}";

  std::ofstream out(path, std::ios::binary);
  assert(out.good());
  write_le64(out, header.size());
  out.write(header.data(), static_cast<std::streamsize>(header.size()));
  for (const auto& tensor : tensors) {
    out.write(reinterpret_cast<const char*>(tensor.values.data()),
              static_cast<std::streamsize>(tensor.values.size() * sizeof(float)));
  }
}

}  // namespace

int main() {
  const fs::path generic_path = make_temp_path("t81-native-ternary");
  const fs::path bitnet_path = make_temp_path("t81-bitnet-ternary");
  const fs::path float_path = make_temp_path("t81-f32-ternary");
  const fs::path invalid_path = make_temp_path("t81-invalid-ternary");

  write_safetensors_file(generic_path, "{\"format\":\"native-ternary-i8\"}", {-1, 0, 1, -1});
  write_safetensors_file(bitnet_path, "{\"architecture\":\"bitnet-b1.58\"}", {-1, 1, 0, -1});
  write_f32_safetensors_file(
      float_path,
      "{\"architecture\":\"llama\",\"llama.block_count\":32,\"llama.embedding_length\":4096,"
      "\"llama.attention.head_count\":32}",
                             {-2.0f, -0.1f, 0.2f, 3.0f}, "model.layers.0.self_attn.q_proj.weight");
  write_safetensors_file(invalid_path, "{\"architecture\":\"bitnet-b1.58\"}", {-1, 2, 0, -1});

  const auto generic = t81::weights::load_safetensors(generic_path);
  assert(generic.format == "SafeTensors(native-ternary-i8)");
  assert(generic.tensors.size() == 1);
  assert(generic.total_trits == 4);
  assert(generic.native.contains("weight"));
  assert(generic.native.at("weight").shape == std::vector<uint64_t>({2, 2}));

  const auto bitnet = t81::weights::load_safetensors(bitnet_path);
  assert(bitnet.format == "SafeTensors(bitnet-b1.58; profile=bitnet-b1.58-v1)");
  assert(bitnet.native.contains("weight"));

  const auto quantized = t81::weights::load_safetensors(float_path);
  assert(quantized.format == "SafeTensors(arch-profile=llama-dense-v1)");
  assert(quantized.native.contains("model.layers.0.self_attn.q_proj.weight"));
  assert(quantized.total_trits == 4);

  const fs::path generic_float_path = make_temp_path("t81-f32-generic");
  write_f32_safetensors_file(generic_float_path, "{\"architecture\":\"unknown-dense\"}",
                             {-2.0f, -0.1f, 0.2f, 3.0f});
  const auto generic_quantized = t81::weights::load_safetensors(generic_float_path);
  assert(generic_quantized.format == "SafeTensors(float-quantized; profile=native-dense-v1)");
  assert(generic_quantized.native.contains("weight"));

  const fs::path invalid_llama_profile_path = make_temp_path("t81-f32-invalid-llama");
  write_f32_safetensors_file(invalid_llama_profile_path,
                             "{\"architecture\":\"llama\",\"llama.block_count\":32,"
                             "\"llama.embedding_length\":4096,\"llama.attention.head_count\":32}",
                             {-2.0f, -0.1f, 0.2f, 3.0f}, "weight");
  bool saw_invalid_llama_profile = false;
  try {
    [[maybe_unused]] auto invalid_profile = t81::weights::load_safetensors(invalid_llama_profile_path);
  } catch (const std::exception& e) {
    saw_invalid_llama_profile =
        std::string(e.what()).find("required architecture tensor signals were not found with valid shapes") !=
        std::string::npos;
  }
  assert(saw_invalid_llama_profile);

  const fs::path invalid_llama_shape_path = make_temp_path("t81-f32-invalid-llama-shape");
  write_f32_safetensors_file(invalid_llama_shape_path,
                             "{\"architecture\":\"llama\",\"llama.block_count\":32,"
                             "\"llama.embedding_length\":4096,\"llama.attention.head_count\":32}",
                             {-2.0f, -0.1f, 0.2f, 3.0f}, "model.layers.0.self_attn.q_proj.weight", "[4]");
  bool saw_invalid_llama_shape = false;
  try {
    [[maybe_unused]] auto invalid_shape = t81::weights::load_safetensors(invalid_llama_shape_path);
  } catch (const std::exception& e) {
    saw_invalid_llama_shape =
        std::string(e.what()).find("required architecture tensor signals were not found with valid shapes") !=
        std::string::npos;
  }
  assert(saw_invalid_llama_shape);

  const fs::path invalid_llama_metadata_path = make_temp_path("t81-f32-invalid-llama-metadata");
  write_f32_safetensors_file(invalid_llama_metadata_path, "{\"architecture\":\"llama\"}",
                             {-2.0f, -0.1f, 0.2f, 3.0f}, "model.layers.0.self_attn.q_proj.weight");
  bool saw_invalid_llama_metadata = false;
  try {
    [[maybe_unused]] auto invalid_metadata = t81::weights::load_safetensors(invalid_llama_metadata_path);
  } catch (const std::exception& e) {
    saw_invalid_llama_metadata =
        std::string(e.what()).find("required scalar metadata was not found") != std::string::npos;
  }
  assert(saw_invalid_llama_metadata);

  const fs::path invalid_llama_moe_path = make_temp_path("t81-f32-invalid-llama-moe");
  write_multi_f32_safetensors_file(
      invalid_llama_moe_path,
      "{\"architecture\":\"llama\",\"llama.block_count\":32,\"llama.embedding_length\":4096,"
      "\"llama.attention.head_count\":32}",
      {
          {"model.layers.0.self_attn.q_proj.weight", "[2,2]", {-2.0f, -0.1f, 0.2f, 3.0f}},
          {"model.layers.0.experts.0.w1.weight", "[2,2]", {-1.0f, 1.0f, -1.0f, 1.0f}},
      });
  bool saw_invalid_llama_moe = false;
  try {
    [[maybe_unused]] auto invalid_moe = t81::weights::load_safetensors(invalid_llama_moe_path);
  } catch (const std::exception& e) {
    saw_invalid_llama_moe =
        std::string(e.what()).find("mixture-of-experts tensors are not supported") != std::string::npos;
  }
  assert(saw_invalid_llama_moe);

  const auto bitnet_forced = t81::weights::load_bitnet_safetensors(generic_path);
  assert(bitnet_forced.format == "SafeTensors(bitnet-b1.58; profile=bitnet-b1.58-v1)");

  bool saw_invalid = false;
  try {
    [[maybe_unused]] auto invalid = t81::weights::load_safetensors(invalid_path);
  } catch (const std::exception& e) {
    saw_invalid = std::string(e.what()).find("BitNet-compatible") != std::string::npos;
  }
  assert(saw_invalid);

  fs::remove(generic_path);
  fs::remove(bitnet_path);
  fs::remove(float_path);
  fs::remove(generic_float_path);
  fs::remove(invalid_llama_profile_path);
  fs::remove(invalid_llama_shape_path);
  fs::remove(invalid_llama_metadata_path);
  fs::remove(invalid_llama_moe_path);
  fs::remove(invalid_path);
  return 0;
}
