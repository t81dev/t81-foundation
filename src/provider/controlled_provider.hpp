#pragma once

#include <atomic>
#include <cstdint>

#include "provider/provider_contract.hpp"

namespace t81::provider {

// Seeded probabilistic provider honoring explicit user-consent, seed pinning,
// token budgets, cancellation, and timeout semantics without external dependencies.
class ControlledProvider final : public IAiProvider {
public:
  explicit ControlledProvider(std::uint64_t simulated_token_delay_ms = 0);

  [[nodiscard]] const ProviderDescriptor& descriptor() const noexcept override {
    return descriptor_;
  }

  ProviderResult execute(const ProviderRequest& request, const ExecutionContext& context,
                         const CancellationToken& cancel) override;

  [[nodiscard]] std::uint64_t invocation_count() const noexcept {
    return invocations_.load(std::memory_order_acquire);
  }

private:
  ProviderDescriptor descriptor_;
  std::uint64_t simulated_token_delay_ms_{0};
  std::atomic<std::uint64_t> invocations_{0};
};

}  // namespace t81::provider
