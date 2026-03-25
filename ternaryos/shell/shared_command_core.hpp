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
  if (shell_cstr_eq(word, "uname")) return ShellBuiltinCommand::Uname;
  if (shell_cstr_eq(word, "version")) return ShellBuiltinCommand::Version;
  if (shell_cstr_eq(word, "policy")) return ShellBuiltinCommand::Policy;
  return ShellBuiltinCommand::None;
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
inline void shell_emit_help(ShellSurface surface, EmitFn emit) {
  for (const auto& spec : kShellCommandCatalog) {
    if (!shell_command_visible(spec, surface)) continue;
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

}  // namespace t81::ternaryos
