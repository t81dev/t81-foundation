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
constexpr std::string_view kMetaRead = "METAREAD";
constexpr std::string_view kMetaWrite = "METAWRITE";
constexpr std::string_view kMetaReflect = "METAREFLECT";
constexpr std::string_view kMetaRefine = "METAREFINE";
constexpr std::string_view kCogTier4Reflect = "cog:tier4:reflect";

// --- Kernel/Policy Events ---
constexpr std::string_view kAxRead = "AXREAD";
constexpr std::string_view kAxSet = "AXSET";
constexpr std::string_view kAxVerify = "AXVERIFY";
constexpr std::string_view kJitTraceEnter = "jit trace enter";
constexpr std::string_view kJitTraceExit = "jit trace exit";
constexpr std::string_view kJitTraceDeopt = "jit trace deopt";
constexpr std::string_view kEnumGuard = "enum guard";
constexpr std::string_view kEnumPayload = "enum payload";
constexpr std::string_view kMetaSlotAxionEvent = "axion event";

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
};

}  // namespace t81::axion
