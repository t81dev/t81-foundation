#pragma once

#include <atomic>
#include <cstdint>

#include "provider/provider_contract.hpp"
#include "t81/axion/policy.hpp"

namespace t81::provider {

// Deterministic TISC v1.9.0 provider executing pinned bytecode via T81VM + AIHookEngine.
class T81VmProvider final : public IAiProvider {
public:
  explicit T81VmProvider(t81::axion::Policy vm_policy = {});

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
  t81::axion::Policy vm_policy_;
  std::atomic<std::uint64_t> invocations_{0};
};

}  // namespace t81::provider
