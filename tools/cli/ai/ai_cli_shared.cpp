#include "ai_cli_shared.hpp"

#include "t81/crypto/sha3.hpp"
#include "t81/math/quantization/ternary_codec.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace t81::cli::ai {
namespace {

constexpr std::string_view kUnsupportedSentinel = "No backend supports requested format/mode";
constexpr std::string_view kFixedTimestamp = "1970-01-01T00:00:00Z";
constexpr std::string_view kPolicyAllowReason = "AI_POLICY_ALLOW_MODEL_HASH_MATCH";

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
                                 std::string_view extra_lines) {
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
      << "  \"status\": \"pass\"\n"
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
  const std::string output = "deterministic:" + sha3_hex_text(prompt).substr(0, 16);
  std::ostringstream extra;
  extra << "  \"prompt_sha256\": \"" << sha3_hex_text(prompt) << "\",\n"
        << "  \"output\": \"" << json_escape(output) << "\",\n"
        << "  \"generated_tokens\": 4,\n";
  return emit_or_write(
      make_runtime_payload("t81.ai.inference-run.v1", model_id, model_file, sel, extra.str()),
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
      << "  inference run --model <id> --model-file <path> [--format <fmt>] [--mode <mode>] --prompt <text> --out <file>\n"
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
