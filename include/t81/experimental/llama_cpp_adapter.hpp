#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "t81/support/expected.hpp"

namespace t81::experimental {

struct LlamaCppInferenceRequest {
  std::string prompt;
  std::string expected_model_hash;
  std::uint32_t seed{0};
  int32_t max_tokens{128};
  int32_t top_k{1};
  float top_p{1.0f};
  float temperature{0.0f};
  int32_t n_threads{1};
};

struct LlamaCppInferenceReceipt {
  std::string model_hash;
  std::string prompt_hash;
  std::vector<int32_t> token_ids;
  std::string text;
  std::string policy_reason;
  bool policy_allowed{false};
};

class LlamaCppAdapter {
public:
  static t81::expected<std::unique_ptr<LlamaCppAdapter>, std::string> create(
      const std::filesystem::path& model_path, std::string policy_text);

  ~LlamaCppAdapter();

  t81::expected<LlamaCppInferenceReceipt, std::string> infer(const LlamaCppInferenceRequest& req);

  const std::string& model_hash() const { return model_hash_; }

public:
  LlamaCppAdapter() = default;

  void* model_{nullptr};
  void* ctx_{nullptr};
  std::string model_hash_;
  std::string policy_text_;
  std::string model_path_;
};

}  // namespace t81::experimental
