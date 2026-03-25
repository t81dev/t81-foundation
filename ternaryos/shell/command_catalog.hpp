#pragma once

namespace t81::ternaryos {

struct ShellCommandSpec {
  const char* name;
  const char* summary;
  bool        hosted_phase5;
  bool        freestanding_slice6;
};

inline constexpr ShellCommandSpec kShellCommandCatalog[] = {
    {"help", "show available commands", true, true},
    {"profile", "show active profile summary", true, false},
    {"name set <label> <ref>", "bind a stable label to a CanonRef", true, false},
    {"name ls", "list named CanonRef labels", true, false},
    {"object pin <kind> <name> <ref>", "pin a named object alias to a CanonRef", true, false},
    {"object ls", "list named object aliases", true, false},
    {"object show <name>", "show a pinned object alias", true, false},
    {"session status", "show hosted shell session metadata", true, false},
    {"session checkpoint", "persist the current transcript as a checkpoint", true, false},
    {"session export", "export the current transcript anchor", true, false},
    {"session import <ref>", "import a prior transcript by CanonRef", true, false},
    {"session diff <ref>", "diff current transcript against a CanonRef", true, false},
    {"session run <ref>", "run a stored script by CanonRef", true, false},
    {"session show durable", "show the durable transcript anchor", true, false},
    {"session refs", "list tracked session CanonRefs", true, false},
    {"show profile", "show object-native profile state", true, false},
    {"show session", "show object-native session state", true, false},
    {"show ref <canonref>", "show a stored object by CanonRef", true, false},
    {"store put <text>", "persist text into CanonFS-backed storage", true, false},
    {"store put script <line>|<line>|...>", "persist a script block into storage", true, false},
    {"store put ref <ref>", "store a referenced object again by CanonRef", true, false},
    {"store cp <ref>", "copy an object by CanonRef", true, false},
    {"store ls", "list stored CanonRefs", true, false},
    {"store get <ref>", "read a stored object by CanonRef", true, false},
    {"store rm <ref>", "remove a stored object by CanonRef", true, false},
    {"history", "show reboot-history anchor state", true, false},
    {"history show session", "show the current session transcript window", true, false},
    {"history show object <ref>", "show a persisted transcript object", true, false},
    {"history use <ref>", "rebind the durable history anchor", true, false},
    {"history show durable", "show the durable history object", true, false},
    {"clear", "clear the hosted shell transcript window", true, false},
    {"uname", "show freestanding system identity", false, true},
    {"version", "show slice6 build and boot path", false, true},
    {"canonfs", "show storage mode and transport status", false, true},
    {"irq", "show timer IRQ and governed wake counters", false, true},
    {"el0", "show EL0 bridge and capability status", false, true},
    {"waits", "show device-wait scheduler view", false, true},
    {"status", "show kernel counters and governance state", false, true},
    {"threads", "show thread table (tid, state, ticks)", false, true},
    {"sched", "show scheduler counters", false, true},
    {"faults", "show retained EL0 fault records", false, true},
    {"gov", "show governance ring counters by event", false, true},
    {"policy", "show Axion policy summary", false, true},
};

inline constexpr decltype(sizeof(0)) kShellCommandCatalogCount =
    sizeof(kShellCommandCatalog) / sizeof(kShellCommandCatalog[0]);

}  // namespace t81::ternaryos
