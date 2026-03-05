#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include "t81/support/expected_minimal.hpp"

// Forward declarations
namespace t81::codec {
    struct GGUTensor;
}

namespace t81::experimental {

struct LlamaCppInferenceRequest {
    std::string prompt;
    int max_tokens = 150;
    float temperature = 0.7f;
    int reasoning_level = 3;
    bool enable_ternary_quantization = true;
    std::string expected_model_hash;
};

struct LlamaCppInferenceReceipt {
    std::string text;
    std::vector<int> token_ids;
    bool policy_allowed{false};
    std::string policy_reason;
};

class LlamaCppAdapter {
public:
  static std::optional<std::unique_ptr<LlamaCppAdapter>> create(
      const std::filesystem::path& model_path, std::string policy_text);

  ~LlamaCppAdapter();

  std::optional<LlamaCppInferenceReceipt> infer(const LlamaCppInferenceRequest& req);

  const std::string& model_hash() const { return model_hash_; }

public:
  LlamaCppAdapter() = default;

  void* model_{nullptr};
  void* ctx_{nullptr};
  std::string model_hash_;
  std::string policy_text_;
  std::string model_path_;
  std::vector<t81::codec::GGUTensor> tensors_;
};

}  // namespace t81::experimental
