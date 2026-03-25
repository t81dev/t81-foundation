// fs/block_backed_driver.cpp
//
// BlockBackedDriver — t81::canonfs::Driver backed by an IBlockDevice.
//
// Wraps ternaryos/dev/CanonStore (content-addressed block store)
// to provide the full CanonFS Driver interface over any IBlockDevice
// implementation: VirtioBlkMmioDevice (bare-metal), HostedBlockDev (test /
// simulation), or any future NVMe / AHCI driver.
//
// CanonBlock payload layout (fixed header):
//   bytes[0..3]   uint32_t payload_size (little-endian)
//   bytes[4..728] payload data (max 725 bytes)
//
// Capability serialisation (all fixed-width, no strings):
//   bytes[0..31]  target.hash.h.bytes   (CanonHash81, 32 B)
//   bytes[32..33] perms                 (uint16_t LE)
//   bytes[34..41] expires_at            (uint64_t LE)
//   bytes[42..73] granted_by.h.bytes    (32 B)
//   bytes[74..105] revocable_by.h.bytes (32 B)
//   Total: 106 bytes — fits comfortably in one 725-byte payload slot.

#include "t81/canonfs/canon_driver.hpp"

#include <cstring>
#include <functional>
#include <map>
#include <memory>

#include "t81/canonfs/canon_types.hpp"
#include "t81/tracing/canonhash.hpp"

#include "dev/block_device.hpp"
#include "dev/canon_store.hpp"

namespace t81::canonfs {

namespace {

// ── Payload helpers ───────────────────────────────────────────────────────────

constexpr std::size_t kHeaderBytes = 4;
constexpr std::size_t kMaxPayload  = CanonBlock::kTryteCount - kHeaderBytes;  // 725

static_assert(kMaxPayload == 725u, "CanonBlock layout invariant");

/// Pack `bytes` into a CanonBlock with a 4-byte size header.
/// Returns nullopt if bytes.size() > kMaxPayload.
std::optional<CanonBlock> pack(std::span<const std::byte> bytes) {
  if (bytes.size() > kMaxPayload) return std::nullopt;
  CanonBlock blk{};
  const auto sz = static_cast<uint32_t>(bytes.size());
  blk.trytes[0] = static_cast<uint8_t>( sz        & 0xFFu);
  blk.trytes[1] = static_cast<uint8_t>((sz >>  8) & 0xFFu);
  blk.trytes[2] = static_cast<uint8_t>((sz >> 16) & 0xFFu);
  blk.trytes[3] = static_cast<uint8_t>((sz >> 24) & 0xFFu);
  std::memcpy(blk.trytes.data() + kHeaderBytes, bytes.data(), bytes.size());
  return blk;
}

/// Unpack a CanonBlock packed by pack(), returning the original byte span.
std::vector<std::byte> unpack(const CanonBlock& blk) {
  const uint32_t sz =
      static_cast<uint32_t>(blk.trytes[0])
    | (static_cast<uint32_t>(blk.trytes[1]) <<  8)
    | (static_cast<uint32_t>(blk.trytes[2]) << 16)
    | (static_cast<uint32_t>(blk.trytes[3]) << 24);
  const auto count = static_cast<std::size_t>(
      sz <= kMaxPayload ? sz : kMaxPayload);
  const auto* src = reinterpret_cast<const std::byte*>(
      blk.trytes.data() + kHeaderBytes);
  return std::vector<std::byte>(src, src + count);
}

// ── Capability serialisation ──────────────────────────────────────────────────

constexpr std::size_t kCapBytes = 32 + 2 + 8 + 32 + 32;  // 106

static_assert(kCapBytes <= kMaxPayload);

std::vector<std::byte> serialize_cap(const CapabilityGrant& g) {
  std::vector<std::byte> out(kCapBytes, std::byte{0});
  std::byte* p = out.data();
  std::memcpy(p, g.target.hash.h.bytes.data(), 32); p += 32;
  const uint16_t perms = g.perms;
  std::memcpy(p, &perms, 2);                         p += 2;
  const uint64_t exp = g.expires_at;
  std::memcpy(p, &exp, 8);                           p += 8;
  std::memcpy(p, g.granted_by.h.bytes.data(), 32);   p += 32;
  std::memcpy(p, g.revocable_by.h.bytes.data(), 32);
  return out;
}

[[maybe_unused]] CapabilityGrant deserialize_cap(std::span<const std::byte> raw) {
  CapabilityGrant g;
  if (raw.size() < kCapBytes) return g;
  const std::byte* p = raw.data();
  std::memcpy(g.target.hash.h.bytes.data(), p, 32);   p += 32;
  uint16_t perms = 0;
  std::memcpy(&perms, p, 2);                           p += 2;
  g.perms = perms;
  uint64_t exp = 0;
  std::memcpy(&exp, p, 8);                             p += 8;
  g.expires_at = exp;
  std::memcpy(g.granted_by.h.bytes.data(), p, 32);    p += 32;
  std::memcpy(g.revocable_by.h.bytes.data(), p, 32);
  return g;
}

// ── BlockBackedDriver ─────────────────────────────────────────────────────────

class BlockBackedDriver final : public Driver {
public:
  explicit BlockBackedDriver(std::unique_ptr<t81::ternaryos::dev::IBlockDevice> dev,
                             bool rebuild_on_open)
      : dev_(std::move(dev)), store_(*dev_) {
    if (rebuild_on_open && dev_->block_count() > 0) {
      store_.rebuild_index();
    }
  }

  void set_axion_hook(std::function<AxionVerdict(OpKind, const CanonRef&)> hook) override {
    hook_ = std::move(hook);
  }

  // ── Write ────────────────────────────────────────────────────────────────────

  Result<CanonRef> write_object(ObjectType, std::span<const std::byte> bytes) override {
    if (!axion_allow(OpKind::Write)) {
      return Result<CanonRef>(t81::unexpect, Error::CapabilityError);
    }
    auto blk = pack(bytes);
    if (!blk) {
      return Result<CanonRef>(t81::unexpect, Error::InvalidObject);
    }
    auto ref = store_.put(*blk);
    if (!ref) {
      return Result<CanonRef>(t81::unexpect, Error::DecodeError);
    }
    store_.flush();
    return *ref;
  }

  // ── Read ─────────────────────────────────────────────────────────────────────

  Result<std::vector<std::byte>> read_object_bytes(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Read)) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::CapabilityError);
    }
    auto blk = store_.get(ref);
    if (!blk) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::NotFound);
    }
    return unpack(*blk);
  }

  // ── Capabilities ──────────────────────────────────────────────────────────────

  Result<void> publish_capability(const CapabilityGrant& grant) override {
    auto raw = serialize_cap(grant);
    auto span = std::span<const std::byte>(raw.data(), raw.size());
    auto blk = pack(span);
    if (!blk) return Result<void>(t81::unexpect, Error::InvalidObject);
    auto ref = store_.put(*blk);
    if (!ref) return Result<void>(t81::unexpect, Error::DecodeError);
    caps_[ref->hash] = grant;
    store_.flush();
    return {};
  }

  Result<void> revoke_capability(const CanonRef& ref) override {
    caps_.erase(ref.hash);
    store_.remove(ref);
    store_.flush();
    return {};
  }

  // ── Parity repair ────────────────────────────────────────────────────────────

  Result<void> parity_repair_subtree(const CanonRef&) override {
    // Block store has no Reed-Solomon layer; rebuild the in-memory index from
    // on-device blocks.  This is the correct recovery action after power loss.
    store_.rebuild_index();
    return {};
  }

private:
  bool axion_allow(OpKind kind) const noexcept {
    if (!hook_) return true;
    const CanonRef nil{};
    return hook_(kind, nil).allow;
  }

  std::unique_ptr<t81::ternaryos::dev::IBlockDevice>   dev_;
  t81::ternaryos::dev::CanonStore                      store_;
  std::map<CanonHash, CapabilityGrant>                 caps_;
  std::function<AxionVerdict(OpKind, const CanonRef&)> hook_;
};

}  // namespace

// ── Factory ───────────────────────────────────────────────────────────────────

std::unique_ptr<Driver> make_block_backed_driver(
    std::unique_ptr<t81::ternaryos::dev::IBlockDevice> dev,
    bool rebuild_on_open) {
  if (!dev) return nullptr;
  return std::make_unique<BlockBackedDriver>(std::move(dev), rebuild_on_open);
}

}  // namespace t81::canonfs
