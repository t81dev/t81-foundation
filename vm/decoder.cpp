#include "t81/vm/decoder.hpp"

#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace fs = std::filesystem;

namespace t81::vm {
namespace {

constexpr std::size_t kHiddenCarryProbeWidth = 4;
constexpr std::size_t kNarrowGreedyMaxVocab = 256;

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

std::string detect_architecture_profile_impl(const t81::weights::ModelFile& model) {
  const auto has = [&](std::string_view key) {
    return model.native.find(std::string(key)) != model.native.end();
  };
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

std::optional<std::string> extract_json_array_field(std::string_view text,
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
  normalized += "\xE2\x96\x81";
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
    while (i < vocab_json.size() && std::isspace(static_cast<unsigned char>(vocab_json[i]))) {
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
  if (auto direct = find_vocab_token_id(*vocab_json, prompt)) {
    return {*direct};
  }
  if (auto sentencepiece =
          find_vocab_token_id(*vocab_json, std::string("▁") + std::string(prompt))) {
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

using DecodeRequest = NativeProbeRequest;

struct BuiltDecodeProgram {
  enum class TensorBindingSlot {
    CarriedHidden,
  };

  DecodeProbe probe;
  std::optional<t81::tisc::Program> program;
  std::optional<std::size_t> carried_hidden_tensor_index;
};

struct DecodeExecutionResult {
  bool ok = false;
  std::string stdout_text;
  std::string trap;
  std::vector<t81::T729DynamicTensor> exported_tensors;
  bool hidden_tensor_import_used = false;
  bool hidden_tensor_blend_used = false;
};

std::optional<IntermediateDecodeState> intermediate_state_from_probe(
    const DecodeProbe& probe,
    std::optional<t81::T729DynamicTensor> hidden_tensor,
    std::string_view hidden_tensor_carry_mode_kind,
    const DecodeState* prior_state = nullptr) {
  const bool has_hidden_tensor = hidden_tensor.has_value();
  const bool has_hidden_summary = !probe.hidden_tensor_signature_sha256.empty();
  const bool has_kv_state = !probe.kv_state_signature_sha256.empty();
  if (!has_hidden_tensor && !has_hidden_summary && !has_kv_state &&
      probe.hidden_carry_row_ids.empty()) {
    return std::nullopt;
  }

  IntermediateDecodeState intermediate;
  intermediate.hidden_carry_row_ids = probe.hidden_carry_row_ids;
  intermediate.hidden_carry_scores = probe.hidden_carry_scores;
  intermediate.hidden_carry_signature_sha256 = hidden_carry_signature_digest(probe);
  intermediate.carry_probe_layout_kind = probe.carry_probe_layout_kind;
  intermediate.hidden_projection_row_ids = probe.hidden_projection_row_ids;
  intermediate.hidden_projection_scores = probe.hidden_projection_scores;
  intermediate.hidden_projection_signature_sha256 = probe.hidden_projection_signature_sha256;
  intermediate.projection_carry_mode_kind = probe.projection_carry_mode_kind;
  intermediate.hidden_state_class = probe.hidden_state_class;
  intermediate.hidden_state_class_signature_sha256 = probe.hidden_state_class_signature_sha256;
  intermediate.hidden_tensor_signature_sha256 = has_hidden_tensor
      ? tensor_signature_sha256(*hidden_tensor)
      : probe.hidden_tensor_signature_sha256;
  intermediate.hidden_tensor_rank = has_hidden_tensor
      ? static_cast<std::size_t>(hidden_tensor->rank())
      : probe.hidden_tensor_rank;
  intermediate.hidden_tensor_elements =
      has_hidden_tensor ? hidden_tensor->size() : probe.hidden_tensor_elements;
  intermediate.hidden_tensor_shape =
      has_hidden_tensor ? hidden_tensor->shape() : probe.hidden_tensor_shape;
  intermediate.hidden_tensor = std::move(hidden_tensor);
  intermediate.hidden_tensor_carry_mode_kind = std::string(hidden_tensor_carry_mode_kind);
  intermediate.kv_state_kind = probe.kv_state_kind;
  intermediate.q_tensor_signature_sha256 = probe.q_tensor_signature_sha256;
  intermediate.k_tensor_signature_sha256 = probe.k_tensor_signature_sha256;
  intermediate.kv_tensor_rank = probe.kv_tensor_rank;
  intermediate.kv_tensor_elements = probe.kv_tensor_elements;
  intermediate.kv_state_signature_sha256 = probe.kv_state_signature_sha256;
  if (prior_state != nullptr) {
    intermediate.forward_state_kind = prior_state->forward_state_kind;
    intermediate.forward_state_row_ids = prior_state->forward_state_row_ids;
    intermediate.forward_state_scores = prior_state->forward_state_scores;
    intermediate.forward_state_signature_sha256 = prior_state->forward_state_signature_sha256;
    intermediate.forward_state_generation = prior_state->forward_state_generation;
    intermediate.forward_state_class = prior_state->forward_state_class;
    intermediate.forward_state_class_signature_sha256 =
        prior_state->forward_state_class_signature_sha256;
    intermediate.architecture_state_kind = prior_state->architecture_state_kind;
    intermediate.architecture_state_signature_sha256 =
        prior_state->architecture_state_signature_sha256;
    intermediate.architecture_state_class = prior_state->architecture_state_class;
    intermediate.architecture_state_class_signature_sha256 =
        prior_state->architecture_state_class_signature_sha256;
    intermediate.kv_state_carry_mode_kind = prior_state->kv_state_carry_mode_kind;
  }
  return intermediate;
}

bool inject_tensor_binding(t81::tisc::Program& program,
                           BuiltDecodeProgram::TensorBindingSlot slot,
                           std::size_t tensor_index,
                           const t81::T729DynamicTensor& tensor,
                           std::string& error) {
  switch (slot) {
    case BuiltDecodeProgram::TensorBindingSlot::CarriedHidden:
      if (tensor_index >= program.tensor_pool.size()) {
        error = "carried_hidden_tensor_binding_out_of_range";
        return false;
      }
      program.tensor_pool[tensor_index] = tensor;
      return true;
  }
  error = "unknown_tensor_binding_slot";
  return false;
}

void apply_selection_policy(DecodeProbe& result) {
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

  if (result.selection_policy_kind == "greedy_argmax_lowest_token_id.v1") {
    double best_score = -std::numeric_limits<double>::infinity();
    std::optional<int> best_token_id;
    std::size_t best_index = 0;
    for (std::size_t i = 0; i < result.sampled_token_scores.size(); ++i) {
      const double score = result.sampled_token_scores[i];
      const int token_id = result.sampled_token_ids[i];
      if (i == 0 || score > best_score ||
          (std::fabs(score - best_score) <= 1e-9 &&
           (!best_token_id.has_value() || token_id < *best_token_id))) {
        best_score = score;
        best_token_id = token_id;
        best_index = i;
      }
    }
    result.selected_token_id = result.sampled_token_ids[best_index];
    result.selected_token_score = result.sampled_token_scores[best_index];
    return;
  }

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

void populate_sampled_logits(DecodeProbe& result) {
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

BuiltDecodeProgram build_decode_program(const DecodeRequest& request) {
  BuiltDecodeProgram built;
  DecodeProbe& result = built.probe;
  if (!request.model) {
    result.trap = "missing_model";
    return built;
  }

  const auto pair = choose_native_probe_tensor_pair(*request.model);
  if (!pair) {
    result.trap = "no_compatible_rank2_tensor_pair";
    return built;
  }

  result.lhs_tensor = pair->first;
  result.rhs_tensor = pair->second;
  if (!request.selection_policy_override.empty()) {
    result.selection_policy_kind = request.selection_policy_override;
  }
  result.probe_kind = (pair->first == "model.layers.0.self_attn.q_proj.weight" &&
                       pair->second == "model.layers.0.self_attn.k_proj.weight")
                          ? "llama_qk_projection"
                          : "generic_matmul";
  std::vector<int> prompt_token_ids;

  std::ostringstream source;
  if (request.architecture_profile == "llama-dense-v1" &&
      request.model->native.find("model.embed_tokens.weight") != request.model->native.end() &&
      request.model->native.find("model.layers.0.self_attn.v_proj.weight") != request.model->native.end() &&
      request.model->native.find("model.layers.1.self_attn.q_proj.weight") != request.model->native.end() &&
      request.model->native.find("model.layers.1.self_attn.k_proj.weight") != request.model->native.end() &&
      request.model->native.find("model.layers.1.self_attn.v_proj.weight") != request.model->native.end()) {
    result.probe_kind = "llama_two_layer_attention_slice";
    result.embed_tensor = "model.embed_tokens.weight";
    result.value_tensor = "model.layers.0.self_attn.v_proj.weight";
    result.lhs_tensor_layer1 = "model.layers.1.self_attn.q_proj.weight";
    result.rhs_tensor_layer1 = "model.layers.1.self_attn.k_proj.weight";
    result.value_tensor_layer1 = "model.layers.1.self_attn.v_proj.weight";
    if (const auto lm_head_it = request.model->native.find("lm_head.weight");
        lm_head_it != request.model->native.end() && lm_head_it->second.shape.size() == 2 &&
        lm_head_it->second.shape[1] == 16) {
      result.logits_row_probe_supported = true;
      result.logits_vocab_size = lm_head_it->second.shape[0];
      result.logits_sample_window = request.greedy_full_vocab
          ? result.logits_vocab_size
          : request.sample_window_override.value_or(
                std::min<std::size_t>(8, result.logits_vocab_size));
      if (request.greedy_full_vocab) {
        if (result.logits_vocab_size > kNarrowGreedyMaxVocab) {
          result.trap = "narrow_greedy_vocab_limit_exceeded";
          return built;
        }
        if (request.tokenizer_path.has_value()) {
          prompt_token_ids = lookup_tokenizer_prompt_token_ids(*request.tokenizer_path,
                                                               request.prompt);
          result.prompt_token_ids = prompt_token_ids;
          if (!prompt_token_ids.empty()) {
            result.tokenizer_seed_supported = true;
            result.candidate_seed_token_id = prompt_token_ids.back();
          }
        }
        if (!result.candidate_seed_token_id.has_value()) {
          std::size_t prompt_seed_window_start = 0;
          const auto prompt_seed_ids = select_prompt_seeded_token_ids(
              request.prompt, result.logits_vocab_size, 1, &prompt_seed_window_start);
          if (!prompt_seed_ids.empty()) {
            result.candidate_seed_token_id = prompt_seed_ids.front();
          }
          result.tokenizer_seed_supported = false;
        }
        result.logits_candidate_window_start = 0;
        result.sampled_token_ids = make_contiguous_window_token_ids(
            0, result.logits_vocab_size, result.logits_vocab_size);
        result.candidate_selection_mode = "greedy_full_vocab";
        result.candidate_selection_basis = "full_vocab_logits_argmax.v1";
        result.selection_policy_kind = "greedy_argmax_lowest_token_id.v1";
      } else {
        if (request.candidate_window_seed.has_value()) {
          result.logits_candidate_window_start =
              static_cast<std::size_t>(*request.candidate_window_seed) % result.logits_vocab_size;
          result.candidate_seed_token_id = *request.candidate_window_seed;
          result.sampled_token_ids = make_contiguous_window_token_ids(
              result.logits_candidate_window_start, result.logits_vocab_size,
              result.logits_sample_window);
          result.candidate_selection_mode =
              request.candidate_mode_override.empty() ? "decode_feedback_token"
                                                      : request.candidate_mode_override;
          result.candidate_selection_basis =
              request.candidate_basis_override.empty()
                  ? "selected_candidate_feedback_contiguous_window.v1"
                  : request.candidate_basis_override;
          result.tokenizer_seed_supported =
              request.tokenizer_seed_supported_override.value_or(false);
        } else if (request.tokenizer_path.has_value()) {
          prompt_token_ids = lookup_tokenizer_prompt_token_ids(*request.tokenizer_path,
                                                               request.prompt);
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
              request.prompt, result.logits_vocab_size, result.logits_sample_window,
              &result.logits_candidate_window_start);
        }
      }
      if (native_tensor_shape_complexity(lm_head_it->second) <= 81u * 81u * 9u) {
        result.logits_projection_supported = true;
      }
      const std::size_t hidden_carry_start =
          (result.logits_candidate_window_start + result.logits_sample_window) %
          result.logits_vocab_size;
      if (!request.carry_probe_layout_override.empty()) {
        result.carry_probe_layout_kind = request.carry_probe_layout_override;
      }
      result.hidden_carry_row_ids = carry_probe_row_ids(
          hidden_carry_start, result.logits_vocab_size,
          std::min<std::size_t>(kHiddenCarryProbeWidth, result.logits_vocab_size),
          result.carry_probe_layout_kind);
    }
    if (request.input_token_override.has_value()) {
      result.token_index = *request.input_token_override;
    } else if (result.candidate_seed_token_id.has_value()) {
      result.token_index = *result.candidate_seed_token_id;
    } else if (!result.sampled_token_ids.empty()) {
      result.token_index = result.sampled_token_ids.front();
    } else {
      result.token_index = 0;
    }
    result.context_token_indices = request.context_token_history;
    if (result.context_token_indices.empty() && prompt_token_ids.size() > 1) {
      result.context_token_indices.assign(prompt_token_ids.begin(), prompt_token_ids.end() - 1);
      result.context_token_indices = recent_context_history(
          result.context_token_indices, 3, prompt_token_ids.front());
    }
    source << "@ternary_inference\n"
           << "@tier(2)\n"
           << "fn main() -> i32 {\n"
           << "  let table: i32 = std.tensor.load(\"" << result.embed_tensor << "\");\n"
           << "  let tok: i32 = " << result.token_index << ";\n"
           << "  let emb_base: Tensor = std.tnn.embed(table, tok);\n";
    if (!result.context_token_indices.empty() || !request.hidden_carry_context_rows.empty()) {
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
      for (std::size_t i = 0; i < request.hidden_carry_context_rows.size(); ++i) {
        source << "  let carry_ctx_tok" << i << ": i32 = " << request.hidden_carry_context_rows[i]
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
    source << "  let lhs: i32 = std.tensor.load(\"" << pair->first << "\");\n"
           << "  let rhs: i32 = std.tensor.load(\"" << pair->second << "\");\n"
           << "  let val_w: i32 = std.tensor.load(\"" << result.value_tensor << "\");\n"
           << "  let q0: Tensor = std.tensor.matmul(emb, lhs);\n"
           << "  let k0: Tensor = std.tensor.matmul(emb, rhs);\n"
           << "  let v0: Tensor = std.tensor.matmul(emb, val_w);\n"
           << "  let attn0: Tensor = std.tensor.attention(q0, k0, v0);\n"
           << "  let lhs1: i32 = std.tensor.load(\"" << result.lhs_tensor_layer1 << "\");\n"
           << "  let rhs1: i32 = std.tensor.load(\"" << result.rhs_tensor_layer1 << "\");\n"
           << "  let val1_w: i32 = std.tensor.load(\"" << result.value_tensor_layer1 << "\");\n";
    if (request.intermediate_state.has_value() &&
        request.intermediate_state->hidden_tensor.has_value()) {
      source << "  let carried_hidden: Tensor = std.tensor.from_list([0]);\n"
             << "  let layer1_input: Tensor = std.tensor.vec_add(attn0, carried_hidden);\n";
    } else {
      source << "  let layer1_input: Tensor = attn0;\n";
    }
    source << "  let q1: Tensor = std.tensor.matmul(layer1_input, lhs1);\n"
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

  built.program = t81::cli::build_program_from_source(source.str(), "<t81 decode>", request.model);
  if (!built.program.has_value()) {
    result.trap = "compile_failure";
    return built;
  }
  if (request.intermediate_state.has_value() &&
      request.intermediate_state->hidden_tensor.has_value()) {
    if (!built.program->tensor_pool.empty()) {
      built.carried_hidden_tensor_index = built.program->tensor_pool.size() - 1;
    } else {
      result.trap = "intermediate_tensor_import_patch_failure";
      built.program.reset();
      return built;
    }
  }
  return built;
}

DecodeExecutionResult execute_decode_program(const BuiltDecodeProgram& built,
                                            const DecodeRequest& request) {
  DecodeExecutionResult execution;
  if (!built.program.has_value()) {
    execution.trap = built.probe.trap.empty() ? "missing_program" : built.probe.trap;
    return execution;
  }

  t81::tisc::Program program = *built.program;
  if (request.intermediate_state.has_value() &&
      request.intermediate_state->hidden_tensor.has_value()) {
    if (!built.carried_hidden_tensor_index.has_value()) {
      execution.trap = "missing_carried_hidden_tensor_binding";
      return execution;
    }
    std::string injection_error;
    if (!inject_tensor_binding(program, BuiltDecodeProgram::TensorBindingSlot::CarriedHidden,
                               *built.carried_hidden_tensor_index,
                               *request.intermediate_state->hidden_tensor,
                               injection_error)) {
      execution.trap = injection_error;
      return execution;
    }
    execution.hidden_tensor_import_used = true;
    execution.hidden_tensor_blend_used = true;
  }

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  const auto run = vm->run_to_halt();
  for (std::size_t i = 0; i < vm->state().printed_output.size(); ++i) {
    if (i != 0) {
      execution.stdout_text.push_back('\n');
    }
    execution.stdout_text += vm->state().printed_output[i];
  }
  if (!run) {
    execution.trap = t81::vm::to_string(run.error());
    return execution;
  }

  if (built.probe.logits_row_probe_supported) {
    for (std::size_t i = vm->state().tensors.size(); i > 0; --i) {
      const std::size_t tensor_index = i - 1;
      if (!vm->state().tensors[tensor_index].has_value()) {
        continue;
      }
      execution.exported_tensors.push_back(*vm->state().tensors[tensor_index]);
      if (execution.exported_tensors.size() >= 3) {
        break;
      }
    }
  }
  execution.ok = true;
  return execution;
}

DecodeProbe interpret_decode_execution(const BuiltDecodeProgram& built,
                                      const DecodeExecutionResult& execution) {
  DecodeProbe result = built.probe;
  if (!execution.ok) {
    result.trap = execution.trap;
    return result;
  }

  result.stdout_text = execution.stdout_text;
  result.hidden_tensor_import_used = execution.hidden_tensor_import_used;
  result.hidden_tensor_blend_used = execution.hidden_tensor_blend_used;
  populate_sampled_logits(result);
  if (result.logits_row_probe_supported) {
    result.intermediate_tensor_export_supported = !execution.exported_tensors.empty();
    if (!execution.exported_tensors.empty()) {
      const auto& tensor = execution.exported_tensors.back();
      result.hidden_tensor_handle = static_cast<int>(execution.exported_tensors.size());
      result.hidden_tensor_rank = static_cast<std::size_t>(tensor.rank());
      result.hidden_tensor_elements = tensor.size();
      result.hidden_tensor_shape = tensor.shape();
      result.hidden_tensor_signature_sha256 = tensor_signature_sha256(tensor);
      result.hidden_tensor = tensor;
    }
    if (execution.exported_tensors.size() >= 2) {
      const auto& k_tensor = execution.exported_tensors[0];
      const auto& q_tensor =
          execution.exported_tensors[execution.exported_tensors.size() >= 3 ? 2 : 1];
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
    result.intermediate_state = intermediate_state_from_probe(
        result, result.hidden_tensor, result.hidden_tensor_carry_mode_kind);
  }
  result.ok = true;
  return result;
}

DecodeProbe run_native_vm_probe_impl(const DecodeRequest& request) {
  const BuiltDecodeProgram built = build_decode_program(request);
  if (!built.program.has_value()) {
    return built.probe;
  }
  const DecodeExecutionResult execution = execute_decode_program(built, request);
  return interpret_decode_execution(built, execution);
}

}  // namespace

std::string detect_architecture_profile(const t81::weights::ModelFile& model) {
  return detect_architecture_profile_impl(model);
}

ModelCompanionFiles find_model_companion_files(const std::filesystem::path& model_path) {
  ModelCompanionFiles companions;
  const fs::path dir = model_path.parent_path();
  const fs::path config = dir / "config.json";
  const fs::path tokenizer = dir / "tokenizer.json";
  companions.has_config = fs::exists(config);
  companions.has_tokenizer = fs::exists(tokenizer);
  companions.config_path = fs::absolute(config);
  companions.tokenizer_path = fs::absolute(tokenizer);
  return companions;
}

NativeProbeRequest make_initial_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    std::optional<std::filesystem::path> tokenizer_path) {
  NativeProbeRequest request;
  request.model = model;
  request.architecture_profile = std::string(architecture_profile);
  request.prompt = std::string(prompt);
  request.tokenizer_path = std::move(tokenizer_path);
  return request;
}

NativeProbeRequest make_initial_greedy_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    std::optional<std::filesystem::path> tokenizer_path) {
  NativeProbeRequest request =
      make_initial_probe_request(model, architecture_profile, prompt, std::move(tokenizer_path));
  request.greedy_full_vocab = true;
  request.selection_policy_override = "greedy_argmax_lowest_token_id.v1";
  request.candidate_mode_override = "greedy_full_vocab";
  request.candidate_basis_override = "full_vocab_logits_argmax.v1";
  return request;
}

NativeProbeRequest make_decode_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    const DecodeState& state,
    std::size_t logits_vocab_size,
    std::optional<std::filesystem::path> tokenizer_path,
    std::optional<bool> tokenizer_seed_supported_override) {
  NativeProbeRequest request;
  request.model = model;
  request.architecture_profile = std::string(architecture_profile);
  request.prompt = std::string(prompt);
  request.tokenizer_path = std::move(tokenizer_path);
  request.candidate_window_seed = *state.seed_token_id;
  request.input_token_override = state.generated_token_history.back();
  request.context_token_history =
      decode_context_history(state, stability_conditioned_context_window(state));
  request.hidden_carry_context_rows = merged_state_input_rows(
      state.forward_state_row_ids, state.hidden_projection_row_ids,
      std::max<std::size_t>(state.forward_state_row_ids.size(),
                            state.hidden_projection_row_ids.size()));
  request.intermediate_state = capture_intermediate_state(state);
  request.sample_window_override = stability_conditioned_sample_window(state, logits_vocab_size);
  request.selection_policy_override =
      std::string(stability_conditioned_selection_policy(state));
  request.carry_probe_layout_override =
      std::string(stability_conditioned_carry_probe_layout(state));
  request.candidate_mode_override = "decode_feedback_history";
  request.candidate_basis_override =
      std::string(stability_conditioned_candidate_basis(state));
  request.tokenizer_seed_supported_override = tokenizer_seed_supported_override;
  return request;
}

NativeProbeRequest make_greedy_decode_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    const DecodeState& state,
    std::size_t logits_vocab_size,
    std::optional<std::filesystem::path> tokenizer_path,
    std::optional<bool> tokenizer_seed_supported_override) {
  NativeProbeRequest request = make_decode_probe_request(
      model, architecture_profile, prompt, state, logits_vocab_size, std::move(tokenizer_path),
      tokenizer_seed_supported_override);
  request.greedy_full_vocab = true;
  request.sample_window_override = logits_vocab_size;
  request.selection_policy_override = "greedy_argmax_lowest_token_id.v1";
  request.candidate_mode_override = "greedy_full_vocab";
  request.candidate_basis_override = "full_vocab_logits_argmax.v1";
  request.candidate_window_seed = std::nullopt;
  return request;
}

DecodeProbe run_native_vm_probe(const NativeProbeRequest& request) {
  return run_native_vm_probe_impl(request);
}

StateTransition derive_initial_transition(const DecodeProbe& probe) {
  StateTransition transition;
  transition.prompt_token_history = probe.prompt_token_ids;
  transition.generated_token_history = {*probe.selected_token_id};
  transition.prompt_anchor_token_id = probe.prompt_token_ids.empty()
                                          ? std::nullopt
                                          : std::optional<int>(probe.prompt_token_ids.front());
  transition.input_token_id = probe.token_index;
  transition.seed_token_id = probe.candidate_seed_token_id;
  transition.window_start = probe.logits_candidate_window_start;
  transition.hidden_carry_row_ids = probe.hidden_carry_row_ids;
  transition.hidden_carry_scores = probe.hidden_carry_scores;
  transition.hidden_carry_signature_sha256 = hidden_carry_signature_digest(probe);
  transition.carry_probe_layout_kind = probe.carry_probe_layout_kind;
  transition.hidden_projection_row_ids = probe.hidden_projection_row_ids;
  transition.hidden_projection_scores = probe.hidden_projection_scores;
  transition.hidden_projection_signature_sha256 = probe.hidden_projection_signature_sha256;
  transition.projection_carry_mode_kind = probe.projection_carry_mode_kind;
  transition.hidden_state_class = probe.hidden_state_class;
  transition.hidden_state_class_signature_sha256 = probe.hidden_state_class_signature_sha256;
  transition.hidden_tensor_signature_sha256 = probe.hidden_tensor_signature_sha256;
  transition.hidden_tensor_rank = probe.hidden_tensor_rank;
  transition.hidden_tensor_elements = probe.hidden_tensor_elements;
  transition.hidden_tensor_shape = probe.hidden_tensor_shape;
  transition.carried_hidden_tensor = probe.hidden_tensor;
  transition.hidden_tensor_carry_mode_kind = "current_only.v1";
  transition.kv_state_kind = probe.kv_state_kind;
  transition.q_tensor_signature_sha256 = probe.q_tensor_signature_sha256;
  transition.k_tensor_signature_sha256 = probe.k_tensor_signature_sha256;
  transition.kv_tensor_rank = probe.kv_tensor_rank;
  transition.kv_tensor_elements = probe.kv_tensor_elements;
  transition.kv_state_signature_sha256 = probe.kv_state_signature_sha256;
  transition.selection_policy_kind = probe.selection_policy_kind;
  transition.confidence_score = probe.confidence_score;
  transition.logits_margin = probe.logits_margin;
  transition.hidden_carry_peak = probe.hidden_carry_peak;
  transition.stability_kind = probe.stability_kind;
  transition.intermediate_state =
      intermediate_state_from_probe(probe, probe.hidden_tensor, "current_only.v1");
  return transition;
}

StateTransition derive_probe_transition(const DecodeState& state,
                                        const DecodeProbe& probe,
                                        std::string_view transition_kind) {
  StateTransition transition;
  transition.prompt_token_history = state.prompt_token_history;
  transition.generated_token_history = state.generated_token_history;
  transition.generated_token_history.push_back(*probe.selected_token_id);
  transition.prompt_anchor_token_id = state.prompt_anchor_token_id;
  transition.input_token_id = probe.token_index;
  transition.seed_token_id = state.seed_token_id;
  transition.window_start = probe.logits_candidate_window_start;
  transition.hidden_carry_row_ids = probe.hidden_carry_row_ids;
  transition.hidden_carry_scores = probe.hidden_carry_scores;
  transition.hidden_carry_signature_sha256 = hidden_carry_signature_digest(probe);
  transition.carry_probe_layout_kind = probe.carry_probe_layout_kind;
  transition.hidden_projection_row_ids = probe.hidden_projection_row_ids;
  transition.hidden_projection_scores = probe.hidden_projection_scores;
  transition.hidden_projection_signature_sha256 = probe.hidden_projection_signature_sha256;
  transition.projection_carry_mode_kind = probe.projection_carry_mode_kind;
  transition.hidden_state_class = probe.hidden_state_class;
  transition.hidden_state_class_signature_sha256 = probe.hidden_state_class_signature_sha256;
  const auto [hidden_current_weight, hidden_previous_weight] =
      hidden_tensor_mix_weights(state);
  const auto evolved_hidden = evolve_hidden(state.carried_hidden_tensor, probe.hidden_tensor,
                                            hidden_current_weight, hidden_previous_weight);
  transition.carried_hidden_tensor = evolved_hidden;
  if (evolved_hidden.has_value()) {
    transition.hidden_tensor_signature_sha256 = tensor_signature_sha256(*evolved_hidden);
    transition.hidden_tensor_rank = static_cast<std::size_t>(evolved_hidden->rank());
    transition.hidden_tensor_elements = evolved_hidden->size();
    transition.hidden_tensor_shape = evolved_hidden->shape();
    transition.hidden_tensor_carry_mode_kind =
        probe.hidden_tensor_import_used
            ? (transition_kind == "architecture_state_feedback_state_transition.v1"
                   ? "architecture_state_evolved_hidden_tensor_feedback.v1"
                   : "evolved_hidden_tensor_feedback.v1")
            : "current_only.v1";
  }
  transition.kv_state_kind = probe.kv_state_kind;
  transition.q_tensor_signature_sha256 = probe.q_tensor_signature_sha256;
  transition.k_tensor_signature_sha256 = probe.k_tensor_signature_sha256;
  transition.kv_tensor_rank = probe.kv_tensor_rank;
  transition.kv_tensor_elements = probe.kv_tensor_elements;
  transition.kv_state_signature_sha256 = probe.kv_state_signature_sha256;
  transition.selection_policy_kind = probe.selection_policy_kind;
  transition.confidence_score = probe.confidence_score;
  transition.logits_margin = probe.logits_margin;
  transition.hidden_carry_peak = probe.hidden_carry_peak;
  transition.stability_kind = probe.stability_kind;
  transition.intermediate_state = intermediate_state_from_probe(
      probe, evolved_hidden, transition.hidden_tensor_carry_mode_kind, &state);
  return transition;
}

void apply_state_transition(DecodeState& state, const StateTransition& transition) {
  state.prompt_token_history = transition.prompt_token_history;
  state.generated_token_history = transition.generated_token_history;
  state.prompt_anchor_token_id = transition.prompt_anchor_token_id;
  state.input_token_id = transition.input_token_id;
  state.seed_token_id = transition.seed_token_id;
  state.window_start = transition.window_start;
  if (transition.intermediate_state.has_value()) {
    apply_intermediate_state(state, *transition.intermediate_state);
  } else {
    state.hidden_carry_row_ids = transition.hidden_carry_row_ids;
    state.hidden_carry_scores = transition.hidden_carry_scores;
    state.hidden_carry_signature_sha256 = transition.hidden_carry_signature_sha256;
    state.carry_probe_layout_kind = transition.carry_probe_layout_kind;
    state.hidden_projection_row_ids = transition.hidden_projection_row_ids;
    state.hidden_projection_scores = transition.hidden_projection_scores;
    state.hidden_projection_signature_sha256 = transition.hidden_projection_signature_sha256;
    state.projection_carry_mode_kind = transition.projection_carry_mode_kind;
    state.hidden_state_class = transition.hidden_state_class;
    state.hidden_state_class_signature_sha256 = transition.hidden_state_class_signature_sha256;
    state.hidden_tensor_signature_sha256 = transition.hidden_tensor_signature_sha256;
    state.hidden_tensor_rank = transition.hidden_tensor_rank;
    state.hidden_tensor_elements = transition.hidden_tensor_elements;
    state.hidden_tensor_shape = transition.hidden_tensor_shape;
    state.carried_hidden_tensor = transition.carried_hidden_tensor;
    state.hidden_tensor_carry_mode_kind = transition.hidden_tensor_carry_mode_kind;
    state.kv_state_kind = transition.kv_state_kind;
    state.q_tensor_signature_sha256 = transition.q_tensor_signature_sha256;
    state.k_tensor_signature_sha256 = transition.k_tensor_signature_sha256;
    state.kv_tensor_rank = transition.kv_tensor_rank;
    state.kv_tensor_elements = transition.kv_tensor_elements;
    state.kv_state_signature_sha256 = transition.kv_state_signature_sha256;
    state.intermediate_state.reset();
  }
  refresh_kv_state(state);
  refresh_forward_state(state);
  refresh_architecture_state(state);
  state.intermediate_state = capture_intermediate_state(state);
  state.selection_policy_kind = transition.selection_policy_kind;
  state.confidence_score = transition.confidence_score;
  state.logits_margin = transition.logits_margin;
  state.hidden_carry_peak = transition.hidden_carry_peak;
  state.stability_kind = transition.stability_kind;
}

std::string bounded_horizon_reason(const DecodeState& state) {
  if (!state.architecture_state_signature_sha256.empty() &&
      state.forward_state_generation >= 2) {
    return "deep_architecture_state_horizon_reached";
  }
  return "bounded_horizon_reached";
}

Decoder::Decoder(DecodeConfig config)
    : config_(std::move(config)), state_(config_) {}

void Decoder::load_model(const std::filesystem::path& model_path) {
  model_ = std::make_shared<t81::weights::ModelFile>(t81::weights::load_t81w(model_path));
  architecture_profile_ = detect_architecture_profile(*model_);
  reset();
}

void Decoder::load_model(t81::weights::ModelFile model) {
  model_ = std::make_shared<t81::weights::ModelFile>(std::move(model));
  architecture_profile_ = detect_architecture_profile(*model_);
  reset();
}

void Decoder::reset() {
  prompt_.clear();
  state_ = DecodeState(config_);
  phase_ = DecodePhase::Initialization;
  mode_ = DecoderMode::BoundedProbe;
  steps_emitted_ = 0;
  consecutive_recovery_steps_ = 0;
  logits_vocab_size_ = 0;
  terminated_ = false;
  termination_reason_.clear();
}

bool Decoder::supports_narrow_greedy_decode() const {
  return architecture_profile_ == "llama-dense-v1";
}

DecoderStepResult Decoder::step_impl(DecoderMode mode, const DecoderInput& input) {
  DecoderStepResult result;
  result.step = steps_emitted_;

  if (!model_) {
    result.probe.trap = "missing_model";
    return result;
  }
  if (terminated_) {
    result.probe.trap = termination_reason_.empty() ? "decoder_terminated" : termination_reason_;
    result.terminated = true;
    result.termination_reason = termination_reason_;
    return result;
  }

  if (phase_ != DecodePhase::Initialization && mode_ != mode) {
    result.probe.trap = "decoder_mode_switch_requires_reset";
    return result;
  }

  if (mode == DecoderMode::NarrowGreedyLlamaDenseV1 && !supports_narrow_greedy_decode()) {
    result.probe.trap = "narrow_greedy_decode_unsupported_architecture";
    return result;
  }

  mode_ = mode;

  if (phase_ == DecodePhase::Initialization) {
    prompt_ = input.prompt.empty() ? "deterministic prompt" : input.prompt;
    if (mode == DecoderMode::NarrowGreedyLlamaDenseV1) {
      result.transition_kind = "prompt_seed_to_narrow_greedy_decode_state.v1";
      result.decode_mode_kind = "narrow_greedy_llama_dense_v1";
      result.probe = run_native_vm_probe_impl(
          make_initial_greedy_probe_request(model_, architecture_profile_, prompt_));
    } else {
      result.transition_kind = "prompt_seed_to_bounded_decode_state.v1";
      result.decode_mode_kind = "initial_probe_decode_mode.v1";
      result.probe = run_native_vm_probe_impl(
          make_initial_probe_request(model_, architecture_profile_, prompt_));
    }
    if (!result.probe.ok || !result.probe.selected_token_id.has_value() ||
        !result.probe.selected_token_score.has_value()) {
      terminated_ = true;
      phase_ = DecodePhase::Terminated;
      termination_reason_ = result.probe.trap.empty() ? "decode_probe_unavailable"
                                                      : result.probe.trap;
      result.terminated = true;
      result.termination_reason = termination_reason_;
      return result;
    }
    result.transition = derive_initial_transition(result.probe);
    apply_state_transition(state_, result.transition);
    logits_vocab_size_ = result.probe.logits_vocab_size;
    phase_ = DecodePhase::Decoding;
  } else {
    if (mode == DecoderMode::NarrowGreedyLlamaDenseV1) {
      result.decode_mode_kind = "narrow_greedy_llama_dense_v1";
      result.transition_kind = "narrow_greedy_state_transition.v1";
      state_.seed_token_id.reset();
      result.probe = run_native_vm_probe_impl(
          make_greedy_decode_probe_request(model_, architecture_profile_, prompt_, state_,
                                           logits_vocab_size_));
    } else {
      result.decode_mode_kind = stability_conditioned_decode_mode(state_);
      result.transition_kind = stability_conditioned_transition_kind(state_);
      state_.seed_token_id =
          static_cast<int>(next_decode_window_start(state_, logits_vocab_size_));
      result.probe = run_native_vm_probe_impl(
          make_decode_probe_request(model_, architecture_profile_, prompt_, state_,
                                    logits_vocab_size_));
    }
    if (!result.probe.ok || !result.probe.selected_token_id.has_value() ||
        !result.probe.selected_token_score.has_value()) {
      terminated_ = true;
      phase_ = DecodePhase::Terminated;
      termination_reason_ = result.probe.trap.empty() ? "decode_probe_unavailable"
                                                      : result.probe.trap;
      result.terminated = true;
      result.termination_reason = termination_reason_;
      return result;
    }
    result.transition =
        derive_probe_transition(state_, result.probe, result.transition_kind);
    apply_state_transition(state_, result.transition);
    logits_vocab_size_ = result.probe.logits_vocab_size;
  }

  result.ok = true;
  ++steps_emitted_;

  if (mode == DecoderMode::BoundedProbe) {
    if (stability_requires_recovery(state_)) {
      ++consecutive_recovery_steps_;
      if (stability_should_terminate_decode(state_, consecutive_recovery_steps_)) {
        terminated_ = true;
        phase_ = DecodePhase::Terminated;
        termination_reason_ = "stability_recovery_exhausted";
      }
    } else {
      consecutive_recovery_steps_ = 0;
    }
  } else {
    consecutive_recovery_steps_ = 0;
  }

  if (!terminated_ && steps_emitted_ >= config_.bounded_horizon_steps) {
    terminated_ = true;
    phase_ = DecodePhase::Terminated;
    termination_reason_ = bounded_horizon_reason(state_);
  }

  result.terminated = terminated_;
  result.termination_reason = termination_reason_;
  return result;
}

DecoderStepResult Decoder::step(const DecoderInput& input) {
  return step_impl(DecoderMode::BoundedProbe, input);
}

DecoderStepResult Decoder::greedy_step(const DecoderInput& input) {
  return step_impl(DecoderMode::NarrowGreedyLlamaDenseV1, input);
}

}  // namespace t81::vm
