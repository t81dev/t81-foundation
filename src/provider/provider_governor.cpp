#include "provider/provider_governor.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "t81/axion/context.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/support/json_string.hpp"

namespace t81::provider {
namespace {

using t81::support::json_string;

std::string sha3_512_prefixed_bytes(std::span<const std::byte> bytes) {
  auto u8_span = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(bytes.data()),
                                               bytes.size());
  return "sha3-512:" + t81::crypto::sha3_512_hex(u8_span);
}

std::string sha3_512_prefixed_str(std::string_view text) {
  auto u8_span = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),
                                               text.size());
  return "sha3-512:" + t81::crypto::sha3_512_hex(u8_span);
}

bool contains_string(const std::vector<std::string>& vec, std::string_view target) {
  return std::find(vec.begin(), vec.end(), target) != vec.end();
}

std::string serialize_params_canonical(const std::map<std::string, std::string>& params) {
  std::string out = "{";
  bool first = true;
  for (const auto& [k, v] : params) {
    if (!first) out += ",";
    first = false;
    out += json_string(k) + ":" + json_string(v);
  }
  out += "}";
  return out;
}

std::string verification_mode_for(ExecutionClass cls) {
  switch (cls) {
    case ExecutionClass::Deterministic:
      return "bit_exact_replay";
    case ExecutionClass::SeededProbabilistic:
      return "seeded_bounded_attestation";
    case ExecutionClass::ExternalProbabilistic:
      return "provenance_only";
  }
  return "provenance_only";
}

bool replayable_for(ExecutionClass cls) { return cls == ExecutionClass::Deterministic; }

std::string build_evidence_json(const ProviderDescriptor& desc, const ProviderRequest& req,
                                const ExecutionContext& ctx,
                                const GovernedExecutionOutcome& outcome) {
  const std::string input_digest = sha3_512_prefixed_bytes(req.input.bytes);
  const std::string params_digest =
      sha3_512_prefixed_str(serialize_params_canonical(req.parameters));
  const std::string output_digest = sha3_512_prefixed_bytes(outcome.result.output.bytes);
  const std::string artifact_ref_json =
      req.artifact.canonfs_ref ? json_string(canon_ref_hex(*req.artifact.canonfs_ref)) : "null";
  const std::string parent_digest_json =
      ctx.parent_evidence_sha3_512 ? json_string(*ctx.parent_evidence_sha3_512) : "null";
  const std::string seed_json = ctx.seed ? std::to_string(*ctx.seed) : "null";

  const std::string recorded_artifact_sha = outcome.authoritative_artifact_sha3_512.empty()
                                                ? req.artifact.sha3_512
                                                : outcome.authoritative_artifact_sha3_512;
  const std::string recorded_policy_sha = outcome.authoritative_policy_sha3_512;

  std::ostringstream os;
  os << "{"
     << "\"artifact\":{"
     << "\"artifact_id\":" << json_string(req.artifact.artifact_id) << ","
     << "\"canonfs_ref\":" << artifact_ref_json << ","
     << "\"format\":" << json_string(req.artifact.format) << ","
     << "\"sha3_512\":" << json_string(recorded_artifact_sha) << "},"
     << "\"authority\":{"
     << "\"policy_sha3_512\":" << json_string(recorded_policy_sha) << ","
     << "\"principal_id\":" << json_string(ctx.principal_id) << ","
     << "\"provider_invoked\":" << (outcome.provider_invoked ? "true" : "false") << ","
     << "\"reason\":" << json_string(outcome.authorization_verdict.reason) << ","
     << "\"user_consent_verified\":" << (ctx.user_consent_verified ? "true" : "false") << ","
     << "\"verdict\":" << json_string(verdict_kind_name(outcome.authorization_verdict.kind)) << "},"
     << "\"challengeability\":{"
     << "\"replayable\":" << (replayable_for(desc.execution_class) ? "true" : "false") << ","
     << "\"verification_mode\":" << json_string(verification_mode_for(desc.execution_class)) << "},"
     << "\"digests\":{"
     << "\"input_sha3_512\":" << json_string(input_digest) << ","
     << "\"output_sha3_512\":" << json_string(output_digest) << ","
     << "\"params_sha3_512\":" << json_string(params_digest) << ","
     << "\"parent_evidence_sha3_512\":" << parent_digest_json << "},"
     << "\"execution\":{"
     << "\"dualrail_fallback_count\":" << outcome.result.dualrail_fallback_count << ","
     << "\"failure_code\":" << json_string(failure_code_name(outcome.result.failure_code)) << ","
     << "\"ok\":" << (outcome.result.ok ? "true" : "false") << ","
     << "\"operation\":" << json_string(req.operation) << ","
     << "\"seed\":" << seed_json << ","
     << "\"steps_consumed\":" << outcome.result.steps_consumed << ","
     << "\"tokens_consumed\":" << outcome.result.tokens_consumed << "},"
     << "\"provider\":{"
     << "\"execution_class\":" << json_string(execution_class_name(desc.execution_class)) << ","
     << "\"isolation_mode\":" << json_string(isolation_mode_name(desc.isolation_mode)) << ","
     << "\"provider_id\":" << json_string(desc.provider_id) << ","
     << "\"provider_version\":" << json_string(desc.provider_version) << ","
     << "\"runtime_environment\":" << json_string(desc.runtime_environment) << "},"
     << "\"schema\":\"t81.provider.evidence.v1\","
     << "\"trit\":" << static_cast<int>(outcome.result.trit) << "}";
  return os.str();
}

}  // namespace

std::string execution_class_name(ExecutionClass cls) {
  switch (cls) {
    case ExecutionClass::Deterministic:
      return "deterministic";
    case ExecutionClass::SeededProbabilistic:
      return "seeded_probabilistic";
    case ExecutionClass::ExternalProbabilistic:
      return "external_probabilistic";
  }
  return "external_probabilistic";
}

std::string isolation_mode_name(IsolationMode mode) {
  switch (mode) {
    case IsolationMode::InProcessReadOnly:
      return "in_process_readonly";
    case IsolationMode::JailedProcess:
      return "jailed_process";
    case IsolationMode::ExternalEndpoint:
      return "external_endpoint";
  }
  return "external_endpoint";
}

std::string failure_code_name(FailureCode code) {
  switch (code) {
    case FailureCode::None:
      return "NONE";
    case FailureCode::PolicyDenied:
      return "POLICY_DENIED";
    case FailureCode::ConsentRequired:
      return "CONSENT_REQUIRED";
    case FailureCode::CapabilityMismatch:
      return "CAPABILITY_MISMATCH";
    case FailureCode::ArtifactDigestError:
      return "ARTIFACT_DIGEST_ERROR";
    case FailureCode::InvalidBudget:
      return "INVALID_BUDGET";
    case FailureCode::BudgetExhausted:
      return "BUDGET_EXHAUSTED";
    case FailureCode::TimedOut:
      return "TIMED_OUT";
    case FailureCode::Cancelled:
      return "CANCELLED";
    case FailureCode::DeterminismViolation:
      return "DETERMINISM_VIOLATION";
    case FailureCode::ProviderFault:
      return "PROVIDER_FAULT";
    case FailureCode::EvidencePersistError:
      return "EVIDENCE_PERSIST_ERROR";
  }
  return "PROVIDER_FAULT";
}

std::string verdict_kind_name(t81::axion::VerdictKind kind) {
  switch (kind) {
    case t81::axion::VerdictKind::Allow:
      return "Allow";
    case t81::axion::VerdictKind::Deny:
      return "Deny";
    case t81::axion::VerdictKind::Defer:
      return "Defer";
    case t81::axion::VerdictKind::Warn:
      return "Warn";
    case t81::axion::VerdictKind::Quarantine:
      return "Quarantine";
  }
  return "Deny";
}

std::string canon_ref_hex(const t81::canonfs::CanonRef& ref) {
  std::ostringstream os;
  os << "canonref:" << std::hex << std::setfill('0');
  for (std::uint8_t b : ref.hash.h.bytes) {
    os << std::setw(2) << static_cast<unsigned int>(b);
  }
  return os.str();
}

std::string ProviderGovernor::compute_policy_sha3_512(const t81::axion::Policy& policy) {
  std::ostringstream ss(std::ios::binary);
  policy.serialize(ss);
  return sha3_512_prefixed_str(ss.str());
}

ProviderGovernor::ProviderGovernor(std::shared_ptr<t81::axion::PolicyEngine> policy_engine,
                                   std::shared_ptr<t81::canonfs::Driver> canonfs_driver)
    : policy_engine_(std::move(policy_engine)), canonfs_driver_(std::move(canonfs_driver)) {}

GovernedExecutionOutcome ProviderGovernor::execute(IAiProvider& provider,
                                                   const ProviderRequest& request,
                                                   const ExecutionContext& context,
                                                   const CancellationToken& cancel) {
  const ProviderDescriptor& desc = provider.descriptor();
  GovernedExecutionOutcome outcome;
  outcome.authorization_verdict = {t81::axion::VerdictKind::Deny, "not_evaluated"};
  outcome.provider_invoked = false;

  auto finalize_and_persist = [&]() -> GovernedExecutionOutcome {
    outcome.evidence_json = build_evidence_json(desc, request, context, outcome);
    outcome.evidence_sha3_512 = sha3_512_prefixed_str(outcome.evidence_json);

    if (!canonfs_driver_) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::EvidencePersistError;
      outcome.result.failure_detail = "CanonFS driver is null";
      outcome.result.output.bytes.clear();
      return outcome;
    }

    std::vector<std::byte> payload(outcome.evidence_json.size());
    for (std::size_t i = 0; i < outcome.evidence_json.size(); ++i) {
      payload[i] = static_cast<std::byte>(outcome.evidence_json[i]);
    }

    std::lock_guard<std::mutex> lock(canonfs_mutex_);
    auto write_res = canonfs_driver_->write_object(t81::canonfs::ObjectType::RawBlock, payload);
    if (!write_res) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::EvidencePersistError;
      outcome.result.failure_detail = "CanonFS write_object failed";
      outcome.result.output.bytes.clear();
      return outcome;
    }

    auto read_res = canonfs_driver_->read_object_bytes(*write_res);
    if (!read_res || sha3_512_prefixed_bytes(*read_res) != outcome.evidence_sha3_512) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::EvidencePersistError;
      outcome.result.failure_detail = "CanonFS read-back SHA3-512 verification failed";
      outcome.result.output.bytes.clear();
      return outcome;
    }

    outcome.evidence_ref = *write_res;
    return outcome;
  };

  // 1. Preflight validation (fail-closed before Axion or provider)
  if (context.budget.capability_units < 0) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::InvalidBudget;
    outcome.result.failure_detail = "capability_units must be non-negative";
    return finalize_and_persist();
  }

  if (!contains_string(desc.capabilities.supported_operations, request.operation) ||
      !contains_string(desc.capabilities.supported_input_types, request.input.media_type)) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::CapabilityMismatch;
    outcome.result.failure_detail = "provider does not support requested operation or input type";
    return finalize_and_persist();
  }

  if (desc.capabilities.max_input_bytes > 0 &&
      request.input.bytes.size() > desc.capabilities.max_input_bytes) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::BudgetExhausted;
    outcome.result.failure_detail = "input exceeds provider max_input_bytes";
    return finalize_and_persist();
  }

  if (desc.execution_class == ExecutionClass::SeededProbabilistic && !context.seed.has_value()) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::CapabilityMismatch;
    outcome.result.failure_detail = "seeded_probabilistic execution requires an explicit seed";
    return finalize_and_persist();
  }

  // 2. Artifact-source precedence & independent digest verification (F-01)
  // Precedence:
  //   (a) canonfs_ref present -> read bytes from CanonFS; if artifact_is_inline_input,
  //       require exact byte + digest identity between CanonFS object and inline input.
  //   (b) artifact_is_inline_input (no canonfs_ref) -> compute SHA3-512 over input.bytes.
  //   (c) non-inline, non-CanonFS artifact -> require non-empty sha3_512 unless
  //   ExternalProbabilistic.
  // Any mismatch against caller-supplied request.artifact.sha3_512 fails closed BEFORE
  // Axion/provider.
  if (request.artifact.canonfs_ref.has_value()) {
    if (!canonfs_driver_) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::ArtifactDigestError;
      outcome.result.failure_detail = "cannot verify artifact canonfs_ref without CanonFS driver";
      return finalize_and_persist();
    }
    std::vector<std::byte> canon_bytes;
    bool read_ok = false;
    {
      std::lock_guard<std::mutex> lock(canonfs_mutex_);
      auto read_art = canonfs_driver_->read_object_bytes(*request.artifact.canonfs_ref);
      if (read_art) {
        canon_bytes = std::move(*read_art);
        read_ok = true;
      }
    }
    if (!read_ok) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::ArtifactDigestError;
      outcome.result.failure_detail = "artifact canonfs_ref missing or unreadable";
      return finalize_and_persist();
    }
    const std::string canon_digest = sha3_512_prefixed_bytes(canon_bytes);
    if (desc.capabilities.artifact_is_inline_input) {
      const std::string inline_digest = sha3_512_prefixed_bytes(request.input.bytes);
      if (canon_bytes != request.input.bytes || canon_digest != inline_digest) {
        outcome.result.ok = false;
        outcome.result.trit = -1;
        outcome.result.failure_code = FailureCode::ArtifactDigestError;
        outcome.result.failure_detail =
            "artifact canonfs_ref bytes do not match inline executable input bytes";
        return finalize_and_persist();
      }
    }
    if (!request.artifact.sha3_512.empty() && request.artifact.sha3_512 != canon_digest) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::ArtifactDigestError;
      outcome.result.failure_detail =
          "caller artifact.sha3_512 does not match CanonFS object bytes";
      return finalize_and_persist();
    }
    outcome.authoritative_artifact_sha3_512 = canon_digest;
  } else if (desc.capabilities.artifact_is_inline_input) {
    const std::string inline_digest = sha3_512_prefixed_bytes(request.input.bytes);
    if (!request.artifact.sha3_512.empty() && request.artifact.sha3_512 != inline_digest) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::ArtifactDigestError;
      outcome.result.failure_detail =
          "caller artifact.sha3_512 does not match inline executable input bytes";
      return finalize_and_persist();
    }
    outcome.authoritative_artifact_sha3_512 = inline_digest;
  } else {
    if (desc.execution_class != ExecutionClass::ExternalProbabilistic &&
        request.artifact.sha3_512.empty()) {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::ArtifactDigestError;
      outcome.result.failure_detail = "artifact sha3_512 digest is required";
      return finalize_and_persist();
    }
    outcome.authoritative_artifact_sha3_512 = request.artifact.sha3_512;
  }

  if (desc.capabilities.requires_user_consent && !context.user_consent_verified) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::ConsentRequired;
    outcome.result.failure_detail = "provider requires verified user consent before dispatch";
    return finalize_and_persist();
  }

  // 3. Pre-execution cancellation check
  if (cancel.is_cancelled()) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::Cancelled;
    outcome.result.failure_detail = "cancelled prior to authorization";
    return finalize_and_persist();
  }

  // 4. Provider-independent Axion authorization gate & policy provenance binding (F-02)
  if (!policy_engine_ || !policy_engine_->active_policy()) {
    outcome.authorization_verdict = {t81::axion::VerdictKind::Deny, "missing_active_policy"};
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::PolicyDenied;
    outcome.result.failure_detail = "fail-closed: no active Axion policy";
    return finalize_and_persist();
  }

  outcome.authoritative_policy_sha3_512 = compute_policy_sha3_512(*policy_engine_->active_policy());
  if (!context.policy_sha3_512.empty() &&
      context.policy_sha3_512 != outcome.authoritative_policy_sha3_512) {
    outcome.authorization_verdict = {t81::axion::VerdictKind::Deny, "policy_sha3_512_mismatch"};
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::PolicyDenied;
    outcome.result.failure_detail = "caller policy_sha3_512 does not match active Axion policy";
    return finalize_and_persist();
  }

  t81::axion::SyscallContext ctx;
  ctx.caller = context.principal_id;
  ctx.syscall = "provider:" + request.operation;
  ctx.payload = outcome.authoritative_artifact_sha3_512;
  ctx.policy = policy_engine_->active_policy();
  ctx.next_opcode = t81::tisc::Opcode::TLoadHash;
  ctx.final_authorization = true;

  outcome.authorization_verdict = policy_engine_->evaluate(ctx);
  if (outcome.authorization_verdict.kind != t81::axion::VerdictKind::Allow) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::PolicyDenied;
    outcome.result.failure_detail = outcome.authorization_verdict.reason;
    return finalize_and_persist();
  }

  // 4. Governed provider execution (only after Axion Allow)
  const auto started = std::chrono::steady_clock::now();
  outcome.provider_invoked = true;
  outcome.result = provider.execute(request, context, cancel);
  const auto elapsed_ms =
      static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - started)
                                     .count());

  if (cancel.is_cancelled() && outcome.result.failure_code == FailureCode::None) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::Cancelled;
    outcome.result.failure_detail = "cancelled during execution";
    outcome.result.output.bytes.clear();
  } else if (context.budget.timeout_ms > 0 && elapsed_ms > context.budget.timeout_ms &&
             outcome.result.failure_code == FailureCode::None) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::TimedOut;
    outcome.result.failure_detail = "execution exceeded timeout_ms budget";
    outcome.result.output.bytes.clear();
  } else if (outcome.result.ok && desc.execution_class == ExecutionClass::Deterministic &&
             outcome.result.dualrail_fallback_count > 0) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::DeterminismViolation;
    outcome.result.failure_detail = "deterministic provider reported dualrail_fallback_count > 0";
    outcome.result.output.bytes.clear();
  }

  if (!outcome.result.ok) {
    outcome.result.trit = -1;
    outcome.result.output.bytes.clear();
  } else if (desc.execution_class == ExecutionClass::ExternalProbabilistic &&
             outcome.result.trit > 0) {
    // External probabilistic outputs default to 0 (provenance-only / unverified replay)
    outcome.result.trit = 0;
  }

  return finalize_and_persist();
}

std::string ProviderGovernor::serialize_capabilities_json(const ProviderDescriptor& descriptor) {
  auto write_str_array = [](std::ostringstream& os, const std::vector<std::string>& arr) {
    os << "[";
    for (std::size_t i = 0; i < arr.size(); ++i) {
      if (i > 0) os << ",";
      os << json_string(arr[i]);
    }
    os << "]";
  };

  std::ostringstream os;
  os << "{"
     << "\"capabilities\":{"
     << "\"max_input_bytes\":" << descriptor.capabilities.max_input_bytes << ","
     << "\"max_output_tokens\":" << descriptor.capabilities.max_output_tokens << ","
     << "\"requires_network\":" << (descriptor.capabilities.requires_network ? "true" : "false")
     << ","
     << "\"requires_user_consent\":"
     << (descriptor.capabilities.requires_user_consent ? "true" : "false") << ","
     << "\"supported_input_types\":";
  write_str_array(os, descriptor.capabilities.supported_input_types);
  os << ",\"supported_operations\":";
  write_str_array(os, descriptor.capabilities.supported_operations);
  os << ",\"supported_output_types\":";
  write_str_array(os, descriptor.capabilities.supported_output_types);
  os << ",\"supports_cancellation\":"
     << (descriptor.capabilities.supports_cancellation ? "true" : "false") << ","
     << "\"supports_streaming\":" << (descriptor.capabilities.supports_streaming ? "true" : "false")
     << "},"
     << "\"execution_class\":" << json_string(execution_class_name(descriptor.execution_class))
     << ","
     << "\"isolation_mode\":" << json_string(isolation_mode_name(descriptor.isolation_mode)) << ","
     << "\"provider_id\":" << json_string(descriptor.provider_id) << ","
     << "\"provider_version\":" << json_string(descriptor.provider_version) << ","
     << "\"runtime_environment\":" << json_string(descriptor.runtime_environment) << ","
     << "\"schema\":\"t81.provider.capabilities.v1\""
     << "}";
  return os.str();
}

std::string ProviderGovernor::project_to_rfc00d4_json(
    const GovernedExecutionOutcome& outcome, const ProviderDescriptor& descriptor,
    const ProviderRequest& request, const ExecutionContext& context, std::string_view timestamp_utc,
    std::string_view signature, std::string_view public_key_uri) {
  const std::string adb_verdict =
      (outcome.authorization_verdict.kind == t81::axion::VerdictKind::Allow)
          ? "ALLOW"
          : (outcome.authorization_verdict.kind == t81::axion::VerdictKind::Warn ? "WARN" : "DENY");
  const std::string input_digest = sha3_512_prefixed_bytes(request.input.bytes);
  const std::string output_digest = sha3_512_prefixed_bytes(outcome.result.output.bytes);

  std::ostringstream os;
  os << "{"
     << "\"schema_version\":\"1.0.0-draft\","
     << "\"bundle_id\":" << json_string(outcome.evidence_sha3_512) << ","
     << "\"timestamp_utc\":" << json_string(std::string(timestamp_utc)) << ","
     << "\"meaning\":{"
     << "\"action_type\":" << json_string("provider:" + request.operation) << ","
     << "\"parameters\":" << serialize_params_canonical(request.parameters) << ","
     << "\"proposed_side_effects\":[]"
     << "},"
     << "\"evidence\":{"
     << "\"input_digest\":" << json_string(input_digest) << ","
     << "\"output_digest\":" << json_string(output_digest) << ","
     << "\"execution_trace_hash\":" << json_string(outcome.evidence_sha3_512);
  if (context.parent_evidence_sha3_512) {
    os << ",\"parent_bundle_ids\":[" << json_string(*context.parent_evidence_sha3_512) << "]";
  }
  os << "},"
     << "\"method\":{"
     << "\"runtime_environment\":" << json_string(descriptor.runtime_environment) << ","
     << "\"transformation_id\":"
     << json_string(descriptor.provider_id + "@" + descriptor.provider_version) << ","
     << "\"model_weights_digest\":"
     << json_string(outcome.authoritative_artifact_sha3_512.empty()
                        ? request.artifact.sha3_512
                        : outcome.authoritative_artifact_sha3_512)
     << "},"
     << "\"authority\":{"
     << "\"policy_id\":\"axion.policy.v1\","
     << "\"policy_hash\":" << json_string(outcome.authoritative_policy_sha3_512) << ","
     << "\"verdict\":" << json_string(adb_verdict) << ","
     << "\"enforcement_mode\":\"fail-closed\","
     << "\"evaluation_timestamp\":" << json_string(std::string(timestamp_utc)) << "},"
     << "\"accountability\":{"
     << "\"principal_id\":" << json_string(context.principal_id) << ","
     << "\"public_key_uri\":" << json_string(std::string(public_key_uri)) << ","
     << "\"signature\":" << json_string(std::string(signature)) << "},"
     << "\"challengeability\":{"
     << "\"replayable\":" << (replayable_for(descriptor.execution_class) ? "true" : "false") << ","
     << "\"verification_procedure\":"
     << json_string(verification_mode_for(descriptor.execution_class)) << "}"
     << "}";
  return os.str();
}

EvidenceVerificationResult ProviderGovernor::verify_evidence_json(
    std::string_view evidence_json, std::string_view expected_sha3_512) {
  EvidenceVerificationResult res;
  res.computed_sha3_512 = sha3_512_prefixed_str(evidence_json);

  if (evidence_json.find("\"schema\":\"t81.provider.evidence.v1\"") == std::string_view::npos) {
    res.valid = false;
    res.reason = "missing or mismatched t81.provider.evidence.v1 schema marker";
    return res;
  }

  if (!expected_sha3_512.empty() && res.computed_sha3_512 != expected_sha3_512) {
    res.valid = false;
    res.reason = "SHA3-512 digest mismatch (tampered evidence)";
    return res;
  }

  // Execution-class honesty invariant: non-deterministic providers MUST NOT claim bit_exact_replay
  const bool is_deterministic =
      evidence_json.find("\"execution_class\":\"deterministic\"") != std::string_view::npos;
  const bool claims_bit_exact =
      evidence_json.find("\"verification_mode\":\"bit_exact_replay\"") != std::string_view::npos ||
      evidence_json.find("\"replayable\":true") != std::string_view::npos;

  if (!is_deterministic && claims_bit_exact) {
    res.valid = false;
    res.reason = "honesty violation: probabilistic provider claims bit_exact_replay";
    return res;
  }

  res.valid = true;
  res.reason = "VERIFIED_EVIDENCE_OK";
  return res;
}

EvidenceVerificationResult ProviderGovernor::verify_persisted_evidence(
    t81::canonfs::Driver& canonfs_driver, const t81::canonfs::CanonRef& ref,
    std::string_view expected_sha3_512) {
  auto bytes_res = canonfs_driver.read_object_bytes(ref);
  if (!bytes_res) {
    return {false, "CanonFS read_object_bytes failed", ""};
  }
  std::string text(reinterpret_cast<const char*>(bytes_res->data()), bytes_res->size());
  return verify_evidence_json(text, expected_sha3_512);
}

}  // namespace t81::provider
