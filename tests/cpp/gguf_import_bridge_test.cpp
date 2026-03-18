#include "test_runtime_check.hpp"
#include "t81/model/gguf_import_bridge.hpp"
#include "t81/weights.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace fs = std::filesystem;

namespace {

template <typename T>
void write_le(std::ofstream& out, T value) {
  using U = std::make_unsigned_t<T>;
  U bits = static_cast<U>(value);
  for (size_t i = 0; i < sizeof(U); ++i) {
    out.put(static_cast<char>((bits >> (8 * i)) & 0xFFu));
  }
}

void write_gguf_string(std::ofstream& out, std::string_view value) {
  write_le<uint64_t>(out, static_cast<uint64_t>(value.size()));
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void write_gguf_u32_kv(std::ofstream& out, std::string_view key, uint32_t value) {
  write_gguf_string(out, key);
  write_le<uint32_t>(out, 4);  // GGUF_TYPE_UINT32
  write_le<uint32_t>(out, value);
}

std::string default_tensor_name_for_architecture(std::string_view architecture) {
  if (architecture == "qwen2") {
    return "model.layers.0.attn.q_proj.weight";
  }
  if (architecture == "gemma" || architecture == "mistral" || architecture == "phi3" ||
      architecture == "llama") {
    return "model.layers.0.self_attn.q_proj.weight";
  }
  return "weight";
}

struct GgufTensorFixture {
  std::string name;
  std::vector<uint64_t> dims = {16, 8};
};

void write_minimal_gguf(const fs::path& path,
                        std::string_view architecture = "llama",
                        std::string_view tensor_name = "",
                        std::vector<uint64_t> dims = {16, 8}) {
  std::ofstream out(path, std::ios::binary);
  T81_TEST_CHECK(static_cast<bool>(out));
  const std::string resolved_tensor_name =
      tensor_name.empty() ? default_tensor_name_for_architecture(architecture) : std::string(tensor_name);
  const std::vector<GgufTensorFixture> tensors = {{resolved_tensor_name, dims}};

  out.write("GGUF", 4);
  write_le<uint32_t>(out, 3);
  write_le<uint64_t>(out, static_cast<uint64_t>(tensors.size()));  // tensor count
  write_le<uint64_t>(out, 4);                                      // kv count

  write_gguf_string(out, "general.architecture");
  write_le<uint32_t>(out, 8);  // GGUF_TYPE_STRING
  write_gguf_string(out, architecture);
  write_gguf_u32_kv(out, std::string(architecture) + ".block_count", 32);
  write_gguf_u32_kv(out, std::string(architecture) + ".embedding_length", 4096);
  write_gguf_u32_kv(out, std::string(architecture) + ".attention.head_count", 32);

  uint64_t data_offset = 0;
  for (const auto& tensor : tensors) {
    write_gguf_string(out, tensor.name);
    write_le<uint32_t>(out, static_cast<uint32_t>(tensor.dims.size()));
    uint64_t element_count = 1;
    for (uint64_t dim : tensor.dims) {
      write_le<uint64_t>(out, dim);
      element_count *= dim;
    }
    write_le<uint32_t>(out, 0);  // GGML_TYPE_F32
    write_le<uint64_t>(out, data_offset);
    data_offset += element_count * sizeof(float);
  }

  const auto meta_size = static_cast<size_t>(out.tellp());
  const size_t aligned = (meta_size + 31u) & ~size_t(31u);
  for (size_t i = meta_size; i < aligned; ++i) {
    out.put('\0');
  }

  for (const auto& tensor : tensors) {
    uint64_t element_count = 1;
    for (uint64_t dim : tensor.dims) {
      element_count *= dim;
    }
    for (uint64_t i = 0; i < element_count; ++i) {
      const float value = static_cast<float>(i) * 0.25f;
      out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
  }
}

void write_multi_tensor_gguf(const fs::path& path,
                             std::string_view architecture,
                             const std::vector<GgufTensorFixture>& tensors) {
  std::ofstream out(path, std::ios::binary);
  T81_TEST_CHECK(static_cast<bool>(out));

  out.write("GGUF", 4);
  write_le<uint32_t>(out, 3);
  write_le<uint64_t>(out, static_cast<uint64_t>(tensors.size()));  // tensor count
  write_le<uint64_t>(out, 4);  // kv count

  write_gguf_string(out, "general.architecture");
  write_le<uint32_t>(out, 8);  // GGUF_TYPE_STRING
  write_gguf_string(out, architecture);
  write_gguf_u32_kv(out, std::string(architecture) + ".block_count", 32);
  write_gguf_u32_kv(out, std::string(architecture) + ".embedding_length", 4096);
  write_gguf_u32_kv(out, std::string(architecture) + ".attention.head_count", 32);

  uint64_t data_offset = 0;
  for (const auto& tensor : tensors) {
    write_gguf_string(out, tensor.name);
    write_le<uint32_t>(out, static_cast<uint32_t>(tensor.dims.size()));
    uint64_t element_count = 1;
    for (uint64_t dim : tensor.dims) {
      write_le<uint64_t>(out, dim);
      element_count *= dim;
    }
    write_le<uint32_t>(out, 0);  // GGML_TYPE_F32
    write_le<uint64_t>(out, data_offset);
    data_offset += element_count * sizeof(float);
  }

  const auto meta_size = static_cast<size_t>(out.tellp());
  const size_t aligned = (meta_size + 31u) & ~size_t(31u);
  for (size_t i = meta_size; i < aligned; ++i) {
    out.put('\0');
  }

  for (const auto& tensor : tensors) {
    uint64_t element_count = 1;
    for (uint64_t dim : tensor.dims) {
      element_count *= dim;
    }
    for (uint64_t i = 0; i < element_count; ++i) {
      const float value = static_cast<float>(i) * 0.25f;
      out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
  }
}

}  // namespace

int main() {
  const fs::path temp = fs::temp_directory_path() / "t81-gguf-import-bridge-test.gguf";
  write_minimal_gguf(temp);

  auto bridge = t81::model::GgufImportBridge::open(temp);
  T81_TEST_CHECK(bridge.has_value());
  T81_TEST_CHECK(bridge.value()->model_architecture() == "llama");

  const auto tensors = bridge.value()->list_tensors();
  T81_TEST_CHECK(tensors.size() == 1);
  T81_TEST_CHECK(tensors[0].name == "model.layers.0.self_attn.q_proj.weight");
  std::string source_type = tensors[0].source_type;
  std::transform(source_type.begin(), source_type.end(), source_type.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  T81_TEST_CHECK(source_type == "f32");
  T81_TEST_CHECK(tensors[0].shape.size() == 2);
  T81_TEST_CHECK(tensors[0].shape[0] == 16);
  T81_TEST_CHECK(tensors[0].shape[1] == 8);
  T81_TEST_CHECK(tensors[0].element_count == 128);
  T81_TEST_CHECK(!tensors[0].quantized);

  auto values = bridge.value()->read_tensor_f32("model.layers.0.self_attn.q_proj.weight");
  T81_TEST_CHECK(values.has_value());
  T81_TEST_CHECK(values->size() == 128);
  T81_TEST_CHECK((*values)[0] == 0.0f);
  T81_TEST_CHECK((*values)[1] == 0.25f);
  T81_TEST_CHECK((*values)[127] == 31.75f);

  auto imported = t81::weights::load_gguf(temp);
  T81_TEST_CHECK(imported.format == "GGUF(llama.cpp bridge; arch=llama; profile=llama-dense-v1)");
  T81_TEST_CHECK(imported.tensors.size() == 1);
  T81_TEST_CHECK(imported.native.find("model.layers.0.self_attn.q_proj.weight") != imported.native.end());
  T81_TEST_CHECK(imported.native.at("model.layers.0.self_attn.q_proj.weight").shape.size() == 2);
  T81_TEST_CHECK(imported.native.at("model.layers.0.self_attn.q_proj.weight").shape[0] == 16);
  T81_TEST_CHECK(imported.native.at("model.layers.0.self_attn.q_proj.weight").shape[1] == 8);
  T81_TEST_CHECK(imported.native.at("model.layers.0.self_attn.q_proj.weight").format ==
                 t81::weights::NativeFormat::BalancedTernary);

  const fs::path phi3 = fs::temp_directory_path() / "t81-gguf-import-bridge-phi3.gguf";
  write_minimal_gguf(phi3, "phi3");
  auto imported_phi3 = t81::weights::load_gguf(phi3);
  T81_TEST_CHECK(imported_phi3.format == "GGUF(llama.cpp bridge; arch=phi3; profile=phi3-dense-v1)");
  T81_TEST_CHECK(imported_phi3.tensors.size() == 1);
  T81_TEST_CHECK(imported_phi3.native.find("model.layers.0.self_attn.q_proj.weight") !=
                 imported_phi3.native.end());

  const fs::path qwen2 = fs::temp_directory_path() / "t81-gguf-import-bridge-qwen2.gguf";
  write_minimal_gguf(qwen2, "qwen2");
  auto imported_qwen2 = t81::weights::load_gguf(qwen2);
  T81_TEST_CHECK(imported_qwen2.format == "GGUF(llama.cpp bridge; arch=qwen2; profile=qwen2-dense-v1)");
  T81_TEST_CHECK(imported_qwen2.tensors.size() == 1);
  T81_TEST_CHECK(imported_qwen2.native.find("model.layers.0.attn.q_proj.weight") !=
                 imported_qwen2.native.end());

  const fs::path mistral = fs::temp_directory_path() / "t81-gguf-import-bridge-mistral.gguf";
  write_minimal_gguf(mistral, "mistral");
  auto imported_mistral = t81::weights::load_gguf(mistral);
  T81_TEST_CHECK(imported_mistral.format ==
                 "GGUF(llama.cpp bridge; arch=mistral; profile=mistral-dense-v1)");
  T81_TEST_CHECK(imported_mistral.tensors.size() == 1);
  T81_TEST_CHECK(imported_mistral.native.find("model.layers.0.self_attn.q_proj.weight") !=
                 imported_mistral.native.end());

  const fs::path gemma = fs::temp_directory_path() / "t81-gguf-import-bridge-gemma.gguf";
  write_minimal_gguf(gemma, "gemma");
  auto imported_gemma = t81::weights::load_gguf(gemma);
  T81_TEST_CHECK(imported_gemma.format ==
                 "GGUF(llama.cpp bridge; arch=gemma; profile=gemma-dense-v1)");
  T81_TEST_CHECK(imported_gemma.tensors.size() == 1);
  T81_TEST_CHECK(imported_gemma.native.find("model.layers.0.self_attn.q_proj.weight") !=
                 imported_gemma.native.end());

  const fs::path invalid_llama =
      fs::temp_directory_path() / "t81-gguf-import-bridge-invalid-llama.gguf";
  write_minimal_gguf(invalid_llama, "llama", "weight");
  bool invalid_llama_rejected = false;
  try {
    static_cast<void>(t81::weights::load_gguf(invalid_llama));
  } catch (const std::runtime_error& ex) {
    invalid_llama_rejected = true;
    const std::string msg = ex.what();
    T81_TEST_CHECK(msg.find("required architecture tensor signals were not found with valid shapes") !=
                   std::string::npos);
    T81_TEST_CHECK(msg.find("llama-dense-v1") != std::string::npos);
  }
  T81_TEST_CHECK(invalid_llama_rejected);

  const fs::path invalid_llama_shape =
      fs::temp_directory_path() / "t81-gguf-import-bridge-invalid-llama-shape.gguf";
  write_minimal_gguf(invalid_llama_shape, "llama", "model.layers.0.self_attn.q_proj.weight", {128});
  bool invalid_llama_shape_rejected = false;
  try {
    static_cast<void>(t81::weights::load_gguf(invalid_llama_shape));
  } catch (const std::runtime_error& ex) {
    invalid_llama_shape_rejected = true;
    const std::string msg = ex.what();
    T81_TEST_CHECK(msg.find("required architecture tensor signals were not found with valid shapes") !=
                   std::string::npos);
    T81_TEST_CHECK(msg.find("llama-dense-v1") != std::string::npos);
  }
  T81_TEST_CHECK(invalid_llama_shape_rejected);

  const fs::path invalid_llama_metadata =
      fs::temp_directory_path() / "t81-gguf-import-bridge-invalid-llama-metadata.gguf";
  {
    std::ofstream out(invalid_llama_metadata, std::ios::binary);
    T81_TEST_CHECK(static_cast<bool>(out));
    out.write("GGUF", 4);
    write_le<uint32_t>(out, 3);
    write_le<uint64_t>(out, 1);
    write_le<uint64_t>(out, 1);
    write_gguf_string(out, "general.architecture");
    write_le<uint32_t>(out, 8);
    write_gguf_string(out, "llama");
    write_gguf_string(out, "model.layers.0.self_attn.q_proj.weight");
    write_le<uint32_t>(out, 2);
    write_le<uint64_t>(out, 16);
    write_le<uint64_t>(out, 8);
    write_le<uint32_t>(out, 0);
    write_le<uint64_t>(out, 0);
    const auto meta_size = static_cast<size_t>(out.tellp());
    const size_t aligned = (meta_size + 31u) & ~size_t(31u);
    for (size_t i = meta_size; i < aligned; ++i) {
      out.put('\0');
    }
    for (uint64_t i = 0; i < 128; ++i) {
      const float value = static_cast<float>(i) * 0.25f;
      out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
  }
  bool invalid_llama_metadata_rejected = false;
  try {
    static_cast<void>(t81::weights::load_gguf(invalid_llama_metadata));
  } catch (const std::runtime_error& ex) {
    invalid_llama_metadata_rejected = true;
    const std::string msg = ex.what();
    T81_TEST_CHECK(msg.find("required scalar metadata was not found") != std::string::npos);
    T81_TEST_CHECK(msg.find("llama-dense-v1") != std::string::npos);
  }
  T81_TEST_CHECK(invalid_llama_metadata_rejected);

  const fs::path invalid_llama_moe =
      fs::temp_directory_path() / "t81-gguf-import-bridge-invalid-llama-moe.gguf";
  write_multi_tensor_gguf(
      invalid_llama_moe, "llama",
      {
          {"model.layers.0.self_attn.q_proj.weight", {16, 8}},
          {"model.layers.0.experts.0.w1.weight", {16, 8}},
      });
  bool invalid_llama_moe_rejected = false;
  try {
    static_cast<void>(t81::weights::load_gguf(invalid_llama_moe));
  } catch (const std::runtime_error& ex) {
    invalid_llama_moe_rejected = true;
    const std::string msg = ex.what();
    T81_TEST_CHECK(msg.find("mixture-of-experts tensors are not supported") != std::string::npos);
    T81_TEST_CHECK(msg.find("llama-dense-v1") != std::string::npos);
  }
  T81_TEST_CHECK(invalid_llama_moe_rejected);

  const fs::path unsupported = fs::temp_directory_path() / "t81-gguf-import-bridge-unsupported.gguf";
  write_minimal_gguf(unsupported, "bert");
  bool rejected = false;
  try {
    static_cast<void>(t81::weights::load_gguf(unsupported));
  } catch (const std::runtime_error& ex) {
    rejected = true;
    const std::string msg = ex.what();
    T81_TEST_CHECK(msg.find("does not yet support native profile") != std::string::npos);
    T81_TEST_CHECK(msg.find("bert") != std::string::npos);
  }
  T81_TEST_CHECK(rejected);

  std::error_code ec;
  fs::remove(temp, ec);
  fs::remove(gemma, ec);
  fs::remove(phi3, ec);
  fs::remove(qwen2, ec);
  fs::remove(mistral, ec);
  fs::remove(invalid_llama, ec);
  fs::remove(invalid_llama_shape, ec);
  fs::remove(invalid_llama_metadata, ec);
  fs::remove(invalid_llama_moe, ec);
  fs::remove(unsupported, ec);
  return 0;
}
