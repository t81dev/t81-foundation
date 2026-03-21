#include "t81/experimental/llama_cpp_adapter.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

#include <llama.h>

#include "t81/axion/context.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/opcodes.hpp"

namespace t81::experimental {
namespace {

using ModelPtr = llama_model*;
using ContextPtr = llama_context*;

void silent_llama_log_callback(ggml_log_level, const char*, void*) {}

std::string sha3_512_prefixed(std::span<const std::uint8_t> bytes) {
  return "sha3-512:" + t81::crypto::sha3_512_hex(bytes);
}

std::string hash_string(std::string_view text) {
  auto ptr = reinterpret_cast<const std::uint8_t*>(text.data());
  return sha3_512_prefixed(std::span<const std::uint8_t>(ptr, text.size()));
}

namespace sha3_streaming {
constexpr uint64_t kKeccakfRoundConstants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};

constexpr int kKeccakfRotc[] = {1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
                                27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44};

constexpr int kKeccakfPiln[] = {10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
                                15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1};

inline uint64_t rol(uint64_t value, int offset) noexcept {
  return (value << offset) | (value >> (64 - offset));
}

inline uint64_t load64(const uint8_t* data) noexcept {
  uint64_t value;
  std::memcpy(&value, data, sizeof(value));
  return value;
}

inline void store64(uint8_t* out, uint64_t value) noexcept {
  std::memcpy(out, &value, sizeof(value));
}

void keccakf(uint64_t state[25]) noexcept {
  for (int round = 0; round < 24; ++round) {
    uint64_t bc[5];
    for (int i = 0; i < 5; ++i) {
      bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
    }
    for (int i = 0; i < 5; ++i) {
      uint64_t temp = bc[(i + 4) % 5] ^ rol(bc[(i + 1) % 5], 1);
      for (int j = 0; j < 25; j += 5) {
        state[j + i] ^= temp;
      }
    }
    uint64_t temp = state[1];
    for (int i = 0; i < 24; ++i) {
      int j = kKeccakfPiln[i];
      uint64_t t = state[j];
      state[j] = rol(temp, kKeccakfRotc[i]);
      temp = t;
    }
    for (int i = 0; i < 5; ++i) {
      for (int j = 0; j < 25; j += 5) {
        uint64_t a = state[j + i];
        uint64_t b = state[j + ((i + 1) % 5)];
        state[j + i] = a ^ ((~b) & state[j + ((i + 2) % 5)]);
      }
    }
    state[0] ^= kKeccakfRoundConstants[round];
  }
}
}  // namespace sha3_streaming

class Sha3_512_Stream {
public:
  void update(std::span<const uint8_t> input) noexcept {
    constexpr size_t kRate = 72;
    size_t offset = 0;
    while (offset < input.size()) {
      size_t to_copy = std::min(kRate - buffered_, input.size() - offset);
      std::memcpy(block_.data() + buffered_, input.data() + offset, to_copy);
      buffered_ += to_copy;
      offset += to_copy;
      if (buffered_ == kRate) {
        absorb_block(block_.data());
        buffered_ = 0;
        block_.fill(0);
      }
    }
  }

  [[nodiscard]] std::array<uint8_t, 64> finalize() noexcept {
    constexpr size_t kRate = 72;
    block_[buffered_] = 0x06;
    block_[kRate - 1] |= 0x80;
    absorb_block(block_.data());

    std::array<uint8_t, 64> digest{};
    size_t produced = 0;
    while (produced < digest.size()) {
      for (size_t lane = 0; lane < kRate / 8 && produced < digest.size(); ++lane) {
        uint8_t lane_buffer[8];
        sha3_streaming::store64(lane_buffer, state_[lane]);
        for (size_t i = 0; i < 8 && produced < digest.size(); ++i) {
          digest[produced++] = lane_buffer[i];
        }
      }
      if (produced < digest.size()) {
        sha3_streaming::keccakf(state_.data());
      }
    }
    return digest;
  }

private:
  void absorb_block(const uint8_t* data) noexcept {
    constexpr size_t kRate = 72;
    for (size_t lane = 0; lane < kRate / 8; ++lane) {
      state_[lane] ^= sha3_streaming::load64(data + lane * 8);
    }
    sha3_streaming::keccakf(state_.data());
  }

  std::array<uint64_t, 25> state_{};
  std::array<uint8_t, 72> block_{};
  size_t buffered_ = 0;
};

std::string hash_file(const std::filesystem::path& file_path) {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    return {};
  }

  Sha3_512_Stream hasher;
  std::array<std::uint8_t, 1 << 20> chunk{};
  while (input) {
    input.read(reinterpret_cast<char*>(chunk.data()),
               static_cast<std::streamsize>(chunk.size()));
    const auto read_count = input.gcount();
    if (read_count <= 0) {
      break;
    }
    hasher.update(std::span<const std::uint8_t>(chunk.data(), static_cast<size_t>(read_count)));
  }

  const auto digest = hasher.finalize();
  static constexpr char kHex[] = "0123456789abcdef";
  std::string hex;
  hex.reserve(digest.size() * 2);
  for (std::uint8_t byte : digest) {
    hex.push_back(kHex[byte >> 4]);
    hex.push_back(kHex[byte & 0x0f]);
  }
  return "sha3-512:" + hex;
}

t81::expected<t81::axion::Policy, std::string> parse_policy_strict(const std::string& text) {
  if (text.empty()) {
    return t81::make_unexpected("policy text is required");
  }
  auto policy = t81::axion::parse_policy(text);
  if (!policy.has_value()) {
    return t81::make_unexpected(policy.error());
  }
  return policy.value();
}

t81::expected<std::string, std::string> detokenize_piece(const llama_vocab* vocab,
                                                         llama_token token) {
  char small_buf[64];
  int32_t n = llama_token_to_piece(vocab, token, small_buf, static_cast<int32_t>(sizeof(small_buf)),
                                   0, true);
  if (n >= 0) {
    return std::string(small_buf, static_cast<size_t>(n));
  }

  const int32_t need = -n;
  if (need <= 0 || need > 32768) {
    return t81::make_unexpected("invalid detokenize size from llama.cpp");
  }
  std::vector<char> large_buf(static_cast<size_t>(need));
  n = llama_token_to_piece(vocab, token, large_buf.data(), need, 0, true);
  if (n < 0) {
    return t81::make_unexpected("failed to convert token to text");
  }
  return std::string(large_buf.data(), static_cast<size_t>(n));
}

}  // namespace

t81::expected<std::unique_ptr<LlamaCppAdapter>, std::string> LlamaCppAdapter::create(
    const std::filesystem::path& model_path, std::string policy_text) {
  if (!std::filesystem::exists(model_path)) {
    return t81::make_unexpected("model file not found: " + model_path.string());
  }

  auto parsed_policy = parse_policy_strict(policy_text);
  if (!parsed_policy.has_value()) {
    return t81::make_unexpected("policy parse failed: " + parsed_policy.error());
  }

  llama_backend_init();
  llama_log_set(silent_llama_log_callback, nullptr);

  llama_model_params model_params = llama_model_default_params();
  ModelPtr model = llama_model_load_from_file(model_path.string().c_str(), model_params);
  if (model == nullptr) {
    llama_backend_free();
    return t81::make_unexpected("llama_model_load_from_file failed");
  }

  llama_context_params ctx_params = llama_context_default_params();
  ctx_params.n_ctx = 4096;
  ctx_params.n_batch = 1024;
  ContextPtr ctx = llama_init_from_model(model, ctx_params);
  if (ctx == nullptr) {
    llama_model_free(model);
    llama_backend_free();
    return t81::make_unexpected("llama_init_from_model failed");
  }

  std::string model_hash = hash_file(model_path);
  if (model_hash.empty()) {
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();
    return t81::make_unexpected("failed to hash model file");
  }

  auto adapter = std::unique_ptr<LlamaCppAdapter>(new LlamaCppAdapter());
  adapter->model_ = model;
  adapter->ctx_ = ctx;
  adapter->model_hash_ = std::move(model_hash);
  adapter->policy_text_ = std::move(policy_text);
  return adapter;
}

LlamaCppAdapter::~LlamaCppAdapter() {
  if (ctx_ != nullptr) {
    llama_free(static_cast<ContextPtr>(ctx_));
  }
  if (model_ != nullptr) {
    llama_model_free(static_cast<ModelPtr>(model_));
  }
  llama_backend_free();
}

t81::expected<LlamaCppInferenceReceipt, std::string> LlamaCppAdapter::infer(
    const LlamaCppInferenceRequest& req) {
  if (req.prompt.empty()) {
    return t81::make_unexpected("prompt must not be empty");
  }
  if (req.max_tokens <= 0) {
    return t81::make_unexpected("max_tokens must be > 0");
  }

  auto parsed_policy = parse_policy_strict(policy_text_);
  if (!parsed_policy.has_value()) {
    return t81::make_unexpected("policy parse failed: " + parsed_policy.error());
  }

  if (!req.expected_model_hash.empty() && req.expected_model_hash != model_hash_) {
    return t81::make_unexpected("expected_model_hash mismatch");
  }

  t81::axion::SyscallContext syscall_ctx;
  syscall_ctx.caller = "t81.llama_cpp_adapter";
  syscall_ctx.syscall = "llama.cpp.infer";
  syscall_ctx.payload = model_hash_;
  syscall_ctx.next_opcode = t81::tisc::Opcode::TLoadHash;
  syscall_ctx.current_tier = parsed_policy->tier;
  syscall_ctx.policy = &(*parsed_policy);

  auto policy_engine = t81::axion::make_policy_engine(*parsed_policy);
  auto verdict = policy_engine->evaluate(syscall_ctx);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    return t81::make_unexpected("policy denied inference: " + verdict.reason);
  }

  auto* model = static_cast<ModelPtr>(model_);
  auto* ctx = static_cast<ContextPtr>(ctx_);
  auto* vocab = llama_model_get_vocab(model);

  llama_set_n_threads(ctx, std::max(1, req.n_threads), std::max(1, req.n_threads));
  llama_memory_clear(llama_get_memory(ctx), true);

  std::vector<llama_token> prompt_tokens(req.prompt.size() + 32);
  int32_t n_prompt = llama_tokenize(vocab, req.prompt.c_str(), static_cast<int32_t>(req.prompt.size()),
                                    prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()),
                                    true, true);
  if (n_prompt < 0) {
    prompt_tokens.resize(static_cast<size_t>(-n_prompt));
    n_prompt = llama_tokenize(vocab, req.prompt.c_str(), static_cast<int32_t>(req.prompt.size()),
                              prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()),
                              true, true);
  }
  if (n_prompt <= 0) {
    return t81::make_unexpected("failed to tokenize prompt");
  }
  prompt_tokens.resize(static_cast<size_t>(n_prompt));

  llama_batch prompt_batch = llama_batch_get_one(prompt_tokens.data(), n_prompt);
  if (llama_decode(ctx, prompt_batch) != 0) {
    return t81::make_unexpected("llama_decode failed on prompt");
  }

  auto sampler_params = llama_sampler_chain_default_params();
  llama_sampler* sampler = llama_sampler_chain_init(sampler_params);
  if (sampler == nullptr) {
    return t81::make_unexpected("failed to create sampler chain");
  }

  if (req.top_k > 1) {
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(req.top_k));
  }
  if (req.top_p < 1.0f) {
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(req.top_p, 1));
  }
  if (req.temperature > 0.0f) {
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(req.temperature));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(req.seed));
  } else {
    llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
  }

  LlamaCppInferenceReceipt receipt;
  receipt.model_hash = model_hash_;
  receipt.prompt_hash = hash_string(req.prompt);
  receipt.policy_reason = verdict.reason;
  receipt.policy_allowed = true;

  for (int i = 0; i < req.max_tokens; ++i) {
    llama_token token = llama_sampler_sample(sampler, ctx, -1);
    if (token == LLAMA_TOKEN_NULL) {
      llama_sampler_free(sampler);
      return t81::make_unexpected("sampler produced LLAMA_TOKEN_NULL");
    }

    llama_sampler_accept(sampler, token);
    if (llama_vocab_is_eog(vocab, token)) {
      break;
    }

    receipt.token_ids.push_back(token);

    auto piece = detokenize_piece(vocab, token);
    if (!piece.has_value()) {
      llama_sampler_free(sampler);
      return t81::make_unexpected(piece.error());
    }
    receipt.text += piece.value();

    llama_token token_buf[1] = {token};
    llama_batch step_batch = llama_batch_get_one(token_buf, 1);
    if (llama_decode(ctx, step_batch) != 0) {
      llama_sampler_free(sampler);
      return t81::make_unexpected("llama_decode failed during generation");
    }
  }

  llama_sampler_free(sampler);
  return receipt;
}

}  // namespace t81::experimental
