#pragma once

namespace t81::ternaryos {

struct ShellCommandSpec {
  const char* name;
  const char* summary;
  bool        hosted_phase5;
  bool        freestanding_slice6;
  bool        main_help;
};

inline constexpr ShellCommandSpec kShellCommandCatalog[] = {
    {"help", "show RFC-00B9 shell commands", true, true, true},
    {"exit [code]", "terminate the current session", true, true, true},
    {"cd <path>", "change working directory", true, true, true},
    {"ls [path]", "list CanonFS directory", true, true, true},
    {"cat <path>", "print CanonFS file contents", true, true, true},
    {"hash <path>", "print CanonHash81 of a CanonFS artifact", true, true, true},
    {"run <file.tisc>", "execute a TISC bytecode file", true, true, true},
    {"compile <f.t81>", "compile a T81Lang source file to TISC bytecode", true, true, true},
    {"policy", "inspect or modify Axion policies", true, true, true},
    {"service <cmd>", "start, stop, or inspect service registry entries", true, true, true},
    {"tier <n>", "elevate the session VM tier", true, true, true},
    {"studio", "launch the t81 studio TUI in the current TTY", true, true, true},
    {"agent", "launch the t81 agent TUI in the current TTY", true, true, true},
    {"tui", "compatibility alias for studio", true, true, false},
    {"uname", "show system identity", true, true, false},
    {"version", "show shell/runtime build info", true, true, false},
    {"canonfs", "show storage mode and transport status", true, true, false},
    {"profile", "show active profile summary", true, false, false},
    {"name set <label> <ref>", "bind a stable label to a CanonRef", true, false, false},
    {"name ls", "list named CanonRef labels", true, false, false},
    {"object pin <kind> <name> <ref>", "pin a named object alias to a CanonRef", true, false, false},
    {"object ls", "list named object aliases", true, false, false},
    {"object show <name>", "show a pinned object alias", true, false, false},
    {"session status", "show hosted shell session metadata", true, false, false},
    {"session checkpoint", "persist the current transcript as a checkpoint", true, false, false},
    {"session export", "export the current transcript anchor", true, false, false},
    {"session import <ref>", "import a prior transcript by CanonRef", true, false, false},
    {"session diff <ref>", "diff current transcript against a CanonRef", true, false, false},
    {"session run <ref>", "run a stored script by CanonRef", true, false, false},
    {"session show durable", "show the durable transcript anchor", true, false, false},
    {"session refs", "list tracked session CanonRefs", true, false, false},
    {"show profile", "show object-native profile state", true, false, false},
    {"show session", "show object-native session state", true, false, false},
    {"show ref <canonref>", "show a stored object by CanonRef", true, false, false},
    {"store put <text>", "persist text into CanonFS-backed storage", true, false, false},
    {"store put script <line>|<line>|...>", "persist a script block into storage", true, false, false},
    {"store put ref <ref>", "store a referenced object again by CanonRef", true, false, false},
    {"store cp <ref>", "copy an object by CanonRef", true, false, false},
    {"store ls", "list stored CanonRefs", true, false, false},
    {"store get <ref>", "read a stored object by CanonRef", true, false, false},
    {"store rm <ref>", "remove a stored object by CanonRef", true, false, false},
    {"history", "show reboot-history anchor state", true, false, false},
    {"history show session", "show the current session transcript window", true, false, false},
    {"history show object <ref>", "show a persisted transcript object", true, false, false},
    {"history use <ref>", "rebind the durable history anchor", true, false, false},
    {"history show durable", "show the durable history object", true, false, false},
    {"clear", "clear the hosted shell transcript window", true, false, false},
    {"canonfs ls", "show known CanonFS artifact inventory", true, true, false},
    {"canonfs hash <alias>", "show retained hash for a known CanonFS artifact", true, true, false},
    {"canonfs run <alias>", "run a known CanonFS artifact", true, true, false},
    {"irq", "show timer IRQ and governed wake counters", false, true, false},
    {"el0", "show EL0 bridge and capability status", false, true, false},
    {"waits", "show device-wait scheduler view", false, true, false},
    {"status", "show kernel counters and governance state", false, true, false},
    {"threads", "show thread table (tid, state, ticks)", false, true, false},
    {"sched", "show scheduler counters", false, true, false},
    {"faults", "show retained EL0 fault records", false, true, false},
    {"gov", "show governance ring counters by event", false, true, false},
};

inline constexpr decltype(sizeof(0)) kShellCommandCatalogCount =
    sizeof(kShellCommandCatalog) / sizeof(kShellCommandCatalog[0]);

}  // namespace t81::ternaryos
