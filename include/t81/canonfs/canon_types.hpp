#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "t81/tracing/canonhash.hpp"
#include "t81/types/base81.hpp"

namespace t81::canonfs {
struct CanonHash {
  t81::hash::CanonHash81 h;

  bool operator<(const CanonHash& o) const noexcept { return h.bytes < o.h.bytes; }
  bool operator==(const CanonHash& o) const noexcept { return h.bytes == o.h.bytes; }
};

// Canonical object kinds per spec/supplemental/canonfs-spec.md.
enum class ObjectType : std::uint8_t {
  RawBlock = 0x00,
  FileNode = 0x01,
  Directory = 0x02,
  Snapshot = 0x03,
  CapabilityGrant = 0x04,
  CapabilityRevoke = 0x05,
  CompressedBlock = 0x10,
  CanonParity = 0x11,
  CanonIndex = 0x12,
  CanonMeta = 0x13,
  CanonSeal = 0x14,
  CanonLink = 0x15,
  CanonExec = 0x16,
  CanonTensor = 0x20,  // Was CanonView
};

struct CanonRef {
  CanonHash hash;  // Content address (spec/supplemental/canonfs-spec.md §2).
};

struct CapabilityGrant {
  CanonRef target;
  struct Subject {
    std::string id;
    std::string pubkey;  // placeholder
  } subject;
  std::uint16_t perms{0};
  CanonHash granted_by;
  std::uint64_t expires_at{0};
  CanonHash revocable_by;
  // signature...
};

struct CanonLink {
  CanonRef target;
  std::optional<std::string> display_hint;
};

struct CanonParityShard {
  CanonRef original;
  std::vector<std::byte> shard_data;
};

// Sparse inverted index tensor
struct CanonIndex {
  struct TermEntry {
    CanonHash term_hash;
    std::vector<CanonRef> refs;
    std::vector<std::uint32_t> offsets;
  };
  std::vector<TermEntry> terms;
};

// Sparse metadata tensor
struct CanonMeta {
  struct MetaPair {
    CanonHash key_hash;
    CanonHash value_hash;
  };
  std::vector<MetaPair> pairs;
};

// Encrypted envelope
struct CanonSeal {
  std::array<std::byte, 24> nonce;  // 24 trytes? Spec says 24 trytes. Assuming byte mapping.
  std::vector<std::byte> ciphertext;
  std::array<std::byte, 16> tag;  // 16 trytes
};

// Optional permission bits (example; extend as needed)
enum : uint16_t {
  CANON_PERM_READ = 1u << 0,
  CANON_PERM_WRITE = 1u << 1,
  CANON_PERM_APPEND = 1u << 2,
  CANON_PERM_ADMIN = 1u << 15
};

// Minimal Axion verdict hook for CanonFS operations.
struct AxionVerdict {
  bool allow{true};
  std::string reason;
};

enum class OpKind {
  Read,
  Write,
  Publish,
  Revoke,
  Repair,
};
}  // namespace t81::canonfs
