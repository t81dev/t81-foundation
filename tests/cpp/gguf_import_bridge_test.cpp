#include "test_runtime_check.hpp"
#include "t81/isa/program.hpp"
#include "t81/model/gguf_import_bridge.hpp"
#include "t81/tensor/native.hpp"
#include "t81/vm/vm.hpp"
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

// Run a synthetic GGUF -> .t81w -> VM TWEMBED round-trip for a given family.
// Proves end-to-end execution evidence without requiring a real model file.
// The minimal GGUF produced by write_minimal_gguf() has tensor dims {16, 8},
// so TWEMBED(row=0) should yield shape [1, 8] with ExactTrit numeric class.
void run_synthetic_family_vm_execution(t81::weights::ModelFile imported,
                                       std::string_view tensor_key,
                                       std::string_view family_label) {
  const fs::path t81w = fs::temp_directory_path() /
                        (std::string("t81-gguf-vm-evidence-") + std::string(family_label) + ".t81w");
  t81::weights::save_t81w(imported.native, t81w);
  auto reloaded = t81::weights::load_t81w(t81w);
  T81_TEST_CHECK(reloaded.native.find(std::string(tensor_key)) != reloaded.native.end());

  auto model = std::make_shared<t81::weights::ModelFile>(std::move(reloaded));

  t81::tisc::Program prog;
  prog.symbol_pool = {std::string(tensor_key)};
  prog.weights_model = model;
  prog.insns.push_back({t81::tisc::Opcode::WeightsLoad, 1, 1, 0});
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  prog.insns.push_back({t81::tisc::Opcode::TWEMBED, 3, 1, 2});
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  prog.axion_policy_text = "(policy (tier 5))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto& state = const_cast<t81::vm::State&>(vm->state());
  state.contexts[0].tier_status.current = t81::cog::TierId::Tier5;
  state.contexts[0].tier_status.label = "tier-5-test";

  // WeightsLoad
  auto step1 = vm->step();
  T81_TEST_CHECK(step1.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[1] == t81::vm::ValueTag::WeightsTensorHandle);
  T81_TEST_CHECK(vm->state().contexts[0].registers[1] > 0);
  // LoadImm
  auto step2 = vm->step();
  T81_TEST_CHECK(step2.has_value());
  // TWEMBED
  auto step3 = vm->step();
  T81_TEST_CHECK(step3.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);

  const auto embed_handle = vm->state().contexts[0].registers[3];
  const auto& embed_tensor = state.tensors[static_cast<std::size_t>(embed_handle - 1)];
  T81_TEST_CHECK(embed_tensor.has_value());
  T81_TEST_CHECK(embed_tensor->shape().size() == 2);
  T81_TEST_CHECK(embed_tensor->shape()[0] == 1);
  T81_TEST_CHECK(embed_tensor->shape()[1] == 8);  // dim[1] from write_minimal_gguf {16,8}
  T81_TEST_CHECK(embed_tensor->numeric_class() == t81::TensorNumericClass::ExactTrit);

  std::error_code ec;
  fs::remove(t81w, ec);
}

fs::path find_repo_model(std::string_view filename) {
  const std::vector<fs::path> candidates = {
      fs::path("models") / filename,
      fs::path("..") / "models" / filename,
  };
  for (const auto& candidate : candidates) {
    if (fs::exists(candidate)) {
      return fs::weakly_canonical(candidate);
    }
  }
  return {};
}

std::vector<float> decode_balanced_ternary_row(const t81::weights::NativeTensor& native,
                                               std::size_t row_index) {
  T81_TEST_CHECK(native.format == t81::weights::NativeFormat::BalancedTernary);
  T81_TEST_CHECK(native.shape.size() == 2);
  const std::size_t rows = static_cast<std::size_t>(native.shape[0]);
  const std::size_t cols = static_cast<std::size_t>(native.shape[1]);
  T81_TEST_CHECK(row_index < rows);

  const std::size_t row_offset = row_index * cols;
  const std::size_t row_end = row_offset + cols;
  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }
  T81_TEST_CHECK(remaining >= row_end);

  std::vector<float> row;
  row.reserve(cols);
  std::size_t global_offset = 0;
  for (uint64_t limb : native.data) {
    const std::size_t count = static_cast<std::size_t>(std::min<uint64_t>(48, remaining));
    if (global_offset >= row_end) {
      break;
    }
    if (global_offset + count <= row_offset) {
      global_offset += count;
      remaining -= count;
      continue;
    }

    std::vector<float> block(count, 0.0f);
    uint64_t value = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = value % 3;
      value /= 3;
      if (static_cast<std::size_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<std::size_t>(i)] = -1.0f;
          break;
        case 1:
          block[static_cast<std::size_t>(i)] = 0.0f;
          break;
        case 2:
          block[static_cast<std::size_t>(i)] = 1.0f;
          break;
        default:
          T81_TEST_CHECK(false);
      }
    }

    const std::size_t local_begin = row_offset > global_offset ? row_offset - global_offset : 0;
    const std::size_t local_end = std::min(count, row_end - global_offset);
    row.insert(row.end(), block.begin() + static_cast<std::ptrdiff_t>(local_begin),
               block.begin() + static_cast<std::ptrdiff_t>(local_end));

    global_offset += count;
    remaining -= count;
  }

  T81_TEST_CHECK(row.size() == cols);
  return row;
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
  T81_TEST_CHECK(imported.provenance.at("source_path") == temp.string());
  T81_TEST_CHECK(imported.provenance.at("bridge_backend") == "llama.cpp");
  T81_TEST_CHECK(imported.provenance.at("bridge_revision") ==
                 t81::model::GgufImportBridge::bridge_revision());
  T81_TEST_CHECK(imported.provenance.at("source_sha3_512").rfind("sha3-512:", 0) == 0);
  T81_TEST_CHECK(imported.provenance.at("source_sha3_512").size() == 137);
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

  const fs::path llama_ggml =
      fs::temp_directory_path() / "t81-gguf-import-bridge-llama-ggml-style.gguf";
  write_minimal_gguf(llama_ggml, "llama", "blk.0.attn_q.weight");
  auto imported_llama_ggml = t81::weights::load_gguf(llama_ggml);
  T81_TEST_CHECK(imported_llama_ggml.format ==
                 "GGUF(llama.cpp bridge; arch=llama; profile=llama-dense-v1)");
  T81_TEST_CHECK(imported_llama_ggml.native.find("blk.0.attn_q.weight") !=
                 imported_llama_ggml.native.end());

  // RFC-00BB: synthetic GGUF -> .t81w -> VM TWEMBED execution evidence for each family.
  // These prove end-to-end native execution without requiring real model files.
  run_synthetic_family_vm_execution(
      std::move(imported_phi3), "model.layers.0.self_attn.q_proj.weight", "phi3");
  run_synthetic_family_vm_execution(
      std::move(imported_qwen2), "model.layers.0.attn.q_proj.weight", "qwen2");
  run_synthetic_family_vm_execution(
      std::move(imported_mistral), "model.layers.0.self_attn.q_proj.weight", "mistral");
  run_synthetic_family_vm_execution(
      std::move(imported_gemma), "model.layers.0.self_attn.q_proj.weight", "gemma");

  const fs::path tinyllama = find_repo_model("tinyllama-1.1b.Q2_K.gguf");
  T81_TEST_CHECK(!tinyllama.empty());
  auto imported_tinyllama = t81::weights::load_gguf(tinyllama);
  T81_TEST_CHECK(imported_tinyllama.format ==
                 "GGUF(llama.cpp bridge; arch=llama; profile=llama-dense-v1)");
  T81_TEST_CHECK(imported_tinyllama.native.find("blk.0.attn_q.weight") !=
                 imported_tinyllama.native.end());
  const fs::path tinyllama_t81w = fs::temp_directory_path() / "t81-gguf-import-bridge-tinyllama.t81w";
  t81::weights::save_t81w(imported_tinyllama.native, tinyllama_t81w);
  auto reloaded_tinyllama = t81::weights::load_t81w(tinyllama_t81w);
  T81_TEST_CHECK(reloaded_tinyllama.native.find("blk.0.attn_q.weight") !=
                 reloaded_tinyllama.native.end());
  auto tinyllama_model = std::make_shared<t81::weights::ModelFile>(std::move(reloaded_tinyllama));

  t81::tisc::Program tinyllama_program;
  tinyllama_program.symbol_pool = {"blk.0.attn_q.weight"};
  tinyllama_program.weights_model = tinyllama_model;
  tinyllama_program.insns.push_back({t81::tisc::Opcode::WeightsLoad, 1, 1, 0});
  tinyllama_program.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  tinyllama_program.insns.push_back({t81::tisc::Opcode::TWEMBED, 3, 1, 2});
  tinyllama_program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  tinyllama_program.axion_policy_text = "(policy (tier 5))";

  auto tinyllama_vm = t81::vm::make_interpreter_vm();
  tinyllama_vm->load_program(tinyllama_program);
  auto& tinyllama_state = const_cast<t81::vm::State&>(tinyllama_vm->state());
  tinyllama_state.contexts[0].tier_status.current = t81::cog::TierId::Tier5;
  tinyllama_state.contexts[0].tier_status.label = "tier-5-test";
  auto tinyllama_step1 = tinyllama_vm->step();
  T81_TEST_CHECK(tinyllama_step1.has_value());
  T81_TEST_CHECK(tinyllama_vm->state().contexts[0].register_tags[1] ==
                 t81::vm::ValueTag::WeightsTensorHandle);
  T81_TEST_CHECK(tinyllama_vm->state().contexts[0].registers[1] > 0);
  auto tinyllama_step2 = tinyllama_vm->step();
  T81_TEST_CHECK(tinyllama_step2.has_value());
  auto tinyllama_result = tinyllama_vm->step();
  T81_TEST_CHECK(tinyllama_result.has_value());
  T81_TEST_CHECK(tinyllama_vm->state().contexts[0].register_tags[3] ==
                 t81::vm::ValueTag::TensorHandle);

  const auto embed_handle = tinyllama_vm->state().contexts[0].registers[3];
  const auto& embed_tensor = tinyllama_state.tensors[static_cast<std::size_t>(embed_handle - 1)];
  T81_TEST_CHECK(embed_tensor.has_value());
  T81_TEST_CHECK(embed_tensor->shape().size() == 2);
  T81_TEST_CHECK(embed_tensor->shape()[0] == 1);
  T81_TEST_CHECK(embed_tensor->numeric_class() == t81::TensorNumericClass::ExactTrit);

  const auto native_it = tinyllama_model->native.find("blk.0.attn_q.weight");
  T81_TEST_CHECK(native_it != tinyllama_model->native.end());
  const auto expected_values = decode_balanced_ternary_row(native_it->second, 0);
  T81_TEST_CHECK(embed_tensor->shape()[1] == static_cast<int>(expected_values.size()));
  const auto actual_values = embed_tensor->snapshot_values();
  const std::size_t row_width = expected_values.size();
  T81_TEST_CHECK(actual_values.size() == row_width);
  for (std::size_t i = 0; i < row_width; ++i) {
    T81_TEST_CHECK(actual_values[i] == expected_values[i]);
  }

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
