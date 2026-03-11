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
#include <cctype>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string_view>

#if !defined(_WIN32)
#  include <cstdio>
#  include <unistd.h>
#endif

using namespace t81::ternaryos::dev;
using namespace t81::ternaryos::hal;

namespace t81::ternaryos {

namespace {

constexpr std::array<const char*, 4> kBuiltinCommands = {
    "help",
    "profile",
    "store put",
    "history",
};

struct ShellStep {
  std::string command;
  std::string result;
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

std::vector<std::string> split_words(const std::string& command) {
  std::istringstream stream(command);
  std::vector<std::string> out;
  std::string word;
  while (stream >> word) out.push_back(word);
  return out;
}

std::string upper_ascii(std::string text) {
  for (char& ch : text) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return text;
}

std::vector<std::string> render_transcript_lines(const std::vector<ShellStep>& steps) {
  std::vector<std::string> lines;
  lines.push_back("TSH PHASE5 READY");
  for (const auto& step : steps) {
    lines.push_back("tsh> " + step.command);
    lines.push_back(upper_ascii(step.result));
  }
  return lines;
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

std::vector<std::string> default_shell_command_sequence() {
  return {kBuiltinCommands.begin(), kBuiltinCommands.end()};
}

std::optional<ShellSessionState> build_scripted_shell_session(bool quiet_boot) {
  const std::string path = "/tmp/ternos_shell_demo_store.blk";
  StdoutSilencer silencer(quiet_boot);

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  t81::canonfs::CanonRef history_ref;
  std::vector<ShellStep> steps;
  const auto command_sequence = default_shell_command_sequence();
  const std::vector<std::string> available_commands = {
      kBuiltinCommands.begin(), kBuiltinCommands.end()};

  {
    HostedBlockDev backing(48, "shell-demo-ahci");
    backing.set_backing_file(path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    if (!guest.has_value()) return std::nullopt;
    if (hal_main(guest->boot_context) != 0) return std::nullopt;

    CanonStore store(*guest->storage.device);
    for (const auto& command : command_sequence) {
      const auto words = split_words(command);
      if (words.empty()) continue;

      if (words[0] == "help") {
        steps.push_back({command, "builtins help profile store put history"});
        continue;
      }

      if (words[0] == "profile") {
        steps.push_back({command, guest->profile_summary});
        continue;
      }

      if (words.size() == 2 && words[0] == "store" && words[1] == "put") {
        const auto persisted_lines = render_transcript_lines(steps);
        const auto persisted_text = join_lines(persisted_lines);
        auto ref = store.put(make_text_block(persisted_text));
        if (!ref.has_value()) return std::nullopt;
        history_ref = *ref;
        if (!store.flush()) return std::nullopt;
        steps.push_back({command, "canon durable ok"});
        continue;
      }
    }
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

  for (const auto& command : command_sequence) {
    const auto words = split_words(command);
    if (words.size() == 1 && words[0] == "history") {
      steps.push_back({command, "reboot recovered " + std::to_string(recovered)});
    }
  }

  const auto lines = render_transcript_lines(steps);
  const std::string transcript = join_lines(lines);

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
  state.available_commands = available_commands;
  for (const auto& step : steps) {
    state.command_records.push_back({step.command, step.result});
  }
  state.transcript_lines = lines;
  state.transcript_text = transcript;
  state.framebuffer_ascii = guest->display.device->last_present_ascii();

  std::filesystem::remove(path);
  return state;
}

}  // namespace t81::ternaryos
