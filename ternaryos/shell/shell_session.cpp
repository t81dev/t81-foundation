// experimental/ternaryos/shell/shell_session.cpp

#include "shell_session.hpp"

#include "command_catalog.hpp"
#include "shared_command_core.hpp"

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

struct ShellStep {
  std::string command;
  std::string result;
};

struct ParsedCommand {
  std::vector<std::string> words;
  std::optional<std::string> error;
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

std::vector<std::string> split_lines(std::string_view text) {
  std::vector<std::string> lines;
  std::string current;
  for (char ch : text) {
    if (ch == '\n') {
      lines.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty() || text.empty()) lines.push_back(current);
  return lines;
}

std::string normalize_script_payload(std::string_view payload) {
  std::string text(payload);
  for (char& ch : text) {
    if (ch == '|') ch = '\n';
  }
  return text;
}

std::string diff_transcript_text(std::string_view current, std::string_view durable) {
  const auto current_lines = split_lines(current);
  const auto durable_lines = split_lines(durable);
  if (current_lines == durable_lines) return "session diff equal";

  const std::size_t shared = std::min(current_lines.size(), durable_lines.size());
  std::size_t diff_index = 0;
  while (diff_index < shared && current_lines[diff_index] == durable_lines[diff_index]) {
    ++diff_index;
  }

  std::ostringstream out;
  out << "session diff mismatch" << '\n'
      << "current lines " << current_lines.size() << '\n'
      << "durable lines " << durable_lines.size() << '\n'
      << "first diff line " << (diff_index + 1);
  if (diff_index < current_lines.size()) {
    out << '\n' << "current> " << current_lines[diff_index];
  } else {
    out << '\n' << "current> <end>";
  }
  if (diff_index < durable_lines.size()) {
    out << '\n' << "durable> " << durable_lines[diff_index];
  } else {
    out << '\n' << "durable> <end>";
  }
  return out.str();
}

bool is_script_block_text(std::string_view text) {
  return text.starts_with("AXION_SCRIPT\n");
}

std::vector<std::string> script_lines(std::string_view text) {
  if (!is_script_block_text(text)) return {};
  return split_lines(text.substr(std::string_view("AXION_SCRIPT\n").size()));
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

ParsedCommand parse_shell_command(const std::string& command) {
  ParsedCommand parsed;
  std::string current;
  bool in_quotes = false;

  for (char ch : command) {
    if (ch == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (!in_quotes && std::isspace(static_cast<unsigned char>(ch))) {
      if (!current.empty()) {
        parsed.words.push_back(current);
        current.clear();
      }
      continue;
    }
    current.push_back(ch);
  }

  if (in_quotes) {
    parsed.error = "parse error: unmatched quote";
    return parsed;
  }
  if (!current.empty()) parsed.words.push_back(current);
  return parsed;
}

std::string upper_ascii(std::string text) {
  for (char& ch : text) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return text;
}

std::string canon_ref_text(const t81::canonfs::CanonRef& ref) {
  return ref.hash.h.to_string();
}

std::optional<t81::canonfs::CanonRef> parse_canon_ref_text(std::string_view text) {
  try {
    return t81::canonfs::CanonRef{
        t81::canonfs::CanonHash{t81::hash::CanonHash81::from_string(text)}};
  } catch (...) {
    return std::nullopt;
  }
}

std::string decode_text_block(const t81::canonfs::CanonBlock& block) {
  std::string text;
  for (auto tryte : block.trytes) {
    if (tryte == 0) break;
    text.push_back(static_cast<char>(tryte));
  }
  return text.empty() ? "<empty>" : text;
}

bool canon_ref_known(const std::vector<t81::canonfs::CanonRef>& refs,
                     const t81::canonfs::CanonRef& ref) {
  for (const auto& candidate : refs) {
    if (candidate.hash == ref.hash) return true;
  }
  return false;
}

bool valid_name_label(std::string_view label) {
  if (label.empty()) return false;
  for (char ch : label) {
    if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_') continue;
    return false;
  }
  return true;
}

bool valid_object_kind(std::string_view kind) {
  return kind == "payload" || kind == "script" || kind == "transcript" || kind == "history";
}

std::optional<t81::canonfs::CanonRef> resolve_named_or_raw_ref(
    std::string_view text,
    const std::vector<ShellNamedRef>& named_refs) {
  if (text.starts_with('@')) {
    const auto label = text.substr(1);
    for (const auto& named : named_refs) {
      if (named.label == label) return named.ref;
    }
    return std::nullopt;
  }
  return parse_canon_ref_text(text);
}

void upsert_named_ref(std::vector<ShellNamedRef>& named_refs,
                      std::string_view label,
                      const t81::canonfs::CanonRef& ref) {
  auto existing =
      std::find_if(named_refs.begin(), named_refs.end(), [&](const auto& named) {
        return named.label == label;
      });
  if (existing == named_refs.end()) {
    named_refs.push_back({std::string(label), ref});
  } else {
    existing->ref = ref;
  }
}

std::vector<std::string> render_transcript_lines(const std::vector<ShellStep>& steps,
                                                 const std::vector<std::string>& imported_lines) {
  if (!imported_lines.empty()) {
    std::vector<std::string> lines = imported_lines;
    lines.push_back("IMPORTED SESSION ACTIVE");
    for (const auto& step : steps) {
      lines.push_back("tsh> " + step.command);
      lines.push_back(upper_ascii(step.result));
    }
    return lines;
  }

  std::vector<std::string> lines;
  lines.push_back("TSH PHASE5 READY");
  lines.push_back("SESSION TRANSCRIPT");
  for (const auto& step : steps) {
    lines.push_back("tsh> " + step.command);
    lines.push_back(upper_ascii(step.result));
  }
  return lines;
}

std::vector<std::string> hosted_command_names() {
  std::vector<std::string> names;
  names.reserve(kShellCommandCatalogCount);
  for (const auto& spec : kShellCommandCatalog) {
    if (spec.hosted_phase5) names.emplace_back(spec.name);
  }
  return names;
}

std::string hosted_help_text() {
  std::string text = "builtins";
  for (const auto& spec : kShellCommandCatalog) {
    if (!shell_command_visible(spec, ShellSurface::HostedPhase5)) continue;
    text += "\n";
    text += spec.name;
    text += " -- ";
    text += spec.summary;
  }
  return text;
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
      "session status",
      "store put \"phase5 durable transcript\"",
      "store ls",
      "history",
  };
}

ShellSession::ShellSession(bool quiet_boot) : quiet_boot_(quiet_boot), store_path_(unique_store_path()) {
  state_.available_commands = hosted_command_names();
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

  CanonStore store(*guest->storage.device);
  state_.recovered_entries = store.rebuild_index();
  state_.session_command_count = state_.command_records.size();
  state_.durable_ref_count = stored_refs_.size();
  state_.named_ref_count = named_refs_.size();
  state_.named_object_count = named_objects_.size();
  state_.durable_anchor_present =
      history_ref_.has_value() && store.contains(*history_ref_);
  state_.named_refs = named_refs_;
  state_.named_objects = named_objects_;

  auto lines = render_transcript_lines([&] {
    std::vector<ShellStep> steps;
    for (const auto& record : state_.command_records) {
      steps.push_back({record.command, record.result});
    }
    return steps;
  }(), imported_transcript_lines_);
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
  const auto parsed = parse_shell_command(command);
  if (parsed.error.has_value()) {
    state_.command_records.push_back({command, *parsed.error});
    return refresh_render();
  }
  const auto& words = parsed.words;
  if (words.empty()) return refresh_render();

  if (words[0] == "help") {
    state_.command_records.push_back({command, hosted_help_text()});
    return refresh_render();
  }

  if (words[0] == "uname") {
    state_.command_records.push_back(
        {command, shell_uname_text(ShellSurface::HostedPhase5)});
    return refresh_render();
  }

  if (words[0] == "version") {
    state_.command_records.push_back(
        {command, shell_version_text(ShellSurface::HostedPhase5)});
    return refresh_render();
  }

  if (words[0] == "profile") {
    state_.command_records.push_back({command, state_.profile_summary});
    return refresh_render();
  }

  if (words.size() == 4 && words[0] == "name" && words[1] == "set") {
    if (!valid_name_label(words[2])) {
      state_.command_records.push_back({command, "name set invalid label"});
      return refresh_render();
    }
    const auto ref = resolve_named_or_raw_ref(words[3], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "name set invalid ref"});
      return refresh_render();
    }

    upsert_named_ref(named_refs_, words[2], *ref);
    state_.command_records.push_back(
        {command, "name set ok " + words[2] + " " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "name" && words[1] == "ls") {
    if (named_refs_.empty()) {
      state_.command_records.push_back({command, "name refs 0"});
      return refresh_render();
    }

    std::string result = "name refs " + std::to_string(named_refs_.size());
    for (const auto& named : named_refs_) {
      result += "\n" + named.label + " " + canon_ref_text(named.ref);
    }
    state_.command_records.push_back({command, result});
    return refresh_render();
  }

  if (words.size() == 5 && words[0] == "object" && words[1] == "pin") {
    if (!valid_object_kind(words[2])) {
      state_.command_records.push_back({command, "object pin invalid kind"});
      return refresh_render();
    }
    if (!valid_name_label(words[3])) {
      state_.command_records.push_back({command, "object pin invalid name"});
      return refresh_render();
    }
    const auto ref = resolve_named_or_raw_ref(words[4], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "object pin invalid ref"});
      return refresh_render();
    }

    auto existing = std::find_if(named_objects_.begin(),
                                 named_objects_.end(),
                                 [&](const auto& object) { return object.name == words[3]; });
    if (existing == named_objects_.end()) {
      named_objects_.push_back({words[2], words[3], *ref});
    } else {
      existing->kind = words[2];
      existing->ref = *ref;
    }
    upsert_named_ref(named_refs_, words[3], *ref);
    state_.command_records.push_back(
        {command,
         "object pin ok " + words[2] + " " + words[3] + " " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "object" && words[1] == "ls") {
    if (named_objects_.empty()) {
      state_.command_records.push_back({command, "object refs 0"});
      return refresh_render();
    }

    std::string result = "object refs " + std::to_string(named_objects_.size());
    for (const auto& object : named_objects_) {
      result += "\n" + object.kind + " " + object.name + " " + canon_ref_text(object.ref);
    }
    state_.command_records.push_back({command, result});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "object" && words[1] == "show") {
    auto object = std::find_if(named_objects_.begin(),
                               named_objects_.end(),
                               [&](const auto& candidate) { return candidate.name == words[2]; });
    if (object == named_objects_.end()) {
      state_.command_records.push_back({command, "object show missing"});
      return refresh_render();
    }

    state_.command_records.push_back(
        {command,
         "object show " + object->name + "\nkind " + object->kind + "\nref " +
             canon_ref_text(object->ref)});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "show" && words[1] == "profile") {
    state_.command_records.push_back({command, "show profile\n" + state_.profile_summary});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "session" && words[1] == "status") {
    std::ostringstream out;
    out << "session profile " << state_.profile_summary << '\n'
        << "storage " << state_.storage_binding_name << '\n'
        << "display " << state_.display_binding_name << '\n'
        << "commands " << state_.command_records.size() << '\n'
        << "durable refs " << stored_refs_.size() << '\n'
        << "durable anchor " << (history_ref_.has_value() ? "tracked" : "none") << '\n'
        << "recovered " << state_.recovered_entries << '\n'
        << "glyphs " << state_.rendered_glyphs;
    state_.command_records.push_back({command, out.str()});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "session" && words[1] == "show" &&
      words[2] == "durable") {
    std::ostringstream out;
    out << "session durable" << '\n'
        << "refs " << state_.durable_ref_count << '\n'
        << "anchor " << (state_.durable_anchor_present ? "present" : "missing");
    if (history_ref_.has_value()) {
      out << '\n' << canon_ref_text(*history_ref_);
    }
    state_.command_records.push_back({command, out.str()});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "show" && words[1] == "session") {
    std::ostringstream out;
    out << "show session" << '\n'
        << "profile " << state_.profile_summary << '\n'
        << "storage " << state_.storage_binding_name << '\n'
        << "display " << state_.display_binding_name << '\n'
        << "session commands " << state_.session_command_count << '\n'
        << "durable refs " << state_.durable_ref_count << '\n'
        << "durable anchor " << (state_.durable_anchor_present ? "present" : "missing") << '\n'
        << "recovered " << state_.recovered_entries;
    state_.command_records.push_back({command, out.str()});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "session" && words[1] == "refs") {
    if (stored_refs_.empty()) {
      state_.command_records.push_back({command, "session refs 0"});
      return refresh_render();
    }

    std::string result = "session refs " + std::to_string(stored_refs_.size());
    for (const auto& ref : stored_refs_) {
      result += "\n" + canon_ref_text(ref);
    }
    state_.command_records.push_back({command, result});
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

  if (words.size() == 2 && words[0] == "session" && words[1] == "checkpoint") {
    state_.recovered_entries = store.rebuild_index();
    auto ref = store.put(make_text_block(state_.transcript_text));
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "session checkpoint failed"});
      return refresh_render();
    }
    if (!canon_ref_known(stored_refs_, *ref)) stored_refs_.push_back(*ref);
    if (!store.flush()) {
      state_.command_records.push_back({command, "session checkpoint flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back(
        {command, "session checkpoint ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "session" && words[1] == "export") {
    state_.recovered_entries = store.rebuild_index();
    auto ref = store.put(make_text_block(state_.transcript_text));
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "session export failed"});
      return refresh_render();
    }
    history_ref_ = *ref;
    if (!canon_ref_known(stored_refs_, *ref)) stored_refs_.push_back(*ref);
    if (!store.flush()) {
      state_.command_records.push_back({command, "session export flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back({command, "session export ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "session" && words[1] == "import") {
    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "session import invalid ref"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    const auto block = store.get(*ref);
    if (!block.has_value()) {
      state_.command_records.push_back({command, "session import missing"});
      return refresh_render();
    }

    imported_transcript_lines_ = split_lines(decode_text_block(*block));
    state_.command_records.push_back({command, "session import ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "session" && words[1] == "diff") {
    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "session diff invalid ref"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    const auto block = store.get(*ref);
    if (!block.has_value()) {
      state_.command_records.push_back({command, "session diff missing"});
      return refresh_render();
    }

    state_.command_records.push_back(
        {command, diff_transcript_text(state_.transcript_text, decode_text_block(*block))});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "session" && words[1] == "run") {
    if (script_run_active_) {
      state_.command_records.push_back({command, "session run nested unsupported"});
      return refresh_render();
    }

    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "session run invalid ref"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    const auto block = store.get(*ref);
    if (!block.has_value()) {
      state_.command_records.push_back({command, "session run missing"});
      return refresh_render();
    }

    const auto text = decode_text_block(*block);
    if (!is_script_block_text(text)) {
      state_.command_records.push_back({command, "session run not-script"});
      return refresh_render();
    }

    const auto lines = script_lines(text);
    script_run_active_ = true;
    std::size_t executed = 0;
    for (const auto& line : lines) {
      if (line.empty()) continue;
      if (line.starts_with("session run ")) {
        script_run_active_ = false;
        state_.command_records.push_back({command, "session run nested unsupported"});
        return refresh_render();
      }
      if (!execute_command(line)) {
        script_run_active_ = false;
        state_.command_records.push_back({command, "session run failed"});
        return refresh_render();
      }
      ++executed;
    }
    script_run_active_ = false;
    state_.command_records.push_back(
        {command, "session run ok " + canon_ref_text(*ref) + " lines " + std::to_string(executed)});
    return refresh_render();
  }

  if (words.size() == 4 && words[0] == "store" && words[1] == "put" && words[2] == "ref") {
    state_.recovered_entries = store.rebuild_index();
    const auto ref = resolve_named_or_raw_ref(words[3], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "store put ref invalid ref"});
      return refresh_render();
    }

    const auto source = store.get(*ref);
    if (!source.has_value()) {
      state_.command_records.push_back({command, "store put ref missing"});
      return refresh_render();
    }

    auto copied_ref = store.put(*source);
    if (!copied_ref.has_value()) {
      state_.command_records.push_back({command, "store put ref failed"});
      return refresh_render();
    }
    history_ref_ = *copied_ref;
    if (!canon_ref_known(stored_refs_, *copied_ref)) stored_refs_.push_back(*copied_ref);
    if (!store.flush()) {
      state_.command_records.push_back({command, "store put ref flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back(
        {command, "canon durable ref ok " + canon_ref_text(*copied_ref)});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "store" && words[1] == "cp") {
    state_.recovered_entries = store.rebuild_index();
    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "store cp invalid ref"});
      return refresh_render();
    }

    const auto source = store.get(*ref);
    if (!source.has_value()) {
      state_.command_records.push_back({command, "store cp missing"});
      return refresh_render();
    }

    auto copied_ref = store.put(*source);
    if (!copied_ref.has_value()) {
      state_.command_records.push_back({command, "store cp failed"});
      return refresh_render();
    }
    if (!canon_ref_known(stored_refs_, *copied_ref)) stored_refs_.push_back(*copied_ref);
    if (!store.flush()) {
      state_.command_records.push_back({command, "store cp flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back({command, "store cp ok " + canon_ref_text(*copied_ref)});
    return refresh_render();
  }

  if (words.size() >= 4 && words[0] == "store" && words[1] == "put" && words[2] == "script") {
    state_.recovered_entries = store.rebuild_index();
    std::string payload = words[3];
    for (std::size_t i = 4; i < words.size(); ++i) {
      payload += " " + words[i];
    }
    payload = "AXION_SCRIPT\n" + normalize_script_payload(payload);
    auto ref = store.put(make_text_block(payload));
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "canon script put failed"});
      return refresh_render();
    }
    if (!canon_ref_known(stored_refs_, *ref)) stored_refs_.push_back(*ref);
    if (!store.flush()) {
      state_.command_records.push_back({command, "canon script flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back({command, "canon script ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() >= 3 && words[0] == "store" && words[1] == "put") {
    state_.recovered_entries = store.rebuild_index();
    std::string payload = words[2];
    for (std::size_t i = 3; i < words.size(); ++i) {
      payload += " " + words[i];
    }
    auto ref = store.put(make_text_block(payload));
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "canon store put failed"});
      return refresh_render();
    }
    history_ref_ = *ref;
    if (!canon_ref_known(stored_refs_, *ref)) stored_refs_.push_back(*ref);
    if (!store.flush()) {
      state_.command_records.push_back({command, "canon flush failed"});
      return refresh_render();
    }
    state_.command_records.push_back({command, "canon durable ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words.size() == 2 && words[0] == "store" && words[1] == "ls") {
    state_.recovered_entries = store.rebuild_index();
    if (stored_refs_.empty()) {
      state_.command_records.push_back({command, "store refs 0"});
      return refresh_render();
    }

    std::string result = "store refs " + std::to_string(stored_refs_.size());
    for (const auto& ref : stored_refs_) {
      result += "\n" + canon_ref_text(ref);
    }
    state_.command_records.push_back({command, result});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "store" && words[1] == "get") {
    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "store get invalid ref"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    const auto block = store.get(*ref);
    if (!block.has_value()) {
      state_.command_records.push_back({command, "store get missing"});
      return refresh_render();
    }

    state_.command_records.push_back({command, "store get " + decode_text_block(*block)});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "show" && words[1] == "ref") {
    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "show ref invalid ref"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    const auto block = store.get(*ref);
    if (!block.has_value()) {
      state_.command_records.push_back({command, "show ref missing"});
      return refresh_render();
    }

    state_.command_records.push_back(
        {command, "show ref " + canon_ref_text(*ref) + "\n" + decode_text_block(*block)});
    return refresh_render();
  }

  if (words.size() == 3 && words[0] == "store" && words[1] == "rm") {
    const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "store rm invalid ref"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    if (!store.remove(*ref)) {
      state_.command_records.push_back({command, "store rm missing"});
      return refresh_render();
    }
    if (!store.flush()) {
      state_.command_records.push_back({command, "store rm flush failed"});
      return refresh_render();
    }

    stored_refs_.erase(std::remove_if(stored_refs_.begin(),
                                      stored_refs_.end(),
                                      [&](const auto& candidate) {
                                        return candidate.hash == ref->hash;
                                      }),
                       stored_refs_.end());
    named_refs_.erase(std::remove_if(named_refs_.begin(),
                                     named_refs_.end(),
                                     [&](const auto& named) {
                                       return named.ref.hash == ref->hash;
                                     }),
                      named_refs_.end());
    named_objects_.erase(std::remove_if(named_objects_.begin(),
                                        named_objects_.end(),
                                        [&](const auto& object) {
                                          return object.ref.hash == ref->hash;
                                        }),
                         named_objects_.end());
    state_.recovered_entries = store.rebuild_index();
    state_.command_records.push_back({command, "store rm ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words[0] == "history") {
    if (words.size() == 3 && words[1] == "show" && words[2] == "session") {
      std::ostringstream out;
      out << "history session " << state_.command_records.size();
      for (const auto& record : state_.command_records) {
        out << '\n' << record.command;
      }
      state_.command_records.push_back({command, out.str()});
      return refresh_render();
    }

    if (words.size() == 4 && words[1] == "show" && words[2] == "object") {
      const auto ref = resolve_named_or_raw_ref(words[3], named_refs_);
      if (!ref.has_value()) {
        state_.command_records.push_back({command, "history object invalid ref"});
        return refresh_render();
      }

      state_.recovered_entries = store.rebuild_index();
      const auto block = store.get(*ref);
      if (!block.has_value()) {
        state_.command_records.push_back({command, "history object missing"});
        return refresh_render();
      }

      state_.command_records.push_back(
          {command, "history object " + canon_ref_text(*ref) + "\n" + decode_text_block(*block)});
      return refresh_render();
    }

    if (words.size() == 3 && words[1] == "use") {
      const auto ref = resolve_named_or_raw_ref(words[2], named_refs_);
      if (!ref.has_value()) {
        state_.command_records.push_back({command, "history use invalid ref"});
        return refresh_render();
      }

      state_.recovered_entries = store.rebuild_index();
      const auto block = store.get(*ref);
      if (!block.has_value()) {
        state_.command_records.push_back({command, "history use missing"});
        return refresh_render();
      }

      history_ref_ = *ref;
      if (!canon_ref_known(stored_refs_, *ref)) stored_refs_.push_back(*ref);
      state_.command_records.push_back({command, "history use ok " + canon_ref_text(*ref)});
      return refresh_render();
    }

    if (words.size() == 3 && words[1] == "show" && words[2] == "durable") {
      state_.recovered_entries = store.rebuild_index();
      if (!history_ref_.has_value()) {
        state_.command_records.push_back({command, "history durable none"});
        return refresh_render();
      }

      const auto history = store.get(*history_ref_);
      if (!history.has_value()) {
        state_.command_records.push_back({command, "history durable missing"});
        return refresh_render();
      }

      state_.command_records.push_back(
          {command,
           "history durable " + canon_ref_text(*history_ref_) + "\n" +
               decode_text_block(*history)});
      return refresh_render();
    }

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

  if (words[0] == "clear") {
    imported_transcript_lines_.clear();
    state_.command_records.clear();
    state_.command_records.push_back({command, "session transcript cleared"});
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
