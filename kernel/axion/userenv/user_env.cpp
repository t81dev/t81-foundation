// src/axion/userenv/user_env.cpp
//
// RFC-00B9 — Implementation of service_registry, session_manager, and t81sh.

#include "t81/axion/userenv/service_registry.hpp"
#include "t81/axion/userenv/session_manager.hpp"
#include "t81/axion/userenv/t81sh.hpp"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace t81::ternaryos::userenv {

// ═══════════════════════════════════════════════════════════════════════════
// Service registry helpers
// ═══════════════════════════════════════════════════════════════════════════

namespace {

// Tiny FNV-1a over a range of chars — used for the boot hash.
constexpr uint64_t fnv1a(const std::string& s) noexcept {
  uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  return h;
}

std::string hex64(uint64_t v) {
  char buf[17];
  std::snprintf(buf, sizeof(buf), "%016" PRIx64, v);
  return buf;
}

}  // namespace

// ─── ServiceRegistry ─────────────────────────────────────────────────────────

const ServiceEntry* ServiceRegistry::find(const std::string& id) const {
  for (const auto& s : services)
    if (s.id == id) return &s;
  return nullptr;
}

bool ServiceRegistry::load_from_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::fprintf(stderr, "Failed to open services file: %s\n", filename.c_str());
    return false;
  }
  
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string json_text = buffer.str();
  
  try {
    ServiceRegistry loaded = load_service_registry(json_text);
    *this = std::move(loaded);
    return true;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "Failed to parse services file %s: %s\n", 
                filename.c_str(), e.what());
    return false;
  }
}

std::vector<const ServiceEntry*> ServiceRegistry::required_order() const {
  // Collect required entries.
  std::vector<const ServiceEntry*> required;
  for (const auto& s : services)
    if (s.activation == ActivationMode::Required) required.push_back(&s);

  // Kahn's algorithm — topological sort on depends graph.
  std::unordered_map<std::string, int> in_degree;
  std::unordered_map<std::string, std::vector<std::string>> adj;  // dep → dependents
  for (const auto* e : required) {
    in_degree.emplace(e->id, 0);
    for (const auto& dep : e->depends) {
      adj[dep].push_back(e->id);
      in_degree[e->id]++;
    }
  }

  std::vector<std::string> queue;
  for (const auto& [id, deg] : in_degree)
    if (deg == 0) queue.push_back(id);
  std::sort(queue.begin(), queue.end());  // deterministic tie-break

  std::vector<const ServiceEntry*> result;
  while (!queue.empty()) {
    auto it = queue.begin();
    const std::string cur = *it;
    queue.erase(it);
    if (const auto* e = find(cur))
      result.push_back(e);
    if (adj.count(cur)) {
      for (const auto& next : adj.at(cur)) {
        if (--in_degree.at(next) == 0)
          queue.push_back(next);
      }
      std::sort(queue.begin(), queue.end());
    }
  }

  if (result.size() != required.size())
    throw std::runtime_error("ServiceRegistry: dependency cycle detected");

  return result;
}

// ─── load_service_registry ────────────────────────────────────────────────
//
// Minimal JSON parser — handles the well-formed subset produced by t81-services.json.
// We do not pull in a JSON library dependency to keep the hosted test environment clean.

namespace {

std::string json_string_value(const std::string& text, const std::string& key) {
  const std::string needle = "\"" + key + "\":";
  auto pos = text.find(needle);
  if (pos == std::string::npos) return {};
  pos = text.find('"', pos + needle.size());
  if (pos == std::string::npos) return {};
  auto end = text.find('"', pos + 1);
  if (end == std::string::npos) return {};
  return text.substr(pos + 1, end - pos - 1);
}

uint32_t json_uint_value(const std::string& text, const std::string& key,
                          uint32_t default_val) {
  const std::string needle = "\"" + key + "\":";
  auto pos = text.find(needle);
  if (pos == std::string::npos) return default_val;
  pos += needle.size();
  while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) ++pos;
  if (pos >= text.size() || !std::isdigit(static_cast<unsigned char>(text[pos])))
    return default_val;
  uint32_t val = 0;
  while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])))
    val = val * 10 + (text[pos++] - '0');
  return val;
}

std::vector<std::string> json_string_array(const std::string& text, const std::string& key) {
  std::vector<std::string> result;
  const std::string needle = "\"" + key + "\":";
  auto pos = text.find(needle);
  if (pos == std::string::npos) return result;
  pos = text.find('[', pos + needle.size());
  if (pos == std::string::npos) return result;
  auto end = text.find(']', pos);
  if (end == std::string::npos) return result;
  const std::string arr = text.substr(pos + 1, end - pos - 1);
  std::istringstream ss(arr);
  std::string token;
  while (std::getline(ss, token, ',')) {
    auto a = token.find('"');
    auto b = token.rfind('"');
    if (a != std::string::npos && b != a)
      result.push_back(token.substr(a + 1, b - a - 1));
  }
  return result;
}

ActivationMode parse_activation(const std::string& s) {
  if (s == "on_demand") return ActivationMode::OnDemand;
  if (s == "manual")    return ActivationMode::Manual;
  return ActivationMode::Required;
}

}  // namespace

ServiceRegistry load_service_registry(const std::string& json_text) {
  ServiceRegistry reg;
  reg.version = json_uint_value(json_text, "version", 1);

  // Find each service object block: split on "id":" boundaries.
  const std::string id_needle = "\"id\":";
  std::size_t pos = 0;
  while ((pos = json_text.find(id_needle, pos)) != std::string::npos) {
    // Find the enclosing object by scanning backwards to '{'.
    std::size_t obj_start = json_text.rfind('{', pos);
    if (obj_start == std::string::npos) { ++pos; continue; }
    // Find the closing '}'.
    int depth = 0;
    std::size_t obj_end = obj_start;
    for (std::size_t i = obj_start; i < json_text.size(); ++i) {
      if (json_text[i] == '{') ++depth;
      else if (json_text[i] == '}') { --depth; if (depth == 0) { obj_end = i; break; } }
    }
    const std::string obj = json_text.substr(obj_start, obj_end - obj_start + 1);

    ServiceEntry e;
    e.id             = json_string_value(obj, "id");
    e.binary         = json_string_value(obj, "binary");
    e.canon_hash     = json_string_value(obj, "canon_hash");
    e.activation     = parse_activation(json_string_value(obj, "activation"));
    e.depends        = json_string_array(obj, "depends");
    e.capabilities   = json_string_array(obj, "capabilities");
    e.start_timeout_ms = json_uint_value(obj, "start_timeout_ms", 2000);
    e.restart_policy = json_string_value(obj, "restart_policy");
    if (e.restart_policy.empty()) e.restart_policy = "always";

    if (!e.id.empty())
      reg.services.push_back(std::move(e));

    pos = obj_end + 1;
  }
  return reg;
}

ServiceRegistry make_service_registry(std::vector<ServiceEntry> entries) {
  ServiceRegistry reg;
  reg.services = std::move(entries);
  return reg;
}

// ─── simulate_boot ────────────────────────────────────────────────────────────

BootResult simulate_boot(
    const ServiceRegistry& registry,
    const std::unordered_map<std::string, std::string>& binary_hashes) {
  BootResult result;

  const auto ordered = registry.required_order();  // throws on cycle
  uint64_t hash_accum = 0xcbf29ce484222325ULL;  // FNV-1a offset

  for (const auto* svc : ordered) {
    SpawnRecord sr;
    sr.service_id = svc->id;

    // 1. Binary integrity check (AC-3).
    if (binary_hashes.count(svc->id)) {
      const std::string& actual = binary_hashes.at(svc->id);
      sr.integrity_ok = (actual == svc->canon_hash);
    }

    // 2. Capability validation (AC-14).
    sr.capability_ok = true;
    for (const auto& cap : svc->capabilities) {
      if (!is_known_capability(cap)) {
        sr.capability_ok = false;
        break;
      }
    }

    // 3. Fire BootService Axion gate.
    AxionGateEvent ev;
    ev.op      = "BootService";
    ev.subject = svc->id;
    ev.payload = svc->canon_hash;
    if (!sr.integrity_ok) {
      ev.verdict    = GateVerdict::Deny;
      sr.verdict    = GateVerdict::Deny;
      sr.started    = false;
    } else if (!sr.capability_ok) {
      ev.verdict    = GateVerdict::Deny;
      sr.verdict    = GateVerdict::Deny;
      sr.started    = false;
    } else {
      ev.verdict = GateVerdict::Allow;
      sr.verdict = GateVerdict::Allow;
      sr.started = true;
    }
    result.gate_events.push_back(ev);
    result.spawn_log.push_back(sr);

    // Accumulate boot hash regardless of outcome.
    hash_accum = fnv1a(svc->id + svc->canon_hash) ^ (hash_accum * 1099511628211ULL);
  }

  result.boot_hash = hex64(hash_accum);
  result.success   = std::all_of(result.spawn_log.begin(), result.spawn_log.end(),
                                  [](const SpawnRecord& s) { return s.started; });
  return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Session manager
// ═══════════════════════════════════════════════════════════════════════════

std::string SessionRecord::canon_path() const {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "canonfs://var/sessions/%" PRIu64 ".json", session_id);
  return buf;
}

SessionEnv make_session_env(const SessionRecord& rec, const std::string& canon_root) {
  SessionEnv env;
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%016" PRIx64, rec.session_id);
  env.T81_SESSION_ID  = buf;
  env.T81_PRINCIPAL   = rec.principal_name;
  env.T81_TTY         = rec.tty_handle;
  std::snprintf(buf, sizeof(buf), "%" PRIu64, rec.start_epoch);
  env.T81_EPOCH       = buf;
  env.T81_CANON_ROOT  = canon_root;
  env.PATH            = "/bin:/usr/bin:/usr/local/bin";
  return env;
}

void SessionManager::load_principals(std::vector<PrincipalEntry> principals) {
  principals_ = std::move(principals);
}

namespace {

std::string session_record_to_json(const SessionRecord& rec) {
  const char* state_str =
    (rec.state == SessionState::Active)     ? "Active" :
    (rec.state == SessionState::Suspended)  ? "Suspended" : "Terminated";
  char buf[512];
  std::snprintf(buf, sizeof(buf),
    "{\"session_id\":%" PRIu64 ","
    "\"principal_id\":%" PRIu64 ","
    "\"principal\":\"%s\",\"tty\":\"%s\",\"start_epoch\":%" PRIu64 ","
    "\"state\":\"%s\",\"canon_hash\":\"%s\"}",
    rec.session_id, rec.principal_id,
    rec.principal_name.c_str(), rec.tty_handle.c_str(), rec.start_epoch,
    state_str, rec.canon_hash.c_str());
  return buf;
}

uint64_t fnv1a_session(const SessionRecord& rec) {
  const std::string data = std::to_string(rec.session_id)
                         + rec.principal_name
                         + rec.tty_handle
                         + std::to_string(rec.start_epoch);
  uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : data) { h ^= c; h *= 1099511628211ULL; }
  return h;
}

}  // namespace

void SessionManager::write_canon_record(const SessionRecord& rec) {
  canon_records_[rec.canon_path()] = session_record_to_json(rec);
}

LoginResult SessionManager::login(const std::string& principal_name,
                                   const std::string& credential_hash,
                                   const std::string& tty_handle,
                                   uint64_t           start_epoch) {
  LoginResult res;
  res.gate_event.op = "SessionCreate";

  // Credential check.
  const PrincipalEntry* principal = nullptr;
  for (const auto& p : principals_)
    if (p.name == principal_name) { principal = &p; break; }

  const bool skip_auth = (credential_hash == SessionManager::kAnonymousHash);
  if (!skip_auth) {
    if (!principal || principal->password_hash != credential_hash) {
      res.status         = LoginStatus::BadCredentials;
      res.gate_event.subject = principal_name;
      res.gate_event.verdict = GateVerdict::Deny;
      gate_events_.push_back(res.gate_event);
      return res;
    }
  }

  // Build session record.
  SessionRecord rec;
  rec.session_id    = next_session_id_++;
  rec.principal_id  = principal ? principal->id : 0;
  rec.principal_name = principal_name;
  rec.tty_handle    = tty_handle;
  rec.start_epoch   = start_epoch;
  rec.state         = SessionState::Active;
  char hash_buf[17];
  std::snprintf(hash_buf, sizeof(hash_buf), "%016" PRIx64, fnv1a_session(rec));
  rec.canon_hash = hash_buf;

  // Fire SessionCreate Axion gate.
  res.gate_event.subject = principal_name;
  res.gate_event.payload = tty_handle + "," + std::to_string(start_epoch);

  // Default policy: Allow (session manager can be extended to Deny here).
  res.gate_event.verdict = GateVerdict::Allow;
  gate_events_.push_back(res.gate_event);

  if (res.gate_event.verdict == GateVerdict::Deny) {
    res.status = LoginStatus::SessionDenied;
    return res;
  }

  sessions_[rec.session_id] = rec;
  write_canon_record(rec);

  res.status = LoginStatus::Success;
  res.record  = rec;
  return res;
}

LogoutResult SessionManager::logout(uint64_t session_id, uint32_t /*drain_timeout_ms*/) {
  LogoutResult res;
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) return res;

  SessionRecord& rec = it->second;
  rec.state = SessionState::Terminated;
  // Update CanonHash to cover final state.
  char hash_buf[17];
  std::snprintf(hash_buf, sizeof(hash_buf), "%016" PRIx64,
                fnv1a_session(rec) ^ 0xdeadbeefULL);
  rec.canon_hash = hash_buf;
  write_canon_record(rec);

  res.drained_cleanly = true;
  res.final_record    = rec;
  return res;
}

const SessionRecord* SessionManager::find_session(uint64_t session_id) const {
  auto it = sessions_.find(session_id);
  return (it != sessions_.end()) ? &it->second : nullptr;
}

// ─── activate_service ─────────────────────────────────────────────────────────

ServiceActivationResult activate_service(const ServiceRegistry& registry,
                                          const std::string&     service_id,
                                          uint64_t               requesting_session_id) {
  ServiceActivationResult res;
  res.gate_event.op      = "ServiceSpawn";
  res.gate_event.subject = service_id;
  res.gate_event.payload = std::to_string(requesting_session_id);

  const ServiceEntry* svc = registry.find(service_id);
  if (!svc) {
    res.rejection_reason   = "service not found: " + service_id;
    res.gate_event.verdict = GateVerdict::Deny;
    return res;
  }

  // Validate all capabilities (AC-14).
  for (const auto& cap : svc->capabilities) {
    if (!is_known_capability(cap)) {
      res.rejection_reason   = "unrecognised capability: " + cap;
      res.gate_event.verdict = GateVerdict::Deny;
      return res;
    }
  }

  res.gate_event.verdict = GateVerdict::Allow;
  res.success = true;
  return res;
}

// ═══════════════════════════════════════════════════════════════════════════
// t81sh
// ═══════════════════════════════════════════════════════════════════════════

// ─── History helpers ──────────────────────────────────────────────────────────

std::string history_entry_to_jsonl(const ShellHistoryEntry& e) {
  char buf[512];
  std::snprintf(buf, sizeof(buf),
    "{\"epoch\":%" PRIu64 ",\"cmd\":\"%s\",\"exit_code\":%d,\"duration_ms\":%u,\"axion_verdict\":\"%s\"}",
    e.epoch, e.cmd.c_str(), e.exit_code, e.duration_ms, e.axion_verdict.c_str());
  return buf;
}

// ─── T81Shell ─────────────────────────────────────────────────────────────────

T81Shell::T81Shell(SessionRecord session, uint32_t tier)
    : session_(std::move(session)), tier_(tier) {}

std::string T81Shell::prompt() const {
  // "[principal@session_id tier=N]$ "
  char buf[128];
  std::snprintf(buf, sizeof(buf), "[%s@%08" PRIx64 " tier=%u]$ ",
                session_.principal_name.c_str(), session_.session_id, tier_);
  return buf;
}

std::string T81Shell::history_canon_path() const {
  char buf[96];
  std::snprintf(buf, sizeof(buf),
                "canonfs://var/sessions/%" PRIu64 "/history.jsonl", session_.session_id);
  return buf;
}

// Builtins defined by RFC-00B9 §8.3.
static constexpr const char* kBuiltins[] = {
  "exit", "cd", "ls", "cat", "hash", "run", "compile",
  "policy", "service", "tier", "studio", "agent",
  "history", "env", "pwd",
};

bool T81Shell::is_builtin(const std::string& first_token) {
  for (const char* b : kBuiltins)
    if (first_token == b) return true;
  return false;
}

// T81Lang inputs: lines starting with keywords fn, let, var, if, for, agent,
// return, std, or a type name (i32, f81, bool, Tensor).
bool T81Shell::is_repl_input(const std::string& line) {
  static constexpr const char* kT81Keywords[] = {
    "fn ", "let ", "var ", "if ", "for ", "agent ", "return ",
    "std.", "i32 ", "f81 ", "bool ", "Tensor ",
  };
  for (const char* kw : kT81Keywords)
    if (line.size() >= std::strlen(kw) &&
        line.substr(0, std::strlen(kw)) == kw)
      return true;
  return false;
}

AxionGateEvent T81Shell::fire_shell_exec_gate(const std::string& cmd,
                                               const std::string& args) const {
  AxionGateEvent ev;
  ev.op      = "ShellExec";
  ev.subject = cmd;
  ev.payload = args + ",session=" + std::to_string(session_.session_id)
             + ",tier=" + std::to_string(tier_);

  // Check deny list.
  for (const auto& denied : denied_commands_)
    if (cmd == denied) { ev.verdict = GateVerdict::Deny; return ev; }

  ev.verdict = GateVerdict::Allow;
  return ev;
}

bool T81Shell::dispatch_builtin(const std::string& line) {
  // Parse first token.
  std::istringstream ss(line);
  std::string cmd;
  ss >> cmd;

  if (cmd == "exit") return true;
  if (cmd == "tier") {
    // `tier N` elevates/lowers the session tier.
    uint32_t new_tier = tier_;
    ss >> new_tier;
    tier_ = new_tier;
    return true;
  }
  if (cmd == "studio" || cmd == "agent") {
    // Simulate TTY raw-mode handoff (AC-12).
    auto ho = handoff_tty_to_tui(session_.tty_handle, cmd);
    (void)restore_tty(ho);
    return ho.transferred;
  }
  if (cmd == "compile" || cmd == "run" || cmd == "hash" ||
      cmd == "policy"  || cmd == "service" ||
      cmd == "cd" || cmd == "ls" || cmd == "cat" ||
      cmd == "history" || cmd == "env" || cmd == "pwd") {
    // All other builtins succeed in simulation.
    return true;
  }
  return false;
}

void T81Shell::record_history(const std::string& cmd, int exit_code,
                               GateVerdict verdict, uint64_t epoch,
                               uint32_t duration_ms) {
  const char* vstr = (verdict == GateVerdict::Allow) ? "Allow" :
                     (verdict == GateVerdict::Deny)  ? "Deny"  : "Warn";
  ShellHistoryEntry e{epoch, cmd, exit_code, duration_ms, vstr};
  history_.push_back(e);
  history_jsonl_.push_back(history_entry_to_jsonl(e));
}

bool T81Shell::exec_repl(const std::string& expr, uint64_t epoch) {
  // Minimal T81Lang REPL simulation.
  // Recognises `let NAME = VALUE` and binds it in repl_state (AC-15).
  std::istringstream ss(expr);
  std::string kw;
  ss >> kw;
  if (kw == "let" || kw == "var") {
    std::string name, eq, value;
    if (ss >> name >> eq >> value && eq == "=")
      repl_.bindings[name] = value;
  }
  repl_.exec_count++;
  record_history(expr, 0, GateVerdict::Allow, epoch);
  last_exit_code_ = 0;
  return true;
}

bool T81Shell::exec_command(const std::string& line, uint64_t epoch) {
  if (line.empty()) return true;

  const uint64_t cur_epoch = (epoch != 0) ? epoch : ++exec_epoch_;

  // T81Lang REPL path.
  if (is_repl_input(line))
    return exec_repl(line, cur_epoch);

  // Parse first token for builtin/gate decision.
  std::istringstream ss(line);
  std::string first_token;
  ss >> first_token;
  std::string rest;
  std::getline(ss, rest);

  if (is_builtin(first_token)) {
    const bool ok = dispatch_builtin(line);
    const int exit_code = ok ? 0 : 1;
    record_history(line, exit_code, GateVerdict::Allow, cur_epoch);
    last_exit_code_ = exit_code;
    return ok;
  }

  // Non-builtin: fire ShellExec Axion gate (AC-11).
  AxionGateEvent ev = fire_shell_exec_gate(first_token, rest);
  gate_events_.push_back(ev);

  if (ev.verdict == GateVerdict::Deny) {
    record_history(line, 1, GateVerdict::Deny, cur_epoch);
    last_exit_code_ = 1;
    return false;
  }

  // Simulated non-builtin succeeds.
  record_history(line, 0, ev.verdict, cur_epoch);
  last_exit_code_ = 0;
  return true;
}

void T81Shell::deny_commands(std::vector<std::string> denied) {
  denied_commands_ = std::move(denied);
}

// ─── TTY handoff (RFC-00B9 §10.2) ─────────────────────────────────────────────

TtyHandoff handoff_tty_to_tui(const std::string& tty_handle,
                                const std::string& tui_command) {
  TtyHandoff ho;
  ho.tty_handle  = tty_handle;
  ho.mode        = TtyMode::Raw;        // TIOCSRAW
  ho.transferred = (!tui_command.empty() && !tty_handle.empty());
  return ho;
}

TtyHandoff restore_tty(TtyHandoff handoff) {
  handoff.mode        = TtyMode::Cooked;  // TIOCSNORMAL
  handoff.transferred = false;
  return handoff;
}

}  // namespace t81::ternaryos::userenv
