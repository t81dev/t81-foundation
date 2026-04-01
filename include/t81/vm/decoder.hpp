#pragma once

#include "t81/vm/decode_state.hpp"
#include "t81/weights.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace t81::vm {

enum class DecoderMode {
  BoundedProbe,
  NarrowGreedyLlamaDenseV1,
};

struct DecoderInput {
  std::string prompt;
};

struct ModelCompanionFiles {
  bool has_config = false;
  bool has_tokenizer = false;
  std::filesystem::path config_path;
  std::filesystem::path tokenizer_path;
};

struct NativeProbeRequest {
  std::shared_ptr<t81::weights::ModelFile> model;
  std::string architecture_profile;
  std::string prompt;
  std::optional<std::filesystem::path> tokenizer_path;
  std::optional<int> candidate_window_seed;
  std::optional<int> input_token_override;
  std::vector<int> context_token_history;
  std::vector<int> hidden_carry_context_rows;
  std::optional<IntermediateDecodeState> intermediate_state;
  std::optional<std::size_t> sample_window_override;
  std::string selection_policy_override;
  std::string carry_probe_layout_override;
  std::string candidate_mode_override;
  std::string candidate_basis_override;
  std::optional<bool> tokenizer_seed_supported_override;
  bool greedy_full_vocab = false;
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
  std::optional<IntermediateDecodeState> intermediate_state;
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

std::string detect_architecture_profile(const t81::weights::ModelFile& model);
ModelCompanionFiles find_model_companion_files(const std::filesystem::path& model_path);
NativeProbeRequest make_initial_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    std::optional<std::filesystem::path> tokenizer_path = std::nullopt);
NativeProbeRequest make_initial_greedy_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    std::optional<std::filesystem::path> tokenizer_path = std::nullopt);
NativeProbeRequest make_decode_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    const DecodeState& state,
    std::size_t logits_vocab_size,
    std::optional<std::filesystem::path> tokenizer_path = std::nullopt,
    std::optional<bool> tokenizer_seed_supported_override = std::nullopt);
NativeProbeRequest make_greedy_decode_probe_request(
    const std::shared_ptr<t81::weights::ModelFile>& model,
    std::string_view architecture_profile,
    std::string_view prompt,
    const DecodeState& state,
    std::size_t logits_vocab_size,
    std::optional<std::filesystem::path> tokenizer_path = std::nullopt,
    std::optional<bool> tokenizer_seed_supported_override = std::nullopt);
DecodeProbe run_native_vm_probe(const NativeProbeRequest& request);

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
  DecoderStepResult greedy_step(const DecoderInput& input = {});

  const DecodeState& state() const { return state_; }
  bool has_model() const { return static_cast<bool>(model_); }
  bool terminated() const { return terminated_; }
  std::string_view termination_reason() const { return termination_reason_; }
  DecoderMode mode() const { return mode_; }
  bool supports_narrow_greedy_decode() const;

private:
  enum class DecodePhase {
    Initialization,
    Decoding,
    Terminated,
  };

  DecoderStepResult step_impl(DecoderMode mode, const DecoderInput& input);

  std::shared_ptr<t81::weights::ModelFile> model_;
  std::string architecture_profile_;
  std::string prompt_;
  DecodeConfig config_;
  DecodeState state_;
  DecodePhase phase_ = DecodePhase::Initialization;
  DecoderMode mode_ = DecoderMode::BoundedProbe;
  std::size_t steps_emitted_ = 0;
  std::size_t consecutive_recovery_steps_ = 0;
  std::size_t logits_vocab_size_ = 0;
  bool terminated_ = false;
  std::string termination_reason_;
};

}  // namespace t81::vm
