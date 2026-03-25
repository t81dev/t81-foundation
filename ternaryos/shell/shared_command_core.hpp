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

template <typename EmitFn>
inline void shell_emit_help(ShellSurface surface, EmitFn emit) {
  for (const auto& spec : kShellCommandCatalog) {
    if (!shell_command_visible(spec, surface)) continue;
    emit(spec.name, spec.summary);
  }
}

}  // namespace t81::ternaryos
