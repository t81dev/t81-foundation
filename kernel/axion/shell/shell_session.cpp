// src/axion/shell/shell_session.cpp

#include "t81/axion/shell/shell_session.hpp"

#include "../../../ternaryos/shell/command_catalog.hpp"
#include "../../../ternaryos/shell/shared_command_core.hpp"

#include "dev/canon_store.hpp"
#include "dev/framebuffer.hpp"
#include "dev/hosted_block_dev.hpp"
#include "dev/ttf.hpp"
#include "hal/hal.hpp"
#include "hal/virtualbox_guest_devices.hpp"
#include "t81/axion/userenv/service_registry.hpp"
#include "t81/axion/userenv/session_manager.hpp"
#include "t81/axion/userenv/t81sh.hpp"

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
    if (shell_command_visible(spec, ShellSurface::HostedPhase5) && spec.main_help) {
      names.emplace_back(spec.name);
    }
  }
  return names;
}

userenv::ServiceRegistry make_hosted_service_registry() {
  using userenv::ActivationMode;
  using userenv::ServiceEntry;

  ServiceEntry session_mgr;
  session_mgr.id = "t81-session-mgr";
  session_mgr.binary = "/bin/t81-session-mgr";
  session_mgr.canon_hash = "deadbeef00000001";
  session_mgr.activation = ActivationMode::Required;
  session_mgr.capabilities = {"SessionCreate", "TtyAllocate", "ServiceSpawn"};
  session_mgr.start_timeout_ms = 2000;

  ServiceEntry canonfs_daemon;
  canonfs_daemon.id = "t81-canonfs-daemon";
  canonfs_daemon.binary = "/bin/t81-canonfs-daemon";
  canonfs_daemon.canon_hash = "deadbeef00000002";
  canonfs_daemon.activation = ActivationMode::Required;
  canonfs_daemon.capabilities = {"CanonFSWrite"};
  canonfs_daemon.start_timeout_ms = 1000;

  ServiceEntry studio;
  studio.id = "t81-studio";
  studio.binary = "/usr/bin/t81";
  studio.canon_hash = "deadbeef00000003";
  studio.activation = ActivationMode::OnDemand;
  studio.depends = {"t81-session-mgr"};
  studio.capabilities = {"TtyRead", "TtyWrite"};
  studio.start_timeout_ms = 5000;
  studio.restart_policy = "never";

  ServiceEntry agent;
  agent.id = "t81-agent";
  agent.binary = "/usr/bin/t81";
  agent.canon_hash = "deadbeef00000004";
  agent.activation = ActivationMode::OnDemand;
  agent.depends = {"t81-session-mgr"};
  agent.capabilities = {"TtyRead", "TtyWrite"};
  agent.start_timeout_ms = 5000;
  agent.restart_policy = "never";

  return userenv::make_service_registry(
      {session_mgr, canonfs_daemon, studio, agent});
}

std::vector<userenv::PrincipalEntry> make_hosted_principals() {
  userenv::PrincipalEntry root;
  root.id = 1;
  root.name = "root";
  root.password_hash = "argon2id:correct_hash";
  root.capabilities = {"SessionCreate", "ServiceSpawn", "TtyAllocate",
                       "CanonFSWrite", "TierElevate", "PrincipalAdmin"};
  root.default_tier = 1;

  userenv::PrincipalEntry agent;
  agent.id = 2;
  agent.name = "agent";
  agent.password_hash = "argon2id:agent_hash";
  agent.capabilities = {"SessionCreate", "ServiceSpawn", "TtyAllocate"};
  agent.default_tier = 2;

  return {root, agent};
}

const char* activation_mode_text(userenv::ActivationMode mode) {
  switch (mode) {
    case userenv::ActivationMode::Required:
      return "required";
    case userenv::ActivationMode::OnDemand:
      return "on_demand";
    case userenv::ActivationMode::Manual:
      return "manual";
  }
  return "unknown";
}

std::string hosted_service_text(const userenv::ServiceRegistry& registry,
                                const userenv::SessionRecord& session,
                                const ParsedCommand& parsed) {
  const auto& words = parsed.words;
  if (words.size() == 1) {
    return "service commands\nlist\nshow <id>\nstart <id>";
  }

  if (words.size() == 2 && words[1] == "list") {
    std::string out = "service list " + std::to_string(registry.services.size());
    for (const auto& service : registry.services) {
      out += "\n" + service.id + " " + activation_mode_text(service.activation);
    }
    return out;
  }

  if (words.size() == 3 && words[1] == "show") {
    const auto* svc = registry.find(words[2]);
    if (svc == nullptr) return "service show missing";

    std::string out = "service show " + svc->id;
    out += "\nbinary " + svc->binary;
    out += "\nactivation " + std::string(activation_mode_text(svc->activation));
    out += "\nrestart " + svc->restart_policy;
    out += "\ncanon_hash " + svc->canon_hash;
    out += "\ncapabilities " + std::to_string(svc->capabilities.size());
    for (const auto& cap : svc->capabilities) out += "\ncap " + cap;
    return out;
  }

  if (words.size() == 3 && words[1] == "start") {
    std::string service_id = words[2];
    if (service_id == "studio") service_id = "t81-studio";
    if (service_id == "agent") service_id = "t81-agent";
    const auto result = userenv::activate_service(registry, service_id, session.session_id);
    if (!result.success) {
      return "service start denied " + service_id + "\nreason " + result.rejection_reason;
    }
    return "service start ok " + service_id;
  }

  return "service invalid";
}

std::optional<t81::canonfs::CanonRef> hosted_path_ref(
    std::string_view path,
    const std::vector<ShellNamedRef>& named_refs) {
  const std::string path_key(path);
  if (const auto ref = resolve_named_or_raw_ref(path, named_refs); ref.has_value()) {
    return ref;
  }
  for (const auto& named : named_refs) {
    if (named.label == path_key) return named.ref;
  }
  return std::nullopt;
}

std::string hosted_hash_text(const userenv::ServiceRegistry& registry,
                             std::string_view path,
                             const std::vector<ShellNamedRef>& named_refs) {
  if (const auto ref = hosted_path_ref(path, named_refs); ref.has_value()) {
    return "hash " + std::string(path) + "\ncanon_hash " + canon_ref_text(*ref);
  }

  for (const auto& service : registry.services) {
    if (service.binary == path) {
      return "hash " + std::string(path) + "\ncanon_hash " + service.canon_hash;
    }
  }

  return "hash missing";
}

std::string hosted_ls_text(const std::vector<t81::canonfs::CanonRef>& stored_refs,
                           const std::vector<ShellNamedRef>& named_refs,
                           const std::vector<ShellNamedObject>& named_objects) {
  std::ostringstream out;
  out << "ls";
  const std::size_t total_entries =
      stored_refs.size() + named_refs.size() + named_objects.size();
  out << '\n' << "entries " << total_entries;

  for (const auto& object : named_objects) {
    out << '\n' << object.kind << ' ' << object.name << ' ' << canon_ref_text(object.ref);
  }
  for (const auto& named : named_refs) {
    out << '\n' << '@' << named.label << ' ' << canon_ref_text(named.ref);
  }
  for (const auto& ref : stored_refs) {
    out << '\n' << canon_ref_text(ref);
  }

  return out.str();
}

std::string hosted_help_text() {
  std::string text = "builtins";
  shell_emit_help(ShellSurface::HostedPhase5, false, [&](const char* name, const char* summary) {
    text += "\n";
    text += name;
    text += " -- ";
    text += summary;
  });
  return text;
}

std::string hosted_operator_help_text() {
  std::string text = "operator commands";
  shell_emit_help(ShellSurface::HostedPhase5, true, [&](const char* name, const char* summary) {
    text += "\n";
    text += name;
    text += " -- ";
    text += summary;
  });
  return text;
}

std::string hosted_rfc00b9_stub(std::string_view command) {
  return std::string(command) + " not yet implemented in the current RFC-00B9 shell lane";
}

std::string hosted_builtin_text(ShellBuiltinCommand command) {
  const auto view = shell_builtin_view(command, ShellSurface::HostedPhase5);
  switch (view.kind) {
    case ShellBuiltinViewKind::HelpCatalog:
      return hosted_help_text();
    case ShellBuiltinViewKind::TextBlock:
      return view.text != nullptr ? std::string(view.text) : std::string();
    case ShellBuiltinViewKind::None:
      break;
  }
  return {};
}

std::string hosted_session_status_text(const ShellSessionState& state,
                                       const std::vector<t81::canonfs::CanonRef>& stored_refs,
                                       bool durable_anchor_tracked,
                                       const std::optional<userenv::T81Shell>& user_shell) {
  ShellCommandContext context{};
  context.surface = ShellSurface::HostedPhase5;
  context.profile_summary = state.profile_summary.c_str();
  context.storage_binding_name = state.storage_binding_name.c_str();
  context.display_binding_name = state.display_binding_name.c_str();
  context.durable_anchor_present = durable_anchor_tracked;
  context.command_count = static_cast<unsigned long long>(state.command_records.size());
  context.durable_ref_count = static_cast<unsigned long long>(stored_refs.size());
  context.recovered_entries = static_cast<unsigned long long>(state.recovered_entries);
  context.rendered_glyphs = static_cast<unsigned long long>(state.rendered_glyphs);
  context.has_hosted_session_status = true;

  std::ostringstream out;
  shell_emit_status_from_context(
      context,
      [&](const char*) {},
      [&](const char* label, const char* value) {
        out << label << ' ' << value << '\n';
      },
      [&](const char* label, unsigned long long value) {
        out << label << ' ' << value << '\n';
      });
  std::string text = out.str();
  if (!text.empty() && text.back() == '\n') text.pop_back();
  if (user_shell.has_value()) {
    text += "\nprompt " + user_shell->prompt();
    text += "\nhistory " + user_shell->history_canon_path();
    text += "\ntier " + std::to_string(user_shell->tier());
  }
  return text;
}

std::string hosted_canonfs_text(const ShellSessionState& state) {
  ShellCommandContext context{};
  context.surface = ShellSurface::HostedPhase5;
  context.profile_summary = state.profile_summary.c_str();
  context.storage_binding_name = state.storage_binding_name.c_str();
  context.display_binding_name = state.display_binding_name.c_str();
  context.canonfs_mode_summary = "persistent (CanonStore-backed)";
  context.canonfs_transport_summary = "hosted-block-dev";
  context.canonfs_binding_summary = state.storage_binding_name.c_str();
  context.canonfs_probe_summary = "n/a (hosted bootstrap seam)";
  context.has_canonfs_status = true;

  std::ostringstream out;
  shell_emit_canonfs_from_context(
      context,
      [&](const char* header) { out << header << '\n'; },
      [&](const char* label, const char* value) { out << "  " << label << ' ' << value << '\n'; });
  std::string text = out.str();
  if (!text.empty() && text.back() == '\n') text.pop_back();
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

ShellSession::ShellSession(bool quiet_boot)
    : quiet_boot_(quiet_boot), store_path_(unique_store_path()) {
  state_.available_commands = hosted_command_names();
  service_registry_ = make_hosted_service_registry();
  session_manager_.load_principals(make_hosted_principals());
}

ShellSession::~ShellSession() = default;

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

  const auto login =
      session_manager_.login("root",
                             userenv::SessionManager::kAnonymousHash,
                             "/dev/tty0",
                             0);
  if (login.status != userenv::LoginStatus::Success) return false;
  user_shell_.emplace(login.record, 1u);

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

ShellCommandContext ShellSession::command_context() const {
  ShellCommandContext context{};
  context.surface = ShellSurface::HostedPhase5;
  context.profile_summary = state_.profile_summary.c_str();
  context.storage_binding_name = state_.storage_binding_name.c_str();
  context.display_binding_name = state_.display_binding_name.c_str();
  context.durable_anchor_present = history_ref_.has_value();
  context.command_count = static_cast<unsigned long long>(state_.command_records.size());
  context.durable_ref_count = static_cast<unsigned long long>(stored_refs_.size());
  context.recovered_entries = static_cast<unsigned long long>(state_.recovered_entries);
  context.rendered_glyphs = static_cast<unsigned long long>(state_.rendered_glyphs);
  context.canonfs_mode_summary = "persistent (CanonStore-backed)";
  context.canonfs_transport_summary = "hosted-block-dev";
  context.canonfs_binding_summary = state_.storage_binding_name.c_str();
  context.canonfs_probe_summary = "n/a (hosted bootstrap seam)";
  context.has_hosted_session_status = true;
  context.has_canonfs_status = true;
  return context;
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

  if (words.size() == 2 && words[0] == "help" && words[1] == "operator") {
    state_.command_records.push_back({command, hosted_operator_help_text()});
    return refresh_render();
  }

  switch (shell_builtin_command(words[0].c_str())) {
    case ShellBuiltinCommand::Help:
      state_.command_records.push_back(
          {command, hosted_builtin_text(ShellBuiltinCommand::Help)});
      return refresh_render();
    case ShellBuiltinCommand::Tui:
    case ShellBuiltinCommand::Studio:
      if (user_shell_.has_value()) (void)user_shell_->exec_command("studio");
      state_.command_records.push_back(
          {command, hosted_builtin_text(ShellBuiltinCommand::Studio)});
      return refresh_render();
    case ShellBuiltinCommand::Agent:
      if (user_shell_.has_value()) (void)user_shell_->exec_command("agent");
      state_.command_records.push_back(
          {command, hosted_builtin_text(ShellBuiltinCommand::Agent)});
      return refresh_render();
    case ShellBuiltinCommand::Uname:
      state_.command_records.push_back(
          {command, hosted_builtin_text(ShellBuiltinCommand::Uname)});
      return refresh_render();
    case ShellBuiltinCommand::Version:
      state_.command_records.push_back(
          {command, hosted_builtin_text(ShellBuiltinCommand::Version)});
      return refresh_render();
    case ShellBuiltinCommand::Policy:
      if (user_shell_.has_value()) (void)user_shell_->exec_command(command);
      state_.command_records.push_back(
          {command, hosted_builtin_text(ShellBuiltinCommand::Policy)});
      return refresh_render();
    case ShellBuiltinCommand::None:
      break;
  }

  if (words[0] == "exit") {
    if (user_shell_.has_value()) (void)user_shell_->exec_command(command);
    state_.command_records.push_back({command, "session exit requested"});
    return refresh_render();
  }

  if (words[0] == "tier") {
    if (!user_shell_.has_value()) {
      state_.command_records.push_back({command, "tier unavailable"});
      return refresh_render();
    }
    const bool ok = user_shell_->exec_command(command);
    const auto tier = user_shell_->tier();
    state_.command_records.push_back(
        {command, ok ? "tier ok " + std::to_string(tier) : "tier failed"});
    return refresh_render();
  }

  if (words[0] == "service") {
    if (!user_shell_.has_value()) {
      state_.command_records.push_back({command, "service unavailable"});
      return refresh_render();
    }
    (void)user_shell_->exec_command(command);
    state_.command_records.push_back(
        {command, hosted_service_text(service_registry_, user_shell_->session(), parsed)});
    return refresh_render();
  }

  if (words[0] == "hash") {
    if (!user_shell_.has_value()) {
      state_.command_records.push_back({command, "hash unavailable"});
      return refresh_render();
    }
    (void)user_shell_->exec_command(command);
    if (words.size() < 2) {
      state_.command_records.push_back({command, "hash invalid"});
      return refresh_render();
    }
    state_.command_records.push_back(
        {command, hosted_hash_text(service_registry_, words[1], named_refs_)});
    return refresh_render();
  }

  if (words[0] == "run") {
    if (!user_shell_.has_value()) {
      state_.command_records.push_back({command, "run unavailable"});
      return refresh_render();
    }
    (void)user_shell_->exec_command(command);
    if (words.size() < 2) {
      state_.command_records.push_back({command, "run invalid"});
      return refresh_render();
    }

    const auto ref = hosted_path_ref(words[1], named_refs_);
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "run missing"});
      return refresh_render();
    }

    std::string session_run_command = "session run " + words[1];
    if (!execute_command(session_run_command)) {
      state_.command_records.push_back({command, "run failed"});
      return refresh_render();
    }
    state_.command_records.push_back(
        {command, "run ok " + canon_ref_text(*ref)});
    return refresh_render();
  }

  if (words[0] == "ls") {
    if (user_shell_.has_value()) (void)user_shell_->exec_command(command);
    state_.command_records.push_back(
        {command, hosted_ls_text(stored_refs_, named_refs_, named_objects_)});
    return refresh_render();
  }

  if (words[0] == "cd" || words[0] == "compile") {
    if (user_shell_.has_value()) (void)user_shell_->exec_command(command);
    state_.command_records.push_back({command, hosted_rfc00b9_stub(words[0])});
    return refresh_render();
  }

  if (words[0] == "profile") {
    state_.command_records.push_back({command, state_.profile_summary});
    return refresh_render();
  }

  if (words.size() == 1 && words[0] == "canonfs") {
    state_.command_records.push_back({command, hosted_canonfs_text(state_)});
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
    state_.command_records.push_back(
        {command, hosted_session_status_text(state_, stored_refs_, history_ref_.has_value(), user_shell_)});
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
    if (user_shell_.has_value()) {
      out << '\n' << "prompt " << user_shell_->prompt()
          << '\n' << "history " << user_shell_->history_canon_path()
          << '\n' << "tier " << user_shell_->tier();
    }
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

  if (words[0] == "cat") {
    if (user_shell_.has_value()) (void)user_shell_->exec_command(command);
    if (words.size() < 2) {
      state_.command_records.push_back({command, "cat invalid"});
      return refresh_render();
    }

    const std::string path_key(words[1]);
    std::optional<t81::canonfs::CanonRef> ref = resolve_named_or_raw_ref(path_key, named_refs_);
    if (!ref.has_value()) {
      for (const auto& named : named_refs_) {
        if (named.label == path_key) {
          ref = named.ref;
          break;
        }
      }
    }
    if (!ref.has_value()) {
      for (const auto& object : named_objects_) {
        if (object.name == path_key) {
          ref = object.ref;
          break;
        }
      }
    }
    if (!ref.has_value()) {
      state_.command_records.push_back({command, "cat missing"});
      return refresh_render();
    }

    state_.recovered_entries = store.rebuild_index();
    const auto block = store.get(*ref);
    if (!block.has_value()) {
      state_.command_records.push_back({command, "cat missing"});
      return refresh_render();
    }

    state_.command_records.push_back(
        {command, "cat " + words[1] + "\n" + decode_text_block(*block)});
    return refresh_render();
  }

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
