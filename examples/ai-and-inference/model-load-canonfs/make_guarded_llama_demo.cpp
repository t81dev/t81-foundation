#include "t81/weights.hpp"

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: t81_make_guarded_llama_demo <out-dir>\n";
    return 1;
  }

  const fs::path out_dir = fs::path(argv[1]);
  const fs::path model_path = out_dir / "guarded-llama-demo.t81w";
  const fs::path tokenizer_path = out_dir / "tokenizer.json";

  std::error_code ec;
  fs::create_directories(out_dir, ec);
  if (ec) {
    std::cerr << "failed to create " << out_dir << "\n";
    return 1;
  }

  t81::weights::NativeModel model;
  const auto make_tensor = [](std::initializer_list<std::uint32_t> shape, std::uint64_t seed) {
    t81::weights::NativeTensor tensor;
    tensor.shape.assign(shape.begin(), shape.end());
    std::size_t element_count = 1;
    for (std::uint32_t dim : tensor.shape) {
      element_count *= static_cast<std::size_t>(dim);
    }
    tensor.trits = static_cast<std::uint64_t>(element_count);
    tensor.data.resize((element_count + 47u) / 48u, 0u);
    for (std::size_t i = 0; i < tensor.data.size(); ++i) {
      tensor.data[i] = seed + static_cast<std::uint64_t>(i * 17u);
    }
    return tensor;
  };

  model["model.embed_tokens.weight"] = make_tensor({64, 16}, 11u);
  model["model.norm.weight"] = make_tensor({16}, 23u);
  model["model.layers.0.self_attn.q_proj.weight"] = make_tensor({16, 16}, 101u);
  model["model.layers.0.self_attn.k_proj.weight"] = make_tensor({16, 16}, 211u);
  model["model.layers.0.self_attn.v_proj.weight"] = make_tensor({16, 16}, 307u);
  model["model.layers.0.self_attn.o_proj.weight"] = make_tensor({16, 16}, 401u);
  model["model.layers.0.mlp.gate_proj.weight"] = make_tensor({16, 16}, 503u);
  model["model.layers.0.mlp.up_proj.weight"] = make_tensor({16, 16}, 601u);
  model["model.layers.0.mlp.down_proj.weight"] = make_tensor({16, 16}, 701u);
  model["model.layers.1.self_attn.q_proj.weight"] = make_tensor({16, 16}, 809u);
  model["model.layers.1.self_attn.k_proj.weight"] = make_tensor({16, 16}, 907u);
  model["model.layers.1.self_attn.v_proj.weight"] = make_tensor({16, 16}, 1009u);
  model["lm_head.weight"] = make_tensor({64, 16}, 2003u);

  t81::weights::save_t81w(model, model_path);

  std::ofstream out(tokenizer_path);
  if (!out) {
    std::cerr << "failed to write " << tokenizer_path << "\n";
    return 1;
  }
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

  std::cout << "model=" << model_path << "\n";
  std::cout << "tokenizer=" << tokenizer_path << "\n";
  return 0;
}
