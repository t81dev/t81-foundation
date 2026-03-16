#pragma once

// include/t81/jit/jit_trace_cache.hpp
//
// RFC-0028 §4 — CanonFS-backed JIT trace cache.
//
// JitTraceCache wraps a canonfs::Driver to provide content-addressed
// persistence for compiled JitTrace objects.  Each entry is stored as a
// CanonJitTrace (ObjectType 0x17) blob and indexed by its canonical
// trace_hash (CanonHash81 of the serialised TISC instruction sequence, as
// defined in RFC-0028 §2).
//
// Serialised payload format (little-endian):
//   [0..1]    Magic: 0x4A 0x54 ("JT")
//   [2]       Version: 0x01
//   [3]       Reserved: 0x00
//   [4..7]    instruction_count (uint32)
//   [8..39]   trace_hash (32 bytes, for integrity check on read)
//   For each instruction (19 bytes):
//     [0..1]  opcode   (uint16)
//     [2..5]  a        (int32)
//     [6..13] b        (int64)
//     [14..17] c       (int32)
//     [18]    literal_kind (uint8)
//
// The in-memory index (trace_hash → CanonRef) is populated on every store()
// call and used by lookup().  For cross-session persistence the index can be
// rebuilt by re-scanning stored objects; this is left as a future enhancement.
//
// Thread safety: not thread-safe.  External synchronisation required if
// shared across threads.

#include <functional>
#include <map>
#include <memory>
#include <optional>

#include "t81/canonfs/canon_driver.hpp"
#include "t81/jit/jit.hpp"
#include "t81/tracing/canonhash.hpp"

namespace t81::vm {

class JitTraceCache {
public:
  /// Construct a cache backed by the given CanonFS driver.
  /// The driver must outlive this cache.
  explicit JitTraceCache(std::shared_ptr<t81::canonfs::Driver> driver);

  // Non-copyable; move-only.
  JitTraceCache(const JitTraceCache&) = delete;
  JitTraceCache& operator=(const JitTraceCache&) = delete;
  JitTraceCache(JitTraceCache&&) = default;
  JitTraceCache& operator=(JitTraceCache&&) = default;

  /// Serialise and store a compiled trace in CanonFS.
  ///
  /// @param trace  A compiled JitTrace (must have a non-zero trace_hash()).
  /// @returns      The CanonRef assigned by the driver, or std::nullopt on
  ///               failure (zero trace_hash, serialisation error, or Axion
  ///               policy deny).
  std::optional<t81::canonfs::CanonRef> store(const JitTrace& trace);

  /// Look up a previously stored trace by its canonical trace_hash.
  ///
  /// @param hash   The CanonHash81 returned by JitTrace::trace_hash().
  /// @returns      A reconstructed JitTrace, or nullptr if not found or if
  ///               the stored payload fails integrity verification.
  std::unique_ptr<JitTrace> lookup(const t81::hash::CanonHash81& hash) const;

  /// Set an Axion verdict hook on the underlying CanonFS driver.
  /// Passed through to driver->set_axion_hook().
  void set_axion_hook(
      std::function<t81::canonfs::AxionVerdict(
          t81::canonfs::OpKind, const t81::canonfs::CanonRef&)> hook);

  /// Number of traces currently indexed in this cache instance.
  std::size_t size() const noexcept { return index_.size(); }

private:
  std::shared_ptr<t81::canonfs::Driver> driver_;

  // Maps trace_hash → CanonRef (content address of the full serialised blob).
  // Using CanonHash as map key via its operator< from canon_types.hpp.
  std::map<t81::canonfs::CanonHash, t81::canonfs::CanonRef> index_;
};

}  // namespace t81::vm
