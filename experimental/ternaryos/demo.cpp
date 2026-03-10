// experimental/ternaryos/demo.cpp

#include "dev/canon_store.hpp"
#include "dev/framebuffer.hpp"
#include "dev/hosted_block_dev.hpp"
#include "dev/net_packet.hpp"
#include "dev/ttf.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using namespace t81::ternaryos::dev;

namespace {

t81::canonfs::CanonBlock make_block(uint8_t fill) {
  t81::canonfs::CanonBlock b;
  b.trytes.fill(fill);
  return b;
}

void print_hash_prefix(const t81::canonfs::CanonRef& ref) {
  for (std::size_t i = 0; i < 6; ++i) {
    std::printf("%02x", ref.hash.h.bytes[i]);
  }
}

}  // namespace

int main() {
  std::puts("=== TernOS Phase 4 Hosted Demo ===");

  const std::string path = "/tmp/ternos_demo_store.blk";

  auto block = make_block(0x51);
  t81::canonfs::CanonRef stored_ref;

  {
    HostedBlockDev dev(32, "demo-nvme0");
    dev.set_backing_file(path);
    CanonStore store(dev);

    auto ref = store.put(block);
    if (!ref.has_value()) {
      std::fputs("store.put failed\n", stderr);
      return 1;
    }
    stored_ref = *ref;

    if (!store.flush()) {
      std::fputs("store.flush failed\n", stderr);
      return 1;
    }
  }

  auto loaded = HostedBlockDev::load(path);
  if (!loaded.has_value()) {
    std::fputs("HostedBlockDev::load failed\n", stderr);
    return 1;
  }

  CanonStore recovered_store(*loaded);
  const auto recovered = recovered_store.rebuild_index();
  auto recovered_block = recovered_store.get(stored_ref);
  std::printf("CanonStore: recovered %zu entry(ies), block lookup %s, ref=",
              recovered,
              recovered_block.has_value() ? "ok" : "failed");
  print_hash_prefix(stored_ref);
  std::puts("");

  TernaryFramebuffer fb(20, 8);
  const auto chars = ttf_render_text(fb, 0, 0, "T81\nOS");
  std::printf("TTF: rendered %zu glyphs\n", chars);
  std::puts("Framebuffer:");
  std::puts(fb.dump_ascii().c_str());

  auto packet = TernaryEthernetPacket::build(
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
      {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
      0x0081,
      {1, 0, -1, -1, 1, 0});
  if (!packet.has_value()) {
    std::fputs("packet build failed\n", stderr);
    return 1;
  }

  auto frame = packet->to_frame();
  auto parsed = frame ? TernaryEthernetPacket::from_frame(*frame) : std::nullopt;
  std::printf("Ethernet: serialized %zu-byte frame, parse %s, words=%zu\n",
              frame ? frame->size() : 0,
              parsed.has_value() ? "ok" : "failed",
              parsed ? parsed->trit_word_count() : 0);

  std::filesystem::remove(path);
  return parsed.has_value() && recovered_block.has_value() ? 0 : 1;
}
