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
#include <chrono>
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
    "store put <text>",
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

std::string unique_store_path() {
  const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
  return "/tmp/ternos_shell_session_" + std::to_string(ticks) + ".blk";
}

std::optional<HostedBlockDev> load_backed_device(const std::string& path) {
  auto loaded = HostedBlockDev::load(path);
  if (!loaded.has_value()) return std::nullopt;
  loaded->set_backing_file(path);
  return loaded;
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
  return {
      "help",
      "profile",
      "store put phase5 durable transcript",
      "history",
  };
}

ShellSession::ShellSession(bool quiet_boot) : quiet_boot_(quiet_boot), store_path_(unique_store_path()) {
  state_.available_commands = {kBuiltinCommands.begin(), kBuiltinCommands.end()};
}

bool ShellSession::initialize() {
  HostedBlockDev backing(48, "shell-demo-ahci");
  backing.set_backing_file(store_path_);

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;

  StdoutSilencer silencer(quiet_boot_);
  auto guest = bootstrap_virtualbox_guest(spec, backing);
  if (!guest.has_value()) return false;
  if (hal_main(guest->boot_context) != 0) return false;

  state_.profile_summary = guest->profile_summary;
  state_.storage_binding_name = guest->storage.binding_name;
  state_.display_binding_name = guest->display.binding_name;
  if (!backing.flush()) return false;
  return refresh_render();
}

bool ShellSession::refresh_render() {
  auto loaded = load_backed_device(store_path_);
  if (!loaded.has_value()) return false;

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;
  StdoutSilencer silencer(quiet_boot_);
  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!guest.has_value()) return false;
  if (hal_main(guest->boot_context) != 0) return false;

  auto lines = render_transcript_lines([&] {
    std::vector<ShellStep> steps;
    for (const auto& record : state_.command_records) {
      steps.push_back({record.command, record.result});
    }
    return steps;
  }());
  state_.transcript_lines = std::move(lines);
  state_.transcript_text = join_lines(state_.transcript_lines);

  auto& fb = guest->display.device->framebuffer();
  fb.clear();
  state_.rendered_glyphs = ttf_render_text(fb, 0, 0, state_.transcript_text);
  if (!guest->display.device->present()) return false;
  state_.framebuffer_ascii = guest->display.device->last_present_ascii();
  return true;
}

std::optional<ShellSession> ShellSession::create(bool quiet_boot) {
  ShellSession session(quiet_boot);
  if (!session.initialize()) return std::nullopt;
  return std::optional<ShellSession>(std::move(session));
}

bool ShellSession::execute_command(std::string_view command_view) {
  const std::string command(command_view);
  const auto words = split_words(command);
  if (words.empty()) return refresh_render();

  if (words[0] == "help") {
    state_.command_records.push_back({command, "builtins help profile store put <text> history"});
    return refresh_render();
  }

  if (words[0] == "profile") {
    state_.command_records.push_back({command, state_.profile_summary});
    return refresh_render();
  }

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024 * 1024;
  auto loaded = load_backed_device(store_path_);
  if (!loaded.has_value()) return false;

  StdoutSilencer silencer(quiet_boot_);
  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!guest.has_value()) return false;
  if (hal_main(guest->boot_context) != 0) return false;

  CanonStore store(*guest->storage.device);

  if (words.size() >= 3 && words[0] == "store" && words[1] == "put") {
    const auto payload = command.substr(std::string("store put ").size());
    auto ref = store.put(make_text_block(payload));
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "canon store put failed"});
      return refresh_render();
    }
    history_ref_ = *ref;
    if (!store.flush()) {
      state_.command_records.push_back({command, "canon flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back({command, "canon durable ok"});
    return refresh_render();
  }

  if (words[0] == "history") {
    std::string result = "reboot recovered 0";
    if (history_ref_.has_value()) {
      const auto recovered = store.rebuild_index();
      const auto history = store.get(*history_ref_);
      if (history.has_value()) {
        result = "reboot recovered " + std::to_string(recovered);
      } else {
        result = "reboot history missing";
      }
    }
    state_.recovered_entries = store.rebuild_index();
    state_.command_records.push_back({command, result});
    return refresh_render();
  }

  state_.command_records.push_back({command, "unknown command"});
  return refresh_render();
}

std::optional<ShellSessionState> build_scripted_shell_session(bool quiet_boot) {
  auto session = ShellSession::create(quiet_boot);
  if (!session.has_value()) return std::nullopt;

  const auto command_sequence = default_shell_command_sequence();
  for (const auto& command : command_sequence) {
    if (!session->execute_command(command)) return std::nullopt;
  }
  return session->state();
}

}  // namespace t81::ternaryos
