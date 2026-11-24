#include "provider/controlled_provider.hpp"

#include <array>
#include <chrono>
#include <random>
#include <sstream>
#include <thread>

#include "t81/config.hpp"

namespace t81::provider {

ControlledProvider::ControlledProvider(std::uint64_t simulated_token_delay_ms)
    : simulated_token_delay_ms_(simulated_token_delay_ms) {
  descriptor_.provider_id = "t81.controlled.rng";
  descriptor_.provider_version = T81_VERSION_STR;
  descriptor_.runtime_environment = std::string("t81-controlled-rng-mt19937_64-") + T81_VERSION_STR;
  descriptor_.execution_class = ExecutionClass::SeededProbabilistic;
  descriptor_.isolation_mode = IsolationMode::InProcessReadOnly;
  descriptor_.capabilities.supported_operations = {"text.generate"};
  descriptor_.capabilities.supported_input_types = {"text/plain;charset=utf-8"};
  descriptor_.capabilities.supported_output_types = {"text/plain;charset=utf-8"};
  descriptor_.capabilities.supports_cancellation = true;
  descriptor_.capabilities.supports_streaming = false;
  descriptor_.capabilities.requires_network = false;
  descriptor_.capabilities.requires_user_consent = true;
  descriptor_.capabilities.max_input_bytes = 65536;
  descriptor_.capabilities.max_output_tokens = 256;
}

ProviderResult ControlledProvider::execute(const ProviderRequest& request,
                                           const ExecutionContext& context,
                                           const CancellationToken& cancel) {
  invocations_.fetch_add(1, std::memory_order_acq_rel);

  ProviderResult res;
  if (!context.seed.has_value()) {
    res.ok = false;
    res.trit = -1;
    res.failure_code = FailureCode::CapabilityMismatch;
    res.failure_detail = "missing seed for seeded_probabilistic provider";
    return res;
  }

  std::uint64_t requested_tokens = 8;
  auto it = request.parameters.find("tokens");
  if (it != request.parameters.end()) {
    const std::string& raw = it->second;
    bool valid_digits = !raw.empty();
    for (char ch : raw) {
      if (ch < '0' || ch > '9') {
        valid_digits = false;
        break;
      }
    }
    if (!valid_digits) {
      res.ok = false;
      res.trit = -1;
      res.failure_code = FailureCode::InvalidBudget;
      res.failure_detail = "tokens parameter must be a non-negative base-10 integer";
      return res;
    }
    try {
      std::size_t pos = 0;
      const auto parsed = std::stoull(raw, &pos, 10);
      if (pos != raw.size()) {
        res.ok = false;
        res.trit = -1;
        res.failure_code = FailureCode::InvalidBudget;
        res.failure_detail = "tokens parameter contains trailing characters";
        return res;
      }
      requested_tokens = parsed;
    } catch (...) {
      res.ok = false;
      res.trit = -1;
      res.failure_code = FailureCode::InvalidBudget;
      res.failure_detail = "invalid tokens parameter";
      return res;
    }
  }

  const std::uint64_t token_cap =
      context.budget.max_output_tokens > 0
          ? std::min(context.budget.max_output_tokens, descriptor_.capabilities.max_output_tokens)
          : descriptor_.capabilities.max_output_tokens;

  if (requested_tokens > token_cap) {
    res.ok = false;
    res.trit = -1;
    res.failure_code = FailureCode::BudgetExhausted;
    res.failure_detail = "requested tokens exceed max_output_tokens budget";
    return res;
  }

  static constexpr std::array<const char*, 8> kVocab = {"alpha", "beta",  "gamma", "delta",
                                                        "trit",  "canon", "axion", "epoch"};

  std::mt19937_64 rng(*context.seed);
  std::ostringstream text;
  const auto started = std::chrono::steady_clock::now();

  for (std::uint64_t i = 0; i < requested_tokens; ++i) {
    if (cancel.is_cancelled()) {
      res.ok = false;
      res.trit = -1;
      res.failure_code = FailureCode::Cancelled;
      res.failure_detail = "cancelled during token generation";
      res.tokens_consumed = i;
      return res;
    }

    if (simulated_token_delay_ms_ > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(simulated_token_delay_ms_));
    }

    if (context.budget.timeout_ms > 0) {
      const auto elapsed_ms =
          static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - started)
                                         .count());
      if (elapsed_ms > context.budget.timeout_ms) {
        res.ok = false;
        res.trit = -1;
        res.failure_code = FailureCode::TimedOut;
        res.failure_detail = "token generation exceeded timeout_ms";
        res.tokens_consumed = i;
        return res;
      }
    }

    if (i > 0) text << ' ';
    text << kVocab[static_cast<std::size_t>(rng() % kVocab.size())];
  }

  const std::string out_str = text.str();
  res.ok = true;
  res.trit = 1;
  res.failure_code = FailureCode::None;
  res.tokens_consumed = requested_tokens;
  res.output.media_type = "text/plain;charset=utf-8";
  res.output.bytes.resize(out_str.size());
  for (std::size_t i = 0; i < out_str.size(); ++i) {
    res.output.bytes[i] = static_cast<std::byte>(out_str[i]);
  }
  return res;
}

}  // namespace t81::provider
