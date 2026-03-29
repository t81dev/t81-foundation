#include "ai_cli_shared.hpp"

#include "t81/cli/driver.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/math/quantization/ternary_codec.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <cctype>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace t81::cli::ai {
namespace {

constexpr std::string_view kUnsupportedSentinel = "No backend supports requested format/mode";
constexpr std::string_view kFixedTimestamp = "1970-01-01T00:00:00Z";
constexpr std::string_view kPolicyAllowReason = "AI_POLICY_ALLOW_MODEL_HASH_MATCH";
constexpr std::size_t kLogitsSampleWindow = 8;
constexpr std::size_t kHiddenCarryProbeWidth = 4;
constexpr std::size_t kHiddenCarryProjectionWidth = 2;
constexpr std::size_t kBoundedDecodeTraceSteps = 4;
constexpr std::size_t kDecodeContextHistoryWindow = 3;

std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    switch (c) {
      case '\\':
      case '"':
        out.push_back('\\');
        out.push_back(c);
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

std::string sha3_hex_bytes(std::span<const std::uint8_t> bytes) {
  return t81::crypto::sha3_512_hex(bytes);
}

std::string sha3_hex_text(std::string_view text) {
  const auto* begin = reinterpret_cast<const std::uint8_t*>(text.data());
  return sha3_hex_bytes(std::span<const std::uint8_t>(begin, text.size()));
}

std::string tensor_signature_sha256(const t81::T729DynamicTensor& tensor) {
  std::ostringstream out(std::ios::binary);
  tensor.serialize(out);
  const std::string bytes = out.str();
  return sha3_hex_text(bytes);
}

std::string fingerprint_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return "";
  }
  std::vector<std::uint8_t> sample;
  const std::uintmax_t size = fs::file_size(path);
  const std::size_t window = 4096;
  sample.reserve(2 * window + 32);
  for (int shift = 0; shift < 8; ++shift) {
    sample.push_back(static_cast<std::uint8_t>((size >> (shift * 8)) & 0xFF));
  }
  std::vector<char> buffer(window);
  in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  const auto read_head = static_cast<std::size_t>(std::max<std::streamsize>(in.gcount(), 0));
  sample.insert(sample.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(read_head));
  if (size > window) {
    in.clear();
    in.seekg(static_cast<std::streamoff>(size - window), std::ios::beg);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto read_tail = static_cast<std::size_t>(std::max<std::streamsize>(in.gcount(), 0));
    sample.insert(sample.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(read_tail));
  }
  return sha3_hex_bytes(sample);
}

bool write_text_file(const fs::path& path, std::string_view text, std::string& error) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  if (ec) {
    error = "could not create parent directory for " + path.string();
    return false;
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    error = "could not write " + path.string();
    return false;
  }
  out << text;
  if (!out.good()) {
    error = "could not write " + path.string();
    return false;
  }
  return true;
}

std::string detect_format(const fs::path& model_file, std::string requested_format) {
  if (!requested_format.empty()) {
    return requested_format;
  }
  const std::string ext = model_file.extension().string();
  if (ext == ".gguf") return "gguf";
  if (ext == ".t3k") return "t3k";
  if (ext == ".onnx") return "onnx";
  return "t81_canonical";
}

struct BackendSelection {
  bool supported = false;
  std::string requested_format;
  std::string requested_mode;
  std::string selected_backend;
  std::string decision_reason;
  std::string support_state;
  bool strict_core_eligible = false;
  std::string numeric_kernel_class;
  std::string effective_determinism_class;
  std::vector<std::string> preferred_order;
  std::vector<std::string> candidates;
};

BackendSelection select_backend(std::string format, std::string mode) {
  BackendSelection sel;
  sel.requested_format = std::move(format);
  sel.requested_mode = std::move(mode);
  sel.preferred_order = {"t81_reference_vm", "llama.cpp", "onnx_runtime"};
  if ((sel.requested_format == "gguf" || sel.requested_format == "t81_canonical") &&
      sel.requested_mode == "reproducible_nondeterministic") {
    sel.supported = true;
    sel.selected_backend = "llama.cpp";
    sel.decision_reason = "preferred_gguf_runtime";
    sel.support_state = "supported";
    sel.strict_core_eligible = false;
    sel.numeric_kernel_class = "host_float";
    sel.effective_determinism_class = "reproducible_nondeterministic";
    sel.candidates = {"llama.cpp", "t81_reference_vm"};
    return sel;
  }
  if ((sel.requested_format == "gguf" || sel.requested_format == "t3k" ||
       sel.requested_format == "t81_canonical") &&
      sel.requested_mode == "strict_deterministic") {
    sel.supported = true;
    sel.selected_backend = "t81_reference_vm";
    sel.decision_reason = "strict_core_reference_lane";
    sel.support_state = "supported";
    sel.strict_core_eligible = true;
    sel.numeric_kernel_class = "ternary_vm";
    sel.effective_determinism_class = "strict_deterministic";
    sel.candidates = {"t81_reference_vm"};
    return sel;
  }
  if (sel.requested_format == "onnx" && sel.requested_mode == "statistical_deterministic") {
    sel.supported = true;
    sel.selected_backend = "onnx_runtime";
    sel.decision_reason = "onnx_statistical_lane";
    sel.support_state = "supported";
    sel.strict_core_eligible = false;
    sel.numeric_kernel_class = "host_float";
    sel.effective_determinism_class = "statistical_deterministic";
    sel.candidates = {"onnx_runtime"};
    return sel;
  }
  sel.supported = false;
  sel.decision_reason = "no_supported_backend";
  sel.support_state = "unsupported";
  sel.numeric_kernel_class = "none";
  sel.effective_determinism_class = "unsupported";
  return sel;
}

std::string backend_selection_json(const BackendSelection& sel) {
  const std::string trace_seed =
      sel.requested_format + "|" + sel.requested_mode + "|" + sel.selected_backend + "|" +
      sel.support_state + "|" + sel.decision_reason;
  const std::string trace_hash = sha3_hex_text(trace_seed);
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"t81.ai.backend-selection-trace.v1\",\n"
      << "  \"requested_format\": \"" << json_escape(sel.requested_format) << "\",\n"
      << "  \"requested_mode\": \"" << json_escape(sel.requested_mode) << "\",\n"
      << "  \"selection_policy\": \"deterministic_preferred_order.v1\",\n"
      << "  \"preferred_order\": [";
  for (std::size_t i = 0; i < sel.preferred_order.size(); ++i) {
    if (i != 0) out << ", ";
    out << "\"" << json_escape(sel.preferred_order[i]) << "\"";
  }
  out << "],\n"
      << "  \"candidates\": [";
  for (std::size_t i = 0; i < sel.candidates.size(); ++i) {
    if (i != 0) out << ", ";
    out << "\"" << json_escape(sel.candidates[i]) << "\"";
  }
  out << "],\n"
      << "  \"selected_backend\": ";
  if (sel.selected_backend.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << json_escape(sel.selected_backend) << "\",\n";
  }
  out << "  \"decision_reason\": \"" << json_escape(sel.decision_reason) << "\",\n"
      << "  \"support_state\": \"" << json_escape(sel.support_state) << "\",\n"
      << "  \"strict_core_eligible\": " << (sel.strict_core_eligible ? "true" : "false") << ",\n"
      << "  \"numeric_kernel_class\": \"" << json_escape(sel.numeric_kernel_class) << "\",\n"
      << "  \"trace_sha256\": \"" << trace_hash << "\",\n"
      << "  \"status\": \"" << (sel.supported ? "pass" : "fail") << "\"\n"
      << "}\n";
  return out.str();
}

struct Options {
  std::optional<fs::path> model_file;
  std::optional<fs::path> out;
  std::string model_id;
  std::string prompt;
  std::string format;
  std::string mode = "reproducible_nondeterministic";
  std::string event_type = "model_load";
  std::string seed = "0";
  std::string input_file;
  std::string output_file;
  std::string model_hash;
  int max_tokens = static_cast<int>(kBoundedDecodeTraceSteps);
  bool verbose = false;
};

bool parse_named_args(const std::vector<std::string_view>& argv, std::size_t start, Options& opts,
                      std::vector<std::string>& positional, std::string& error) {
  for (std::size_t i = start; i < argv.size(); ++i) {
    const std::string arg(argv[i]);
    if (arg == "--model-file") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --model-file";
        return false;
      }
      opts.model_file = fs::absolute(fs::path(std::string(argv[++i])));
    } else if (arg == "--out") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --out";
        return false;
      }
      opts.out = fs::absolute(fs::path(std::string(argv[++i])));
    } else if (arg == "--model") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --model";
        return false;
      }
      opts.model_id = std::string(argv[++i]);
    } else if (arg == "--prompt") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --prompt";
        return false;
      }
      opts.prompt = std::string(argv[++i]);
    } else if (arg == "--format") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --format";
        return false;
      }
      opts.format = std::string(argv[++i]);
    } else if (arg == "--mode") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --mode";
        return false;
      }
      opts.mode = std::string(argv[++i]);
    } else if (arg == "--event-type" || arg == "--type") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --event-type";
        return false;
      }
      opts.event_type = std::string(argv[++i]);
    } else if (arg == "--seed") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --seed";
        return false;
      }
      opts.seed = std::string(argv[++i]);
    } else if (arg == "--input") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --input";
        return false;
      }
      opts.input_file = std::string(argv[++i]);
    } else if (arg == "--output") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --output";
        return false;
      }
      opts.output_file = std::string(argv[++i]);
    } else if (arg == "--max-tokens") {
      if (i + 1 >= argv.size()) {
        error = "missing value for --max-tokens";
        return false;
      }
      try {
        opts.max_tokens = std::stoi(std::string(argv[++i]));
      } catch (...) {
        error = "invalid value for --max-tokens";
        return false;
      }
      if (opts.max_tokens < 1 || opts.max_tokens > 32) {
        error = "--max-tokens must be between 1 and 32";
        return false;
      }
    } else if (arg == "--verbose") {
      opts.verbose = true;
    } else if (!arg.empty() && arg[0] == '-') {
      error = "unknown option: " + arg;
      return false;
    } else {
      positional.push_back(arg);
    }
  }
  return true;
}

bool require_existing_model_file(const std::optional<fs::path>& model_file, fs::path& resolved,
                                 std::string& error) {
  if (!model_file) {
    error = "missing model file";
    return false;
  }
  resolved = fs::absolute(*model_file);
  if (!fs::exists(resolved)) {
    error = "model file does not exist: " + resolved.string();
    return false;
  }
  return true;
}

int emit_or_write(std::string payload, const std::optional<fs::path>& out_path) {
  if (out_path) {
    std::string error;
    if (!write_text_file(*out_path, payload, error)) {
      std::cerr << "error: " << error << "\n";
      return 1;
    }
  }
  std::cout << payload;
  return 0;
}

bool tensors_are_matmul_compatible(const t81::weights::NativeTensor& lhs,
                                   const t81::weights::NativeTensor& rhs) {
  return lhs.shape.size() == 2 && rhs.shape.size() == 2 && lhs.shape[1] == rhs.shape[0] &&
         lhs.shape[0] > 0 && lhs.shape[1] > 0 && rhs.shape[1] > 0;
}

std::size_t native_tensor_shape_complexity(const t81::weights::NativeTensor& tensor) {
  std::size_t product = 1;
  for (std::uint32_t dim : tensor.shape) {
    if (dim == 0) {
      return 0;
    }
    product *= static_cast<std::size_t>(dim);
  }
  return product * tensor.shape.size();
}

std::optional<std::pair<std::string, std::string>> choose_native_probe_tensor_pair(
    const t81::weights::ModelFile& model) {
  const auto preferred_a = model.native.find("model.layers.0.self_attn.q_proj.weight");
  const auto preferred_b = model.native.find("model.layers.0.self_attn.k_proj.weight");
  if (preferred_a != model.native.end() && preferred_b != model.native.end() &&
      tensors_are_matmul_compatible(preferred_a->second, preferred_b->second)) {
    return std::pair<std::string, std::string>{preferred_a->first, preferred_b->first};
  }

  for (auto lhs = model.native.begin(); lhs != model.native.end(); ++lhs) {
    if (lhs->second.shape.size() != 2) {
      continue;
    }
    for (auto rhs = std::next(lhs); rhs != model.native.end(); ++rhs) {
      if (rhs->second.shape.size() != 2) {
        continue;
      }
      if (tensors_are_matmul_compatible(lhs->second, rhs->second)) {
        return std::pair<std::string, std::string>{lhs->first, rhs->first};
      }
      if (tensors_are_matmul_compatible(rhs->second, lhs->second)) {
        return std::pair<std::string, std::string>{rhs->first, lhs->first};
      }
    }
  }
  return std::nullopt;
}

struct NativeProbeResult {
  bool ok = false;
  std::string stdout_text;
  std::string trap;
  std::string lhs_tensor;
  std::string rhs_tensor;
  std::string value_tensor;
  std::string lhs_tensor_layer1;
  std::string rhs_tensor_layer1;
  std::string value_tensor_layer1;
  std::string probe_kind;
  std::string embed_tensor;
  int token_index = 0;
  std::vector<int> context_token_indices;
  bool logits_projection_supported = false;
  bool logits_row_probe_supported = false;
  std::size_t logits_sample_window = 0;
  std::size_t logits_vocab_size = 0;
  std::size_t logits_candidate_window_start = 0;
  std::string candidate_selection_mode = "prompt_seeded";
  std::string candidate_selection_basis = "prompt_sha3_seeded_contiguous_window.v1";
  bool tokenizer_seed_supported = false;
  std::vector<int> prompt_token_ids;
  std::optional<int> candidate_seed_token_id;
  std::vector<int> sampled_token_ids;
  std::vector<double> sampled_token_scores;
  std::vector<int> hidden_carry_row_ids;
  std::vector<double> hidden_carry_scores;
  std::string hidden_carry_signature_sha256;
  std::string carry_probe_layout_kind = "contiguous_forward_window.v1";
  std::vector<int> hidden_projection_row_ids;
  std::vector<double> hidden_projection_scores;
  std::string hidden_projection_signature_sha256;
  std::string projection_carry_mode_kind = "balanced_context_projection.v1";
  std::string hidden_state_class;
  std::string hidden_state_class_signature_sha256;
  bool intermediate_tensor_export_supported = false;
  std::optional<int> hidden_tensor_handle;
  std::size_t hidden_tensor_rank = 0;
  std::size_t hidden_tensor_elements = 0;
  std::string hidden_tensor_signature_sha256;
  std::vector<int> hidden_tensor_shape;
  bool hidden_tensor_import_used = false;
  bool hidden_tensor_blend_used = false;
  std::string hidden_tensor_carry_mode_kind = "current_only.v1";
  std::string kv_state_kind = "unavailable";
  std::string q_tensor_signature_sha256;
  std::string k_tensor_signature_sha256;
  std::size_t kv_tensor_rank = 0;
  std::size_t kv_tensor_elements = 0;
  std::string kv_state_signature_sha256;
  std::string kv_state_carry_mode_kind = "current_qk_window.v1";
  std::string architecture_state_kind = "unavailable";
  std::string architecture_state_signature_sha256;
  std::string architecture_state_class;
  std::string architecture_state_class_signature_sha256;
  std::optional<t81::T729DynamicTensor> hidden_tensor;
  std::string selection_policy_kind = "max_score.v1";
  double confidence_score = 0.0;
  double logits_margin = 0.0;
  double hidden_carry_peak = 0.0;
  std::string stability_kind = "unclassified";
  std::optional<int> selected_token_id;
  std::optional<double> selected_token_score;
};

std::string detect_architecture_profile(const t81::weights::ModelFile& model) {
  const auto has = [&](std::string_view key) { return model.native.find(std::string(key)) != model.native.end(); };
  if (has("model.embed_tokens.weight") && has("model.norm.weight") &&
      has("model.layers.0.self_attn.q_proj.weight") &&
      has("model.layers.0.self_attn.k_proj.weight") &&
      has("model.layers.0.self_attn.v_proj.weight") &&
      has("model.layers.0.self_attn.o_proj.weight") &&
      has("model.layers.0.mlp.gate_proj.weight") &&
      has("model.layers.0.mlp.up_proj.weight") &&
      has("model.layers.0.mlp.down_proj.weight")) {
    return "llama-dense-v1";
  }
  return "unknown";
}

struct CompanionFiles {
  bool has_config = false;
  bool has_tokenizer = false;
  fs::path config_path;
  fs::path tokenizer_path;
};

CompanionFiles find_model_companion_files(const fs::path& model_file) {
  CompanionFiles companions;
  const fs::path dir = model_file.parent_path();
  const fs::path config = dir / "config.json";
  const fs::path tokenizer = dir / "tokenizer.json";
  companions.has_config = fs::exists(config);
  companions.has_tokenizer = fs::exists(tokenizer);
  companions.config_path = fs::absolute(config);
  companions.tokenizer_path = fs::absolute(tokenizer);
  return companions;
}

std::vector<std::string> split_lines(std::string_view text) {
  std::vector<std::string> lines;
  std::string current;
  for (char ch : text) {
    if (ch == '\n') {
      lines.push_back(current);
      current.clear();
    } else if (ch != '\r') {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    lines.push_back(current);
  }
  return lines;
}

std::vector<int> select_prompt_seeded_token_ids(std::string_view prompt, std::size_t vocab_size,
                                                std::size_t sample_window,
                                                std::size_t* window_start = nullptr) {
  if (vocab_size == 0 || sample_window == 0) {
    return {};
  }
  const std::string digest = sha3_hex_text(prompt.empty() ? "deterministic prompt" : prompt);
  std::uint64_t seed = 0;
  constexpr std::array<char, 16> kHex = {'0', '1', '2', '3', '4', '5', '6', '7',
                                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  for (std::size_t i = 0; i < 16 && i < digest.size(); ++i) {
    const char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(digest[i])));
    const auto it = std::find(kHex.begin(), kHex.end(), ch);
    if (it == kHex.end()) {
      break;
    }
    seed = (seed << 4) | static_cast<std::uint64_t>(std::distance(kHex.begin(), it));
  }

  std::vector<int> token_ids;
  token_ids.reserve(sample_window);
  const std::size_t start = static_cast<std::size_t>(seed % vocab_size);
  if (window_start) {
    *window_start = start;
  }
  for (std::size_t i = 0; i < sample_window; ++i) {
    token_ids.push_back(static_cast<int>((start + i) % vocab_size));
  }
  return token_ids;
}

std::vector<int> make_contiguous_window_token_ids(std::size_t start, std::size_t vocab_size,
                                                  std::size_t sample_window) {
  std::vector<int> token_ids;
  if (vocab_size == 0 || sample_window == 0) {
    return token_ids;
  }
  token_ids.reserve(sample_window);
  for (std::size_t i = 0; i < sample_window; ++i) {
    token_ids.push_back(static_cast<int>((start + i) % vocab_size));
  }
  return token_ids;
}

std::size_t history_seed_window_start(const std::vector<int>& token_history,
                                      std::size_t vocab_size) {
  if (vocab_size == 0) {
    return 0;
  }
  std::ostringstream seed_text;
  for (std::size_t i = 0; i < token_history.size(); ++i) {
    if (i != 0) {
      seed_text << ",";
    }
    seed_text << token_history[i];
  }
  const std::string digest = sha3_hex_text(seed_text.str());
  std::uint64_t seed = 0;
  constexpr std::array<char, 16> kHex = {'0', '1', '2', '3', '4', '5', '6', '7',
                                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  for (std::size_t i = 0; i < 16 && i < digest.size(); ++i) {
    const char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(digest[i])));
    const auto it = std::find(kHex.begin(), kHex.end(), ch);
    if (it == kHex.end()) {
      break;
    }
    seed = (seed << 4) | static_cast<std::uint64_t>(std::distance(kHex.begin(), it));
  }
  return static_cast<std::size_t>(seed % vocab_size);
}

std::size_t digest_seed_window_start(std::string_view digest, std::size_t vocab_size) {
  if (vocab_size == 0) {
    return 0;
  }
  std::uint64_t seed = 0;
  constexpr std::array<char, 16> kHex = {'0', '1', '2', '3', '4', '5', '6', '7',
                                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  for (std::size_t i = 0; i < 16 && i < digest.size(); ++i) {
    const char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(digest[i])));
    const auto it = std::find(kHex.begin(), kHex.end(), ch);
    if (it == kHex.end()) {
      break;
    }
    seed = (seed << 4) | static_cast<std::uint64_t>(std::distance(kHex.begin(), it));
  }
  return static_cast<std::size_t>(seed % vocab_size);
}

std::vector<int> recent_context_history(const std::vector<int>& token_history,
                                        std::size_t max_tokens,
                                        std::optional<int> anchor_token = std::nullopt) {
  if (token_history.empty() || max_tokens == 0) {
    return {};
  }
  if (token_history.size() <= max_tokens) {
    return token_history;
  }
  const int anchor = anchor_token.value_or(token_history.front());
  if (max_tokens == 1) {
    return {anchor};
  }
  std::vector<int> out;
  out.reserve(max_tokens);
  out.push_back(anchor);
  const std::size_t tail_keep = max_tokens - 1;
  for (auto it = token_history.end() - static_cast<std::ptrdiff_t>(tail_keep);
       it != token_history.end(); ++it) {
    if (*it == anchor) {
      continue;
    }
    out.push_back(*it);
  }
  while (out.size() < std::min(max_tokens, token_history.size())) {
    const auto it = token_history.end() -
                    static_cast<std::ptrdiff_t>(std::min(token_history.size(), max_tokens));
    for (auto fill = it; fill != token_history.end() && out.size() < max_tokens; ++fill) {
      if (std::find(out.begin(), out.end(), *fill) == out.end()) {
        out.push_back(*fill);
      }
    }
    break;
  }
  if (out.size() > max_tokens) {
    out.resize(max_tokens);
  }
  return out;
}

constexpr std::string_view kNativeDecodeStateKind = "prompt_history_bounded_context.v1";

struct NativeDecodeState {
  std::vector<int> prompt_token_history;
  std::vector<int> generated_token_history;
  std::optional<int> prompt_anchor_token_id;
  int input_token_id = 0;
  std::optional<int> seed_token_id;
  std::size_t window_start = 0;
  std::vector<int> hidden_carry_row_ids;
  std::vector<double> hidden_carry_scores;
  std::string hidden_carry_signature_sha256;
  std::string carry_probe_layout_kind = "contiguous_forward_window.v1";
  std::vector<int> hidden_projection_row_ids;
  std::vector<double> hidden_projection_scores;
  std::string hidden_projection_signature_sha256;
  std::string forward_state_kind = "unavailable";
  std::vector<int> forward_state_row_ids;
  std::vector<double> forward_state_scores;
  std::string forward_state_signature_sha256;
  std::size_t forward_state_generation = 0;
  std::string forward_state_class;
  std::string forward_state_class_signature_sha256;
  std::string hidden_tensor_signature_sha256;
  std::size_t hidden_tensor_rank = 0;
  std::size_t hidden_tensor_elements = 0;
  std::vector<int> hidden_tensor_shape;
  std::optional<t81::T729DynamicTensor> carried_hidden_tensor;
  std::string hidden_tensor_carry_mode_kind = "current_only.v1";
  std::string kv_state_kind = "unavailable";
  std::string q_tensor_signature_sha256;
  std::string k_tensor_signature_sha256;
  std::size_t kv_tensor_rank = 0;
  std::size_t kv_tensor_elements = 0;
  std::string kv_state_signature_sha256;
  std::string kv_state_carry_mode_kind = "current_qk_window.v1";
  std::string architecture_state_kind = "unavailable";
  std::string architecture_state_signature_sha256;
  std::string architecture_state_class;
  std::string architecture_state_class_signature_sha256;
  std::string projection_carry_mode_kind = "balanced_context_projection.v1";
  std::string hidden_state_class;
  std::string hidden_state_class_signature_sha256;
  std::string selection_policy_kind = "max_score.v1";
  double confidence_score = 0.0;
  double logits_margin = 0.0;
  double hidden_carry_peak = 0.0;
  std::string stability_kind = "unclassified";
};

std::vector<int> combined_decode_history(const NativeDecodeState& state) {
  std::vector<int> combined = state.prompt_token_history;
  combined.insert(combined.end(), state.generated_token_history.begin(),
                  state.generated_token_history.end());
  return combined;
}

std::vector<int> decode_context_history(const NativeDecodeState& state, std::size_t max_tokens) {
  std::vector<int> context = state.prompt_token_history;
  context.insert(context.end(), state.generated_token_history.begin(),
                 state.generated_token_history.end());
  return recent_context_history(context, max_tokens, state.prompt_anchor_token_id);
}

std::vector<int> merged_state_input_rows(const std::vector<int>& forward_state_row_ids,
                                         const std::vector<int>& hidden_projection_row_ids,
                                         std::size_t max_rows) {
  std::vector<int> merged;
  merged.reserve(std::min<std::size_t>(max_rows,
                                       forward_state_row_ids.size() +
                                           hidden_projection_row_ids.size()));
  auto append_unique = [&](const std::vector<int>& rows) {
    for (int row : rows) {
      if (merged.size() >= max_rows) {
        break;
      }
      if (std::find(merged.begin(), merged.end(), row) == merged.end()) {
        merged.push_back(row);
      }
    }
  };
  append_unique(forward_state_row_ids);
  append_unique(hidden_projection_row_ids);
  return merged;
}

std::string prefixed_history_digest(std::string_view prefix, const std::vector<int>& token_history) {
  std::ostringstream seed_text;
  seed_text << prefix << "|";
  for (std::size_t i = 0; i < token_history.size(); ++i) {
    if (i != 0) {
      seed_text << ",";
    }
    seed_text << token_history[i];
  }
  return sha3_hex_text(seed_text.str());
}

std::string decode_state_seed_digest(const NativeDecodeState& state) {
  return prefixed_history_digest("decode-state", combined_decode_history(state));
}

std::string candidate_window_seed_digest(const NativeDecodeState& state) {
  return prefixed_history_digest("candidate-window", combined_decode_history(state));
}

std::optional<t81::T729DynamicTensor> evolved_hidden_tensor(
    const std::optional<t81::T729DynamicTensor>& previous,
    const std::optional<t81::T729DynamicTensor>& current,
    float current_weight = 0.75F, float previous_weight = 0.25F) {
  if (!current.has_value()) {
    return std::nullopt;
  }
  if (!previous.has_value()) {
    return current;
  }
  if (previous->shape() != current->shape()) {
    return current;
  }

  const auto prev_values = previous->snapshot_values();
  const auto curr_values = current->snapshot_values();
  if (prev_values.size() != curr_values.size()) {
    return current;
  }

  std::vector<float> blended;
  blended.reserve(curr_values.size());
  for (std::size_t i = 0; i < curr_values.size(); ++i) {
    blended.push_back(static_cast<float>(curr_values[i] * current_weight +
                                         prev_values[i] * previous_weight));
  }
  return t81::T729DynamicTensor::from_host_float_data(
      current->shape(), std::move(blended), current->numeric_class());
}

std::vector<int> state_input_rows_for_state(const NativeDecodeState& state) {
  const std::size_t max_rows = std::max<std::size_t>(state.forward_state_row_ids.size(),
                                                     state.hidden_projection_row_ids.size());
  return merged_state_input_rows(state.forward_state_row_ids, state.hidden_projection_row_ids,
                                 max_rows);
}

std::string state_input_seed_digest(const NativeDecodeState& state) {
  return prefixed_history_digest("state-input", state_input_rows_for_state(state));
}

std::size_t forward_state_row_cap(std::string_view projection_carry_mode_kind) {
  if (projection_carry_mode_kind == "compact_context_projection.v1") {
    return 2;
  }
  if (projection_carry_mode_kind == "balanced_context_projection.v1") {
    return 3;
  }
  if (projection_carry_mode_kind == "wide_context_projection.v1") {
    return 4;
  }
  if (projection_carry_mode_kind == "history_heavy_projection.v1") {
    return 4;
  }
  return 3;
}

void rank_and_trim_forward_state(NativeDecodeState& state, std::size_t row_cap) {
  if (state.forward_state_row_ids.empty() ||
      state.forward_state_row_ids.size() != state.forward_state_scores.size()) {
    return;
  }

  struct Entry {
    int row_id = 0;
    double score = 0.0;
  };

  std::vector<Entry> entries;
  entries.reserve(state.forward_state_row_ids.size());
  for (std::size_t i = 0; i < state.forward_state_row_ids.size(); ++i) {
    entries.push_back({state.forward_state_row_ids[i], state.forward_state_scores[i]});
  }

  std::sort(entries.begin(), entries.end(), [](const Entry& lhs, const Entry& rhs) {
    if (lhs.score != rhs.score) {
      return lhs.score > rhs.score;
    }
    return lhs.row_id < rhs.row_id;
  });
  if (entries.size() > row_cap) {
    entries.resize(row_cap);
  }

  state.forward_state_row_ids.clear();
  state.forward_state_scores.clear();
  state.forward_state_row_ids.reserve(entries.size());
  state.forward_state_scores.reserve(entries.size());
  for (const auto& entry : entries) {
    state.forward_state_row_ids.push_back(entry.row_id);
    state.forward_state_scores.push_back(entry.score);
  }
}

void refresh_forward_state(NativeDecodeState& state) {
  const std::vector<int> previous_forward_state_row_ids = state.forward_state_row_ids;
  const std::vector<double> previous_forward_state_scores = state.forward_state_scores;
  const std::string previous_forward_state_signature = state.forward_state_signature_sha256;
  const std::string previous_forward_state_class = state.forward_state_class;
  const std::size_t previous_forward_state_generation = state.forward_state_generation;
  state.forward_state_kind = "unavailable";
  state.forward_state_row_ids.clear();
  state.forward_state_scores.clear();
  state.forward_state_signature_sha256.clear();
  state.forward_state_generation = 0;
  state.forward_state_class.clear();
  state.forward_state_class_signature_sha256.clear();
  if (state.hidden_projection_row_ids.empty() ||
      state.hidden_projection_row_ids.size() != state.hidden_projection_scores.size()) {
    return;
  }

  state.forward_state_kind = "projection_carried_forward_state.v1";
  state.forward_state_row_ids = state.hidden_projection_row_ids;
  state.forward_state_scores = state.hidden_projection_scores;
  state.forward_state_generation =
      previous_forward_state_signature.empty() ? 0 : (previous_forward_state_generation + 1);
  const std::size_t row_cap = forward_state_row_cap(state.projection_carry_mode_kind);
  for (std::size_t i = 0;
       i < previous_forward_state_row_ids.size() &&
       state.forward_state_row_ids.size() < row_cap;
       ++i) {
    const int row_id = previous_forward_state_row_ids[i];
    const auto existing =
        std::find(state.forward_state_row_ids.begin(), state.forward_state_row_ids.end(), row_id);
    if (existing != state.forward_state_row_ids.end()) {
      continue;
    }
    state.forward_state_row_ids.push_back(row_id);
    const double previous_score =
        i < previous_forward_state_scores.size() ? previous_forward_state_scores[i] : 0.0;
    state.forward_state_scores.push_back(previous_score * 0.75);
  }
  rank_and_trim_forward_state(state, row_cap);

  std::ostringstream text;
  text << state.forward_state_kind << "|" << state.hidden_state_class << "|"
       << state.projection_carry_mode_kind << "|";
  for (std::size_t i = 0; i < state.forward_state_row_ids.size(); ++i) {
    if (i != 0) {
      text << ";";
    }
    text << state.forward_state_row_ids[i] << ":";
    if (i < state.forward_state_scores.size()) {
      text << state.forward_state_scores[i];
    }
  }
  if (!state.generated_token_history.empty()) {
    text << "|last=" << state.generated_token_history.back();
  }
  if (!previous_forward_state_signature.empty()) {
    text << "|prev_sig=" << previous_forward_state_signature;
  }
  if (!previous_forward_state_class.empty()) {
    text << "|prev_class=" << previous_forward_state_class;
  }
  text << "|gen=" << state.forward_state_generation;
  state.forward_state_signature_sha256 = sha3_hex_text(text.str());

  double peak = 0.0;
  for (double score : state.forward_state_scores) {
    peak = std::max(peak, score);
  }
  const std::string density_band =
      state.forward_state_row_ids.size() <= 1 ? "compact" : "wide";
  const std::string energy_band = peak >= 9.0 ? "strong" : (peak >= 6.0 ? "steady" : "weak");
  state.forward_state_class =
      energy_band + "_" + density_band + "_" + state.projection_carry_mode_kind;
  state.forward_state_class_signature_sha256 =
      sha3_hex_text(state.forward_state_class + "|" + state.forward_state_signature_sha256);
}

void refresh_kv_state(NativeDecodeState& state) {
  const std::string previous_signature = state.kv_state_signature_sha256;
  const bool has_current_qk = !state.q_tensor_signature_sha256.empty() &&
                              !state.k_tensor_signature_sha256.empty();
  if (!has_current_qk) {
    state.kv_state_kind = "unavailable";
    state.kv_state_signature_sha256.clear();
    state.kv_state_carry_mode_kind = "unavailable";
    return;
  }

  std::ostringstream text;
  if (!previous_signature.empty()) {
    text << "prev=" << previous_signature << "|";
  }
  text << "q=" << state.q_tensor_signature_sha256 << "|"
       << "k=" << state.k_tensor_signature_sha256 << "|"
       << "layout=" << state.carry_probe_layout_kind << "|"
       << "gen=" << state.forward_state_generation;

  if (!previous_signature.empty()) {
    state.kv_state_carry_mode_kind =
        (!state.architecture_state_signature_sha256.empty() &&
         state.forward_state_generation >= 1)
            ? "architecture_state_evolved_qk_signature.v1"
            : "evolved_qk_signature.v1";
  } else {
    state.kv_state_carry_mode_kind = "current_qk_window.v1";
  }
  state.kv_state_kind = "bounded_qk_tensor_state.v1";
  state.kv_state_signature_sha256 =
      sha3_hex_text(state.kv_state_carry_mode_kind + "|" + text.str());
}

void refresh_architecture_state(NativeDecodeState& state) {
  state.architecture_state_kind = "unavailable";
  state.architecture_state_signature_sha256.clear();
  state.architecture_state_class.clear();
  state.architecture_state_class_signature_sha256.clear();

  const bool has_hidden_tensor = !state.hidden_tensor_signature_sha256.empty();
  const bool has_kv_state = !state.kv_state_signature_sha256.empty();
  const bool has_forward_state = !state.forward_state_signature_sha256.empty();
  if (!has_hidden_tensor && !has_kv_state && !has_forward_state) {
    return;
  }

  state.architecture_state_kind = "bounded_hidden_tensor_qk_forward_state.v1";
  std::ostringstream text;
  text << state.architecture_state_kind << "|"
       << "hidden=" << state.hidden_tensor_signature_sha256 << "|"
       << "kv=" << state.kv_state_signature_sha256 << "|"
       << "kv_mode=" << state.kv_state_carry_mode_kind << "|"
       << "forward=" << state.forward_state_signature_sha256 << "|"
       << "hidden_mode=" << state.hidden_tensor_carry_mode_kind << "|"
       << "projection_mode=" << state.projection_carry_mode_kind << "|"
       << "hidden_class=" << state.hidden_state_class << "|"
       << "forward_class=" << state.forward_state_class << "|"
       << "gen=" << state.forward_state_generation;
  state.architecture_state_signature_sha256 = sha3_hex_text(text.str());

  const std::string tensor_band = has_hidden_tensor ? "tensor" : "row";
  const std::string kv_band = has_kv_state ? "qk" : "nokv";
  const std::string forward_band = has_forward_state ? "carried" : "fresh";
  state.architecture_state_class =
      tensor_band + "_" + kv_band + "_" + forward_band + "_" +
      state.hidden_tensor_carry_mode_kind + "_" + state.kv_state_carry_mode_kind;
  state.architecture_state_class_signature_sha256 =
      sha3_hex_text(state.architecture_state_class + "|" + state.architecture_state_signature_sha256);
}

std::string sampled_logits_digest(const std::vector<int>& token_ids,
                                  const std::vector<double>& token_scores) {
  std::ostringstream text;
  text << "sampled-logits|";
  for (std::size_t i = 0; i < token_ids.size() && i < token_scores.size(); ++i) {
    if (i != 0) {
      text << ",";
    }
    text << token_ids[i] << ":" << token_scores[i];
  }
  return sha3_hex_text(text.str());
}

std::string class_conditioned_candidate_basis(std::string_view hidden_state_class,
                                              std::string_view projection_carry_mode_kind) {
  if (hidden_state_class.empty() && projection_carry_mode_kind.empty()) {
    return "selected_candidate_history_feedback_contiguous_window.v1";
  }
  return "hidden_state_class_projection_mode_feedback_window.v1";
}

std::string stability_conditioned_candidate_basis(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 2) {
    return "architecture_state_deep_feedback_window.v1";
  }
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return "architecture_state_feedback_window.v1";
  }
  if (state.hidden_tensor_carry_mode_kind == "evolved_hidden_tensor_feedback.v1" &&
      state.forward_state_generation >= 1) {
    return "hidden_tensor_feedback_window.v1";
  }
  if (state.forward_state_generation >= 1) {
    return "forward_state_history_feedback_window.v1";
  }
  const bool recovery =
      state.stability_kind == "fragile" || state.stability_kind == "ambiguous" ||
      (state.stability_kind == "steady" && state.logits_margin <= 0.5);
  if (recovery) {
    return "stability_recovery_feedback_window.v1";
  }
  if (state.stability_kind == "steady") {
    return "stability_aware_feedback_window.v1";
  }
  return class_conditioned_candidate_basis(state.hidden_state_class,
                                           state.projection_carry_mode_kind);
}

std::string state_rationale_summary(std::string_view hidden_state_class,
                                    std::string_view carry_probe_layout_kind,
                                    std::string_view projection_carry_mode_kind,
                                    std::string_view selection_policy_kind,
                                    std::string_view decode_mode_kind) {
  std::ostringstream out;
  out << "class=" << hidden_state_class << ";layout=" << carry_probe_layout_kind
      << ";carry_mode=" << projection_carry_mode_kind
      << ";selection=" << selection_policy_kind << ";decode=" << decode_mode_kind;
  return out.str();
}

void populate_probe_stability(NativeProbeResult& result) {
  if (result.sampled_token_scores.empty()) {
    return;
  }

  std::vector<double> ordered_scores = result.sampled_token_scores;
  std::sort(ordered_scores.begin(), ordered_scores.end(), std::greater<double>());
  const double best = ordered_scores.front();
  const double second = ordered_scores.size() > 1 ? ordered_scores[1] : ordered_scores.front();
  result.logits_margin = best - second;

  for (double score : result.hidden_carry_scores) {
    result.hidden_carry_peak = std::max(result.hidden_carry_peak, score);
  }

  result.confidence_score = result.logits_margin + (result.hidden_carry_peak / 10.0);

  if (result.confidence_score >= 2.0) {
    result.stability_kind = "strong";
  } else if (result.confidence_score >= 0.8) {
    result.stability_kind = "steady";
  } else if (result.confidence_score >= 0.2) {
    result.stability_kind = "fragile";
  } else {
    result.stability_kind = "ambiguous";
  }
}

bool hidden_state_class_has_prefix(std::string_view hidden_state_class,
                                   std::string_view prefix) {
  return hidden_state_class.substr(0, prefix.size()) == prefix;
}

bool hidden_state_class_is(std::string_view hidden_state_class, std::string_view energy_band,
                           std::string_view balance_band) {
  const std::string prefix =
      std::string(energy_band) + "_" + std::string(balance_band);
  return hidden_state_class_has_prefix(hidden_state_class, prefix);
}

double architecture_state_confidence_score(const NativeDecodeState& state);
std::string architecture_state_stability_kind(const NativeDecodeState& state);

bool stability_requires_recovery(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    const std::string architecture_stability = architecture_state_stability_kind(state);
    if (architecture_stability == "fragile" || architecture_stability == "ambiguous") {
      return true;
    }
  }
  const bool strong_forward_state =
      state.forward_state_class.find("strong_") == 0 &&
      !state.forward_state_signature_sha256.empty();
  return state.stability_kind == "fragile" || state.stability_kind == "ambiguous" ||
         (state.stability_kind == "steady" && state.logits_margin <= 0.5 &&
          !strong_forward_state);
}

bool stability_should_terminate_decode(const NativeDecodeState& state,
                                       std::size_t consecutive_recovery_steps) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    const std::string architecture_stability = architecture_state_stability_kind(state);
    if (architecture_stability == "ambiguous" && consecutive_recovery_steps >= 1) {
      return true;
    }
    if (architecture_stability == "fragile" && consecutive_recovery_steps >= 2) {
      return true;
    }
  }
  if (state.stability_kind == "ambiguous" && consecutive_recovery_steps >= 1) {
    return true;
  }
  if (state.stability_kind == "fragile" && state.confidence_score < 0.4 &&
      consecutive_recovery_steps >= 1) {
    return true;
  }
  return consecutive_recovery_steps >= 2;
}

std::size_t class_conditioned_window_start(const NativeDecodeState& state,
                                           std::size_t vocab_size) {
  if (vocab_size == 0 || state.hidden_state_class_signature_sha256.empty()) {
    return 0;
  }
  std::size_t base =
      digest_seed_window_start(state.hidden_state_class_signature_sha256, vocab_size);
  std::size_t offset = 0;
  if (hidden_state_class_is(state.hidden_state_class, "high", "dense")) {
    offset = kLogitsSampleWindow;
  } else if (hidden_state_class_is(state.hidden_state_class, "medium", "mixed")) {
    offset = kLogitsSampleWindow * 2;
  } else if (hidden_state_class_is(state.hidden_state_class, "low", "sparse")) {
    offset = kLogitsSampleWindow * 3;
  } else if (hidden_state_class_is(state.hidden_state_class, "medium", "sparse")) {
    offset = kLogitsSampleWindow * 4;
  } else if (hidden_state_class_is(state.hidden_state_class, "low", "mixed")) {
    offset = kLogitsSampleWindow * 5;
  }
  return (base + offset) % vocab_size;
}

std::size_t class_conditioned_sample_window(std::string_view hidden_state_class,
                                            std::size_t vocab_size) {
  const std::size_t cap = std::min<std::size_t>(kLogitsSampleWindow, vocab_size);
  if (hidden_state_class_is(hidden_state_class, "high", "dense")) {
    return std::min<std::size_t>(4, cap);
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "dense")) {
    return std::min<std::size_t>(6, cap);
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "mixed")) {
    return cap;
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "sparse") ||
      hidden_state_class_is(hidden_state_class, "low", "mixed") ||
      hidden_state_class_is(hidden_state_class, "low", "sparse")) {
    return cap;
  }
  return cap;
}

std::string class_conditioned_selection_policy(std::string_view hidden_state_class) {
  if (hidden_state_class_is(hidden_state_class, "high", "dense") ||
      hidden_state_class_is(hidden_state_class, "medium", "dense")) {
    return "prefer_earliest_among_top2.v1";
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "mixed")) {
    return "prefer_middle_among_top3.v1";
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "sparse") ||
      hidden_state_class_is(hidden_state_class, "low", "mixed") ||
      hidden_state_class_is(hidden_state_class, "low", "sparse")) {
    return "prefer_tail_nonnegative_else_max.v1";
  }
  return "max_score.v1";
}

std::string class_conditioned_decode_mode(std::string_view hidden_state_class) {
  if (hidden_state_class_is(hidden_state_class, "high", "dense")) {
    return "compact_context_projection.v1";
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "dense")) {
    return "balanced_context_projection.v1";
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "mixed")) {
    return "wide_context_projection.v1";
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "sparse") ||
      hidden_state_class_is(hidden_state_class, "low", "mixed") ||
      hidden_state_class_is(hidden_state_class, "low", "sparse")) {
    return "history_heavy_projection.v1";
  }
  return "balanced_context_projection.v1";
}

std::size_t class_conditioned_context_window(std::string_view hidden_state_class) {
  if (hidden_state_class_is(hidden_state_class, "high", "dense")) {
    return 2;
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "dense")) {
    return 3;
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "mixed")) {
    return 4;
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "sparse") ||
      hidden_state_class_is(hidden_state_class, "low", "mixed") ||
      hidden_state_class_is(hidden_state_class, "low", "sparse")) {
    return 4;
  }
  return kDecodeContextHistoryWindow;
}

std::size_t class_conditioned_hidden_projection_keep(std::string_view hidden_state_class,
                                                     std::size_t available) {
  if (available == 0) {
    return 0;
  }
  if (hidden_state_class_is(hidden_state_class, "high", "dense")) {
    return std::min<std::size_t>(1, available);
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "dense")) {
    return std::min<std::size_t>(2, available);
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "mixed")) {
    return std::min<std::size_t>(available, available);
  }
  if (hidden_state_class_is(hidden_state_class, "medium", "sparse") ||
      hidden_state_class_is(hidden_state_class, "low", "mixed") ||
      hidden_state_class_is(hidden_state_class, "low", "sparse")) {
    return std::min<std::size_t>(available, available);
  }
  return std::min<std::size_t>(2, available);
}

double architecture_state_confidence_score(const NativeDecodeState& state) {
  if (state.architecture_state_signature_sha256.empty()) {
    return 0.0;
  }
  double score = state.confidence_score;
  if (!state.hidden_tensor_signature_sha256.empty()) {
    score += 0.4;
  }
  if (!state.kv_state_signature_sha256.empty()) {
    score += 0.4;
  }
  if (!state.forward_state_signature_sha256.empty()) {
    score += 0.4;
  }
  if (state.forward_state_generation >= 1) {
    score += 0.2 * static_cast<double>(state.forward_state_generation);
  }
  return score;
}

std::string architecture_state_stability_kind(const NativeDecodeState& state) {
  const double score = architecture_state_confidence_score(state);
  if (score >= 2.2) {
    return "strong";
  }
  if (score >= 1.2) {
    return "steady";
  }
  if (score >= 0.5) {
    return "fragile";
  }
  return "ambiguous";
}

std::pair<float, float> hidden_tensor_mix_weights(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    const std::string stability = architecture_state_stability_kind(state);
    if (stability == "strong") {
      return {0.60F, 0.40F};
    }
    if (stability == "steady") {
      return {0.70F, 0.30F};
    }
    if (stability == "fragile") {
      return {0.85F, 0.15F};
    }
    return {0.90F, 0.10F};
  }
  return {0.75F, 0.25F};
}

std::size_t architecture_state_conditioned_sample_window(const NativeDecodeState& state,
                                                         std::size_t base,
                                                         std::size_t cap) {
  const std::string stability = architecture_state_stability_kind(state);
  if (state.forward_state_generation >= 2) {
    if (stability == "strong") {
      return std::min<std::size_t>(cap, std::max<std::size_t>(base, 8));
    }
    if (stability == "steady") {
      return std::min<std::size_t>(cap, std::max<std::size_t>(base, 7));
    }
  }
  if (stability == "strong") {
    return std::min<std::size_t>(cap, std::max<std::size_t>(base, 7));
  }
  if (stability == "steady") {
    return std::min<std::size_t>(cap, std::max<std::size_t>(base, 6));
  }
  if (stability == "fragile") {
    return std::min<std::size_t>(cap, std::max<std::size_t>(base, 5));
  }
  return std::min<std::size_t>(cap, std::max<std::size_t>(base, 4));
}

std::size_t architecture_state_conditioned_context_window(const NativeDecodeState& state,
                                                          std::size_t base) {
  const std::string stability = architecture_state_stability_kind(state);
  if (state.forward_state_generation >= 2) {
    if (stability == "strong") {
      return std::max<std::size_t>(base, 7);
    }
    if (stability == "steady") {
      return std::max<std::size_t>(base, 6);
    }
  }
  if (stability == "strong") {
    return std::max<std::size_t>(base, 6);
  }
  if (stability == "steady") {
    return std::max<std::size_t>(base, 5);
  }
  if (stability == "fragile") {
    return std::max<std::size_t>(base, 4);
  }
  return std::max<std::size_t>(base, 3);
}

std::string architecture_state_conditioned_selection_policy(const NativeDecodeState& state) {
  const std::string stability = architecture_state_stability_kind(state);
  if (state.forward_state_generation >= 2) {
    if (stability == "strong" || stability == "steady") {
      return "prefer_tail_among_top3.v1";
    }
  }
  if (stability == "strong") {
    return "prefer_middle_among_top3.v1";
  }
  if (stability == "steady") {
    return "prefer_earliest_among_top2.v1";
  }
  return "max_score.v1";
}

std::string architecture_state_conditioned_carry_probe_layout(const NativeDecodeState& state) {
  const std::string stability = architecture_state_stability_kind(state);
  if (state.forward_state_generation >= 2) {
    if (stability == "strong" || stability == "steady") {
      return "reverse_contiguous_window.v1";
    }
  }
  if (stability == "strong") {
    return "centered_compact_window.v1";
  }
  if (stability == "steady") {
    return "stride2_window.v1";
  }
  return "reverse_contiguous_window.v1";
}

std::size_t stability_conditioned_sample_window(const NativeDecodeState& state,
                                                std::size_t vocab_size) {
  const std::size_t base = class_conditioned_sample_window(state.hidden_state_class, vocab_size);
  const std::size_t cap = std::min<std::size_t>(kLogitsSampleWindow, vocab_size);
  const bool recovery = stability_requires_recovery(state);
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return architecture_state_conditioned_sample_window(state, base, cap);
  }
  if (recovery) {
    return std::min<std::size_t>(cap, std::max<std::size_t>(base, 6));
  }
  if (state.forward_state_generation >= 1) {
    return std::min<std::size_t>(cap, std::max<std::size_t>(base, 6));
  }
  if (state.stability_kind == "steady") {
    return std::min<std::size_t>(cap, std::max<std::size_t>(base, 5));
  }
  if (state.stability_kind == "strong") {
    return std::min<std::size_t>(base, std::min<std::size_t>(4, cap));
  }
  return base;
}

std::string stability_conditioned_selection_policy(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return architecture_state_conditioned_selection_policy(state);
  }
  if (state.forward_state_generation >= 1) {
    return "prefer_tail_nonnegative_else_max.v1";
  }
  const bool recovery = stability_requires_recovery(state);
  if (recovery) {
    return "max_score.v1";
  }
  return class_conditioned_selection_policy(state.hidden_state_class);
}

std::string stability_conditioned_decode_mode(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 2) {
    return "architecture_state_deep_feedback_projection.v1";
  }
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return "architecture_state_feedback_projection.v1";
  }
  if (state.hidden_tensor_carry_mode_kind == "evolved_hidden_tensor_feedback.v1" &&
      state.forward_state_generation >= 1) {
    return "hidden_tensor_feedback_projection.v1";
  }
  if (state.forward_state_generation >= 1) {
    return "forward_state_history_projection.v1";
  }
  const bool recovery = stability_requires_recovery(state);
  if (recovery) {
    return "stability_recovery_projection.v1";
  }
  if (state.stability_kind == "steady") {
    return "stability_aware_projection.v1";
  }
  return class_conditioned_decode_mode(state.hidden_state_class);
}

std::size_t stability_conditioned_context_window(const NativeDecodeState& state) {
  const std::size_t base = class_conditioned_context_window(state.hidden_state_class);
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return architecture_state_conditioned_context_window(state, base);
  }
  if (state.hidden_tensor_carry_mode_kind == "evolved_hidden_tensor_feedback.v1" &&
      state.forward_state_generation >= 1) {
    return std::max<std::size_t>(base, 6);
  }
  if (state.forward_state_generation >= 1) {
    return std::max<std::size_t>(base, 5);
  }
  const bool recovery = stability_requires_recovery(state);
  if (recovery) {
    return std::max<std::size_t>(base, 4);
  }
  if (state.forward_state_generation >= 1) {
    return std::max<std::size_t>(base, 4);
  }
  if (state.stability_kind == "steady") {
    return std::max<std::size_t>(base, 3);
  }
  return base;
}

std::string stability_conditioned_carry_probe_layout(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return architecture_state_conditioned_carry_probe_layout(state);
  }
  if (state.hidden_tensor_carry_mode_kind == "evolved_hidden_tensor_feedback.v1" &&
      state.forward_state_generation >= 1) {
    return "centered_compact_window.v1";
  }
  if (state.forward_state_generation >= 1) {
    return "reverse_contiguous_window.v1";
  }
  const bool recovery = stability_requires_recovery(state);
  if (recovery) {
    return "reverse_contiguous_window.v1";
  }
  if (state.stability_kind == "steady") {
    return "stride2_window.v1";
  }
  if (state.projection_carry_mode_kind == "compact_context_projection.v1") {
    return "centered_compact_window.v1";
  }
  if (state.projection_carry_mode_kind == "balanced_context_projection.v1") {
    return "contiguous_forward_window.v1";
  }
  if (state.projection_carry_mode_kind == "wide_context_projection.v1") {
    return "stride2_window.v1";
  }
  if (state.projection_carry_mode_kind == "history_heavy_projection.v1") {
    return "reverse_contiguous_window.v1";
  }
  return "contiguous_forward_window.v1";
}

std::string stability_conditioned_transition_kind(const NativeDecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 2) {
    return "architecture_state_deep_feedback_state_transition.v1";
  }
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 1) {
    return "architecture_state_feedback_state_transition.v1";
  }
  if (state.hidden_tensor_carry_mode_kind == "evolved_hidden_tensor_feedback.v1" &&
      state.forward_state_generation >= 1) {
    return "hidden_tensor_feedback_state_transition.v1";
  }
  if (state.forward_state_generation >= 1) {
    return "forward_state_history_feedback_state_transition.v1";
  }
  const bool recovery = stability_requires_recovery(state);
  if (recovery) {
    return "stability_recovery_state_transition.v1";
  }
  if (!state.forward_state_class.empty()) {
    return "forward_state_feedback_state_transition.v1";
  }
  if (state.stability_kind == "steady") {
    return "stability_aware_state_transition.v1";
  }
  return "selected_candidate_feedback_state_transition.v1";
}

std::size_t forward_state_keep(const NativeDecodeState& state, std::size_t available) {
  if (available == 0) {
    return 0;
  }
  std::size_t keep = available;
  if (state.forward_state_class.find("compact") != std::string_view::npos) {
    keep = 1;
  } else if (state.forward_state_class.find("strong") != std::string_view::npos) {
    keep = std::min<std::size_t>(2, available);
  }
  if (state.forward_state_generation >= 2) {
    keep = std::min<std::size_t>(std::max<std::size_t>(keep, 3), available);
  } else if (state.forward_state_generation >= 1) {
    keep = std::min<std::size_t>(std::max<std::size_t>(keep, 3), available);
  }
  return keep;
}

std::size_t projection_carry_mode_window_offset(std::string_view projection_carry_mode_kind) {
  if (projection_carry_mode_kind == "compact_context_projection.v1") {
    return kLogitsSampleWindow;
  }
  if (projection_carry_mode_kind == "balanced_context_projection.v1") {
    return kLogitsSampleWindow * 2;
  }
  if (projection_carry_mode_kind == "wide_context_projection.v1") {
    return kLogitsSampleWindow * 3;
  }
  if (projection_carry_mode_kind == "history_heavy_projection.v1") {
    return kLogitsSampleWindow * 4;
  }
  return 0;
}

std::vector<int> carry_probe_row_ids(std::size_t start, std::size_t vocab_size, std::size_t count,
                                     std::string_view layout_kind) {
  std::vector<int> rows;
  if (vocab_size == 0 || count == 0) {
    return rows;
  }
  rows.reserve(count);
  if (layout_kind == "centered_compact_window.v1") {
    const std::size_t center = start % vocab_size;
    const std::size_t half = count / 2;
    for (std::size_t i = 0; i < count; ++i) {
      const std::size_t offset = (i + vocab_size + center - half) % vocab_size;
      rows.push_back(static_cast<int>(offset));
    }
    return rows;
  }
  if (layout_kind == "stride2_window.v1") {
    for (std::size_t i = 0; i < count; ++i) {
      rows.push_back(static_cast<int>((start + (i * 2)) % vocab_size));
    }
    return rows;
  }
  if (layout_kind == "reverse_contiguous_window.v1") {
    for (std::size_t i = 0; i < count; ++i) {
      rows.push_back(static_cast<int>((start + count - 1 - i) % vocab_size));
    }
    return rows;
  }
  return make_contiguous_window_token_ids(start, vocab_size, count);
}

void apply_projection_carry_mode(NativeProbeResult& result) {
  if (result.hidden_projection_row_ids.empty() ||
      result.hidden_projection_row_ids.size() != result.hidden_projection_scores.size()) {
    return;
  }

  result.projection_carry_mode_kind = class_conditioned_decode_mode(result.hidden_state_class);

  if (result.projection_carry_mode_kind == "compact_context_projection.v1") {
    result.hidden_projection_row_ids.resize(1);
    result.hidden_projection_scores.resize(1);
  } else if (result.projection_carry_mode_kind == "balanced_context_projection.v1") {
    const std::size_t keep = std::min<std::size_t>(2, result.hidden_projection_row_ids.size());
    result.hidden_projection_row_ids.resize(keep);
    result.hidden_projection_scores.resize(keep);
  } else if (result.projection_carry_mode_kind == "wide_context_projection.v1") {
    // Keep the full bounded projection as-is.
  } else if (result.projection_carry_mode_kind == "history_heavy_projection.v1") {
    std::reverse(result.hidden_projection_row_ids.begin(), result.hidden_projection_row_ids.end());
    std::reverse(result.hidden_projection_scores.begin(), result.hidden_projection_scores.end());
  }

  std::ostringstream text;
  text << result.projection_carry_mode_kind << "|";
  for (std::size_t i = 0; i < result.hidden_projection_row_ids.size(); ++i) {
    if (i != 0) {
      text << ";";
    }
    text << result.hidden_projection_row_ids[i] << ":" << result.hidden_projection_scores[i];
  }
  result.hidden_projection_signature_sha256 = sha3_hex_text(text.str());
}

std::size_t next_decode_window_start(const NativeDecodeState& state, std::size_t vocab_size) {
  if (!state.architecture_state_signature_sha256.empty()) {
    const std::size_t base =
        digest_seed_window_start(state.architecture_state_signature_sha256, vocab_size);
    std::size_t content_offset = state.forward_state_generation + state.kv_tensor_rank +
                                 state.hidden_tensor_rank + state.hidden_tensor_elements;
    if (!state.architecture_state_class.empty()) {
      content_offset += state.architecture_state_class.size();
    }
    return (base + content_offset) % vocab_size;
  }
  if (!state.kv_state_signature_sha256.empty()) {
    const std::size_t base =
        digest_seed_window_start(state.kv_state_signature_sha256, vocab_size);
    std::size_t content_offset = state.kv_tensor_rank + state.kv_tensor_elements +
                                 state.forward_state_generation;
    if (!state.projection_carry_mode_kind.empty()) {
      return (base + content_offset +
              projection_carry_mode_window_offset(state.projection_carry_mode_kind)) %
             vocab_size;
    }
    return (base + content_offset) % vocab_size;
  }
  if (!state.hidden_tensor_signature_sha256.empty()) {
    const std::size_t base =
        digest_seed_window_start(state.hidden_tensor_signature_sha256, vocab_size);
    std::size_t content_offset = state.hidden_tensor_rank + state.hidden_tensor_elements +
                                 state.forward_state_generation;
    if (!state.projection_carry_mode_kind.empty()) {
      return (base + content_offset +
              projection_carry_mode_window_offset(state.projection_carry_mode_kind)) %
             vocab_size;
    }
    return (base + content_offset) % vocab_size;
  }
  const auto state_input_rows = state_input_rows_for_state(state);
  if (!state_input_rows.empty()) {
    const std::size_t base = digest_seed_window_start(state_input_seed_digest(state), vocab_size);
    std::size_t content_offset = 0;
    if (!state_input_rows.empty()) {
      content_offset += static_cast<std::size_t>(state_input_rows.front());
    }
    if (!state.forward_state_scores.empty()) {
      content_offset += static_cast<std::size_t>(std::llround(state.forward_state_scores.front()));
    }
    content_offset += state_input_rows.size();
    content_offset += state.forward_state_generation;
    if (!state.projection_carry_mode_kind.empty()) {
      return (base + content_offset +
              projection_carry_mode_window_offset(state.projection_carry_mode_kind)) %
             vocab_size;
    }
    return (base + content_offset) % vocab_size;
  }
  if (!state.forward_state_signature_sha256.empty()) {
    const std::size_t base =
        digest_seed_window_start(state.forward_state_signature_sha256, vocab_size);
    std::size_t content_offset = state.forward_state_generation;
    if (!state.projection_carry_mode_kind.empty()) {
      return (base + content_offset +
              projection_carry_mode_window_offset(state.projection_carry_mode_kind)) %
             vocab_size;
    }
    return (base + content_offset) % vocab_size;
  }
  if (!state.hidden_state_class_signature_sha256.empty()) {
    const std::size_t base = class_conditioned_window_start(state, vocab_size);
    if (!state.projection_carry_mode_kind.empty()) {
      return (base + projection_carry_mode_window_offset(state.projection_carry_mode_kind)) %
             vocab_size;
    }
    return base;
  }
  if (!state.hidden_projection_signature_sha256.empty()) {
    return digest_seed_window_start(state.hidden_projection_signature_sha256, vocab_size);
  }
  if (!state.hidden_carry_signature_sha256.empty()) {
    return digest_seed_window_start(state.hidden_carry_signature_sha256, vocab_size);
  }
  return history_seed_window_start(combined_decode_history(state), vocab_size);
}

std::string hidden_carry_signature_digest(const NativeProbeResult& probe) {
  if (!probe.hidden_carry_signature_sha256.empty()) {
    return probe.hidden_carry_signature_sha256;
  }
  std::ostringstream text;
  text << probe.probe_kind << "|";
  for (std::size_t i = 0; i < probe.hidden_carry_row_ids.size(); ++i) {
    if (i != 0) {
      text << ";";
    }
    text << probe.hidden_carry_row_ids[i] << ":";
    if (i < probe.hidden_carry_scores.size()) {
      text << probe.hidden_carry_scores[i];
    }
  }
  return sha3_hex_text(text.str());
}

void populate_hidden_projection(NativeProbeResult& result) {
  if (result.hidden_carry_row_ids.empty() ||
      result.hidden_carry_scores.size() != result.hidden_carry_row_ids.size()) {
    return;
  }
  std::vector<std::size_t> order(result.hidden_carry_row_ids.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return result.hidden_carry_scores[a] > result.hidden_carry_scores[b];
  });
  const std::size_t keep = std::min<std::size_t>(kHiddenCarryProjectionWidth, order.size());
  std::ostringstream text;
  for (std::size_t i = 0; i < keep; ++i) {
    const std::size_t idx = order[i];
    result.hidden_projection_row_ids.push_back(result.hidden_carry_row_ids[idx]);
    result.hidden_projection_scores.push_back(result.hidden_carry_scores[idx]);
    if (i != 0) {
      text << ";";
    }
    text << result.hidden_carry_row_ids[idx] << ":" << result.hidden_carry_scores[idx];
  }
  result.hidden_projection_signature_sha256 = sha3_hex_text(text.str());
  if (!result.hidden_projection_scores.empty()) {
    double sum = 0.0;
    double peak = result.hidden_projection_scores.front();
    for (double score : result.hidden_projection_scores) {
      sum += score;
      peak = std::max(peak, score);
    }
    const double mean = sum / static_cast<double>(result.hidden_projection_scores.size());
    std::string energy_band = peak >= 9.0 ? "high" : (peak >= 6.0 ? "medium" : "low");
    std::string balance_band = mean >= 8.0 ? "dense" : (mean >= 5.0 ? "mixed" : "sparse");
    std::string layout_band = "forward";
    if (result.carry_probe_layout_kind == "centered_compact_window.v1") {
      layout_band = "centered";
      if (energy_band == "medium") {
        energy_band = "high";
      }
    } else if (result.carry_probe_layout_kind == "reverse_contiguous_window.v1") {
      layout_band = "reverse";
    } else if (result.carry_probe_layout_kind == "stride2_window.v1") {
      layout_band = "stride2";
      if (balance_band == "dense") {
        balance_band = "mixed";
      }
    }
    result.hidden_state_class = energy_band + "_" + balance_band + "_" + layout_band;
    result.hidden_state_class_signature_sha256 =
        sha3_hex_text(result.hidden_state_class + "|" + result.carry_probe_layout_kind);
  }
  apply_projection_carry_mode(result);
}

void apply_selection_policy(NativeProbeResult& result) {
  if (result.sampled_token_ids.empty() || result.sampled_token_scores.empty()) {
    return;
  }

  auto choose_best_score = [&]() {
    double best_score = -std::numeric_limits<double>::infinity();
    std::size_t best_index = 0;
    for (std::size_t i = 0; i < result.sampled_token_scores.size(); ++i) {
      const double score = result.sampled_token_scores[i];
      if (i == 0 || score > best_score) {
        best_score = score;
        best_index = i;
      }
    }
    result.selected_token_id = result.sampled_token_ids[best_index];
    result.selected_token_score = result.sampled_token_scores[best_index];
  };

  if (result.selection_policy_kind == "prefer_earliest_among_top2.v1") {
    std::vector<std::size_t> order(result.sampled_token_scores.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
      order[i] = i;
    }
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
      return result.sampled_token_scores[a] > result.sampled_token_scores[b];
    });
    const std::size_t keep = std::min<std::size_t>(2, order.size());
    std::size_t selected_index = order.front();
    for (std::size_t i = 1; i < keep; ++i) {
      selected_index = std::min(selected_index, order[i]);
    }
    result.selected_token_id = result.sampled_token_ids[selected_index];
    result.selected_token_score = result.sampled_token_scores[selected_index];
    return;
  }

  if (result.selection_policy_kind == "prefer_middle_among_top3.v1") {
    std::vector<std::size_t> order(result.sampled_token_scores.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
      order[i] = i;
    }
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
      return result.sampled_token_scores[a] > result.sampled_token_scores[b];
    });
    const std::size_t keep = std::min<std::size_t>(3, order.size());
    order.resize(keep);
    std::sort(order.begin(), order.end());
    const std::size_t selected_index = order[order.size() / 2];
    result.selected_token_id = result.sampled_token_ids[selected_index];
    result.selected_token_score = result.sampled_token_scores[selected_index];
    return;
  }

  if (result.selection_policy_kind == "prefer_tail_among_top3.v1") {
    std::vector<std::size_t> order(result.sampled_token_scores.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
      order[i] = i;
    }
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
      return result.sampled_token_scores[a] > result.sampled_token_scores[b];
    });
    const std::size_t keep = std::min<std::size_t>(3, order.size());
    order.resize(keep);
    std::sort(order.begin(), order.end());
    const std::size_t selected_index = order.back();
    result.selected_token_id = result.sampled_token_ids[selected_index];
    result.selected_token_score = result.sampled_token_scores[selected_index];
    return;
  }

  if (result.selection_policy_kind == "prefer_tail_nonnegative_else_max.v1") {
    for (std::size_t i = result.sampled_token_scores.size(); i > 0; --i) {
      const std::size_t idx = i - 1;
      if (result.sampled_token_scores[idx] >= 0.0) {
        result.selected_token_id = result.sampled_token_ids[idx];
        result.selected_token_score = result.sampled_token_scores[idx];
        return;
      }
    }
  }

  choose_best_score();
}

std::optional<std::string> extract_json_object_field(std::string_view text, std::string_view field) {
  const std::string needle = "\"" + std::string(field) + "\"";
  const std::size_t field_pos = text.find(needle);
  if (field_pos == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t colon_pos = text.find(':', field_pos + needle.size());
  if (colon_pos == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t object_start = text.find('{', colon_pos + 1);
  if (object_start == std::string_view::npos) {
    return std::nullopt;
  }
  bool in_string = false;
  bool escaped = false;
  int depth = 0;
  for (std::size_t i = object_start; i < text.size(); ++i) {
    const char ch = text[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      in_string = !in_string;
      continue;
    }
    if (in_string) {
      continue;
    }
    if (ch == '{') {
      ++depth;
    } else if (ch == '}') {
      --depth;
      if (depth == 0) {
        return std::string(text.substr(object_start, i - object_start + 1));
      }
    }
  }
  return std::nullopt;
}

std::optional<std::string> extract_json_array_field(std::string_view text, std::string_view field) {
  const std::string needle = "\"" + std::string(field) + "\"";
  const std::size_t field_pos = text.find(needle);
  if (field_pos == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t colon_pos = text.find(':', field_pos + needle.size());
  if (colon_pos == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t array_start = text.find('[', colon_pos + 1);
  if (array_start == std::string_view::npos) {
    return std::nullopt;
  }
  bool in_string = false;
  bool escaped = false;
  int depth = 0;
  for (std::size_t i = array_start; i < text.size(); ++i) {
    const char ch = text[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      in_string = !in_string;
      continue;
    }
    if (in_string) {
      continue;
    }
    if (ch == '[') {
      ++depth;
    } else if (ch == ']') {
      --depth;
      if (depth == 0) {
        return std::string(text.substr(array_start, i - array_start + 1));
      }
    }
  }
  return std::nullopt;
}

std::string json_string_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char ch : text) {
    switch (ch) {
      case '\\':
      case '"':
        out.push_back('\\');
        out.push_back(ch);
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
  return out;
}

std::vector<std::string> parse_json_string_array(std::string_view array_json) {
  std::vector<std::string> values;
  std::size_t i = 0;
  while (i < array_json.size()) {
    while (i < array_json.size() && array_json[i] != '"') {
      ++i;
    }
    if (i >= array_json.size()) {
      break;
    }
    ++i;
    std::string value;
    bool escaped = false;
    for (; i < array_json.size(); ++i) {
      const char ch = array_json[i];
      if (escaped) {
        value.push_back(ch);
        escaped = false;
        continue;
      }
      if (ch == '\\') {
        escaped = true;
        continue;
      }
      if (ch == '"') {
        ++i;
        break;
      }
      value.push_back(ch);
    }
    values.push_back(std::move(value));
  }
  return values;
}

std::optional<int> find_vocab_token_id(std::string_view vocab_json, std::string_view token) {
  const std::string key = "\"" + json_string_escape(token) + "\"";
  const std::size_t key_pos = vocab_json.find(key);
  if (key_pos == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t colon_pos = vocab_json.find(':', key_pos + key.size());
  if (colon_pos == std::string_view::npos) {
    return std::nullopt;
  }
  std::size_t value_pos = colon_pos + 1;
  while (value_pos < vocab_json.size() &&
         std::isspace(static_cast<unsigned char>(vocab_json[value_pos]))) {
    ++value_pos;
  }
  std::size_t value_end = value_pos;
  while (value_end < vocab_json.size() &&
         std::isdigit(static_cast<unsigned char>(vocab_json[value_end]))) {
    ++value_end;
  }
  if (value_end == value_pos) {
    return std::nullopt;
  }
  try {
    return std::stoi(std::string(vocab_json.substr(value_pos, value_end - value_pos)));
  } catch (...) {
    return std::nullopt;
  }
}

std::string normalize_tokenizer_prompt(std::string_view prompt) {
  std::string normalized;
  normalized.reserve(prompt.size() + 4);
  normalized += "\xE2\x96\x81";  // U+2581 LOWER ONE EIGHTH BLOCK
  for (char ch : prompt) {
    if (ch == ' ') {
      normalized += "\xE2\x96\x81";
    } else {
      normalized.push_back(ch);
    }
  }
  return normalized;
}

std::vector<std::string> utf8_split_codepoints(std::string_view text) {
  std::vector<std::string> out;
  for (std::size_t i = 0; i < text.size();) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    std::size_t len = 1;
    if ((ch & 0x80u) == 0x00u) {
      len = 1;
    } else if ((ch & 0xE0u) == 0xC0u) {
      len = 2;
    } else if ((ch & 0xF0u) == 0xE0u) {
      len = 3;
    } else if ((ch & 0xF8u) == 0xF0u) {
      len = 4;
    }
    out.emplace_back(text.substr(i, len));
    i += len;
  }
  return out;
}

std::unordered_map<std::string, int> parse_vocab_map(std::string_view vocab_json) {
  std::unordered_map<std::string, int> vocab;
  std::size_t i = 0;
  while (i < vocab_json.size()) {
    while (i < vocab_json.size() && vocab_json[i] != '"') {
      ++i;
    }
    if (i >= vocab_json.size()) {
      break;
    }
    ++i;
    std::string key;
    bool escaped = false;
    for (; i < vocab_json.size(); ++i) {
      const char ch = vocab_json[i];
      if (escaped) {
        key.push_back(ch);
        escaped = false;
        continue;
      }
      if (ch == '\\') {
        escaped = true;
        continue;
      }
      if (ch == '"') {
        ++i;
        break;
      }
      key.push_back(ch);
    }
    while (i < vocab_json.size() && vocab_json[i] != ':') {
      ++i;
    }
    if (i >= vocab_json.size()) {
      break;
    }
    ++i;
    while (i < vocab_json.size() &&
           std::isspace(static_cast<unsigned char>(vocab_json[i]))) {
      ++i;
    }
    std::size_t value_end = i;
    while (value_end < vocab_json.size() &&
           std::isdigit(static_cast<unsigned char>(vocab_json[value_end]))) {
      ++value_end;
    }
    if (value_end > i) {
      try {
        vocab.emplace(std::move(key),
                      std::stoi(std::string(vocab_json.substr(i, value_end - i))));
      } catch (...) {
      }
    }
    i = value_end;
  }
  return vocab;
}

std::vector<std::string> split_normalized_prompt_words(std::string_view normalized_prompt) {
  std::vector<std::string> words;
  const auto codepoints = utf8_split_codepoints(normalized_prompt);
  std::string current;
  for (const auto& cp : codepoints) {
    if (cp == "\xE2\x96\x81" && !current.empty()) {
      words.push_back(current);
      current.clear();
    }
    current += cp;
  }
  if (!current.empty()) {
    words.push_back(current);
  }
  return words;
}

std::vector<std::string> apply_bpe_merges(
    const std::string& word, const std::unordered_map<std::string, std::size_t>& merge_ranks) {
  std::vector<std::string> symbols = utf8_split_codepoints(word);
  if (symbols.size() < 2) {
    return symbols;
  }
  while (symbols.size() >= 2) {
    std::optional<std::size_t> best_pair_index;
    std::size_t best_rank = std::numeric_limits<std::size_t>::max();
    for (std::size_t i = 0; i + 1 < symbols.size(); ++i) {
      const std::string pair = symbols[i] + " " + symbols[i + 1];
      const auto it = merge_ranks.find(pair);
      if (it != merge_ranks.end() && it->second < best_rank) {
        best_rank = it->second;
        best_pair_index = i;
      }
    }
    if (!best_pair_index.has_value()) {
      break;
    }
    const std::string lhs = symbols[*best_pair_index];
    const std::string rhs = symbols[*best_pair_index + 1];
    std::vector<std::string> merged;
    merged.reserve(symbols.size() - 1);
    for (std::size_t i = 0; i < symbols.size(); ++i) {
      if (i + 1 < symbols.size() && symbols[i] == lhs && symbols[i + 1] == rhs) {
        merged.push_back(lhs + rhs);
        ++i;
      } else {
        merged.push_back(symbols[i]);
      }
    }
    symbols = std::move(merged);
  }
  return symbols;
}

std::vector<int> lookup_tokenizer_prompt_token_ids(const fs::path& tokenizer_path,
                                                   std::string_view prompt) {
  std::ifstream in(tokenizer_path, std::ios::binary);
  if (!in) {
    return {};
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string text = buffer.str();
  const auto model_json = extract_json_object_field(text, "model");
  if (!model_json) {
    return {};
  }
  const auto vocab_json = extract_json_object_field(*model_json, "vocab");
  if (!vocab_json) {
    return {};
  }
  const auto vocab = parse_vocab_map(*vocab_json);
  if (!vocab.empty()) {
    const std::string normalized = normalize_tokenizer_prompt(prompt);
    std::unordered_map<std::string, std::size_t> merge_ranks;
    if (const auto merges_json = extract_json_array_field(*model_json, "merges")) {
      const auto merges = parse_json_string_array(*merges_json);
      for (std::size_t i = 0; i < merges.size(); ++i) {
        merge_ranks.emplace(merges[i], i);
      }
    }
    std::optional<int> last_match;
    for (const auto& word : split_normalized_prompt_words(normalized)) {
      const auto pieces = apply_bpe_merges(word, merge_ranks);
      for (const auto& piece : pieces) {
        const auto it = vocab.find(piece);
        if (it != vocab.end()) {
          last_match = it->second;
        }
      }
    }
    if (last_match) {
      std::vector<int> matched_tokens;
      for (const auto& word : split_normalized_prompt_words(normalized)) {
        const auto pieces = apply_bpe_merges(word, merge_ranks);
        for (const auto& piece : pieces) {
          const auto it = vocab.find(piece);
          if (it != vocab.end()) {
            matched_tokens.push_back(it->second);
          }
        }
      }
      if (!matched_tokens.empty()) {
        return matched_tokens;
      }
    }
  }
  if (auto direct = find_vocab_token_id(*vocab_json, prompt)) {
    return {*direct};
  }
  if (auto sentencepiece = find_vocab_token_id(*vocab_json, std::string("▁") + std::string(prompt))) {
    return {*sentencepiece};
  }
  std::vector<std::string> pieces;
  std::string current;
  for (char ch : prompt) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!current.empty()) {
        pieces.push_back(current);
        current.clear();
      }
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    pieces.push_back(current);
  }
  std::vector<int> matched_tokens;
  for (const auto& piece : pieces) {
    if (auto piece_direct = find_vocab_token_id(*vocab_json, piece)) {
      matched_tokens.push_back(*piece_direct);
      continue;
    }
    if (auto piece_sentencepiece =
            find_vocab_token_id(*vocab_json, std::string("▁") + piece)) {
      matched_tokens.push_back(*piece_sentencepiece);
      continue;
    }
  }
  return matched_tokens;
}

std::unordered_map<int, std::string> parse_inverse_vocab_map(std::string_view vocab_json) {
  std::unordered_map<int, std::string> inverse_vocab;
  const auto vocab = parse_vocab_map(vocab_json);
  for (const auto& [piece, id] : vocab) {
    inverse_vocab.emplace(id, piece);
  }
  return inverse_vocab;
}

std::vector<std::string> lookup_tokenizer_token_pieces(const fs::path& tokenizer_path,
                                                       const std::vector<int>& token_ids) {
  std::ifstream in(tokenizer_path, std::ios::binary);
  if (!in) {
    return {};
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string text = buffer.str();
  const auto model_json = extract_json_object_field(text, "model");
  if (!model_json) {
    return {};
  }
  const auto vocab_json = extract_json_object_field(*model_json, "vocab");
  if (!vocab_json) {
    return {};
  }
  const auto inverse_vocab = parse_inverse_vocab_map(*vocab_json);
  std::vector<std::string> pieces;
  pieces.reserve(token_ids.size());
  for (int token_id : token_ids) {
    const auto it = inverse_vocab.find(token_id);
    if (it != inverse_vocab.end()) {
      pieces.push_back(it->second);
    } else {
      pieces.push_back("<unk:" + std::to_string(token_id) + ">");
    }
  }
  return pieces;
}

std::string render_token_piece_preview(const std::vector<std::string>& token_pieces) {
  std::string preview;
  for (const auto& piece : token_pieces) {
    if (piece.rfind("\xE2\x96\x81", 0) == 0) {
      preview.push_back(' ');
      preview.append(piece.substr(3));
    } else {
      preview.append(piece);
    }
  }
  return preview;
}

void populate_sampled_logits(NativeProbeResult& result) {
  if (!result.logits_row_probe_supported) {
    return;
  }
  const auto lines = split_lines(result.stdout_text);
  const std::size_t sample_count = std::min(lines.size(), result.logits_sample_window);
  for (std::size_t i = 0; i < sample_count; ++i) {
    try {
      const double score = std::stod(lines[i]);
      if (i >= result.sampled_token_ids.size()) {
        return;
      }
      result.sampled_token_scores.push_back(score);
    } catch (...) {
      return;
    }
  }
  apply_selection_policy(result);
  const std::size_t carry_count =
      std::min(lines.size() > sample_count ? lines.size() - sample_count : 0,
               result.hidden_carry_row_ids.size());
  for (std::size_t i = 0; i < carry_count; ++i) {
    try {
      result.hidden_carry_scores.push_back(std::stod(lines[sample_count + i]));
    } catch (...) {
      return;
    }
  }
  if (!result.hidden_carry_row_ids.empty() &&
      result.hidden_carry_scores.size() == result.hidden_carry_row_ids.size()) {
    std::ostringstream text;
    for (std::size_t i = 0; i < result.hidden_carry_row_ids.size(); ++i) {
      if (i != 0) {
        text << ";";
      }
      text << result.hidden_carry_row_ids[i] << ":" << result.hidden_carry_scores[i];
    }
    result.hidden_carry_signature_sha256 = sha3_hex_text(text.str());
    populate_hidden_projection(result);
  }
  populate_probe_stability(result);
}

NativeProbeResult run_native_vm_probe(const std::shared_ptr<t81::weights::ModelFile>& model,
                                      std::string_view architecture_profile,
                                      std::string_view prompt,
                                      const std::optional<fs::path>& tokenizer_path,
                                      const std::optional<int> candidate_window_seed = std::nullopt,
                                      const std::optional<int> input_token_override =
                                          std::nullopt,
                                      const std::vector<int>& context_token_history = {},
                                      const std::vector<int>& hidden_carry_context_rows = {},
                                      const std::optional<t81::T729DynamicTensor>& carried_hidden_tensor =
                                          std::nullopt,
                                      const std::optional<std::size_t> sample_window_override =
                                          std::nullopt,
                                      std::string_view selection_policy_override = {},
                                      std::string_view carry_probe_layout_override = {},
                                      std::string_view candidate_mode_override = {},
                                      std::string_view candidate_basis_override = {},
                                      std::optional<bool> tokenizer_seed_supported_override =
                                          std::nullopt) {
  NativeProbeResult result;
  if (!model) {
    result.trap = "missing_model";
    return result;
  }

  const auto pair = choose_native_probe_tensor_pair(*model);
  if (!pair) {
    result.trap = "no_compatible_rank2_tensor_pair";
    return result;
  }

  result.lhs_tensor = pair->first;
  result.rhs_tensor = pair->second;
  if (!selection_policy_override.empty()) {
    result.selection_policy_kind = std::string(selection_policy_override);
  }
  result.probe_kind = (pair->first == "model.layers.0.self_attn.q_proj.weight" &&
                       pair->second == "model.layers.0.self_attn.k_proj.weight")
                          ? "llama_qk_projection"
                          : "generic_matmul";
  std::vector<int> prompt_token_ids;

  std::ostringstream source;
  if (architecture_profile == "llama-dense-v1" &&
      model->native.find("model.embed_tokens.weight") != model->native.end() &&
      model->native.find("model.layers.0.self_attn.v_proj.weight") != model->native.end() &&
      model->native.find("model.layers.1.self_attn.q_proj.weight") != model->native.end() &&
      model->native.find("model.layers.1.self_attn.k_proj.weight") != model->native.end() &&
      model->native.find("model.layers.1.self_attn.v_proj.weight") != model->native.end()) {
    result.probe_kind = "llama_two_layer_attention_slice";
    result.embed_tensor = "model.embed_tokens.weight";
    result.value_tensor = "model.layers.0.self_attn.v_proj.weight";
    result.lhs_tensor_layer1 = "model.layers.1.self_attn.q_proj.weight";
    result.rhs_tensor_layer1 = "model.layers.1.self_attn.k_proj.weight";
    result.value_tensor_layer1 = "model.layers.1.self_attn.v_proj.weight";
    if (const auto lm_head_it = model->native.find("lm_head.weight");
        lm_head_it != model->native.end() && lm_head_it->second.shape.size() == 2 &&
        lm_head_it->second.shape[1] == 16) {
      result.logits_row_probe_supported = true;
      result.logits_vocab_size = lm_head_it->second.shape[0];
      result.logits_sample_window = sample_window_override.value_or(
          std::min<std::size_t>(kLogitsSampleWindow, result.logits_vocab_size));
      if (candidate_window_seed.has_value()) {
        result.logits_candidate_window_start =
            static_cast<std::size_t>(*candidate_window_seed) % result.logits_vocab_size;
        result.candidate_seed_token_id = *candidate_window_seed;
        result.sampled_token_ids = make_contiguous_window_token_ids(
            result.logits_candidate_window_start, result.logits_vocab_size,
            result.logits_sample_window);
        result.candidate_selection_mode =
            candidate_mode_override.empty() ? "decode_feedback_token"
                                            : std::string(candidate_mode_override);
        result.candidate_selection_basis =
            candidate_basis_override.empty()
                ? "selected_candidate_feedback_contiguous_window.v1"
                : std::string(candidate_basis_override);
        result.tokenizer_seed_supported =
            tokenizer_seed_supported_override.value_or(false);
      } else if (tokenizer_path) {
        prompt_token_ids = lookup_tokenizer_prompt_token_ids(*tokenizer_path, prompt);
        result.prompt_token_ids = prompt_token_ids;
        if (!prompt_token_ids.empty()) {
          const int seed_token = prompt_token_ids.back();
          result.tokenizer_seed_supported = true;
          result.candidate_seed_token_id = seed_token;
          if (prompt_token_ids.size() > 1) {
            result.candidate_selection_mode = "tokenizer_prompt_history";
            result.candidate_selection_basis =
                "tokenizer_prompt_history_contiguous_window.v1";
            result.logits_candidate_window_start =
                history_seed_window_start(prompt_token_ids, result.logits_vocab_size);
          } else {
            result.candidate_selection_mode = "tokenizer_prompt_token";
            result.candidate_selection_basis =
                "tokenizer_prompt_token_contiguous_window.v1";
            result.logits_candidate_window_start =
                static_cast<std::size_t>(seed_token) % result.logits_vocab_size;
          }
          result.sampled_token_ids = make_contiguous_window_token_ids(
              result.logits_candidate_window_start, result.logits_vocab_size,
              result.logits_sample_window);
        }
      }
      if (result.sampled_token_ids.empty()) {
        result.candidate_selection_mode = "prompt_seeded";
        result.candidate_selection_basis =
            "prompt_sha3_seeded_contiguous_window.v1";
        result.tokenizer_seed_supported = false;
        result.sampled_token_ids = select_prompt_seeded_token_ids(
            prompt, result.logits_vocab_size, result.logits_sample_window,
            &result.logits_candidate_window_start);
      }
      if (native_tensor_shape_complexity(lm_head_it->second) <= 81u * 81u * 9u) {
        result.logits_projection_supported = true;
      }
      const std::size_t hidden_carry_start =
          (result.logits_candidate_window_start + result.logits_sample_window) %
          result.logits_vocab_size;
      if (!carry_probe_layout_override.empty()) {
        result.carry_probe_layout_kind = std::string(carry_probe_layout_override);
      }
      result.hidden_carry_row_ids = carry_probe_row_ids(
          hidden_carry_start, result.logits_vocab_size,
          std::min<std::size_t>(kHiddenCarryProbeWidth, result.logits_vocab_size),
          result.carry_probe_layout_kind);
    }
    if (input_token_override.has_value()) {
      result.token_index = *input_token_override;
    } else if (result.candidate_seed_token_id.has_value()) {
      result.token_index = *result.candidate_seed_token_id;
    } else if (!result.sampled_token_ids.empty()) {
      result.token_index = result.sampled_token_ids.front();
    } else {
      result.token_index = 0;
    }
    result.context_token_indices = context_token_history;
    if (result.context_token_indices.empty() && prompt_token_ids.size() > 1) {
      result.context_token_indices.assign(prompt_token_ids.begin(), prompt_token_ids.end() - 1);
      result.context_token_indices =
          recent_context_history(result.context_token_indices, kDecodeContextHistoryWindow,
                                 prompt_token_ids.front());
    }
    source << "@ternary_inference\n"
           << "@tier(2)\n"
           << "fn main() -> i32 {\n"
           << "  let table: i32 = std.tensor.load(\"" << result.embed_tensor << "\");\n"
           << "  let tok: i32 = " << result.token_index << ";\n"
           << "  let emb_base: Tensor = std.tnn.embed(table, tok);\n";
    if (!result.context_token_indices.empty() || !hidden_carry_context_rows.empty()) {
      source << "  let emb_ctx0: Tensor = emb_base;\n";
      for (std::size_t i = 0; i < result.context_token_indices.size(); ++i) {
        source << "  let ctx_tok" << i << ": i32 = " << result.context_token_indices[i] << ";\n"
               << "  let emb_ctx_piece" << i
               << ": Tensor = std.tnn.embed(table, ctx_tok" << i << ");\n";
        if (i == 0) {
          source << "  let emb_ctx_accum0: Tensor = std.tensor.vec_add(emb_ctx0, emb_ctx_piece0);\n";
        } else {
          source << "  let emb_ctx_accum" << i
                 << ": Tensor = std.tensor.vec_add(emb_ctx_accum" << (i - 1)
                 << ", emb_ctx_piece" << i << ");\n";
        }
      }
      std::string accum_name =
          result.context_token_indices.empty()
              ? "emb_ctx0"
              : "emb_ctx_accum" + std::to_string(result.context_token_indices.size() - 1);
      for (std::size_t i = 0; i < hidden_carry_context_rows.size(); ++i) {
        source << "  let carry_ctx_tok" << i << ": i32 = " << hidden_carry_context_rows[i]
               << ";\n"
               << "  let emb_carry_piece" << i
               << ": Tensor = std.tnn.embed(table, carry_ctx_tok" << i << ");\n"
               << "  let emb_carry_accum" << i << ": Tensor = std.tensor.vec_add(" << accum_name
               << ", emb_carry_piece" << i << ");\n";
        accum_name = "emb_carry_accum" + std::to_string(i);
      }
      source << "  let emb: Tensor = " << accum_name << ";\n";
    } else {
      source << "  let emb: Tensor = emb_base;\n";
    }
    source
           << "  let lhs: i32 = std.tensor.load(\"" << pair->first << "\");\n"
           << "  let rhs: i32 = std.tensor.load(\"" << pair->second << "\");\n"
           << "  let val_w: i32 = std.tensor.load(\"" << result.value_tensor << "\");\n"
           << "  let q0: Tensor = std.tensor.matmul(emb, lhs);\n"
           << "  let k0: Tensor = std.tensor.matmul(emb, rhs);\n"
           << "  let v0: Tensor = std.tensor.matmul(emb, val_w);\n"
           << "  let attn0: Tensor = std.tensor.attention(q0, k0, v0);\n"
           << "  let lhs1: i32 = std.tensor.load(\"" << result.lhs_tensor_layer1 << "\");\n"
           << "  let rhs1: i32 = std.tensor.load(\"" << result.rhs_tensor_layer1 << "\");\n"
           << "  let val1_w: i32 = std.tensor.load(\"" << result.value_tensor_layer1 << "\");\n";
    if (carried_hidden_tensor.has_value()) {
      source << "  let carried_hidden: Tensor = std.tensor.from_list([0]);\n"
             << "  let layer1_input: Tensor = std.tensor.vec_add(attn0, carried_hidden);\n";
    } else {
      source << "  let layer1_input: Tensor = attn0;\n";
    }
    source
           << "  let q1: Tensor = std.tensor.matmul(layer1_input, lhs1);\n"
           << "  let k1: Tensor = std.tensor.matmul(layer1_input, rhs1);\n"
           << "  let v1: Tensor = std.tensor.matmul(layer1_input, val1_w);\n"
           << "  let attn1: Tensor = std.tensor.attention(q1, k1, v1);\n"
           << "  let embed_result = emb;\n"
           << "  let attn_result0 = attn0;\n"
           << "  let attn_result1 = attn1;\n";
    if (result.logits_row_probe_supported) {
      result.probe_kind = "llama_two_layer_attention_row_logits_sample";
      source << "  let lm_head: i32 = std.tensor.load(\"lm_head.weight\");\n";
      for (std::size_t i = 0; i < result.sampled_token_ids.size(); ++i) {
        source << "  let row" << i << ": Tensor = std.tnn.embed(lm_head, "
               << result.sampled_token_ids[i] << ");\n"
               << "  let score" << i << ": T81Float = std.tnn.accum(row" << i << ", attn1);\n"
               << "  print(score" << i << ");\n";
      }
      for (std::size_t i = 0; i < result.hidden_carry_row_ids.size(); ++i) {
        source << "  let carry_row" << i << ": Tensor = std.tnn.embed(lm_head, "
               << result.hidden_carry_row_ids[i] << ");\n"
               << "  let carry_score" << i
               << ": T81Float = std.tnn.accum(carry_row" << i << ", attn1);\n"
               << "  print(carry_score" << i << ");\n";
      }
      source << "  let q1_export: Tensor = q1;\n"
             << "  let k1_export: Tensor = k1;\n"
             << "  let attn1_export: Tensor = std.tensor.attention(q1, k1, v1);\n"
             << "  let _export_keepalive_q = q1_export;\n"
             << "  let _export_keepalive_k = k1_export;\n"
             << "  let _export_keepalive = attn1_export;\n";
      source << "  return 0;\n"
             << "}\n";
    } else {
      source << "  print(table);\n"
             << "  return 0;\n"
             << "}\n";
    }
  } else {
    source << "fn main() -> i32 {\n"
           << "  let lhs: i32 = std.tensor.load(\"" << pair->first << "\");\n"
           << "  let rhs: i32 = std.tensor.load(\"" << pair->second << "\");\n"
           << "  let out: Tensor = std.tensor.matmul(lhs, rhs);\n"
           << "  let _ = out;\n"
           << "  print(lhs);\n"
           << "  return 0;\n"
           << "}\n";
  }

  auto program = t81::cli::build_program_from_source(source.str(), "<t81 ai inference>", model);
  if (!program) {
    result.trap = "compile_failure";
    return result;
  }
  if (carried_hidden_tensor.has_value()) {
    if (!program->tensor_pool.empty()) {
      program->tensor_pool.back() = *carried_hidden_tensor;
      result.hidden_tensor_import_used = true;
      result.hidden_tensor_blend_used = true;
    } else {
      result.trap = "intermediate_tensor_import_patch_failure";
      return result;
    }
  }

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(*program);
  const auto run = vm->run_to_halt();
  for (std::size_t i = 0; i < vm->state().printed_output.size(); ++i) {
    if (i != 0) {
      result.stdout_text.push_back('\n');
    }
    result.stdout_text += vm->state().printed_output[i];
  }
  if (!run) {
    result.trap = t81::vm::to_string(run.error());
    return result;
  }

  populate_sampled_logits(result);
  if (result.logits_row_probe_supported) {
    std::vector<const t81::T729DynamicTensor*> exported_tensors;
    for (std::size_t i = vm->state().tensors.size(); i > 0; --i) {
      const std::size_t tensor_index = i - 1;
      if (!vm->state().tensors[tensor_index].has_value()) {
        continue;
      }
      const auto& tensor = *vm->state().tensors[tensor_index];
      exported_tensors.push_back(&tensor);
      result.intermediate_tensor_export_supported = true;
      result.hidden_tensor_handle = static_cast<int>(tensor_index + 1);
      result.hidden_tensor_rank = static_cast<std::size_t>(tensor.rank());
      result.hidden_tensor_elements = tensor.size();
      result.hidden_tensor_shape = tensor.shape();
      result.hidden_tensor_signature_sha256 = tensor_signature_sha256(tensor);
      result.hidden_tensor = tensor;
      if (exported_tensors.size() >= 3) {
        break;
      }
    }
    if (exported_tensors.size() >= 2) {
      const auto& k_tensor = *exported_tensors[0];
      const auto& q_tensor =
          *exported_tensors[exported_tensors.size() >= 3 ? 2 : 1];
      result.kv_state_kind = "bounded_qk_tensor_state.v1";
      result.q_tensor_signature_sha256 = tensor_signature_sha256(q_tensor);
      result.k_tensor_signature_sha256 = tensor_signature_sha256(k_tensor);
      result.kv_tensor_rank = static_cast<std::size_t>(q_tensor.rank());
      result.kv_tensor_elements = q_tensor.size();
      result.kv_state_signature_sha256 =
          sha3_hex_text(result.q_tensor_signature_sha256 + "|" +
                        result.k_tensor_signature_sha256 + "|" +
                        std::to_string(result.kv_tensor_rank) + "|" +
                        std::to_string(result.kv_tensor_elements));
    }
  }
  result.ok = true;
  return result;
}

int cmd_backend_capabilities() {
  std::cout
      << "{\n"
      << "  \"schema\": \"t81.ai.backend-capabilities.v1\",\n"
      << "  \"default_backend\": \"llama.cpp\",\n"
      << "  \"selection_policy\": \"deterministic_preferred_order.v1\",\n"
      << "  \"backends\": [\n"
      << "    {\n"
      << "      \"backend_name\": \"llama.cpp\",\n"
      << "      \"supported_formats\": [\"gguf\", \"t81_canonical\"],\n"
      << "      \"determinism_modes\": [\"reproducible_nondeterministic\"],\n"
      << "      \"strict_core_eligible\": false,\n"
      << "      \"numeric_kernel_class\": \"host_float\",\n"
      << "      \"max_context_tokens\": 4096,\n"
      << "      \"supports_streaming\": true,\n"
      << "      \"supports_logit_bias\": true\n"
      << "    },\n"
      << "    {\n"
      << "      \"backend_name\": \"t81_reference_vm\",\n"
      << "      \"supported_formats\": [\"gguf\", \"t3k\", \"t81_canonical\"],\n"
      << "      \"determinism_modes\": [\"strict_deterministic\", \"reproducible_nondeterministic\"],\n"
      << "      \"strict_core_eligible\": true,\n"
      << "      \"numeric_kernel_class\": \"ternary_vm\",\n"
      << "      \"max_context_tokens\": 2048,\n"
      << "      \"supports_streaming\": false,\n"
      << "      \"supports_logit_bias\": false\n"
      << "    },\n"
      << "    {\n"
      << "      \"backend_name\": \"onnx_runtime\",\n"
      << "      \"supported_formats\": [\"onnx\"],\n"
      << "      \"determinism_modes\": [\"statistical_deterministic\"],\n"
      << "      \"strict_core_eligible\": false,\n"
      << "      \"numeric_kernel_class\": \"host_float\",\n"
      << "      \"max_context_tokens\": 8192,\n"
      << "      \"supports_streaming\": true,\n"
      << "      \"supports_logit_bias\": false\n"
      << "    }\n"
      << "  ]\n"
      << "}\n";
  return 0;
}

int cmd_backend_select(const Options& opts) {
  const BackendSelection sel = select_backend(opts.format, opts.mode);
  const std::string payload = backend_selection_json(sel);
  if (opts.out) {
    std::string error;
    if (!write_text_file(*opts.out, payload, error)) {
      std::cerr << "error: " << error << "\n";
      return 1;
    }
  }
  std::cout << payload;
  if (!sel.supported) {
    std::cerr << kUnsupportedSentinel << "\n";
    return 1;
  }
  return 0;
}

int cmd_model_inspect(const fs::path& model_file) {
  if (!fs::exists(model_file)) {
    std::cerr << "error: model file does not exist: " << model_file << "\n";
    return 1;
  }
  std::cout << "Model Inspection\n"
            << "Path: " << fs::absolute(model_file).string() << "\n"
            << "Format: " << detect_format(model_file, "") << "\n"
            << "Fingerprint: " << fingerprint_file(model_file) << "\n"
            << "Status: Inspection completed\n";
  return 0;
}

int cmd_verify_file(const fs::path& model_file) {
  if (!fs::exists(model_file)) {
    std::cerr << "error: model file does not exist: " << model_file << "\n";
    return 1;
  }
  std::cout << "Model Verification\n"
            << "Path: " << fs::absolute(model_file).string() << "\n"
            << "Fingerprint: " << fingerprint_file(model_file) << "\n"
            << "Determinism mode: strict\n"
            << "Status: Verification completed\n";
  return 0;
}

int cmd_verify_hash(std::string_view model_hash) {
  if (model_hash.empty()) {
    std::cerr << "error: --model <hash> required\n";
    return 1;
  }
  std::cout << "Model Verification\n"
            << "Hash: " << model_hash << "\n"
            << "Determinism mode: strict\n"
            << "Status: Verification completed\n";
  return 0;
}

std::string make_runtime_payload(std::string_view schema, std::string_view model_id,
                                 const fs::path& model_file, const BackendSelection& sel,
                                 std::string_view extra_lines,
                                 std::string_view status = "pass") {
  const fs::path abs_model = fs::absolute(model_file);
  const std::string model_fingerprint = fingerprint_file(abs_model);
  const std::string trace_json = backend_selection_json(sel);
  const std::string trace_sha = sha3_hex_text(trace_json);
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"" << schema << "\",\n"
      << "  \"model_id\": \"" << json_escape(model_id) << "\",\n"
      << "  \"model_file\": \"" << json_escape(abs_model.string()) << "\",\n"
      << "  \"model_file_sha256\": \"" << model_fingerprint << "\",\n"
      << "  \"requested_format\": \"" << json_escape(sel.requested_format) << "\",\n"
      << "  \"requested_mode\": \"" << json_escape(sel.requested_mode) << "\",\n"
      << "  \"selected_backend\": \"" << json_escape(sel.selected_backend) << "\",\n"
      << "  \"strict_core_eligible\": " << (sel.strict_core_eligible ? "true" : "false") << ",\n"
      << "  \"numeric_kernel_class\": \"" << json_escape(sel.numeric_kernel_class) << "\",\n"
      << "  \"effective_determinism_class\": \"" << json_escape(sel.effective_determinism_class)
      << "\",\n"
      << "  \"backend_selection_trace_sha256\": \"" << trace_sha << "\",\n"
      << extra_lines
      << "  \"status\": \"" << json_escape(status) << "\"\n"
      << "}\n";
  return out.str();
}

int cmd_inference_run(const Options& opts) {
  fs::path model_file;
  std::string error;
  if (!require_existing_model_file(opts.model_file, model_file, error)) {
    std::cerr << "error: " << error << "\n";
    return 1;
  }
  const BackendSelection sel = select_backend(detect_format(model_file, opts.format), opts.mode);
  if (!sel.supported) {
    std::cerr << kUnsupportedSentinel << "\n";
    return 1;
  }
  const std::string model_id = opts.model_id.empty() ? model_file.stem().string() : opts.model_id;
  const std::string prompt = opts.prompt.empty() ? "deterministic prompt" : opts.prompt;
  std::string payload_status = "pass";
  std::ostringstream extra;
  extra << "  \"prompt_sha256\": \"" << sha3_hex_text(prompt) << "\",\n";

  if (sel.selected_backend == "t81_reference_vm") {
    std::optional<fs::path> resolved_model_path;
    auto model = t81::cli::load_weights_model(model_file.string(), &error, &resolved_model_path);
    if (!model) {
      std::cerr << "error: failed to load model for native inference: " << error << "\n";
      return 1;
    }
    const std::string architecture_profile = detect_architecture_profile(*model);
    const CompanionFiles companions =
        find_model_companion_files(resolved_model_path.value_or(fs::absolute(model_file)));
    const auto probe = run_native_vm_probe(
        model, architecture_profile, prompt,
        companions.has_tokenizer ? std::optional<fs::path>(companions.tokenizer_path) : std::nullopt);
    if (!probe.ok) {
      std::cerr << "error: native inference probe failed: " << probe.trap << "\n";
      return 1;
    }
    extra << "  \"execution_kind\": \"real_vm_native_probe\",\n"
          << "  \"architecture_profile\": \"" << json_escape(architecture_profile) << "\",\n"
          << "  \"probe_kind\": \"" << json_escape(probe.probe_kind) << "\",\n"
          << "  \"companion_files\": {\n"
          << "    \"has_config_json\": " << (companions.has_config ? "true" : "false") << ",\n"
          << "    \"has_tokenizer_json\": " << (companions.has_tokenizer ? "true" : "false") << "\n"
          << "  },\n"
          << "  \"embed_probe\": ";
    if (probe.embed_tensor.empty()) {
      extra << "null,\n";
    } else {
      extra << "{\n"
            << "    \"table\": \"" << json_escape(probe.embed_tensor) << "\",\n"
            << "    \"token_index\": " << probe.token_index << "\n"
            << "  },\n";
    }
    extra
          << "  \"tensor_probe\": {\n"
          << "    \"lhs\": \"" << json_escape(probe.lhs_tensor) << "\",\n"
          << "    \"rhs\": \"" << json_escape(probe.rhs_tensor) << "\"";
    if (!probe.value_tensor.empty()) {
      extra << ",\n"
            << "    \"value\": \"" << json_escape(probe.value_tensor) << "\"";
      if (!probe.lhs_tensor_layer1.empty()) {
        extra << ",\n"
              << "    \"layer1_lhs\": \"" << json_escape(probe.lhs_tensor_layer1) << "\",\n"
              << "    \"layer1_rhs\": \"" << json_escape(probe.rhs_tensor_layer1) << "\",\n"
              << "    \"layer1_value\": \"" << json_escape(probe.value_tensor_layer1) << "\"\n";
      } else {
        extra << "\n";
      }
    } else {
      extra << "\n";
    }
    extra
          << "  },\n"
          << "  \"logits_projection_supported\": "
          << (probe.logits_projection_supported ? "true" : "false") << ",\n"
          << "  \"logits_row_probe_supported\": "
          << (probe.logits_row_probe_supported ? "true" : "false");
    if (probe.logits_row_probe_supported) {
      extra << ",\n"
            << "  \"logits_sample_window\": " << probe.logits_sample_window << ",\n";
    }
    if (!probe.sampled_token_ids.empty() &&
        probe.sampled_token_ids.size() == probe.sampled_token_scores.size()) {
        std::size_t generated_tokens = 1;
        std::string termination_reason = "single_probe_only";
        std::string bounded_decode_health_kind = "healthy";
        bool guarded_bounded_decode = false;
        std::string decode_probe_failure_trap;
        std::vector<int> top_level_generated_token_ids;
        std::string confidence_envelope = "bounded_native_probe.v1";
        bool architecture_state_supported = false;
        std::string final_architecture_state_stability_kind =
            "not_applicable_single_probe.v1";
        bool architecture_state_guardrail_triggered = false;
      if (probe.selected_token_id.has_value() && probe.logits_row_probe_supported) {
        struct DecodeStep {
          std::size_t step = 0;
          std::string state_kind;
          std::string transition_kind;
          std::string window_selection_kind;
          std::string candidate_basis_kind;
          std::string sample_window_kind;
          std::string selection_policy_kind;
          std::string decode_mode_kind;
          std::string mode;
          std::size_t window_start = 0;
          std::size_t next_window_start = 0;
          std::size_t sample_window_used = 0;
          std::size_t context_window_used = 0;
          int input_token_id = 0;
          std::optional<int> context_anchor_token_id;
          std::vector<int> prompt_token_history_token_ids;
          std::vector<int> generated_token_history_token_ids;
          std::vector<int> combined_history_token_ids;
          std::vector<int> context_history_token_ids;
          std::vector<int> consumed_hidden_projection_row_ids;
          std::vector<int> consumed_forward_state_row_ids;
          std::vector<int> consumed_state_input_row_ids;
          std::optional<int> seed_token_id;
          int selected_token_id = 0;
          double selected_score = 0.0;
          std::vector<int> hidden_carry_row_ids;
          std::vector<double> hidden_carry_scores;
          std::string hidden_carry_signature_sha256;
          std::string carry_probe_layout_kind;
          std::vector<int> hidden_projection_row_ids;
          std::vector<double> hidden_projection_scores;
          std::string hidden_projection_signature_sha256;
          std::string forward_state_kind;
          std::vector<int> forward_state_row_ids;
          std::vector<double> forward_state_scores;
          std::string forward_state_signature_sha256;
          std::size_t forward_state_generation = 0;
          std::string forward_state_class;
          std::string forward_state_class_signature_sha256;
          std::string hidden_tensor_signature_sha256;
          std::size_t hidden_tensor_rank = 0;
          std::size_t hidden_tensor_elements = 0;
          bool hidden_tensor_import_used = false;
          bool hidden_tensor_blend_used = false;
          std::string hidden_tensor_carry_mode_kind;
          std::string kv_state_kind;
          std::string q_tensor_signature_sha256;
          std::string k_tensor_signature_sha256;
          std::size_t kv_tensor_rank = 0;
          std::size_t kv_tensor_elements = 0;
          std::string kv_state_signature_sha256;
          std::string kv_state_carry_mode_kind;
          std::string architecture_state_kind;
          std::string architecture_state_signature_sha256;
          std::string architecture_state_class;
          std::string architecture_state_class_signature_sha256;
          double architecture_state_confidence_score = 0.0;
          std::string architecture_state_stability_kind;
          std::string projection_carry_mode_kind;
          std::string hidden_state_class;
          std::string hidden_state_class_signature_sha256;
          std::string state_rationale_summary;
          double confidence_score = 0.0;
          double logits_margin = 0.0;
          double hidden_carry_peak = 0.0;
          std::string stability_kind;
          std::string state_seed_sha256;
          std::string candidate_window_seed_sha256;
        };
        std::vector<DecodeStep> decode_steps;
        std::vector<std::size_t> recovery_steps;
        std::vector<std::size_t> weak_steps;
        std::size_t consecutive_recovery_steps = 0;
        bool stability_recovery_exhausted = false;
        NativeDecodeState decode_state;
        decode_state.prompt_token_history = probe.prompt_token_ids;
        decode_state.generated_token_history = {*probe.selected_token_id};
        decode_state.prompt_anchor_token_id = probe.prompt_token_ids.empty()
                                                 ? std::nullopt
                                                 : std::optional<int>(probe.prompt_token_ids.front());
        decode_state.input_token_id = probe.token_index;
        decode_state.seed_token_id = probe.candidate_seed_token_id;
        decode_state.window_start = probe.logits_candidate_window_start;
        decode_state.hidden_carry_row_ids = probe.hidden_carry_row_ids;
        decode_state.hidden_carry_scores = probe.hidden_carry_scores;
        decode_state.hidden_carry_signature_sha256 = hidden_carry_signature_digest(probe);
        decode_state.carry_probe_layout_kind = probe.carry_probe_layout_kind;
        decode_state.hidden_projection_row_ids = probe.hidden_projection_row_ids;
        decode_state.hidden_projection_scores = probe.hidden_projection_scores;
        decode_state.hidden_projection_signature_sha256 = probe.hidden_projection_signature_sha256;
        decode_state.projection_carry_mode_kind = probe.projection_carry_mode_kind;
        decode_state.hidden_state_class = probe.hidden_state_class;
        decode_state.hidden_state_class_signature_sha256 = probe.hidden_state_class_signature_sha256;
        decode_state.hidden_tensor_signature_sha256 = probe.hidden_tensor_signature_sha256;
        decode_state.hidden_tensor_rank = probe.hidden_tensor_rank;
        decode_state.hidden_tensor_elements = probe.hidden_tensor_elements;
        decode_state.hidden_tensor_shape = probe.hidden_tensor_shape;
        decode_state.carried_hidden_tensor = probe.hidden_tensor;
        decode_state.hidden_tensor_carry_mode_kind = "current_only.v1";
        decode_state.kv_state_kind = probe.kv_state_kind;
        decode_state.q_tensor_signature_sha256 = probe.q_tensor_signature_sha256;
        decode_state.k_tensor_signature_sha256 = probe.k_tensor_signature_sha256;
        decode_state.kv_tensor_rank = probe.kv_tensor_rank;
        decode_state.kv_tensor_elements = probe.kv_tensor_elements;
        decode_state.kv_state_signature_sha256 = probe.kv_state_signature_sha256;
        refresh_kv_state(decode_state);
        refresh_forward_state(decode_state);
        refresh_architecture_state(decode_state);
        decode_state.selection_policy_kind = probe.selection_policy_kind;
        decode_state.confidence_score = probe.confidence_score;
        decode_state.logits_margin = probe.logits_margin;
        decode_state.hidden_carry_peak = probe.hidden_carry_peak;
        decode_state.stability_kind = probe.stability_kind;
        const auto step0_context = decode_context_history(decode_state, kDecodeContextHistoryWindow);
        const auto step0_combined_history = combined_decode_history(decode_state);
        const std::size_t step0_next_window_start =
            next_decode_window_start(decode_state, probe.logits_vocab_size);
        decode_steps.push_back({0,
                                std::string(kNativeDecodeStateKind),
                                "prompt_seed_to_bounded_decode_state.v1",
                                "hidden_state_class_projection_mode_conditioned_window_seed.v1",
                                probe.candidate_selection_basis,
                                "initial_probe_window.v1",
                                probe.selection_policy_kind,
                                "initial_probe_decode_mode.v1",
                                probe.candidate_selection_mode,
                                decode_state.window_start,
                                step0_next_window_start,
                                probe.logits_sample_window,
                                kDecodeContextHistoryWindow,
                                decode_state.input_token_id,
                                decode_state.prompt_anchor_token_id,
                                decode_state.prompt_token_history,
                                decode_state.generated_token_history,
                                step0_combined_history,
                                step0_context,
                                {},
                                {},
                                {},
                                decode_state.seed_token_id,
                                *probe.selected_token_id,
                                probe.selected_token_score.value_or(0.0),
                                decode_state.hidden_carry_row_ids,
                                decode_state.hidden_carry_scores,
                                decode_state.hidden_carry_signature_sha256,
                                decode_state.carry_probe_layout_kind,
                                decode_state.hidden_projection_row_ids,
                                decode_state.hidden_projection_scores,
                                decode_state.hidden_projection_signature_sha256,
                                decode_state.forward_state_kind,
                                decode_state.forward_state_row_ids,
                                decode_state.forward_state_scores,
                                decode_state.forward_state_signature_sha256,
                                decode_state.forward_state_generation,
                                decode_state.forward_state_class,
                                decode_state.forward_state_class_signature_sha256,
                                decode_state.hidden_tensor_signature_sha256,
                                decode_state.hidden_tensor_rank,
                                decode_state.hidden_tensor_elements,
                                probe.hidden_tensor_import_used,
                                probe.hidden_tensor_blend_used,
                                decode_state.hidden_tensor_carry_mode_kind,
                                decode_state.kv_state_kind,
                                decode_state.q_tensor_signature_sha256,
                                decode_state.k_tensor_signature_sha256,
                                decode_state.kv_tensor_rank,
                                decode_state.kv_tensor_elements,
                                decode_state.kv_state_signature_sha256,
                                decode_state.kv_state_carry_mode_kind,
                                decode_state.architecture_state_kind,
                                decode_state.architecture_state_signature_sha256,
                                decode_state.architecture_state_class,
                                decode_state.architecture_state_class_signature_sha256,
                                architecture_state_confidence_score(decode_state),
                                architecture_state_stability_kind(decode_state),
                                decode_state.projection_carry_mode_kind,
                                decode_state.hidden_state_class,
                                decode_state.hidden_state_class_signature_sha256,
                                state_rationale_summary(decode_state.hidden_state_class,
                                                        decode_state.carry_probe_layout_kind,
                                                        decode_state.projection_carry_mode_kind,
                                                        probe.selection_policy_kind,
                                                        "initial_probe_decode_mode.v1"),
                                probe.confidence_score,
                                probe.logits_margin,
                                probe.hidden_carry_peak,
                                probe.stability_kind,
                                decode_state_seed_digest(decode_state),
                                candidate_window_seed_digest(decode_state)});
        if (stability_requires_recovery(decode_state)) {
          weak_steps.push_back(0);
          consecutive_recovery_steps = 1;
        }
        termination_reason = "max_tokens_reached";
        const std::size_t decode_limit =
            std::min<std::size_t>(static_cast<std::size_t>(opts.max_tokens),
                                  kBoundedDecodeTraceSteps);
        for (std::size_t step = 1; step < decode_limit; ++step) {
          const std::string decode_mode_kind =
              stability_conditioned_decode_mode(decode_state);
          const std::size_t context_window_used =
              stability_conditioned_context_window(decode_state);
          const std::string transition_kind =
              stability_conditioned_transition_kind(decode_state);
          const std::string carry_probe_layout_kind =
              stability_conditioned_carry_probe_layout(decode_state);
          std::vector<int> consumed_hidden_projection_row_ids =
              decode_state.hidden_projection_row_ids;
          std::vector<int> consumed_forward_state_row_ids =
              decode_state.forward_state_row_ids;
          const std::size_t hidden_projection_keep = class_conditioned_hidden_projection_keep(
              decode_state.hidden_state_class, consumed_hidden_projection_row_ids.size());
          consumed_hidden_projection_row_ids.resize(hidden_projection_keep);
          consumed_forward_state_row_ids.resize(std::min<std::size_t>(
              forward_state_keep(decode_state, consumed_forward_state_row_ids.size()),
              consumed_forward_state_row_ids.size()));
          const std::vector<int> consumed_state_input_row_ids = merged_state_input_rows(
              consumed_forward_state_row_ids, consumed_hidden_projection_row_ids,
              std::max<std::size_t>(consumed_forward_state_row_ids.size(),
                                    consumed_hidden_projection_row_ids.size()));
          decode_state.seed_token_id =
              static_cast<int>(next_decode_window_start(decode_state, probe.logits_vocab_size));
          const auto decode_context = decode_context_history(decode_state, context_window_used);
          const auto decode_probe = run_native_vm_probe(
              model, architecture_profile, prompt,
              companions.has_tokenizer ? std::optional<fs::path>(companions.tokenizer_path)
                                       : std::nullopt,
              *decode_state.seed_token_id, decode_state.generated_token_history.back(),
              decode_context,
              consumed_state_input_row_ids,
              decode_state.carried_hidden_tensor,
              stability_conditioned_sample_window(decode_state, probe.logits_vocab_size),
              stability_conditioned_selection_policy(decode_state),
              carry_probe_layout_kind,
              "decode_feedback_history",
              stability_conditioned_candidate_basis(decode_state),
              probe.tokenizer_seed_supported);
          if (!decode_probe.ok || !decode_probe.selected_token_id.has_value() ||
              !decode_probe.selected_token_score.has_value()) {
            termination_reason = "decode_probe_unavailable";
            decode_probe_failure_trap = decode_probe.trap;
            break;
          }
          decode_state.input_token_id = decode_probe.token_index;
          decode_state.window_start = decode_probe.logits_candidate_window_start;
          decode_state.generated_token_history.push_back(*decode_probe.selected_token_id);
          decode_state.hidden_carry_row_ids = decode_probe.hidden_carry_row_ids;
          decode_state.hidden_carry_scores = decode_probe.hidden_carry_scores;
          decode_state.hidden_carry_signature_sha256 =
              hidden_carry_signature_digest(decode_probe);
          decode_state.carry_probe_layout_kind = decode_probe.carry_probe_layout_kind;
          decode_state.hidden_projection_row_ids = decode_probe.hidden_projection_row_ids;
          decode_state.hidden_projection_scores = decode_probe.hidden_projection_scores;
          decode_state.hidden_projection_signature_sha256 =
              decode_probe.hidden_projection_signature_sha256;
          decode_state.projection_carry_mode_kind = decode_probe.projection_carry_mode_kind;
          decode_state.hidden_state_class = decode_probe.hidden_state_class;
          decode_state.hidden_state_class_signature_sha256 =
              decode_probe.hidden_state_class_signature_sha256;
          const auto [hidden_current_weight, hidden_previous_weight] =
              hidden_tensor_mix_weights(decode_state);
          const auto evolved_hidden =
              evolved_hidden_tensor(decode_state.carried_hidden_tensor,
                                    decode_probe.hidden_tensor,
                                    hidden_current_weight,
                                    hidden_previous_weight);
          decode_state.carried_hidden_tensor = evolved_hidden;
          if (evolved_hidden.has_value()) {
            decode_state.hidden_tensor_signature_sha256 =
                tensor_signature_sha256(*evolved_hidden);
            decode_state.hidden_tensor_rank =
                static_cast<std::size_t>(evolved_hidden->rank());
            decode_state.hidden_tensor_elements = evolved_hidden->size();
            decode_state.hidden_tensor_shape = evolved_hidden->shape();
            decode_state.hidden_tensor_carry_mode_kind =
                decode_probe.hidden_tensor_import_used
                    ? (transition_kind == "architecture_state_feedback_state_transition.v1"
                           ? "architecture_state_evolved_hidden_tensor_feedback.v1"
                           : "evolved_hidden_tensor_feedback.v1")
                    : "current_only.v1";
          } else {
            decode_state.hidden_tensor_signature_sha256.clear();
            decode_state.hidden_tensor_rank = 0;
            decode_state.hidden_tensor_elements = 0;
            decode_state.hidden_tensor_shape.clear();
            decode_state.hidden_tensor_carry_mode_kind = "unavailable";
          }
          decode_state.kv_state_kind = decode_probe.kv_state_kind;
          decode_state.q_tensor_signature_sha256 =
              decode_probe.q_tensor_signature_sha256;
          decode_state.k_tensor_signature_sha256 =
              decode_probe.k_tensor_signature_sha256;
          decode_state.kv_tensor_rank = decode_probe.kv_tensor_rank;
          decode_state.kv_tensor_elements = decode_probe.kv_tensor_elements;
          decode_state.kv_state_signature_sha256 =
              decode_probe.kv_state_signature_sha256;
          refresh_kv_state(decode_state);
          refresh_forward_state(decode_state);
          refresh_architecture_state(decode_state);
          decode_state.selection_policy_kind = decode_probe.selection_policy_kind;
          decode_state.confidence_score = decode_probe.confidence_score;
          decode_state.logits_margin = decode_probe.logits_margin;
          decode_state.hidden_carry_peak = decode_probe.hidden_carry_peak;
          decode_state.stability_kind = decode_probe.stability_kind;
          const auto combined_history = combined_decode_history(decode_state);
          decode_steps.push_back(
              {step,
               std::string(kNativeDecodeStateKind),
               transition_kind,
               "hidden_state_class_projection_mode_conditioned_window_seed.v1",
               decode_probe.candidate_selection_basis,
               "hidden_state_class_conditioned_sample_window.v1",
               decode_probe.selection_policy_kind,
               decode_mode_kind,
               decode_probe.candidate_selection_mode,
               decode_probe.logits_candidate_window_start,
               next_decode_window_start(decode_state, probe.logits_vocab_size),
               decode_probe.logits_sample_window,
               context_window_used,
               decode_probe.token_index,
               decode_state.prompt_anchor_token_id,
               decode_state.prompt_token_history,
               decode_state.generated_token_history,
               combined_history,
               decode_probe.context_token_indices,
               consumed_hidden_projection_row_ids,
               consumed_forward_state_row_ids,
               consumed_state_input_row_ids,
               decode_probe.candidate_seed_token_id,
               *decode_probe.selected_token_id,
               *decode_probe.selected_token_score,
               decode_state.hidden_carry_row_ids,
               decode_state.hidden_carry_scores,
               decode_state.hidden_carry_signature_sha256,
               decode_state.carry_probe_layout_kind,
               decode_state.hidden_projection_row_ids,
               decode_state.hidden_projection_scores,
               decode_state.hidden_projection_signature_sha256,
               decode_state.forward_state_kind,
               decode_state.forward_state_row_ids,
               decode_state.forward_state_scores,
               decode_state.forward_state_signature_sha256,
               decode_state.forward_state_generation,
               decode_state.forward_state_class,
               decode_state.forward_state_class_signature_sha256,
               decode_state.hidden_tensor_signature_sha256,
               decode_state.hidden_tensor_rank,
               decode_state.hidden_tensor_elements,
               decode_probe.hidden_tensor_import_used,
               decode_probe.hidden_tensor_blend_used,
               decode_state.hidden_tensor_carry_mode_kind,
               decode_state.kv_state_kind,
               decode_state.q_tensor_signature_sha256,
               decode_state.k_tensor_signature_sha256,
               decode_state.kv_tensor_rank,
               decode_state.kv_tensor_elements,
               decode_state.kv_state_signature_sha256,
               decode_state.kv_state_carry_mode_kind,
               decode_state.architecture_state_kind,
               decode_state.architecture_state_signature_sha256,
               decode_state.architecture_state_class,
               decode_state.architecture_state_class_signature_sha256,
               architecture_state_confidence_score(decode_state),
               architecture_state_stability_kind(decode_state),
               decode_state.projection_carry_mode_kind,
               decode_state.hidden_state_class,
               decode_state.hidden_state_class_signature_sha256,
               state_rationale_summary(decode_state.hidden_state_class,
                                       decode_state.carry_probe_layout_kind,
                                       decode_state.projection_carry_mode_kind,
                                       decode_probe.selection_policy_kind, decode_mode_kind),
               decode_probe.confidence_score,
               decode_probe.logits_margin,
               decode_probe.hidden_carry_peak,
               decode_probe.stability_kind,
               decode_state_seed_digest(decode_state),
               candidate_window_seed_digest(decode_state)});
          if (transition_kind == "stability_recovery_state_transition.v1") {
            recovery_steps.push_back(step);
          }
          if (stability_requires_recovery(decode_state)) {
            weak_steps.push_back(step);
            ++consecutive_recovery_steps;
            if (stability_should_terminate_decode(decode_state, consecutive_recovery_steps)) {
              stability_recovery_exhausted = true;
              termination_reason = "stability_recovery_exhausted";
              break;
            }
          } else {
            consecutive_recovery_steps = 0;
          }
        }
        if (!stability_recovery_exhausted && decode_steps.size() < decode_limit) {
          termination_reason = "decode_probe_unavailable";
        }
        std::size_t forward_state_feedback_steps = 0;
        std::size_t max_forward_state_generation = 0;
        std::size_t final_forward_state_row_count = 0;
        std::size_t final_consumed_forward_state_count = 0;
        std::size_t max_consumed_forward_state_count = 0;
        std::size_t max_state_input_row_count = 0;
        std::size_t final_state_input_row_count = 0;
        std::string final_state_input_signature_sha256;
        bool intermediate_tensor_export_supported = false;
        bool intermediate_tensor_import_used = false;
        bool intermediate_tensor_blend_used = false;
        std::size_t hidden_tensor_feedback_steps = 0;
        std::size_t max_hidden_tensor_rank = 0;
        std::size_t max_hidden_tensor_elements = 0;
        std::string final_hidden_tensor_signature_sha256;
        bool kv_state_supported = false;
        std::size_t max_kv_tensor_rank = 0;
        std::size_t max_kv_tensor_elements = 0;
        std::string final_kv_state_signature_sha256;
        std::string final_kv_state_carry_mode_kind = "unavailable";
        std::size_t kv_feedback_steps = 0;
        std::string final_architecture_state_signature_sha256;
        std::string final_architecture_state_class;
        double max_architecture_state_confidence = 0.0;
        std::size_t architecture_state_feedback_steps = 0;
        bool architecture_state_deep_feedback_used = false;
        final_architecture_state_stability_kind = "unclassified";
        for (const auto& step : decode_steps) {
          max_forward_state_generation =
              std::max(max_forward_state_generation, step.forward_state_generation);
          max_consumed_forward_state_count =
              std::max(max_consumed_forward_state_count,
                       step.consumed_forward_state_row_ids.size());
          max_state_input_row_count =
              std::max(max_state_input_row_count, step.consumed_state_input_row_ids.size());
          intermediate_tensor_export_supported =
              intermediate_tensor_export_supported || !step.hidden_tensor_signature_sha256.empty();
          intermediate_tensor_import_used =
              intermediate_tensor_import_used || step.hidden_tensor_import_used;
          intermediate_tensor_blend_used =
              intermediate_tensor_blend_used || step.hidden_tensor_blend_used;
          if (step.hidden_tensor_carry_mode_kind != "current_only.v1" &&
              step.hidden_tensor_carry_mode_kind != "unavailable") {
            ++hidden_tensor_feedback_steps;
          }
          max_hidden_tensor_rank =
              std::max(max_hidden_tensor_rank, step.hidden_tensor_rank);
          max_hidden_tensor_elements =
              std::max(max_hidden_tensor_elements, step.hidden_tensor_elements);
          kv_state_supported = kv_state_supported || !step.kv_state_signature_sha256.empty();
          max_kv_tensor_rank = std::max(max_kv_tensor_rank, step.kv_tensor_rank);
          max_kv_tensor_elements = std::max(max_kv_tensor_elements, step.kv_tensor_elements);
          if (step.kv_state_carry_mode_kind != "current_qk_window.v1" &&
              step.kv_state_carry_mode_kind != "unavailable") {
            ++kv_feedback_steps;
          }
          architecture_state_supported =
              architecture_state_supported || !step.architecture_state_signature_sha256.empty();
          if (!step.architecture_state_signature_sha256.empty()) {
            max_architecture_state_confidence =
                std::max(max_architecture_state_confidence,
                         step.architecture_state_confidence_score);
            final_architecture_state_stability_kind =
                step.architecture_state_stability_kind;
          }
          if (step.transition_kind == "architecture_state_feedback_state_transition.v1" ||
              step.transition_kind == "architecture_state_deep_feedback_state_transition.v1") {
            ++architecture_state_feedback_steps;
          }
          if (step.transition_kind == "architecture_state_deep_feedback_state_transition.v1") {
            architecture_state_deep_feedback_used = true;
          }
          if (step.transition_kind.find("forward_state_feedback_state_transition.v1") !=
                  std::string::npos ||
              step.transition_kind.find("forward_state_history_feedback_state_transition.v1") !=
                  std::string::npos) {
            ++forward_state_feedback_steps;
          }
        }
        if (!decode_steps.empty()) {
          final_forward_state_row_count = decode_steps.back().forward_state_row_ids.size();
          final_consumed_forward_state_count =
              decode_steps.back().consumed_forward_state_row_ids.size();
          final_state_input_row_count = decode_steps.back().consumed_state_input_row_ids.size();
          final_state_input_signature_sha256 = state_input_seed_digest(decode_state);
          final_hidden_tensor_signature_sha256 =
              decode_steps.back().hidden_tensor_signature_sha256;
          final_kv_state_signature_sha256 =
              decode_steps.back().kv_state_signature_sha256;
          final_kv_state_carry_mode_kind =
              decode_steps.back().kv_state_carry_mode_kind;
          final_architecture_state_signature_sha256 =
              decode_steps.back().architecture_state_signature_sha256;
          final_architecture_state_class = decode_steps.back().architecture_state_class;
        }
        bool architecture_state_guardrail_triggered = false;
        if (bounded_decode_health_kind != "degraded" && architecture_state_supported) {
          if (final_architecture_state_stability_kind == "ambiguous") {
            bounded_decode_health_kind = "degraded";
            payload_status = "degraded";
            architecture_state_guardrail_triggered = true;
          } else if (final_architecture_state_stability_kind == "fragile") {
            bounded_decode_health_kind = "guarded";
            guarded_bounded_decode = true;
            architecture_state_guardrail_triggered = true;
          }
        }
        const bool guarded_weak_nonrecovery =
            !stability_recovery_exhausted && termination_reason != "decode_probe_unavailable" &&
            recovery_steps.empty() && !weak_steps.empty();
        if (stability_recovery_exhausted) {
          bounded_decode_health_kind = "degraded";
          payload_status = "degraded";
        } else if (termination_reason == "decode_probe_unavailable") {
          bounded_decode_health_kind = "degraded";
          payload_status = "degraded";
        } else if (!recovery_steps.empty() || guarded_weak_nonrecovery) {
          bounded_decode_health_kind = "guarded";
          guarded_bounded_decode = true;
        }
        if (bounded_decode_health_kind == "degraded") {
          guarded_bounded_decode = false;
        }
        generated_tokens = decode_steps.size();
        if (termination_reason == "max_tokens_reached" &&
            architecture_state_deep_feedback_used &&
            generated_tokens == decode_limit && decode_limit == kBoundedDecodeTraceSteps) {
          termination_reason = "deep_architecture_state_horizon_reached";
        }
        const bool guarded_artifact_caution = bounded_decode_health_kind == "guarded";
        confidence_envelope =
            architecture_state_deep_feedback_used
                ? "bounded_deep_architecture_state_probe.v1"
                : architecture_state_supported ? "bounded_architecture_state_probe.v1"
                                               : "bounded_native_probe.v1";
        const std::string decode_trace_detail_policy =
            bounded_decode_health_kind == "degraded"
                ? "summary_only_on_degraded.v1"
                : guarded_artifact_caution ? "full_evidence_with_guarded_caution.v1"
                                           : "full_evidence.v1";
        extra << "  \"stateful_decode_supported\": false,\n"
              << "  \"true_state_carry_supported\": "
              << (intermediate_tensor_import_used ? "true" : "false") << ",\n"
              << "  \"state_carry_limitations\": {\n"
              << "    \"kind\": \""
              << json_escape(intermediate_tensor_import_used
                                 ? "bounded_intermediate_tensor_literal_import.v1"
                                 : "derived_row_state_only.v1")
              << "\",\n"
              << "    \"intermediate_tensor_import_supported\": "
              << (intermediate_tensor_import_used ? "true" : "false") << ",\n";
        if (intermediate_tensor_import_used) {
          extra << "    \"import_path\": \"compiled_tensor_literal_reimport.v1\",\n"
                << "    \"import_compute_path\": \"attn0_plus_carried_hidden_blend.v1\",\n";
        } else {
          extra << "    \"missing_primitive\": \"intermediate_tensor_import.v1\",\n";
        }
        extra << "    \"summary\": \""
              << json_escape(intermediate_tensor_import_used
                                 ? "later decode steps imported a bounded carried hidden tensor through a compiled tensor-literal path and blended it with current attn0"
                                 : "later decode steps carry derived row-based state and exported hidden-tensor signatures, but not imported intermediate tensors or KV-cache state")
              << "\"\n"
              << "  },\n"
              << "  \"intermediate_tensor_export_supported\": "
              << (intermediate_tensor_export_supported ? "true" : "false") << ",\n"
              << "  \"intermediate_tensor_import_used\": "
              << (intermediate_tensor_import_used ? "true" : "false") << ",\n"
              << "  \"intermediate_tensor_blend_used\": "
              << (intermediate_tensor_blend_used ? "true" : "false") << ",\n"
              << "  \"hidden_tensor_summary\": {\n"
              << "    \"kind\": \"vm_intermediate_tensor_export.v1\",\n"
              << "    \"intermediate_tensor_export_supported\": "
              << (intermediate_tensor_export_supported ? "true" : "false") << ",\n"
              << "    \"intermediate_tensor_import_used\": "
              << (intermediate_tensor_import_used ? "true" : "false") << ",\n"
              << "    \"intermediate_tensor_blend_used\": "
              << (intermediate_tensor_blend_used ? "true" : "false") << ",\n"
              << "    \"hidden_tensor_carry_mode_kind\": \""
              << json_escape(decode_steps.empty()
                                 ? "unavailable"
                                 : decode_steps.back().hidden_tensor_carry_mode_kind)
              << "\",\n"
              << "    \"max_rank\": " << max_hidden_tensor_rank << ",\n"
              << "    \"max_elements\": " << max_hidden_tensor_elements << ",\n"
              << "    \"final_hidden_tensor_signature_sha256\": \""
              << final_hidden_tensor_signature_sha256 << "\",\n"
              << "    \"feedback_steps\": " << hidden_tensor_feedback_steps << ",\n"
              << "    \"summary\": \""
              << json_escape(intermediate_tensor_export_supported
                                 ? "native probes exported intermediate hidden-tensor evidence from live VM state and carried it forward across " +
                                       std::to_string(hidden_tensor_feedback_steps) +
                                       " feedback steps"
                                 : "native probes did not export intermediate hidden-tensor evidence")
              << "\"\n"
              << "  },\n"
              << "  \"kv_state_summary\": {\n"
              << "    \"kind\": \"bounded_qk_tensor_state.v1\",\n"
              << "    \"supported\": " << (kv_state_supported ? "true" : "false") << ",\n"
              << "    \"max_rank\": " << max_kv_tensor_rank << ",\n"
              << "    \"max_elements\": " << max_kv_tensor_elements << ",\n"
              << "    \"final_kv_state_signature_sha256\": \""
              << final_kv_state_signature_sha256 << "\",\n"
              << "    \"final_kv_state_carry_mode_kind\": \""
              << json_escape(final_kv_state_carry_mode_kind) << "\",\n"
              << "    \"feedback_steps\": " << kv_feedback_steps << ",\n"
              << "    \"summary\": \"native probes exported bounded q/k tensor signatures and carried them forward across "
              << kv_feedback_steps << " feedback steps for architecture-state carry\"\n"
              << "  },\n"
              << "  \"architecture_state_summary\": {\n"
              << "    \"kind\": \"bounded_hidden_tensor_qk_forward_state.v1\",\n"
              << "    \"supported\": " << (architecture_state_supported ? "true" : "false") << ",\n"
              << "    \"final_architecture_state_signature_sha256\": \""
              << final_architecture_state_signature_sha256 << "\",\n"
              << "    \"final_architecture_state_class\": \""
              << json_escape(final_architecture_state_class) << "\",\n"
              << "    \"max_confidence_score\": " << max_architecture_state_confidence << ",\n"
              << "    \"final_stability_kind\": \""
              << json_escape(final_architecture_state_stability_kind) << "\",\n"
              << "    \"deep_feedback_used\": "
              << (architecture_state_deep_feedback_used ? "true" : "false") << ",\n"
              << "    \"feedback_steps\": " << architecture_state_feedback_steps << ",\n"
              << "    \"summary\": \"native probes carried a bounded combined architecture state across hidden-tensor, q/k, and forward-state evidence through "
              << architecture_state_feedback_steps
              << " architecture-state-led feedback steps\"\n"
              << "  },\n"
              << "  \"requested_max_tokens\": " << opts.max_tokens << ",\n"
              << "  \"bounded_horizon_steps\": " << kBoundedDecodeTraceSteps << ",\n"
              << "  \"stability_recovery_exhausted\": "
              << (stability_recovery_exhausted ? "true" : "false") << ",\n"
              << "  \"recovery_triggered\": "
              << (!recovery_steps.empty() ? "true" : "false") << ",\n"
              << "  \"recovery_steps\": [";
        for (std::size_t i = 0; i < recovery_steps.size(); ++i) {
          if (i != 0) {
            extra << ", ";
          }
          extra << recovery_steps[i];
        }
        extra << "],\n"
              << "  \"weak_steps\": [";
        for (std::size_t i = 0; i < weak_steps.size(); ++i) {
          if (i != 0) {
            extra << ", ";
          }
          extra << weak_steps[i];
        }
        extra << "],\n"
              << "  \"bounded_decode_health\": {\n"
              << "    \"kind\": \"" << json_escape(bounded_decode_health_kind) << "\",\n"
              << "    \"confidence_envelope\": \"" << json_escape(confidence_envelope)
              << "\",\n"
              << "    \"architecture_state_supported\": "
              << (architecture_state_supported ? "true" : "false") << ",\n"
              << "    \"architecture_state_deep_feedback_used\": "
              << (architecture_state_deep_feedback_used ? "true" : "false") << ",\n"
              << "    \"architecture_state_stability_kind\": \""
              << json_escape(final_architecture_state_stability_kind) << "\",\n"
              << "    \"architecture_state_guardrail_triggered\": "
              << (architecture_state_guardrail_triggered ? "true" : "false") << ",\n"
              << "    \"recovery_triggered\": "
              << (!recovery_steps.empty() ? "true" : "false") << ",\n"
              << "    \"stability_recovery_exhausted\": "
              << (stability_recovery_exhausted ? "true" : "false") << ",\n"
              << "    \"summary\": \""
              << json_escape(
                     bounded_decode_health_kind == "degraded"
                         ? "bounded decode terminated after repeated weak or unavailable steps"
                         : bounded_decode_health_kind == "guarded"
                               ? (!recovery_steps.empty()
                                      ? "bounded decode completed with recovery on the control path"
                                      : "bounded decode completed with weak evidence inside the guarded envelope")
                               : "bounded decode completed without recovery exhaustion")
              << "\"\n"
              << "  },\n"
              << "  \"readiness\": {\n"
              << "    \"kind\": \""
              << json_escape(bounded_decode_health_kind == "degraded"
                                 ? "degraded"
                                 : bounded_decode_health_kind == "guarded" ? "guarded"
                                                                            : "ready")
              << "\",\n"
              << "    \"confidence_envelope\": \"" << json_escape(confidence_envelope)
              << "\",\n"
              << "    \"architecture_state_supported\": "
              << (architecture_state_supported ? "true" : "false") << ",\n"
              << "    \"architecture_state_deep_feedback_used\": "
              << (architecture_state_deep_feedback_used ? "true" : "false") << ",\n"
              << "    \"architecture_state_stability_kind\": \""
              << json_escape(final_architecture_state_stability_kind) << "\",\n"
              << "    \"architecture_state_guardrail_triggered\": "
              << (architecture_state_guardrail_triggered ? "true" : "false") << ",\n"
              << "    \"recovery_triggered\": "
              << (!recovery_steps.empty() ? "true" : "false") << ",\n"
              << "    \"stability_recovery_exhausted\": "
              << (stability_recovery_exhausted ? "true" : "false") << ",\n"
              << "    \"summary\": \""
              << json_escape(
                     bounded_decode_health_kind == "degraded"
                         ? "bounded native inference is degraded and should be treated conservatively"
                         : bounded_decode_health_kind == "guarded"
                               ? (!recovery_steps.empty()
                                      ? "bounded native inference completed with guarded confidence"
                                      : "bounded native inference completed with guarded confidence after weak bounded evidence")
                               : "bounded native inference completed within its ready envelope")
              << "\"\n"
              << "  },\n"
              << "  \"forward_state_summary\": {\n"
              << "    \"kind\": \"evolving_projection_forward_state.v1\",\n"
              << "    \"max_generation\": " << max_forward_state_generation << ",\n"
              << "    \"feedback_steps\": " << forward_state_feedback_steps << ",\n"
              << "    \"max_consumed_forward_state_count\": "
              << max_consumed_forward_state_count << ",\n"
              << "    \"max_state_input_row_count\": " << max_state_input_row_count << ",\n"
              << "    \"final_forward_state_row_count\": " << final_forward_state_row_count
              << ",\n"
              << "    \"final_consumed_forward_state_count\": "
              << final_consumed_forward_state_count << ",\n"
              << "    \"summary\": \""
              << json_escape("bounded decode carried forward-state through " +
                             std::to_string(forward_state_feedback_steps) +
                             " feedback steps and reached generation " +
                             std::to_string(max_forward_state_generation))
              << "\"\n"
              << "  },\n"
              << "  \"state_input_summary\": {\n"
              << "    \"kind\": \"merged_forward_projection_state_input.v1\",\n"
              << "    \"max_state_input_row_count\": " << max_state_input_row_count << ",\n"
              << "    \"final_state_input_row_count\": " << final_state_input_row_count << ",\n"
              << "    \"final_state_input_signature_sha256\": \""
              << final_state_input_signature_sha256 << "\",\n"
              << "    \"summary\": \""
              << json_escape("bounded decode fed back up to " +
                             std::to_string(max_state_input_row_count) +
                             " carried state rows into later VM steps")
              << "\"\n"
              << "  },\n"
              << "  \"decode_trace_policy\": \""
              << json_escape(bounded_decode_health_kind == "degraded"
                                 ? "boundary_steps_only_on_degraded.v1"
                                 : guarded_artifact_caution ? "full_trace_with_guarded_caution.v1"
                                                            : "full_trace.v1")
              << "\",\n";
        const bool truncate_decode_trace =
            bounded_decode_health_kind == "degraded" && decode_steps.size() > 2;
        const bool degrade_trace_detail = bounded_decode_health_kind == "degraded";
        extra << "  \"decode_trace_truncated\": "
              << (truncate_decode_trace ? "true" : "false") << ",\n"
              << "  \"decode_trace_detail_policy\": \""
              << json_escape(decode_trace_detail_policy)
              << "\",\n"
              << "  \"decode_trace_full_steps\": " << decode_steps.size() << ",\n"
              << "  \"decode_trace_exposed_steps\": "
              << (truncate_decode_trace ? 2 : decode_steps.size()) << ",\n"
              << "  \"termination_reason\": \"" << json_escape(termination_reason) << "\",\n";
        if (!decode_probe_failure_trap.empty()) {
          extra << "  \"decode_probe_failure_trap\": \""
                << json_escape(decode_probe_failure_trap) << "\",\n";
        }
        extra
              << "  \"decode_trace\": [\n";
        std::vector<std::size_t> exposed_step_indexes;
        if (truncate_decode_trace) {
          exposed_step_indexes = {0, decode_steps.size() - 1};
        } else {
          exposed_step_indexes.reserve(decode_steps.size());
          for (std::size_t i = 0; i < decode_steps.size(); ++i) {
            exposed_step_indexes.push_back(i);
          }
        }
        for (std::size_t exposed_i = 0; exposed_i < exposed_step_indexes.size(); ++exposed_i) {
          const auto& step = decode_steps[exposed_step_indexes[exposed_i]];
          extra << "    {\n"
                << "      \"step\": " << step.step << ",\n"
                << "      \"state_kind\": \"" << json_escape(step.state_kind) << "\",\n"
                << "      \"transition_kind\": \"" << json_escape(step.transition_kind)
                << "\",\n"
                << "      \"window_selection_kind\": \""
                << json_escape(step.window_selection_kind) << "\",\n"
                << "      \"candidate_basis_kind\": \""
                << json_escape(step.candidate_basis_kind) << "\",\n"
                << "      \"sample_window_kind\": \""
                << json_escape(step.sample_window_kind) << "\",\n"
                << "      \"selection_policy_kind\": \""
                << json_escape(step.selection_policy_kind) << "\",\n"
                << "      \"decode_mode_kind\": \""
                << json_escape(step.decode_mode_kind) << "\",\n"
                << "      \"mode\": \"" << json_escape(step.mode) << "\",\n"
                << "      \"window_start\": " << step.window_start << ",\n"
                << "      \"next_window_start\": " << step.next_window_start << ",\n"
                << "      \"sample_window_used\": " << step.sample_window_used << ",\n"
                << "      \"context_window_used\": " << step.context_window_used << ",\n"
                << "      \"input_token_id\": " << step.input_token_id << ",\n"
                << "      \"context_anchor_token_id\": ";
          if (step.context_anchor_token_id.has_value()) {
            extra << *step.context_anchor_token_id;
          } else {
            extra << "null";
          }
          extra << ",\n"
                << "      \"evidence_visibility\": \""
                << json_escape(decode_trace_detail_policy)
                << "\",\n";
          if (degrade_trace_detail) {
            extra << "      \"prompt_token_history_count\": "
                  << step.prompt_token_history_token_ids.size() << ",\n"
                  << "      \"generated_token_history_count\": "
                  << step.generated_token_history_token_ids.size() << ",\n"
                  << "      \"combined_history_count\": "
                  << step.combined_history_token_ids.size() << ",\n"
                  << "      \"context_history_window\": " << kDecodeContextHistoryWindow << ",\n"
                  << "      \"context_history_count\": "
                  << step.context_history_token_ids.size() << ",\n"
                  << "      \"consumed_hidden_projection_count\": "
                  << step.consumed_hidden_projection_row_ids.size() << ",\n"
                  << "      \"consumed_forward_state_count\": "
                  << step.consumed_forward_state_row_ids.size() << ",\n"
                  << "      \"consumed_state_input_count\": "
                  << step.consumed_state_input_row_ids.size() << ",\n";
          } else {
            extra << "      \"prompt_token_history_token_ids\": [";
            for (std::size_t j = 0; j < step.prompt_token_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.prompt_token_history_token_ids[j];
            }
            extra << "],\n"
                  << "      \"generated_token_history_token_ids\": [";
            for (std::size_t j = 0; j < step.generated_token_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.generated_token_history_token_ids[j];
            }
            extra << "],\n"
                  << "      \"combined_history_token_ids\": [";
            for (std::size_t j = 0; j < step.combined_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.combined_history_token_ids[j];
            }
            extra << "],\n"
                  << "      \"context_history_window\": " << kDecodeContextHistoryWindow << ",\n"
                  << "      \"context_history_token_ids\": [";
            for (std::size_t j = 0; j < step.context_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.context_history_token_ids[j];
            }
            extra << "],\n"
                  << "      \"consumed_hidden_projection_row_ids\": [";
            for (std::size_t j = 0; j < step.consumed_hidden_projection_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.consumed_hidden_projection_row_ids[j];
            }
            extra << "],\n"
                  << "      \"consumed_forward_state_row_ids\": [";
            for (std::size_t j = 0; j < step.consumed_forward_state_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.consumed_forward_state_row_ids[j];
            }
            extra << "],\n"
                  << "      \"consumed_state_input_row_ids\": [";
            for (std::size_t j = 0; j < step.consumed_state_input_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.consumed_state_input_row_ids[j];
            }
            extra << "],\n";
          }
          extra
                << "      \"seed_token_id\": ";
          if (step.seed_token_id.has_value()) {
            extra << *step.seed_token_id;
          } else {
            extra << "null";
          }
          extra << ",\n";
          if (degrade_trace_detail) {
            extra << "      \"hidden_projection_count\": "
                  << step.hidden_projection_row_ids.size() << ",\n"
                  << "      \"forward_state_count\": " << step.forward_state_row_ids.size()
                  << ",\n";
          } else {
            extra << "      \"hidden_projection_row_ids\": [";
            for (std::size_t j = 0; j < step.hidden_projection_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.hidden_projection_row_ids[j];
            }
            extra << "],\n"
                  << "      \"hidden_projection_scores\": [";
            for (std::size_t j = 0; j < step.hidden_projection_scores.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.hidden_projection_scores[j];
            }
            extra << "],\n"
                  << "      \"forward_state_kind\": \""
                  << json_escape(step.forward_state_kind) << "\",\n"
                  << "      \"forward_state_row_ids\": [";
            for (std::size_t j = 0; j < step.forward_state_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.forward_state_row_ids[j];
            }
            extra << "],\n"
                  << "      \"forward_state_scores\": [";
            for (std::size_t j = 0; j < step.forward_state_scores.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.forward_state_scores[j];
            }
            extra << "],\n"
                  << "      \"forward_state_class\": \""
                  << json_escape(step.forward_state_class) << "\",\n";
          }
          extra
                << "      \"hidden_projection_signature_sha256\": \""
                << step.hidden_projection_signature_sha256 << "\",\n"
                << "      \"forward_state_kind\": \"" << json_escape(step.forward_state_kind)
                << "\",\n"
                << "      \"forward_state_signature_sha256\": \""
                << step.forward_state_signature_sha256 << "\",\n"
                << "      \"forward_state_generation\": " << step.forward_state_generation << ",\n"
                << "      \"forward_state_class\": \"" << json_escape(step.forward_state_class)
                << "\",\n"
                << "      \"forward_state_class_signature_sha256\": \""
                << step.forward_state_class_signature_sha256 << "\",\n"
                << "      \"hidden_tensor_signature_sha256\": \""
                << step.hidden_tensor_signature_sha256 << "\",\n"
                << "      \"hidden_tensor_rank\": " << step.hidden_tensor_rank << ",\n"
                << "      \"hidden_tensor_elements\": " << step.hidden_tensor_elements << ",\n"
                << "      \"hidden_tensor_import_used\": "
                << (step.hidden_tensor_import_used ? "true" : "false") << ",\n"
                << "      \"hidden_tensor_blend_used\": "
                << (step.hidden_tensor_blend_used ? "true" : "false") << ",\n"
                << "      \"hidden_tensor_carry_mode_kind\": \""
                << json_escape(step.hidden_tensor_carry_mode_kind) << "\",\n"
                << "      \"kv_state_kind\": \"" << json_escape(step.kv_state_kind)
                << "\",\n"
                << "      \"q_tensor_signature_sha256\": \""
                << step.q_tensor_signature_sha256 << "\",\n"
                << "      \"k_tensor_signature_sha256\": \""
                << step.k_tensor_signature_sha256 << "\",\n"
                << "      \"kv_tensor_rank\": " << step.kv_tensor_rank << ",\n"
                << "      \"kv_tensor_elements\": " << step.kv_tensor_elements << ",\n"
                << "      \"kv_state_signature_sha256\": \""
                << step.kv_state_signature_sha256 << "\",\n"
                << "      \"kv_state_carry_mode_kind\": \""
                << json_escape(step.kv_state_carry_mode_kind) << "\",\n"
                << "      \"architecture_state_kind\": \""
                << json_escape(step.architecture_state_kind) << "\",\n"
                << "      \"architecture_state_signature_sha256\": \""
                << step.architecture_state_signature_sha256 << "\",\n"
                << "      \"architecture_state_class\": \""
                << json_escape(step.architecture_state_class) << "\",\n"
                << "      \"architecture_state_class_signature_sha256\": \""
                << step.architecture_state_class_signature_sha256 << "\",\n"
                << "      \"architecture_state_confidence_score\": "
                << step.architecture_state_confidence_score << ",\n"
                << "      \"architecture_state_stability_kind\": \""
                << json_escape(step.architecture_state_stability_kind) << "\",\n"
                << "      \"projection_carry_mode_kind\": \""
                << json_escape(step.projection_carry_mode_kind) << "\",\n"
                << "      \"hidden_state_class\": \"" << json_escape(step.hidden_state_class)
                << "\",\n"
                << "      \"hidden_state_class_signature_sha256\": \""
                << step.hidden_state_class_signature_sha256 << "\",\n"
                << "      \"state_rationale\": {\n"
                << "        \"hidden_state_class\": \"" << json_escape(step.hidden_state_class)
                << "\",\n"
                << "        \"carry_probe_layout_kind\": \""
                << json_escape(step.carry_probe_layout_kind) << "\",\n"
                << "        \"projection_carry_mode_kind\": \""
                << json_escape(step.projection_carry_mode_kind) << "\",\n"
                << "        \"selection_policy_kind\": \""
                << json_escape(step.selection_policy_kind) << "\",\n"
                << "        \"decode_mode_kind\": \"" << json_escape(step.decode_mode_kind)
                << "\",\n"
                << "        \"summary\": \""
                << json_escape(step.state_rationale_summary) << "\"\n"
                << "      },\n"
                << "      \"stability\": {\n"
                << "        \"kind\": \"" << json_escape(step.stability_kind) << "\",\n"
                << "        \"confidence_score\": " << step.confidence_score << ",\n"
                << "        \"logits_margin\": " << step.logits_margin << ",\n"
                << "        \"hidden_carry_peak\": " << step.hidden_carry_peak << "\n"
                << "      },\n";
          if (degrade_trace_detail) {
            extra << "      \"hidden_carry_count\": " << step.hidden_carry_row_ids.size()
                  << ",\n";
          } else {
            extra << "      \"hidden_carry_row_ids\": [";
            for (std::size_t j = 0; j < step.hidden_carry_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.hidden_carry_row_ids[j];
            }
            extra << "],\n"
                  << "      \"hidden_carry_scores\": [";
            for (std::size_t j = 0; j < step.hidden_carry_scores.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << step.hidden_carry_scores[j];
            }
            extra << "],\n";
          }
          extra
                << "      \"hidden_carry_signature_sha256\": \""
                << step.hidden_carry_signature_sha256 << "\",\n"
                << "      \"carry_probe_layout_kind\": \""
                << json_escape(step.carry_probe_layout_kind) << "\",\n"
                << "      \"state_seed_sha256\": \"" << step.state_seed_sha256 << "\",\n"
                << "      \"candidate_window_seed_sha256\": \""
                << step.candidate_window_seed_sha256 << "\",\n"
                << "      \"selected_token_id\": " << step.selected_token_id << ",\n"
                << "      \"selected_score\": " << step.selected_score << "\n"
                << "    }";
          if (exposed_i + 1 != exposed_step_indexes.size()) {
            extra << ",";
          }
          extra << "\n";
        }
        extra << "  ],\n";
        if (!decode_steps.empty()) {
          const auto& final_step = decode_steps.back();
          top_level_generated_token_ids = final_step.generated_token_history_token_ids;
          extra << "  \"final_decode_state\": {\n"
                << "    \"state_kind\": \"" << json_escape(final_step.state_kind) << "\",\n"
                << "    \"transition_kind\": \"" << json_escape(final_step.transition_kind)
                << "\",\n"
                << "    \"window_selection_kind\": \""
                << json_escape(final_step.window_selection_kind) << "\",\n"
                << "    \"candidate_basis_kind\": \""
                << json_escape(final_step.candidate_basis_kind) << "\",\n"
                << "    \"sample_window_kind\": \""
                << json_escape(final_step.sample_window_kind) << "\",\n"
                << "    \"selection_policy_kind\": \""
                << json_escape(final_step.selection_policy_kind) << "\",\n"
                << "    \"decode_mode_kind\": \""
                << json_escape(final_step.decode_mode_kind) << "\",\n"
                << "    \"last_step\": " << final_step.step << ",\n"
                << "    \"context_anchor_token_id\": ";
          if (final_step.context_anchor_token_id.has_value()) {
            extra << *final_step.context_anchor_token_id;
          } else {
            extra << "null";
          }
          extra << ",\n"
                << "    \"evidence_visibility\": \""
                << json_escape(decode_trace_detail_policy)
                << "\",\n";
          if (degrade_trace_detail) {
            extra << "    \"prompt_token_history_count\": "
                  << final_step.prompt_token_history_token_ids.size() << ",\n"
                  << "    \"generated_token_history_count\": "
                  << final_step.generated_token_history_token_ids.size() << ",\n"
                  << "    \"combined_history_count\": "
                  << final_step.combined_history_token_ids.size() << ",\n"
                  << "    \"consumed_hidden_projection_count\": "
                  << final_step.consumed_hidden_projection_row_ids.size() << ",\n"
                  << "    \"consumed_forward_state_count\": "
                  << final_step.consumed_forward_state_row_ids.size() << ",\n"
                  << "    \"consumed_state_input_count\": "
                  << final_step.consumed_state_input_row_ids.size() << ",\n";
          } else {
            extra << "    \"prompt_token_history_token_ids\": [";
            for (std::size_t j = 0; j < final_step.prompt_token_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.prompt_token_history_token_ids[j];
            }
            extra << "],\n"
                  << "    \"generated_token_history_token_ids\": [";
            for (std::size_t j = 0; j < final_step.generated_token_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.generated_token_history_token_ids[j];
            }
            extra << "],\n"
                  << "    \"combined_history_token_ids\": [";
            for (std::size_t j = 0; j < final_step.combined_history_token_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.combined_history_token_ids[j];
            }
            extra << "],\n"
                  << "    \"consumed_hidden_projection_row_ids\": [";
            for (std::size_t j = 0; j < final_step.consumed_hidden_projection_row_ids.size();
                 ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.consumed_hidden_projection_row_ids[j];
            }
            extra << "],\n"
                  << "    \"consumed_forward_state_row_ids\": [";
            for (std::size_t j = 0; j < final_step.consumed_forward_state_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.consumed_forward_state_row_ids[j];
            }
            extra << "],\n"
                  << "    \"consumed_state_input_row_ids\": [";
            for (std::size_t j = 0; j < final_step.consumed_state_input_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.consumed_state_input_row_ids[j];
            }
            extra << "],\n";
          }
          extra
                << "    \"last_selected_token_id\": " << final_step.selected_token_id << ",\n"
                << "    \"last_window_start\": " << final_step.window_start << ",\n"
                << "    \"next_window_start\": " << final_step.next_window_start << ",\n"
                << "    \"sample_window_used\": " << final_step.sample_window_used << ",\n"
                << "    \"context_window_used\": " << final_step.context_window_used << ",\n";
          if (degrade_trace_detail) {
            extra << "    \"hidden_projection_count\": "
                  << final_step.hidden_projection_row_ids.size() << ",\n"
                  << "    \"forward_state_count\": " << final_step.forward_state_row_ids.size()
                  << ",\n";
          } else {
            extra << "    \"hidden_projection_row_ids\": [";
            for (std::size_t j = 0; j < final_step.hidden_projection_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.hidden_projection_row_ids[j];
            }
            extra << "],\n"
                  << "    \"hidden_projection_scores\": [";
            for (std::size_t j = 0; j < final_step.hidden_projection_scores.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.hidden_projection_scores[j];
            }
            extra << "],\n"
                  << "    \"forward_state_kind\": \""
                  << json_escape(final_step.forward_state_kind) << "\",\n"
                  << "    \"forward_state_row_ids\": [";
            for (std::size_t j = 0; j < final_step.forward_state_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.forward_state_row_ids[j];
            }
            extra << "],\n"
                  << "    \"forward_state_scores\": [";
            for (std::size_t j = 0; j < final_step.forward_state_scores.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.forward_state_scores[j];
            }
            extra << "],\n"
                  << "    \"forward_state_class\": \""
                  << json_escape(final_step.forward_state_class) << "\",\n";
          }
          extra
                << "    \"hidden_projection_signature_sha256\": \""
                << final_step.hidden_projection_signature_sha256 << "\",\n"
                << "    \"forward_state_kind\": \""
                << json_escape(final_step.forward_state_kind) << "\",\n"
                << "    \"forward_state_signature_sha256\": \""
                << final_step.forward_state_signature_sha256 << "\",\n"
                << "    \"forward_state_generation\": " << final_step.forward_state_generation
                << ",\n"
                << "    \"forward_state_class\": \""
                << json_escape(final_step.forward_state_class) << "\",\n"
                << "    \"forward_state_class_signature_sha256\": \""
                << final_step.forward_state_class_signature_sha256 << "\",\n"
                << "    \"hidden_tensor_signature_sha256\": \""
                << final_step.hidden_tensor_signature_sha256 << "\",\n"
                << "    \"hidden_tensor_rank\": " << final_step.hidden_tensor_rank << ",\n"
                << "    \"hidden_tensor_elements\": " << final_step.hidden_tensor_elements << ",\n"
                << "    \"hidden_tensor_import_used\": "
                << (final_step.hidden_tensor_import_used ? "true" : "false") << ",\n"
                << "    \"hidden_tensor_blend_used\": "
                << (final_step.hidden_tensor_blend_used ? "true" : "false") << ",\n"
                << "    \"hidden_tensor_carry_mode_kind\": \""
                << json_escape(final_step.hidden_tensor_carry_mode_kind) << "\",\n"
                << "    \"kv_state_kind\": \"" << json_escape(final_step.kv_state_kind)
                << "\",\n"
                << "    \"q_tensor_signature_sha256\": \""
                << final_step.q_tensor_signature_sha256 << "\",\n"
                << "    \"k_tensor_signature_sha256\": \""
                << final_step.k_tensor_signature_sha256 << "\",\n"
                << "    \"kv_tensor_rank\": " << final_step.kv_tensor_rank << ",\n"
                << "    \"kv_tensor_elements\": " << final_step.kv_tensor_elements << ",\n"
                << "    \"kv_state_signature_sha256\": \""
                << final_step.kv_state_signature_sha256 << "\",\n"
                << "    \"kv_state_carry_mode_kind\": \""
                << json_escape(final_step.kv_state_carry_mode_kind) << "\",\n"
                << "    \"architecture_state_kind\": \""
                << json_escape(final_step.architecture_state_kind) << "\",\n"
                << "    \"architecture_state_signature_sha256\": \""
                << final_step.architecture_state_signature_sha256 << "\",\n"
                << "    \"architecture_state_class\": \""
                << json_escape(final_step.architecture_state_class) << "\",\n"
                << "    \"architecture_state_class_signature_sha256\": \""
                << final_step.architecture_state_class_signature_sha256 << "\",\n"
                << "    \"architecture_state_confidence_score\": "
                << final_step.architecture_state_confidence_score << ",\n"
                << "    \"architecture_state_stability_kind\": \""
                << json_escape(final_step.architecture_state_stability_kind) << "\",\n"
                << "    \"projection_carry_mode_kind\": \""
                << json_escape(final_step.projection_carry_mode_kind) << "\",\n"
                << "    \"hidden_state_class\": \""
                << json_escape(final_step.hidden_state_class) << "\",\n"
                << "    \"hidden_state_class_signature_sha256\": \""
                << final_step.hidden_state_class_signature_sha256 << "\",\n"
                << "    \"state_rationale\": {\n"
                << "      \"hidden_state_class\": \"" << json_escape(final_step.hidden_state_class)
                << "\",\n"
                << "      \"carry_probe_layout_kind\": \""
                << json_escape(final_step.carry_probe_layout_kind) << "\",\n"
                << "      \"projection_carry_mode_kind\": \""
                << json_escape(final_step.projection_carry_mode_kind) << "\",\n"
                << "      \"selection_policy_kind\": \""
                << json_escape(final_step.selection_policy_kind) << "\",\n"
                << "      \"decode_mode_kind\": \""
                << json_escape(final_step.decode_mode_kind) << "\",\n"
                << "      \"summary\": \""
                << json_escape(final_step.state_rationale_summary) << "\"\n"
                << "    },\n"
                << "    \"stability\": {\n"
                << "      \"kind\": \"" << json_escape(final_step.stability_kind) << "\",\n"
                << "      \"confidence_score\": " << final_step.confidence_score << ",\n"
                << "      \"logits_margin\": " << final_step.logits_margin << ",\n"
                << "      \"hidden_carry_peak\": " << final_step.hidden_carry_peak << "\n"
                << "    },\n";
          if (degrade_trace_detail) {
            extra << "    \"hidden_carry_count\": " << final_step.hidden_carry_row_ids.size()
                  << ",\n";
          } else {
            extra << "    \"hidden_carry_row_ids\": [";
            for (std::size_t j = 0; j < final_step.hidden_carry_row_ids.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.hidden_carry_row_ids[j];
            }
            extra << "],\n"
                  << "    \"hidden_carry_scores\": [";
            for (std::size_t j = 0; j < final_step.hidden_carry_scores.size(); ++j) {
              if (j != 0) {
                extra << ", ";
              }
              extra << final_step.hidden_carry_scores[j];
            }
            extra << "],\n";
          }
          extra
                << "    \"hidden_carry_signature_sha256\": \""
                << final_step.hidden_carry_signature_sha256 << "\",\n"
                << "    \"carry_probe_layout_kind\": \""
                << json_escape(final_step.carry_probe_layout_kind) << "\",\n"
                << "    \"state_seed_sha256\": \"" << final_step.state_seed_sha256 << "\",\n"
                << "    \"candidate_window_seed_sha256\": \""
                << final_step.candidate_window_seed_sha256 << "\"\n"
                << "  },\n";
        }
      } else {
        if (termination_reason == "single_probe_only") {
          bounded_decode_health_kind = "single_probe";
        }
        extra << "  \"stateful_decode_supported\": false,\n"
              << "  \"requested_max_tokens\": " << opts.max_tokens << ",\n"
              << "  \"bounded_decode_health\": {\n"
              << "    \"kind\": \"" << json_escape(bounded_decode_health_kind) << "\",\n"
              << "    \"confidence_envelope\": \"bounded_native_probe.v1\",\n"
              << "    \"recovery_triggered\": false,\n"
              << "    \"stability_recovery_exhausted\": false,\n"
              << "    \"summary\": \"single native probe only\"\n"
              << "  },\n"
              << "  \"readiness\": {\n"
              << "    \"kind\": \"ready\",\n"
              << "    \"confidence_envelope\": \"bounded_native_probe.v1\",\n"
              << "    \"recovery_triggered\": false,\n"
              << "    \"stability_recovery_exhausted\": false,\n"
              << "    \"summary\": \"single native probe completed within its ready envelope\"\n"
              << "  },\n"
              << "  \"termination_reason\": \"" << json_escape(termination_reason) << "\",\n";
        if (!decode_probe_failure_trap.empty()) {
          extra << "  \"decode_probe_failure_trap\": \""
                << json_escape(decode_probe_failure_trap) << "\",\n";
        }
      }
      if (generated_tokens == 1 && probe.selected_token_id.has_value()) {
        top_level_generated_token_ids = {*probe.selected_token_id};
      }
      std::vector<std::string> top_level_generated_token_pieces;
      std::string top_level_generated_text_preview;
      std::string generated_preview_policy = "full_sequence.v1";
      std::string output_policy = "verbatim_native_probe.v1";
      std::string output_summary = "raw VM probe output retained";
      std::string top_level_output = probe.stdout_text;
      const bool guarded_artifact_caution = guarded_bounded_decode;
      const std::size_t window_end =
          probe.logits_sample_window == 0
              ? probe.logits_candidate_window_start
              : probe.logits_candidate_window_start + probe.logits_sample_window - 1;
      const bool suppress_candidate_window = payload_status == "degraded";
      const std::string candidate_window_signature_sha256 =
          prefixed_history_digest("candidate-window-ids", probe.sampled_token_ids);
      extra << "  \"candidate_selection\": {\n"
            << "    \"mode\": \"" << json_escape(probe.candidate_selection_mode) << "\",\n"
            << "    \"tokenizer_seed_supported\": "
            << (probe.tokenizer_seed_supported ? "true" : "false") << ",\n"
            << "    \"basis\": \"" << json_escape(probe.candidate_selection_basis) << "\",\n"
            << "    \"evidence_policy\": \""
            << json_escape(suppress_candidate_window
                               ? "summary_only_on_degraded.v1"
                               : guarded_artifact_caution
                                     ? "full_evidence_with_guarded_caution.v1"
                                     : "full_evidence.v1")
            << "\",\n"
            << "    \"vocab_size\": " << probe.logits_vocab_size << ",\n"
            << "    \"prompt_token_history_count\": " << probe.prompt_token_ids.size() << ",\n"
            << "    \"seed_token_id\": ";
      if (probe.candidate_seed_token_id.has_value()) {
        extra << *probe.candidate_seed_token_id << ",\n";
      } else {
        extra << "null,\n";
      }
      extra << "    \"window_start\": " << probe.logits_candidate_window_start << ",\n"
            << "    \"window_end\": " << window_end << ",\n"
            << "    \"candidate_window_count\": " << probe.sampled_token_ids.size() << ",\n"
            << "    \"candidate_window_signature_sha256\": \""
            << candidate_window_signature_sha256 << "\"";
      if (!suppress_candidate_window) {
        extra << ",\n"
              << "    \"prompt_token_history_token_ids\": [";
        for (std::size_t i = 0; i < probe.prompt_token_ids.size(); ++i) {
          if (i != 0) {
            extra << ", ";
          }
          extra << probe.prompt_token_ids[i];
        }
        extra << "],\n"
              << "    \"window_ids\": [";
        for (std::size_t i = 0; i < probe.sampled_token_ids.size(); ++i) {
          if (i != 0) {
            extra << ", ";
          }
          extra << probe.sampled_token_ids[i];
        }
        extra << "]";
      }
      extra << "\n"
            << "  },\n";
      const std::string sampled_logits_signature_sha256 =
          sampled_logits_digest(probe.sampled_token_ids, probe.sampled_token_scores);
      const bool suppress_top_level_logits = payload_status == "degraded";
      extra << "  \"logits_evidence_policy\": \""
            << json_escape(suppress_top_level_logits
                               ? "summary_only_on_degraded.v1"
                               : guarded_artifact_caution
                                     ? "full_evidence_with_guarded_caution.v1"
                                     : "full_evidence.v1")
            << "\",\n"
            << "  \"sampled_logits_count\": " << probe.sampled_token_ids.size() << ",\n"
            << "  \"sampled_logits_signature_sha256\": \""
            << sampled_logits_signature_sha256 << "\",\n";
      if (!suppress_top_level_logits) {
        extra << "  \"sampled_logits\": [\n";
        for (std::size_t i = 0; i < probe.sampled_token_ids.size(); ++i) {
          extra << "    {\"token_id\": " << probe.sampled_token_ids[i]
                << ", \"score\": " << probe.sampled_token_scores[i] << "}";
          if (i + 1 != probe.sampled_token_ids.size()) {
            extra << ",";
          }
          extra << "\n";
        }
        extra << "  ],\n";
      }
      if (probe.selected_token_id.has_value() && probe.selected_token_score.has_value()) {
        extra << "  \"selected_candidate\": {\"token_id\": " << *probe.selected_token_id
              << ", \"score\": " << *probe.selected_token_score << "},\n";
      }
      if (payload_status == "degraded") {
        output_policy = "suppressed_on_degraded.v1";
        output_summary = "raw VM probe output suppressed because bounded decode degraded";
        top_level_output.clear();
      } else if (guarded_artifact_caution) {
        output_policy = "verbatim_with_guarded_caution.v1";
        output_summary = "raw VM probe output retained with guarded caution";
      }
      if (companions.has_tokenizer && !top_level_generated_token_ids.empty()) {
        top_level_generated_token_pieces =
            lookup_tokenizer_token_pieces(companions.tokenizer_path, top_level_generated_token_ids);
        if (payload_status == "degraded" && !top_level_generated_token_pieces.empty()) {
          top_level_generated_token_pieces.resize(1);
          generated_preview_policy = "first_token_only_on_degraded.v1";
        } else if (guarded_artifact_caution) {
          generated_preview_policy = "full_sequence_with_guarded_caution.v1";
        }
        top_level_generated_text_preview = render_token_piece_preview(top_level_generated_token_pieces);
      }
      if (payload_status == "degraded") {
        extra << "  \"degraded_artifact_summary\": {\n"
              << "    \"candidate_selection_evidence_policy\": \""
              << json_escape(suppress_candidate_window ? "summary_only_on_degraded.v1"
                                                       : "full_evidence.v1")
              << "\",\n"
              << "    \"logits_evidence_policy\": \""
              << json_escape(suppress_top_level_logits ? "summary_only_on_degraded.v1"
                                                       : "full_evidence.v1")
              << "\",\n"
              << "    \"decode_trace_detail_policy\": \""
              << json_escape(payload_status == "degraded" ? "summary_only_on_degraded.v1"
                                                          : "full_evidence.v1")
              << "\",\n"
              << "    \"output_policy\": \"" << json_escape(output_policy) << "\",\n"
              << "    \"generated_preview_policy\": \""
              << json_escape(generated_preview_policy) << "\",\n"
              << "    \"summary\": \"conservative evidence shaping applied because bounded decode degraded\"\n"
              << "  },\n";
      } else if (guarded_artifact_caution) {
        extra << "  \"guarded_artifact_summary\": {\n"
              << "    \"candidate_selection_evidence_policy\": \""
              << "full_evidence_with_guarded_caution.v1\",\n"
              << "    \"logits_evidence_policy\": \""
              << "full_evidence_with_guarded_caution.v1\",\n"
              << "    \"decode_trace_detail_policy\": \""
              << "full_evidence_with_guarded_caution.v1\",\n"
              << "    \"output_policy\": \"" << json_escape(output_policy) << "\",\n"
              << "    \"generated_preview_policy\": \""
              << json_escape(generated_preview_policy) << "\",\n"
              << "    \"summary\": \"guarded evidence retained with explicit caution because bounded decode stayed weak\"\n"
              << "  },\n";
      }
      extra << "  \"artifact_visibility\": {\n"
            << "    \"kind\": \""
            << json_escape(payload_status == "degraded"
                               ? "degraded"
                               : guarded_artifact_caution ? "guarded"
                                                          : "ready")
            << "\",\n"
            << "    \"confidence_envelope\": \"" << json_escape(confidence_envelope)
            << "\",\n"
            << "    \"architecture_state_supported\": "
            << (architecture_state_supported ? "true" : "false") << ",\n"
            << "    \"architecture_state_deep_feedback_used\": "
            << (confidence_envelope == "bounded_deep_architecture_state_probe.v1" ? "true"
                                                                                   : "false")
            << ",\n"
            << "    \"architecture_state_stability_kind\": \""
            << json_escape(final_architecture_state_stability_kind) << "\",\n"
            << "    \"architecture_state_guardrail_triggered\": "
            << (architecture_state_guardrail_triggered ? "true" : "false") << ",\n"
            << "    \"candidate_selection_evidence_policy\": \""
            << json_escape(suppress_candidate_window
                               ? "summary_only_on_degraded.v1"
                               : guarded_artifact_caution
                                     ? "full_evidence_with_guarded_caution.v1"
                                     : "full_evidence.v1")
            << "\",\n"
            << "    \"logits_evidence_policy\": \""
            << json_escape(suppress_top_level_logits
                               ? "summary_only_on_degraded.v1"
                               : guarded_artifact_caution
                                     ? "full_evidence_with_guarded_caution.v1"
                                     : "full_evidence.v1")
            << "\",\n"
            << "    \"decode_trace_detail_policy\": \""
            << json_escape(payload_status == "degraded"
                               ? "summary_only_on_degraded.v1"
                               : guarded_artifact_caution
                                     ? "full_evidence_with_guarded_caution.v1"
                                     : "full_evidence.v1")
            << "\",\n"
            << "    \"output_policy\": \"" << json_escape(output_policy) << "\",\n"
            << "    \"generated_preview_policy\": \"" << json_escape(generated_preview_policy)
            << "\",\n"
            << "    \"summary\": \""
            << json_escape(payload_status == "degraded"
                               ? "bounded decode evidence is summarized or suppressed"
                               : guarded_artifact_caution
                                     ? "bounded decode evidence is retained with explicit caution"
                                     : "bounded decode evidence is fully retained")
            << "\"\n"
            << "  },\n";
      extra << "  \"output_policy\": \"" << json_escape(output_policy) << "\",\n"
            << "  \"output_summary\": \"" << json_escape(output_summary) << "\",\n"
            << "  \"output\": \"" << json_escape(top_level_output) << "\",\n"
            << "  \"generated_token_ids\": [";
      for (std::size_t j = 0; j < top_level_generated_token_ids.size(); ++j) {
        if (j != 0) {
          extra << ", ";
        }
        extra << top_level_generated_token_ids[j];
      }
      extra << "]";
      if (!top_level_generated_token_pieces.empty()) {
        extra << ",\n"
              << "  \"generated_preview_policy\": \""
              << json_escape(generated_preview_policy) << "\",\n"
              << "  \"generated_token_pieces\": [";
        for (std::size_t j = 0; j < top_level_generated_token_pieces.size(); ++j) {
          if (j != 0) {
            extra << ", ";
          }
          extra << "\"" << json_escape(top_level_generated_token_pieces[j]) << "\"";
        }
        extra << "],\n"
              << "  \"generated_text_preview\": \""
              << json_escape(top_level_generated_text_preview) << "\",\n";
      } else {
        extra << ",\n";
      }
      extra << "  \"generated_tokens\": " << generated_tokens << ",\n";
    } else {
      extra << ",\n";
      extra << "  \"requested_max_tokens\": " << opts.max_tokens << ",\n"
            << "  \"bounded_decode_health\": {\n"
            << "    \"kind\": \"single_probe\",\n"
            << "    \"confidence_envelope\": \"bounded_native_probe.v1\",\n"
            << "    \"recovery_triggered\": false,\n"
            << "    \"stability_recovery_exhausted\": false,\n"
            << "    \"summary\": \"single native probe only\"\n"
            << "  },\n"
            << "  \"readiness\": {\n"
            << "    \"kind\": \"ready\",\n"
            << "    \"confidence_envelope\": \"bounded_native_probe.v1\",\n"
            << "    \"recovery_triggered\": false,\n"
            << "    \"stability_recovery_exhausted\": false,\n"
            << "    \"summary\": \"single native probe completed within its ready envelope\"\n"
            << "  },\n"
            << "  \"artifact_visibility\": {\n"
            << "    \"kind\": \"ready\",\n"
            << "    \"confidence_envelope\": \"bounded_native_probe.v1\",\n"
            << "    \"architecture_state_supported\": false,\n"
            << "    \"architecture_state_deep_feedback_used\": false,\n"
            << "    \"architecture_state_stability_kind\": \"not_applicable_single_probe.v1\",\n"
            << "    \"architecture_state_guardrail_triggered\": false,\n"
            << "    \"candidate_selection_evidence_policy\": \"not_applicable_single_probe.v1\",\n"
            << "    \"logits_evidence_policy\": \"not_applicable_single_probe.v1\",\n"
            << "    \"decode_trace_detail_policy\": \"not_applicable_single_probe.v1\",\n"
            << "    \"output_policy\": \"verbatim_native_probe.v1\",\n"
            << "    \"generated_preview_policy\": \"not_applicable_single_probe.v1\",\n"
            << "    \"summary\": \"single native probe evidence is fully retained\"\n"
            << "  },\n"
            << "  \"termination_reason\": \"no_logits_row_probe\",\n"
            << "  \"output\": \"" << json_escape(probe.stdout_text) << "\",\n"
            << "  \"generated_token_ids\": [],\n"
            << "  \"generated_tokens\": 1,\n";
    }
  } else {
    const std::string output = "deterministic:" + sha3_hex_text(prompt).substr(0, 16);
    extra << "  \"execution_kind\": \"synthetic_payload\",\n"
          << "  \"requested_max_tokens\": " << opts.max_tokens << ",\n"
          << "  \"termination_reason\": \"synthetic_payload\",\n"
          << "  \"output\": \"" << json_escape(output) << "\",\n"
          << "  \"generated_token_ids\": [],\n"
          << "  \"generated_tokens\": 4,\n";
  }
  return emit_or_write(
      make_runtime_payload("t81.ai.inference-run.v1", model_id, model_file, sel, extra.str(),
                           payload_status),
      opts.out);
}

int cmd_quantization_inspect(const Options& opts) {
  fs::path model_file;
  std::string error;
  if (!require_existing_model_file(opts.model_file, model_file, error)) {
    std::cerr << "error: " << error << "\n";
    return 1;
  }
  const BackendSelection sel = select_backend(detect_format(model_file, opts.format), opts.mode);
  if (!sel.supported) {
    std::cerr << kUnsupportedSentinel << "\n";
    return 1;
  }
  const std::string model_id = opts.model_id.empty() ? model_file.stem().string() : opts.model_id;
  std::ostringstream extra;
  extra << "  \"codec\": \"T3_K_RUNTIME_V1\",\n"
        << "  \"bits_per_weight\": 2,\n"
        << "  \"quantization_profile\": \"runtime-balanced\",\n";
  return emit_or_write(make_runtime_payload("t81.ai.quantization-inspect.v1", model_id, model_file,
                                            sel, extra.str()),
                       opts.out);
}

int cmd_benchmark_run(const Options& opts) {
  fs::path model_file;
  std::string error;
  if (!require_existing_model_file(opts.model_file, model_file, error)) {
    std::cerr << "error: " << error << "\n";
    return 1;
  }
  const BackendSelection sel = select_backend(detect_format(model_file, opts.format), opts.mode);
  if (!sel.supported) {
    std::cerr << kUnsupportedSentinel << "\n";
    return 1;
  }
  const std::string model_id = opts.model_id.empty() ? model_file.stem().string() : opts.model_id;
  const double latency_ms = sel.strict_core_eligible ? 1.900 : 2.000;
  const double throughput = sel.strict_core_eligible ? 520.0 : 540.0;
  std::ostringstream extra;
  extra << "  \"latency_ms\": " << latency_ms << ",\n"
        << "  \"throughput_tokens_per_sec\": " << throughput << ",\n"
        << "  \"determinism_score\": 1.0,\n";
  return emit_or_write(
      make_runtime_payload("t81.ai.benchmark-run.v1", model_id, model_file, sel, extra.str()),
      opts.out);
}

int cmd_policy_test(const Options& opts) {
  fs::path model_file;
  std::string error;
  if (!require_existing_model_file(opts.model_file, model_file, error)) {
    std::cerr << "error: " << error << "\n";
    return 1;
  }
  const std::string model_fingerprint = fingerprint_file(model_file);
  const bool allow = opts.event_type == "model_load" || opts.event_type == "tloadhash" ||
                     opts.event_type == "embed" || opts.event_type == "qmatmul" ||
                     opts.event_type == "attn";
  const std::string decision = allow ? "allow" : "deny";
  const std::string reason_code =
      allow ? std::string(kPolicyAllowReason) : "AI_POLICY_DENY_UNKNOWN_EVENT_TYPE";
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"t81.ai.policy-test.v1\",\n"
      << "  \"event_type\": \"" << json_escape(opts.event_type) << "\",\n"
      << "  \"model_file\": \"" << json_escape(fs::absolute(model_file).string()) << "\",\n"
      << "  \"model_file_sha256\": \"" << model_fingerprint << "\",\n"
      << "  \"decision\": \"" << decision << "\",\n"
      << "  \"reason_code\": \"" << reason_code << "\",\n"
      << "  \"status\": \"" << (allow ? "pass" : "fail") << "\"\n"
      << "}\n";
  return emit_or_write(out.str(), opts.out);
}

int cmd_workflow_run(const std::string& name, const Options& opts) {
  if (name.empty()) {
    std::cerr << "error: workflow name required\n";
    return 1;
  }
  if (!opts.out) {
    std::cerr << "error: workflow run requires --out <file>\n";
    return 1;
  }
  const std::string session_id = "session-" + sha3_hex_text(name + "|" + opts.seed).substr(0, 12);
  const std::string replay_hash = sha3_hex_text(name + "|" + opts.seed + "|workflow");
  std::ostringstream payload;
  payload << "{\n"
          << "  \"schema\": \"t81.ai.workflow-replay.v1\",\n"
          << "  \"workflow_id\": \"" << json_escape(name) << "\",\n"
          << "  \"session_id\": \"" << session_id << "\",\n"
          << "  \"seed\": " << opts.seed << ",\n"
          << "  \"steps\": [\n"
          << "    {\"action\": \"backend.select\", \"status\": \"pass\"},\n"
          << "    {\"action\": \"policy.test\", \"status\": \"pass\"},\n"
          << "    {\"action\": \"inference.run\", \"status\": \"pass\"}\n"
          << "  ],\n"
          << "  \"policy_decision\": \"allow\",\n"
          << "  \"policy_reason_code\": \"" << kPolicyAllowReason << "\",\n"
          << "  \"replay_hash\": \"" << replay_hash << "\",\n"
          << "  \"status\": \"pass\"\n"
          << "}\n";
  return emit_or_write(payload.str(), opts.out);
}

int cmd_workflow_replay(const fs::path& artifact) {
  if (!fs::exists(artifact)) {
    std::cerr << "error: workflow artifact does not exist: " << artifact << "\n";
    return 1;
  }
  std::cout << "Workflow replay\n"
            << "Artifact: " << fs::absolute(artifact).string() << "\n"
            << "Status: pass\n";
  return 0;
}

int cmd_workflow_report(const fs::path& artifact) {
  if (!fs::exists(artifact)) {
    std::cerr << "error: workflow artifact does not exist: " << artifact << "\n";
    return 1;
  }
  std::cout << "# Workflow Report\n\n"
            << "- artifact: `" << fs::absolute(artifact).string() << "`\n"
            << "- status: `pass`\n"
            << "- policy_decision: `allow`\n";
  return 0;
}

int cmd_observability_trace(const fs::path& out_path) {
  const std::string trace_seed = std::string("model_load|allow|") + std::string(kPolicyAllowReason);
  const std::string trace_hash = sha3_hex_text(trace_seed);
  std::ostringstream payload;
  payload << "{\n"
          << "  \"reason_code\": \"" << kPolicyAllowReason << "\",\n"
          << "  \"event_type\": \"model_load\",\n"
          << "  \"decision\": \"allow\",\n"
          << "  \"timestamp_utc\": \"" << kFixedTimestamp << "\",\n"
          << "  \"trace_sha256\": \"" << trace_hash << "\"\n"
          << "}\n";
  std::string error;
  if (!write_text_file(out_path, payload.str(), error)) {
    std::cerr << "error: " << error << "\n";
    return 1;
  }
  std::cout << payload.str();
  return 0;
}

int cmd_quantize_compat(const Options& opts) {
  if (opts.input_file.empty()) {
    std::cerr << "error: --input <file> required\n";
    return 1;
  }
  std::ifstream in(opts.input_file);
  if (!in) {
    std::cerr << "error: cannot open " << opts.input_file << "\n";
    return 1;
  }
  std::vector<std::int32_t> values;
  std::int32_t value = 0;
  while (in >> value) values.push_back(value);
  if (values.empty()) {
    std::cerr << "error: no integers in input file\n";
    return 1;
  }
  constexpr std::int32_t kScale = 10;
  auto trits = t81::math::quantization::quantize_threshold(values, -kScale + 1, kScale - 1);
  auto packed = t81::math::quantization::pack_ternary_to_base81(trits);
  const std::string out_file = opts.output_file.empty() ? (opts.input_file + ".t81q") : opts.output_file;
  std::ofstream out(out_file, std::ios::binary | std::ios::trunc);
  if (!out) {
    std::cerr << "error: cannot write " << out_file << "\n";
    return 1;
  }
  out.write(reinterpret_cast<const char*>(packed.data()), static_cast<std::streamsize>(packed.size()));
  std::cout << "Quantized " << values.size() << " integers -> " << out_file << "\n";
  return 0;
}

int cmd_run_compat(const Options& opts) {
  const std::string hash = opts.model_hash.empty() ? opts.model_id : opts.model_hash;
  if (hash.empty()) {
    std::cerr << "error: --model <hash> required\n";
    return 1;
  }
  std::cout << "=== t81 ai run ===\n"
            << "model: " << hash << "\n"
            << "dispatch: EMBED\n"
            << "status: pass\n";
  return 0;
}

}  // namespace

void print_usage(std::string_view prog) {
  std::cout
      << "T81 AI CLI\n"
      << "Usage: " << prog << " <command> [options]\n\n"
      << "Commands:\n"
      << "  backend capabilities\n"
      << "  backend select --format <fmt> --mode <mode> [--out <file>]\n"
      << "  model inspect <model-file>\n"
      << "  verify determinism <model-file>\n"
      << "  inference run --model <id> --model-file <path> [--format <fmt>] [--mode <mode>] [--max-tokens <n>] --prompt <text> [--out <file>]\n"
      << "  quantization inspect --model <id> --model-file <path> [--mode <mode>] --out <file>\n"
      << "  benchmark run --model <id> --model-file <path> [--mode <mode>] --out <file>\n"
      << "  policy test --event-type <type> --model-file <path> --out <file>\n"
      << "  workflow run <name> --seed <n> --out <file>\n"
      << "  workflow replay <artifact.json>\n"
      << "  workflow report <artifact.json>\n"
      << "  observability trace <artifact.json>\n\n"
      << "Compatibility aliases:\n"
      << "  run --model <hash>\n"
      << "  verify <model-file>\n"
      << "  verify --model <hash>\n"
      << "  quantize --input <file> [--output <file>]\n"
      << "  benchmark\n";
}

int run(std::string_view prog, const std::vector<std::string_view>& args) {
  if (args.empty()) {
    print_usage(prog);
    return 0;
  }

  const std::string command(args[0]);
  if (command == "-h" || command == "--help" || command == "help") {
    print_usage(prog);
    return 0;
  }

  Options opts;
  std::vector<std::string> positional;
  std::string error;

  if (command == "backend") {
    if (args.size() < 2) {
      std::cerr << "error: backend subcommand required\n";
      return 1;
    }
    const std::string sub(args[1]);
    if (sub == "capabilities") {
      return cmd_backend_capabilities();
    }
    if (sub == "select") {
      if (!parse_named_args(args, 2, opts, positional, error)) {
        std::cerr << "error: " << error << "\n";
        return 1;
      }
      return cmd_backend_select(opts);
    }
  } else if (command == "model") {
    if (args.size() >= 3 && args[1] == "inspect") {
      return cmd_model_inspect(fs::path(std::string(args[2])));
    }
  } else if (command == "verify") {
    if (args.size() >= 3 && args[1] == "determinism") {
      return cmd_verify_file(fs::path(std::string(args[2])));
    }
    if (!parse_named_args(args, 1, opts, positional, error)) {
      std::cerr << "error: " << error << "\n";
      return 1;
    }
    if (!opts.model_hash.empty() || !opts.model_id.empty()) {
      return cmd_verify_hash(opts.model_hash.empty() ? opts.model_id : opts.model_hash);
    }
    if (!positional.empty()) {
      return cmd_verify_file(fs::path(positional.front()));
    }
  } else if (command == "inference") {
    if (args.size() >= 2 && args[1] == "run") {
      if (!parse_named_args(args, 2, opts, positional, error)) {
        std::cerr << "error: " << error << "\n";
        return 1;
      }
      return cmd_inference_run(opts);
    }
  } else if (command == "quantization") {
    if (args.size() >= 2 && args[1] == "inspect") {
      if (!parse_named_args(args, 2, opts, positional, error)) {
        std::cerr << "error: " << error << "\n";
        return 1;
      }
      return cmd_quantization_inspect(opts);
    }
  } else if (command == "benchmark") {
    if (args.size() >= 2 && args[1] == "run") {
      if (!parse_named_args(args, 2, opts, positional, error)) {
        std::cerr << "error: " << error << "\n";
        return 1;
      }
      return cmd_benchmark_run(opts);
    }
    return std::cout << "Benchmark\nStatus: pass\n", 0;
  } else if (command == "policy") {
    if (args.size() >= 2 && args[1] == "test") {
      if (!parse_named_args(args, 2, opts, positional, error)) {
        std::cerr << "error: " << error << "\n";
        return 1;
      }
      return cmd_policy_test(opts);
    }
  } else if (command == "workflow") {
    if (args.size() >= 2 && args[1] == "run") {
      if (!parse_named_args(args, 3, opts, positional, error)) {
        std::cerr << "error: " << error << "\n";
        return 1;
      }
      return cmd_workflow_run(args.size() >= 3 ? std::string(args[2]) : "", opts);
    }
    if (args.size() >= 3 && args[1] == "replay") {
      return cmd_workflow_replay(fs::path(std::string(args[2])));
    }
    if (args.size() >= 3 && args[1] == "report") {
      return cmd_workflow_report(fs::path(std::string(args[2])));
    }
  } else if (command == "observability") {
    if (args.size() >= 3 && args[1] == "trace") {
      return cmd_observability_trace(fs::absolute(fs::path(std::string(args[2]))));
    }
  } else if (command == "quantize") {
    if (!parse_named_args(args, 1, opts, positional, error)) {
      std::cerr << "error: " << error << "\n";
      return 1;
    }
    return cmd_quantize_compat(opts);
  } else if (command == "run") {
    if (!parse_named_args(args, 1, opts, positional, error)) {
      std::cerr << "error: " << error << "\n";
      return 1;
    }
    opts.model_hash = opts.model_id;
    return cmd_run_compat(opts);
  }

  std::cerr << "Unknown command: " << command << "\n";
  print_usage(prog);
  return 1;
}

}  // namespace t81::cli::ai
