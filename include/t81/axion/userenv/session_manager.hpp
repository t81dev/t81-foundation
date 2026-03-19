#pragma once
// experimental/ternaryos/userenv/session_manager.hpp
//
// RFC-00B9 §6 — Session model: identity, login, lifecycle, TTY, CanonFS records.

#include "t81/axion/userenv/service_registry.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace t81::ternaryos::userenv {

// ─── Principal store entry (RFC-00B9 §11) ────────────────────────────────────

struct PrincipalEntry {
  uint64_t                 id{0};
  std::string              name;
  std::string              password_hash;   // Argon2id hash (hex); not plaintext
  std::vector<std::string> capabilities;
  uint32_t                 default_tier{1};
};

// ─── Session state ───────────────────────────────────────────────────────────

enum class SessionState : uint8_t { Active = 0, Suspended, Terminated };

// ─── Session record (RFC-00B9 §6.1) ──────────────────────────────────────────

struct SessionRecord {
  uint64_t     session_id{0};     // monotonic; never reused
  uint64_t     principal_id{0};
  std::string  principal_name;
  std::string  tty_handle;        // e.g. "/dev/tty0"
  uint64_t     root_pgid{0};
  uint64_t     start_epoch{0};    // TISC deterministic epoch counter
  SessionState state{SessionState::Active};
  std::string  canon_hash;        // CanonHash81 hex of session config at login
  // CanonFS path: canonfs://var/sessions/<session_id>.json
  [[nodiscard]] std::string canon_path() const;
};

// ─── Login result ─────────────────────────────────────────────────────────────

enum class LoginStatus : uint8_t {
  Success    = 0,
  BadCredentials,
  SessionDenied,   // Axion SessionCreate gate returned Deny
};

struct LoginResult {
  LoginStatus      status{LoginStatus::BadCredentials};
  SessionRecord    record;          // populated on Success
  AxionGateEvent   gate_event;
};

// ─── Logout result ────────────────────────────────────────────────────────────

struct LogoutResult {
  bool        drained_cleanly{false};   // all processes exited within timeout
  SessionRecord final_record;           // state = Terminated, updated canon_hash
};

// ─── Session environment variables (RFC-00B9 §6.4) ───────────────────────────

struct SessionEnv {
  std::string T81_SESSION_ID;
  std::string T81_PRINCIPAL;
  std::string T81_TTY;
  std::string T81_EPOCH;
  std::string T81_CANON_ROOT;
  std::string PATH;
};

[[nodiscard]] SessionEnv make_session_env(const SessionRecord& rec,
                                          const std::string& canon_root = "canonfs://");

// ─── Session manager ──────────────────────────────────────────────────────────

// Simulates t81-session-mgr (RFC-00B9 §5/§6).  Maintains the session ID counter
// and the in-memory session table.  CanonFS writes are modelled as a string map
// (canon_records_) keyed by the canonfs:// path; tests can inspect these.

class SessionManager {
public:
  SessionManager() = default;

  // Register a principal store for credential verification.
  void load_principals(std::vector<PrincipalEntry> principals);

  // Attempt to log in.  Verifies credential hash, fires SessionCreate gate,
  // allocates a new SessionId, writes SessionRecord to canon_records_.
  // credential_hash: pre-hashed credential (Argon2id hex) — compared directly
  //   against PrincipalEntry::password_hash.  Pass the sentinel value
  //   kAnonymousHash to skip authentication (for tests that don't need it).
  [[nodiscard]] LoginResult login(const std::string& principal_name,
                                  const std::string& credential_hash,
                                  const std::string& tty_handle = "/dev/tty0",
                                  uint64_t           start_epoch = 0);

  // Terminate a session.  Fires session drain, updates SessionRecord.
  [[nodiscard]] LogoutResult logout(uint64_t session_id,
                                    uint32_t drain_timeout_ms = 5000);

  // Look up a live session.
  [[nodiscard]] const SessionRecord* find_session(uint64_t session_id) const;

  // All CanonFS writes (path → JSON string).
  [[nodiscard]] const std::unordered_map<std::string, std::string>& canon_records() const {
    return canon_records_;
  }

  // All Axion gate events fired.
  [[nodiscard]] const std::vector<AxionGateEvent>& gate_events() const {
    return gate_events_;
  }

  // Sentinel: skip credential check in tests.
  static constexpr const char* kAnonymousHash = "__anonymous__";

private:
  uint64_t                                    next_session_id_{1};
  std::vector<PrincipalEntry>                 principals_;
  std::unordered_map<uint64_t, SessionRecord> sessions_;
  std::unordered_map<std::string, std::string> canon_records_;
  std::vector<AxionGateEvent>                 gate_events_;

  void write_canon_record(const SessionRecord& rec);
};

// ─── On-demand service activation (RFC-00B9 §7.2, AC-13/14) ─────────────────

struct ServiceActivationResult {
  bool            success{false};
  AxionGateEvent  gate_event;
  std::string     rejection_reason;  // non-empty on failure
};

// Simulates on_demand service activation from a session context.
// Fires ServiceSpawn gate and validates capabilities (AC-13, AC-14).
[[nodiscard]] ServiceActivationResult activate_service(
    const ServiceRegistry& registry,
    const std::string&     service_id,
    uint64_t               requesting_session_id);

}  // namespace t81::ternaryos::userenv
