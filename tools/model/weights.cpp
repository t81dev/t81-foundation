#include "t81/weights.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/model/gguf_import_bridge.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace t81::weights {

namespace {
// Packs balanced trits (-1,0,+1) into 48-trit limbs.
NativeTensor pack_trits(std::span<const int8_t> src, const std::vector<uint64_t>& shape) {
  NativeTensor tensor;
  tensor.shape = shape;
  tensor.data.clear();
  tensor.data.reserve((src.size() + 47) / 48);
  tensor.trits = src.size();

  for (size_t offset = 0; offset < src.size(); offset += 48) {
    uint64_t limb = 0;
    size_t count = std::min<size_t>(48, src.size() - offset);
    for (size_t i = 0; i < count; ++i) {
      int8_t trit = src[offset + i];
      if (trit < -1 || trit > 1) {
        throw std::runtime_error("invalid trit value");
      }
      limb = limb * 3 + static_cast<uint64_t>(trit + 1);
    }
    for (size_t i = count; i < 48; ++i) {
      limb *= 3;
    }
    tensor.data.push_back(limb);
  }
  return tensor;
}

class JsonParser {
public:
  explicit JsonParser(const std::string& text) : text_(text), idx_(0) {}

  JsonValue parse() {
    skip();
    return parse_value();
  }

private:
  const std::string& text_;
  size_t idx_;

  void skip() {
    while (idx_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[idx_]))) {
      ++idx_;
    }
  }

  char peek() const { return idx_ < text_.size() ? text_[idx_] : '\0'; }

  char consume() { return idx_ < text_.size() ? text_[idx_++] : '\0'; }

  JsonValue parse_value() {
    skip();
    char c = peek();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '"') return parse_string();
    if ((c >= '0' && c <= '9') || c == '-') return parse_number();
    throw std::runtime_error("JSON parse error");
  }

  JsonValue parse_object() {
    consume();
    std::map<std::string, JsonValue> map;
    skip();
    while (peek() != '}' && idx_ < text_.size()) {
      auto key = parse_string().string_value;
      skip();
      if (consume() != ':') throw std::runtime_error("JSON object missing ':'");
      auto value = parse_value();
      map.emplace(key, std::move(value));
      skip();
      if (peek() == ',') {
        consume();
        skip();
      } else {
        break;
      }
    }
    if (consume() != '}') throw std::runtime_error("JSON object missing '}'");
    return JsonValue::make_object(std::move(map));
  }

  JsonValue parse_array() {
    consume();
    std::vector<JsonValue> arr;
    skip();
    while (peek() != ']' && idx_ < text_.size()) {
      arr.push_back(parse_value());
      skip();
      if (peek() == ',') {
        consume();
        skip();
      } else {
        break;
      }
    }
    if (consume() != ']') throw std::runtime_error("JSON array missing ']'");
    return JsonValue::make_array(std::move(arr));
  }

  JsonValue parse_string() {
    consume();
    std::string out;
    while (idx_ < text_.size()) {
      char c = consume();
      if (c == '"') break;
      if (c == '\\') {
        char esc = consume();
        switch (esc) {
          case '"':
            out.push_back('"');
            break;
          case '\\':
            out.push_back('\\');
            break;
          case '/':
            out.push_back('/');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          default:
            out.push_back(esc);
            break;
        }
      } else {
        out.push_back(c);
      }
    }
    return JsonValue::make_string(std::move(out));
  }

  JsonValue parse_number() {
    size_t start = idx_;
    if (peek() == '-') ++idx_;
    while (std::isdigit(peek())) ++idx_;
    if (peek() == '.') {
      ++idx_;
      while (std::isdigit(peek())) ++idx_;
    }
    double value = std::stod(text_.substr(start, idx_ - start));
    return JsonValue::make_number(value);
  }
};

uint64_t json_to_uint(const JsonValue& val) {
  if (!val.is_number) throw std::runtime_error("JSON: expected number");
  return static_cast<uint64_t>(val.number_value);
}

std::vector<uint64_t> json_to_shape(const JsonValue& val) {
  if (val.array_value.empty()) throw std::runtime_error("JSON: expected array for shape");
  std::vector<uint64_t> shape;
  shape.reserve(val.array_value.size());
  for (const auto& entry : val.array_value) {
    shape.push_back(json_to_uint(entry));
  }
  return shape;
}

uint64_t product_of(const std::vector<uint64_t>& shape) {
  uint64_t acc = 1;
  for (uint64_t dim : shape) {
    if (dim == 0) throw std::runtime_error("shape dimension zero");
    if (acc > std::numeric_limits<uint64_t>::max() / dim) {
      throw std::overflow_error("shape overflow");
    }
    acc *= dim;
  }
  return acc;
}

uint64_t count_zero_trits(const NativeTensor& tensor) {
  uint64_t zeros = 0;
  uint64_t remaining = tensor.num_trits();
  for (uint64_t limb : tensor.data) {
    uint64_t value = limb;
    uint64_t digits = std::min<uint64_t>(48, remaining);
    for (uint64_t i = 0; i < digits; ++i) {
      if ((value % 3) == 1) {
        ++zeros;
      }
      value /= 3;
    }
    if (remaining <= 48) {
      break;
    }
    remaining -= digits;
  }
  return zeros;
}

NativeTensor quantize_f32_to_balanced_ternary(std::span<const float> src,
                                              const std::vector<uint64_t>& shape,
                                              float threshold = 0.5f) {
  std::vector<int8_t> trits;
  trits.reserve(src.size());
  for (float value : src) {
    if (value < -threshold) {
      trits.push_back(-1);
    } else if (value > threshold) {
      trits.push_back(1);
    } else {
      trits.push_back(0);
    }
  }
  return pack_trits(trits, shape);
}

std::string native_gguf_profile_for_architecture(std::string_view architecture) {
  if (architecture == "llama") {
    return "llama-dense-v1";
  }
  if (architecture == "gemma") {
    return "gemma-dense-v1";
  }
  if (architecture == "mistral") {
    return "mistral-dense-v1";
  }
  if (architecture == "phi3") {
    return "phi3-dense-v1";
  }
  if (architecture == "qwen2") {
    return "qwen2-dense-v1";
  }
  return {};
}

std::string native_safetensors_profile_for_architecture(std::string_view architecture) {
  if (architecture == "bitnet" || architecture == "bitnet-b1.58") {
    return "bitnet-b1.58-v1";
  }
  return native_gguf_profile_for_architecture(architecture);
}

template <typename TensorDesc>
bool has_named_2d_tensor(std::span<const TensorDesc> tensors, std::string_view signal) {
  for (const auto& tensor : tensors) {
    if (tensor.name.find(signal) == std::string::npos) {
      continue;
    }
    if (tensor.shape.size() == 2 && tensor.shape[0] > 0 && tensor.shape[1] > 0) {
      return true;
    }
  }
  return false;
}

template <typename TensorDesc>
bool profile_has_required_structure(std::string_view profile, std::span<const TensorDesc> tensors) {
  if (profile == "llama-dense-v1" || profile == "mistral-dense-v1" || profile == "gemma-dense-v1" ||
      profile == "phi3-dense-v1") {
    return has_named_2d_tensor(tensors, "self_attn.q_proj") ||
           has_named_2d_tensor(tensors, "attn_q.weight");
  }
  if (profile == "qwen2-dense-v1") {
    return has_named_2d_tensor(tensors, "attn.q_proj.weight");
  }
  return true;
}

[[maybe_unused]] bool gguf_profile_has_required_structure(
    std::string_view profile, const std::vector<t81::model::GgufTensorDescriptor>& tensors) {
  return profile_has_required_structure(profile, std::span<const t81::model::GgufTensorDescriptor>(tensors));
}

bool metadata_has_key(std::initializer_list<std::string_view> keys,
                      const std::unordered_map<std::string, uint64_t>& metadata) {
  for (std::string_view key : keys) {
    if (metadata.contains(std::string(key))) {
      return true;
    }
  }
  return false;
}

bool safetensors_metadata_has_required_scalars(std::string_view profile,
                                               const std::unordered_map<std::string, uint64_t>& metadata) {
  if (profile == "llama-dense-v1") {
    return metadata_has_key({"llama.block_count", "block_count"}, metadata) &&
           metadata_has_key({"llama.embedding_length", "embedding_length"}, metadata) &&
           metadata_has_key({"llama.attention.head_count", "attention.head_count"}, metadata);
  }
  if (profile == "mistral-dense-v1") {
    return metadata_has_key({"mistral.block_count", "block_count"}, metadata) &&
           metadata_has_key({"mistral.embedding_length", "embedding_length"}, metadata) &&
           metadata_has_key({"mistral.attention.head_count", "attention.head_count"}, metadata);
  }
  if (profile == "gemma-dense-v1") {
    return metadata_has_key({"gemma.block_count", "block_count"}, metadata) &&
           metadata_has_key({"gemma.embedding_length", "embedding_length"}, metadata) &&
           metadata_has_key({"gemma.attention.head_count", "attention.head_count"}, metadata);
  }
  if (profile == "phi3-dense-v1") {
    return metadata_has_key({"phi3.block_count", "block_count"}, metadata) &&
           metadata_has_key({"phi3.embedding_length", "embedding_length"}, metadata) &&
           metadata_has_key({"phi3.attention.head_count", "attention.head_count"}, metadata);
  }
  if (profile == "qwen2-dense-v1") {
    return metadata_has_key({"qwen2.block_count", "block_count"}, metadata) &&
           metadata_has_key({"qwen2.embedding_length", "embedding_length"}, metadata) &&
           metadata_has_key({"qwen2.attention.head_count", "attention.head_count"}, metadata);
  }
  return true;
}

template <typename TensorDesc>
std::string profile_unsupported_feature_reason(std::string_view profile,
                                               std::span<const TensorDesc> tensors) {
  if (!(profile == "llama-dense-v1" || profile == "mistral-dense-v1" || profile == "gemma-dense-v1" ||
        profile == "phi3-dense-v1" || profile == "qwen2-dense-v1")) {
    return {};
  }

  for (const auto& tensor : tensors) {
    const std::string_view name = tensor.name;
    if (name.find("experts") != std::string::npos || name.find("expert") != std::string::npos ||
        name.find("router") != std::string::npos || name.find("moe") != std::string::npos) {
      return "mixture-of-experts tensors are not supported by dense native profiles";
    }
    if (name.find("vision_tower") != std::string::npos || name.find("vision") != std::string::npos ||
        name.find("mm_projector") != std::string::npos || name.find("projector") != std::string::npos ||
        name.find("multimodal") != std::string::npos) {
      return "multimodal/projector tensors are not supported by dense native profiles";
    }
  }
  return {};
}

std::string_view json_string_or_empty(const JsonValue* value) {
  return (value != nullptr && value->is_string) ? std::string_view(value->string_value)
                                                : std::string_view{};
}

bool contains_ascii_case_insensitive(std::string_view haystack, std::string_view needle) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return false;
  }
  for (size_t start = 0; start + needle.size() <= haystack.size(); ++start) {
    bool match = true;
    for (size_t i = 0; i < needle.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(haystack[start + i])) !=
          std::tolower(static_cast<unsigned char>(needle[i]))) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

std::string detect_safetensors_profile(const JsonValue& root) {
  const auto metadata_it = root.object_value.find("__metadata__");
  if (metadata_it == root.object_value.end() || metadata_it->second.object_value.empty()) {
    return "native-ternary-i8";
  }

  const auto& metadata = metadata_it->second.object_value;
  auto metadata_string = [&](const char* key) -> const JsonValue* {
    auto it = metadata.find(key);
    return it != metadata.end() ? &it->second : nullptr;
  };
  std::array<std::string_view, 5> candidates = {
      json_string_or_empty(metadata_string("general.architecture")),
      json_string_or_empty(metadata_string("architecture")),
      json_string_or_empty(metadata_string("model_type")),
      json_string_or_empty(metadata_string("family")),
      json_string_or_empty(metadata_string("format")),
  };

  for (std::string_view candidate : candidates) {
    if (contains_ascii_case_insensitive(candidate, "bitnet")) {
      return "bitnet-b1.58-v1";
    }
    if (auto profile = native_safetensors_profile_for_architecture(candidate); !profile.empty()) {
      return profile;
    }
  }
  return "native-ternary-i8";
}

inline float fp16_to_fp32(uint16_t h);

uint64_t resolve_safetensors_data_offset(uint64_t file_size,
                                         uint64_t header_len,
                                         uint64_t offset,
                                         uint64_t length) {
  const uint64_t data_base = 8 + header_len;
  if (offset + length <= file_size - data_base) {
    return data_base + offset;
  }
  if (offset + length <= file_size) {
    return offset;
  }
  throw std::runtime_error("SafeTensors: data range out of bounds");
}

std::vector<float> read_safetensors_tensor_f32(const std::vector<uint8_t>& buffer,
                                               uint64_t header_len,
                                               const std::string& key,
                                               std::string_view dtype,
                                               uint64_t offset,
                                               uint64_t count) {
  const uint64_t file_offset =
      resolve_safetensors_data_offset(static_cast<uint64_t>(buffer.size()), header_len, offset,
                                      count * (dtype == "F32" ? 4u : 2u));
  std::vector<float> values(count);
  if (dtype == "F32") {
    std::memcpy(values.data(), buffer.data() + file_offset, static_cast<size_t>(count * 4));
    return values;
  }

  std::vector<uint16_t> raw(count);
  std::memcpy(raw.data(), buffer.data() + file_offset, static_cast<size_t>(count * 2));
  for (uint64_t i = 0; i < count; ++i) {
    if (dtype == "F16") {
      values[i] = fp16_to_fp32(raw[i]);
    } else if (dtype == "BF16") {
      values[i] = std::bit_cast<float>(static_cast<uint32_t>(raw[i]) << 16);
    } else {
      throw std::runtime_error("SafeTensors tensor '" + key + "' uses unsupported dtype '" +
                               std::string(dtype) + "'");
    }
  }
  return values;
}

ModelFile load_native_ternary_safetensors_impl(const std::filesystem::path& path,
                                               bool force_bitnet_profile,
                                               bool allow_float_quantization) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("cannot open SafeTensors file");
  }

  uint64_t header_len = 0;
  f.read(reinterpret_cast<char*>(&header_len), sizeof(header_len));
  std::string header(header_len, '\0');
  f.read(header.data(), header_len);
  JsonParser parser(header);
  JsonValue root = parser.parse();
  if (root.object_value.empty()) {
    throw std::runtime_error("SafeTensors: empty header");
  }

  ModelFile mf;
  auto file_size = std::filesystem::file_size(path);
  std::vector<uint8_t> buffer(file_size);
  f.seekg(0);
  f.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

  const std::string profile = force_bitnet_profile ? "bitnet-b1.58-v1" : detect_safetensors_profile(root);
  bool imported_native_i8 = false;
  bool imported_float_quantized = false;
  std::unordered_map<std::string, uint64_t> scalar_metadata;
  struct ImportedTensorDesc {
    std::string name;
    std::vector<uint64_t> shape;
  };
  std::vector<ImportedTensorDesc> imported_tensors;
  if (const auto metadata_it = root.object_value.find("__metadata__");
      metadata_it != root.object_value.end() && !metadata_it->second.object_value.empty()) {
    for (const auto& [key, value] : metadata_it->second.object_value) {
      if (value.is_number) {
        scalar_metadata.emplace(key, static_cast<uint64_t>(value.number_value));
      }
    }
  }
  for (const auto& [key, value] : root.object_value) {
    if (key.rfind("__", 0) == 0) continue;
    if (value.object_value.empty()) continue;

    auto dtype_it = value.object_value.find("dtype");
    if (dtype_it == value.object_value.end()) continue;
    const auto& dtype = dtype_it->second;
    if (!dtype.is_string) continue;

    auto shape_it = value.object_value.find("shape");
    if (shape_it == value.object_value.end()) continue;
    auto shape = json_to_shape(shape_it->second);
    uint64_t count = product_of(shape);

    auto offsets_it = value.object_value.find("data_offsets");
    auto lengths_it = value.object_value.find("data_lengths");
    if (offsets_it == value.object_value.end() || lengths_it == value.object_value.end()) {
      throw std::runtime_error("SafeTensors: missing offsets/lengths");
    }
    if (offsets_it->second.array_value.empty() || lengths_it->second.array_value.empty()) {
      throw std::runtime_error("SafeTensors: empty offset/length arrays");
    }

    uint64_t offset = json_to_uint(offsets_it->second.array_value[0]);
    uint64_t length = json_to_uint(lengths_it->second.array_value[0]);
    NativeTensor native;
    if (dtype.string_value == "I8") {
      if (length != count) {
        throw std::runtime_error("SafeTensors tensor '" + key +
                                 "' must store one int8 ternary value per element");
      }
      const uint64_t file_offset =
          resolve_safetensors_data_offset(static_cast<uint64_t>(buffer.size()), header_len, offset, length);
      std::span<const int8_t> raw(reinterpret_cast<const int8_t*>(buffer.data() + file_offset),
                                  static_cast<size_t>(length));
      try {
        native = import_bitnet_b158(raw, shape);
      } catch (const std::exception&) {
        throw std::runtime_error("SafeTensors tensor '" + key +
                                 "' is not native ternary I8/BitNet-compatible");
      }
      imported_native_i8 = true;
    } else if (allow_float_quantization &&
               (dtype.string_value == "F16" || dtype.string_value == "BF16" || dtype.string_value == "F32")) {
      auto values = read_safetensors_tensor_f32(buffer, header_len, key, dtype.string_value, offset, count);
      native = quantize_f32_to_balanced_ternary(values, shape);
      native.trits = count;
      native.format = NativeFormat::BalancedTernary;
      imported_float_quantized = true;
    } else {
      continue;
    }

    TensorInfo info;
    info.name = key;
    info.shape = shape;
    info.num_trits = count;
    info.sparsity = static_cast<double>(count_zero_trits(native)) / static_cast<double>(count);
    mf.tensors.push_back(info);
    mf.total_trits += count;
    mf.total_parameters += count;
    mf.native.emplace(key, std::move(native));
    imported_tensors.push_back(ImportedTensorDesc{key, shape});
  }

  if (mf.native.empty()) {
    if (allow_float_quantization) {
      throw std::runtime_error("SafeTensors import currently supports native ternary I8 tensors and "
                               "float tensors with dtype F16/BF16/F32");
    }
    throw std::runtime_error("BitNet import currently supports only native ternary I8 SafeTensors tensors");
  }

  if (profile == "bitnet-b1.58-v1") {
    mf.format = "SafeTensors(bitnet-b1.58; profile=bitnet-b1.58-v1)";
  } else if (profile == "llama-dense-v1" || profile == "gemma-dense-v1" ||
             profile == "mistral-dense-v1" || profile == "phi3-dense-v1" ||
             profile == "qwen2-dense-v1") {
    if (!profile_has_required_structure(profile, std::span<const ImportedTensorDesc>(imported_tensors))) {
      throw std::runtime_error("SafeTensors import metadata requested profile '" + profile +
                               "', but required architecture tensor signals were not found with valid shapes");
    }
    if (!safetensors_metadata_has_required_scalars(profile, scalar_metadata)) {
      throw std::runtime_error("SafeTensors import metadata requested profile '" + profile +
                               "', but required scalar metadata was not found");
    }
    if (const std::string reason =
            profile_unsupported_feature_reason(profile, std::span<const ImportedTensorDesc>(imported_tensors));
        !reason.empty()) {
      throw std::runtime_error("SafeTensors import metadata requested profile '" + profile + "', but " +
                               reason);
    }
    mf.format = "SafeTensors(arch-profile=" + profile + ")";
  } else if (imported_float_quantized && !imported_native_i8) {
    mf.format = "SafeTensors(float-quantized; profile=native-dense-v1)";
  } else {
    mf.format = "SafeTensors(native-ternary-i8)";
  }
  return mf;
}
}  // namespace

ModelFile build_from_header(const JsonValue& root, const std::vector<uint8_t>& buffer) {
  ModelFile mf;
  for (const auto& [key, value] : root.object_value) {
    if (key.rfind("__", 0) == 0) continue;
    if (value.object_value.empty()) continue;
    auto dtype_it = value.object_value.find("dtype");
    if (dtype_it == value.object_value.end()) continue;
    const auto& dtype = dtype_it->second;
    if (!dtype.is_string || dtype.string_value != "I8") continue;
    auto shape_it = value.object_value.find("shape");
    if (shape_it == value.object_value.end()) continue;
    auto shape = json_to_shape(shape_it->second);
    uint64_t count = product_of(shape);

    auto offsets_it = value.object_value.find("data_offsets");
    auto lengths_it = value.object_value.find("data_lengths");
    if (offsets_it == value.object_value.end() || lengths_it == value.object_value.end()) continue;
    if (offsets_it->second.array_value.empty() || lengths_it->second.array_value.empty()) continue;
    uint64_t offset = json_to_uint(offsets_it->second.array_value[0]);
    uint64_t length = json_to_uint(lengths_it->second.array_value[0]);
    if (offset + length > buffer.size()) throw std::runtime_error("tensor data out of bounds");

    std::span<const int8_t> raw(reinterpret_cast<const int8_t*>(buffer.data() + offset),
                                static_cast<size_t>(length));
    auto native = import_bitnet_b158(raw, shape);

    TensorInfo info;
    info.name = key;
    info.shape = shape;
    info.num_trits = count;
    mf.tensors.push_back(info);
    mf.total_trits += count;
    mf.total_parameters += count;
    mf.native.emplace(key, std::move(native));
  }
  return mf;
}

ModelFile load_safetensors(const std::filesystem::path& path) {
  return load_native_ternary_safetensors_impl(path, false, true);
}

ModelFile load_bitnet_safetensors(const std::filesystem::path& path) {
  return load_native_ternary_safetensors_impl(path, true, false);
}

NativeTensor import_bitnet_b158(std::span<const int8_t> src, const std::vector<uint64_t>& shape) {
  return pack_trits(src, shape);
}

void print_info(const ModelFile& mf) {
  uint64_t limbs = 0;
  for (const auto& [name, tensor] : mf.native) {
    limbs += tensor.padded_limbs();
  }
  std::cout << "Model contains " << mf.tensors.size() << " tensors, "
            << format_bytes(mf.total_trits / 5) << " avg density\n";
  std::cout << "Trits:        " << mf.total_trits << "\n";
  std::cout << "Limbs:        " << limbs << "\n";
  if (!mf.format.empty()) {
    std::cout << "Format:       " << mf.format << "\n";
  }
  if (const auto it = mf.provenance.find("source_sha3_512"); it != mf.provenance.end()) {
    std::cout << "Source SHA3:  " << it->second << "\n";
  }
  if (const auto it = mf.provenance.find("bridge_revision"); it != mf.provenance.end()) {
    std::cout << "Bridge Rev:   " << it->second << "\n";
  }
}

std::string format_bytes_impl(uint64_t bytes) {
  constexpr const char* const units[] = {"B", "KB", "MB", "GB"};
  double value = static_cast<double>(bytes);
  size_t idx = 0;
  while (idx < 3 && value >= 1024.0) {
    value /= 1024.0;
    ++idx;
  }
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.2f %s", value, units[idx]);
  return buf;
}

std::string format_bytes(uint64_t bytes) { return format_bytes_impl(bytes); }

std::string format_count(uint64_t value) {
  constexpr const char* const suffixes[] = {"", "K", "M", "B", "T"};
  double scaled = static_cast<double>(value);
  size_t idx = 0;
  while (idx + 1 < std::size(suffixes) && scaled >= 1000.0) {
    scaled /= 1000.0;
    ++idx;
  }
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.2f %s", scaled, suffixes[idx]);
  return buf;
}

std::string sha3_512_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("cannot open " + path.string());
  }
  std::vector<uint8_t> bytes;
  char ch = 0;
  while (in.get(ch)) {
    bytes.push_back(static_cast<uint8_t>(static_cast<unsigned char>(ch)));
  }
  return "sha3-512:" + crypto::sha3_512_hex(bytes);
}

namespace {

struct QuantTensorInfo {
  std::string name;
  std::vector<uint64_t> shape;
  std::string dtype;
  uint64_t data_offset = 0;
  uint64_t data_size = 0;
};

struct SafetensorFile {
  std::filesystem::path path;
  uint64_t header_size = 0;
  std::vector<uint8_t> header;
  std::vector<QuantTensorInfo> tensors;
};

inline float fp16_to_fp32(uint16_t h) {
  uint32_t sign = (h & 0x8000u) << 16;
  int32_t exp = (h & 0x7C00u) >> 10;
  uint32_t mant = h & 0x03FFu;
  if (exp == 0) {
    if (mant == 0) return std::bit_cast<float>(sign);
    exp = -14;
    do {
      exp++;
      mant <<= 1;
    } while ((mant & 0x0400u) == 0);
    mant &= 0x03FFu;
  } else if (exp == 31) {
    return std::bit_cast<float>(sign | 0x7F800000u | (mant << 13));
  } else {
    exp += (127 - 15);
  }
  uint32_t f = sign | (uint32_t(exp) << 23) | (mant << 13);
  return std::bit_cast<float>(f);
}

enum class Trit : int8_t { M = -1, Z = 0, P = 1 };

constexpr uint8_t trit_to_u3(Trit t) { return static_cast<uint8_t>(static_cast<int8_t>(t) + 1); }

struct T3Block {
  float scale = 0.0f;
  uint8_t packed[26] = {};
};

constexpr uint64_t kT3KScaleBytes = sizeof(float);
constexpr uint64_t kT3KPackedBytes = 26;
constexpr uint64_t kT3KBlockBytes = kT3KScaleBytes + kT3KPackedBytes;

void write_t3_block(std::span<uint8_t> dst, const T3Block& block) {
  if (dst.size() != kT3KBlockBytes) {
    throw std::runtime_error("invalid T3_K block destination size");
  }
  std::memcpy(dst.data(), &block.scale, sizeof(block.scale));
  std::memcpy(dst.data() + sizeof(block.scale), block.packed, sizeof(block.packed));
}

namespace sha3_streaming {
constexpr uint64_t kKeccakfRoundConstants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};

constexpr int kKeccakfRotc[] = {1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
                                27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44};

constexpr int kKeccakfPiln[] = {10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
                                15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1};

inline uint64_t rol(uint64_t value, int offset) noexcept {
  return (value << offset) | (value >> (64 - offset));
}

inline uint64_t load64(const uint8_t* data) noexcept {
  uint64_t value;
  std::memcpy(&value, data, sizeof(value));
  return value;
}

inline void store64(uint8_t* out, uint64_t value) noexcept {
  std::memcpy(out, &value, sizeof(value));
}

void keccakf(uint64_t state[25]) noexcept {
  for (int round = 0; round < 24; ++round) {
    uint64_t bc[5];
    for (int i = 0; i < 5; ++i) {
      bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
    }
    for (int i = 0; i < 5; ++i) {
      uint64_t temp = bc[(i + 4) % 5] ^ rol(bc[(i + 1) % 5], 1);
      for (int j = 0; j < 25; j += 5) {
        state[j + i] ^= temp;
      }
    }
    uint64_t temp = state[1];
    for (int i = 0; i < 24; ++i) {
      int j = kKeccakfPiln[i];
      uint64_t t = state[j];
      state[j] = rol(temp, kKeccakfRotc[i]);
      temp = t;
    }
    for (int i = 0; i < 5; ++i) {
      for (int j = 0; j < 25; j += 5) {
        uint64_t a = state[j + i];
        uint64_t b = state[j + ((i + 1) % 5)];
        state[j + i] = a ^ ((~b) & state[j + ((i + 2) % 5)]);
      }
    }
    state[0] ^= kKeccakfRoundConstants[round];
  }
}
}  // namespace sha3_streaming

class Sha3_512_Stream {
public:
  void update(std::span<const uint8_t> input) noexcept {
    constexpr size_t kRate = 72;
    size_t offset = 0;
    while (offset < input.size()) {
      size_t to_copy = std::min(kRate - buffered_, input.size() - offset);
      std::memcpy(block_.data() + buffered_, input.data() + offset, to_copy);
      buffered_ += to_copy;
      offset += to_copy;
      if (buffered_ == kRate) {
        absorb_block(block_.data());
        buffered_ = 0;
        block_.fill(0);
      }
    }
  }

  [[nodiscard]] std::array<uint8_t, 64> finalize() noexcept {
    constexpr size_t kRate = 72;
    block_[buffered_] = 0x06;
    block_[kRate - 1] |= 0x80;
    absorb_block(block_.data());
    std::array<uint8_t, 64> digest{};
    size_t produced = 0;
    while (produced < digest.size()) {
      for (size_t lane = 0; lane < kRate / 8 && produced < digest.size(); ++lane) {
        uint8_t lane_buffer[8];
        sha3_streaming::store64(lane_buffer, state_[lane]);
        for (size_t i = 0; i < 8 && produced < digest.size(); ++i) {
          digest[produced++] = lane_buffer[i];
        }
      }
      if (produced < digest.size()) {
        sha3_streaming::keccakf(state_.data());
      }
    }
    return digest;
  }

private:
  void absorb_block(const uint8_t* data) noexcept {
    constexpr size_t kRate = 72;
    for (size_t lane = 0; lane < kRate / 8; ++lane) {
      state_[lane] ^= sha3_streaming::load64(data + lane * 8);
    }
    sha3_streaming::keccakf(state_.data());
  }

  std::array<uint64_t, 25> state_{};
  std::array<uint8_t, 72> block_{};
  size_t buffered_ = 0;
};

void quantize_block_t3(const float* src, T3Block& block) {
  constexpr int kBlockTrits = 128;
  constexpr int kCandidateCount = 16;
  constexpr float kTieEpsilon = 1e-7f;
  constexpr std::array<uint8_t, 5> kPow3 = {1, 3, 9, 27, 81};

  std::array<float, kBlockTrits> abs_vals{};
  for (int i = 0; i < kBlockTrits; ++i) {
    abs_vals[i] = std::abs(src[i]);
  }
  std::array<float, kBlockTrits> sorted_abs = abs_vals;
  std::sort(sorted_abs.begin(), sorted_abs.end());

  std::array<float, kCandidateCount> taus{};
  for (int k = 1; k <= kCandidateCount; ++k) {
    int idx = ((k * kBlockTrits) + kCandidateCount - 1) / kCandidateCount - 1;
    idx = std::clamp(idx, 0, kBlockTrits - 1);
    taus[k - 1] = sorted_abs[static_cast<size_t>(idx)];
  }

  float best_tau = taus[0];
  float best_alpha = 0.0f;
  float best_mse = std::numeric_limits<float>::infinity();
  for (float tau : taus) {
    double mag_sum = 0.0;
    int nonzero_count = 0;
    for (int i = 0; i < kBlockTrits; ++i) {
      if (abs_vals[i] > tau) {
        mag_sum += abs_vals[i];
        ++nonzero_count;
      }
    }
    float alpha = nonzero_count == 0 ? 0.0f : static_cast<float>(mag_sum / nonzero_count);

    double mse = 0.0;
    for (int i = 0; i < kBlockTrits; ++i) {
      float q = 0.0f;
      if (abs_vals[i] > tau) {
        q = src[i] >= 0.0f ? 1.0f : -1.0f;
      }
      float err = src[i] - (alpha * q);
      mse += static_cast<double>(err) * static_cast<double>(err);
    }
    float mse_f = static_cast<float>(mse / static_cast<double>(kBlockTrits));
    if (mse_f + kTieEpsilon < best_mse ||
        (std::abs(mse_f - best_mse) <= kTieEpsilon && tau < best_tau)) {
      best_mse = mse_f;
      best_tau = tau;
      best_alpha = alpha;
    }
  }

  block.scale = best_alpha;
  std::array<uint8_t, 130> mapped{};
  mapped.fill(static_cast<uint8_t>(trit_to_u3(Trit::Z)));  // Canonical padding trits.
  for (int i = 0; i < kBlockTrits; ++i) {
    Trit t = Trit::Z;
    if (abs_vals[i] > best_tau) {
      t = src[i] >= 0.0f ? Trit::P : Trit::M;
    }
    mapped[static_cast<size_t>(i)] = trit_to_u3(t);
  }

  for (size_t j = 0; j < std::size(block.packed); ++j) {
    uint16_t byte = 0;
    for (size_t i = 0; i < 5; ++i) {
      byte += static_cast<uint16_t>(mapped[j * 5 + i]) * kPow3[i];
    }
    block.packed[j] = static_cast<uint8_t>(byte);
  }
}

std::vector<QuantTensorInfo> parse_safetensors_header(const std::vector<uint8_t>& header) {
  std::string text(header.begin(), header.end());
  JsonParser parser(text);
  JsonValue root = parser.parse();
  std::vector<QuantTensorInfo> tensors;
  for (const auto& [key, value] : root.object_value) {
    if (key.rfind("__", 0) == 0) continue;
    if (value.object_value.empty()) continue;
    auto dtype_it = value.object_value.find("dtype");
    if (dtype_it == value.object_value.end() || !dtype_it->second.is_string) continue;
    auto shape_it = value.object_value.find("shape");
    if (shape_it == value.object_value.end()) continue;
    auto offsets_it = value.object_value.find("data_offsets");
    auto lengths_it = value.object_value.find("data_lengths");
    if (offsets_it == value.object_value.end() || lengths_it == value.object_value.end()) continue;
    if (offsets_it->second.array_value.empty() || lengths_it->second.array_value.empty()) continue;
    QuantTensorInfo info;
    info.name = key;
    info.dtype = dtype_it->second.string_value;
    info.shape = json_to_shape(shape_it->second);
    info.data_offset = json_to_uint(offsets_it->second.array_value[0]);
    uint64_t length = json_to_uint(lengths_it->second.array_value[0]);
    info.data_size = length - info.data_offset;
    tensors.push_back(info);
  }
  return tensors;
}

struct ModelInfo {
  std::string arch = "llama";
  uint32_t n_layer = 0;
  uint32_t n_head = 0;
  uint32_t n_embd = 0;
  uint32_t context_length = 32768;
};

ModelInfo detect_model(const std::vector<QuantTensorInfo>& tensors) {
  ModelInfo info;
  for (const auto& t : tensors) {
    if (t.name.find("model.layers.") != std::string::npos) {
      size_t dot = t.name.find('.', 13);
      if (dot != std::string::npos) {
        uint32_t layer = 0;
        std::from_chars(t.name.data() + 13, t.name.data() + dot, layer);
        info.n_layer = std::max(info.n_layer, layer + 1);
      }
    }
    if (t.name.find("attn.q.weight") != std::string::npos ||
        t.name.find("self_attn.q_proj") != std::string::npos) {
      if (t.shape.size() >= 2) info.n_embd = static_cast<uint32_t>(t.shape[1]);
    }
    if (t.name.find("q_proj.weight") != std::string::npos && t.shape.size() == 2 &&
        info.n_embd != 0) {
      info.n_head = static_cast<uint32_t>(t.shape[0] / info.n_embd);
    }
  }
  if (info.n_layer == 28 && info.n_embd == 4096) info.context_length = 131072;
  if (info.n_layer == 32 && info.n_embd == 4096) info.context_length = 131072;
  if (info.n_layer == 32 && info.n_embd == 5120) info.arch = "qwen2";
  return info;
}

template <typename T>
void append_le(std::vector<uint8_t>& out, T value) {
  using U = std::make_unsigned_t<T>;
  U u = static_cast<U>(value);
  for (size_t i = 0; i < sizeof(U); ++i) {
    out.push_back(static_cast<uint8_t>((u >> (8 * i)) & 0xFFu));
  }
}

void append_string(std::vector<uint8_t>& out, const std::string& value) {
  append_le<uint64_t>(out, static_cast<uint64_t>(value.size()));
  out.insert(out.end(), value.begin(), value.end());
}

uint64_t align_up(uint64_t value, uint64_t alignment) {
  if (alignment == 0 || value % alignment == 0) {
    return value;
  }
  return value + (alignment - (value % alignment));
}

enum : uint32_t {
  kGGUFTypeUInt8 = 0,
  kGGUFTypeUInt32 = 4,
  kGGUFTypeString = 8,
  kGGUFTypeArray = 9,
};

constexpr uint32_t kGGUFVersion = 3;
constexpr uint32_t kGGUFAlignment = 32;
constexpr uint32_t kGGMLTypeT3K = 99;  // Custom tensor type; requires patched ggml/llama.cpp.

std::string ggml_type_name(uint32_t type) {
  switch (type) {
    case 0:
      return "F32";
    case 1:
      return "F16";
    case 2:
      return "Q4_0";
    case 3:
      return "Q4_1";
    case 6:
      return "Q5_0";
    case 7:
      return "Q5_1";
    case 8:
      return "Q8_0";
    case 9:
      return "Q8_1";
    case 10:
      return "Q2_K";
    case 11:
      return "Q3_K";
    case 12:
      return "Q4_K";
    case 13:
      return "Q5_K";
    case 14:
      return "Q6_K";
    case 15:
      return "Q8_K";
    case 16:
      return "IQ2_XXS";
    case 17:
      return "IQ2_XS";
    case 18:
      return "IQ3_XXS";
    case 19:
      return "IQ1_S";
    case 20:
      return "IQ4_NL";
    case 21:
      return "IQ3_S";
    case 22:
      return "IQ2_S";
    case 23:
      return "IQ4_XS";
    case kGGMLTypeT3K:
      return "T3_K";
    default:
      return "TYPE_" + std::to_string(type);
  }
}

[[maybe_unused]] std::string format_ggml_type_histogram(const std::map<uint32_t, uint64_t>& counts) {
  if (counts.empty()) {
    return "none";
  }

  std::ostringstream out;
  bool first = true;
  for (const auto& [type, count] : counts) {
    if (!first) {
      out << ", ";
    }
    first = false;
    out << ggml_type_name(type) << "=" << count;
  }
  return out.str();
}

using GGUFValue = std::variant<uint32_t, std::string, std::array<uint8_t, 64>>;

struct TensorPlan {
  size_t file_index = 0;
  QuantTensorInfo tensor;
  std::vector<uint64_t> gguf_shape;
  uint64_t element_count = 0;
  uint64_t data_offset = 0;
  uint64_t data_size = 0;
};

uint32_t read_u32(const uint8_t* ptr) {
  uint32_t v = 0;
  for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(ptr[i]) << (i * 8);
  return v;
}

uint64_t read_u64(const uint8_t* ptr) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(ptr[i]) << (i * 8);
  return v;
}

std::string read_string(const uint8_t*& ptr, const uint8_t* end) {
  if (ptr + 8 > end) throw std::runtime_error("GGUF: unexpected EOF reading string len");
  uint64_t len = read_u64(ptr);
  ptr += 8;
  if (len > static_cast<uint64_t>(end - ptr)) {
    throw std::runtime_error("GGUF: unexpected EOF reading string body");
  }
  std::string s(reinterpret_cast<const char*>(ptr), len);
  ptr += len;
  return s;
}

}  // namespace

void save_t81w(const NativeModel& model, const std::filesystem::path& path) {
  std::vector<uint8_t> buffer;
  auto append_bytes = [&](const void* data, size_t size) {
    const auto* ptr = static_cast<const uint8_t*>(data);
    buffer.insert(buffer.end(), ptr, ptr + size);
  };
  auto append_le64 = [&](uint64_t value) {
    for (int i = 0; i < 8; ++i) {
      buffer.push_back(static_cast<uint8_t>(value & 0xFF));
      value >>= 8;
    }
  };

  // Bump to T81W2 to support formats
  const std::string magic = "T81W2\n";
  append_bytes(magic.data(), magic.size());
  const size_t hash_pos = buffer.size();
  buffer.insert(buffer.end(), 128, '0');
  buffer.push_back('\n');
  const size_t payload_start = buffer.size();

  append_le64(model.size());
  for (const auto& [name, tensor] : model) {
    append_le64(name.size());
    append_bytes(name.data(), name.size());
    append_le64(tensor.shape.size());
    for (uint64_t dim : tensor.shape) {
      append_le64(dim);
    }

    // New fields in V2: Format (u8), Trits (u64), DataLen (u64), Data
    buffer.push_back(static_cast<uint8_t>(tensor.format));
    const uint64_t trits = tensor.num_trits();
    append_le64(trits);

    if (tensor.format == NativeFormat::T3_K) {
      // T3_K data is stored as raw bytes packed in u64s
      uint64_t blocks = (trits + 127) / 128;
      uint64_t bytes_len = blocks * kT3KBlockBytes;
      append_le64(bytes_len);
      if (!tensor.data.empty()) {
        append_bytes(tensor.data.data(), bytes_len);
      } else {
        // Should not happen for valid tensor
        std::vector<uint8_t> zeros(bytes_len, 0);
        append_bytes(zeros.data(), bytes_len);
      }
    } else {
      // Balanced Ternary
      uint64_t limbs = (trits + 47) / 48;
      uint64_t bytes_len = limbs * 8;
      append_le64(bytes_len);
      for (uint64_t li = 0; li < limbs; ++li) {
        uint64_t limb = li < tensor.data.size() ? tensor.data[li] : 0;
        append_le64(limb);
      }
    }
  }

  auto payload =
      std::span<const uint8_t>(buffer.data() + payload_start, buffer.size() - payload_start);
  const std::string hash_hex = crypto::sha3_512_hex(payload);
  std::copy(hash_hex.begin(), hash_hex.end(), buffer.begin() + hash_pos);

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("cannot write " + path.string());
  }
  out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

ModelFile load_t81w(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open " + path.string());

  std::vector<std::byte> bytes;
  char ch = 0;
  while (in.get(ch)) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return load_t81w_bytes(bytes);
}

ModelFile load_t81w_bytes(std::span<const std::byte> bytes) {
  const auto* raw = reinterpret_cast<const uint8_t*>(bytes.data());
  const std::span<const uint8_t> file(raw, bytes.size());
  auto find_newline = [&](size_t start) -> size_t {
    for (size_t i = start; i < file.size(); ++i) {
      if (file[i] == static_cast<uint8_t>('\n')) {
        return i;
      }
    }
    return file.size();
  };

  const size_t magic_end = find_newline(0);
  if (magic_end == file.size()) {
    throw std::runtime_error("invalid t81w file");
  }
  const std::string magic(reinterpret_cast<const char*>(file.data()), magic_end);
  if (magic != "T81W1" && magic != "T81W2") {
    throw std::runtime_error("invalid t81w file");
  }

  const size_t checksum_start = magic_end + 1;
  const size_t checksum_end = find_newline(checksum_start);
  if (checksum_end == file.size()) {
    throw std::runtime_error("t81w: missing or malformed checksum");
  }
  const std::string checksum(reinterpret_cast<const char*>(file.data() + checksum_start),
                             checksum_end - checksum_start);
  if (checksum.size() != 128) {
    throw std::runtime_error("t81w: missing or malformed checksum");
  }

  const uint64_t header_end = static_cast<uint64_t>(checksum_end + 1);
  const uint64_t file_size = static_cast<uint64_t>(file.size());
  if (file_size < header_end) {
    throw std::runtime_error("t81w: file truncated");
  }

  const auto payload = file.subspan(static_cast<size_t>(header_end));
  const std::string computed = crypto::sha3_512_hex(payload);
  if (computed != checksum) {
    throw std::runtime_error("t81w: checksum mismatch");
  }

  ModelFile mf;
  mf.format = magic == "T81W2" ? "T81W2" : "T81W1 native balanced ternary";
  mf.checksum = computed;
  mf.file_size = file_size;

  const uint8_t* cursor = payload.data();
  const uint8_t* end = payload.data() + payload.size();

  auto read_le64 = [&]() -> uint64_t {
    if (cursor + 8 > end) throw std::runtime_error("t81w: truncated metadata");
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
      value |= static_cast<uint64_t>(cursor[i]) << (8 * i);
    }
    cursor += 8;
    return value;
  };

  auto read_bytes = [&](size_t count, std::string& out) {
    if (cursor + count > end) throw std::runtime_error("t81w: truncated name");
    out.assign(reinterpret_cast<const char*>(cursor), count);
    cursor += count;
  };

  uint64_t num_tensors = read_le64();
  uint64_t zero_trits = 0;
  for (uint64_t ti = 0; ti < num_tensors; ++ti) {
    uint64_t name_len = read_le64();
    std::string name;
    read_bytes(name_len, name);
    uint64_t rank = read_le64();
    std::vector<uint64_t> shape;
    shape.reserve(rank);
    for (uint64_t r = 0; r < rank; ++r) {
      shape.push_back(read_le64());
    }

    NativeFormat fmt = NativeFormat::BalancedTernary;
    if (magic == "T81W2") {
      if (cursor >= end) throw std::runtime_error("t81w: truncated format");
      fmt = static_cast<NativeFormat>(*cursor++);
    }

    uint64_t trits = read_le64();
    uint64_t data_bytes = 0;

    if (magic == "T81W2") {
      data_bytes = read_le64();
    } else {
      uint64_t limbs = (trits + 47) / 48;
      data_bytes = limbs * 8;
    }

    if (cursor + data_bytes > end) {
      throw std::runtime_error("t81w: truncated tensor data");
    }

    NativeTensor tensor;
    tensor.shape = shape;
    tensor.trits = trits;
    tensor.format = fmt;

    size_t u64_count = (data_bytes + 7) / 8;
    tensor.data.resize(u64_count);

    if (fmt == NativeFormat::T3_K) {
      std::memcpy(tensor.data.data(), cursor, data_bytes);
      cursor += data_bytes;
    } else {
      // Balanced Ternary - read as LE64
      size_t limbs = data_bytes / 8;
      for (size_t i = 0; i < limbs; ++i) {
        tensor.data[i] = read_le64();
      }
    }

    if (fmt == NativeFormat::BalancedTernary) {
      zero_trits += count_zero_trits(tensor);
    }

    TensorInfo info;
    info.name = name;
    info.shape = shape;
    info.num_trits = trits;
    mf.tensors.push_back(info);
    mf.total_trits += trits;
    mf.total_parameters += trits;
    mf.native.emplace(name, std::move(tensor));
  }

  if (mf.total_trits > 0) {
    mf.bits_per_trit = static_cast<double>(file_size * 8) / static_cast<double>(mf.total_trits);
    mf.sparsity = static_cast<double>(zero_trits) / static_cast<double>(mf.total_trits);
  }

  return mf;
}

ModelFile load_gguf(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) throw std::runtime_error("cannot open GGUF file");
  size_t file_size = f.tellg();
  f.seekg(0, std::ios::beg);
  std::vector<uint8_t> buffer(file_size);
  if (!f.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
    throw std::runtime_error("failed to read GGUF file");
  }

  const uint8_t* ptr = buffer.data();
  const uint8_t* end = buffer.data() + file_size;

  if (ptr + 4 > end || std::memcmp(ptr, "GGUF", 4) != 0) {
    throw std::runtime_error("invalid GGUF magic");
  }
  ptr += 4;

  if (ptr + 4 > end) throw std::runtime_error("GGUF: truncated version");
  uint32_t version = read_u32(ptr);
  ptr += 4;
  if (version != kGGUFVersion) {
    throw std::runtime_error("unsupported GGUF version: " + std::to_string(version));
  }

  if (ptr + 16 > end) throw std::runtime_error("GGUF: truncated counts");
  uint64_t tensor_count = read_u64(ptr);
  ptr += 8;
  uint64_t kv_count = read_u64(ptr);
  ptr += 8;

  ModelFile mf;
  mf.format = "GGUF";
  mf.file_size = file_size;
  std::unordered_map<std::string, uint64_t> scalar_metadata;

  // Read KV pairs
  for (uint64_t i = 0; i < kv_count; ++i) {
    std::string key = read_string(ptr, end);
    if (ptr + 4 > end) throw std::runtime_error("GGUF: truncated kv type");
    uint32_t type = read_u32(ptr);
    ptr += 4;

    // std::cerr << "KV: " << key << " Type: " << type << "\n";

    // We only care about specific metadata keys for now, but we must skip values correctly.
    auto skip_value = [&](auto& self, uint32_t t) -> void {
      switch (t) {
        case kGGUFTypeUInt8:
          ptr += 1;
          break;
        case 1:
          ptr += 1;
          break;  // int8
        case 2:
          ptr += 2;
          break;  // uint16
        case 3:
          ptr += 2;
          break;  // int16
        case kGGUFTypeUInt32:
          ptr += 4;
          break;
        case 5:
          ptr += 4;
          break;  // int32
        case 6:
          ptr += 4;
          break;  // float32
        case 7:
          ptr += 1;
          break;  // bool
        case kGGUFTypeString: {
          read_string(ptr, end);
          break;
        }
        case kGGUFTypeArray: {
          if (ptr + 12 > end) throw std::runtime_error("GGUF: truncated array");
          uint32_t et = read_u32(ptr);
          ptr += 4;
          uint64_t len = read_u64(ptr);
          ptr += 8;
          for (uint64_t k = 0; k < len; ++k) self(self, et);
          break;
        }
        case 10:
          ptr += 8;
          break;  // uint64
        case 11:
          ptr += 8;
          break;  // int64
        case 12:
          ptr += 8;
          break;  // float64
        default:
          throw std::runtime_error("unknown GGUF KV type " + std::to_string(t));
      }
      if (ptr > end) throw std::runtime_error("GGUF: unexpected EOF in kv value");
    };

    if (key == "t3_k.source_sha3" && type == kGGUFTypeArray) {
      // Parse checksum
      if (ptr + 12 > end) throw std::runtime_error("GGUF: truncated checksum array");
      uint32_t et = read_u32(ptr);
      ptr += 4;
      uint64_t len = read_u64(ptr);
      ptr += 8;
      if (et == kGGUFTypeUInt8 && len == 64) {
        std::string hex;
        for (size_t k = 0; k < 64; ++k) {
          char buf[3];
          snprintf(buf, sizeof(buf), "%02x", ptr[k]);
          hex += buf;
        }
        mf.checksum = hex;
        ptr += 64;
      } else {
        // Just skip if not what we expect
        for (uint64_t k = 0; k < len; ++k) skip_value(skip_value, et);
      }
    } else if (type == kGGUFTypeUInt32 &&
               (key.find(".block_count") != std::string::npos ||
                key.find(".embedding_length") != std::string::npos ||
                key.find(".attention.head_count") != std::string::npos || key == "block_count" ||
                key == "embedding_length" || key == "attention.head_count")) {
      if (ptr + 4 > end) throw std::runtime_error("GGUF: truncated uint32 value");
      scalar_metadata.emplace(key, read_u32(ptr));
      ptr += 4;
    } else {
      skip_value(skip_value, type);
    }
  }

  struct GGUFTensorHeader {
    std::string name;
    std::vector<uint64_t> shape;
    uint32_t type;
    uint64_t offset;
  };
  std::vector<GGUFTensorHeader> headers;
  headers.reserve(tensor_count);
  std::map<uint32_t, uint64_t> tensor_type_counts;

  for (uint64_t i = 0; i < tensor_count; ++i) {
    std::string name = read_string(ptr, end);
    if (ptr + 4 > end) throw std::runtime_error("GGUF: truncated tensor ndim");
    uint32_t ndim = read_u32(ptr);
    ptr += 4;

    std::vector<uint64_t> shape;
    for (uint32_t d = 0; d < ndim; ++d) {
      if (ptr + 8 > end) throw std::runtime_error("GGUF: truncated tensor dims");
      shape.push_back(read_u64(ptr));
      ptr += 8;
    }
    if (ptr + 12 > end) throw std::runtime_error("GGUF: truncated tensor type/offset");
    uint32_t type = read_u32(ptr);
    ptr += 4;
    uint64_t offset = read_u64(ptr);
    ptr += 8;

    GGUFTensorHeader h;
    h.name = name;
    h.shape = shape;
    h.type = type;
    h.offset = offset;
    headers.push_back(std::move(h));
    ++tensor_type_counts[type];
  }

  uint64_t header_end_offset = ptr - buffer.data();
  uint64_t data_base_offset = align_up(header_end_offset, kGGUFAlignment);
  const uint8_t* data_base = buffer.data() + data_base_offset;
  std::size_t imported_t3k_tensors = 0;

  for (const auto& h : headers) {
    if (h.type != kGGMLTypeT3K) continue;

    TensorInfo info;
    info.name = h.name;
    info.shape = h.shape;

    // Calculate elements
    uint64_t num_elements = 1;
    for (auto d : h.shape) num_elements *= d;
    info.num_trits = num_elements;

    // Calculate data size
    uint64_t num_blocks = (num_elements + 127) / 128;
    uint64_t data_bytes = num_blocks * kT3KBlockBytes;

    const uint8_t* tensor_data = data_base + h.offset;

    if (tensor_data + data_bytes > end) {
      throw std::runtime_error("GGUF: tensor data out of bounds");
    }

    NativeTensor native;
    native.shape = h.shape;
    native.trits = num_elements;
    native.format = NativeFormat::T3_K;

    size_t u64_count = (data_bytes + 7) / 8;
    native.data.resize(u64_count);
    std::memcpy(native.data.data(), tensor_data, data_bytes);

    mf.native.emplace(h.name, std::move(native));
    mf.tensors.push_back(info);
    mf.total_trits += num_elements;
    mf.total_parameters += num_elements;
    ++imported_t3k_tensors;
  }

  if (!headers.empty() && imported_t3k_tensors == 0) {
#if defined(T81_HAS_LLAMA_CPP)
    auto bridge = t81::model::GgufImportBridge::open(path);
    if (!bridge.has_value()) {
      throw std::runtime_error("GGUF import could not open llama.cpp bridge: " + bridge.error());
    }

    const std::string source_sha3_512 = sha3_512_file(path);
    const std::string bridge_revision = t81::model::GgufImportBridge::bridge_revision();
    const std::string architecture = bridge.value()->model_architecture();
    const std::string profile = native_gguf_profile_for_architecture(architecture);
    if (profile.empty()) {
      throw std::runtime_error("GGUF llama.cpp bridge import does not yet support native profile for "
                               "architecture '" +
                               architecture + "'");
    }

    const auto tensors = bridge.value()->list_tensors();
    if (tensors.empty()) {
      throw std::runtime_error("GGUF import bridge returned no tensors for " + path.string());
    }
    if (!gguf_profile_has_required_structure(profile, tensors)) {
      throw std::runtime_error("GGUF llama.cpp bridge import metadata requested profile '" + profile +
                               "', but required architecture tensor signals were not found with valid shapes");
    }
    if (!safetensors_metadata_has_required_scalars(profile, scalar_metadata)) {
      throw std::runtime_error("GGUF llama.cpp bridge import metadata requested profile '" + profile +
                               "', but required scalar metadata was not found");
    }
    if (const std::string reason =
            profile_unsupported_feature_reason(profile, std::span<const t81::model::GgufTensorDescriptor>(tensors));
        !reason.empty()) {
      throw std::runtime_error("GGUF llama.cpp bridge import metadata requested profile '" + profile +
                               "', but " + reason);
    }

    for (const auto& tensor : tensors) {
      auto values = bridge.value()->read_tensor_f32(tensor.name);
      if (!values.has_value()) {
        throw std::runtime_error("GGUF import bridge failed for tensor '" + tensor.name +
                                 "': " + values.error());
      }

      auto native = quantize_f32_to_balanced_ternary(*values, tensor.shape);
      native.trits = tensor.element_count;
      native.format = NativeFormat::BalancedTernary;

      TensorInfo info;
      info.name = tensor.name;
      info.shape = tensor.shape;
      info.num_trits = tensor.element_count;

      mf.native.emplace(tensor.name, std::move(native));
      mf.tensors.push_back(std::move(info));
      mf.total_trits += tensor.element_count;
      mf.total_parameters += tensor.element_count;
    }

    mf.format = "GGUF(llama.cpp bridge; arch=" + architecture + "; profile=" + profile + ")";
    mf.provenance.emplace("source_path", path.string());
    mf.provenance.emplace("source_sha3_512", source_sha3_512);
    mf.provenance.emplace("bridge_backend", "llama.cpp");
    mf.provenance.emplace("bridge_revision", bridge_revision);
    return mf;
#else
    throw std::runtime_error("GGUF import currently supports only native ternary T3_K tensors; "
                             "this GGUF does not contain any T3_K tensors. "
                             "Found tensor types: " +
                             format_ggml_type_histogram(tensor_type_counts));
#endif
  }

  return mf;
}

void quantize_safetensors_to_gguf(const std::filesystem::path& input,
                                  const std::filesystem::path& output) {
  if (!std::filesystem::exists(input)) {
    throw std::runtime_error("input path not found");
  }
  std::vector<SafetensorFile> files;
  auto add_file = [&](const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path.string());
    uint64_t header_size = 0;
    f.read(reinterpret_cast<char*>(&header_size), sizeof(header_size));
    std::vector<uint8_t> header(header_size);
    f.read(reinterpret_cast<char*>(header.data()), header_size);
    SafetensorFile file;
    file.path = path;
    file.header_size = header_size;
    file.header = std::move(header);
    file.tensors = parse_safetensors_header(file.header);
    files.push_back(std::move(file));
  };
  if (std::filesystem::is_directory(input)) {
    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator(input)) {
      if (entry.path().extension() == ".safetensors") {
        paths.push_back(entry.path());
      }
    }
    std::sort(paths.begin(), paths.end());
    for (const auto& path : paths) {
      add_file(path);
    }
  } else if (input.extension() == ".safetensors") {
    add_file(input);
  } else {
    throw std::runtime_error("input must be a .safetensors file or directory");
  }
  if (files.empty()) {
    throw std::runtime_error("no safetensors found in input");
  }
  std::vector<QuantTensorInfo> all_tensors;
  for (const auto& file : files) {
    all_tensors.insert(all_tensors.end(), file.tensors.begin(), file.tensors.end());
  }

  ModelInfo model = detect_model(all_tensors);

  std::vector<TensorPlan> plans;
  plans.reserve(all_tensors.size());
  uint64_t data_cursor = 0;
  for (size_t file_idx = 0; file_idx < files.size(); ++file_idx) {
    for (const auto& tensor : files[file_idx].tensors) {
      if (tensor.shape.empty()) {
        continue;
      }
      if (tensor.dtype != "F16" && tensor.dtype != "BF16" && tensor.dtype != "F32") {
        continue;
      }

      uint64_t n_elements = 1;
      for (uint64_t dim : tensor.shape) {
        if (dim == 0) {
          throw std::runtime_error("tensor dimension zero");
        }
        if (n_elements > std::numeric_limits<uint64_t>::max() / dim) {
          throw std::overflow_error("tensor shape overflow");
        }
        n_elements *= dim;
      }

      TensorPlan plan;
      plan.file_index = file_idx;
      plan.tensor = tensor;
      plan.element_count = n_elements;
      plan.gguf_shape = tensor.shape;
      std::reverse(plan.gguf_shape.begin(), plan.gguf_shape.end());

      uint64_t blocks = (n_elements + 127) / 128;
      plan.data_size = blocks * kT3KBlockBytes;
      plan.data_offset = align_up(data_cursor, kGGUFAlignment);
      data_cursor = plan.data_offset + plan.data_size;
      plans.push_back(std::move(plan));
    }
  }

  if (plans.empty()) {
    throw std::runtime_error("no supported tensors found for quantization");
  }

  std::vector<uint8_t> quantized_data(static_cast<size_t>(data_cursor), 0);
  Sha3_512_Stream source_hasher;

  std::vector<float> tmp(128);
  for (const auto& plan : plans) {
    const auto& file = files[plan.file_index];
    std::ifstream f(file.path, std::ios::binary);
    if (!f) {
      throw std::runtime_error("cannot reopen " + file.path.string());
    }

    std::vector<float> float_data(plan.element_count);
    f.seekg(8 + static_cast<std::streamoff>(file.header_size) + plan.tensor.data_offset,
            std::ios::beg);
    if (!f) {
      throw std::runtime_error("failed to seek tensor payload for " + plan.tensor.name);
    }

    if (plan.tensor.dtype == "F16" || plan.tensor.dtype == "BF16") {
      std::vector<uint16_t> raw(plan.element_count);
      f.read(reinterpret_cast<char*>(raw.data()),
             static_cast<std::streamsize>(plan.element_count * 2));
      if (!f) {
        throw std::runtime_error("failed reading tensor payload for " + plan.tensor.name);
      }
      auto bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(raw.data()),
                                            static_cast<size_t>(raw.size() * sizeof(uint16_t)));
      source_hasher.update(bytes);
      for (uint64_t i = 0; i < plan.element_count; ++i) {
        if (plan.tensor.dtype == "F16") {
          float_data[i] = fp16_to_fp32(raw[i]);
        } else {
          uint32_t bits = static_cast<uint32_t>(raw[i]) << 16;
          float_data[i] = std::bit_cast<float>(bits);
        }
      }
    } else {
      f.read(reinterpret_cast<char*>(float_data.data()),
             static_cast<std::streamsize>(plan.element_count * 4));
      if (!f) {
        throw std::runtime_error("failed reading tensor payload for " + plan.tensor.name);
      }
      auto bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(float_data.data()),
                                            static_cast<size_t>(float_data.size() * sizeof(float)));
      source_hasher.update(bytes);
    }

    uint64_t data_write_off = plan.data_offset;
    for (uint64_t idx = 0; idx < plan.element_count; idx += 128) {
      const uint64_t block_count = std::min<uint64_t>(128, plan.element_count - idx);
      std::copy_n(float_data.data() + idx, block_count, tmp.data());
      if (block_count < 128) {
        std::fill(tmp.begin() + static_cast<std::ptrdiff_t>(block_count), tmp.end(), 0.0f);
      }
      T3Block block{};
      quantize_block_t3(tmp.data(), block);
      write_t3_block(std::span<uint8_t>(quantized_data.data() + static_cast<size_t>(data_write_off),
                                        static_cast<size_t>(kT3KBlockBytes)),
                     block);
      data_write_off += kT3KBlockBytes;
    }
  }

  const auto source_sha3 = source_hasher.finalize();
  const std::vector<std::pair<std::string, GGUFValue>> kvs = {
      {"general.architecture", model.arch},
      {"general.name", output.stem().string()},
      {"general.file_type", uint32_t(32)},
      {"general.alignment", uint32_t(kGGUFAlignment)},
      {model.arch + ".context_length", model.context_length},
      {model.arch + ".block_count", model.n_layer},
      {model.arch + ".embedding_length", model.n_embd},
      {model.arch + ".attention.head_count", model.n_head},
      {"tokenizer.ggml.model", std::string("llama")},
      {"t3_k.version", uint32_t(1)},
      {"t3_k.block_trits", uint32_t(128)},
      {"t3_k.scale_type", uint32_t(1)},
      {"t3_k.tau_strategy", std::string("fixed_quantile_16_min_tau")},
      {"t3_k.source_sha3", source_sha3},
  };

  std::vector<uint8_t> meta;
  meta.reserve(1024 + plans.size() * 64);
  meta.insert(meta.end(), {'G', 'G', 'U', 'F'});
  append_le<uint32_t>(meta, kGGUFVersion);
  append_le<uint64_t>(meta, static_cast<uint64_t>(plans.size()));
  append_le<uint64_t>(meta, static_cast<uint64_t>(kvs.size()));

  for (const auto& [key, value] : kvs) {
    append_string(meta, key);
    if (std::holds_alternative<std::string>(value)) {
      append_le<uint32_t>(meta, kGGUFTypeString);
      append_string(meta, std::get<std::string>(value));
    } else if (std::holds_alternative<uint32_t>(value)) {
      append_le<uint32_t>(meta, kGGUFTypeUInt32);
      append_le<uint32_t>(meta, std::get<uint32_t>(value));
    } else {
      append_le<uint32_t>(meta, kGGUFTypeArray);
      append_le<uint32_t>(meta, kGGUFTypeUInt8);
      append_le<uint64_t>(meta,
                          static_cast<uint64_t>(std::get<std::array<uint8_t, 64>>(value).size()));
      const auto& arr = std::get<std::array<uint8_t, 64>>(value);
      meta.insert(meta.end(), arr.begin(), arr.end());
    }
  }

  for (const auto& plan : plans) {
    append_string(meta, plan.tensor.name);
    append_le<uint32_t>(meta, static_cast<uint32_t>(plan.gguf_shape.size()));
    for (uint64_t dim : plan.gguf_shape) {
      append_le<uint64_t>(meta, dim);
    }
    append_le<uint32_t>(meta, kGGMLTypeT3K);
    append_le<uint64_t>(meta, plan.data_offset);
  }

  const uint64_t meta_padded_size = align_up(static_cast<uint64_t>(meta.size()), kGGUFAlignment);
  std::vector<uint8_t> file_bytes(static_cast<size_t>(meta_padded_size + data_cursor), 0);
  std::copy(meta.begin(), meta.end(), file_bytes.begin());
  std::copy(quantized_data.begin(), quantized_data.end(),
            file_bytes.begin() + static_cast<std::ptrdiff_t>(meta_padded_size));

  std::ofstream out(output, std::ios::binary);
  if (!out) throw std::runtime_error("cannot write " + output.string());
  out.write(reinterpret_cast<const char*>(file_bytes.data()),
            static_cast<std::streamsize>(file_bytes.size()));
  uint64_t mb = file_bytes.size() >> 20;
  std::cout << "Success! T3_K GGUF created: " << output << " (" << mb << " MB)\n";
  std::cout << "Run with llama.cpp (latest):\n";
  std::cout << "  ./llama-cli -m " << output << " -p \"Hello\" -n 512 --color\n";
  std::cout << "Note: You need llama.cpp with T3_K support (PR coming soon)\n";
}

}  // namespace t81::weights
