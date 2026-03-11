// experimental/ternaryos/shell_session.cpp

#include "shell_session.hpp"

#include "dev/canon_store.hpp"
#include "dev/framebuffer.hpp"
#include "dev/hosted_block_dev.hpp"
#include "dev/ttf.hpp"
#include "hal/hal.hpp"
#include "hal/virtualbox_guest_devices.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <string_view>

#if !defined(_WIN32)
#  include <cstdio>
#  include <unistd.h>
#endif

using namespace t81::ternaryos::dev;
using namespace t81::ternaryos::hal;

namespace t81::ternaryos {

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

std::string join_lines(const std::vector<std::string>& lines) {
  std::string joined;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i != 0) joined.push_back('\n');
    joined += lines[i];
  }
  return joined;
}

}  // namespace

namespace {

#if !defined(_WIN32)
class StdoutSilencer {
public:
  explicit StdoutSilencer(bool enabled) : enabled_(enabled) {
    if (!enabled_) return;
    std::fflush(stdout);
    saved_fd_ = dup(STDOUT_FILENO);
    if (saved_fd_ < 0) {
      enabled_ = false;
      return;
    }
    std::freopen("/dev/null", "w", stdout);
  }

  ~StdoutSilencer() {
    if (!enabled_) return;
    std::fflush(stdout);
    dup2(saved_fd_, STDOUT_FILENO);
    close(saved_fd_);
  }

private:
  bool enabled_{false};
  int  saved_fd_{-1};
};
#else
class StdoutSilencer {
public:
  explicit StdoutSilencer(bool) {}
};
#endif

}  // namespace

std::vector<std::string> scripted_shell_lines() {
  return {kShellScreenLines.begin(), kShellScreenLines.end()};
}

std::optional<ShellSessionState> build_scripted_shell_session(bool quiet_boot) {
  const std::string path = "/tmp/ternos_shell_demo_store.blk";
  const auto lines = scripted_shell_lines();
  const std::string transcript = join_lines(lines);
  StdoutSilencer silencer(quiet_boot);

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  t81::canonfs::CanonRef history_ref;

  {
    HostedBlockDev backing(48, "shell-demo-ahci");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    if (!guest.has_value()) return std::nullopt;
    if (hal_main(guest->boot_context) != 0) return std::nullopt;

    CanonStore store(*guest->storage.device);
    auto ref = store.put(make_text_block(transcript));
    if (!ref.has_value()) return std::nullopt;
    history_ref = *ref;

    if (!store.flush()) return std::nullopt;
  }

  auto loaded = HostedBlockDev::load(path);
  if (!loaded.has_value()) return std::nullopt;

  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!guest.has_value()) return std::nullopt;
  if (hal_main(guest->boot_context) != 0) return std::nullopt;

  CanonStore recovered_store(*guest->storage.device);
  const auto recovered = recovered_store.rebuild_index();
  const auto history = recovered_store.get(history_ref);
  if (!history.has_value()) return std::nullopt;

  auto& fb = guest->display.device->framebuffer();
  fb.clear();
  const auto rendered = ttf_render_text(fb, 0, 0, transcript);
  if (!guest->display.device->present()) return std::nullopt;

  ShellSessionState state;
  state.profile_summary = guest->profile_summary;
  state.storage_binding_name = guest->storage.binding_name;
  state.display_binding_name = guest->display.binding_name;
  state.recovered_entries = recovered;
  state.rendered_glyphs = rendered;
  state.transcript_lines = lines;
  state.transcript_text = transcript;
  state.framebuffer_ascii = guest->display.device->last_present_ascii();

  std::filesystem::remove(path);
  return state;
}

}  // namespace t81::ternaryos
