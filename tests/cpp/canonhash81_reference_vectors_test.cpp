// RFC-0000 §1: CanonHash-81 reference vector tests.
// Verifies determinism, domain separation, and CanonBlock hash consistency.
// "Reference vectors" means: a fixed input always produces the same Base-81 digest.

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "t81/canonfs/canon_types.hpp"
#include "t81/tracing/canonhash.hpp"

static bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

int main() {
  bool ok = true;

  // ── Vector 1: empty input ──────────────────────────────────────────────────
  {
    auto h1 = t81::hash::hash_string("");
    auto h2 = t81::hash::hash_string("");
    ok &= expect(h1 == h2,     "empty-string hash is non-deterministic");
    ok &= expect(h1.to_string().size() > 0, "empty-string hash produces empty Base-81 string");
  }

  // ── Vector 2: single-byte inputs ──────────────────────────────────────────
  {
    std::vector<std::uint8_t> zero_byte{0x00};
    std::vector<std::uint8_t> one_byte{0x01};
    auto hz = t81::hash::hash_bytes(zero_byte);
    auto ho = t81::hash::hash_bytes(one_byte);
    ok &= expect(hz == t81::hash::hash_bytes(zero_byte), "0x00 hash non-deterministic");
    ok &= expect(ho == t81::hash::hash_bytes(one_byte),  "0x01 hash non-deterministic");
    ok &= expect(hz != ho, "0x00 and 0x01 should hash to different values");
  }

  // ── Vector 3: known-string inputs ─────────────────────────────────────────
  {
    auto h_hello = t81::hash::hash_string("hello");
    auto h_world = t81::hash::hash_string("world");
    auto h_t81   = t81::hash::hash_string("T81CanonHash");
    ok &= expect(h_hello == t81::hash::hash_string("hello"), "\"hello\" hash non-deterministic");
    ok &= expect(h_world == t81::hash::hash_string("world"), "\"world\" hash non-deterministic");
    ok &= expect(h_hello != h_world, "\"hello\" and \"world\" must not collide");
    ok &= expect(h_hello != h_t81,   "\"hello\" and \"T81CanonHash\" must not collide");
  }

  // ── Vector 4: CanonBlock all-zeros ────────────────────────────────────────
  {
    t81::canonfs::CanonBlock blk{};  // default: all trytes = 0
    auto block_hash = blk.hash();

    // Must equal hash_bytes over a 729-byte zero array.
    std::vector<std::uint8_t> raw(729, 0x00);
    auto direct_hash = t81::hash::hash_bytes(raw);
    ok &= expect(block_hash.h.bytes == direct_hash.bytes,
                 "CanonBlock all-zeros hash != direct hash_bytes of 729 zero bytes");

    // Deterministic second call.
    ok &= expect(blk.hash().h.bytes == block_hash.h.bytes,
                 "CanonBlock::hash() is non-deterministic");
  }

  // ── Vector 5: CanonBlock all-ones (tryte value 1) ─────────────────────────
  {
    t81::canonfs::CanonBlock blk{};
    blk.trytes.fill(0x01);
    auto h1 = blk.hash();
    auto h2 = blk.hash();
    ok &= expect(h1.h.bytes == h2.h.bytes, "CanonBlock all-ones hash non-deterministic");

    // Must differ from all-zeros.
    t81::canonfs::CanonBlock zeros{};
    ok &= expect(h1.h.bytes != zeros.hash().h.bytes,
                 "all-ones CanonBlock must not hash equal to all-zeros");
  }

  // ── Vector 6: CanonBlock serialization round-trip ─────────────────────────
  {
    t81::canonfs::CanonBlock blk{};
    for (std::size_t i = 0; i < t81::canonfs::CanonBlock::kTryteCount; ++i) {
      blk.trytes[i] = static_cast<std::uint8_t>(i % 81);
    }
    auto original_hash = blk.hash();
    auto bytes = blk.to_bytes();
    ok &= expect(bytes.size() == 729, "to_bytes() must produce exactly 729 bytes");

    auto restored = t81::canonfs::CanonBlock::from_bytes(
        std::span<const std::byte>(bytes.data(), bytes.size()));
    ok &= expect(restored.has_value(), "from_bytes() failed on valid 729-byte span");
    if (restored.has_value()) {
      auto restored_hash = restored->hash();
      ok &= expect(restored_hash.h.bytes == original_hash.h.bytes,
                   "round-trip CanonBlock hash mismatch");
    }
  }

  // ── Vector 7: from_bytes() rejects wrong-size input ───────────────────────
  {
    std::vector<std::byte> short_buf(100, std::byte{0x00});
    auto res = t81::canonfs::CanonBlock::from_bytes(
        std::span<const std::byte>(short_buf.data(), short_buf.size()));
    ok &= expect(!res.has_value(), "from_bytes() must reject non-729-byte input");
  }

  // ── Vector 8: Base-81 string round-trip ───────────────────────────────────
  {
    auto h = t81::hash::hash_string("RFC-0000-reference-vector");
    std::string b81 = h.to_string();
    ok &= expect(!b81.empty(), "Base-81 string must not be empty");
    // Determinism of to_string().
    ok &= expect(h.to_string() == b81, "to_string() is non-deterministic");
  }

  if (ok) {
    std::cout << "canonhash81_reference_vectors_test: all checks PASSED\n";
    return 0;
  }
  std::cerr << "canonhash81_reference_vectors_test: FAILED\n";
  return 1;
}
