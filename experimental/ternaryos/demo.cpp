// experimental/ternaryos/demo.cpp

#include "dev/canon_store.hpp"
#include "dev/framebuffer.hpp"
#include "dev/hosted_block_dev.hpp"
#include "dev/net_packet.hpp"
#include "dev/ttf.hpp"
#include "hal/hal.hpp"
#include "hal/virtualbox_guest_devices.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using namespace t81::ternaryos::dev;
using namespace t81::ternaryos::hal;

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
  std::puts("=== TernOS Phase 4 Hosted + VirtualBox Demo ===");

  const std::string path = "/tmp/ternos_demo_store.blk";

  auto block = make_block(0x51);
  t81::canonfs::CanonRef stored_ref;
  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  {
    HostedBlockDev backing(32, "demo-ahci-backing");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    if (!guest.has_value()) {
      std::fputs("bootstrap_virtualbox_guest failed\n", stderr);
      return 1;
    }
    if (hal_main(guest->boot_context) != 0) {
      std::fputs("hal_main failed for VirtualBox bootstrap\n", stderr);
      return 1;
    }

    std::printf("VirtualBox: profile=%s devices=%zu storage=%s network=%s\n",
                guest->profile_summary.c_str(),
                guest->device_map.size(),
                guest->storage.binding_name.c_str(),
                guest->network.binding_name.c_str());

    CanonStore store(*guest->storage.device);

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

  auto rebooted_guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!rebooted_guest.has_value()) {
    std::fputs("bootstrap_virtualbox_guest failed after reboot\n", stderr);
    return 1;
  }
  if (hal_main(rebooted_guest->boot_context) != 0) {
    std::fputs("hal_main failed for rebooted VirtualBox bootstrap\n", stderr);
    return 1;
  }

  CanonStore recovered_store(*rebooted_guest->storage.device);
  const auto recovered = recovered_store.rebuild_index();
  auto recovered_block = recovered_store.get(stored_ref);
  std::printf("CanonStore: recovered %zu entry(ies) through %s, block lookup %s, ref=",
              recovered,
              rebooted_guest->storage.binding_name.c_str(),
              recovered_block.has_value() ? "ok" : "failed");
  print_hash_prefix(stored_ref);
  std::puts("");

  auto display_guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!display_guest.has_value()) {
    std::fputs("bootstrap_virtualbox_guest failed for display path\n", stderr);
    return 1;
  }
  auto& fb = display_guest->display.device->framebuffer();
  fb = TernaryFramebuffer(20, 8);
  const auto chars = ttf_render_text(fb, 0, 0, "T81\nOS");
  if (!display_guest->display.device->present()) {
    std::fputs("display present failed\n", stderr);
    return 1;
  }
  std::printf("TTF: rendered %zu glyphs through %s\n",
              chars,
              display_guest->display.binding_name.c_str());
  std::puts("Framebuffer:");
  std::puts(display_guest->display.device->last_present_ascii().c_str());

  auto packet = TernaryEthernetPacket::build(
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
      {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
      0x0081,
      {1, 0, -1, -1, 1, 0});
  if (!packet.has_value()) {
    std::fputs("packet build failed\n", stderr);
    return 1;
  }

  auto net_guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!net_guest.has_value()) {
    std::fputs("bootstrap_virtualbox_guest failed for network path\n", stderr);
    return 1;
  }
  auto frame = net_guest->network.device->send_packet(*packet);
  if (!frame.has_value()) {
    std::fputs("send_packet failed\n", stderr);
    return 1;
  }
  if (!net_guest->network.device->inject_frame(*frame)) {
    std::fputs("inject_frame failed\n", stderr);
    return 1;
  }
  auto parsed = net_guest->network.device->receive_packet();
  std::printf("Ethernet: %s serialized %zu-byte frame, parse %s, words=%zu\n",
              net_guest->network.binding_name.c_str(),
              frame->size(),
              parsed.has_value() ? "ok" : "failed",
              parsed ? parsed->trit_word_count() : 0);

  std::filesystem::remove(path);
  return parsed.has_value() && recovered_block.has_value() ? 0 : 1;
}
