#pragma once

#include "t81/vm/decode_state.hpp"
#include "t81/weights.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace t81::vm {

struct DecoderInput {
  std::string prompt;
};

struct StateTransition {
  std::vector<int> prompt_token_history;
  std::vector<int> generated_token_history;
  std::optional<int> prompt_anchor_token_id;
  int input_token_id = 0;
  std::optional<int> seed_token_id;
  std::size_t window_start = 0;
  std::vector<int> hidden_carry_row_ids;
  std::vector<double> hidden_carry_scores;
  std::string hidden_carry_signature_sha256;
  std::string carry_probe_layout_kind;
  std::vector<int> hidden_projection_row_ids;
  std::vector<double> hidden_projection_scores;
  std::string hidden_projection_signature_sha256;
  std::string projection_carry_mode_kind;
  std::string hidden_state_class;
  std::string hidden_state_class_signature_sha256;
  std::string hidden_tensor_signature_sha256;
  std::size_t hidden_tensor_rank = 0;
  std::size_t hidden_tensor_elements = 0;
  std::vector<int> hidden_tensor_shape;
  std::optional<t81::T729DynamicTensor> carried_hidden_tensor;
  std::string hidden_tensor_carry_mode_kind = "unavailable";
  std::string kv_state_kind;
  std::string q_tensor_signature_sha256;
  std::string k_tensor_signature_sha256;
  std::size_t kv_tensor_rank = 0;
  std::size_t kv_tensor_elements = 0;
  std::string kv_state_signature_sha256;
  std::string selection_policy_kind;
  double confidence_score = 0.0;
  double logits_margin = 0.0;
  double hidden_carry_peak = 0.0;
  std::string stability_kind;
};

struct DecoderStepResult {
  bool ok = false;
  std::size_t step = 0;
  std::string transition_kind;
  std::string decode_mode_kind;
  std::string termination_reason;
  bool terminated = false;
  StateTransition transition;
  DecodeProbe probe;
};

StateTransition derive_initial_transition(const DecodeProbe& probe);
StateTransition derive_probe_transition(const DecodeState& state,
                                        const DecodeProbe& probe,
                                        std::string_view transition_kind);
void apply_state_transition(DecodeState& state, const StateTransition& transition);

class Decoder {
public:
  explicit Decoder(DecodeConfig config = DecodeConfig{});

  void load_model(const std::filesystem::path& model_path);
  void load_model(t81::weights::ModelFile model);
  void reset();

  DecoderStepResult step(const DecoderInput& input = {});

  const DecodeState& state() const { return state_; }
  bool has_model() const { return static_cast<bool>(model_); }
  bool terminated() const { return terminated_; }
  std::string_view termination_reason() const { return termination_reason_; }

private:
  enum class DecodePhase {
    Initialization,
    Decoding,
    Terminated,
  };

  std::shared_ptr<t81::weights::ModelFile> model_;
  std::string architecture_profile_;
  std::string prompt_;
  DecodeConfig config_;
  DecodeState state_;
  DecodePhase phase_ = DecodePhase::Initialization;
  std::size_t steps_emitted_ = 0;
  std::size_t consecutive_recovery_steps_ = 0;
  std::size_t logits_vocab_size_ = 0;
  bool terminated_ = false;
  std::string termination_reason_;
};

}  // namespace t81::vm
