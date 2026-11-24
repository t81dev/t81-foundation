#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "provider/provider_contract.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace t81::provider {

class ProviderGovernor {
public:
  ProviderGovernor(std::shared_ptr<t81::axion::PolicyEngine> policy_engine,
                   std::shared_ptr<t81::canonfs::Driver> canonfs_driver);

  // Execute a provider request under provider-independent Axion governance
  // and persist a canonical t81.provider.evidence.v1 artifact into CanonFS.
  [[nodiscard]] GovernedExecutionOutcome execute(IAiProvider& provider,
                                                 const ProviderRequest& request,
                                                 const ExecutionContext& context,
                                                 const CancellationToken& cancel);

  // Compute canonical SHA3-512 digest over Policy::serialize() binary TLV output.
  [[nodiscard]] static std::string compute_policy_sha3_512(const t81::axion::Policy& policy);

  // Serialize a ProviderDescriptor to canonical t81.provider.capabilities.v1 JSON.
  [[nodiscard]] static std::string serialize_capabilities_json(
      const ProviderDescriptor& descriptor);

  // Project a verified t81.provider.evidence.v1 JSON payload into a draft RFC-00D4
  // Accountable Decision Bundle wrapper (non-normative projection).
  [[nodiscard]] static std::string project_to_rfc00d4_json(const GovernedExecutionOutcome& outcome,
                                                           const ProviderDescriptor& descriptor,
                                                           const ProviderRequest& request,
                                                           const ExecutionContext& context,
                                                           std::string_view timestamp_utc,
                                                           std::string_view signature = "",
                                                           std::string_view public_key_uri = "");

  // Offline verification of a persisted CanonFS evidence object:
  // checks CanonFS read-back, SHA3-512 digest integrity, and execution-class honesty.
  [[nodiscard]] static EvidenceVerificationResult verify_persisted_evidence(
      t81::canonfs::Driver& canonfs_driver, const t81::canonfs::CanonRef& ref,
      std::string_view expected_sha3_512);

  // Offline verification of raw evidence JSON bytes against expected SHA3-512
  // and execution-class / replay-mode invariants.
  [[nodiscard]] static EvidenceVerificationResult verify_evidence_json(
      std::string_view evidence_json, std::string_view expected_sha3_512);

private:
  std::shared_ptr<t81::axion::PolicyEngine> policy_engine_;
  std::shared_ptr<t81::canonfs::Driver> canonfs_driver_;
  mutable std::mutex canonfs_mutex_;
};

}  // namespace t81::provider
