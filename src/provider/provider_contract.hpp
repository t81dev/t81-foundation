#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "t81/axion/verdict.hpp"
#include "t81/canonfs/canon_types.hpp"

namespace t81::provider {

enum class ExecutionClass : std::uint8_t {
  Deterministic = 0,
  SeededProbabilistic = 1,
  ExternalProbabilistic = 2,
};

enum class ExecutionPlane : std::uint8_t {
  Deterministic = 0,
  Stochastic = 1,
  Hybrid = 2,
};

enum class TransitionKind : std::uint8_t {
  ExecutionPath = 0,
  TransitionPath = 1,
};

struct PlaneTransitionEnvelope {
  ExecutionPlane source_plane{ExecutionPlane::Deterministic};
  ExecutionPlane target_plane{ExecutionPlane::Deterministic};
  TransitionKind transition_kind{TransitionKind::ExecutionPath};
  std::optional<std::string> state_a_ref;
  std::optional<std::string> state_c_ref;
  bool hybrid_reconciled{false};
};

enum class IsolationMode : std::uint8_t {
  InProcessReadOnly = 0,
  JailedProcess = 1,
  ExternalEndpoint = 2,
};

struct ArtifactIdentity {
  std::string artifact_id;
  std::string format;
  std::string sha3_512;
  std::optional<t81::canonfs::CanonRef> canonfs_ref;
};

struct ProviderCapabilities {
  std::vector<std::string> supported_operations;
  std::vector<std::string> supported_input_types;
  std::vector<std::string> supported_output_types;
  bool supports_cancellation{false};
  bool supports_streaming{false};
  bool requires_network{false};
  bool requires_user_consent{false};
  bool artifact_is_inline_input{false};
  std::uint64_t max_input_bytes{0};
  std::uint64_t max_output_tokens{0};
};

struct ProviderDescriptor {
  std::string provider_id;
  std::string provider_version;
  std::string runtime_environment;
  ExecutionClass execution_class{ExecutionClass::ExternalProbabilistic};
  IsolationMode isolation_mode{IsolationMode::InProcessReadOnly};
  ProviderCapabilities capabilities;
};

struct TypedPayload {
  std::string media_type;
  std::vector<std::byte> bytes;
  std::optional<t81::canonfs::CanonRef> canonfs_ref;
};

struct ResourceBudget {
  std::uint64_t max_steps{0};
  std::uint64_t max_output_tokens{0};
  std::uint64_t max_memory_bytes{0};
  std::uint64_t timeout_ms{30000};
  std::int64_t capability_units{1};
};

struct ExecutionContext {
  std::string correlation_id;
  std::string principal_id;
  std::string policy_sha3_512;
  std::optional<std::string> parent_evidence_sha3_512;
  std::optional<std::uint64_t> seed;
  bool user_consent_verified{false};
  ResourceBudget budget;
  std::optional<PlaneTransitionEnvelope> plane_transition;
};

struct ProviderRequest {
  std::string operation;
  ArtifactIdentity artifact;
  TypedPayload input;
  std::map<std::string, std::string> parameters;
};

class CancellationToken {
public:
  void request_cancel() noexcept { cancelled_.store(true, std::memory_order_release); }
  [[nodiscard]] bool is_cancelled() const noexcept {
    return cancelled_.load(std::memory_order_acquire);
  }

private:
  std::atomic<bool> cancelled_{false};
};

enum class FailureCode : std::uint16_t {
  None = 0,
  PolicyDenied = 100,
  ConsentRequired = 101,
  CapabilityMismatch = 102,
  ArtifactDigestError = 103,
  InvalidBudget = 104,
  PlaneBypassDenied = 105,
  BudgetExhausted = 200,
  TimedOut = 201,
  Cancelled = 202,
  DeterminismViolation = 203,
  ProviderFault = 300,
  EvidencePersistError = 400,
};

struct ProviderResult {
  bool ok{false};
  std::int8_t trit{-1};
  FailureCode failure_code{FailureCode::None};
  std::string failure_detail;
  TypedPayload output;
  std::uint64_t steps_consumed{0};
  std::uint64_t tokens_consumed{0};
  std::uint64_t dualrail_fallback_count{0};
};

class IAiProvider {
public:
  virtual ~IAiProvider() = default;
  [[nodiscard]] virtual const ProviderDescriptor& descriptor() const noexcept = 0;

  // MUST ONLY be invoked by ProviderGovernor after Axion PolicyEngine returns Allow.
  virtual ProviderResult execute(const ProviderRequest& request, const ExecutionContext& context,
                                 const CancellationToken& cancel) = 0;
};

struct GovernedExecutionOutcome {
  ProviderResult result;
  t81::axion::Verdict authorization_verdict;
  bool provider_invoked{false};
  std::string authoritative_policy_sha3_512;
  std::string authoritative_artifact_sha3_512;
  std::string evidence_json;
  std::string evidence_sha3_512;
  std::optional<t81::canonfs::CanonRef> evidence_ref;
};

struct EvidenceVerificationResult {
  bool valid{false};
  std::string reason;
  std::string computed_sha3_512;
};

[[nodiscard]] std::string execution_class_name(ExecutionClass cls);
[[nodiscard]] std::string execution_plane_name(ExecutionPlane plane);
[[nodiscard]] std::string transition_kind_name(TransitionKind kind);
[[nodiscard]] std::string isolation_mode_name(IsolationMode mode);
[[nodiscard]] std::string failure_code_name(FailureCode code);
[[nodiscard]] std::string verdict_kind_name(t81::axion::VerdictKind kind);
[[nodiscard]] std::string canon_ref_hex(const t81::canonfs::CanonRef& ref);

}  // namespace t81::provider
