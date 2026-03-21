#include "internal/runtime_state_helpers.hpp"

#include <algorithm>

namespace t81::vm::internal {
namespace {

std::uint64_t fnv1a64(std::uint64_t hash, std::uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    hash ^= static_cast<std::uint8_t>((value >> (i * 8)) & 0xFFu);
    hash *= 1099511628211ull;
  }
  return hash;
}

}  // namespace

std::int64_t compute_lineage_signature(const t81::tisc::Program& program) {
  std::uint64_t hash = 1469598103934665603ull;
  hash = fnv1a64(hash, static_cast<std::uint64_t>(program.insns.size()));
  for (const auto& insn : program.insns) {
    hash = fnv1a64(hash, static_cast<std::uint64_t>(insn.opcode));
    hash = fnv1a64(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(insn.a)));
    hash = fnv1a64(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(insn.b)));
    hash = fnv1a64(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(insn.c)));
    hash = fnv1a64(hash, static_cast<std::uint64_t>(insn.literal_kind));
  }
  for (char ch : program.axion_policy_text) {
    hash = fnv1a64(hash, static_cast<std::uint64_t>(static_cast<unsigned char>(ch)));
  }
  for (char ch : program.match_metadata_text) {
    hash = fnv1a64(hash, static_cast<std::uint64_t>(static_cast<unsigned char>(ch)));
  }
  std::int64_t sig = static_cast<std::int64_t>(hash & 0x7FFFFFFFFFFFFFFFull);
  return sig == 0 ? 1 : sig;
}

std::int64_t compute_entropy_signature(std::size_t instruction_count,
                                       std::size_t contradiction_events, const ThreadContext& ctx) {
  std::uint64_t hash = 1469598103934665603ull;
  hash = fnv1a64(hash, static_cast<std::uint64_t>(instruction_count));
  hash = fnv1a64(hash, static_cast<std::uint64_t>(contradiction_events));
  hash = fnv1a64(hash, static_cast<std::uint64_t>(ctx.call_depth));
  hash = fnv1a64(hash, static_cast<std::uint64_t>(ctx.stack_frames.size()));
  hash = fnv1a64(hash, static_cast<std::uint64_t>(ctx.pc));
  hash = fnv1a64(hash, static_cast<std::uint64_t>(ctx.tier_status.current));
  std::int64_t sig = static_cast<std::int64_t>(hash & 0x7FFFFFFFFFFFFFFFull);
  return sig == 0 ? 1 : sig;
}

std::int64_t compute_constitutional_mask(const State& state) {
  constexpr std::int64_t kPolicyLoaded = (1ll << 0);
  constexpr std::int64_t kMaxInstructions = (1ll << 1);
  constexpr std::int64_t kMaxRecursion = (1ll << 2);
  constexpr std::int64_t kMaxStack = (1ll << 3);
  constexpr std::int64_t kMaxReflections = (1ll << 4);
  constexpr std::int64_t kMaxMetaWrites = (1ll << 5);
  constexpr std::int64_t kAllowedTensorHashes = (1ll << 6);
  constexpr std::int64_t kMatchGuards = (1ll << 7);
  constexpr std::int64_t kSegmentOrAxionRequirements = (1ll << 8);
  constexpr std::int64_t kAlignmentRequirements = (1ll << 9);

  if (!state.policy.has_value()) return 0;
  std::int64_t mask = kPolicyLoaded;
  const auto& p = *state.policy;
  if (p.max_instructions.has_value()) mask |= kMaxInstructions;
  if (p.max_recursion.has_value()) mask |= kMaxRecursion;
  if (p.max_stack.has_value()) mask |= kMaxStack;
  if (p.max_reflections.has_value()) mask |= kMaxReflections;
  if (p.max_meta_writes.has_value()) mask |= kMaxMetaWrites;
  if (!p.allowed_tensor_hashes.empty()) mask |= kAllowedTensorHashes;
  if (!p.match_guards.empty()) mask |= kMatchGuards;
  if (!p.segment_requirements.empty() || !p.axion_event_requirements.empty()) {
    mask |= kSegmentOrAxionRequirements;
  }
  if (!p.alignment_requirements.empty()) mask |= kAlignmentRequirements;
  return mask;
}

void sync_system_registers(State& state, const t81::tisc::Program& program,
                           std::size_t instruction_count, std::size_t current_context) {
  if (state.contexts.empty() || current_context >= state.contexts.size()) return;
  auto& ctx = state.contexts[current_context];
  ctx.registers[0] = 0;
  ctx.register_tags[0] = ValueTag::Int;

  // R75: Global Tick.
  ctx.registers[75] = static_cast<std::int64_t>(instruction_count);
  ctx.register_tags[75] = ValueTag::Int;

  // R76: Lineage Root Hash.
  ctx.registers[76] = compute_lineage_signature(program);
  ctx.register_tags[76] = ValueTag::Int;

  // R77: Current Entropy Signature.
  ctx.registers[77] = compute_entropy_signature(instruction_count, state.contradiction_events, ctx);
  ctx.register_tags[77] = ValueTag::Int;

  // R78: Active Constitutional Mask.
  ctx.registers[78] = compute_constitutional_mask(state);
  ctx.register_tags[78] = ValueTag::Int;

  // R79: Recursion depth counter.
  ctx.registers[79] = static_cast<std::int64_t>(std::max(ctx.stack_frames.size(), ctx.call_depth));
  ctx.register_tags[79] = ValueTag::Int;

  // R80: Axion seal / capability word.
  ctx.registers[80] = state.halted ? 0 : 1;
  ctx.register_tags[80] = ValueTag::Int;
}

}  // namespace t81::vm::internal
