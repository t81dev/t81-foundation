// experimental/ternaryos/tests/device_driver_test.cpp
//
// Phase 4 acceptance tests: block device, CanonStore, framebuffer, net packet.
// RFC-00B2 acceptance criteria AC-D1 through AC-D7.

#include "../dev/block_device.hpp"
#include "../dev/hosted_block_dev.hpp"
#include "../dev/canon_store.hpp"
#include "../dev/framebuffer.hpp"
#include "../dev/net_packet.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace t81::ternaryos::dev;

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
  return cond;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static t81::canonfs::CanonBlock make_block(uint8_t fill) {
  t81::canonfs::CanonBlock b;
  b.trytes.fill(fill);
  return b;
}

// ─── AC-D1: HostedBlockDev read/write round-trip ─────────────────────────────

static void test_hosted_block_dev_rw() {
  std::printf("\n[D1] HostedBlockDev read/write round-trip\n");
  HostedBlockDev dev(32);

  auto info = dev.info();
  check(info.total_blocks == 32,           "total_blocks == 32");
  check(info.block_size_bytes == 729,      "block_size_bytes == 729");
  check(!info.read_only,                   "not read-only");
  check(dev.block_count() == 32,           "block_count() == 32");

  // Write a block
  BlockData wr{};
  wr.fill(0x42);
  check(dev.write_block(0, wr),  "write_block(0) returns true");
  check(!dev.write_block(32, wr),"write_block out-of-range returns false");

  // Read it back
  BlockData rd{};
  check(dev.read_block(0, rd),   "read_block(0) returns true");
  check(rd == wr,                "data round-trips bit-exact");
  check(!dev.read_block(99, rd), "read_block out-of-range returns false");

  // Unwritten block is zero-initialised
  BlockData zero{};
  dev.read_block(1, zero);
  check(zero == BlockData{},     "unwritten block is all-zero");
}

// ─── AC-D1 (cont): Save / load "reboot" simulation ───────────────────────────

static void test_hosted_block_dev_persist() {
  std::printf("\n[D1b] HostedBlockDev save/load (reboot simulation)\n");

  const std::string path = "/tmp/ternos_test_blockdev.blk";

  {
    HostedBlockDev dev(16);
    BlockData data{};
    for (std::size_t i = 0; i < 729; ++i) data[i] = static_cast<uint8_t>(i % 256);
    dev.write_block(3, data);
    check(dev.save(path), "save returns true");
  }

  auto loaded = HostedBlockDev::load(path);
  check(loaded.has_value(), "load returns a device");

  if (loaded) {
    check(loaded->block_count() == 16, "loaded device has 16 blocks");
    BlockData rd{};
    loaded->read_block(3, rd);
    bool ok = true;
    for (std::size_t i = 0; i < 729; ++i) {
      if (rd[i] != static_cast<uint8_t>(i % 256)) { ok = false; break; }
    }
    check(ok, "block 3 data survives save/load");
  }

  // Invalid file
  auto bad = HostedBlockDev::load("/tmp/no_such_file_ternos_12345.blk");
  check(!bad.has_value(), "load of nonexistent file returns nullopt");

  std::filesystem::remove(path);
}

// ─── AC-D2: CanonStore put / deduplication ───────────────────────────────────

static void test_canon_store_put() {
  std::printf("\n[D2] CanonStore::put and deduplication\n");
  HostedBlockDev dev(32);
  CanonStore store(dev);

  auto b1 = make_block(0x01);
  auto b2 = make_block(0x02);

  auto r1a = store.put(b1);
  check(r1a.has_value(), "put(b1) succeeds");

  auto r1b = store.put(b1);
  check(r1b.has_value(), "put(b1) again succeeds (dedup)");

  if (r1a && r1b) {
    check(r1a->hash.h.bytes == r1b->hash.h.bytes, "dedup: same ref returned");
  }
  check(store.size() == 1,            "only one entry after dedup");
  check(store.next_lba() == 2,        "only LBA 1 consumed (LBA 0 = header)");

  auto r2 = store.put(b2);
  check(r2.has_value(), "put(b2) succeeds");
  check(store.size() == 2,            "two unique blocks stored");
  check(store.next_lba() == 3,        "two data LBAs consumed");

  // Distinct refs for distinct content
  if (r1a && r2) {
    check(!(r1a->hash.h.bytes == r2->hash.h.bytes),
          "distinct content → distinct CanonRefs");
  }
}

// ─── AC-D4: CanonStore get / unknown ref ─────────────────────────────────────

static void test_canon_store_get_unknown() {
  std::printf("\n[D4] CanonStore::get returns nullopt for unknown ref\n");
  HostedBlockDev dev(16);
  CanonStore store(dev);

  t81::canonfs::CanonRef unknown_ref;
  unknown_ref.hash.h.bytes.fill(0xAB);

  check(!store.get(unknown_ref).has_value(), "unknown ref → nullopt");
  check(!store.contains(unknown_ref),        "contains() false for unknown ref");
}

// ─── AC-D3 & AC-D4: flush + reboot + rebuild ────────────────────────────────

static void test_canon_store_reboot() {
  std::printf("\n[D3] CanonStore flush + reboot cycle\n");
  const std::string path = "/tmp/ternos_test_canonstore.blk";

  // Blocks to store
  auto b1 = make_block(0x10);
  auto b2 = make_block(0x20);
  auto b3 = make_block(0x30);

  t81::canonfs::CanonRef ref1, ref2, ref3;

  // Write phase
  {
    HostedBlockDev dev(32, "nvme0");
    dev.set_backing_file(path);
    CanonStore store(dev);

    ref1 = *store.put(b1);
    ref2 = *store.put(b2);
    ref3 = *store.put(b3);
    check(store.size() == 3, "3 blocks stored pre-flush");
    check(store.flush(),     "flush returns true");
  }

  // Reload and rebuild
  auto loaded = HostedBlockDev::load(path);
  check(loaded.has_value(), "device loads after reboot");

  if (loaded) {
    CanonStore store2(*loaded);
    auto recovered = store2.rebuild_index();
    check(recovered == 3, "rebuild_index recovers 3 entries");

    auto g1 = store2.get(ref1);
    auto g2 = store2.get(ref2);
    auto g3 = store2.get(ref3);

    check(g1.has_value(), "block 1 recoverable after reboot");
    check(g2.has_value(), "block 2 recoverable after reboot");
    check(g3.has_value(), "block 3 recoverable after reboot");

    if (g1) check(g1->trytes == b1.trytes, "block 1 data intact");
    if (g2) check(g2->trytes == b2.trytes, "block 2 data intact");
    if (g3) check(g3->trytes == b3.trytes, "block 3 data intact");
  }

  std::filesystem::remove(path);
}

// ─── AC-D7: hash verification on corrupted block ─────────────────────────────

static void test_canon_store_corruption_detection() {
  std::printf("\n[D7] CanonStore detects corrupted block on read\n");
  HostedBlockDev dev(16);
  CanonStore store(dev);

  auto b = make_block(0x55);
  auto ref = *store.put(b);

  // Directly corrupt the block on the device (LBA 1 = first data block).
  BlockData corrupt{};
  dev.read_block(1, corrupt);
  corrupt[0] ^= 0xFF;  // flip bits in first tryte byte
  dev.write_block(1, corrupt);

  // get() must detect the mismatch and return nullopt.
  auto result = store.get(ref);
  check(!result.has_value(), "corrupted block → nullopt (hash mismatch detected)");
}

// ─── AC-D5: TernaryFramebuffer ───────────────────────────────────────────────

static void test_framebuffer() {
  std::printf("\n[D5] TernaryFramebuffer\n");

  TernaryFramebuffer fb;
  check(fb.width()  == 81,         "default width == 81 (3^4)");
  check(fb.height() == 27,         "default height == 27 (3^3)");
  check(fb.pixel_count() == 81*27, "pixel_count == 81*27");
  check(fb.count(0)  == 81*27,     "all pixels zero after construction");
  check(fb.count(1)  == 0,         "no +1 pixels initially");
  check(fb.count(-1) == 0,         "no -1 pixels initially");

  // set_pixel / get_pixel
  check(fb.set_pixel(0, 0, TritPixel{+1}),  "set_pixel(0,0,+1)");
  check(fb.set_pixel(80, 26, TritPixel{-1}),"set_pixel(80,26,-1)");
  check(fb.get_pixel(0,  0).value  == +1,   "get_pixel(0,0) == +1");
  check(fb.get_pixel(80, 26).value == -1,   "get_pixel(80,26) == -1");
  check(fb.get_pixel(0, 0).value   != 0,    "corner pixel is non-zero");

  // Out-of-range access
  check(!fb.set_pixel(81, 0, TritPixel{1}),  "set out-of-range returns false");
  check(fb.get_pixel(99, 99).value == 0,     "get out-of-range returns {0}");

  // clear
  fb.clear(TritPixel{+1});
  check(fb.count(1) == 81*27, "after clear(+1): all pixels == +1");
  fb.clear();
  check(fb.count(0) == 81*27, "after clear(): all pixels == 0");

  // dump_ascii is non-empty
  fb.set_pixel(5, 3, TritPixel{+1});
  fb.set_pixel(6, 3, TritPixel{-1});
  auto s = fb.dump_ascii();
  check(!s.empty(),           "dump_ascii is non-empty");
  check(s.find('+') != std::string::npos, "dump_ascii contains '+'");
  check(s.find('-') != std::string::npos, "dump_ascii contains '-'");
}

// ─── AC-D6: TernaryEthernetPacket ────────────────────────────────────────────

static void test_net_packet() {
  std::printf("\n[D6] TernaryEthernetPacket\n");

  std::array<uint8_t,6> dst = {0x01,0x02,0x03,0x04,0x05,0x06};
  std::array<uint8_t,6> src = {0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};

  // Valid payload: length multiple of 3
  std::vector<int8_t> payload = {1, 0, -1, -1, 1, 0, 0, -1, 1};  // 9 trits
  auto pkt = TernaryEthernetPacket::build(dst, src, 0x0081, payload);
  check(pkt.has_value(),               "build() succeeds for valid payload");
  if (pkt) {
    check(pkt->valid(),                "valid() returns true");
    check(pkt->trit_word_count() == 3, "3 ternary words (9/3)");
    // content_ref must be non-zero (hashed payload)
    bool nonzero = false;
    for (auto b : pkt->content_ref.hash.h.bytes) if (b) { nonzero = true; break; }
    check(nonzero, "content_ref hash is non-zero");
  }

  // Invalid: payload size not multiple of 3
  std::vector<int8_t> bad_size = {1, 0};  // 2 trits
  auto bad_pkt = TernaryEthernetPacket::build(dst, src, 0x0081, bad_size);
  check(!bad_pkt.has_value(), "build() fails for payload size not multiple of 3");

  // Invalid: out-of-range trit value
  std::vector<int8_t> bad_val = {1, 2, -1};  // 2 is out of {-1,0,+1}
  auto bad_pkt2 = TernaryEthernetPacket::build(dst, src, 0x0081, bad_val);
  check(!bad_pkt2.has_value(), "build() fails for out-of-range trit value (2)");

  // Same payload → same content_ref (deterministic hash)
  auto pkt2 = TernaryEthernetPacket::build(dst, src, 0x0081, payload);
  if (pkt && pkt2) {
    check(pkt->content_ref.hash.h.bytes == pkt2->content_ref.hash.h.bytes,
          "same payload → identical content_ref");
  }

  // Empty payload (0 trits = valid, 0 is a multiple of 3)
  auto empty_pkt = TernaryEthernetPacket::build(dst, src, 0x0081, {});
  check(empty_pkt.has_value(),              "empty payload is valid");
  if (empty_pkt) {
    check(empty_pkt->trit_word_count() == 0, "0 ternary words for empty payload");
  }
}

// ─── CanonStore capacity limit ───────────────────────────────────────────────

static void test_canon_store_capacity() {
  std::printf("\n[D2b] CanonStore Phase 4 index cap (17 entries)\n");
  HostedBlockDev dev(64);
  CanonStore store(dev);

  for (std::size_t i = 0; i < CanonStore::kMaxIndexEntries; ++i) {
    t81::canonfs::CanonBlock b;
    b.trytes.fill(static_cast<uint8_t>(i + 1));
    auto ref = store.put(b);
    check(ref.has_value(), "put within cap succeeds");
    if (!ref.has_value()) break;
  }
  check(store.size() == CanonStore::kMaxIndexEntries, "store at Phase 4 cap");

  // One more should fail
  t81::canonfs::CanonBlock overflow;
  overflow.trytes.fill(0xFF);
  auto ov = store.put(overflow);
  check(!ov.has_value(), "put beyond Phase 4 cap returns nullopt");
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== TernOS Device Driver tests (Phase 4 / RFC-00B2) ===\n");

  test_hosted_block_dev_rw();
  test_hosted_block_dev_persist();
  test_canon_store_put();
  test_canon_store_get_unknown();
  test_canon_store_reboot();
  test_canon_store_corruption_detection();
  test_framebuffer();
  test_net_packet();
  test_canon_store_capacity();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
