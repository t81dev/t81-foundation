// runtime/jit/jit_trace_cache.cpp
//
// RFC-0028 §4 — CanonFS-backed JIT trace cache implementation.

#include "t81/jit/jit_trace_cache.hpp"

#include <cassert>
#include <cstring>

#include "t81/canonfs/canon_types.hpp"
#include "t81/tracing/canonhash.hpp"

// ThreadedJitTrace is defined inside jit_compiler.cpp (same translation unit
// group via the OBJECT library).  We only need its public base JitTrace here.
// Reconstruction uses the public JitCompiler + record_instruction path.
#include "t81/jit/jit.hpp"
#include "t81/isa/program.hpp"

namespace t81::vm {

// ── Wire-format constants ─────────────────────────────────────────────────────

static constexpr std::uint8_t kMagic0   = 0x4A;  // 'J'
static constexpr std::uint8_t kMagic1   = 0x54;  // 'T'
static constexpr std::uint8_t kVersion  = 0x01;
static constexpr std::size_t  kHeaderSize   = 40;  // 4 (magic+ver+res) + 4 (count) + 32 (hash)
static constexpr std::size_t  kInsnWireSize = 19;  // opcode(2)+a(4)+b(8)+c(4)+literal_kind(1)

// ── Serialisation helpers ─────────────────────────────────────────────────────

static void write_u16le(std::vector<std::byte>& out, std::uint16_t v) {
  out.push_back(static_cast<std::byte>(v & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
}

static void write_u32le(std::vector<std::byte>& out, std::uint32_t v) {
  for (int i = 0; i < 4; ++i)
    out.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
}

static void write_i32le(std::vector<std::byte>& out, std::int32_t v) {
  write_u32le(out, static_cast<std::uint32_t>(v));
}

static void write_i64le(std::vector<std::byte>& out, std::int64_t v) {
  for (int i = 0; i < 8; ++i)
    out.push_back(static_cast<std::byte>((static_cast<std::uint64_t>(v) >> (8 * i)) & 0xFF));
}

static std::uint16_t read_u16le(const std::byte* p) {
  return static_cast<std::uint16_t>(
      static_cast<std::uint8_t>(p[0]) |
      (static_cast<std::uint16_t>(static_cast<std::uint8_t>(p[1])) << 8));
}

static std::uint32_t read_u32le(const std::byte* p) {
  std::uint32_t v = 0;
  for (int i = 0; i < 4; ++i)
    v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(p[i])) << (8 * i);
  return v;
}

static std::int32_t read_i32le(const std::byte* p) {
  return static_cast<std::int32_t>(read_u32le(p));
}

static std::int64_t read_i64le(const std::byte* p) {
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i)
    v |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(p[i])) << (8 * i);
  return static_cast<std::int64_t>(v);
}

// ── Serialise a JitTrace to the wire format ───────────────────────────────────

static std::vector<std::byte> serialise(const JitTrace& trace,
                                        const std::vector<t81::tisc::Insn>& insns) {
  const auto n = static_cast<std::uint32_t>(insns.size());
  std::vector<std::byte> out;
  out.reserve(kHeaderSize + kInsnWireSize * n);

  // Header: magic, version, reserved.
  out.push_back(static_cast<std::byte>(kMagic0));
  out.push_back(static_cast<std::byte>(kMagic1));
  out.push_back(static_cast<std::byte>(kVersion));
  out.push_back(std::byte{0});  // reserved

  // Instruction count.
  write_u32le(out, n);

  // trace_hash (32 bytes) — for integrity verification on read.
  for (const auto b : trace.trace_hash().bytes)
    out.push_back(static_cast<std::byte>(b));

  // Instructions.
  for (const auto& insn : insns) {
    write_u16le(out, static_cast<std::uint16_t>(insn.opcode));
    write_i32le(out, insn.a);
    write_i64le(out, insn.b);
    write_i32le(out, insn.c);
    out.push_back(static_cast<std::byte>(insn.literal_kind));
  }

  return out;
}

// ── Deserialise wire bytes to a JitTrace ─────────────────────────────────────

static std::unique_ptr<JitTrace> deserialise(const std::vector<std::byte>& raw) {
  if (raw.size() < kHeaderSize) return nullptr;

  const std::byte* p = raw.data();

  // Verify magic and version.
  if (static_cast<std::uint8_t>(p[0]) != kMagic0 ||
      static_cast<std::uint8_t>(p[1]) != kMagic1 ||
      static_cast<std::uint8_t>(p[2]) != kVersion) {
    return nullptr;
  }

  const std::uint32_t count = read_u32le(p + 4);

  // Read stored trace_hash (bytes 8..39).
  t81::hash::CanonHash81 stored_hash{};
  for (std::size_t i = 0; i < 32; ++i)
    stored_hash.bytes[i] = static_cast<std::uint8_t>(p[8 + i]);

  const std::size_t expected_size = kHeaderSize + kInsnWireSize * count;
  if (raw.size() < expected_size) return nullptr;

  // Reconstruct instruction vector.
  std::vector<t81::tisc::Insn> insns;
  insns.reserve(count);
  const std::byte* ip = p + kHeaderSize;
  for (std::uint32_t i = 0; i < count; ++i, ip += kInsnWireSize) {
    t81::tisc::Insn insn;
    insn.opcode       = static_cast<t81::tisc::Opcode>(read_u16le(ip));
    insn.a            = read_i32le(ip + 2);
    insn.b            = read_i64le(ip + 6);
    insn.c            = read_i32le(ip + 14);
    insn.literal_kind = static_cast<t81::tisc::LiteralKind>(
        static_cast<std::uint8_t>(ip[18]));
    insns.push_back(insn);
  }

  // Re-compile through JitCompiler to get a proper ThreadedJitTrace
  // with the trace_hash set.  This also recomputes the hash for integrity.
  JitCompiler compiler;
  compiler.start_tracing(0);
  for (const auto& insn : insns)
    compiler.record_instruction(insn);
  auto trace = compiler.compile();
  if (!trace) return nullptr;

  // Verify integrity: recomputed hash must match stored hash.
  if (trace->trace_hash().bytes != stored_hash.bytes) return nullptr;

  return trace;
}

// ── JitTraceCache ─────────────────────────────────────────────────────────────

JitTraceCache::JitTraceCache(std::shared_ptr<t81::canonfs::Driver> driver)
    : driver_(std::move(driver)) {
  assert(driver_ != nullptr && "JitTraceCache requires a non-null Driver");
}

void JitTraceCache::set_axion_hook(
    std::function<t81::canonfs::AxionVerdict(
        t81::canonfs::OpKind, const t81::canonfs::CanonRef&)> hook) {
  driver_->set_axion_hook(std::move(hook));
}

std::optional<t81::canonfs::CanonRef> JitTraceCache::store(const JitTrace& trace) {
  // Reject traces with zero hash (not compiled via JitCompiler).
  const auto& th = trace.trace_hash();
  const bool zero = std::all_of(th.bytes.begin(), th.bytes.end(),
                                [](std::uint8_t b) { return b == 0; });
  if (zero) return std::nullopt;

  const auto insns_span = trace.instructions();

  std::vector<t81::tisc::Insn> insns(insns_span.begin(), insns_span.end());
  auto payload = serialise(trace, insns);

  auto result = driver_->write_object(
      t81::canonfs::ObjectType::CanonJitTrace,
      std::span<const std::byte>(payload));

  if (!result.has_value()) return std::nullopt;

  const auto ref = *result;
  // Record in session index: trace_hash → CanonRef.
  t81::canonfs::CanonHash key{th};
  index_[key] = ref;
  return ref;
}

std::unique_ptr<JitTrace> JitTraceCache::lookup(
    const t81::hash::CanonHash81& hash) const {
  t81::canonfs::CanonHash key{hash};
  const auto it = index_.find(key);
  if (it == index_.end()) return nullptr;

  auto result = driver_->read_object_bytes(it->second);
  if (!result.has_value()) return nullptr;

  return deserialise(*result);
}

}  // namespace t81::vm
