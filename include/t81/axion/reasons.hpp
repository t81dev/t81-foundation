#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace t81::axion {

/**
 * @namespace reasons
 * @brief Canonical Axion reason strings for deterministic trace auditing.
 */
namespace reasons {

// --- Core VM Events ---
constexpr std::string_view kStep = "step";
constexpr std::string_view kHalt = "halt";

// --- Memory Segment Access ---
constexpr std::string_view kMemLoad = "memory load";
constexpr std::string_view kMemStore = "memory store";
constexpr std::string_view kStackAlloc = "stack frame allocated";
constexpr std::string_view kStackFree = "stack frame freed";
constexpr std::string_view kHeapAlloc = "heap block allocated";
constexpr std::string_view kHeapFree = "heap block freed";
constexpr std::string_view kTensorAlloc = "tensor slot allocated";
constexpr std::string_view kStringSplit = "string split";
constexpr std::string_view kStringJoin = "string join";

// --- Faults ---
constexpr std::string_view kBoundsFault = "bounds fault";
constexpr std::string_view kStackFault = "stack fault";
constexpr std::string_view kTypeFault = "type fault";
constexpr std::string_view kDivisionFault = "division fault";
constexpr std::string_view kSecurityFault = "security fault";
constexpr std::string_view kShapeFault = "shape fault";
constexpr std::string_view kDecodeFault = "decode fault";
constexpr std::string_view kRecursionCeiling = "recursion ceiling exceeded";
constexpr std::string_view kContradictionDetected = "contradiction detected";

// --- Garbage Collection ---
constexpr std::string_view kGcCycle = "GC cycle";
constexpr std::string_view kHeapCompaction = "heap compaction";
constexpr std::string_view kHeapRelocation = "heap relocation";

// --- Tier 4 Reflection & Meta-Opcodes ---
constexpr std::string_view kCogTier2Reflect = "cog:tier2:reflect";
constexpr std::string_view kMetaRead = "METAREAD";
constexpr std::string_view kMetaWrite = "METAWRITE";
constexpr std::string_view kMetaReflect = "METAREFLECT";
constexpr std::string_view kMetaRefine = "METAREFINE";
constexpr std::string_view kCogTier4Reflect = "cog:tier4:reflect";

// --- Kernel/Policy Events ---
constexpr std::string_view kAxRead = "AXREAD";
constexpr std::string_view kAxSet = "AXSET";
constexpr std::string_view kAxVerify = "AXVERIFY";
constexpr std::string_view kAxCheck = "AXCHECK";
constexpr std::string_view kAxReport = "AXREPORT";
constexpr std::string_view kJitTraceEnter = "jit trace enter";
constexpr std::string_view kJitTraceExit = "jit trace exit";
constexpr std::string_view kJitTraceDeopt = "jit trace deopt";
constexpr std::string_view kEnumGuard = "enum guard";
constexpr std::string_view kEnumPayload = "enum payload";
constexpr std::string_view kMetaSlotAxionEvent = "axion event";
constexpr std::string_view kTensorProvenance = "tensor provenance";

// --- AI-Native Inference (RFC-0026) ---
// Canonical CanonFS action string for WLOAD weight materialization audit events.
// Produces "meta slot axion event segment=meta addr=<n> action=WeightLoad"
// via log_canonfs_operation() when a CanonFS driver is attached (AI-M4).
constexpr std::string_view kWeightLoad = "WeightLoad";

// ---------------------------------------------------------------------------
// Canonical reason string builders (AX-M6)
//
// These functions produce the normative verbatim strings required by RFC-0020
// and spec/axion-kernel.md §1.8. All callers MUST use these builders instead
// of constructing segment/addr/action reason strings ad-hoc.
// ---------------------------------------------------------------------------

/// "meta slot axion event segment=meta addr=<addr>"
/// Used by policy_trace_bridge log_meta_slot() for every AxionEvent recorded.
inline std::string canonical_meta_slot_reason(std::size_t addr) {
  return "meta slot axion event segment=meta addr=" + std::to_string(addr);
}

/// "meta slot axion event segment=meta addr=<addr> action=<action>"
/// Used by CanonFS hook for Write/Read/Publish/Revoke/Repair operations.
inline std::string canonical_meta_slot_reason(std::size_t addr, std::string_view action) {
  std::string result;
  result.reserve(64);
  result += "meta slot axion event segment=meta addr=";
  result += std::to_string(addr);
  result += " action=";
  result += action;
  return result;
}

/// "axion event segment=meta addr=<addr> action=<action>"
/// Secondary trace record emitted alongside the meta slot event in CanonFS hook.
inline std::string canonical_meta_event_reason(std::size_t addr, std::string_view action) {
  std::string result;
  result.reserve(64);
  result += kMetaSlotAxionEvent;  // "axion event"
  result += " segment=meta addr=";
  result += std::to_string(addr);
  result += " action=";
  result += action;
  return result;
}

/// "segment=<segment> addr=<addr> action=<action>"
/// Normative RFC-0020 segment-trace format for general segment access events.
inline std::string canonical_segment_reason(std::string_view segment, std::size_t addr,
                                            std::string_view action) {
  std::string result;
  result.reserve(64);
  result += "segment=";
  result += segment;
  result += " addr=";
  result += std::to_string(addr);
  result += " action=";
  result += action;
  return result;
}

/// "<guard_type> guard segment=<segment> addr=<addr>"
/// Used for AXREAD/AXSET guard events, e.g. "AxRead guard segment=stack addr=42".
inline std::string canonical_guard_reason(std::string_view guard_type, std::string_view segment,
                                          std::size_t addr) {
  std::string result;
  result.reserve(64);
  result += guard_type;
  result += " guard segment=";
  result += segment;
  result += " addr=";
  result += std::to_string(addr);
  return result;
}

/// "bounds fault segment=<segment> addr=<addr> action=<action>"
/// Canonical form for all bounds fault reason strings.
inline std::string canonical_bounds_fault_reason(std::string_view segment, int addr,
                                                 std::string_view action) {
  std::string result;
  result.reserve(64);
  result += kBoundsFault;  // "bounds fault"
  result += " segment=";
  result += segment;
  result += " addr=";
  result += std::to_string(addr);
  result += " action=";
  result += action;
  return result;
}

/// "tensor provenance segment=tensor addr=<addr> action=<action> storage=<storage> numeric=<numeric> strict=<strict>"
inline std::string canonical_tensor_provenance_reason(std::size_t addr, std::string_view action,
                                                      std::string_view storage,
                                                      std::string_view numeric, bool strict) {
  std::string result;
  result.reserve(128);
  result += kTensorProvenance;
  result += " segment=tensor addr=";
  result += std::to_string(addr);
  result += " action=";
  result += action;
  result += " storage=";
  result += storage;
  result += " numeric=";
  result += numeric;
  result += " strict=";
  result += strict ? "1" : "0";
  return result;
}

}  // namespace reasons

/**
 * @struct StructuredEvent
 * @brief Represents a structured Axion event for consistent audit trails.
 */
struct StructuredEvent {
  std::string reason;
  std::uint32_t policy_id{0};
  std::uint64_t pc{0};
  std::int64_t handle_id{0};
  std::string_view decision;
  std::string event_type;
  std::string reason_code;
  std::string storage_class;
  std::string numeric_class;
  bool strict_core_eligible{false};
};

}  // namespace t81::axion
