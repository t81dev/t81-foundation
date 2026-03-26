#pragma once

#include "command_catalog.hpp"

namespace t81::ternaryos {

enum class ShellSurface {
  HostedPhase5,
  FreestandingSlice6,
};

enum class ShellBuiltinCommand {
  None,
  Help,
  Tui,
  Studio,
  Agent,
  Uname,
  Version,
  Policy,
};

enum class ShellBuiltinViewKind {
  None,
  TextBlock,
  HelpCatalog,
};

struct ShellBuiltinView {
  ShellBuiltinViewKind kind;
  const char*          text;
};

struct ShellStatusTextField {
  const char* label;
  const char* value;
};

struct ShellStatusUintField {
  const char*          label;
  unsigned long long   value;
  bool                 present;
};

struct ShellCommandContext {
  ShellSurface       surface;
  const char*        profile_summary;
  const char*        storage_binding_name;
  const char*        display_binding_name;
  const char*        path_summary;
  const char*        canonfs_summary;
  const char*        canonfs_mode_summary;
  const char*        canonfs_transport_summary;
  const char*        canonfs_binding_summary;
  const char*        canonfs_probe_summary;
  const char*        policy_engine_summary;
  bool               durable_anchor_present;
  unsigned long long command_count;
  unsigned long long durable_ref_count;
  unsigned long long recovered_entries;
  unsigned long long rendered_glyphs;
  unsigned long long thread_count;
  unsigned long long uptime_s;
  unsigned long long loop_iters;
  unsigned long long tick_count;
  unsigned long long sched_switches;
  unsigned long long interrupt_count;
  bool               has_hosted_session_status;
  bool               has_kernel_status;
  bool               has_canonfs_status;
};

inline bool shell_cstr_eq(const char* lhs, const char* rhs) {
  while (*lhs != '\0' && *rhs != '\0') {
    if (*lhs != *rhs) return false;
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

inline bool shell_command_visible(const ShellCommandSpec& spec, ShellSurface surface) {
  switch (surface) {
    case ShellSurface::HostedPhase5:
      return spec.hosted_phase5;
    case ShellSurface::FreestandingSlice6:
      return spec.freestanding_slice6;
  }
  return false;
}

inline ShellBuiltinCommand shell_builtin_command(const char* word) {
  if (shell_cstr_eq(word, "help")) return ShellBuiltinCommand::Help;
  if (shell_cstr_eq(word, "tui")) return ShellBuiltinCommand::Tui;
  if (shell_cstr_eq(word, "studio")) return ShellBuiltinCommand::Studio;
  if (shell_cstr_eq(word, "agent")) return ShellBuiltinCommand::Agent;
  if (shell_cstr_eq(word, "uname")) return ShellBuiltinCommand::Uname;
  if (shell_cstr_eq(word, "version")) return ShellBuiltinCommand::Version;
  if (shell_cstr_eq(word, "policy")) return ShellBuiltinCommand::Policy;
  return ShellBuiltinCommand::None;
}

inline const char* shell_studio_text(ShellSurface surface) {
  switch (surface) {
    case ShellSurface::HostedPhase5:
      return "[axion studio]\n"
             "  frontend    : hosted FTXUI\n"
             "  entry       : ./build/t81_ternaryos_shell_tui\n"
             "  note        : uses the shared shell backend contract";
    case ShellSurface::FreestandingSlice6:
      return "[axion studio]\n"
             "  frontend    : unavailable in slice6 guest\n"
             "  handoff     : host-assisted launcher bridge\n"
             "  host entry  : ./build/t81_ternaryos_shell_tui\n"
             "  [axion studio handoff] hosted-phase5";
  }
  return "[axion studio]";
}

inline const char* shell_agent_text(ShellSurface surface) {
  switch (surface) {
    case ShellSurface::HostedPhase5:
      return "[axion agent]\n"
             "  frontend    : unavailable in current hosted shell lane\n"
             "  note        : RFC-0033 agent frontend is not yet wired here";
    case ShellSurface::FreestandingSlice6:
      return "[axion agent]\n"
             "  frontend    : unavailable in current shell lane\n"
             "  note        : RFC-0033 agent handoff is not yet wired";
  }
  return "[axion agent]";
}

inline const char* shell_uname_text(ShellSurface surface) {
  switch (surface) {
    case ShellSurface::HostedPhase5:
      return "T81 TernaryOS 1.0 hosted axion-shell";
    case ShellSurface::FreestandingSlice6:
      return "T81 TernaryOS 1.0 AArch64 axion-kernel (bare-metal EFI)";
  }
  return "T81 TernaryOS";
}

inline const char* shell_version_text(ShellSurface surface) {
  switch (surface) {
    case ShellSurface::HostedPhase5:
      return "T81 / Axion  --  hosted shell session\n"
             "Architecture : Hosted C++ guest-bootstrap seam\n"
             "Boot path    : ShellSession -> bootstrap_virtualbox_guest -> hal_main";
    case ShellSurface::FreestandingSlice6:
      return "T81 / Axion  --  ternary OS kernel (bare-metal EFI bridge)\n"
             "Architecture : AArch64 (QEMU virt, cortex-a57, EDK2)\n"
             "Boot path    : EFI efi_main -> ExitBootServices -> C++ bridge";
  }
  return "T81 / Axion";
}

inline const char* shell_policy_text(ShellSurface surface) {
  switch (surface) {
    case ShellSurface::HostedPhase5:
      return "[axion policy]\n"
             "  governance  : active\n"
             "  audit trail : canonfs-backed hosted session\n"
             "  constraints : RFC-00B0 ethics-first boot";
    case ShellSurface::FreestandingSlice6:
      return "[axion policy]\n"
             "  governance  : active\n"
             "  audit trail : canonfs (in-memory)\n"
             "  constraints : RFC-00B0 ethics-first boot";
  }
  return "[axion policy]";
}

inline ShellBuiltinView shell_builtin_view(ShellBuiltinCommand command,
                                           ShellSurface surface) {
  switch (command) {
    case ShellBuiltinCommand::Help:
      return {ShellBuiltinViewKind::HelpCatalog, nullptr};
    case ShellBuiltinCommand::Tui:
    case ShellBuiltinCommand::Studio:
      return {ShellBuiltinViewKind::TextBlock, shell_studio_text(surface)};
    case ShellBuiltinCommand::Agent:
      return {ShellBuiltinViewKind::TextBlock, shell_agent_text(surface)};
    case ShellBuiltinCommand::Uname:
      return {ShellBuiltinViewKind::TextBlock, shell_uname_text(surface)};
    case ShellBuiltinCommand::Version:
      return {ShellBuiltinViewKind::TextBlock, shell_version_text(surface)};
    case ShellBuiltinCommand::Policy:
      return {ShellBuiltinViewKind::TextBlock, shell_policy_text(surface)};
    case ShellBuiltinCommand::None:
      break;
  }
  return {ShellBuiltinViewKind::None, nullptr};
}

template <typename EmitFn>
inline void shell_emit_help(ShellSurface surface, bool include_operator, EmitFn emit) {
  for (const auto& spec : kShellCommandCatalog) {
    if (!shell_command_visible(spec, surface)) continue;
    if (!include_operator && !spec.main_help) continue;
    if (include_operator && spec.main_help) continue;
    emit(spec.name, spec.summary);
  }
}

template <typename EmitHeaderFn, typename EmitTextFn, typename EmitUintFn>
inline void shell_emit_status_view(const char* header,
                                   const ShellStatusTextField* text_fields,
                                   decltype(sizeof(0)) text_count,
                                   const ShellStatusUintField* uint_fields,
                                   decltype(sizeof(0)) uint_count,
                                   EmitHeaderFn emit_header,
                                   EmitTextFn emit_text,
                                   EmitUintFn emit_uint) {
  emit_header(header);
  for (decltype(text_count) i = 0; i < text_count; ++i) {
    if (text_fields[i].value == nullptr) continue;
    emit_text(text_fields[i].label, text_fields[i].value);
  }
  for (decltype(uint_count) i = 0; i < uint_count; ++i) {
    if (!uint_fields[i].present) continue;
    emit_uint(uint_fields[i].label, uint_fields[i].value);
  }
}

template <typename EmitHeaderFn, typename EmitTextFn, typename EmitUintFn>
inline void shell_emit_status_from_context(const ShellCommandContext& context,
                                           EmitHeaderFn emit_header,
                                           EmitTextFn emit_text,
                                           EmitUintFn emit_uint) {
  if (context.has_hosted_session_status) {
    const ShellStatusTextField text_fields[] = {
        {"session profile", context.profile_summary},
        {"storage", context.storage_binding_name},
        {"display", context.display_binding_name},
        {"durable anchor", context.durable_anchor_present ? "tracked" : "none"},
    };
    const ShellStatusUintField uint_fields[] = {
        {"commands", context.command_count, true},
        {"durable refs", context.durable_ref_count, true},
        {"recovered", context.recovered_entries, true},
        {"glyphs", context.rendered_glyphs, true},
    };
    shell_emit_status_view(
        nullptr,
        text_fields,
        sizeof(text_fields) / sizeof(text_fields[0]),
        uint_fields,
        sizeof(uint_fields) / sizeof(uint_fields[0]),
        [&](const char*) {},
        emit_text,
        emit_uint);
    return;
  }

  if (context.has_kernel_status) {
    const ShellStatusTextField text_fields[] = {
        {"path", context.path_summary},
        {"canonfs", context.canonfs_summary},
        {"policy engine", context.policy_engine_summary},
    };
    const ShellStatusUintField uint_fields[] = {
        {"threads", context.thread_count, true},
        {"uptime (s)", context.uptime_s, true},
        {"loop_iters", context.loop_iters, true},
        {"tick_count", context.tick_count, true},
        {"sched switches", context.sched_switches, true},
        {"interrupts", context.interrupt_count, true},
        {"commands", context.command_count, true},
    };
    shell_emit_status_view(
        "[kernel]",
        text_fields,
        sizeof(text_fields) / sizeof(text_fields[0]),
        uint_fields,
        sizeof(uint_fields) / sizeof(uint_fields[0]),
        emit_header,
        emit_text,
        emit_uint);
  }
}

template <typename EmitHeaderFn, typename EmitTextFn>
inline void shell_emit_canonfs_from_context(const ShellCommandContext& context,
                                            EmitHeaderFn emit_header,
                                            EmitTextFn emit_text) {
  if (!context.has_canonfs_status) return;
  emit_header("[canonfs]");
  const ShellStatusTextField text_fields[] = {
      {"mode", context.canonfs_mode_summary},
      {"transport", context.canonfs_transport_summary},
      {"binding", context.canonfs_binding_summary},
      {"io_probe", context.canonfs_probe_summary},
  };
  for (decltype(sizeof(0)) i = 0; i < sizeof(text_fields) / sizeof(text_fields[0]); ++i) {
    if (text_fields[i].value == nullptr) continue;
    emit_text(text_fields[i].label, text_fields[i].value);
  }
}

}  // namespace t81::ternaryos
