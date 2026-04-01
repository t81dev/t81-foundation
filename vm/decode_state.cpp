#include "t81/vm/decode_state.hpp"
#include "t81/crypto/sha3.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <sstream>

namespace t81::vm {

std::string sha3_hex_text(std::string_view text) {
  const auto* begin = reinterpret_cast<const std::uint8_t*>(text.data());
  return t81::crypto::sha3_512_hex(std::span<const std::uint8_t>(begin, text.size()));
}

std::string tensor_signature_sha256(const t81::T729DynamicTensor& tensor) {
  std::ostringstream out(std::ios::binary);
  tensor.serialize(out);
  const std::string bytes = out.str();
  return sha3_hex_text(bytes);
}

constexpr std::size_t kLogitsSampleWindow = 8;
constexpr std::size_t kHiddenCarryProjectionWidth = 2;
constexpr std::size_t kDecodeContextHistoryWindow = 3;

std::vector<int> select_prompt_seeded_token_ids(std::string_view prompt, std::size_t vocab_size,
                                                std::size_t sample_window,
                                                std::size_t* window_start) {
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
                                        std::optional<int> anchor_token) {
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




std::vector<int> combined_decode_history(const DecodeState& state) {
  std::vector<int> combined = state.prompt_token_history;
  combined.insert(combined.end(), state.generated_token_history.begin(),
                  state.generated_token_history.end());
  return combined;
}

std::vector<int> decode_context_history(const DecodeState& state, std::size_t max_tokens) {
  std::vector<int> context = state.prompt_token_history;
  context.insert(context.end(), state.generated_token_history.begin(),
                 state.generated_token_history.end());
  return recent_context_history(context, max_tokens, state.prompt_anchor_token_id);
}

std::optional<IntermediateDecodeState> capture_intermediate_state(const DecodeState& state) {
  const bool has_hidden_tensor = state.carried_hidden_tensor.has_value();
  const bool has_hidden_summary = !state.hidden_tensor_signature_sha256.empty();
  const bool has_forward_state = !state.forward_state_signature_sha256.empty();
  const bool has_kv_state = !state.kv_state_signature_sha256.empty();
  const bool has_architecture_state = !state.architecture_state_signature_sha256.empty();
  if (!has_hidden_tensor && !has_hidden_summary && !has_forward_state && !has_kv_state &&
      !has_architecture_state) {
    return std::nullopt;
  }

  IntermediateDecodeState intermediate;
  intermediate.hidden_carry_row_ids = state.hidden_carry_row_ids;
  intermediate.hidden_carry_scores = state.hidden_carry_scores;
  intermediate.hidden_carry_signature_sha256 = state.hidden_carry_signature_sha256;
  intermediate.carry_probe_layout_kind = state.carry_probe_layout_kind;
  intermediate.hidden_projection_row_ids = state.hidden_projection_row_ids;
  intermediate.hidden_projection_scores = state.hidden_projection_scores;
  intermediate.hidden_projection_signature_sha256 = state.hidden_projection_signature_sha256;
  intermediate.projection_carry_mode_kind = state.projection_carry_mode_kind;
  intermediate.hidden_state_class = state.hidden_state_class;
  intermediate.hidden_state_class_signature_sha256 = state.hidden_state_class_signature_sha256;
  intermediate.hidden_tensor_signature_sha256 = state.hidden_tensor_signature_sha256;
  intermediate.hidden_tensor_rank = state.hidden_tensor_rank;
  intermediate.hidden_tensor_elements = state.hidden_tensor_elements;
  intermediate.hidden_tensor_shape = state.hidden_tensor_shape;
  intermediate.hidden_tensor = state.carried_hidden_tensor;
  intermediate.hidden_tensor_carry_mode_kind = state.hidden_tensor_carry_mode_kind;
  intermediate.forward_state_kind = state.forward_state_kind;
  intermediate.forward_state_row_ids = state.forward_state_row_ids;
  intermediate.forward_state_scores = state.forward_state_scores;
  intermediate.forward_state_signature_sha256 = state.forward_state_signature_sha256;
  intermediate.forward_state_generation = state.forward_state_generation;
  intermediate.forward_state_class = state.forward_state_class;
  intermediate.forward_state_class_signature_sha256 =
      state.forward_state_class_signature_sha256;
  intermediate.kv_state_kind = state.kv_state_kind;
  intermediate.q_tensor_signature_sha256 = state.q_tensor_signature_sha256;
  intermediate.k_tensor_signature_sha256 = state.k_tensor_signature_sha256;
  intermediate.kv_tensor_rank = state.kv_tensor_rank;
  intermediate.kv_tensor_elements = state.kv_tensor_elements;
  intermediate.kv_state_signature_sha256 = state.kv_state_signature_sha256;
  intermediate.kv_state_carry_mode_kind = state.kv_state_carry_mode_kind;
  intermediate.architecture_state_kind = state.architecture_state_kind;
  intermediate.architecture_state_signature_sha256 =
      state.architecture_state_signature_sha256;
  intermediate.architecture_state_class = state.architecture_state_class;
  intermediate.architecture_state_class_signature_sha256 =
      state.architecture_state_class_signature_sha256;
  return intermediate;
}

void apply_intermediate_state(DecodeState& state, const IntermediateDecodeState& intermediate_state) {
  state.hidden_carry_row_ids = intermediate_state.hidden_carry_row_ids;
  state.hidden_carry_scores = intermediate_state.hidden_carry_scores;
  state.hidden_carry_signature_sha256 = intermediate_state.hidden_carry_signature_sha256;
  state.carry_probe_layout_kind = intermediate_state.carry_probe_layout_kind;
  state.hidden_projection_row_ids = intermediate_state.hidden_projection_row_ids;
  state.hidden_projection_scores = intermediate_state.hidden_projection_scores;
  state.hidden_projection_signature_sha256 =
      intermediate_state.hidden_projection_signature_sha256;
  state.projection_carry_mode_kind = intermediate_state.projection_carry_mode_kind;
  state.hidden_state_class = intermediate_state.hidden_state_class;
  state.hidden_state_class_signature_sha256 =
      intermediate_state.hidden_state_class_signature_sha256;
  state.hidden_tensor_signature_sha256 = intermediate_state.hidden_tensor_signature_sha256;
  state.hidden_tensor_rank = intermediate_state.hidden_tensor_rank;
  state.hidden_tensor_elements = intermediate_state.hidden_tensor_elements;
  state.hidden_tensor_shape = intermediate_state.hidden_tensor_shape;
  state.carried_hidden_tensor = intermediate_state.hidden_tensor;
  state.hidden_tensor_carry_mode_kind = intermediate_state.hidden_tensor_carry_mode_kind;
  state.forward_state_kind = intermediate_state.forward_state_kind;
  state.forward_state_row_ids = intermediate_state.forward_state_row_ids;
  state.forward_state_scores = intermediate_state.forward_state_scores;
  state.forward_state_signature_sha256 = intermediate_state.forward_state_signature_sha256;
  state.forward_state_generation = intermediate_state.forward_state_generation;
  state.forward_state_class = intermediate_state.forward_state_class;
  state.forward_state_class_signature_sha256 =
      intermediate_state.forward_state_class_signature_sha256;
  state.kv_state_kind = intermediate_state.kv_state_kind;
  state.q_tensor_signature_sha256 = intermediate_state.q_tensor_signature_sha256;
  state.k_tensor_signature_sha256 = intermediate_state.k_tensor_signature_sha256;
  state.kv_tensor_rank = intermediate_state.kv_tensor_rank;
  state.kv_tensor_elements = intermediate_state.kv_tensor_elements;
  state.kv_state_signature_sha256 = intermediate_state.kv_state_signature_sha256;
  state.kv_state_carry_mode_kind = intermediate_state.kv_state_carry_mode_kind;
  state.architecture_state_kind = intermediate_state.architecture_state_kind;
  state.architecture_state_signature_sha256 =
      intermediate_state.architecture_state_signature_sha256;
  state.architecture_state_class = intermediate_state.architecture_state_class;
  state.architecture_state_class_signature_sha256 =
      intermediate_state.architecture_state_class_signature_sha256;
  state.intermediate_state = intermediate_state;
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

std::string decode_state_seed_digest(const DecodeState& state) {
  return prefixed_history_digest("decode-state", combined_decode_history(state));
}

std::string candidate_window_seed_digest(const DecodeState& state) {
  return prefixed_history_digest("candidate-window", combined_decode_history(state));
}

std::optional<t81::T729DynamicTensor> evolve_hidden(
    const std::optional<t81::T729DynamicTensor>& previous,
    const std::optional<t81::T729DynamicTensor>& current,
    float current_weight, float previous_weight) {
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

std::vector<int> state_input_rows_for_state(const DecodeState& state) {
  const std::size_t max_rows = std::max<std::size_t>(state.forward_state_row_ids.size(),
                                                     state.hidden_projection_row_ids.size());
  return merged_state_input_rows(state.forward_state_row_ids, state.hidden_projection_row_ids,
                                 max_rows);
}

std::string state_input_seed_digest(const DecodeState& state) {
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

void rank_and_trim_forward_state(DecodeState& state, std::size_t row_cap) {
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

void refresh_forward_state(DecodeState& state) {
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

void refresh_kv_state(DecodeState& state) {
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

void refresh_architecture_state(DecodeState& state) {
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

std::string stability_conditioned_candidate_basis(const DecodeState& state) {
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

void populate_probe_stability(DecodeProbe& result) {
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

double architecture_state_confidence_score(const DecodeState& state);
std::string architecture_state_stability_kind(const DecodeState& state);

bool stability_requires_recovery(const DecodeState& state) {
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

bool stability_should_terminate_decode(const DecodeState& state,
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

std::size_t class_conditioned_window_start(const DecodeState& state,
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

double architecture_state_confidence_score(const DecodeState& state) {
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

std::string architecture_state_stability_kind(const DecodeState& state) {
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

std::pair<float, float> hidden_tensor_mix_weights(const DecodeState& state) {
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

std::size_t architecture_state_conditioned_sample_window(const DecodeState& state,
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

std::size_t architecture_state_conditioned_context_window(const DecodeState& state,
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

std::string architecture_state_conditioned_selection_policy(const DecodeState& state) {
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

std::string architecture_state_conditioned_carry_probe_layout(const DecodeState& state) {
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

std::size_t stability_conditioned_sample_window(const DecodeState& state,
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

std::string stability_conditioned_selection_policy(const DecodeState& state) {
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

std::string stability_conditioned_decode_mode(const DecodeState& state) {
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

std::size_t stability_conditioned_context_window(const DecodeState& state) {
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

std::string stability_conditioned_carry_probe_layout(const DecodeState& state) {
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

std::string stability_conditioned_transition_kind(const DecodeState& state) {
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

std::size_t forward_state_keep(const DecodeState& state, std::size_t available) {
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

void apply_projection_carry_mode(DecodeProbe& result) {
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

std::size_t next_decode_window_start(const DecodeState& state, std::size_t vocab_size) {
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

std::string hidden_carry_signature_digest(const DecodeProbe& probe) {
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

void populate_hidden_projection(DecodeProbe& result) {
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


void DecodeState::push_step(DecodeStep&& step) {
    history_.push_back(std::move(step));
    enforce_bounds();
}

std::vector<int> DecodeState::accumulate_history() const {
    std::vector<int> combined = prompt_token_history;
    combined.insert(combined.end(), generated_token_history.begin(), generated_token_history.end());
    return combined;
}

std::vector<int> DecodeState::context_history(std::size_t max_tokens) const {
    std::vector<int> context = prompt_token_history;
    context.insert(context.end(), generated_token_history.begin(), generated_token_history.end());
    return recent_context_history(context, max_tokens, prompt_anchor_token_id);
}

void DecodeState::enforce_bounds() {
    if (history_.size() > config_.max_history) {
        history_.pop_front();
    }
}

} // namespace t81::vm
