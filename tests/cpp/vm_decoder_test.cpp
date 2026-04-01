#include "t81/vm/decoder.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_path(const std::string& stem, const std::string& ext) {
  return fs::temp_directory_path() / (stem + ext);
}

t81::weights::NativeTensor make_tensor(std::initializer_list<std::uint32_t> shape) {
  t81::weights::NativeTensor tensor;
  tensor.shape.assign(shape.begin(), shape.end());
  std::size_t element_count = 1;
  for (std::uint32_t dim : tensor.shape) {
    element_count *= static_cast<std::size_t>(dim);
  }
  tensor.trits = static_cast<std::uint64_t>(element_count);
  tensor.data.assign((element_count + 47u) / 48u, 0u);
  return tensor;
}

int greedy_argmax_lowest_token_id(const t81::vm::DecodeProbe& probe) {
  assert(!probe.sampled_token_ids.empty());
  assert(probe.sampled_token_ids.size() == probe.sampled_token_scores.size());
  int best_token_id = probe.sampled_token_ids.front();
  double best_score = probe.sampled_token_scores.front();
  for (std::size_t i = 1; i < probe.sampled_token_ids.size(); ++i) {
    const int token_id = probe.sampled_token_ids[i];
    const double score = probe.sampled_token_scores[i];
    if (score > best_score || (score == best_score && token_id < best_token_id)) {
      best_score = score;
      best_token_id = token_id;
    }
  }
  return best_token_id;
}

}  // namespace

int main() {
  const fs::path model_dir = make_temp_path("t81-vm-decoder-model-dir", "");
  const fs::path model_path = model_dir / "decoder-model.t81w";
  const fs::path tokenizer_path = model_dir / "tokenizer.json";
  std::error_code ignore_ec;
  fs::create_directories(model_dir, ignore_ec);

  t81::weights::NativeModel model;
  model["model.embed_tokens.weight"] = make_tensor({64, 16});
  model["model.norm.weight"] = make_tensor({16});
  model["model.layers.0.self_attn.q_proj.weight"] = make_tensor({16, 16});
  model["model.layers.0.self_attn.k_proj.weight"] = make_tensor({16, 16});
  model["model.layers.0.self_attn.v_proj.weight"] = make_tensor({16, 16});
  model["model.layers.0.self_attn.o_proj.weight"] = make_tensor({16, 16});
  model["model.layers.0.mlp.gate_proj.weight"] = make_tensor({16, 16});
  model["model.layers.0.mlp.up_proj.weight"] = make_tensor({16, 16});
  model["model.layers.0.mlp.down_proj.weight"] = make_tensor({16, 16});
  model["model.layers.1.self_attn.q_proj.weight"] = make_tensor({16, 16});
  model["model.layers.1.self_attn.k_proj.weight"] = make_tensor({16, 16});
  model["model.layers.1.self_attn.v_proj.weight"] = make_tensor({16, 16});
  model["lm_head.weight"] = make_tensor({64, 16});

  t81::weights::save_t81w(model, model_path);
  {
    std::ofstream out(tokenizer_path);
    out << R"({
  "model": {
    "type": "BPE",
    "vocab": {
      "greet": 7,
      "hello": 11,
      "world": 12,
      "▁greet": 17,
      "▁hello": 21
    }
  }
}
)";
  }

  auto probe_model = std::make_shared<t81::weights::ModelFile>(t81::weights::load_t81w(model_path));
  const auto companions = t81::vm::find_model_companion_files(model_path);
  assert(companions.has_tokenizer);
  assert(companions.tokenizer_path == fs::absolute(tokenizer_path));

  const auto tokenizer_history_probe = t81::vm::run_native_vm_probe(
      {probe_model,
       t81::vm::detect_architecture_profile(*probe_model),
       "greet hello",
       companions.tokenizer_path,
       std::nullopt,
       std::nullopt,
       {},
       {},
       std::nullopt,
       std::nullopt,
       {},
       {},
       {},
       {},
       std::nullopt});
  assert(tokenizer_history_probe.ok);
  assert(tokenizer_history_probe.logits_row_probe_supported);
  assert(tokenizer_history_probe.tokenizer_seed_supported);
  assert(tokenizer_history_probe.candidate_selection_mode == "tokenizer_prompt_history");
  assert(tokenizer_history_probe.candidate_selection_basis ==
         "tokenizer_prompt_history_contiguous_window.v1");
  assert(tokenizer_history_probe.prompt_token_ids.size() == 2);
  assert(tokenizer_history_probe.prompt_token_ids.front() == 7);
  assert(tokenizer_history_probe.prompt_token_ids.back() == 11);
  assert(tokenizer_history_probe.sampled_token_ids.size() == 8);
  assert(tokenizer_history_probe.sampled_token_scores.size() == 8);
  assert(tokenizer_history_probe.selected_token_id.has_value());
  assert(tokenizer_history_probe.hidden_carry_row_ids.size() == 4);
  assert(tokenizer_history_probe.hidden_carry_scores.size() == 4);

  const auto prompt_fallback_probe = t81::vm::run_native_vm_probe(
      {probe_model,
       t81::vm::detect_architecture_profile(*probe_model),
       "greet_hello",
       companions.tokenizer_path,
       std::nullopt,
       std::nullopt,
       {},
       {},
       std::nullopt,
       std::nullopt,
       {},
       {},
       {},
       {},
       std::nullopt});
  assert(prompt_fallback_probe.ok);
  assert(prompt_fallback_probe.logits_row_probe_supported);
  assert(!prompt_fallback_probe.tokenizer_seed_supported);
  assert(prompt_fallback_probe.prompt_token_ids.empty());
  assert(prompt_fallback_probe.candidate_selection_mode == "prompt_seeded");
  assert(prompt_fallback_probe.candidate_selection_basis ==
         "prompt_sha3_seeded_contiguous_window.v1");
  assert(prompt_fallback_probe.sampled_token_ids.size() == 8);
  assert(prompt_fallback_probe.sampled_token_scores.size() == 8);
  assert(prompt_fallback_probe.selected_token_id.has_value());
  assert(prompt_fallback_probe.hidden_carry_row_ids.size() == 4);
  assert(prompt_fallback_probe.hidden_carry_scores.size() == 4);

  auto run_manual_public_fallback_loop = [&](std::optional<fs::path> tokenizer_override) {
    t81::vm::DecodeState manual_state;
    auto manual_probe = t81::vm::run_native_vm_probe(
        {probe_model,
         t81::vm::detect_architecture_profile(*probe_model),
         "greet_hello",
         tokenizer_override,
         std::nullopt,
         std::nullopt,
         {},
         {},
         std::nullopt,
         std::nullopt,
         {},
         {},
         {},
         {},
         std::nullopt});
    assert(manual_probe.ok);
    assert(manual_probe.selected_token_id.has_value());
    assert(manual_probe.selected_token_score.has_value());
    auto manual_transition = t81::vm::derive_initial_transition(manual_probe);
    t81::vm::apply_state_transition(manual_state, manual_transition);
    assert(manual_state.kv_state_carry_mode_kind == "evolved_qk_signature.v1");
    assert(manual_state.intermediate_state.has_value());
    assert(manual_state.intermediate_state->hidden_tensor.has_value());

    const auto manual_context_step1 = t81::vm::decode_context_history(
        manual_state, manual_state.config().decode_context_history_window);
    manual_state.seed_token_id = static_cast<int>(
        t81::vm::next_decode_window_start(manual_state, manual_probe.logits_vocab_size));
    auto manual_request = t81::vm::make_decode_probe_request(
        probe_model, t81::vm::detect_architecture_profile(*probe_model), "greet_hello",
        manual_state, manual_probe.logits_vocab_size, tokenizer_override,
        manual_probe.tokenizer_seed_supported);
    manual_request.context_token_history = manual_context_step1;
    manual_probe = t81::vm::run_native_vm_probe(manual_request);
    assert(manual_probe.ok);
    assert(manual_probe.selected_token_id.has_value());
    manual_transition = t81::vm::derive_probe_transition(
        manual_state, manual_probe, t81::vm::stability_conditioned_transition_kind(manual_state));
    t81::vm::apply_state_transition(manual_state, manual_transition);
    assert(manual_state.kv_state_carry_mode_kind == "evolved_qk_signature.v1");

    const auto manual_context_step2 = t81::vm::decode_context_history(
        manual_state, manual_state.config().decode_context_history_window);
    manual_state.seed_token_id = static_cast<int>(
        t81::vm::next_decode_window_start(manual_state, manual_probe.logits_vocab_size));
    manual_request = t81::vm::make_decode_probe_request(
        probe_model, t81::vm::detect_architecture_profile(*probe_model), "greet_hello",
        manual_state, manual_probe.logits_vocab_size, tokenizer_override,
        manual_probe.tokenizer_seed_supported);
    manual_request.context_token_history = manual_context_step2;
    manual_probe = t81::vm::run_native_vm_probe(manual_request);
    assert(manual_probe.ok);
    assert(manual_probe.selected_token_id.has_value());
    manual_transition = t81::vm::derive_probe_transition(
        manual_state, manual_probe, t81::vm::stability_conditioned_transition_kind(manual_state));
    t81::vm::apply_state_transition(manual_state, manual_transition);
    assert(manual_state.kv_state_carry_mode_kind ==
           "architecture_state_evolved_qk_signature.v1");
    assert(manual_state.architecture_state_kind ==
           "bounded_hidden_tensor_qk_forward_state.v1");
    return manual_state;
  };

  const auto manual_state_without_tokenizer = run_manual_public_fallback_loop(std::nullopt);
  const auto manual_state_with_unmatched_tokenizer =
      run_manual_public_fallback_loop(companions.tokenizer_path);
  assert(manual_state_with_unmatched_tokenizer.kv_state_carry_mode_kind ==
         manual_state_without_tokenizer.kv_state_carry_mode_kind);
  assert(manual_state_with_unmatched_tokenizer.architecture_state_kind ==
         manual_state_without_tokenizer.architecture_state_kind);
  assert(manual_state_with_unmatched_tokenizer.forward_state_generation ==
         manual_state_without_tokenizer.forward_state_generation);
  assert(manual_state_with_unmatched_tokenizer.intermediate_state.has_value());
  assert(manual_state_with_unmatched_tokenizer.intermediate_state->hidden_tensor.has_value());
  assert(manual_state_with_unmatched_tokenizer.intermediate_state->kv_state_carry_mode_kind ==
         "architecture_state_evolved_qk_signature.v1");

  auto greedy_manual_probe = t81::vm::run_native_vm_probe(
      t81::vm::make_initial_greedy_probe_request(
          probe_model, t81::vm::detect_architecture_profile(*probe_model), "greet hello"));
  assert(greedy_manual_probe.ok);
  assert(greedy_manual_probe.selection_policy_kind ==
         "greedy_argmax_lowest_token_id.v1");
  assert(greedy_manual_probe.candidate_selection_mode == "greedy_full_vocab");
  assert(greedy_manual_probe.candidate_selection_basis == "full_vocab_logits_argmax.v1");
  assert(greedy_manual_probe.logits_vocab_size == 64);
  assert(greedy_manual_probe.sampled_token_ids.size() == greedy_manual_probe.logits_vocab_size);
  assert(greedy_manual_probe.sampled_token_scores.size() == greedy_manual_probe.logits_vocab_size);
  assert(greedy_manual_probe.selected_token_id.has_value());
  assert(*greedy_manual_probe.selected_token_id ==
         greedy_argmax_lowest_token_id(greedy_manual_probe));

  t81::vm::DecodeState greedy_manual_state;
  auto greedy_manual_transition = t81::vm::derive_initial_transition(greedy_manual_probe);
  t81::vm::apply_state_transition(greedy_manual_state, greedy_manual_transition);
  assert(greedy_manual_state.intermediate_state.has_value());
  assert(greedy_manual_state.intermediate_state->hidden_tensor.has_value());

  auto greedy_manual_request = t81::vm::make_greedy_decode_probe_request(
      probe_model, t81::vm::detect_architecture_profile(*probe_model), "greet hello",
      greedy_manual_state, greedy_manual_probe.logits_vocab_size);
  greedy_manual_probe = t81::vm::run_native_vm_probe(greedy_manual_request);
  assert(greedy_manual_probe.ok);
  assert(greedy_manual_probe.selection_policy_kind ==
         "greedy_argmax_lowest_token_id.v1");
  assert(greedy_manual_probe.candidate_selection_mode == "greedy_full_vocab");
  assert(greedy_manual_probe.candidate_selection_basis == "full_vocab_logits_argmax.v1");
  assert(greedy_manual_probe.sampled_token_ids.size() ==
         greedy_manual_probe.logits_vocab_size);
  assert(greedy_manual_probe.selected_token_id.has_value());
  assert(*greedy_manual_probe.selected_token_id ==
         greedy_argmax_lowest_token_id(greedy_manual_probe));
  greedy_manual_transition = t81::vm::derive_probe_transition(
      greedy_manual_state, greedy_manual_probe, "narrow_greedy_state_transition.v1");
  t81::vm::apply_state_transition(greedy_manual_state, greedy_manual_transition);
  assert(greedy_manual_state.generated_token_history.size() == 2);
  assert(greedy_manual_state.intermediate_state.has_value());
  assert(greedy_manual_state.intermediate_state->hidden_tensor.has_value());

  t81::vm::Decoder decoder;
  decoder.load_model(model_path);
  assert(decoder.has_model());
  assert(!decoder.terminated());

  const auto first = decoder.step({.prompt = "greet hello"});
  assert(first.ok);
  assert(first.step == 0);
  assert(first.probe.ok);
  assert(first.probe.selected_token_id.has_value());
  assert(first.transition.generated_token_history.size() == 1);
  assert(first.transition.hidden_tensor_carry_mode_kind == "current_only.v1");
  assert(first.transition.kv_state_kind == "bounded_qk_tensor_state.v1");
  assert(decoder.state().generated_token_history.size() == 1);
  assert(decoder.state().forward_state_kind == "projection_carried_forward_state.v1");
  assert(!decoder.state().hidden_tensor_signature_sha256.empty());
  assert(decoder.state().intermediate_state.has_value());
  assert(decoder.state().intermediate_state->hidden_tensor.has_value());

  const auto second = decoder.step();
  assert(second.ok);
  assert(second.step == 1);
  assert(second.probe.ok);
  assert(second.probe.selected_token_id.has_value());
  assert(second.transition.generated_token_history.size() == 2);
  assert(second.transition.hidden_tensor_carry_mode_kind ==
         "evolved_hidden_tensor_feedback.v1");
  assert(second.transition.kv_state_kind == "bounded_qk_tensor_state.v1");
  assert(decoder.state().generated_token_history.size() == 2);
  assert(decoder.state().hidden_tensor_carry_mode_kind ==
         "evolved_hidden_tensor_feedback.v1");
  assert(decoder.state().kv_state_kind == "bounded_qk_tensor_state.v1");
  assert(decoder.state().intermediate_state.has_value());
  assert(decoder.state().intermediate_state->hidden_tensor.has_value());

  decoder.reset();
  const auto fallback_first = decoder.step({.prompt = "greet_hello"});
  assert(fallback_first.ok);
  assert(fallback_first.probe.ok);
  assert(fallback_first.probe.candidate_selection_mode == "prompt_seeded");
  assert(fallback_first.probe.candidate_selection_basis ==
         "prompt_sha3_seeded_contiguous_window.v1");
  assert(!fallback_first.probe.tokenizer_seed_supported);
  assert(fallback_first.probe.prompt_token_ids.empty());
  assert(decoder.state().kv_state_carry_mode_kind == "evolved_qk_signature.v1");

  const auto fallback_second = decoder.step();
  assert(fallback_second.ok);
  assert(fallback_second.probe.ok);
  assert(decoder.state().kv_state_carry_mode_kind == "evolved_qk_signature.v1");

  const auto fallback_third = decoder.step();
  assert(fallback_third.ok);
  assert(fallback_third.probe.ok);
  assert(fallback_third.transition_kind ==
         "architecture_state_feedback_state_transition.v1");
  assert(decoder.state().kv_state_carry_mode_kind ==
         "architecture_state_evolved_qk_signature.v1");
  assert(decoder.state().architecture_state_kind ==
         "bounded_hidden_tensor_qk_forward_state.v1");
  assert(!decoder.state().architecture_state_signature_sha256.empty());

  decoder.reset();
  assert(!decoder.terminated());
  assert(decoder.termination_reason().empty());
  assert(decoder.state().generated_token_history.empty());
  assert(decoder.state().prompt_token_history.empty());
  assert(decoder.state().hidden_tensor_signature_sha256.empty());

  decoder.reset();
  assert(decoder.supports_narrow_greedy_decode());
  const auto greedy_first = decoder.greedy_step({.prompt = "greet hello"});
  assert(greedy_first.ok);
  assert(greedy_first.decode_mode_kind == "narrow_greedy_llama_dense_v1");
  assert(greedy_first.transition_kind ==
         "prompt_seed_to_narrow_greedy_decode_state.v1");
  assert(greedy_first.probe.ok);
  assert(greedy_first.probe.selected_token_id.has_value());
  assert(greedy_first.probe.selection_policy_kind ==
         "greedy_argmax_lowest_token_id.v1");
  assert(greedy_first.probe.candidate_selection_mode == "greedy_full_vocab");
  assert(greedy_first.probe.candidate_selection_basis == "full_vocab_logits_argmax.v1");
  assert(greedy_first.probe.sampled_token_ids.size() ==
         greedy_first.probe.logits_vocab_size);
  assert(greedy_first.probe.selected_token_id.value() ==
         greedy_argmax_lowest_token_id(greedy_first.probe));
  assert(decoder.mode() == t81::vm::DecoderMode::NarrowGreedyLlamaDenseV1);
  assert(decoder.state().generated_token_history.size() == 1);
  assert(decoder.state().intermediate_state.has_value());
  assert(decoder.state().intermediate_state->hidden_tensor.has_value());

  const auto greedy_second = decoder.greedy_step();
  assert(greedy_second.ok);
  assert(greedy_second.decode_mode_kind == "narrow_greedy_llama_dense_v1");
  assert(greedy_second.transition_kind == "narrow_greedy_state_transition.v1");
  assert(greedy_second.probe.ok);
  assert(greedy_second.probe.selected_token_id.has_value());
  assert(greedy_second.probe.selection_policy_kind ==
         "greedy_argmax_lowest_token_id.v1");
  assert(greedy_second.probe.candidate_selection_mode == "greedy_full_vocab");
  assert(greedy_second.probe.selected_token_id.value() ==
         greedy_argmax_lowest_token_id(greedy_second.probe));
  assert(decoder.state().generated_token_history.size() == 2);
  assert(decoder.state().intermediate_state.has_value());
  assert(decoder.state().intermediate_state->hidden_tensor.has_value());

  const auto greedy_third = decoder.greedy_step();
  assert(greedy_third.ok);
  assert(greedy_third.probe.ok);
  assert(greedy_third.probe.selected_token_id.has_value());
  assert(greedy_third.probe.selected_token_id.value() ==
         greedy_argmax_lowest_token_id(greedy_third.probe));
  assert(decoder.state().generated_token_history.size() == 3);
  assert(decoder.state().intermediate_state.has_value());
  assert(decoder.state().intermediate_state->hidden_tensor.has_value());
  assert(decoder.state().generated_token_history.front() ==
         greedy_first.probe.selected_token_id.value());
  assert(decoder.state().generated_token_history[1] ==
         greedy_second.probe.selected_token_id.value());

  fs::remove(tokenizer_path, ignore_ec);
  fs::remove(model_path, ignore_ec);
  fs::remove(model_dir, ignore_ec);
  return 0;
}
