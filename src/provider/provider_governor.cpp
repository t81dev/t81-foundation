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
    auto fail_persist = [&](std::string detail) -> GovernedExecutionOutcome {
      outcome.result.ok = false;
      outcome.result.trit = -1;
      outcome.result.failure_code = FailureCode::EvidencePersistError;
      outcome.result.failure_detail = std::move(detail);
      outcome.result.output.bytes.clear();
      // D2: Never expose an unpersisted success-shaped receipt on persistence failure.
      outcome.evidence_ref.reset();
      outcome.evidence_json.clear();
      outcome.evidence_sha3_512.clear();
      return outcome;
    };

    const std::string candidate_json = build_evidence_json(desc, request, context, outcome);
    const std::string candidate_sha = sha3_512_prefixed_str(candidate_json);

    if (!canonfs_driver_) {
      return fail_persist("CanonFS driver is null");
    }

    std::vector<std::byte> payload(candidate_json.size());
    for (std::size_t i = 0; i < candidate_json.size(); ++i) {
      payload[i] = static_cast<std::byte>(candidate_json[i]);
    }

    std::lock_guard<std::mutex> lock(canonfs_mutex_);
    auto write_res = canonfs_driver_->write_object(t81::canonfs::ObjectType::RawBlock, payload);
    if (!write_res) {
      return fail_persist("CanonFS write_object failed");
    }

    auto read_res = canonfs_driver_->read_object_bytes(*write_res);
    if (!read_res || sha3_512_prefixed_bytes(*read_res) != candidate_sha) {
      return fail_persist("CanonFS read-back SHA3-512 verification failed");
    }

    outcome.evidence_json = candidate_json;
    outcome.evidence_sha3_512 = candidate_sha;
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

  // F-03: In M1, max_memory_bytes bounds input/output payload sizes only (NOT process RSS).
  if (context.budget.max_memory_bytes > 0 &&
      request.input.bytes.size() > context.budget.max_memory_bytes) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::BudgetExhausted;
    outcome.result.failure_detail = "input payload exceeds budget max_memory_bytes";
    return finalize_and_persist();
  }

  // F-03: In-process timeouts are cooperative; reject timeout_ms > 0 on non-cancellable providers.
  if (context.budget.timeout_ms > 0 && !desc.capabilities.supports_cancellation) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::CapabilityMismatch;
    outcome.result.failure_detail =
        "provider does not support cooperative cancellation required for timeout_ms";
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

  // 5. Governed provider execution (only after Axion Allow; exceptions caught fail-closed)
  const auto started = std::chrono::steady_clock::now();
  outcome.provider_invoked = true;
  try {
    outcome.result = provider.execute(request, context, cancel);
  } catch (const std::exception& e) {
    outcome.result = ProviderResult{};
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::ProviderFault;
    outcome.result.failure_detail = std::string("provider threw exception: ") + e.what();
  } catch (...) {
    outcome.result = ProviderResult{};
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::ProviderFault;
    outcome.result.failure_detail = "provider threw unknown exception";
  }
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
    outcome.result.failure_detail = "execution exceeded cooperative timeout_ms budget";
    outcome.result.output.bytes.clear();
  } else if (outcome.result.ok && context.budget.max_memory_bytes > 0 &&
             outcome.result.output.bytes.size() > context.budget.max_memory_bytes) {
    outcome.result.ok = false;
    outcome.result.trit = -1;
    outcome.result.failure_code = FailureCode::BudgetExhausted;
    outcome.result.failure_detail = "output payload exceeds budget max_memory_bytes";
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
     << "\"artifact_is_inline_input\":"
     << (descriptor.capabilities.artifact_is_inline_input ? "true" : "false") << ","
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

namespace {

struct JsonVal {
  enum class Kind { Null, Bool, Int, String, Array, Object };
  Kind kind{Kind::Null};
  bool b{false};
  std::int64_t i{0};
  std::string s;
  std::vector<JsonVal> arr;
  std::map<std::string, JsonVal> obj;
};

class StrictJsonParser {
public:
  explicit StrictJsonParser(std::string_view src) : src_(src) {}

  bool parse_root(JsonVal& out) {
    skip_ws();
    if (!parse_value(out, 0)) return false;
    skip_ws();
    return pos_ == src_.size();
  }

private:
  void skip_ws() {
    while (pos_ < src_.size() &&
           (src_[pos_] == ' ' || src_[pos_] == '\t' || src_[pos_] == '\n' || src_[pos_] == '\r')) {
      ++pos_;
    }
  }

  bool parse_string(std::string& out) {
    if (pos_ >= src_.size() || src_[pos_] != '"') return false;
    ++pos_;
    out.clear();
    while (pos_ < src_.size()) {
      char c = src_[pos_++];
      if (c == '"') return true;
      if (static_cast<unsigned char>(c) < 0x20) return false;
      if (c == '\\') {
        if (pos_ >= src_.size()) return false;
        char esc = src_[pos_++];
        switch (esc) {
          case '"':
          case '\\':
          case '/':
            out.push_back(esc);
            break;
          case 'b':
            out.push_back('\b');
            break;
          case 'f':
            out.push_back('\f');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          case 'u': {
            if (pos_ + 4 > src_.size()) return false;
            unsigned int code = 0;
            for (int k = 0; k < 4; ++k) {
              char h = src_[pos_++];
              code <<= 4;
              if (h >= '0' && h <= '9')
                code |= static_cast<unsigned int>(h - '0');
              else if (h >= 'a' && h <= 'f')
                code |= static_cast<unsigned int>(h - 'a' + 10);
              else if (h >= 'A' && h <= 'F')
                code |= static_cast<unsigned int>(h - 'A' + 10);
              else
                return false;
            }
            out.push_back(static_cast<char>(code & 0xFF));
            break;
          }
          default:
            return false;
        }
      } else {
        out.push_back(c);
      }
    }
    return false;
  }

  bool parse_int(std::int64_t& out) {
    const std::size_t start = pos_;
    if (pos_ < src_.size() && src_[pos_] == '-') ++pos_;
    if (pos_ >= src_.size() || src_[pos_] < '0' || src_[pos_] > '9') return false;
    while (pos_ < src_.size() && src_[pos_] >= '0' && src_[pos_] <= '9') ++pos_;
    try {
      std::size_t consumed = 0;
      const std::string token(src_.substr(start, pos_ - start));
      out = std::stoll(token, &consumed, 10);
      return consumed == token.size();
    } catch (...) {
      return false;
    }
  }

  bool parse_value(JsonVal& out, int depth) {
    if (depth > 16) return false;
    skip_ws();
    if (pos_ >= src_.size()) return false;
    const char c = src_[pos_];
    if (c == '"') {
      out.kind = JsonVal::Kind::String;
      return parse_string(out.s);
    }
    if (c == '{') {
      out.kind = JsonVal::Kind::Object;
      out.obj.clear();
      ++pos_;
      skip_ws();
      if (pos_ < src_.size() && src_[pos_] == '}') {
        ++pos_;
        return true;
      }
      while (true) {
        skip_ws();
        std::string key;
        if (!parse_string(key)) return false;
        skip_ws();
        if (pos_ >= src_.size() || src_[pos_] != ':') return false;
        ++pos_;
        JsonVal val;
        if (!parse_value(val, depth + 1)) return false;
        if (out.obj.find(key) != out.obj.end()) return false;  // reject duplicate keys
        out.obj.emplace(std::move(key), std::move(val));
        skip_ws();
        if (pos_ >= src_.size()) return false;
        if (src_[pos_] == '}') {
          ++pos_;
          return true;
        }
        if (src_[pos_] != ',') return false;
        ++pos_;
      }
    }
    if (c == '[') {
      out.kind = JsonVal::Kind::Array;
      out.arr.clear();
      ++pos_;
      skip_ws();
      if (pos_ < src_.size() && src_[pos_] == ']') {
        ++pos_;
        return true;
      }
      while (true) {
        JsonVal elem;
        if (!parse_value(elem, depth + 1)) return false;
        out.arr.push_back(std::move(elem));
        skip_ws();
        if (pos_ >= src_.size()) return false;
        if (src_[pos_] == ']') {
          ++pos_;
          return true;
        }
        if (src_[pos_] != ',') return false;
        ++pos_;
      }
    }
    if (src_.substr(pos_, 4) == "true") {
      out.kind = JsonVal::Kind::Bool;
      out.b = true;
      pos_ += 4;
      return true;
    }
    if (src_.substr(pos_, 5) == "false") {
      out.kind = JsonVal::Kind::Bool;
      out.b = false;
      pos_ += 5;
      return true;
    }
    if (src_.substr(pos_, 4) == "null") {
      out.kind = JsonVal::Kind::Null;
      pos_ += 4;
      return true;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
      out.kind = JsonVal::Kind::Int;
      return parse_int(out.i);
    }
    return false;
  }

  std::string_view src_;
  std::size_t pos_{0};
};

const JsonVal* get_field(const JsonVal& obj, const std::string& key, JsonVal::Kind kind) {
  if (obj.kind != JsonVal::Kind::Object) return nullptr;
  auto it = obj.obj.find(key);
  if (it == obj.obj.end() || it->second.kind != kind) return nullptr;
  return &it->second;
}

}  // namespace

EvidenceVerificationResult ProviderGovernor::verify_evidence_json(
    std::string_view evidence_json, std::string_view expected_sha3_512) {
  EvidenceVerificationResult res;
  res.computed_sha3_512 = sha3_512_prefixed_str(evidence_json);

  if (!expected_sha3_512.empty() && res.computed_sha3_512 != expected_sha3_512) {
    res.valid = false;
    res.reason = "SHA3-512 digest mismatch (tampered evidence)";
    return res;
  }

  JsonVal root;
  StrictJsonParser parser(evidence_json);
  if (!parser.parse_root(root) || root.kind != JsonVal::Kind::Object) {
    res.valid = false;
    res.reason = "malformed JSON evidence object";
    return res;
  }

  const auto* schema = get_field(root, "schema", JsonVal::Kind::String);
  if (!schema || schema->s != "t81.provider.evidence.v1") {
    res.valid = false;
    res.reason = "missing or mismatched t81.provider.evidence.v1 schema marker";
    return res;
  }

  const auto* trit = get_field(root, "trit", JsonVal::Kind::Int);
  if (!trit || (trit->i != -1 && trit->i != 0 && trit->i != 1)) {
    res.valid = false;
    res.reason = "invalid or missing trit domain {-1, 0, 1}";
    return res;
  }

  const auto* provider = get_field(root, "provider", JsonVal::Kind::Object);
  const auto* challenge = get_field(root, "challengeability", JsonVal::Kind::Object);
  if (!provider || !challenge) {
    res.valid = false;
    res.reason = "missing required provider or challengeability object";
    return res;
  }

  const auto* exec_cls = get_field(*provider, "execution_class", JsonVal::Kind::String);
  const auto* replayable = get_field(*challenge, "replayable", JsonVal::Kind::Bool);
  const auto* ver_mode = get_field(*challenge, "verification_mode", JsonVal::Kind::String);
  if (!exec_cls || !replayable || !ver_mode) {
    res.valid = false;
    res.reason = "missing required execution_class, replayable, or verification_mode fields";
    return res;
  }

  // Structural execution-class honesty invariant (immune to substring spoofing in other fields)
  const bool is_deterministic = (exec_cls->s == "deterministic");
  const bool claims_bit_exact = (ver_mode->s == "bit_exact_replay") || replayable->b;

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
