#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "t81/experimental/llama_cpp_adapter.hpp"

namespace {

std::string load_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: " << argv[0] << " <model.gguf> <policy.apl> <prompt>\n";
    return 2;
  }

  const std::string model_path = argv[1];
  const std::string policy_path = argv[2];
  const std::string prompt = argv[3];

  const std::string policy_text = load_file(policy_path);
  if (policy_text.empty()) {
    std::cerr << "failed to read policy file: " << policy_path << "\n";
    return 2;
  }

  auto adapter = t81::experimental::LlamaCppAdapter::create(model_path, policy_text);
  if (!adapter.has_value()) {
    std::cerr << "adapter init failed: " << adapter.error() << "\n";
    return 1;
  }

  t81::experimental::LlamaCppInferenceRequest req;
  req.prompt = prompt;
  req.max_tokens = 64;
  req.temperature = 0.0f;
  req.top_k = 1;
  req.top_p = 1.0f;
  req.seed = 0;
  req.n_threads = 1;

  auto receipt = adapter.value()->infer(req);
  if (!receipt.has_value()) {
    std::cerr << "inference failed: " << receipt.error() << "\n";
    return 1;
  }

  std::cout << "model_hash: " << receipt->model_hash << "\n";
  std::cout << "prompt_hash: " << receipt->prompt_hash << "\n";
  std::cout << "policy_reason: " << receipt->policy_reason << "\n";
  std::cout << "generated_tokens: " << receipt->token_ids.size() << "\n";
  std::cout << "text:\n" << receipt->text << "\n";

  return 0;
}
