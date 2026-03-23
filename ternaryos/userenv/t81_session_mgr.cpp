// experimental/ternaryos/userenv/t81_session_mgr.cpp
//
// RFC-00B9 §6 — t81-session-mgr: Session lifecycle and TTY allocation.
//
// t81-session-mgr is the service that handles user login, session creation,
// TTY allocation, and session termination. It fires the SessionCreate Axion gate
// and maintains the session table with monotonic SessionId assignment.

#include "session_manager.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace t81::ternaryos::userenv {

// ─── Principal store management ─────────────────────────────────────────────

void SessionManager::load_principals(std::vector<PrincipalEntry> principals) {
  principals_ = std::move(principals);
  std::printf("[t81-session-mgr] Loaded %zu principals\n", principals_.size());
}

// ─── Session creation (AC-4, AC-5, AC-6) ───────────────────────────────────────

LoginResult SessionManager::login(const std::string& principal_name,
                                  const std::string& credential_hash,
                                  const std::string& tty_handle,
                                  uint64_t           start_epoch) {
  LoginResult result;
  
  std::printf("[t81-session-mgr] Login attempt: principal='%s', tty='%s'\n",
              principal_name.c_str(), tty_handle.c_str());
  
  // Find principal
  const PrincipalEntry* principal = nullptr;
  for (const auto& p : principals_) {
    if (p.name == principal_name) {
      principal = &p;
      break;
    }
  }
  
  if (!principal) {
    result.status = LoginStatus::BadCredentials;
    std::printf("[t81-session-mgr] Login failed: principal '%s' not found\n",
                principal_name.c_str());
    return result;
  }
  
  // Verify credentials (skip for anonymous hash)
  if (credential_hash != kAnonymousHash && 
      principal->password_hash != credential_hash) {
    result.status = LoginStatus::BadCredentials;
    std::printf("[t81-session-mgr] Login failed: bad credentials for '%s'\n",
                principal_name.c_str());
    return result;
  }
  
  // Fire SessionCreate Axion gate (AC-6)
  AxionGateEvent gate_event;
  gate_event.op = "SessionCreate";
  gate_event.subject = principal_name;
  gate_event.payload = "tty=" + tty_handle + ",tier=" + std::to_string(principal->default_tier);
  
  // Simulate Axion gate decision (always allow for demo)
  gate_event.verdict = GateVerdict::Allow;
  gate_events_.push_back(gate_event);
  
  if (gate_event.verdict == GateVerdict::Deny) {
    result.status = LoginStatus::SessionDenied;
    result.gate_event = gate_event;
    std::printf("[t81-session-mgr] Login denied by Axion gate for '%s'\n",
                principal_name.c_str());
    return result;
  }
  
  // Allocate unique SessionId (AC-4)
  uint64_t session_id = next_session_id_++;
  
  // Create session record (AC-5)
  SessionRecord record;
  record.session_id = session_id;
  record.principal_id = principal->id;
  record.principal_name = principal_name;
  record.tty_handle = tty_handle;
  record.root_pgid = session_id * 1000; // Simulate PGID allocation
  record.start_epoch = start_epoch;
  record.state = SessionState::Active;
  
  // Compute session canon hash (simplified - hash of session config)
  std::string session_config = principal_name + "|" + tty_handle + "|" + std::to_string(session_id);
  uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : session_config) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  char buf[17];
  std::snprintf(buf, sizeof(buf), "%016" PRIx64, hash);
  record.canon_hash = buf;
  
  // Store session
  sessions_[session_id] = record;
  
  // Write to CanonFS (AC-5)
  write_canon_record(record);
  
  // Prepare result
  result.status = LoginStatus::Success;
  result.record = record;
  result.gate_event = gate_event;
  
  std::printf("[t81-session-mgr] Login successful: session_id=%" PRIu64 ", principal='%s'\n",
              session_id, principal_name.c_str());
  std::printf("[t81-session-mgr] Session record written to: %s\n",
              record.canon_path().c_str());
  
  return result;
}

// ─── Session termination (AC-7) ─────────────────────────────────────────────

LogoutResult SessionManager::logout(uint64_t session_id, uint32_t drain_timeout_ms) {
  LogoutResult result;
  
  std::printf("[t81-session-mgr] Logout attempt: session_id=%" PRIu64 "\n", session_id);
  
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) {
    std::printf("[t81-session-mgr] Logout failed: session %" PRIu64 " not found\n", session_id);
    return result;
  }
  
  SessionRecord& record = it->second;
  record.state = SessionState::Terminated;
  
  // Simulate session drain (AC-7)
  // In real implementation, this would:
  // 1. Send SIGTERM to all processes in the session's PGID
  // 2. Wait up to drain_timeout_ms for processes to exit
  // 3. Send SIGKILL to any remaining processes
  
  std::printf("[t81-session-mgr] Draining session PGID %" PRIu64 " (timeout: %u ms)\n",
              record.root_pgid, drain_timeout_ms);
  
  // Simulate drain completion
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate some drain time
  result.drained_cleanly = true;
  
  // Update final record
  write_canon_record(record);
  result.final_record = record;
  
  // Remove from active sessions
  sessions_.erase(it);
  
  std::printf("[t81-session-mgr] Session %" PRIu64 " terminated successfully\n", session_id);
  
  return result;
}

// ─── Session lookup ───────────────────────────────────────────────────────────

const SessionRecord* SessionManager::find_session(uint64_t session_id) const {
  auto it = sessions_.find(session_id);
  return (it != sessions_.end()) ? &it->second : nullptr;
}

// ─── CanonFS record management ───────────────────────────────────────────────

void SessionManager::write_canon_record(const SessionRecord& rec) {
  // Generate JSON representation of session record
  std::ostringstream json;
  json << "{"
       << "\"session_id\":" << rec.session_id << ","
       << "\"principal_id\":" << rec.principal_id << ","
       << "\"principal_name\":\"" << rec.principal_name << "\","
       << "\"tty_handle\":\"" << rec.tty_handle << "\","
       << "\"root_pgid\":" << rec.root_pgid << ","
       << "\"start_epoch\":" << rec.start_epoch << ","
       << "\"state\":\"" << (rec.state == SessionState::Active ? "active" : "terminated") << "\","
       << "\"canon_hash\":\"" << rec.canon_hash << "\""
       << "}";
  
  canon_records_[rec.canon_path()] = json.str();
}

// ─── Session environment helpers ─────────────────────────────────────────────

std::string SessionRecord::canon_path() const {
  return "canonfs://var/sessions/" + std::to_string(session_id) + ".json";
}

SessionEnv make_session_env(const SessionRecord& rec, const std::string& canon_root) {
  SessionEnv env;
  env.T81_SESSION_ID = std::to_string(rec.session_id);
  env.T81_PRINCIPAL = rec.principal_name;
  env.T81_TTY = rec.tty_handle;
  env.T81_EPOCH = std::to_string(rec.start_epoch);
  env.T81_CANON_ROOT = canon_root;
  env.PATH = "/bin:/usr/bin:/usr/local/bin";
  return env;
}

// ─── Service activation (AC-13, AC-14) ───────────────────────────────────────────

ServiceActivationResult activate_service(
    const ServiceRegistry& registry,
    const std::string&     service_id,
    uint64_t               requesting_session_id) {
  
  ServiceActivationResult result;
  
  std::printf("[t81-session-mgr] Service activation request: service='%s', session=%" PRIu64 "\n",
              service_id.c_str(), requesting_session_id);
  
  // Find service
  const ServiceEntry* service = registry.find(service_id);
  if (!service) {
    result.rejection_reason = "Service not found: " + service_id;
    return result;
  }
  
  // Check activation mode
  if (service->activation != ActivationMode::OnDemand) {
    result.rejection_reason = "Service not on-demand: " + service_id;
    return result;
  }
  
  // Fire ServiceSpawn Axion gate (AC-13)
  AxionGateEvent gate_event;
  gate_event.op = "ServiceSpawn";
  gate_event.subject = std::to_string(requesting_session_id);
  gate_event.payload = "service=" + service_id + ",binary=" + service->binary;
  
  // Validate capabilities (AC-14)
  for (const auto& cap : service->capabilities) {
    if (!is_known_capability(cap)) {
      gate_event.verdict = GateVerdict::Deny;
      result.rejection_reason = "Unknown capability: " + cap;
      break;
    }
  }
  
  if (gate_event.verdict != GateVerdict::Deny) {
    gate_event.verdict = GateVerdict::Allow; // Allow for demo
    result.success = true;
  }
  
  result.gate_event = gate_event;
  
  std::printf("[t81-session-mgr] Service activation %s: %s (%s)\n",
              result.success ? "succeeded" : "failed",
              service_id.c_str(),
              result.rejection_reason.c_str());
  
  return result;
}

} // namespace t81::ternaryos::userenv

// ─── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
  std::printf("[t81-session-mgr] Starting TernaryOS Session Manager\n");
  
  t81::ternaryos::userenv::SessionManager mgr;
  
  // Load demo principals
  std::vector<t81::ternaryos::userenv::PrincipalEntry> principals = {
    {1, "admin", "7c222fb2927d828af22f592134e8932480637c0d", {"SessionCreate", "TtyAllocate", "PrincipalAdmin"}, 3},
    {2, "user", "b6d81b360a5672d80c27430f39153e2c4c3b6b5c", {"SessionCreate", "TtyAllocate", "ShellExec"}, 1},
    {3, "guest", t81::ternaryos::userenv::SessionManager::kAnonymousHash, {"SessionCreate"}, 1}
  };
  
  mgr.load_principals(principals);
  
  // Demo login session
  auto login_result = mgr.login("user", "b6d81b360a5672d80c27430f39153e2c4c3b6b5c", "/dev/tty0", 12345);
  
  if (login_result.status == t81::ternaryos::userenv::LoginStatus::Success) {
    std::printf("[t81-session-mgr] Demo login successful, session ready for shell\n");
    
    // Simulate some session activity
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Logout
    auto logout_result = mgr.logout(login_result.record.session_id);
    std::printf("[t81-session-mgr] Session management demo complete\n");
  }
  
  return 0;
}
