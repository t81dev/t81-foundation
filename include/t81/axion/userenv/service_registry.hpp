#pragma once
// experimental/ternaryos/userenv/service_registry.hpp
//
// RFC-00B9 §5/§7 — TernaryOS service registry model.
//
// Defines ServiceEntry, ServiceRegistry (the parsed t81-services.json), and the
// topological-sort-based spawn ordering required by AC-1.
//
// This is a hosted simulation layer: it models the init/session semantics in
// pure C++ without actual process spawning so the acceptance tests can run on
// any host (macOS, Linux, CI).

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace t81::ternaryos::userenv {

// ─── Activation mode ─────────────────────────────────────────────────────────

enum class ActivationMode : uint8_t {
  Required  = 0,  // Started by t81-init at boot; halt on failure
  OnDemand  = 1,  // Started by session manager on first request
  Manual    = 2,  // Never started automatically
};

// ─── Service entry ───────────────────────────────────────────────────────────

struct ServiceEntry {
  std::string              id;
  std::string              binary;
  std::string              canon_hash;       // CanonHash81 hex of binary
  ActivationMode           activation{ActivationMode::Required};
  std::vector<std::string> depends;          // ids that must start first
  std::vector<std::string> capabilities;     // capability names required
  uint32_t                 start_timeout_ms{2000};
  std::string              restart_policy{"always"};  // "always"|"never"|"on-failure"
};

// ─── Known capability names (RFC-00B9 §7.3) ──────────────────────────────────

inline constexpr const char* kKnownCapabilities[] = {
    "SessionCreate",
    "TtyAllocate",
    "TtyRead",
    "TtyWrite",
    "ServiceSpawn",
    "CanonFSWrite",
    "TierElevate",
    "PrincipalAdmin",
    "BootSupervisor",
};

[[nodiscard]] inline bool is_known_capability(const std::string& cap) {
  for (const char* k : kKnownCapabilities)
    if (cap == k) return true;
  return false;
}

// ─── Axion gate verdicts (RFC-00B9 §9) ───────────────────────────────────────

enum class GateVerdict : uint8_t { Allow = 0, Deny = 1, Warn = 2 };

struct AxionGateEvent {
  std::string  op;       // "BootService", "SessionCreate", "ServiceSpawn", "ShellExec"
  std::string  subject;  // service id, PrincipalId, command token
  std::string  payload;  // canon_hash, tty_handle, capabilities, full arg list
  GateVerdict  verdict{GateVerdict::Allow};
};

// ─── Service registry ────────────────────────────────────────────────────────

struct ServiceRegistry {
  uint32_t                    version{1};
  std::vector<ServiceEntry>   services;

  // Returns entries with activation == Required in topologically sorted order.
  // Throws std::runtime_error on cycle detection.
  [[nodiscard]] std::vector<const ServiceEntry*> required_order() const;

  // Returns the entry with the given id, or nullptr.
  [[nodiscard]] const ServiceEntry* find(const std::string& id) const;
  
  // Load service registry from JSON file (t81-services.json format)
  bool load_from_file(const std::string& filename);
};

// ─── Registry loader ─────────────────────────────────────────────────────────

// Parse a JSON string produced by the t81-services.json format (RFC-00B9 §7.1).
// Returns a populated ServiceRegistry.  Throws std::runtime_error on parse error.
[[nodiscard]] ServiceRegistry load_service_registry(const std::string& json_text);

// Convenience: construct a registry from a vector of ServiceEntry objects directly
// (used by tests that don't want to round-trip through JSON).
[[nodiscard]] ServiceRegistry make_service_registry(std::vector<ServiceEntry> entries);

// ─── t81-init boot result ────────────────────────────────────────────────────

struct SpawnRecord {
  std::string service_id;
  bool        integrity_ok{true};
  bool        capability_ok{true};
  GateVerdict verdict{GateVerdict::Allow};
  bool        started{false};
};

struct BootResult {
  bool                    success{false};
  std::vector<SpawnRecord> spawn_log;       // one entry per required service (in order)
  std::string              boot_hash;       // hex CanonHash81 of the service graph
  std::vector<AxionGateEvent> gate_events;
};

// Simulates the t81-init boot sequence (RFC-00B9 §5.2).
//
// For each required service (topological order):
//   1. Fire BootService Axion gate
//   2. Verify canon_hash against binary_hashes[id] (if provided)
//   3. Check all capabilities are known (RFC-00B9 §7.3)
//   4. Record SpawnRecord
//
// The boot_hash is FNV-1a over all service ids + canon_hashes in spawn order.
//
// binary_hashes: map from service id → actual binary hash string.
//   If a service id is absent, binary integrity is assumed OK.
//   If present and mismatches entry.canon_hash, integrity check fails.
[[nodiscard]] BootResult simulate_boot(
    const ServiceRegistry& registry,
    const std::unordered_map<std::string, std::string>& binary_hashes = {});

}  // namespace t81::ternaryos::userenv
