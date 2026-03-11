// experimental/ternaryos/shell_demo.cpp

#include "dev/canon_store.hpp"
#include "dev/framebuffer.hpp"
#include "dev/hosted_block_dev.hpp"
#include "dev/ttf.hpp"
#include "hal/hal.hpp"
#include "hal/virtualbox_guest_devices.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>

using namespace t81::ternaryos::dev;
using namespace t81::ternaryos::hal;

namespace {

constexpr std::array<const char*, 7> kShellScreenLines = {
    "TSH PHASE5 READY",
    "tsh> profile",
    "VBOX EFI AHCI E1K",
    "tsh> store put",
    "CANON DURABLE OK",
    "tsh> history",
    "REBOOT RECOVERED 1",
};

t81::canonfs::CanonBlock make_text_block(std::string_view text) {
  t81::canonfs::CanonBlock block;
  block.trytes.fill(0);
  const auto count = std::min(block.trytes.size(), text.size());
  std::memcpy(block.trytes.data(), text.data(), count);
  return block;
}

std::string join_lines() {
  std::string joined;
  for (std::size_t i = 0; i < kShellScreenLines.size(); ++i) {
    if (i != 0) joined.push_back('\n');
    joined += kShellScreenLines[i];
  }
  return joined;
}

}  // namespace

int main() {
  std::puts("=== TernOS Phase 5 Shell Demo ===");

  const std::string path = "/tmp/ternos_shell_demo_store.blk";
  const std::string transcript = join_lines();

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  t81::canonfs::CanonRef history_ref;

  {
    HostedBlockDev backing(48, "shell-demo-ahci");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    if (!guest.has_value()) {
      std::fputs("bootstrap_virtualbox_guest failed\n", stderr);
      return 1;
    }
    if (hal_main(guest->boot_context) != 0) {
      std::fputs("hal_main failed for shell bootstrap\n", stderr);
      return 1;
    }

    CanonStore store(*guest->storage.device);
    auto ref = store.put(make_text_block(transcript));
    if (!ref.has_value()) {
      std::fputs("store.put failed for shell transcript\n", stderr);
      return 1;
    }
    history_ref = *ref;

    if (!store.flush()) {
      std::fputs("store.flush failed for shell transcript\n", stderr);
      return 1;
    }

    std::printf("Shell: stored transcript through %s\n",
                guest->storage.binding_name.c_str());
  }

  auto loaded = HostedBlockDev::load(path);
  if (!loaded.has_value()) {
    std::fputs("HostedBlockDev::load failed for shell demo\n", stderr);
    return 1;
  }

  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!guest.has_value()) {
    std::fputs("bootstrap_virtualbox_guest failed after reboot\n", stderr);
    return 1;
  }
  if (hal_main(guest->boot_context) != 0) {
    std::fputs("hal_main failed after reboot\n", stderr);
    return 1;
  }

  CanonStore recovered_store(*guest->storage.device);
  const auto recovered = recovered_store.rebuild_index();
  const auto history = recovered_store.get(history_ref);
  if (!history.has_value()) {
    std::fputs("recovered shell transcript missing after reboot\n", stderr);
    return 1;
  }

  auto& fb = guest->display.device->framebuffer();
  fb.clear();
  const auto rendered = ttf_render_text(fb, 0, 0, transcript);
  if (!guest->display.device->present()) {
    std::fputs("display present failed for shell demo\n", stderr);
    return 1;
  }

  std::printf("Shell: recovered %zu entry(ies), rendered %zu glyphs through %s\n",
              recovered,
              rendered,
              guest->display.binding_name.c_str());
  std::puts("Shell framebuffer:");
  std::puts(guest->display.device->last_present_ascii().c_str());

  std::filesystem::remove(path);
  return 0;
}
