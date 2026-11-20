#include "tools/cli/core/cmd_weights_model.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "t81/cli/driver.hpp"
#include "t81/weights.hpp"
#include "tools/cli/core/help_text.hpp"
#include "tools/cli/logging.hpp"

namespace fs = std::filesystem;

std::string json_escape(std::string_view text);

namespace {

struct ScopedStreamRedirect {
  std::ostream& from;
  std::streambuf* old_buf;

  ScopedStreamRedirect(std::ostream& from_stream, std::ostream& to_stream)
      : from(from_stream), old_buf(from_stream.rdbuf(to_stream.rdbuf())) {}

  ~ScopedStreamRedirect() { from.rdbuf(old_buf); }
};

template <typename Fn>
int emit_help(Fn&& fn, int rc = 0) {
  ScopedStreamRedirect redirect(std::cerr, std::cout);
  fn();
  return rc;
}

}  // namespace

struct WeightsImportOptions {
  fs::path input;
  std::optional<fs::path> output;
  std::string format = "safetensors";
  bool format_explicit = false;
};

struct ModelInspectOptions {
  fs::path input;
  std::string format = "safetensors";
  bool format_explicit = false;
  bool as_json = false;
  std::optional<fs::path> output;
  std::optional<fs::path> manifest_output;
};

bool has_extension_ci(const fs::path& path, std::string_view expected_ext) {
  auto lower = [](std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
      out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
  };
  std::string ext = lower(path.extension().string());
  std::string expected = lower(expected_ext);
  return ext == expected;
}

class ModelManifestJsonParser {
public:
  explicit ModelManifestJsonParser(const std::string& text) : text_(text), idx_(0) {}

  t81::weights::JsonValue parse() {
    skip();
    return parse_value();
  }

private:
  const std::string& text_;
  std::size_t idx_;

  void skip() {
    while (idx_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[idx_]))) {
      ++idx_;
    }
  }

  char peek() const { return idx_ < text_.size() ? text_[idx_] : '\0'; }

  char consume() { return idx_ < text_.size() ? text_[idx_++] : '\0'; }

  t81::weights::JsonValue parse_value() {
    skip();
    const char c = peek();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '"') return parse_string();
    if ((c >= '0' && c <= '9') || c == '-') return parse_number();
    throw std::runtime_error("JSON parse error");
  }

  t81::weights::JsonValue parse_object() {
    consume();
    std::map<std::string, t81::weights::JsonValue> map;
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
    return t81::weights::JsonValue::make_object(std::move(map));
  }

  t81::weights::JsonValue parse_array() {
    consume();
    std::vector<t81::weights::JsonValue> arr;
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
    return t81::weights::JsonValue::make_array(std::move(arr));
  }

  t81::weights::JsonValue parse_string() {
    consume();
    std::string out;
    while (idx_ < text_.size()) {
      const char c = consume();
      if (c == '"') break;
      if (c == '\\') {
        const char esc = consume();
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
    return t81::weights::JsonValue::make_string(std::move(out));
  }

  t81::weights::JsonValue parse_number() {
    const std::size_t start = idx_;
    if (peek() == '-') ++idx_;
    while (std::isdigit(static_cast<unsigned char>(peek()))) ++idx_;
    if (peek() == '.') {
      ++idx_;
      while (std::isdigit(static_cast<unsigned char>(peek()))) ++idx_;
    }
    return t81::weights::JsonValue::make_number(std::stod(text_.substr(start, idx_ - start)));
  }
};

const t81::weights::JsonValue& require_object_field(const t81::weights::JsonValue& object,
                                                    std::string_view key,
                                                    std::string_view command_name) {
  const auto it = object.object_value.find(std::string(key));
  if (it == object.object_value.end()) {
    throw std::runtime_error(std::string(command_name) + ": manifest missing field '" +
                             std::string(key) + "'");
  }
  return it->second;
}

std::string require_string_field(const t81::weights::JsonValue& object, std::string_view key,
                                 std::string_view command_name) {
  const auto& value = require_object_field(object, key, command_name);
  if (!value.is_string) {
    throw std::runtime_error(std::string(command_name) + ": manifest field '" + std::string(key) +
                             "' must be a string");
  }
  return value.string_value;
}

uint64_t require_uint_field(const t81::weights::JsonValue& object, std::string_view key,
                            std::string_view command_name) {
  const auto& value = require_object_field(object, key, command_name);
  if (!value.is_number || value.number_value < 0.0 ||
      std::floor(value.number_value) != value.number_value) {
    throw std::runtime_error(std::string(command_name) + ": manifest field '" + std::string(key) +
                             "' must be a non-negative whole number");
  }
  return static_cast<uint64_t>(value.number_value);
}

std::vector<uint64_t> manifest_shape_from_json(const t81::weights::JsonValue& value,
                                               std::string_view command_name) {
  if (value.array_value.empty()) {
    throw std::runtime_error(std::string(command_name) +
                             ": manifest tensor shape must be an array");
  }
  std::vector<uint64_t> shape;
  shape.reserve(value.array_value.size());
  for (const auto& entry : value.array_value) {
    if (!entry.is_number || entry.number_value < 1.0 ||
        std::floor(entry.number_value) != entry.number_value) {
      throw std::runtime_error(std::string(command_name) +
                               ": manifest tensor shape entries must be positive whole numbers");
    }
    shape.push_back(static_cast<uint64_t>(entry.number_value));
  }
  return shape;
}

std::string read_text_file_or_throw(const fs::path& path, std::string_view command_name) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error(std::string(command_name) + ": could not read " + path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

struct LoadedModelArtifact {
  t81::weights::ModelFile model;
  std::string source_format;
};

LoadedModelArtifact load_model_manifest_artifact(const fs::path& path,
                                                 std::string_view command_name) {
  const std::string text = read_text_file_or_throw(path, command_name);
  ModelManifestJsonParser parser(text);
  const auto root = parser.parse();
  const std::string schema = require_string_field(root, "schema", command_name);
  if (schema != "t81.model-manifest.v1") {
    throw std::runtime_error(std::string(command_name) + ": unsupported manifest schema '" +
                             schema + "'");
  }

  LoadedModelArtifact loaded;
  loaded.source_format = require_string_field(root, "source_format", command_name);
  auto& mf = loaded.model;
  const auto imported_format_it = root.object_value.find("imported_format_summary");
  if (imported_format_it != root.object_value.end()) {
    if (!imported_format_it->second.is_string) {
      throw std::runtime_error(std::string(command_name) +
                               ": manifest field 'imported_format_summary' must be a string");
    }
    mf.format = imported_format_it->second.string_value;
  } else {
    mf.format = require_string_field(root, "artifact_format", command_name);
  }
  mf.total_parameters = require_uint_field(root, "parameters", command_name);
  mf.total_trits = require_uint_field(root, "trits", command_name);
  mf.file_size = require_uint_field(root, "file_size_bytes", command_name);

  const auto& provenance = require_object_field(root, "provenance", command_name);
  for (const auto& [key, value] : provenance.object_value) {
    if (!value.is_string) {
      throw std::runtime_error(std::string(command_name) +
                               ": manifest provenance values must be strings");
    }
    mf.provenance.emplace(key, value.string_value);
  }

  const auto& tensors = require_object_field(root, "tensors", command_name);
  mf.tensors.reserve(tensors.array_value.size());
  for (const auto& tensor_value : tensors.array_value) {
    t81::weights::TensorInfo info;
    info.name = require_string_field(tensor_value, "name", command_name);
    info.shape = manifest_shape_from_json(require_object_field(tensor_value, "shape", command_name),
                                          command_name);
    info.num_trits = require_uint_field(tensor_value, "trits", command_name);
    info.sparsity = std::numeric_limits<double>::quiet_NaN();
    const auto sparsity_it = tensor_value.object_value.find("sparsity");
    if (sparsity_it != tensor_value.object_value.end()) {
      if (!sparsity_it->second.is_number) {
        throw std::runtime_error(std::string(command_name) +
                                 ": manifest tensor field 'sparsity' must be a number");
      }
      info.sparsity = sparsity_it->second.number_value;
    }
    mf.tensors.push_back(std::move(info));
  }

  return loaded;
}

t81::weights::ModelFile load_model_artifact(const fs::path& input, const std::string& format,
                                            std::string_view command_name,
                                            std::string_view help_topic);

LoadedModelArtifact load_model_artifact_or_manifest(const fs::path& input,
                                                    const std::string& format,
                                                    std::string_view command_name,
                                                    std::string_view help_topic) {
  if (has_extension_ci(input, ".json")) {
    return load_model_manifest_artifact(input, command_name);
  }
  LoadedModelArtifact loaded;
  loaded.source_format = format;
  loaded.model = load_model_artifact(input, format, command_name, help_topic);
  return loaded;
}

bool resolve_model_input_and_format(fs::path& input, std::string& format, bool format_explicit,
                                    std::string_view command_name) {
  if (!fs::exists(input)) {
    std::string resolution_error;
    auto resolved = t81::cli::resolve_repo_model_path(input.string(), {".gguf", ".safetensors"},
                                                      &resolution_error);
    if (!resolved) {
      error(std::string(command_name) + ": " + resolution_error);
      return false;
    }
    input = *resolved;
  }

  if (!format_explicit) {
    if (has_extension_ci(input, ".gguf")) {
      format = "gguf";
    } else if (has_extension_ci(input, ".safetensors")) {
      format = "safetensors";
    }
  }
  return true;
}

t81::weights::ModelFile load_model_artifact(const fs::path& input, const std::string& format,
                                            std::string_view command_name,
                                            std::string_view help_topic) {
  if (format == "safetensors") {
    if (!has_extension_ci(input, ".safetensors")) {
      throw std::runtime_error(
          std::string(command_name) +
          ": --format safetensors requires a .safetensors input file. Run 't81 " +
          std::string(help_topic) + "'.");
    }
    return t81::weights::load_safetensors(input);
  }
  if (format == "bitnet") {
    if (!has_extension_ci(input, ".safetensors")) {
      throw std::runtime_error(std::string(command_name) +
                               ": --format bitnet requires a .safetensors input file. Run 't81 " +
                               std::string(help_topic) + "'.");
    }
    return t81::weights::load_bitnet_safetensors(input);
  }
  if (format == "gguf") {
    if (!has_extension_ci(input, ".gguf")) {
      throw std::runtime_error(std::string(command_name) +
                               ": --format gguf requires a .gguf input file. Run 't81 " +
                               std::string(help_topic) + "'.");
    }
    return t81::weights::load_gguf(input);
  }
  throw std::runtime_error(std::string(command_name) + ": unsupported format '" + format +
                           "'. Supported: safetensors, bitnet, gguf. Run 't81 " +
                           std::string(help_topic) + "'.");
}

void emit_model_import_json(const fs::path& input, const std::string& source_format,
                            const t81::weights::ModelFile& mf) {
  std::cout << "{\n";
  std::cout << "  \"schema\": \"t81.model-import.v1\",\n";
  std::cout << "  \"ok\": true,\n";
  std::cout << "  \"input\": \"" << json_escape(input.string()) << "\",\n";
  std::cout << "  \"source_format\": \"" << json_escape(source_format) << "\",\n";
  std::cout << "  \"artifact_format\": \"" << json_escape(mf.format) << "\",\n";
  std::cout << "  \"tensor_count\": " << mf.tensors.size() << ",\n";
  std::cout << "  \"parameters\": " << mf.total_parameters << ",\n";
  std::cout << "  \"trits\": " << mf.total_trits << ",\n";
  std::cout << "  \"file_size_bytes\": " << mf.file_size << ",\n";
  std::cout << "  \"provenance\": {\n";
  std::size_t provenance_index = 0;
  for (const auto& [key, value] : mf.provenance) {
    std::cout << "    \"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
    if (++provenance_index < mf.provenance.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << "  },\n";
  std::cout << "  \"tensors\": [\n";
  for (std::size_t i = 0; i < mf.tensors.size(); ++i) {
    const auto& tensor = mf.tensors[i];
    std::cout << "    {\n";
    std::cout << "      \"name\": \"" << json_escape(tensor.name) << "\",\n";
    std::cout << "      \"shape\": [";
    for (std::size_t j = 0; j < tensor.shape.size(); ++j) {
      if (j > 0) {
        std::cout << ", ";
      }
      std::cout << tensor.shape[j];
    }
    std::cout << "],\n";
    std::cout << "      \"trits\": " << tensor.num_trits << ",\n";
    std::cout << "      \"sparsity\": " << std::fixed << std::setprecision(6) << tensor.sparsity
              << "\n";
    std::cout << std::defaultfloat;
    std::cout << "    }";
    if (i + 1 < mf.tensors.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << "  ]\n";
  std::cout << "}\n";
}

void emit_model_import_report(const fs::path& input, const std::string& source_format,
                              const t81::weights::ModelFile& mf) {
  std::cout << "Model Artifact: " << input << "\n";
  std::cout << "Source Format:  " << source_format << "\n";
  std::cout << "Artifact Type:  " << mf.format << "\n";
  std::cout << "Tensors:        " << mf.tensors.size() << "\n";
  std::cout << "Parameters:     " << t81::weights::format_count(mf.total_parameters) << "\n";
  std::cout << "Trits:          " << t81::weights::format_count(mf.total_trits) << "\n";
  if (mf.file_size > 0) {
    std::cout << "File Size:      " << t81::weights::format_bytes(mf.file_size) << "\n";
  }
  if (!mf.provenance.empty()) {
    std::cout << "Provenance:\n";
    for (const auto& [key, value] : mf.provenance) {
      std::cout << "  " << key << ": " << value << "\n";
    }
  }
  std::cout << "Tensor Inventory:\n";
  for (const auto& tensor : mf.tensors) {
    std::cout << "  - " << tensor.name << " shape=[";
    for (std::size_t i = 0; i < tensor.shape.size(); ++i) {
      if (i > 0) {
        std::cout << "x";
      }
      std::cout << tensor.shape[i];
    }
    std::cout << "] trits=" << tensor.num_trits;
    std::cout << " sparsity=" << std::fixed << std::setprecision(3) << (tensor.sparsity * 100.0)
              << "%" << std::defaultfloat << "\n";
  }
}

void emit_model_manifest_json(const fs::path& input, const std::string& source_format,
                              const t81::weights::ModelFile& mf) {
  std::map<std::string, std::string> manifest_provenance;
  manifest_provenance.emplace("host_format_reader", source_format);
  if (const auto it = mf.provenance.find("source_sha3_512"); it != mf.provenance.end()) {
    manifest_provenance.emplace(it->first, it->second);
  }
  if (const auto it = mf.provenance.find("gguf_version"); it != mf.provenance.end()) {
    manifest_provenance.emplace(it->first, it->second);
  }
  if (const auto it = mf.provenance.find("embedded_source_sha3_512"); it != mf.provenance.end()) {
    manifest_provenance.emplace(it->first, it->second);
  }
  if (const auto it = mf.provenance.find("bridge_backend"); it != mf.provenance.end()) {
    manifest_provenance.emplace(it->first, it->second);
  }
  if (const auto it = mf.provenance.find("bridge_revision"); it != mf.provenance.end()) {
    manifest_provenance.emplace(it->first, it->second);
  }

  std::cout << "{\n";
  std::cout << "  \"schema\": \"t81.model-manifest.v1\",\n";
  std::cout << "  \"source_path\": \"" << json_escape(input.string()) << "\",\n";
  std::cout << "  \"source_format\": \"" << json_escape(source_format) << "\",\n";
  std::cout << "  \"normalized_artifact_type\": \"model-tensor-manifest\",\n";
  std::cout << "  \"imported_format_summary\": \"" << json_escape(mf.format) << "\",\n";
  std::cout << "  \"tensor_count\": " << mf.tensors.size() << ",\n";
  std::cout << "  \"parameters\": " << mf.total_parameters << ",\n";
  std::cout << "  \"trits\": " << mf.total_trits << ",\n";
  std::cout << "  \"file_size_bytes\": " << mf.file_size << ",\n";
  std::cout << "  \"provenance\": {\n";
  std::size_t provenance_index = 0;
  for (const auto& [key, value] : manifest_provenance) {
    std::cout << "    \"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
    if (++provenance_index < manifest_provenance.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << "  },\n";
  std::cout << "  \"tensors\": [\n";
  for (std::size_t i = 0; i < mf.tensors.size(); ++i) {
    const auto& tensor = mf.tensors[i];
    std::cout << "    {\n";
    std::cout << "      \"name\": \"" << json_escape(tensor.name) << "\",\n";
    std::cout << "      \"shape\": [";
    for (std::size_t j = 0; j < tensor.shape.size(); ++j) {
      if (j > 0) {
        std::cout << ", ";
      }
      std::cout << tensor.shape[j];
    }
    std::cout << "],\n";
    std::cout << "      \"trits\": " << tensor.num_trits << "\n";
    std::cout << "    }";
    if (i + 1 < mf.tensors.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << "  ]\n";
  std::cout << "}\n";
}

struct ModelTensorDiffSummary {
  std::vector<std::string> lhs_only;
  std::vector<std::string> rhs_only;
  std::vector<std::string> changed;
};

struct ProvenanceDiffSummary {
  std::vector<std::string> lhs_only;
  std::vector<std::string> rhs_only;
  std::vector<std::string> changed;
};

struct NormalizedModelTensorDiffSummary {
  std::vector<std::string> lhs_only;
  std::vector<std::string> rhs_only;
  std::vector<std::string> changed;
  std::vector<std::string> normalized_matches;
  std::vector<std::string> normalization_rules;
  std::map<std::string, std::string> normalized_match_reasons;
};

void emit_count_field(std::string_view key, std::size_t value, int indent = 2) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  std::cout << pad << "\"" << key << "\": " << value;
}

std::map<std::string, const t81::weights::TensorInfo*> index_model_tensors(
    const t81::weights::ModelFile& mf) {
  std::map<std::string, const t81::weights::TensorInfo*> indexed;
  for (const auto& tensor : mf.tensors) {
    indexed.emplace(tensor.name, &tensor);
  }
  return indexed;
}

ProvenanceDiffSummary diff_provenance(const std::map<std::string, std::string>& lhs,
                                      const std::map<std::string, std::string>& rhs) {
  ProvenanceDiffSummary summary;
  for (const auto& [key, lhs_value] : lhs) {
    const auto rhs_it = rhs.find(key);
    if (rhs_it == rhs.end()) {
      summary.lhs_only.push_back(key);
      continue;
    }
    if (lhs_value != rhs_it->second) {
      summary.changed.push_back(key);
    }
  }

  for (const auto& [key, rhs_value] : rhs) {
    (void)rhs_value;
    if (!lhs.contains(key)) {
      summary.rhs_only.push_back(key);
    }
  }
  return summary;
}

bool same_tensor_shape(const std::vector<uint64_t>& lhs, const std::vector<uint64_t>& rhs) {
  return lhs == rhs;
}

bool tensor_sparsity_mismatch(double lhs, double rhs) {
  if (std::isnan(lhs) || std::isnan(rhs)) {
    return false;
  }
  return std::abs(lhs - rhs) > 1e-12;
}

ModelTensorDiffSummary diff_model_tensors(const t81::weights::ModelFile& lhs,
                                          const t81::weights::ModelFile& rhs) {
  ModelTensorDiffSummary summary;
  const auto lhs_index = index_model_tensors(lhs);
  const auto rhs_index = index_model_tensors(rhs);

  for (const auto& [name, lhs_tensor] : lhs_index) {
    const auto rhs_it = rhs_index.find(name);
    if (rhs_it == rhs_index.end()) {
      summary.lhs_only.push_back(name);
      continue;
    }
    const auto* rhs_tensor = rhs_it->second;
    if (!same_tensor_shape(lhs_tensor->shape, rhs_tensor->shape) ||
        lhs_tensor->num_trits != rhs_tensor->num_trits ||
        tensor_sparsity_mismatch(lhs_tensor->sparsity, rhs_tensor->sparsity)) {
      summary.changed.push_back(name);
    }
  }

  for (const auto& [name, rhs_tensor] : rhs_index) {
    (void)rhs_tensor;
    if (!lhs_index.contains(name)) {
      summary.rhs_only.push_back(name);
    }
  }
  return summary;
}

bool is_cross_format_normalization_pair(std::string_view lhs_source_format,
                                        std::string_view rhs_source_format) {
  const bool lhs_gguf = lhs_source_format == "gguf";
  const bool rhs_gguf = rhs_source_format == "gguf";
  const bool lhs_safetensors_family =
      lhs_source_format == "safetensors" || lhs_source_format == "bitnet";
  const bool rhs_safetensors_family =
      rhs_source_format == "safetensors" || rhs_source_format == "bitnet";
  return (lhs_gguf && rhs_safetensors_family) || (rhs_gguf && lhs_safetensors_family);
}

bool shape_matches_normalized(std::span<const uint64_t> lhs, std::span<const uint64_t> rhs,
                              bool allow_known_transpose) {
  if (lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin())) {
    return true;
  }
  if (allow_known_transpose && lhs.size() == 2 && rhs.size() == 2 && lhs[0] == rhs[1] &&
      lhs[1] == rhs[0]) {
    return true;
  }
  return false;
}

NormalizedModelTensorDiffSummary diff_model_tensors_normalized(const LoadedModelArtifact& lhs,
                                                               const LoadedModelArtifact& rhs) {
  NormalizedModelTensorDiffSummary summary;
  const auto lhs_index = index_model_tensors(lhs.model);
  const auto rhs_index = index_model_tensors(rhs.model);
  const bool allow_known_transpose =
      is_cross_format_normalization_pair(lhs.source_format, rhs.source_format);
  bool used_known_transpose = false;

  for (const auto& [name, lhs_tensor] : lhs_index) {
    const auto rhs_it = rhs_index.find(name);
    if (rhs_it == rhs_index.end()) {
      summary.lhs_only.push_back(name);
      continue;
    }

    const auto* rhs_tensor = rhs_it->second;
    if (lhs_tensor->num_trits != rhs_tensor->num_trits) {
      summary.changed.push_back(name);
      continue;
    }

    if (lhs_tensor->shape == rhs_tensor->shape) {
      continue;
    }

    if (allow_known_transpose && lhs_tensor->shape.size() == 2 && rhs_tensor->shape.size() == 2 &&
        lhs_tensor->shape[0] == rhs_tensor->shape[1] &&
        lhs_tensor->shape[1] == rhs_tensor->shape[0]) {
      summary.normalized_matches.push_back(name);
      summary.normalized_match_reasons.emplace(name, "known_2d_transpose_for_gguf_safetensors");
      used_known_transpose = true;
      continue;
    }

    summary.changed.push_back(name);
  }

  for (const auto& [name, rhs_tensor] : rhs_index) {
    (void)rhs_tensor;
    if (!lhs_index.contains(name)) {
      summary.rhs_only.push_back(name);
    }
  }

  if (used_known_transpose) {
    summary.normalization_rules.push_back("known_2d_transpose_for_gguf_safetensors");
  }
  return summary;
}

void emit_string_array(std::string_view key, const std::vector<std::string>& values,
                       int indent = 2) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  std::cout << pad << "\"" << key << "\": [";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      std::cout << ", ";
    }
    std::cout << "\"" << json_escape(values[i]) << "\"";
  }
  std::cout << "]";
}

void emit_string_map(std::string_view key, const std::map<std::string, std::string>& values,
                     int indent = 2) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  const std::string child_pad(static_cast<std::size_t>(indent + 2), ' ');
  std::cout << pad << "\"" << key << "\": {";
  if (values.empty()) {
    std::cout << "}";
    return;
  }
  std::cout << "\n";
  std::size_t index = 0;
  for (const auto& [map_key, map_value] : values) {
    std::cout << child_pad << "\"" << json_escape(map_key) << "\": \"" << json_escape(map_value)
              << "\"";
    if (++index < values.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << pad << "}";
}

void emit_model_diff_json(const fs::path& lhs_path, const fs::path& rhs_path,
                          const t81::weights::ModelFile& lhs, const t81::weights::ModelFile& rhs,
                          const ModelTensorDiffSummary& tensor_diff,
                          const ProvenanceDiffSummary& provenance_diff) {
  const bool identical = lhs.format == rhs.format && lhs.total_parameters == rhs.total_parameters &&
                         lhs.total_trits == rhs.total_trits &&
                         lhs.tensors.size() == rhs.tensors.size() && tensor_diff.lhs_only.empty() &&
                         tensor_diff.rhs_only.empty() && tensor_diff.changed.empty();

  std::cout << "{\n";
  std::cout << "  \"schema\": \"t81.model-diff.v1\",\n";
  std::cout << "  \"lhs\": \"" << json_escape(lhs_path.string()) << "\",\n";
  std::cout << "  \"rhs\": \"" << json_escape(rhs_path.string()) << "\",\n";
  std::cout << "  \"identical\": " << (identical ? "true" : "false") << ",\n";
  std::cout << "  \"lhs_format\": \"" << json_escape(lhs.format) << "\",\n";
  std::cout << "  \"rhs_format\": \"" << json_escape(rhs.format) << "\",\n";
  std::cout << "  \"lhs_tensor_count\": " << lhs.tensors.size() << ",\n";
  std::cout << "  \"rhs_tensor_count\": " << rhs.tensors.size() << ",\n";
  std::cout << "  \"lhs_parameters\": " << lhs.total_parameters << ",\n";
  std::cout << "  \"rhs_parameters\": " << rhs.total_parameters << ",\n";
  std::cout << "  \"lhs_trits\": " << lhs.total_trits << ",\n";
  std::cout << "  \"rhs_trits\": " << rhs.total_trits << ",\n";
  emit_count_field("lhs_only_count", tensor_diff.lhs_only.size());
  std::cout << ",\n";
  emit_count_field("rhs_only_count", tensor_diff.rhs_only.size());
  std::cout << ",\n";
  emit_count_field("changed_count", tensor_diff.changed.size());
  std::cout << ",\n";
  emit_count_field("provenance_lhs_only_count", provenance_diff.lhs_only.size());
  std::cout << ",\n";
  emit_count_field("provenance_rhs_only_count", provenance_diff.rhs_only.size());
  std::cout << ",\n";
  emit_count_field("provenance_changed_count", provenance_diff.changed.size());
  std::cout << ",\n";
  emit_string_array("lhs_only", tensor_diff.lhs_only);
  std::cout << ",\n";
  emit_string_array("rhs_only", tensor_diff.rhs_only);
  std::cout << ",\n";
  emit_string_array("changed", tensor_diff.changed);
  std::cout << ",\n";
  emit_string_array("provenance_lhs_only", provenance_diff.lhs_only);
  std::cout << ",\n";
  emit_string_array("provenance_rhs_only", provenance_diff.rhs_only);
  std::cout << ",\n";
  emit_string_array("provenance_changed", provenance_diff.changed);
  std::cout << "\n}\n";
}

void emit_model_diff_normalized_json(const fs::path& lhs_path, const fs::path& rhs_path,
                                     const LoadedModelArtifact& lhs, const LoadedModelArtifact& rhs,
                                     const NormalizedModelTensorDiffSummary& tensor_diff,
                                     const ProvenanceDiffSummary& provenance_diff) {
  const bool identical = lhs.model.total_parameters == rhs.model.total_parameters &&
                         lhs.model.total_trits == rhs.model.total_trits &&
                         lhs.model.tensors.size() == rhs.model.tensors.size() &&
                         tensor_diff.lhs_only.empty() && tensor_diff.rhs_only.empty() &&
                         tensor_diff.changed.empty();

  std::cout << "{\n";
  std::cout << "  \"schema\": \"t81.model-diff-normalized.v1\",\n";
  std::cout << "  \"comparison_mode\": \"normalized\",\n";
  std::cout << "  \"lhs\": \"" << json_escape(lhs_path.string()) << "\",\n";
  std::cout << "  \"rhs\": \"" << json_escape(rhs_path.string()) << "\",\n";
  std::cout << "  \"identical\": " << (identical ? "true" : "false") << ",\n";
  std::cout << "  \"lhs_format\": \"" << json_escape(lhs.model.format) << "\",\n";
  std::cout << "  \"rhs_format\": \"" << json_escape(rhs.model.format) << "\",\n";
  std::cout << "  \"lhs_source_format\": \"" << json_escape(lhs.source_format) << "\",\n";
  std::cout << "  \"rhs_source_format\": \"" << json_escape(rhs.source_format) << "\",\n";
  std::cout << "  \"lhs_tensor_count\": " << lhs.model.tensors.size() << ",\n";
  std::cout << "  \"rhs_tensor_count\": " << rhs.model.tensors.size() << ",\n";
  std::cout << "  \"lhs_parameters\": " << lhs.model.total_parameters << ",\n";
  std::cout << "  \"rhs_parameters\": " << rhs.model.total_parameters << ",\n";
  std::cout << "  \"lhs_trits\": " << lhs.model.total_trits << ",\n";
  std::cout << "  \"rhs_trits\": " << rhs.model.total_trits << ",\n";
  emit_count_field("lhs_only_count", tensor_diff.lhs_only.size());
  std::cout << ",\n";
  emit_count_field("rhs_only_count", tensor_diff.rhs_only.size());
  std::cout << ",\n";
  emit_count_field("changed_count", tensor_diff.changed.size());
  std::cout << ",\n";
  emit_count_field("normalized_match_count", tensor_diff.normalized_matches.size());
  std::cout << ",\n";
  emit_count_field("provenance_lhs_only_count", provenance_diff.lhs_only.size());
  std::cout << ",\n";
  emit_count_field("provenance_rhs_only_count", provenance_diff.rhs_only.size());
  std::cout << ",\n";
  emit_count_field("provenance_changed_count", provenance_diff.changed.size());
  std::cout << ",\n";
  emit_string_array("lhs_only", tensor_diff.lhs_only);
  std::cout << ",\n";
  emit_string_array("rhs_only", tensor_diff.rhs_only);
  std::cout << ",\n";
  emit_string_array("changed", tensor_diff.changed);
  std::cout << ",\n";
  emit_string_array("normalized_matches", tensor_diff.normalized_matches);
  std::cout << ",\n";
  emit_string_array("normalization_rules", tensor_diff.normalization_rules);
  std::cout << ",\n";
  emit_string_array("provenance_lhs_only", provenance_diff.lhs_only);
  std::cout << ",\n";
  emit_string_array("provenance_rhs_only", provenance_diff.rhs_only);
  std::cout << ",\n";
  emit_string_array("provenance_changed", provenance_diff.changed);
  std::cout << ",\n";
  emit_string_map("normalized_match_reasons", tensor_diff.normalized_match_reasons);
  std::cout << "\n}\n";
}

void emit_model_diff_report(const fs::path& lhs_path, const fs::path& rhs_path,
                            const t81::weights::ModelFile& lhs, const t81::weights::ModelFile& rhs,
                            const ModelTensorDiffSummary& tensor_diff,
                            const ProvenanceDiffSummary& provenance_diff) {
  const bool identical = lhs.format == rhs.format && lhs.total_parameters == rhs.total_parameters &&
                         lhs.total_trits == rhs.total_trits &&
                         lhs.tensors.size() == rhs.tensors.size() && tensor_diff.lhs_only.empty() &&
                         tensor_diff.rhs_only.empty() && tensor_diff.changed.empty();

  std::cout << "LHS:            " << lhs_path << "\n";
  std::cout << "RHS:            " << rhs_path << "\n";
  std::cout << "Identical:      " << (identical ? "yes" : "no") << "\n";
  std::cout << "LHS Format:     " << lhs.format << "\n";
  std::cout << "RHS Format:     " << rhs.format << "\n";
  std::cout << "LHS Tensors:    " << lhs.tensors.size() << "\n";
  std::cout << "RHS Tensors:    " << rhs.tensors.size() << "\n";
  std::cout << "LHS Parameters: " << lhs.total_parameters << "\n";
  std::cout << "RHS Parameters: " << rhs.total_parameters << "\n";
  std::cout << "LHS Trits:      " << lhs.total_trits << "\n";
  std::cout << "RHS Trits:      " << rhs.total_trits << "\n";
  std::cout << "Summary:\n";
  std::cout << "  tensor lhs_only=" << tensor_diff.lhs_only.size()
            << " rhs_only=" << tensor_diff.rhs_only.size()
            << " changed=" << tensor_diff.changed.size() << "\n";
  std::cout << "  provenance lhs_only=" << provenance_diff.lhs_only.size()
            << " rhs_only=" << provenance_diff.rhs_only.size()
            << " changed=" << provenance_diff.changed.size() << "\n";
  if (!tensor_diff.lhs_only.empty()) {
    std::cout << "Only in LHS:\n";
    for (const auto& name : tensor_diff.lhs_only) {
      std::cout << "  - " << name << "\n";
    }
  }
  if (!tensor_diff.rhs_only.empty()) {
    std::cout << "Only in RHS:\n";
    for (const auto& name : tensor_diff.rhs_only) {
      std::cout << "  - " << name << "\n";
    }
  }
  if (!tensor_diff.changed.empty()) {
    std::cout << "Changed Tensors:\n";
    for (const auto& name : tensor_diff.changed) {
      std::cout << "  - " << name << "\n";
    }
  }
  if (!provenance_diff.lhs_only.empty() || !provenance_diff.rhs_only.empty() ||
      !provenance_diff.changed.empty()) {
    std::cout << "Provenance Differences:\n";
    for (const auto& key : provenance_diff.lhs_only) {
      std::cout << "  - lhs_only: " << key << "\n";
    }
    for (const auto& key : provenance_diff.rhs_only) {
      std::cout << "  - rhs_only: " << key << "\n";
    }
    for (const auto& key : provenance_diff.changed) {
      std::cout << "  - changed: " << key << "\n";
    }
  }
}

bool write_text_file(const fs::path& output, std::string_view text, std::string_view command_name) {
  std::ofstream out(output, std::ios::binary | std::ios::trunc);
  if (!out) {
    error(std::string(command_name) + ": could not write " + output.string());
    return false;
  }
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (!out) {
    error(std::string(command_name) + ": could not write " + output.string());
    return false;
  }
  return true;
}

std::vector<int8_t> unpack_native_trits(const t81::weights::NativeTensor& tensor) {
  std::vector<int8_t> out;
  const uint64_t total_trits = tensor.num_trits();
  out.reserve(total_trits);
  constexpr std::array<int8_t, 3> kMap = {-1, 0, 1};
  for (std::size_t limb_index = 0; limb_index < tensor.data.size() && out.size() < total_trits;
       ++limb_index) {
    uint64_t limb = tensor.data[limb_index];
    std::array<int8_t, 48> decoded{};
    for (int i = 47; i >= 0; --i) {
      decoded[static_cast<std::size_t>(i)] = kMap[limb % 3];
      limb /= 3;
    }
    for (int i = 0; i < 48 && out.size() < total_trits; ++i) {
      out.push_back(decoded[static_cast<std::size_t>(i)]);
    }
  }
  return out;
}

int run_weights_import(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights import requires an input file. Run 't81 help weights import'.");
    return 1;
  }
  WeightsImportOptions opts;
  opts.input = fs::path(args.command_args[1]);
  opts.output = args.output;
  size_t idx = 2;
  while (idx < args.command_args.size()) {
    const auto& token = args.command_args[idx++];
    if (token == "--format") {
      if (idx >= args.command_args.size()) {
        error("weights import: missing argument for --format. Run 't81 help weights import'.");
        return 1;
      }
      opts.format = args.command_args[idx++];
      opts.format_explicit = true;
    } else if (token == "-o" || token == "--out" || token == "--output") {
      if (idx >= args.command_args.size()) {
        error("weights import: missing argument for " + token + ". Run 't81 help weights import'.");
        return 1;
      }
      opts.output = fs::path(args.command_args[idx++]);
    } else if (opts.input.empty()) {
      opts.input = fs::path(token);
    } else {
      error("weights import: unexpected argument '" + token + "'. Run 't81 help weights import'.");
      return 1;
    }
  }
  if (opts.input.empty()) {
    error("weights import requires an input file. Run 't81 help weights import'.");
    return 1;
  }
  if (!opts.output) {
    opts.output = opts.input.stem();
    opts.output->replace_extension(".t81w");
  }
  if (!has_extension_ci(*opts.output, ".t81w")) {
    error("weights import: output must use .t81w extension. Run 't81 help weights import'.");
    return 1;
  }

  if (!resolve_model_input_and_format(opts.input, opts.format, opts.format_explicit,
                                      "weights import")) {
    return 1;
  }

  t81::weights::ModelFile mf;
  try {
    mf = load_model_artifact(opts.input, opts.format, "weights import", "help weights import");
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }
  t81::weights::print_info(mf);
  if (mf.native.empty()) {
    error("weights import: loader produced no native tensors");
    return 1;
  }
  t81::weights::save_t81w(mf.native, *opts.output);
  info("Saved " + opts.output->string());
  return 0;
}

int run_model_import(const Args& args) {
  if (args.command_args.size() < 2) {
    error("model import requires an input file. Run 't81 help model import'.");
    return 1;
  }

  ModelInspectOptions opts;
  opts.input = fs::path(args.command_args[1]);
  opts.output = args.output;
  std::size_t idx = 2;
  while (idx < args.command_args.size()) {
    const auto& token = args.command_args[idx++];
    if (token == "--format") {
      if (idx >= args.command_args.size()) {
        error("model import: missing argument for --format. Run 't81 help model import'.");
        return 1;
      }
      opts.format = args.command_args[idx++];
      opts.format_explicit = true;
    } else if (token == "--manifest") {
      if (idx >= args.command_args.size()) {
        error("model import: missing argument for --manifest. Run 't81 help model import'.");
        return 1;
      }
      opts.manifest_output = fs::path(args.command_args[idx++]);
    } else if (token == "--json") {
      opts.as_json = true;
    } else if (token == "--report") {
      opts.as_json = false;
    } else if (opts.input.empty()) {
      opts.input = fs::path(token);
    } else {
      error("model import: unexpected argument '" + token + "'. Run 't81 help model import'.");
      return 1;
    }
  }

  if (opts.input.empty()) {
    error("model import requires an input file. Run 't81 help model import'.");
    return 1;
  }
  if (!resolve_model_input_and_format(opts.input, opts.format, opts.format_explicit,
                                      "model import")) {
    return 1;
  }

  try {
    const auto mf =
        load_model_artifact(opts.input, opts.format, "model import", "help model import");
    std::ostringstream rendered;
    {
      ScopedStreamRedirect redirect(std::cout, rendered);
      if (opts.as_json) {
        emit_model_import_json(opts.input, opts.format, mf);
      } else {
        emit_model_import_report(opts.input, opts.format, mf);
      }
    }
    const std::string payload = rendered.str();
    if (opts.output && !write_text_file(*opts.output, payload, "model import")) {
      return 1;
    }
    if (opts.manifest_output) {
      std::ostringstream manifest_rendered;
      {
        ScopedStreamRedirect redirect(std::cout, manifest_rendered);
        emit_model_manifest_json(opts.input, opts.format, mf);
      }
      if (!write_text_file(*opts.manifest_output, manifest_rendered.str(), "model import")) {
        return 1;
      }
    }
    std::cout << payload;
  } catch (const std::exception& e) {
    if (opts.as_json) {
      std::ostringstream rendered;
      rendered << "{\n";
      rendered << "  \"schema\": \"t81.model-import.v1\",\n";
      rendered << "  \"ok\": false,\n";
      rendered << "  \"input\": \"" << json_escape(opts.input.string()) << "\",\n";
      rendered << "  \"error\": \"" << json_escape(e.what()) << "\"\n";
      rendered << "}\n";
      const std::string payload = rendered.str();
      if (opts.output && !write_text_file(*opts.output, payload, "model import")) {
        return 1;
      }
      std::cout << payload;
    } else {
      error(e.what());
    }
    return 1;
  }

  return 0;
}

int run_model_diff(const Args& args) {
  bool as_json = false;
  std::string comparison_mode = "raw";
  fs::path lhs_path;
  fs::path rhs_path;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const auto& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--mode") {
      if (i + 1 >= args.command_args.size()) {
        error("model diff: missing argument for --mode. Run 't81 help model diff'.");
        return 1;
      }
      comparison_mode = args.command_args[++i];
      if (comparison_mode != "raw" && comparison_mode != "normalized") {
        error("model diff: unsupported mode '" + comparison_mode + "'. Run 't81 help model diff'.");
        return 1;
      }
    } else if (token == "-h" || token == "--help") {
      print_help_model_diff();
      return 0;
    } else if (lhs_path.empty()) {
      lhs_path = fs::path(token);
    } else if (rhs_path.empty()) {
      rhs_path = fs::path(token);
    } else {
      error("model diff: unexpected argument '" + token + "'. Run 't81 help model diff'.");
      return 1;
    }
  }

  if (lhs_path.empty() || rhs_path.empty()) {
    error("model diff requires two input files. Run 't81 help model diff'.");
    return 1;
  }

  try {
    std::string lhs_format = "safetensors";
    std::string rhs_format = "safetensors";
    if (!has_extension_ci(lhs_path, ".json") &&
        !resolve_model_input_and_format(lhs_path, lhs_format, false, "model diff")) {
      return 1;
    }
    if (!has_extension_ci(rhs_path, ".json") &&
        !resolve_model_input_and_format(rhs_path, rhs_format, false, "model diff")) {
      return 1;
    }

    const auto lhs =
        load_model_artifact_or_manifest(lhs_path, lhs_format, "model diff", "help model diff");
    const auto rhs =
        load_model_artifact_or_manifest(rhs_path, rhs_format, "model diff", "help model diff");
    const auto provenance_diff = diff_provenance(lhs.model.provenance, rhs.model.provenance);
    if (comparison_mode == "normalized") {
      const auto tensor_diff = diff_model_tensors_normalized(lhs, rhs);
      if (as_json) {
        emit_model_diff_normalized_json(lhs_path, rhs_path, lhs, rhs, tensor_diff, provenance_diff);
      } else {
        std::cout << "Comparison Mode: normalized\n";
        emit_model_diff_report(lhs_path, rhs_path, lhs.model, rhs.model,
                               {tensor_diff.lhs_only, tensor_diff.rhs_only, tensor_diff.changed},
                               provenance_diff);
        if (!tensor_diff.normalized_matches.empty()) {
          std::cout << "Normalized Matches:\n";
          for (const auto& name : tensor_diff.normalized_matches) {
            const auto reason_it = tensor_diff.normalized_match_reasons.find(name);
            if (reason_it != tensor_diff.normalized_match_reasons.end()) {
              std::cout << "  - " << name << " (" << reason_it->second << ")\n";
            } else {
              std::cout << "  - " << name << "\n";
            }
          }
        }
        if (!tensor_diff.normalization_rules.empty()) {
          std::cout << "Normalization Rules:\n";
          for (const auto& rule : tensor_diff.normalization_rules) {
            std::cout << "  - " << rule << "\n";
          }
        }
      }

      const bool identical = lhs.model.total_parameters == rhs.model.total_parameters &&
                             lhs.model.total_trits == rhs.model.total_trits &&
                             lhs.model.tensors.size() == rhs.model.tensors.size() &&
                             tensor_diff.lhs_only.empty() && tensor_diff.rhs_only.empty() &&
                             tensor_diff.changed.empty();
      return identical ? 0 : 1;
    }

    const auto tensor_diff = diff_model_tensors(lhs.model, rhs.model);
    if (as_json) {
      emit_model_diff_json(lhs_path, rhs_path, lhs.model, rhs.model, tensor_diff, provenance_diff);
    } else {
      emit_model_diff_report(lhs_path, rhs_path, lhs.model, rhs.model, tensor_diff,
                             provenance_diff);
    }

    const bool identical = lhs.model.format == rhs.model.format &&
                           lhs.model.total_parameters == rhs.model.total_parameters &&
                           lhs.model.total_trits == rhs.model.total_trits &&
                           lhs.model.tensors.size() == rhs.model.tensors.size() &&
                           tensor_diff.lhs_only.empty() && tensor_diff.rhs_only.empty() &&
                           tensor_diff.changed.empty();
    return identical ? 0 : 1;
  } catch (const std::exception& e) {
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \""
                << (comparison_mode == "normalized" ? "t81.model-diff-normalized.v1"
                                                    : "t81.model-diff.v1")
                << "\",\n";
      std::cout << "  \"lhs\": \"" << json_escape(lhs_path.string()) << "\",\n";
      std::cout << "  \"rhs\": \"" << json_escape(rhs_path.string()) << "\",\n";
      std::cout << "  \"error\": \"" << json_escape(e.what()) << "\"\n";
      std::cout << "}\n";
    } else {
      error(e.what());
    }
    return 1;
  }
}

int run_weights_info(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights info requires a .t81w file path. Run 't81 help weights info'.");
    return 1;
  }
  fs::path path;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("weights info: unknown option '" + token + "'. Run 't81 help weights info'.");
      return 1;
    } else if (path.empty()) {
      path = fs::path(token);
    } else {
      error("weights info: unexpected argument '" + token + "'. Run 't81 help weights info'.");
      return 1;
    }
  }
  if (path.empty()) {
    error("weights info requires a .t81w file path. Run 't81 help weights info'.");
    return 1;
  }
  if (!has_extension_ci(path, ".t81w")) {
    error("weights info expects a .t81w file path. Run 't81 help weights info'.");
    return 1;
  }
  auto emit_json_error = [&](std::string_view message) {
    auto escape = [](std::string_view text) {
      std::string out;
      out.reserve(text.size() + 8);
      for (char c : text) {
        if (c == '\\' || c == '"') out.push_back('\\');
        if (c == '\n') {
          out += "\\n";
          continue;
        }
        if (c == '\r') {
          out += "\\r";
          continue;
        }
        if (c == '\t') {
          out += "\\t";
          continue;
        }
        out.push_back(c);
      }
      return out;
    };
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.weights-info.v1\",\n";
    std::cout << "  \"ok\": false,\n";
    std::cout << "  \"model\": \"" << escape(path.string()) << "\",\n";
    std::cout << "  \"error\": \"" << escape(message) << "\"\n";
    std::cout << "}\n";
  };
  try {
    auto mf = t81::weights::load_t81w(path);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.weights-info.v1\",\n";
      std::cout << "  \"ok\": true,\n";
      std::cout << "  \"model\": \"" << path.string() << "\",\n";
      std::cout << "  \"parameters\": " << mf.total_parameters << ",\n";
      std::cout << "  \"trits\": " << mf.total_trits << ",\n";
      std::cout << "  \"file_size_bytes\": " << mf.file_size << ",\n";
      std::cout << "  \"bits_per_trit\": " << std::fixed << std::setprecision(6) << mf.bits_per_trit
                << ",\n";
      std::cout << "  \"sparsity\": " << std::fixed << std::setprecision(6) << mf.sparsity << ",\n";
      std::cout << "  \"format\": \"" << mf.format << "\",\n";
      std::cout << "  \"checksum_sha3_512\": \"" << mf.checksum << "\"\n";
      std::cout << "}\n";
      std::cout << std::defaultfloat;
    } else {
      std::cout << "Model:        " << path << "\n";
      std::cout << "Parameters:   " << t81::weights::format_count(mf.total_parameters) << "\n";
      std::cout << "Trits:        " << t81::weights::format_count(mf.total_trits) << " trits\n";
      std::cout << "Storage:      " << t81::weights::format_bytes(mf.file_size) << " ("
                << std::fixed << std::setprecision(3) << mf.bits_per_trit << " bits/trit avg)\n";
      std::cout << std::fixed << std::setprecision(1) << "Sparsity:     " << (mf.sparsity * 100.0)
                << "% zeros\n";
      std::cout << std::defaultfloat;
      std::cout << "Format:       " << mf.format << "\n";
      std::cout << "Checksum:     sha3-512:" << mf.checksum << " (CanonFS-ready)\n";
    }
  } catch (const std::exception& e) {
    if (as_json) {
      emit_json_error(e.what());
      return 1;
    }
    error(e.what());
    return 1;
  }
  return 0;
}

int run_weights_verify(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights verify requires a .t81w file path. Run 't81 help weights verify'.");
    return 1;
  }
  fs::path path;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("weights verify: unknown option '" + token + "'. Run 't81 help weights verify'.");
      return 1;
    } else if (path.empty()) {
      path = fs::path(token);
    } else {
      error("weights verify: unexpected argument '" + token + "'. Run 't81 help weights verify'.");
      return 1;
    }
  }
  if (path.empty()) {
    error("weights verify requires a .t81w file path. Run 't81 help weights verify'.");
    return 1;
  }
  if (!has_extension_ci(path, ".t81w")) {
    error("weights verify expects a .t81w file path. Run 't81 help weights verify'.");
    return 1;
  }

  try {
    auto mf = t81::weights::load_t81w(path);
    const bool ok = !mf.native.empty() && !mf.checksum.empty() && mf.file_size > 0;
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.weights-verify.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"model\": \"" << json_escape(path.string()) << "\",\n"
                << "  \"tensors\": " << mf.native.size() << ",\n"
                << "  \"parameters\": " << mf.total_parameters << ",\n"
                << "  \"file_size_bytes\": " << mf.file_size << ",\n"
                << "  \"checksum_sha3_512\": \"" << json_escape(mf.checksum) << "\"\n"
                << "}\n";
    } else {
      std::cout << "Verified model: " << path.string() << "\n";
      std::cout << "Tensors:        " << mf.native.size() << "\n";
      std::cout << "Parameters:     " << t81::weights::format_count(mf.total_parameters) << "\n";
      std::cout << "File size:      " << t81::weights::format_bytes(mf.file_size) << "\n";
      std::cout << "Checksum:       sha3-512:" << mf.checksum << "\n";
    }
    return ok ? 0 : 1;
  } catch (const std::exception& e) {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.weights-verify.v1\",\n"
                << "  \"ok\": false,\n"
                << "  \"model\": \"" << json_escape(path.string()) << "\",\n"
                << "  \"error\": \"" << json_escape(e.what()) << "\"\n"
                << "}\n";
    } else {
      error(e.what());
    }
    return 1;
  }
}

int run_weights_quantize(const Args& args) {
  if (args.command_args.size() != 4 || args.command_args[2] != "--to-gguf") {
    error("weights quantize requires: quantize <input> --to-gguf <output>. Run 't81 help weights "
          "quantize'.");
    return 1;
  }
  fs::path input = args.command_args[1];
  fs::path output = args.command_args[3];
  if (!has_extension_ci(input, ".safetensors")) {
    error("weights quantize expects a .safetensors input. Run 't81 help weights quantize'.");
    return 1;
  }
  if (!has_extension_ci(output, ".gguf")) {
    error("weights quantize expects a .gguf output path. Run 't81 help weights quantize'.");
    return 1;
  }
  try {
    t81::weights::quantize_safetensors_to_gguf(input, output);
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }
  return 0;
}

int run_weights_export(const Args& args) {
  if (args.command_args.size() < 4 || args.command_args[2] != "--to-safetensors") {
    error("weights export requires: export <model.t81w> --to-safetensors <output>. Run 't81 help "
          "weights export'.");
    return 1;
  }
  const fs::path input = args.command_args[1];
  const fs::path output = args.command_args[3];
  bool as_json = false;
  for (std::size_t i = 4; i < args.command_args.size(); ++i) {
    if (args.command_args[i] == "--json") {
      as_json = true;
    } else {
      error("weights export: unexpected argument '" + args.command_args[i] +
            "'. Run 't81 help weights export'.");
      return 1;
    }
  }
  if (!has_extension_ci(input, ".t81w")) {
    error("weights export expects a .t81w input file.");
    return 1;
  }
  if (!has_extension_ci(output, ".safetensors")) {
    error("weights export expects a .safetensors output file.");
    return 1;
  }

  t81::weights::ModelFile mf;
  try {
    mf = t81::weights::load_t81w(input);
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }

  struct ExportTensor {
    std::string name;
    std::vector<uint64_t> shape;
    uint64_t trits = 0;
    std::vector<int8_t> raw;
  };
  std::vector<ExportTensor> tensors;
  tensors.reserve(mf.native.size());
  for (const auto& [name, tensor] : mf.native) {
    tensors.push_back(
        ExportTensor{name, tensor.shape, tensor.num_trits(), unpack_native_trits(tensor)});
  }

  std::string header_text;
  std::uint64_t header_len = 0;
  for (int iter = 0; iter < 4; ++iter) {
    std::ostringstream header;
    header << "{";
    std::uint64_t offset = sizeof(std::uint64_t) + header_len;
    for (std::size_t index = 0; index < tensors.size(); ++index) {
      const auto& tensor = tensors[index];
      header << "\"" << json_escape(tensor.name) << "\":{" << "\"dtype\":\"I8\"," << "\"shape\":[";
      const uint64_t shape_product = std::accumulate(tensor.shape.begin(), tensor.shape.end(),
                                                     uint64_t{1}, std::multiplies<uint64_t>());
      if (!tensor.shape.empty() && shape_product == tensor.trits) {
        for (std::size_t i = 0; i < tensor.shape.size(); ++i) {
          header << tensor.shape[i];
          if (i + 1 < tensor.shape.size()) {
            header << ",";
          }
        }
      } else {
        header << tensor.trits;
      }
      header << "],\"data_offsets\":[" << offset << "],\"data_lengths\":[" << tensor.raw.size()
             << "]}";
      offset += tensor.raw.size();
      if (index + 1 < tensors.size()) {
        header << ",";
      }
    }
    header << "}";
    header_text = header.str();
    if (header_text.size() == header_len) {
      break;
    }
    header_len = static_cast<std::uint64_t>(header_text.size());
  }
  header_len = static_cast<std::uint64_t>(header_text.size());

  std::ofstream out(output, std::ios::binary | std::ios::trunc);
  if (!out) {
    error("weights export: could not open output file: " + output.string());
    return 1;
  }
  out.write(reinterpret_cast<const char*>(&header_len), sizeof(header_len));
  out.write(header_text.data(), static_cast<std::streamsize>(header_len));
  for (const auto& tensor : tensors) {
    out.write(reinterpret_cast<const char*>(tensor.raw.data()),
              static_cast<std::streamsize>(tensor.raw.size()));
  }
  if (!out.good()) {
    error("weights export: failed writing output file: " + output.string());
    return 1;
  }

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.weights-export.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
              << "  \"output\": \"" << json_escape(output.string()) << "\",\n"
              << "  \"tensors\": " << mf.native.size() << "\n"
              << "}\n";
    return 0;
  }

  std::cout << "Exported " << mf.native.size() << " tensor(s) to " << output.string() << "\n";
  return 0;
}

int run_weights(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_weights(); });
  }
  if (args.command_args.size() >= 2 &&
      (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
    const std::string sub = args.command_args[0];
    if (sub == "import") {
      print_help_weights_import();
      return 0;
    }
    if (sub == "info") {
      print_help_weights_info();
      return 0;
    }
    if (sub == "verify") {
      print_help_weights_verify();
      return 0;
    }
    if (sub == "quantize") {
      print_help_weights_quantize();
      return 0;
    }
    if (sub == "export") {
      return emit_help([&] {
        std::cerr << "Usage: t81 weights export <model.t81w> --to-safetensors <out> [--json]\n";
      });
    }
    error("weights: unknown subcommand '" + sub + "'. Run 't81 help weights'.");
    return 1;
  }
  const std::string sub = args.command_args[0];
  if (sub == "import") {
    return run_weights_import(args);
  } else if (sub == "info") {
    return run_weights_info(args);
  } else if (sub == "verify") {
    return run_weights_verify(args);
  } else if (sub == "quantize") {
    return run_weights_quantize(args);
  } else if (sub == "export") {
    return run_weights_export(args);
  }
  error("weights: unknown subcommand '" + sub + "'. Run 't81 help weights'.");
  return 1;
}

int run_model(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_model(); });
  }
  if (args.command_args.size() >= 2 &&
      (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
    const std::string sub = args.command_args[0];
    if (sub == "import") {
      print_help_model_import();
      return 0;
    }
    if (sub == "diff") {
      print_help_model_diff();
      return 0;
    }
    error("model: unknown subcommand '" + sub + "'. Run 't81 help model'.");
    return 1;
  }

  const std::string sub = args.command_args[0];
  if (sub == "import") {
    return run_model_import(args);
  }
  if (sub == "diff") {
    return run_model_diff(args);
  }

  error("model: unknown subcommand '" + sub + "'. Run 't81 help model'.");
  return 1;
}
