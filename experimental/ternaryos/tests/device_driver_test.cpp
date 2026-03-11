// experimental/ternaryos/tests/device_driver_test.cpp
//
// Phase 4 acceptance tests: block device, CanonStore, framebuffer, net packet.
// RFC-00B2 acceptance criteria AC-D1 through AC-D7.

#include "../dev/block_device.hpp"
#include "../dev/hosted_block_dev.hpp"
#include "../dev/virtualbox_ahci_dev.hpp"
#include "../dev/virtualbox_e1000_dev.hpp"
#include "../dev/virtualbox_vmsvga_dev.hpp"
#include "../dev/canon_store.hpp"
#include "../dev/framebuffer.hpp"
#include "../dev/net_packet.hpp"
#include "../dev/ttf.hpp"
#include "../hal/virtualbox_guest_devices.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace t81::ternaryos::dev;
using namespace t81::ternaryos::hal;

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

// ─── AC-D1c: VirtualBox AHCI adapter over hosted backing ────────────────────

static void test_virtualbox_ahci_adapter() {
  std::printf("\n[D1c] VirtualBox AHCI adapter scaffold\n");

  HostedBlockDev backing(24, "hosted-ahci-backing");
  VirtualBoxAhciDev dev(backing);

  auto info = dev.info();
  check(info.total_blocks == 24,      "AHCI adapter total_blocks == 24");
  check(info.block_size_bytes == 729, "AHCI adapter block size == 729");
  check(info.device_id == "vbox-ahci0", "AHCI adapter device id == vbox-ahci0");

  const auto& ahci = dev.ahci_info();
  check(ahci.abar_base == 0xF0400000ULL, "AHCI ABAR base matches profile");
  check(ahci.abar_span_bytes == 0x2000ULL, "AHCI ABAR span matches profile");
  check(ahci.irq == 19, "AHCI IRQ == 19");
  check(ahci.port_count == 1, "AHCI scaffold exposes one port");
  check(ahci.bootable, "AHCI scaffold marked bootable");

  BlockData wr{};
  wr.fill(0x5A);
  check(dev.write_block(2, wr), "AHCI adapter write_block succeeds");

  BlockData rd{};
  check(dev.read_block(2, rd), "AHCI adapter read_block succeeds");
  check(rd == wr, "AHCI adapter round-trips block data");
  check(dev.flush(), "AHCI adapter flush succeeds");

  check(dev.read_ops() == 1, "AHCI adapter read op count increments");
  check(dev.write_ops() == 1, "AHCI adapter write op count increments");
  check(dev.flush_ops() == 1, "AHCI adapter flush op count increments");
  check(backing.block_count() == dev.block_count(), "backing and adapter block counts match");
}

// ─── AC-D1d: VirtualBox E1000 adapter scaffold ─────────────────────────────

static void test_virtualbox_e1000_adapter() {
  std::printf("\n[D1d] VirtualBox E1000 adapter scaffold\n");

  VirtualBoxE1000Dev dev;
  check(dev.device_id() == "vbox-e10000", "E1000 adapter device id == vbox-e10000");

  const auto& nic = dev.e1000_info();
  check(nic.abar_base == 0xF0200000ULL, "E1000 MMIO base matches profile");
  check(nic.abar_span_bytes == 0x20000ULL, "E1000 MMIO span matches profile");
  check(nic.irq == 11, "E1000 IRQ == 11");
  check(nic.mac[0] == 0x08 && nic.mac[2] == 0x27, "E1000 scaffold exposes VBox-style MAC prefix");
  check(nic.link_up, "E1000 scaffold starts link-up");

  auto packet = TernaryEthernetPacket::build(
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
      nic.mac,
      0x0081,
      {1, 0, -1, 1, 0, -1});
  check(packet.has_value(), "E1000 test packet builds");

  auto frame = packet ? dev.send_packet(*packet) : std::nullopt;
  check(frame.has_value(), "E1000 send_packet serializes a frame");
  check(dev.tx_frames() == 1, "E1000 tx frame count increments");
  check(dev.tx_bytes() == (frame ? frame->size() : 0), "E1000 tx byte count increments");
  check(dev.pending_tx_frames() == 1, "E1000 queues one transmitted frame");

  check(frame && dev.inject_frame(*frame), "E1000 loopback frame injection succeeds");
  check(dev.rx_frames() == 1, "E1000 rx frame count increments");
  check(dev.pending_rx_frames() == 1, "E1000 queues one received frame");

  auto parsed = dev.receive_packet();
  check(parsed.has_value(), "E1000 receive_packet parses queued frame");
  check(parsed && parsed->trit_payload == packet->trit_payload, "E1000 preserves ternary payload");
  check(parsed && parsed->content_ref.hash.h.bytes == packet->content_ref.hash.h.bytes,
        "E1000 preserves packet content hash");
  check(dev.pending_rx_frames() == 0, "E1000 receive_packet drains RX queue");
}

// ─── AC-D1e: VirtualBox VMSVGA adapter scaffold ────────────────────────────

static void test_virtualbox_vmsvga_adapter() {
  std::printf("\n[D1e] VirtualBox VMSVGA adapter scaffold\n");

  VirtualBoxVmsvgaDev dev;
  check(dev.device_id() == "vbox-vmsvga0", "VMSVGA adapter device id == vbox-vmsvga0");

  const auto& display = dev.vmsvga_info();
  check(display.mmio_base == 0xE0000000ULL, "VMSVGA MMIO base matches profile");
  check(display.mmio_span_bytes == 0x01000000ULL, "VMSVGA MMIO span matches profile");
  check(display.irq == 16, "VMSVGA IRQ == 16");
  check(display.width == 81, "VMSVGA default width == 81");
  check(display.height == 27, "VMSVGA default height == 27");

  auto& fb = dev.framebuffer();
  check(fb.set_pixel(1, 1, TritPixel{1}), "VMSVGA framebuffer accepts pixel writes");
  check(fb.set_pixel(2, 1, TritPixel{-1}), "VMSVGA framebuffer accepts second pixel write");
  check(dev.present(), "VMSVGA present succeeds");
  check(dev.present_count() == 1, "VMSVGA present count increments");

  const auto presented = dev.last_present_ascii();
  check(!presented.empty(), "VMSVGA present captures an ASCII frame");
  check(presented.find('+') != std::string::npos, "VMSVGA present includes + pixel");
  check(presented.find('-') != std::string::npos, "VMSVGA present includes - pixel");
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

// ─── AC-D3b: VirtualBox guest bootstrap persistence path ───────────────────

static void test_virtualbox_guest_reboot_persistence() {
  std::printf("\n[D3b] VirtualBox guest bootstrap persistence path\n");

  const std::string path = "/tmp/ternos_test_vbox_guest_reboot.blk";
  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  auto block = make_block(0x44);
  t81::canonfs::CanonRef stored_ref;

  {
    HostedBlockDev backing(32, "vbox-guest-reboot");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    check(guest.has_value(), "bootstrap_virtualbox_guest succeeds before reboot");
    if (!guest) {
      std::filesystem::remove(path);
      return;
    }

    CanonStore store(*guest->storage.device);
    auto ref = store.put(block);
    check(ref.has_value(), "guest storage stores a CanonBlock");
    if (!ref) {
      std::filesystem::remove(path);
      return;
    }
    stored_ref = *ref;
    check(store.flush(), "guest storage flush succeeds");
  }

  auto loaded1 = HostedBlockDev::load(path);
  check(loaded1.has_value(), "device reloads after first reboot");
  if (!loaded1) {
    std::filesystem::remove(path);
    return;
  }

  auto guest1 = bootstrap_virtualbox_guest(spec, *loaded1);
  check(guest1.has_value(), "bootstrap_virtualbox_guest succeeds after first reboot");
  if (!guest1) {
    std::filesystem::remove(path);
    return;
  }

  CanonStore recovered1(*guest1->storage.device);
  check(recovered1.rebuild_index() == 1, "rebuild_index finds one stored block");
  auto block1 = recovered1.get(stored_ref);
  check(block1.has_value(), "stored CanonRef survives first guest reboot");
  if (block1) {
    check(block1->trytes == block.trytes, "stored block payload intact after first reboot");
  }
  check(recovered1.flush(), "recovered guest storage flush succeeds");

  auto loaded2 = HostedBlockDev::load(path);
  check(loaded2.has_value(), "device reloads after second reboot");
  if (!loaded2) {
    std::filesystem::remove(path);
    return;
  }

  auto guest2 = bootstrap_virtualbox_guest(spec, *loaded2);
  check(guest2.has_value(), "bootstrap_virtualbox_guest succeeds after second reboot");
  if (!guest2) {
    std::filesystem::remove(path);
    return;
  }

  CanonStore recovered2(*guest2->storage.device);
  check(recovered2.rebuild_index() == 1, "rebuild_index remains stable after second reboot");
  auto block2 = recovered2.get(stored_ref);
  check(block2.has_value(), "stored CanonRef survives second guest reboot");
  if (block2) {
    check(block2->trytes == block.trytes, "stored block payload intact after second reboot");
  }

  std::filesystem::remove(path);
}

// ─── AC-D3c: Guest bootstrap recovery after header + payload corruption ────

static void test_virtualbox_guest_rebuild_after_corruption() {
  std::printf("\n[D3c] VirtualBox guest rebuild after header + payload corruption\n");

  const std::string path = "/tmp/ternos_test_vbox_guest_rebuild.blk";
  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  auto good_block = make_block(0x61);
  auto bad_block = make_block(0x72);
  t81::canonfs::CanonRef good_ref;
  t81::canonfs::CanonRef bad_ref;

  {
    HostedBlockDev backing(32, "vbox-guest-rebuild");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    check(guest.has_value(), "bootstrap_virtualbox_guest succeeds before corruption");
    if (!guest) {
      std::filesystem::remove(path);
      return;
    }

    CanonStore store(*guest->storage.device);
    auto ref1 = store.put(good_block);
    auto ref2 = store.put(bad_block);
    check(ref1.has_value(), "first guest block stores successfully");
    check(ref2.has_value(), "second guest block stores successfully");
    if (!ref1 || !ref2) {
      std::filesystem::remove(path);
      return;
    }
    good_ref = *ref1;
    bad_ref = *ref2;
    check(store.flush(), "guest storage flush succeeds before corruption");
  }

  auto damaged = HostedBlockDev::load(path);
  check(damaged.has_value(), "device reloads for corruption injection");
  if (!damaged) {
    std::filesystem::remove(path);
    return;
  }

  BlockData header{};
  check(damaged->read_block(0, header), "reads persisted CanonStore header");
  header[0] ^= 0xFF;
  check(damaged->write_block(0, header), "corrupts persisted CanonStore header");

  BlockData corrupted_payload{};
  check(damaged->read_block(2, corrupted_payload), "reads second persisted payload block");
  corrupted_payload[13] ^= 0x5A;
  check(damaged->write_block(2, corrupted_payload), "corrupts second persisted payload block");
  check(damaged->save(path), "saves corrupted guest backing file");

  auto loaded = HostedBlockDev::load(path);
  check(loaded.has_value(), "device reloads after corruption");
  if (!loaded) {
    std::filesystem::remove(path);
    return;
  }

  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  check(guest.has_value(), "bootstrap_virtualbox_guest succeeds after corruption");
  if (!guest) {
    std::filesystem::remove(path);
    return;
  }

  CanonStore rebuilt(*guest->storage.device);
  check(rebuilt.rebuild_index() == 2,
        "rebuild_index scans payload blocks when header magic is damaged");

  auto recovered_good = rebuilt.get(good_ref);
  check(recovered_good.has_value(), "intact CanonRef survives guest rebuild");
  if (recovered_good) {
    check(recovered_good->trytes == good_block.trytes,
          "intact payload remains exact after guest rebuild");
  }

  auto recovered_bad = rebuilt.get(bad_ref);
  check(!recovered_bad.has_value(), "corrupted CanonRef is rejected after guest rebuild");
  check(rebuilt.contains(good_ref), "rebuilt index still contains intact CanonRef");
  check(!rebuilt.contains(bad_ref), "rebuilt index no longer matches the pre-corruption CanonRef");
  check(rebuilt.size() == 2, "rebuilt index retains both physical payload blocks after scan");

  std::filesystem::remove(path);
}

// ─── AC-D3d: Guest bootstrap persistence at Phase 4 index cap ──────────────

static void test_virtualbox_guest_capacity_reboot() {
  std::printf("\n[D3d] VirtualBox guest persistence at Phase 4 index cap\n");

  const std::string path = "/tmp/ternos_test_vbox_guest_capacity.blk";
  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  std::array<t81::canonfs::CanonBlock, CanonStore::kMaxIndexEntries> blocks{};
  std::array<t81::canonfs::CanonRef, CanonStore::kMaxIndexEntries> refs{};

  {
    HostedBlockDev backing(48, "vbox-guest-capacity");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    check(guest.has_value(), "bootstrap_virtualbox_guest succeeds at capacity setup");
    if (!guest) {
      std::filesystem::remove(path);
      return;
    }

    CanonStore store(*guest->storage.device);
    for (std::size_t i = 0; i < CanonStore::kMaxIndexEntries; ++i) {
      blocks[i] = make_block(static_cast<uint8_t>(i + 1));
      auto ref = store.put(blocks[i]);
      check(ref.has_value(), "guest store accepts block within Phase 4 cap");
      if (!ref) {
        std::filesystem::remove(path);
        return;
      }
      refs[i] = *ref;
    }

    check(store.size() == CanonStore::kMaxIndexEntries,
          "guest store reaches Phase 4 index cap");
    check(store.flush(), "guest storage flush succeeds at capacity");
  }

  auto loaded = HostedBlockDev::load(path);
  check(loaded.has_value(), "device reloads after capacity reboot");
  if (!loaded) {
    std::filesystem::remove(path);
    return;
  }

  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  check(guest.has_value(), "bootstrap_virtualbox_guest succeeds after capacity reboot");
  if (!guest) {
    std::filesystem::remove(path);
    return;
  }

  CanonStore rebuilt(*guest->storage.device);
  check(rebuilt.rebuild_index() == CanonStore::kMaxIndexEntries,
        "rebuild_index recovers every capped entry");

  for (std::size_t i = 0; i < CanonStore::kMaxIndexEntries; ++i) {
    auto block = rebuilt.get(refs[i]);
    check(block.has_value(), "recovered capped CanonRef remains readable");
    if (block) {
      check(block->trytes == blocks[i].trytes,
            "recovered capped payload remains exact");
    }
  }

  std::filesystem::remove(path);
}

// ─── AC-D3e: Guest bootstrap recovery after torn index header ──────────────

static void test_virtualbox_guest_torn_header_recovery() {
  std::printf("\n[D3e] VirtualBox guest recovery after torn index header\n");

  const std::string path = "/tmp/ternos_test_vbox_guest_torn_header.blk";
  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  auto block_a = make_block(0x81);
  auto block_b = make_block(0x82);
  auto block_c = make_block(0x83);
  t81::canonfs::CanonRef ref_a, ref_b, ref_c;

  {
    HostedBlockDev backing(32, "vbox-guest-torn-header");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    check(guest.has_value(), "bootstrap_virtualbox_guest succeeds before torn-header injection");
    if (!guest) {
      std::filesystem::remove(path);
      return;
    }

    CanonStore store(*guest->storage.device);
    auto a = store.put(block_a);
    auto b = store.put(block_b);
    auto c = store.put(block_c);
    check(a.has_value(), "first block stores before torn-header injection");
    check(b.has_value(), "second block stores before torn-header injection");
    check(c.has_value(), "third block stores before torn-header injection");
    if (!a || !b || !c) {
      std::filesystem::remove(path);
      return;
    }
    ref_a = *a;
    ref_b = *b;
    ref_c = *c;
    check(store.flush(), "guest storage flush succeeds before torn-header injection");
  }

  auto damaged = HostedBlockDev::load(path);
  check(damaged.has_value(), "device reloads for torn-header injection");
  if (!damaged) {
    std::filesystem::remove(path);
    return;
  }

  BlockData header{};
  check(damaged->read_block(0, header), "reads persisted header for torn-header injection");
  uint32_t impossible_count = static_cast<uint32_t>(CanonStore::kMaxIndexEntries + 5);
  std::memcpy(header.data() + 4, &impossible_count, sizeof(impossible_count));
  std::memset(header.data() + 8 + 40, 0xEE, 16);
  check(damaged->write_block(0, header), "writes torn header with impossible entry count");
  check(damaged->save(path), "saves torn-header guest backing file");

  auto loaded = HostedBlockDev::load(path);
  check(loaded.has_value(), "device reloads after torn-header injection");
  if (!loaded) {
    std::filesystem::remove(path);
    return;
  }

  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  check(guest.has_value(), "bootstrap_virtualbox_guest succeeds after torn-header injection");
  if (!guest) {
    std::filesystem::remove(path);
    return;
  }

  CanonStore rebuilt(*guest->storage.device);
  check(rebuilt.rebuild_index() == 3,
        "rebuild_index falls back to payload scan after impossible entry count");

  auto recovered_a = rebuilt.get(ref_a);
  auto recovered_b = rebuilt.get(ref_b);
  auto recovered_c = rebuilt.get(ref_c);
  check(recovered_a.has_value(), "first CanonRef survives torn-header recovery");
  check(recovered_b.has_value(), "second CanonRef survives torn-header recovery");
  check(recovered_c.has_value(), "third CanonRef survives torn-header recovery");
  if (recovered_a) check(recovered_a->trytes == block_a.trytes, "first payload intact after torn-header recovery");
  if (recovered_b) check(recovered_b->trytes == block_b.trytes, "second payload intact after torn-header recovery");
  if (recovered_c) check(recovered_c->trytes == block_c.trytes, "third payload intact after torn-header recovery");

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

// ─── AC-D5b: TTF encode/decode and framebuffer text rendering ───────────────

static void test_ttf_rendering() {
  std::printf("\n[D5b] TTF encode/decode and text rendering\n");

  auto encoded = ttf_encode_ascii('A');
  check(encoded.has_value(), "encode_ascii('A') succeeds");
  if (encoded) {
    auto decoded = ttf_decode_ascii(*encoded);
    check(decoded.has_value(), "decode_ascii(encoded 'A') succeeds");
    if (decoded) {
      check(*decoded == 'A', "encode/decode round-trip preserves 'A'");
    }
  }

  auto bad = ttf_encode_ascii(static_cast<char>(0xFF));
  check(!bad.has_value(), "encode_ascii rejects non-ASCII byte");

  TtfGlyph invalid = {0, 0, 0, 0, 0, 2};
  check(!ttf_decode_ascii(invalid).has_value(),
        "decode_ascii rejects out-of-range trit");

  TernaryFramebuffer fb(12, 8);
  check(ttf_render_char(fb, 0, 0, 'A'), "render_char('A') succeeds");
  check(fb.get_pixel(0, 0).value != 0 || fb.get_pixel(1, 0).value != 0 ||
            fb.get_pixel(2, 0).value != 0 || fb.get_pixel(0, 1).value != 0 ||
            fb.get_pixel(1, 1).value != 0 || fb.get_pixel(2, 1).value != 0,
        "render_char writes non-zero glyph pixels");
  check(fb.get_pixel(0, 2).value == 0 && fb.get_pixel(1, 2).value == 0 &&
            fb.get_pixel(2, 2).value == 0,
        "render_char leaves third row blank");

  fb.clear();
  auto n = ttf_render_text(fb, 0, 0, "AB\nC");
  check(n == 3, "render_text renders 3 ASCII characters");
  check(fb.count(1) + fb.count(-1) > 0, "render_text produces visible pixels");
  check(fb.get_pixel(0, 4).value != 0 || fb.get_pixel(1, 4).value != 0 ||
            fb.get_pixel(2, 4).value != 0 || fb.get_pixel(0, 5).value != 0 ||
            fb.get_pixel(1, 5).value != 0 || fb.get_pixel(2, 5).value != 0,
        "render_text newline advances to the next glyph row");

  check(!ttf_render_char(fb, 10, 0, 'Z'),
        "render_char fails when full glyph cell would overflow");
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

    auto frame = pkt->to_frame();
    check(frame.has_value(), "to_frame() succeeds for valid packet");
    if (frame) {
      check(frame->size() == 16 + payload.size(), "frame size matches header + payload");
      auto parsed = TernaryEthernetPacket::from_frame(*frame);
      check(parsed.has_value(), "from_frame() parses serialized frame");
      if (parsed) {
        check(parsed->dst_mac == pkt->dst_mac, "parsed dst_mac matches");
        check(parsed->src_mac == pkt->src_mac, "parsed src_mac matches");
        check(parsed->ethertype == pkt->ethertype, "parsed ethertype matches");
        check(parsed->trit_payload == pkt->trit_payload, "parsed payload matches");
        check(parsed->content_ref.hash.h.bytes == pkt->content_ref.hash.h.bytes,
              "parsed content_ref matches");
      }
    }
  }

  // Invalid: payload size not multiple of 3
  std::vector<int8_t> bad_size = {1, 0};  // 2 trits
  auto bad_pkt = TernaryEthernetPacket::build(dst, src, 0x0081, bad_size);
  check(!bad_pkt.has_value(), "build() fails for payload size not multiple of 3");

  // Invalid: out-of-range trit value
  std::vector<int8_t> bad_val = {1, 2, -1};  // 2 is out of {-1,0,+1}
  auto bad_pkt2 = TernaryEthernetPacket::build(dst, src, 0x0081, bad_val);
  check(!bad_pkt2.has_value(), "build() fails for out-of-range trit value (2)");

  std::vector<uint8_t> bad_frame = {
      0x01,0x02,0x03,0x04,0x05,0x06,
      0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
      0x00,0x81,
      0x00,0x03,
      0x00,0x01,0x04};
  check(!TernaryEthernetPacket::from_frame(bad_frame).has_value(),
        "from_frame() rejects out-of-range encoded trit");

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
  test_virtualbox_ahci_adapter();
  test_virtualbox_e1000_adapter();
  test_virtualbox_vmsvga_adapter();
  test_canon_store_put();
  test_canon_store_get_unknown();
  test_canon_store_reboot();
  test_virtualbox_guest_reboot_persistence();
  test_virtualbox_guest_rebuild_after_corruption();
  test_virtualbox_guest_capacity_reboot();
  test_virtualbox_guest_torn_header_recovery();
  test_canon_store_corruption_detection();
  test_framebuffer();
  test_ttf_rendering();
  test_net_packet();
  test_canon_store_capacity();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
