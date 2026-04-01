#pragma once

#include "t81/vm/vm.hpp"

#include <vector>
#include <string>
#include <optional>
#include <deque>

namespace t81::vm {

struct DecodeConfig {
    std::size_t max_history = 16;
    bool carry_hidden = true;
    bool carry_qk = true;
    std::size_t bounded_horizon_steps = 4;
    std::size_t decode_context_history_window = 3;
};

struct IntermediateDecodeState {
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
    std::string hidden_tensor_signature_sha256;
    std::size_t hidden_tensor_rank = 0;
    std::size_t hidden_tensor_elements = 0;
    std::vector<int> hidden_tensor_shape;
    std::optional<t81::T729DynamicTensor> hidden_tensor;
    std::string hidden_tensor_carry_mode_kind = "current_only.v1";
    std::string forward_state_kind = "unavailable";
    std::vector<int> forward_state_row_ids;
    std::vector<double> forward_state_scores;
    std::string forward_state_signature_sha256;
    std::size_t forward_state_generation = 0;
    std::string forward_state_class;
    std::string forward_state_class_signature_sha256;
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
};

struct DecodeProbe {
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
    std::optional<IntermediateDecodeState> intermediate_state;
    std::string selection_policy_kind = "max_score.v1";
    double confidence_score = 0.0;
    double logits_margin = 0.0;
    double hidden_carry_peak = 0.0;
    std::string stability_kind = "unclassified";
    std::optional<int> selected_token_id;
    std::optional<double> selected_token_score;
};

struct DecodeStep {
    std::optional<t81::T729DynamicTensor> hidden;
    std::string q_tensor_signature_sha256;
    std::string k_tensor_signature_sha256;

    DecodeProbe probe;
};

class DecodeState {
public:
    explicit DecodeState(DecodeConfig config = DecodeConfig{}) : config_(std::move(config)) {}

    void push_step(DecodeStep&& step);

    std::vector<int> accumulate_history() const;
    std::vector<int> context_history(std::size_t max_tokens) const;

    // Existing fields from NativeDecodeState, exposed as public state for now
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
    std::optional<IntermediateDecodeState> intermediate_state;
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

    const DecodeConfig& config() const { return config_; }

private:
    void enforce_bounds();

    DecodeConfig config_;
    std::deque<DecodeStep> history_;
};

// Extracted helpers from ai_cli_shared.cpp
std::optional<t81::T729DynamicTensor> evolve_hidden(
    const std::optional<t81::T729DynamicTensor>& previous,
    const std::optional<t81::T729DynamicTensor>& current,
    float current_weight = 0.75F, float previous_weight = 0.25F);

std::vector<int> state_input_rows_for_state(const DecodeState& state);
std::string state_input_seed_digest(const DecodeState& state);
void rank_and_trim_forward_state(DecodeState& state, std::size_t row_cap);
void refresh_forward_state(DecodeState& state);
void refresh_kv_state(DecodeState& state);
void refresh_architecture_state(DecodeState& state);

std::string stability_conditioned_candidate_basis(const DecodeState& state);
double architecture_state_confidence_score(const DecodeState& state);
std::string architecture_state_stability_kind(const DecodeState& state);
bool stability_requires_recovery(const DecodeState& state);
bool stability_should_terminate_decode(const DecodeState& state, std::size_t consecutive_recovery_steps);
std::size_t class_conditioned_window_start(const DecodeState& state, std::size_t vocab_size);
std::pair<float, float> hidden_tensor_mix_weights(const DecodeState& state);
std::size_t architecture_state_conditioned_sample_window(const DecodeState& state, std::size_t base, std::size_t cap);
std::size_t architecture_state_conditioned_context_window(const DecodeState& state, std::size_t base);
std::string architecture_state_conditioned_selection_policy(const DecodeState& state);
std::string architecture_state_conditioned_carry_probe_layout(const DecodeState& state);
std::size_t stability_conditioned_sample_window(const DecodeState& state, std::size_t vocab_size);
std::string stability_conditioned_selection_policy(const DecodeState& state);
std::string stability_conditioned_decode_mode(const DecodeState& state);
std::size_t stability_conditioned_context_window(const DecodeState& state);
std::string stability_conditioned_carry_probe_layout(const DecodeState& state);
std::string stability_conditioned_transition_kind(const DecodeState& state);
std::size_t forward_state_keep(const DecodeState& state, std::size_t available);
std::size_t next_decode_window_start(const DecodeState& state, std::size_t vocab_size);

std::string decode_state_seed_digest(const DecodeState& state);
std::string candidate_window_seed_digest(const DecodeState& state);
std::string state_rationale_summary(std::string_view hidden_state_class,
                                    std::string_view carry_probe_layout_kind,
                                    std::string_view projection_carry_mode_kind,
                                    std::string_view selection_policy_kind,
                                    std::string_view decode_mode_kind);
void populate_probe_stability(DecodeProbe& result);
void apply_projection_carry_mode(DecodeProbe& result);
void populate_hidden_projection(DecodeProbe& result);


inline constexpr std::string_view kDecodeStateKind = "prompt_history_bounded_context.v1";

std::optional<IntermediateDecodeState> capture_intermediate_state(const DecodeState& state);
void apply_intermediate_state(DecodeState& state, const IntermediateDecodeState& intermediate_state);

std::vector<int> combined_decode_history(const DecodeState& state);
std::vector<int> decode_context_history(const DecodeState& state, std::size_t max_tokens);
std::vector<int> merged_state_input_rows(const std::vector<int>& forward_state_row_ids,
                                         const std::vector<int>& hidden_projection_row_ids,
                                         std::size_t max_rows);
std::string prefixed_history_digest(std::string_view prefix, const std::vector<int>& token_history);
std::string sampled_logits_digest(const std::vector<int>& token_ids, const std::vector<double>& token_scores);
std::size_t class_conditioned_hidden_projection_keep(std::string_view hidden_state_class, std::size_t available);
std::string hidden_carry_signature_digest(const DecodeProbe& result);
std::string tensor_signature_sha256(const t81::T729DynamicTensor& tensor);
std::string sha3_hex_text(std::string_view text);

// Add missing select_prompt_seeded_token_ids, make_contiguous_window_token_ids, history_seed_window_start
std::vector<int> select_prompt_seeded_token_ids(std::string_view prompt, std::size_t vocab_size,
                                                std::size_t sample_window,
                                                std::size_t* window_start = nullptr);
std::vector<int> make_contiguous_window_token_ids(std::size_t start, std::size_t vocab_size,
                                                  std::size_t sample_window);
std::size_t history_seed_window_start(const std::vector<int>& token_history, std::size_t vocab_size);


std::vector<int> recent_context_history(const std::vector<int>& token_history,
                                        std::size_t max_tokens,
                                        std::optional<int> anchor_token = std::nullopt);
std::vector<int> carry_probe_row_ids(std::size_t start, std::size_t vocab_size, std::size_t count,
                                     std::string_view carry_probe_layout_kind);

} // namespace t81::vm
