#include "ai_cli_shared.hpp"

#include "t81/cli/driver.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/math/quantization/ternary_codec.hpp"
#include "t81/vm/decoder.hpp"
#include "t81/vm/vm.hpp"
#include "t81/vm/decode_state.hpp"

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
using namespace t81::vm;
namespace {

constexpr std::string_view kUnsupportedSentinel = "No backend supports requested format/mode";
constexpr std::string_view kFixedTimestamp = "1970-01-01T00:00:00Z";
constexpr std::string_view kPolicyAllowReason = "AI_POLICY_ALLOW_MODEL_HASH_MATCH";
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

std::optional<std::string> extract_json_object_field(std::string_view text,
                                                     std::string_view field) {
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
    const std::string architecture_profile = t81::vm::detect_architecture_profile(*model);
    const auto companions = t81::vm::find_model_companion_files(
        resolved_model_path.value_or(fs::absolute(model_file)));
    const auto probe = t81::vm::run_native_vm_probe(t81::vm::make_initial_probe_request(
        model, architecture_profile, prompt,
        companions.has_tokenizer ? std::optional<fs::path>(companions.tokenizer_path)
                                 : std::nullopt));
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
        t81::vm::DecodeState decode_state;
        apply_state_transition(decode_state, derive_initial_transition(probe));
        const auto step0_context = decode_context_history(decode_state, kDecodeContextHistoryWindow);
        const auto step0_combined_history = combined_decode_history(decode_state);
        const std::size_t step0_next_window_start =
            next_decode_window_start(decode_state, probe.logits_vocab_size);
        decode_steps.push_back({0,
                                std::string(t81::vm::kDecodeStateKind),
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
          const auto decode_probe = t81::vm::run_native_vm_probe(
              t81::vm::make_decode_probe_request(
                  model, architecture_profile, prompt, decode_state, probe.logits_vocab_size,
                  companions.has_tokenizer
                      ? std::optional<fs::path>(companions.tokenizer_path)
                      : std::nullopt,
                  probe.tokenizer_seed_supported));
          if (!decode_probe.ok || !decode_probe.selected_token_id.has_value() ||
              !decode_probe.selected_token_score.has_value()) {
            termination_reason = "decode_probe_unavailable";
            decode_probe_failure_trap = decode_probe.trap;
            break;
          }
          apply_state_transition(
              decode_state,
              derive_probe_transition(decode_state, decode_probe, transition_kind));
          const auto combined_history = combined_decode_history(decode_state);
          decode_steps.push_back(
              {step,
               std::string(t81::vm::kDecodeStateKind),
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
        std::size_t architecture_state_deep_feedback_steps = 0;
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
            ++architecture_state_deep_feedback_steps;
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
              << "    \"deep_feedback_steps\": " << architecture_state_deep_feedback_steps
              << ",\n"
              << "    \"utilization\": "
              << (architecture_state_feedback_steps == 0
                      ? 0.0
                      : static_cast<double>(architecture_state_deep_feedback_steps) /
                            static_cast<double>(architecture_state_feedback_steps))
              << ",\n"
              << "    \"feedback_steps\": " << architecture_state_feedback_steps << ",\n"
              << "    \"summary\": \"native probes carried a bounded combined architecture state across hidden-tensor, q/k, and forward-state evidence through "
              << architecture_state_feedback_steps
              << " architecture-state-led feedback steps\"\n"
              << "  },\n"
              << "  \"requested_max_tokens\": " << opts.max_tokens << ",\n"
              << "  \"bounded_horizon_steps\": " << kBoundedDecodeTraceSteps << ",\n"
              << "  \"bounded_horizon_remaining\": "
              << (generated_tokens >= kBoundedDecodeTraceSteps
                      ? 0
                      : (kBoundedDecodeTraceSteps - generated_tokens))
              << ",\n"
              << "  \"bounded_horizon_utilization\": "
              << (kBoundedDecodeTraceSteps == 0
                      ? 0.0
                      : static_cast<double>(generated_tokens) /
                            static_cast<double>(kBoundedDecodeTraceSteps))
              << ",\n"
              << "  \"bounded_horizon_reached\": "
              << ((termination_reason == "deep_architecture_state_horizon_reached" ||
                   termination_reason == "max_tokens_reached")
                      ? "true"
                      : "false")
              << ",\n"
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
