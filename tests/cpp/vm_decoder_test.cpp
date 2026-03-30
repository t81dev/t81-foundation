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

}  // namespace

int main() {
  const fs::path model_path = make_temp_path("t81-vm-decoder-model", ".t81w");
  std::error_code ignore_ec;

  t81::weights::NativeModel model;
  model["model.embed_tokens.weight"] = make_tensor({32, 16});
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

  decoder.reset();
  assert(!decoder.terminated());
  assert(decoder.termination_reason().empty());
  assert(decoder.state().generated_token_history.empty());
  assert(decoder.state().prompt_token_history.empty());
  assert(decoder.state().hidden_tensor_signature_sha256.empty());

  fs::remove(model_path, ignore_ec);
  return 0;
}
