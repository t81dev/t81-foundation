#include "tools/cli/core/mcp_server.hpp"

#include "crypto/ed25519_bundle.hpp"
#include "provider/provider_governor.hpp"
#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/config.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/vm/vm.hpp"
#include "vm/internal/tensor_helpers.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace t81::cli {

namespace {

namespace fs = std::filesystem;

struct ActiveTx {
  std::string tx_id;
  std::string agent_id;
  fs::path target_path;
  fs::path backup_path;
  fs::path staging_path;
  std::string before_sha3_512;
  bool isolated = false;
};

std::string json_escape_mcp(std::string_view input) {
  std::string out;
  out.reserve(input.size() + 8);
  for (char c : input) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

struct JsonNode {
  enum class Kind { Null, Bool, Number, String, Array, Object } kind = Kind::Null;
  std::string str;  // decoded string for String; raw token for Number/Bool/Null
  std::string raw;  // exact raw JSON slice (useful for preserving JSON-RPC id)
  std::unordered_map<std::string, JsonNode> obj;
  std::vector<JsonNode> arr;
};

class ScopedJsonParser {
public:
  explicit ScopedJsonParser(std::string_view src) : s_(src) {}

  bool parse(JsonNode& out) {
    skip_ws();
    if (!parse_val(out)) return false;
    skip_ws();
    return i_ == s_.size();
  }

private:
  void skip_ws() {
    while (i_ < s_.size() &&
           (s_[i_] == ' ' || s_[i_] == '\t' || s_[i_] == '\r' || s_[i_] == '\n')) {
      ++i_;
    }
  }

  bool parse_string(std::string& decoded, std::string& raw) {
    if (i_ >= s_.size() || s_[i_] != '"') return false;
    const size_t start = i_++;
    decoded.clear();
    while (i_ < s_.size()) {
      const char c = s_[i_++];
      if (c == '"') {
        raw.assign(s_.substr(start, i_ - start));
        return true;
      }
      if (c == '\\') {
        if (i_ >= s_.size()) return false;
        const char e = s_[i_++];
        switch (e) {
          case '"':
            decoded.push_back('"');
            break;
          case '\\':
            decoded.push_back('\\');
            break;
          case '/':
            decoded.push_back('/');
            break;
          case 'b':
            decoded.push_back('\b');
            break;
          case 'f':
            decoded.push_back('\f');
            break;
          case 'n':
            decoded.push_back('\n');
            break;
          case 'r':
            decoded.push_back('\r');
            break;
          case 't':
            decoded.push_back('\t');
            break;
          default:
            decoded.push_back(e);
            break;
        }
      } else {
        decoded.push_back(c);
      }
    }
    return false;
  }

  bool parse_val(JsonNode& out, int depth = 0) {
    static constexpr int kMaxDepth = 64;
    if (depth > kMaxDepth) return false;
    skip_ws();
    if (i_ >= s_.size()) return false;
    const char c = s_[i_];
    if (c == '"') {
      out.kind = JsonNode::Kind::String;
      return parse_string(out.str, out.raw);
    }
    if (c == '{') {
      out.kind = JsonNode::Kind::Object;
      const size_t start = i_++;
      skip_ws();
      if (i_ < s_.size() && s_[i_] == '}') {
        ++i_;
        out.raw.assign(s_.substr(start, i_ - start));
        return true;
      }
      while (true) {
        skip_ws();
        std::string k, kraw;
        if (!parse_string(k, kraw)) return false;
        skip_ws();
        if (i_ >= s_.size() || s_[i_] != ':') return false;
        ++i_;
        JsonNode val;
        if (!parse_val(val, depth + 1)) return false;
        out.obj[std::move(k)] = std::move(val);
        skip_ws();
        if (i_ < s_.size() && s_[i_] == ',') {
          ++i_;
          continue;
        }
        if (i_ < s_.size() && s_[i_] == '}') {
          ++i_;
          out.raw.assign(s_.substr(start, i_ - start));
          return true;
        }
        return false;
      }
    }
    if (c == '[') {
      out.kind = JsonNode::Kind::Array;
      const size_t start = i_++;
      skip_ws();
      if (i_ < s_.size() && s_[i_] == ']') {
        ++i_;
        out.raw.assign(s_.substr(start, i_ - start));
        return true;
      }
      while (true) {
        JsonNode elem;
        if (!parse_val(elem, depth + 1)) return false;
        out.arr.push_back(std::move(elem));
        skip_ws();
        if (i_ < s_.size() && s_[i_] == ',') {
          ++i_;
          continue;
        }
        if (i_ < s_.size() && s_[i_] == ']') {
          ++i_;
          out.raw.assign(s_.substr(start, i_ - start));
          return true;
        }
        return false;
      }
    }
    const size_t start = i_;
    while (i_ < s_.size() && s_[i_] != ',' && s_[i_] != '}' && s_[i_] != ']' && s_[i_] != ' ' &&
           s_[i_] != '\t' && s_[i_] != '\r' && s_[i_] != '\n') {
      ++i_;
    }
    if (i_ == start) return false;
    out.raw.assign(s_.substr(start, i_ - start));
    out.str = out.raw;
    if (out.raw == "null") {
      out.kind = JsonNode::Kind::Null;
    } else if (out.raw == "true" || out.raw == "false") {
      out.kind = JsonNode::Kind::Bool;
    } else {
      out.kind = JsonNode::Kind::Number;
    }
    return true;
  }

  std::string_view s_;
  size_t i_ = 0;
};

std::string arg_str(const JsonNode& obj, const std::string& key) {
  if (obj.kind != JsonNode::Kind::Object) return {};
  auto it = obj.obj.find(key);
  if (it == obj.obj.end() || it->second.kind != JsonNode::Kind::String) return {};
  return it->second.str;
}

bool arg_bool(const JsonNode& obj, const std::string& key, bool default_val = false) {
  if (obj.kind != JsonNode::Kind::Object) return default_val;
  auto it = obj.obj.find(key);
  if (it == obj.obj.end() || it->second.kind != JsonNode::Kind::Bool) return default_val;
  return it->second.str == "true";
}

bool arg_int64_strict(const JsonNode& obj, const std::string& key, int64_t& out,
                      int64_t default_val = 0) {
  out = default_val;
  if (obj.kind != JsonNode::Kind::Object) return true;
  auto it = obj.obj.find(key);
  if (it == obj.obj.end()) return true;
  if (it->second.kind != JsonNode::Kind::Number) return false;
  try {
    std::size_t pos = 0;
    const long long parsed = std::stoll(it->second.raw, &pos, 10);
    if (pos != it->second.raw.size()) return false;
    out = static_cast<int64_t>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

bool env_compact_mode() {
  const char* v = std::getenv("T81_MCP_COMPACT");
  return v != nullptr && (std::strcmp(v, "1") == 0 || std::strcmp(v, "true") == 0);
}

const char* trap_name_str(t81::vm::Trap t) {
  using t81::vm::Trap;
  switch (t) {
    case Trap::None:
      return "None";
    case Trap::DecodeFault:
      return "DecodeFault";
    case Trap::TypeFault:
      return "TypeFault";
    case Trap::BoundsFault:
      return "BoundsFault";
    case Trap::StackFault:
      return "StackFault";
    case Trap::DivisionFault:
      return "DivisionFault";
    case Trap::SecurityFault:
      return "SecurityFault";
    case Trap::TierFault:
      return "TierFault";
    case Trap::ShapeFault:
      return "ShapeFault";
    case Trap::TrapInstruction:
      return "TrapInstruction";
    case Trap::Unimplemented:
      return "Unimplemented";
    case Trap::AssertionFailed:
      return "AssertionFailed";
    case Trap::EthicsViolation:
      return "EthicsViolation";
    case Trap::CapabilityDenied:
      return "CapabilityDenied";
    case Trap::ActivationFault:
      return "ActivationFault";
    case Trap::FFIPolicyDenied:
      return "FFIPolicyDenied";
    default:
      return "UnknownTrap";
  }
}

bool arg_double(const JsonNode& obj, const std::string& key, double default_val, double& out) {
  out = default_val;
  if (obj.kind != JsonNode::Kind::Object) return true;
  auto it = obj.obj.find(key);
  if (it == obj.obj.end() || it->second.kind == JsonNode::Kind::Null) return true;
  if (it->second.kind != JsonNode::Kind::Number && it->second.kind != JsonNode::Kind::String) {
    return false;
  }
  const std::string& s = it->second.str;
  if (s.empty() || std::isspace(static_cast<unsigned char>(s.front()))) return false;
  errno = 0;
  char* endp = nullptr;
  const double v = std::strtod(s.c_str(), &endp);
  if (errno == ERANGE || endp == s.c_str() || *endp != '\0') return false;
  out = v;
  return true;
}

bool arg_u64(const JsonNode& obj, const std::string& key, uint64_t default_val, uint64_t& out) {
  out = default_val;
  if (obj.kind != JsonNode::Kind::Object) return true;
  auto it = obj.obj.find(key);
  if (it == obj.obj.end() || it->second.kind == JsonNode::Kind::Null) return true;
  if (it->second.kind != JsonNode::Kind::Number && it->second.kind != JsonNode::Kind::String) {
    return false;
  }
  const std::string& s = it->second.str;
  if (s.empty() || !std::isdigit(static_cast<unsigned char>(s.front()))) return false;
  errno = 0;
  char* endp = nullptr;
  const unsigned long long v = std::strtoull(s.c_str(), &endp, 10);
  if (errno == ERANGE || endp == s.c_str() || *endp != '\0') return false;
  out = static_cast<uint64_t>(v);
  return true;
}

struct PathJail {
  std::vector<fs::path> roots;
  mutable std::string last_error;
  static constexpr std::uintmax_t kMaxReadBytes = 64ull * 1024ull * 1024ull;

  bool within(const fs::path& canonical) const {
    const fs::path canon = canonical.lexically_normal();
    for (const auto& root : roots) {
      if (canon == root) return true;
      const fs::path rel = canon.lexically_relative(root);
      if (rel.empty() || rel.is_absolute()) continue;
      bool escapes = false;
      for (const auto& part : rel) {
        if (part == "..") {
          escapes = true;
          break;
        }
      }
      if (!escapes) return true;
    }
    return false;
  }

  // Relative paths resolve under the first root. Absolute paths must land inside any root
  // after symlink and ".." canonicalization.
  bool resolve(const fs::path& input, fs::path& canonical) const {
    last_error.clear();
    if (input.empty()) {
      last_error = "empty path";
      return false;
    }
    const auto native = input.native();
    if (std::find(native.begin(), native.end(), fs::path::value_type{}) != native.end()) {
      last_error = "could not resolve path";
      return false;
    }
    std::error_code ec;
    fs::path abs = input;
    if (abs.is_relative()) {
      if (roots.empty()) {
        last_error = "MCP filesystem root is not configured";
        return false;
      }
      abs = roots.front() / abs;
    }
    const fs::path canon = fs::weakly_canonical(abs, ec);
    if (ec) {
      last_error = "could not resolve path";
      return false;
    }
    if (!within(canon)) {
      last_error = "path is outside the MCP filesystem root";
      return false;
    }
    canonical = canon;
    return true;
  }

  bool resolve(const std::string& input, fs::path& canonical) const {
    if (input.find('\0') != std::string::npos) {
      last_error = "could not resolve path";
      return false;
    }
    return resolve(fs::path(input), canonical);
  }
};

bool write_file_bytes_atomic(const PathJail& jail, const fs::path& path,
                             const std::vector<uint8_t>& data) {
  fs::path canonical;
  if (!jail.resolve(path, canonical)) return false;
  fs::path tmp = canonical;
  tmp += ".tmp.t81";
  std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
  if (!ofs) {
    jail.last_error = "could not write file";
    return false;
  }
  if (!data.empty()) {
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
  }
  ofs.flush();
  const bool ok = static_cast<bool>(ofs);
  ofs.close();
  if (!ok) {
    std::error_code ec;
    fs::remove(tmp, ec);
    jail.last_error = "could not write file";
    return false;
  }
  std::error_code ec;
  fs::rename(tmp, canonical, ec);
  if (ec) {
    fs::remove(tmp, ec);
    jail.last_error = "could not write file";
    return false;
  }
  return true;
}

std::string sha3_512_hex(const std::vector<uint8_t>& bytes) {
  const auto digest = t81::crypto::sha3_512(bytes);
  static const char* kHex = "0123456789abcdef";
  std::string hex;
  hex.reserve(digest.size() * 2);
  for (uint8_t b : digest) {
    hex.push_back(kHex[(b >> 4) & 0xF]);
    hex.push_back(kHex[b & 0xF]);
  }
  return "sha3-512:" + hex;
}

bool read_file_bytes(const PathJail& jail, const fs::path& path, std::vector<uint8_t>& out) {
  fs::path canonical;
  if (!jail.resolve(path, canonical)) return false;
  std::ifstream ifs(canonical, std::ios::binary);
  if (!ifs) {
    jail.last_error = "could not open file";
    return false;
  }
  out.clear();
  char buf[8192];
  while (ifs) {
    ifs.read(buf, sizeof(buf));
    const auto n = static_cast<std::size_t>(ifs.gcount());
    if (n == 0) break;
    if (out.size() + n > PathJail::kMaxReadBytes) {
      out.clear();
      jail.last_error = "file exceeds MCP read limit";
      return false;
    }
    out.insert(out.end(), buf, buf + n);
  }
  if (ifs.bad()) {
    out.clear();
    jail.last_error = "could not open file";
    return false;
  }
  return true;
}

bool write_file_bytes(const PathJail& jail, const fs::path& path,
                      const std::vector<uint8_t>& data) {
  fs::path canonical;
  if (!jail.resolve(path, canonical)) return false;
  std::ofstream ofs(canonical, std::ios::binary | std::ios::trunc);
  if (!ofs) {
    jail.last_error = "could not write file";
    return false;
  }
  if (!data.empty()) {
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
  }
  if (!ofs.good()) {
    jail.last_error = "could not write file";
    return false;
  }
  return true;
}

bool path_text_has_nul(const std::string& path) { return path.find('\0') != std::string::npos; }

bool read_file_bytes(const PathJail& jail, const std::string& path, std::vector<uint8_t>& out) {
  if (path_text_has_nul(path)) {
    jail.last_error = "could not resolve path";
    return false;
  }
  return read_file_bytes(jail, fs::path(path), out);
}

bool write_file_bytes_atomic(const PathJail& jail, const std::string& path,
                             const std::vector<uint8_t>& data) {
  if (path_text_has_nul(path)) {
    jail.last_error = "could not resolve path";
    return false;
  }
  return write_file_bytes_atomic(jail, fs::path(path), data);
}

std::string io_failure_json(const PathJail& jail, std::string_view fallback) {
  const std::string& err = jail.last_error.empty() ? std::string(fallback) : jail.last_error;
  return "{\"ok\":false,\"error\":\"" + json_escape_mcp(err) + "\"}";
}

bool policy_admits_hash(const t81::axion::Policy& policy, const std::string& hash) {
  for (const auto& h : policy.allowed_tensor_hashes) {
    if (h == hash) return true;
  }
  for (const auto& h : policy.allowed_ternary_model_hashes) {
    if (h == hash) return true;
  }
  return false;
}

size_t policy_allowed_hash_count(const t81::axion::Policy& policy) {
  return policy.allowed_tensor_hashes.size() + policy.allowed_ternary_model_hashes.size();
}

int mode_rank(const std::string& m) {
  if (m == "observe-only") return 0;
  if (m == "warn") return 1;
  if (m == "fail-closed") return 2;
  return -1;
}

bool action_type_token(std::string_view text) {
  if (text.empty() || text.size() > 128) return false;
  for (unsigned char c : text) {
    if (std::isalnum(c) || c == '_' || c == '.' || c == ':' || c == '-') continue;
    return false;
  }
  return true;
}

void skip_policy_ws(std::string_view text, size_t& index) {
  while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index]))) {
    ++index;
  }
}

bool consume_literal(std::string_view text, size_t& index, std::string_view literal) {
  if (text.substr(index).starts_with(literal)) {
    index += literal.size();
    return true;
  }
  return false;
}

// First structured `(require-axion-event (reason "<prefix><mode>"))` clause wins.
// A bare mention of `enforcement-mode:observe-only` in a comment or another string does not.
std::optional<std::string> match_reason_mode(std::string_view text,
                                             std::string_view reason_prefix) {
  static constexpr std::string_view kHead = "(require-axion-event";
  static constexpr std::string_view kModes[] = {"observe-only", "fail-closed", "warn"};
  size_t search = 0;
  while (search < text.size()) {
    const size_t found = text.find(kHead, search);
    if (found == std::string_view::npos) return std::nullopt;
    size_t index = found + kHead.size();
    skip_policy_ws(text, index);
    if (!consume_literal(text, index, "(reason")) {
      search = found + 1;
      continue;
    }
    skip_policy_ws(text, index);
    if (!consume_literal(text, index, "\"")) {
      search = found + 1;
      continue;
    }
    if (!consume_literal(text, index, reason_prefix)) {
      search = found + 1;
      continue;
    }
    std::optional<std::string> mode;
    for (const std::string_view candidate : kModes) {
      if (text.substr(index).starts_with(candidate) && index + candidate.size() < text.size() &&
          text[index + candidate.size()] == '"') {
        mode = std::string(candidate);
        index += candidate.size();
        break;
      }
    }
    if (!mode) {
      search = found + 1;
      continue;
    }
    if (!consume_literal(text, index, "\"")) {
      search = found + 1;
      continue;
    }
    skip_policy_ws(text, index);
    if (!consume_literal(text, index, ")")) {
      search = found + 1;
      continue;
    }
    skip_policy_ws(text, index);
    if (!consume_literal(text, index, ")")) {
      search = found + 1;
      continue;
    }
    return mode;
  }
  return std::nullopt;
}

std::string resolve_floor(const std::string& policy_text, const std::string& action_type) {
  if (action_type_token(action_type)) {
    const std::string prefix = "action-policy:" + action_type + "=";
    if (const auto mode = match_reason_mode(policy_text, prefix)) return *mode;
  }
  if (const auto mode = match_reason_mode(policy_text, "enforcement-mode:")) return *mode;
  return "fail-closed";
}

std::string trim_ws(std::string s) {
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
  return s;
}

std::string read_git_head_sha(const PathJail& jail, const fs::path& repo_root,
                              std::string& ref_out) {
  fs::path head_path;
  if (!jail.resolve(repo_root / ".git" / "HEAD", head_path)) return "unknown";
  std::ifstream ifs(head_path);
  if (!ifs) return "unknown";
  std::string line;
  std::getline(ifs, line);
  line = trim_ws(line);
  if (line.rfind("ref: ", 0) == 0) {
    ref_out = line.substr(5);
    fs::path ref_path;
    if (!jail.resolve(repo_root / ".git" / ref_out, ref_path)) return "ref-unpacked";
    std::ifstream rfs(ref_path);
    if (rfs) {
      std::string sha;
      std::getline(rfs, sha);
      return trim_ws(sha);
    }
    return "ref-unpacked";
  }
  ref_out = "detached";
  return line;
}

std::string build_tools_list_json() {
  return "["
         "{\"name\":\"t81.policy_validate\","
         "\"description\":\"Validate an Axion .apl policy in-process.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"policy_path\"],"
         "\"properties\":{\"policy_path\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.policy_test\","
         "\"description\":\"Evaluate a SHA3-512 hash against an Axion .apl policy in-process.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"policy_path\",\"model_hash\"],"
         "\"properties\":{\"policy_path\":{\"type\":\"string\"},\"model_hash\":{\"type\":"
         "\"string\"}}}},"

         "{\"name\":\"t81.get_effective_policy\","
         "\"description\":\"Introspect authoritative .apl policy floor and SHA3-512 digest "
         "in-process.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"policy_path\"],"
         "\"properties\":{\"policy_path\":{\"type\":\"string\"},\"action_type\":{\"type\":"
         "\"string\"}}}},"

         "{\"name\":\"t81.gate_check\","
         "\"description\":\"Evaluate artifact admission with anti-downgrade per-action enforcement "
         "floor in-process.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"artifact_path\",\"policy_path\"],"
         "\"properties\":{\"artifact_path\":{\"type\":\"string\"},\"policy_path\":{\"type\":"
         "\"string\"},"
         "\"action_type\":{\"type\":\"string\"},\"mode\":{\"type\":\"string\",\"enum\":[\"observe-"
         "only\",\"warn\",\"fail-closed\"]}}}},"

         "{\"name\":\"t81.canonfs_fsck\","
         "\"description\":\"Verify CanonFS object hashes and snapshots in-process.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"canonfs_root\"],"
         "\"properties\":{\"canonfs_root\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.determinism_hash\","
         "\"description\":\"Compute SHA3-512 digest of a file in-process.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"path\"],"
         "\"properties\":{\"path\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.session_bootstrap\","
         "\"description\":\"Return a compact, live ground-truth workspace packet (git HEAD, "
         "PUBLIC_API_LOCK digest, registry presence) in one call.\","
         "\"inputSchema\":{\"type\":\"object\","
         "\"properties\":{\"repo_root\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.tx_begin\","
         "\"description\":\"Begin an atomic file transaction by sealing a SHA3-512 verified backup "
         "before mutation (with optional per-agent isolated staging).\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"target_path\",\"backup_dir\"],"
         "\"properties\":{\"target_path\":{\"type\":\"string\"},\"backup_dir\":{\"type\":"
         "\"string\"},\"agent_id\":{\"type\":\"string\"},\"isolated\":{\"type\":\"boolean\"}}}},"

         "{\"name\":\"t81.tx_commit\","
         "\"description\":\"Commit an active file transaction after required .apl policy, "
         "concurrency, capability-budget, optional CanonFS SBDR1T2D, and T81VM subgraph checks.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"tx_id\",\"policy_path\"],"
         "\"properties\":{\"tx_id\":{\"type\":\"string\"},\"policy_path\":{\"type\":\"string\"},"
         "\"agent_id\":{\"type\":\"string\"},\"expected_parent_sha3_512\":{\"type\":\"string\"},"
         "\"canonfs_root\":{\"type\":\"string\"},\"sbdr_path\":{\"type\":\"string\"},"
         "\"bytecode_hex\":{\"type\":\"string\"},"
         "\"max_abs_err\":{\"type\":\"integer\"},\"dualrail_fallback_count\":{\"type\":\"integer\"}"
         ","
         "\"repeat_sha3_identical\":{\"type\":\"boolean\"},"
         "\"auto_abort_on_deny\":{\"type\":\"boolean\"}}}},"

         "{\"name\":\"t81.tx_abort\","
         "\"description\":\"Abort an active file transaction and restore the exact pre-mutation "
         "bytes verified by SHA3-512.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"tx_id\"],"
         "\"properties\":{\"tx_id\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.verify_subgraph\","
         "\"description\":\"Statically verify a narrowed TISC tensor+policy subgraph "
         "(WeightsLoad->ChkShape->AxVerify->TMatMul->Halt) before execution and return its "
         "SHA3-512 digest and recommended Dual-Rail backend.\","
         "\"inputSchema\":{\"type\":\"object\","
         "\"properties\":{\"bytecode_path\":{\"type\":\"string\"},\"bytecode_hex\":{\"type\":"
         "\"string\"}}}},"

         "{\"name\":\"t81.run_subgraph\","
         "\"description\":\"Statically verify and execute a narrowed TISC subgraph inside T81VM "
         "under Axion policy enforcement, returning a canonical SHA3-512 execution receipt.\","
         "\"inputSchema\":{\"type\":\"object\","
         "\"properties\":{\"bytecode_path\":{\"type\":\"string\"},\"bytecode_hex\":{\"type\":"
         "\"string\"},\"policy_path\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.diff_check\","
         "\"description\":\"Audit exact Hamming weight-flip distance between two Dual-Rail (P,N) "
         "bitplane checkpoints against a fail-closed max_flip_pct budget.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"base_pos_path\",\"base_neg_path\","
         "\"candidate_pos_path\",\"candidate_neg_path\"],"
         "\"properties\":{\"base_pos_path\":{\"type\":\"string\"},\"base_neg_path\":{\"type\":"
         "\"string\"},\"candidate_pos_path\":{\"type\":\"string\"},\"candidate_neg_path\":{"
         "\"type\":\"string\"},\"max_flip_pct\":{\"type\":\"number\"}}}},"

         "{\"name\":\"t81.memory_search\","
         "\"description\":\"Zero-float deterministic top-K similarity search over a Dual-Rail "
         "(P,N) agent memory index, returning exact integer scores and a SHA3-512 retrieval "
         "receipt.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"index_pos_path\","
         "\"index_neg_path\",\"query_pos_path\",\"query_neg_path\",\"dim\"],"
         "\"properties\":{\"index_pos_path\":{\"type\":\"string\"},\"index_neg_path\":{\"type\":"
         "\"string\"},\"query_pos_path\":{\"type\":\"string\"},\"query_neg_path\":{\"type\":"
         "\"string\"},\"dim\":{\"type\":\"integer\"},\"top_k\":{\"type\":\"integer\"}}}},"

         "{\"name\":\"t81.adapter_apply\","
         "\"description\":\"Compose a Dual-Rail ternary adapter delta (dense pos/neg or compact "
         "SBDR1 sparse-block wire format) onto base weights via zero-dequantization saturated "
         "addition clamp(W + delta, -1, +1) with a fail-closed max_flip_pct gate and SHA3-512 "
         "adapter receipt.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"base_pos_path\",\"base_neg_path\","
         "\"out_pos_path\",\"out_neg_path\"],"
         "\"properties\":{\"base_pos_path\":{\"type\":\"string\"},\"base_neg_path\":{\"type\":"
         "\"string\"},\"delta_pos_path\":{\"type\":\"string\"},\"delta_neg_path\":{\"type\":"
         "\"string\"},\"delta_sbdr_path\":{\"type\":\"string\"},\"out_pos_path\":{\"type\":"
         "\"string\"},\"out_neg_path\":{\"type\":\"string\"},\"max_flip_pct\":{\"type\":"
         "\"number\"}}}}"
         "]";
}

std::string detect_host_capabilities_json() {
#if defined(__aarch64__) && defined(__ARM_NEON)
  const char* cpu_simd = "arm64_neon";
#elif defined(__x86_64__) && defined(__AVX2__)
  const char* cpu_simd = "x86_64_avx2";
#else
  const char* cpu_simd = "scalar";
#endif
#if defined(__APPLE__) && defined(__aarch64__)
  const char* gpu_backend = "metal_unified_memory";
#else
  const char* gpu_backend = "none";
#endif
  return std::string("{\"cpu_simd\":\"") + cpu_simd + "\",\"gpu_backend\":\"" + gpu_backend +
         "\",\"dualrail_layout\":\"DualRail_PN_64\",\"bytes_per_trit\":0.25,"
         "\"bit_exact_cross_device_sha3_512\":true,"
         "\"dualrail_crossover\":{\"min_k\":64,\"min_mn\":64,\"reuse_k_star\":4,"
         "\"gpu_fused_min_matmuls\":4},"
         "\"regime_routing\":{"
         "\"R1_ternary_act\":{\"dense\":\"DualRail_PN_64\",\"sparse_min_pct\":50,"
         "\"sparse_kernel\":\"SBDR128_SIMD\",\"gpu_kernel\":\"GPU_R1_DualRail_Pop\","
         "\"gpu_sparse_kernel\":\"GPU_R1_BitmaskSkip_Pop\",\"gpu_sparse_min_pct\":75,"
         "\"m1_high_sparsity_min_pct\":90,\"m1_high_sparsity_kernel\":\"CPU_1T_SBDR128_SIMD\","
         "\"m1_low_sparsity_max_pct\":50,\"gpu_min_batch\":4,"
         "\"k_star_prepacked\":0,\"k_star_simd4t_vs_gpu_int4\":2,"
         "\"k_star_simd4t_vs_cpu_int4\":3,\"avoid_per_layer_cpu_thread_barrier\":true},"
         "\"R2_int8_act\":{\"m1_to_4\":\"CPU_4T_INT4W\",\"m16_plus\":\"CPU_4T_INT8W\","
         "\"avoid_cpu_dualrail_unpack\":true}}}";
}

bool decode_hex_bytes(std::string_view hex, std::vector<uint8_t>& out) {
  if (hex.size() % 2 != 0) return false;
  out.clear();
  out.reserve(hex.size() / 2);
  auto nibble = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  for (size_t i = 0; i < hex.size(); i += 2) {
    int hi = nibble(hex[i]);
    int lo = nibble(hex[i + 1]);
    if (hi < 0 || lo < 0) return false;
    out.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return true;
}

struct SubgraphVerifyResult {
  bool ok = false;
  std::string verdict;
  size_t matmul_count = 0;
};

SubgraphVerifyResult verify_tisc_subgraph(const t81::tisc::Program& prog) {
  using t81::tisc::Opcode;
  if (prog.insns.empty()) return {false, "REJECT_EMPTY_PROGRAM", 0};
  std::unordered_set<int32_t> loaded, shaped, verified;
  bool saw_halt = false;
  size_t matmuls = 0;
  size_t pure_compute_ops = 0;
  for (const auto& insn : prog.insns) {
    if (saw_halt) return {false, "REJECT_TRAILING_CODE_AFTER_HALT", matmuls};
    switch (insn.opcode) {
      case Opcode::Nop:
        break;
      case Opcode::LoadImm:
      case Opcode::Add:
      case Opcode::Sub:
      case Opcode::Mul:
      case Opcode::TNew:
        ++pure_compute_ops;
        break;
      case Opcode::WeightsLoad:
        loaded.insert(insn.a);
        break;
      case Opcode::ChkShape:
        if (!loaded.contains(insn.a)) return {false, "REJECT_SHAPE_BEFORE_LOAD", matmuls};
        shaped.insert(insn.a);
        break;
      case Opcode::AxVerify:
        if (!loaded.contains(insn.b)) return {false, "REJECT_POLICY_BEFORE_LOAD", matmuls};
        verified.insert(insn.b);
        break;
      case Opcode::TMatMul:
        if (!loaded.contains(insn.c)) return {false, "REJECT_UNLOADED_WEIGHT", matmuls};
        if (!shaped.contains(insn.c)) return {false, "REJECT_UNCHECKED_SHAPE", matmuls};
        if (!verified.contains(insn.c)) return {false, "REJECT_UNVERIFIED_POLICY", matmuls};
        ++matmuls;
        break;
      case Opcode::Halt:
        saw_halt = true;
        break;
      default:
        return {false, "REJECT_DISALLOWED_OPCODE", matmuls};
    }
  }
  if (!saw_halt) return {false, "REJECT_MISSING_HALT", matmuls};
  if (matmuls == 0 && pure_compute_ops == 0) return {false, "REJECT_NO_COMPUTE", 0};
  return {true, "VERIFIED_OK", matmuls};
}

}  // namespace

int run_mcp_serve(const std::vector<std::string>& command_args,
                  const std::function<int(const std::string&)>& run_canonfs_fsck_json) {
  if (command_args.empty() || command_args[0] != "serve") {
    std::cerr << "Usage: t81 mcp serve [--root PATH]...\n"
              << "Run 't81 help mcp' to list all 15 native MCP tools.\n";
    return 1;
  }

  std::vector<fs::path> root_args;
  for (size_t i = 1; i < command_args.size(); ++i) {
    const std::string& arg = command_args[i];
    if (arg == "--root") {
      if (i + 1 >= command_args.size() || command_args[i + 1].empty()) {
        std::cerr << "t81 mcp serve: --root requires a path\n";
        return 1;
      }
      root_args.emplace_back(command_args[++i]);
    } else if (arg.rfind("--root=", 0) == 0) {
      const std::string value = arg.substr(std::string("--root=").size());
      if (value.empty()) {
        std::cerr << "t81 mcp serve: --root requires a path\n";
        return 1;
      }
      root_args.emplace_back(value);
    } else {
      std::cerr << "t81 mcp serve: unknown argument '" << arg << "'\n"
                << "Usage: t81 mcp serve [--root PATH]...\n";
      return 1;
    }
  }
  if (root_args.empty()) {
    std::error_code cwd_ec;
    const fs::path cwd = fs::current_path(cwd_ec);
    if (cwd_ec) {
      std::cerr << "t81 mcp serve: could not resolve the working directory\n";
      return 1;
    }
    root_args.push_back(cwd);
  }

  PathJail jail;
  jail.roots.reserve(root_args.size());
  for (const fs::path& root_arg : root_args) {
    std::error_code ec;
    const fs::path canon = fs::weakly_canonical(root_arg, ec);
    std::error_code dir_ec;
    if (ec || !fs::is_directory(canon, dir_ec)) {
      std::cerr << "t81 mcp serve: --root is not a directory: " << root_arg.string() << "\n";
      return 1;
    }
    jail.roots.push_back(canon.lexically_normal());
  }

  std::unordered_map<std::string, ActiveTx> active_txs;
  std::unordered_map<std::string, std::string> last_commit_receipts;
  uint64_t tx_counter = 0;

  // M2 Persistent MCP Receipt Ledger: per-session native Ed25519 keypair + lazy CanonFS store.
  const t81::crypto::Ed25519KeyPair session_kp = t81::crypto::ed25519_generate_keypair();
  const std::string session_pub_pem = t81::crypto::export_public_key_pem(session_kp.public_key);
  std::shared_ptr<t81::canonfs::Driver> receipt_canonfs;
  auto ensure_receipt_canonfs =
      [&](const fs::path& explicit_root) -> std::shared_ptr<t81::canonfs::Driver> {
    if (!explicit_root.empty()) {
      auto d = std::shared_ptr<t81::canonfs::Driver>(
          t81::canonfs::make_persistent_driver(explicit_root));
      if (d) return d;
      return nullptr;
    }
    if (!receipt_canonfs) {
      const fs::path ledger_dir = jail.roots.front() / ".t81_receipt_ledger";
      receipt_canonfs =
          std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_persistent_driver(ledger_dir));
    }
    return receipt_canonfs;
  };
  auto persist_and_sign_receipt =
      [&](const fs::path& explicit_canonfs_root, const std::string& meaning_json,
          const std::string& evidence_json, const std::string& authority_json,
          std::string& out_ref_hex, std::string& out_sig_b64, std::string& out_err_code,
          std::string& out_err_msg) -> bool {
    const std::string payload =
        t81::crypto::rfc00d4_canonical_signing_payload(meaning_json, evidence_json, authority_json);
    out_sig_b64 =
        t81::crypto::sign_rfc00d4_payload_b64(session_kp.seed, session_kp.public_key, payload);
    if (!t81::crypto::verify_rfc00d4_payload_b64(session_kp.public_key, payload, out_sig_b64)) {
      out_err_code = "ERR_RECEIPT_VERIFY_FAILED";
      out_err_msg = "native Ed25519 self-verification failed for sealed MCP receipt";
      return false;
    }
    auto drv = ensure_receipt_canonfs(explicit_canonfs_root);
    if (!drv) {
      out_err_code = "ERR_EVIDENCE_PERSIST_FAILED";
      out_err_msg = "failed to initialize CanonFS persistent receipt ledger";
      return false;
    }
    const std::string sealed = "{\"schema\":\"t81.provider.evidence.v1\",\"payload\":" + payload +
                               ",\"ed25519_signature_b64\":\"" + out_sig_b64 + "\"}";
    std::vector<std::byte> raw_bytes(sealed.size());
    std::memcpy(raw_bytes.data(), sealed.data(), sealed.size());
    auto wres = drv->write_object(t81::canonfs::ObjectType::RawBlock, raw_bytes);
    if (!wres.has_value()) {
      out_err_code = "ERR_EVIDENCE_PERSIST_FAILED";
      out_err_msg = "failed to persist sealed receipt into CanonFS";
      return false;
    }
    auto rres = drv->read_object_bytes(wres.value());
    if (!rres.has_value() || rres.value().size() != raw_bytes.size() ||
        std::memcmp(rres.value().data(), raw_bytes.data(), raw_bytes.size()) != 0) {
      out_err_code = "ERR_EVIDENCE_PERSIST_FAILED";
      out_err_msg = "CanonFS read-back verification failed for sealed receipt";
      return false;
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : wres.value().hash.h.bytes) oss << std::setw(2) << static_cast<unsigned>(b);
    out_ref_hex = oss.str();
    return true;
  };

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    JsonNode req;
    ScopedJsonParser parser(line);
    if (!parser.parse(req) || req.kind != JsonNode::Kind::Object) {
      continue;
    }
    const auto id_it = req.obj.find("id");
    const bool has_id = (id_it != req.obj.end() && id_it->second.kind != JsonNode::Kind::Null);
    const std::string id = has_id ? id_it->second.raw : "null";
    const std::string method = arg_str(req, "method");
    // Per JSON-RPC 2.0 / MCP spec, notifications carry no id and must not receive a response.
    if (!has_id || method.rfind("notifications/", 0) == 0) {
      continue;
    }
    if (method == "initialize") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"protocolVersion\":\"2024-11-05\",\"serverInfo\":{\"name\":\"t81-"
                   "native-mcp\",\"version\":\""
                << T81_VERSION_STR << "\"},\"capabilities\":{\"tools\":{}}}}\n"
                << std::flush;
    } else if (method == "ping") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":{}}\n" << std::flush;
    } else if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"tools\":" << build_tools_list_json() << "}}\n"
                << std::flush;
    } else if (method == "tools/call") {
      static const JsonNode kEmptyObj{JsonNode::Kind::Object, {}, "{}", {}, {}};
      const auto params_it = req.obj.find("params");
      const JsonNode& params =
          (params_it != req.obj.end() && params_it->second.kind == JsonNode::Kind::Object)
              ? params_it->second
              : kEmptyObj;
      const std::string name = arg_str(params, "name");
      const auto args_it = params.obj.find("arguments");
      const JsonNode& args =
          (args_it != params.obj.end() && args_it->second.kind == JsonNode::Kind::Object)
              ? args_it->second
              : kEmptyObj;
      std::string structured = "{\"ok\":false,\"error\":\"unknown tool\",\"hint\":\"call "
                               "tools/list to inspect valid tool names and inputSchema\"}";

      if (name == "t81.policy_validate" || name == "t81.policy_test" ||
          name == "t81.get_effective_policy") {
        const std::string ppath = arg_str(args, "policy_path");
        if (ppath.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required argument: "
                       "policy_path\",\"expected\":[\"policy_path\"]}";
        } else {
          std::vector<uint8_t> pbytes;
          if (!read_file_bytes(jail, ppath, pbytes)) {
            structured = "{\"ok\":false,\"valid\":false,\"error\":\"could not open policy file\"}";
          } else {
            std::string content(pbytes.begin(), pbytes.end());
            auto res = t81::axion::parse_policy(content);
            if (!res) {
              structured =
                  "{\"ok\":false,\"valid\":false,\"error\":\"" + json_escape_mcp(res.error()) +
                  "\",\"hint\":\"use valid .apl clauses such as (tier 1), (allowed-tensor-hashes "
                  "[...]), (require-axion-event (reason \\\"...\\\"))\"}";
            } else if (name == "t81.policy_validate") {
              structured =
                  "{\"ok\":true,\"valid\":true,\"tier\":" + std::to_string(res.value().tier) + "}";
            } else if (name == "t81.get_effective_policy") {
              const std::string action_type = arg_str(args, "action_type");
              const std::string floor = resolve_floor(content, action_type);
              structured = "{\"ok\":true,\"policy_hash\":\"" + sha3_512_hex(pbytes) +
                           "\",\"action_type\":\"" + json_escape_mcp(action_type) +
                           "\",\"policy_floor\":\"" + floor + "\",\"allowed_hash_count\":" +
                           std::to_string(policy_allowed_hash_count(res.value())) + "}";
            } else {
              const std::string mhash = arg_str(args, "model_hash");
              if (mhash.empty()) {
                structured = "{\"ok\":false,\"error\":\"missing required argument: "
                             "model_hash\",\"expected\":[\"policy_path\",\"model_hash\"]}";
              } else {
                const bool allowed = policy_admits_hash(res.value(), mhash);
                structured = std::string("{\"ok\":") + (allowed ? "true" : "false") +
                             ",\"admitted\":" + (allowed ? "true" : "false") + "}";
              }
            }
          }
        }
      } else if (name == "t81.gate_check") {
        const std::string apath = arg_str(args, "artifact_path");
        const std::string ppath = arg_str(args, "policy_path");
        const std::string action_type = arg_str(args, "action_type");
        const std::string req_mode = arg_str(args, "mode");
        std::vector<uint8_t> abytes, pbytes;
        if (apath.empty() || ppath.empty()) {
          structured = "{\"ok\":false,\"code\":\"ERR_MISSING_ARG\",\"decision\":\"DENY\","
                       "\"blocked\":true,\"error\":\"missing required arguments\","
                       "\"expected\":[\"artifact_path\",\"policy_path\"]}";
        } else if (!req_mode.empty() && mode_rank(req_mode) < 0) {
          structured = "{\"ok\":false,\"code\":\"ERR_INVALID_ARG\",\"decision\":\"DENY\","
                       "\"blocked\":true,\"error\":\"invalid mode: expected observe-only, warn, "
                       "or fail-closed\"}";
        } else if (!read_file_bytes(jail, apath, abytes) || !read_file_bytes(jail, ppath, pbytes)) {
          structured = "{\"ok\":false,\"decision\":\"DENY\",\"blocked\":true,\"error\":\"missing "
                       "artifact or policy file\"}";
        } else {
          std::string content(pbytes.begin(), pbytes.end());
          auto res = t81::axion::parse_policy(content);
          if (!res) {
            structured = "{\"ok\":false,\"decision\":\"DENY\",\"policy_floor\":\"fail-closed\","
                         "\"mode_applied\":\"fail-closed\",\"downgrade_attempt_blocked\":false,"
                         "\"blocked\":true,\"error\":\"" +
                         json_escape_mcp(res.error()) + "\"}";
          } else {
            const std::string asha = sha3_512_hex(abytes);
            const std::string psha = sha3_512_hex(pbytes);
            const std::string floor = resolve_floor(content, action_type);
            std::string eff = floor;
            bool downgrade_blocked = false;
            if (mode_rank(req_mode) >= 0) {
              if (mode_rank(req_mode) < mode_rank(floor)) {
                downgrade_blocked = true;
              } else {
                eff = req_mode;
              }
            }
            const bool admitted = policy_admits_hash(res.value(), asha);
            const bool blocked = (!admitted && eff == "fail-closed");
            const std::string decision = admitted ? "ALLOW" : (eff == "warn" ? "WARN" : "DENY");
            structured =
                std::string("{\"ok\":") + (blocked ? "false" : "true") + ",\"decision\":\"" +
                decision + "\",\"policy_floor\":\"" + floor + "\",\"mode_applied\":\"" + eff +
                "\",\"downgrade_attempt_blocked\":" + (downgrade_blocked ? "true" : "false") +
                ",\"blocked\":" + (blocked ? "true" : "false") + ",\"artifact_sha3_512\":\"" +
                asha + "\",\"policy_hash\":\"" + psha + "\"}";
          }
        }
      } else if (name == "t81.canonfs_fsck") {
        const std::string croot = arg_str(args, "canonfs_root");
        fs::path canon_root;
        if (croot.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required argument: "
                       "canonfs_root\",\"expected\":[\"canonfs_root\"]}";
        } else if (!jail.resolve(croot, canon_root)) {
          structured = io_failure_json(jail, "path is outside the MCP filesystem root");
        } else {
          std::ostringstream cap;
          std::streambuf* old_buf = std::cout.rdbuf(cap.rdbuf());
          const int rc = run_canonfs_fsck_json(canon_root.string());
          std::cout.rdbuf(old_buf);
          std::string raw = cap.str();
          if (!raw.empty() && raw.front() == '{') {
            std::string compact;
            compact.reserve(raw.size());
            for (char ch : raw) {
              if (ch != '\n' && ch != '\r') compact.push_back(ch);
            }
            structured = compact;
          } else {
            structured = std::string("{\"ok\":") + (rc == 0 ? "true" : "false") + "}";
          }
        }
      } else if (name == "t81.determinism_hash") {
        const std::string fpath = arg_str(args, "path");
        std::vector<uint8_t> bytes;
        if (fpath.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required argument: "
                       "path\",\"expected\":[\"path\"]}";
        } else if (!read_file_bytes(jail, fpath, bytes)) {
          structured = io_failure_json(jail, "could not open file");
        } else {
          structured = "{\"ok\":true,\"sha3_512\":\"" + sha3_512_hex(bytes) + "\"}";
        }
      } else if (name == "t81.session_bootstrap") {
        const std::string root_arg = arg_str(args, "repo_root");
        fs::path root;
        const fs::path requested = root_arg.empty() ? fs::path(".") : fs::path(root_arg);
        if (!jail.resolve(requested, root)) {
          structured = io_failure_json(jail, "path is outside the MCP filesystem root");
        } else {
          std::string git_ref;
          const std::string head_sha = read_git_head_sha(jail, root, git_ref);
          const fs::path lock_path = root / "docs" / "governance" / "PUBLIC_API_LOCK.json";
          const fs::path reg_path =
              root / "docs" / "governance" / "DETERMINISM_SURFACE_REGISTRY.md";
          std::vector<uint8_t> lock_bytes;
          const bool lock_ok = read_file_bytes(jail, lock_path, lock_bytes);
          fs::path reg_canon;
          std::error_code reg_ec;
          const bool reg_ok =
              jail.resolve(reg_path, reg_canon) && fs::is_regular_file(reg_canon, reg_ec);
          structured =
              std::string("{\"ok\":true,\"version\":\"") + T81_VERSION_STR + "\",\"repo_root\":\"" +
              json_escape_mcp(root.string()) + "\",\"git_ref\":\"" + json_escape_mcp(git_ref) +
              "\",\"git_head_sha\":\"" + json_escape_mcp(head_sha) +
              "\",\"public_api_lock_present\":" + (lock_ok ? "true" : "false") +
              ",\"public_api_lock_sha3_512\":\"" + (lock_ok ? sha3_512_hex(lock_bytes) : "") +
              "\",\"determinism_registry_present\":" + (reg_ok ? "true" : "false") +
              ",\"native_mcp_tools\":15,\"capabilities\":" + detect_host_capabilities_json() + "}";
        }
      } else if (name == "t81.tx_begin") {
        const std::string tpath = arg_str(args, "target_path");
        const std::string bdir = arg_str(args, "backup_dir");
        const std::string req_agent = arg_str(args, "agent_id");
        const std::string agent_id = req_agent.empty() ? "default" : req_agent;
        const bool isolated = arg_bool(args, "isolated", false);
        std::vector<uint8_t> bytes;
        if (tpath.empty() || bdir.empty()) {
          structured = "{\"ok\":false,\"trit\":-1,\"error\":\"missing required "
                       "arguments\",\"expected\":[\"target_path\",\"backup_dir\"]}";
        } else if (!read_file_bytes(jail, tpath, bytes)) {
          structured = io_failure_json(jail, "could not read target_path");
        } else {
          fs::path target_canon;
          fs::path backup_root;
          if (!jail.resolve(tpath, target_canon) || !jail.resolve(bdir, backup_root)) {
            structured = io_failure_json(jail, "path is outside the MCP filesystem root");
          } else {
            std::error_code ec;
            fs::create_directories(backup_root, ec);
            const std::string tx_id = "tx-" + std::to_string(++tx_counter);
            const fs::path bpath = backup_root / (tx_id + ".bak");
            const fs::path spath = isolated ? (backup_root / (tx_id + ".stage")) : fs::path{};
            if (ec || !write_file_bytes(jail, bpath, bytes) ||
                (isolated && !write_file_bytes(jail, spath, bytes))) {
              structured = io_failure_json(jail, "could not write transaction backup");
            } else {
              const std::string before_sha = sha3_512_hex(bytes);
              active_txs[tx_id] =
                  ActiveTx{tx_id, agent_id, target_canon, bpath, spath, before_sha, isolated};
              structured = "{\"ok\":true,\"trit\":0,\"tx_id\":\"" + tx_id + "\",\"agent_id\":\"" +
                           json_escape_mcp(agent_id) +
                           "\",\"isolated\":" + (isolated ? "true" : "false") +
                           ",\"before_sha3_512\":\"" + before_sha + "\",\"backup_path\":\"" +
                           json_escape_mcp(bpath.string()) + "\"";
              if (isolated) {
                structured += ",\"staging_path\":\"" + json_escape_mcp(spath.string()) + "\"";
              }
              structured += "}";
            }
          }
        }
      } else if (name == "t81.tx_abort") {
        const std::string tx_id = arg_str(args, "tx_id");
        auto it = active_txs.find(tx_id);
        if (it == active_txs.end()) {
          structured = "{\"ok\":false,\"trit\":-1,\"error\":\"unknown or closed tx_id\"}";
        } else {
          std::vector<uint8_t> bbytes;
          if (!read_file_bytes(jail, it->second.backup_path, bbytes) ||
              sha3_512_hex(bbytes) != it->second.before_sha3_512) {
            structured = "{\"ok\":false,\"trit\":-1,\"error\":\"backup integrity verification "
                         "failed (SHA3-512 mismatch)\"}";
          } else if (!it->second.isolated &&
                     !write_file_bytes(jail, it->second.target_path, bbytes)) {
            structured = "{\"ok\":false,\"trit\":-1,\"error\":\"failed restoring target_path\"}";
          } else {
            std::error_code ec;
            fs::remove(it->second.backup_path, ec);
            if (it->second.isolated && !it->second.staging_path.empty()) {
              fs::remove(it->second.staging_path, ec);
            }
            const std::string restored_sha = it->second.before_sha3_512;
            const std::string aid = it->second.agent_id;
            active_txs.erase(it);
            structured = "{\"ok\":true,\"trit\":-1,\"tx_id\":\"" + tx_id + "\",\"agent_id\":\"" +
                         json_escape_mcp(aid) + "\",\"aborted\":true,\"restored_sha3_512\":\"" +
                         restored_sha + "\"}";
          }
        }
      } else if (name == "t81.tx_commit") {
        const std::string tx_id = arg_str(args, "tx_id");
        const std::string ppath = arg_str(args, "policy_path");
        const std::string req_agent = arg_str(args, "agent_id");
        const std::string expected_parent = arg_str(args, "expected_parent_sha3_512");
        const std::string expected_parent_rec = arg_str(args, "expected_parent_receipt");
        const std::string canonfs_root_arg = arg_str(args, "canonfs_root");
        const std::string sbdr_path_arg = arg_str(args, "sbdr_path");
        const std::string bc_hex_arg = arg_str(args, "bytecode_hex");
        int64_t max_abs_err = 0;
        const bool max_abs_err_valid = arg_int64_strict(args, "max_abs_err", max_abs_err, 0);
        int64_t fallback_cnt = 0;
        const bool fallback_cnt_valid =
            arg_int64_strict(args, "dualrail_fallback_count", fallback_cnt, 0);
        const bool repeat_ok = arg_bool(args, "repeat_sha3_identical", true);
        const bool req_auto_abort = arg_bool(args, "auto_abort_on_deny", false);
        auto it = active_txs.find(tx_id);
        if (it == active_txs.end()) {
          structured = "{\"ok\":false,\"trit\":-1,\"error\":\"unknown or closed tx_id\"}";
        } else if (ppath.empty()) {
          structured = "{\"ok\":false,\"trit\":-1,\"committed\":false,\"tx_id\":\"" + tx_id +
                       "\",\"error\":\"missing required argument: policy_path\"}";
        } else if (!req_agent.empty() && req_agent != it->second.agent_id) {
          const ActiveTx tx = it->second;
          const bool do_auto_abort = req_auto_abort || tx.isolated;
          bool auto_aborted = false;
          if (do_auto_abort) {
            std::vector<uint8_t> bbytes;
            if (!tx.isolated && read_file_bytes(jail, tx.backup_path, bbytes) &&
                sha3_512_hex(bbytes) == tx.before_sha3_512) {
              write_file_bytes(jail, tx.target_path, bbytes);
            }
            std::error_code ec;
            fs::remove(tx.backup_path, ec);
            if (tx.isolated && !tx.staging_path.empty()) {
              fs::remove(tx.staging_path, ec);
            }
            active_txs.erase(it);
            auto_aborted = true;
          }
          structured = "{\"ok\":false,\"trit\":-1,\"committed\":false,\"auto_aborted\":" +
                       std::string(auto_aborted ? "true" : "false") + ",\"tx_id\":\"" + tx_id +
                       "\",\"agent_id\":\"" + json_escape_mcp(tx.agent_id) +
                       "\",\"code\":\"ERR_AGENT_MISMATCH\",\"error\":\"agent_id does not own "
                       "transaction\"}";
        } else {
          const ActiveTx tx = it->second;
          const bool do_auto_abort = req_auto_abort || tx.isolated;
          const fs::path candidate_path = tx.isolated ? tx.staging_path : tx.target_path;
          std::vector<uint8_t> after_bytes;
          std::vector<uint8_t> live_bytes;
          if (!read_file_bytes(jail, candidate_path, after_bytes)) {
            structured =
                "{\"ok\":false,\"trit\":-1,\"error\":\"could not read mutated target_path\"}";
          } else if (!read_file_bytes(jail, tx.target_path, live_bytes)) {
            structured = "{\"ok\":false,\"trit\":-1,\"error\":\"could not read live target_path\"}";
          } else {
            const std::string after_sha = sha3_512_hex(after_bytes);
            const std::string live_sha = sha3_512_hex(live_bytes);
            bool policy_ok = true;
            std::string deny_code;
            std::string policy_err;
            std::string sbdr_sha;
            std::string vm_exec_receipt;
            uint64_t vm_skipped_blocks = 0;
            bool vm_repeat_verified = false;

            if (tx.target_path.filename() == "axion_root.apl") {
              policy_ok = false;
              deny_code = "ERR_IMMUTABLE_POLICY_ROOT";
              policy_err = "axion_root.apl is immutable regardless of supplied policy_path";
            } else if (!max_abs_err_valid || !fallback_cnt_valid) {
              policy_ok = false;
              deny_code = "ERR_INVALID_BUDGET";
              policy_err = "capability budget arguments must be exact integers";
            } else if (tx.isolated && live_sha != tx.before_sha3_512) {
              policy_ok = false;
              deny_code = "ERR_TX_CONFLICT";
              policy_err = "stale transaction: target_path was committed by a peer; rebase on "
                           "current_sha3_512";
            } else if (!expected_parent.empty() && live_sha != expected_parent) {
              policy_ok = false;
              deny_code = "ERR_PARENT_RECEIPT_MISMATCH";
              policy_err = "expected_parent_sha3_512 does not match live target_path SHA3-512";
            } else if (!expected_parent_rec.empty() &&
                       (last_commit_receipts.find(tx.target_path.string()) ==
                            last_commit_receipts.end() ||
                        last_commit_receipts[tx.target_path.string()] != expected_parent_rec)) {
              policy_ok = false;
              deny_code = "ERR_PARENT_RECEIPT_MISMATCH";
              policy_err = "expected_parent_receipt does not match latest committed receipt";
            } else if (max_abs_err != 0) {
              policy_ok = false;
              deny_code = "ERR_BUDGET_MAX_ABS_ERR";
              policy_err = "capability budget violated: max_abs_err != 0";
            } else if (fallback_cnt != 0) {
              policy_ok = false;
              deny_code = "ERR_BUDGET_DUALRAIL_FALLBACK";
              policy_err = "capability budget violated: dualrail_fallback_count != 0";
            } else if (!repeat_ok) {
              policy_ok = false;
              deny_code = "ERR_BUDGET_REPEAT_DETERMINISM";
              policy_err = "capability budget violated: repeat_sha3_identical != true";
            } else {
              fs::path policy_canon;
              std::vector<uint8_t> pbytes;
              if (!jail.resolve(ppath, policy_canon) || !read_file_bytes(jail, ppath, pbytes)) {
                policy_ok = false;
                deny_code = "ERR_POLICY_READ";
                policy_err =
                    jail.last_error.empty() ? "could not read policy_path" : jail.last_error;
              } else if (policy_canon == tx.target_path) {
                policy_ok = false;
                deny_code = "ERR_IMMUTABLE_POLICY_ROOT";
                policy_err = "policy_path is immutable and outside agent tx_commit write authority";
              } else {
                std::string ptext(pbytes.begin(), pbytes.end());
                auto pres = t81::axion::parse_policy(ptext);
                if (!pres) {
                  policy_ok = false;
                  deny_code = "ERR_POLICY_PARSE";
                  policy_err = pres.error();
                } else if (!policy_admits_hash(pres.value(), after_sha)) {
                  policy_ok = false;
                  deny_code = "ERR_POLICY_HASH_DENIED";
                  policy_err =
                      "mutated target_path SHA3-512 denied by policy; call t81.tx_abort to restore";
                } else {
                  if (policy_ok && !canonfs_root_arg.empty()) {
                    fs::path canon_root;
                    if (!jail.resolve(canonfs_root_arg, canon_root)) {
                      policy_ok = false;
                      deny_code = "ERR_CANONFS_OUTSIDE_ROOT";
                      policy_err = "canonfs_root is outside MCP filesystem root";
                    } else {
                      std::ostringstream cap;
                      std::streambuf* old_buf = std::cout.rdbuf(cap.rdbuf());
                      const int rc = run_canonfs_fsck_json(canon_root.string());
                      std::cout.rdbuf(old_buf);
                      if (rc != 0) {
                        policy_ok = false;
                        deny_code = "ERR_CANONFS_FSCK_FAILED";
                        policy_err = "CanonFS integrity check failed (tampered store)";
                      }
                    }
                  }
                  if (policy_ok && !sbdr_path_arg.empty()) {
                    std::vector<uint8_t> sbytes;
                    if (!read_file_bytes(jail, sbdr_path_arg, sbytes)) {
                      policy_ok = false;
                      deny_code = "ERR_SBDR1T2D_READ";
                      policy_err = "could not read sbdr_path";
                    } else if (sbytes.size() < 24 ||
                               std::memcmp(sbytes.data(), "SBDR1\0\0\0", 8) != 0) {
                      policy_ok = false;
                      deny_code = "ERR_SBDR1T2D_INVALID";
                      policy_err = "invalid SBDR1T2D header or non-zero K* flag";
                    } else {
                      uint64_t total_blocks = 0;
                      std::memcpy(&total_blocks, sbytes.data() + 8, sizeof(uint64_t));
                      const uint64_t mask_words = (total_blocks + 63ULL) / 64ULL;
                      const uint64_t header_and_mask_bytes = 16ULL + mask_words * sizeof(uint64_t);
                      if (total_blocks == 0 || mask_words > (sbytes.size() / sizeof(uint64_t)) ||
                          sbytes.size() < header_and_mask_bytes) {
                        policy_ok = false;
                        deny_code = "ERR_SBDR1T2D_INVALID";
                        policy_err = "SBDR1T2D truncated or invalid block bitmap";
                      } else {
                        uint64_t active_blocks = 0;
                        bool bitmap_overflow = false;
                        for (uint64_t wi = 0; wi < mask_words; ++wi) {
                          uint64_t mw = 0;
                          std::memcpy(&mw, sbytes.data() + 16 + wi * sizeof(uint64_t),
                                      sizeof(uint64_t));
                          if (wi + 1 == mask_words && (total_blocks % 64ULL) != 0) {
                            const uint64_t valid_mask = (1ULL << (total_blocks % 64ULL)) - 1ULL;
                            if ((mw & ~valid_mask) != 0) {
                              bitmap_overflow = true;
                              break;
                            }
                          }
                          active_blocks += static_cast<uint64_t>(std::popcount(mw));
                        }
                        const uint64_t expected_size =
                            header_and_mask_bytes + active_blocks * 16ULL;
                        if (bitmap_overflow || active_blocks > total_blocks ||
                            sbytes.size() != expected_size) {
                          policy_ok = false;
                          deny_code = "ERR_SBDR1T2D_INVALID";
                          policy_err = "SBDR1T2D bitmap or payload length mismatch";
                        } else {
                          sbdr_sha = sha3_512_hex(sbytes);
                          vm_skipped_blocks += (total_blocks - active_blocks);
                          if (!policy_admits_hash(pres.value(), sbdr_sha)) {
                            policy_ok = false;
                            deny_code = "ERR_SBDR1T2D_POLICY_DENIED";
                            policy_err = "SBDR1T2D artifact SHA3-512 denied by policy";
                          }
                        }
                      }
                    }
                  }
                  if (policy_ok && !bc_hex_arg.empty()) {
                    std::vector<uint8_t> raw_bc;
                    if (!decode_hex_bytes(bc_hex_arg, raw_bc) || raw_bc.empty()) {
                      policy_ok = false;
                      deny_code = "ERR_VM_INVALID_BYTECODE";
                      policy_err = "invalid bytecode_hex in tx_commit";
                    } else {
                      std::vector<std::byte> bvec(raw_bc.size());
                      std::memcpy(bvec.data(), raw_bc.data(), raw_bc.size());
                      auto dec = t81::tisc::decode(bvec);
                      if (!dec.has_value()) {
                        policy_ok = false;
                        deny_code = "ERR_VM_DECODE_FAILED";
                        policy_err = "TISC v1.9.0 bytecode decode failed";
                      } else {
                        const auto vr = verify_tisc_subgraph(dec.value());
                        if (!vr.ok) {
                          policy_ok = false;
                          deny_code = "ERR_VM_VERIFY_FAILED";
                          policy_err = "TISC subgraph verification failed: " + vr.verdict;
                        } else {
                          const std::string prog_sha = sha3_512_hex(raw_bc);
                          std::string receipts[2];
                          for (int run_i = 0; run_i < 2 && policy_ok; ++run_i) {
                            t81::vm::internal::reset_dualrail_telemetry();
                            auto vm = t81::vm::make_interpreter_vm(
                                std::make_unique<t81::axion::AIHookEngine>(
                                    std::make_unique<t81::axion::PolicyEngine>(pres.value())));
                            vm->load_program(dec.value());
                            const auto run_res = vm->run_to_halt();
                            const auto dr_tel = t81::vm::internal::dualrail_telemetry();
                            if (!run_res.has_value()) {
                              policy_ok = false;
                              deny_code = "ERR_VM_TRAP";
                              policy_err = std::string("T81VM trapped during tx_commit: ") +
                                           trap_name_str(run_res.error());
                            } else if (dr_tel.fallback_count != 0) {
                              policy_ok = false;
                              deny_code = "ERR_VM_DUALRAIL_FALLBACK";
                              policy_err = "T81VM dualrail_fallback_count != 0 during tx_commit";
                            } else {
                              if (run_i == 0) {
                                vm_skipped_blocks += dr_tel.sparse128_blocks_skipped;
                              }
                              const std::string rec_in =
                                  prog_sha + "|HALT|0|" +
                                  std::to_string(vm->state().axion_log.size()) + "|" +
                                  std::to_string(dr_tel.fastpath_hits) + "|" +
                                  std::to_string(dr_tel.fallback_count);
                              receipts[run_i] =
                                  sha3_512_hex(std::vector<uint8_t>(rec_in.begin(), rec_in.end()));
                            }
                          }
                          if (policy_ok && receipts[0] != receipts[1]) {
                            policy_ok = false;
                            deny_code = "ERR_VM_REPEAT_MISMATCH";
                            policy_err = "T81VM repeat_sha3_identical failed during tx_commit";
                          } else if (policy_ok) {
                            vm_exec_receipt = receipts[0];
                            vm_repeat_verified = true;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            if (!policy_ok) {
              bool auto_aborted = false;
              if (do_auto_abort) {
                std::vector<uint8_t> bbytes;
                if (!tx.isolated && read_file_bytes(jail, tx.backup_path, bbytes) &&
                    sha3_512_hex(bbytes) == tx.before_sha3_512) {
                  (void)write_file_bytes(jail, tx.target_path, bbytes);
                }
                std::error_code ec;
                fs::remove(tx.backup_path, ec);
                if (tx.isolated && !tx.staging_path.empty()) {
                  fs::remove(tx.staging_path, ec);
                }
                active_txs.erase(it);
                auto_aborted = true;
              }
              structured = "{\"ok\":false,\"trit\":-1,\"committed\":false,\"auto_aborted\":" +
                           std::string(auto_aborted ? "true" : "false") + ",\"tx_id\":\"" + tx_id +
                           "\",\"agent_id\":\"" + json_escape_mcp(tx.agent_id) + "\",\"code\":\"" +
                           deny_code + "\",\"current_sha3_512\":\"" + live_sha + "\",\"error\":\"" +
                           json_escape_mcp(policy_err) + "\"}";
            } else {
              const std::string before_sha = tx.before_sha3_512;
              const bool changed = (before_sha != after_sha);
              const auto prev_it = last_commit_receipts.find(tx.target_path.string());
              const std::string parent_receipt_ref =
                  (prev_it != last_commit_receipts.end()) ? prev_it->second : "";
              std::string receipt_in = before_sha + "|" + after_sha + "|" + tx.agent_id;
              if (!sbdr_sha.empty()) receipt_in += "|" + sbdr_sha;
              if (!vm_exec_receipt.empty()) receipt_in += "|" + vm_exec_receipt;
              const std::string commit_receipt =
                  sha3_512_hex(std::vector<uint8_t>(receipt_in.begin(), receipt_in.end()));

              // Compute authoritative policy digest & seal signed receipt into CanonFS (M2).
              std::string policy_sha;
              {
                std::vector<uint8_t> pbytes;
                if (read_file_bytes(jail, ppath, pbytes)) {
                  auto pres = t81::axion::parse_policy(std::string(pbytes.begin(), pbytes.end()));
                  if (pres.has_value()) {
                    policy_sha =
                        t81::provider::ProviderGovernor::compute_policy_sha3_512(pres.value());
                  }
                }
              }
              const std::string repro_class =
                  vm_repeat_verified ? "deterministic" : "provenance_only";
              const std::string meaning_json =
                  "{\"action_type\":\"mcp.tx_commit\",\"agent_id\":\"" +
                  json_escape_mcp(tx.agent_id) + "\",\"tx_id\":\"" + json_escape_mcp(tx_id) + "\"}";
              const std::string evidence_json =
                  "{\"after_sha3_512\":\"" + after_sha + "\",\"before_sha3_512\":\"" + before_sha +
                  "\",\"commit_receipt\":\"" + commit_receipt + "\",\"parent_receipt_ref\":\"" +
                  json_escape_mcp(parent_receipt_ref) + "\",\"reproducibility_class\":\"" +
                  repro_class + "\"}";
              const std::string authority_json =
                  "{\"policy_sha3_512\":\"" + policy_sha + "\",\"verdict\":\"Allow\"}";

              std::string evidence_ref_hex;
              std::string sig_b64;
              std::string persist_code;
              std::string persist_err;
              const bool sealed_ok =
                  persist_and_sign_receipt(fs::path{}, meaning_json, evidence_json, authority_json,
                                           evidence_ref_hex, sig_b64, persist_code, persist_err);
              if (!sealed_ok) {
                std::error_code ec;
                fs::remove(tx.backup_path, ec);
                if (tx.isolated && !tx.staging_path.empty()) {
                  fs::remove(tx.staging_path, ec);
                }
                active_txs.erase(it);
                structured = "{\"ok\":false,\"trit\":-1,\"committed\":false,\"auto_aborted\":true,"
                             "\"tx_id\":\"" +
                             tx_id + "\",\"code\":\"" + persist_code + "\",\"error\":\"" +
                             json_escape_mcp(persist_err) + "\"}";
              } else if (tx.isolated && !write_file_bytes(jail, tx.target_path, after_bytes)) {
                structured =
                    "{\"ok\":false,\"trit\":-1,\"committed\":false,\"error\":\"failed publishing "
                    "staged artifact to target_path\"}";
              } else {
                std::error_code ec;
                fs::remove(tx.backup_path, ec);
                if (tx.isolated && !tx.staging_path.empty()) {
                  fs::remove(tx.staging_path, ec);
                }
                last_commit_receipts[tx.target_path.string()] = commit_receipt;
                active_txs.erase(it);
                structured = "{\"ok\":true,\"trit\":1,\"committed\":true,\"tx_id\":\"" + tx_id +
                             "\",\"agent_id\":\"" + json_escape_mcp(tx.agent_id) +
                             "\",\"before_sha3_512\":\"" + before_sha + "\",\"after_sha3_512\":\"" +
                             after_sha + "\",\"commit_receipt\":\"" + commit_receipt +
                             "\",\"parent_receipt_ref\":\"" + json_escape_mcp(parent_receipt_ref) +
                             "\",\"policy_sha3_512\":\"" + policy_sha +
                             "\",\"reproducibility_class\":\"" + repro_class +
                             "\",\"evidence_canonfs_ref\":\"" + evidence_ref_hex +
                             "\",\"ed25519_signature_b64\":\"" + sig_b64 +
                             "\",\"dualrail_sparse128_blocks_skipped\":" +
                             std::to_string(vm_skipped_blocks) +
                             ",\"dualrail_fallback_count\":0,\"repeat_sha3_identical\":" +
                             (vm_repeat_verified ? "true" : "null") +
                             ",\"changed\":" + (changed ? "true" : "false");
                if (!vm_exec_receipt.empty()) {
                  structured += ",\"vm_execution_receipt\":\"" + vm_exec_receipt + "\"";
                }
                structured += "}";
              }
            }
          }
        }
      } else if (name == "t81.verify_subgraph") {
        const std::string bc_path = arg_str(args, "bytecode_path");
        const std::string bc_hex = arg_str(args, "bytecode_hex");
        std::vector<uint8_t> raw;
        bool loaded_ok = false;
        if (!bc_hex.empty()) {
          loaded_ok = decode_hex_bytes(bc_hex, raw);
        } else if (!bc_path.empty()) {
          loaded_ok = read_file_bytes(jail, bc_path, raw);
        }
        if (!loaded_ok || raw.empty()) {
          structured = "{\"ok\":false,\"verdict\":\"REJECT_INVALID_INPUT\","
                       "\"hint\":\"provide valid bytecode_hex or readable bytecode_path\"}";
        } else {
          std::vector<std::byte> bvec(raw.size());
          std::memcpy(bvec.data(), raw.data(), raw.size());
          auto dec = t81::tisc::decode(bvec);
          if (!dec.has_value()) {
            const std::string err_code = dec.error() == t81::tisc::EncodingError::Truncated
                                             ? "REJECT_TRUNCATED_BYTECODE"
                                             : "REJECT_INVALID_OPCODE";
            structured = "{\"ok\":false,\"verdict\":\"" + err_code + "\"}";
          } else {
            const auto vr = verify_tisc_subgraph(dec.value());
            const std::string prog_sha = sha3_512_hex(raw);
            const std::string rec_backend =
                vr.matmul_count >= 4 ? "gpu_dualrail_fused" : "cpu_dualrail_neon";
            structured = "{\"ok\":" + std::string(vr.ok ? "true" : "false") + ",\"verdict\":\"" +
                         vr.verdict +
                         "\",\"instruction_count\":" + std::to_string(dec.value().insns.size()) +
                         ",\"matmul_count\":" + std::to_string(vr.matmul_count) +
                         ",\"bytecode_bytes\":" + std::to_string(raw.size()) +
                         ",\"program_sha3_512\":\"" + prog_sha + "\",\"recommended_backend\":\"" +
                         rec_backend + "\"}";
          }
        }
      } else if (name == "t81.run_subgraph") {
        const std::string bc_path = arg_str(args, "bytecode_path");
        const std::string bc_hex = arg_str(args, "bytecode_hex");
        const std::string ppath = arg_str(args, "policy_path");
        std::vector<uint8_t> raw;
        bool loaded_ok = false;
        if (!bc_hex.empty()) {
          loaded_ok = decode_hex_bytes(bc_hex, raw);
        } else if (!bc_path.empty()) {
          loaded_ok = read_file_bytes(jail, bc_path, raw);
        }
        if (!loaded_ok || raw.empty()) {
          structured = "{\"ok\":false,\"verdict\":\"REJECT_INVALID_INPUT\","
                       "\"hint\":\"provide valid bytecode_hex or readable bytecode_path\"}";
        } else {
          std::vector<std::byte> bvec(raw.size());
          std::memcpy(bvec.data(), raw.data(), raw.size());
          auto dec = t81::tisc::decode(bvec);
          if (!dec.has_value()) {
            const std::string err_code = dec.error() == t81::tisc::EncodingError::Truncated
                                             ? "REJECT_TRUNCATED_BYTECODE"
                                             : "REJECT_INVALID_OPCODE";
            structured = "{\"ok\":false,\"verdict\":\"" + err_code + "\"}";
          } else {
            const auto vr = verify_tisc_subgraph(dec.value());
            const std::string prog_sha = sha3_512_hex(raw);
            if (!vr.ok) {
              structured = "{\"ok\":false,\"verdict\":\"" + vr.verdict +
                           "\",\"executed\":false,\"program_sha3_512\":\"" + prog_sha + "\"}";
            } else if (ppath.empty()) {
              structured = "{\"ok\":false,\"verdict\":\"REJECT_MISSING_POLICY\","
                           "\"executed\":false,\"program_sha3_512\":\"" +
                           prog_sha + "\"}";
            } else {
              t81::axion::Policy pol;
              bool pol_ok = true;
              {
                std::vector<uint8_t> pbytes;
                if (!read_file_bytes(jail, ppath, pbytes)) {
                  pol_ok = false;
                } else {
                  auto parsed = t81::axion::parse_policy(std::string(pbytes.begin(), pbytes.end()));
                  if (!parsed.has_value()) {
                    pol_ok = false;
                  } else {
                    pol = std::move(parsed.value());
                  }
                }
              }
              if (!pol_ok) {
                structured = "{\"ok\":false,\"verdict\":\"REJECT_INVALID_POLICY\","
                             "\"executed\":false,\"program_sha3_512\":\"" +
                             prog_sha + "\"}";
              } else {
                t81::vm::internal::reset_dualrail_telemetry();
                auto vm = t81::vm::make_interpreter_vm(std::make_unique<t81::axion::AIHookEngine>(
                    std::make_unique<t81::axion::PolicyEngine>(pol)));
                vm->load_program(dec.value());
                const auto run_res = vm->run_to_halt();
                const auto dr_tel = t81::vm::internal::dualrail_telemetry();
                const bool halted = run_res.has_value();
                const int trap_code = halted ? 0 : static_cast<int>(run_res.error());
                const std::string trap_name = halted ? "" : trap_name_str(run_res.error());
                const uint64_t fault_pc =
                    vm->state().contexts.empty() ? 0ULL : vm->state().contexts[0].pc;
                const size_t ax_events = vm->state().axion_log.size();
                const std::string last_ax_reason =
                    ax_events == 0 ? "" : vm->state().axion_log.back().verdict.reason;
                const std::string rec_in =
                    prog_sha + "|" + (halted ? "HALT" : "TRAP") + "|" + std::to_string(trap_code) +
                    "|" + std::to_string(ax_events) + "|" + std::to_string(dr_tel.fastpath_hits) +
                    "|" + std::to_string(dr_tel.fallback_count);
                const std::vector<uint8_t> rec_bytes(rec_in.begin(), rec_in.end());
                const std::string exec_rec = sha3_512_hex(rec_bytes);
                const std::string policy_sha =
                    t81::provider::ProviderGovernor::compute_policy_sha3_512(pol);
                std::string evidence_ref_hex;
                std::string sig_b64;
                std::string persist_code;
                std::string persist_err;
                const std::string meaning_json =
                    "{\"action_type\":\"mcp.run_subgraph\",\"program_sha3_512\":\"" + prog_sha +
                    "\"}";
                const std::string evidence_json = "{\"execution_receipt\":\"" + exec_rec +
                                                  "\",\"halted\":" + (halted ? "true" : "false") +
                                                  ",\"reproducibility_class\":\"deterministic\"}";
                const std::string authority_json = "{\"policy_sha3_512\":\"" + policy_sha +
                                                   "\",\"verdict\":\"" +
                                                   (halted ? "Allow" : "Deny") + "\"}";
                const bool sealed_ok = persist_and_sign_receipt(
                    fs::path{}, meaning_json, evidence_json, authority_json, evidence_ref_hex,
                    sig_b64, persist_code, persist_err);
                if (!sealed_ok) {
                  structured = "{\"ok\":false,\"verdict\":\"" + persist_code +
                               "\",\"executed\":false,\"error\":\"" + json_escape_mcp(persist_err) +
                               "\"}";
                } else {
                  structured =
                      std::string("{\"ok\":") + (halted ? "true" : "false") + ",\"verdict\":\"" +
                      (halted ? "VERIFIED_AND_EXECUTED" : "VM_TRAP") +
                      "\",\"executed\":true,\"halted\":" + (halted ? "true" : "false") +
                      ",\"trap_code\":" + std::to_string(trap_code) + ",\"trap_name\":\"" +
                      trap_name + "\",\"fault_pc\":" + std::to_string(fault_pc) +
                      ",\"last_axion_reason\":\"" + json_escape_mcp(last_ax_reason) +
                      "\",\"dualrail_fastpath_hits\":" + std::to_string(dr_tel.fastpath_hits) +
                      ",\"dualrail_dense_hits\":" + std::to_string(dr_tel.dense_hits) +
                      ",\"dualrail_sparse128_hits\":" + std::to_string(dr_tel.sparse128_hits) +
                      ",\"dualrail_sparse128_blocks_skipped\":" +
                      std::to_string(dr_tel.sparse128_blocks_skipped) +
                      ",\"dualrail_fallback_count\":" + std::to_string(dr_tel.fallback_count) +
                      ",\"instruction_count\":" + std::to_string(dec.value().insns.size()) +
                      ",\"matmul_count\":" + std::to_string(vr.matmul_count) +
                      ",\"axion_events\":" + std::to_string(ax_events) +
                      ",\"program_sha3_512\":\"" + prog_sha + "\",\"policy_sha3_512\":\"" +
                      policy_sha +
                      "\",\"reproducibility_class\":\"deterministic\",\"evidence_canonfs_ref\":\"" +
                      evidence_ref_hex + "\",\"ed25519_signature_b64\":\"" + sig_b64 +
                      "\",\"execution_receipt\":\"" + exec_rec + "\"}";
                }
              }
            }
          }
        }
      } else if (name == "t81.diff_check") {
        const std::string bp_path = arg_str(args, "base_pos_path");
        const std::string bn_path = arg_str(args, "base_neg_path");
        const std::string cp_path = arg_str(args, "candidate_pos_path");
        const std::string cn_path = arg_str(args, "candidate_neg_path");
        double max_flip_pct = 1.0;
        const bool max_ok = arg_double(args, "max_flip_pct", 1.0, max_flip_pct) &&
                            max_flip_pct >= 0.0 && max_flip_pct <= 100.0;
        std::vector<uint8_t> bp, bn, cp, cn;
        if (!max_ok) {
          structured = "{\"ok\":false,\"allowed\":false,\"verdict\":\"REJECT_INVALID_ARGUMENT\"}";
        } else if (bp_path.empty() || bn_path.empty() || cp_path.empty() || cn_path.empty() ||
                   !read_file_bytes(jail, bp_path, bp) || !read_file_bytes(jail, bn_path, bn) ||
                   !read_file_bytes(jail, cp_path, cp) || !read_file_bytes(jail, cn_path, cn)) {
          structured = "{\"ok\":false,\"allowed\":false,\"verdict\":\"REJECT_IO_ERROR\"}";
        } else if (bp.empty() || bp.size() != bn.size() || bp.size() != cp.size() ||
                   bp.size() != cn.size() || (bp.size() % 8 != 0)) {
          structured = "{\"ok\":false,\"allowed\":false,\"verdict\":\"REJECT_SIZE_MISMATCH\"}";
        } else {
          const size_t words = bp.size() / 8;
          const uint64_t total_weights = static_cast<uint64_t>(words) * 64ULL;
          uint64_t flips = 0;
          bool disjoint_ok = true;
          for (size_t w = 0; w < words; ++w) {
            uint64_t p0 = 0, n0 = 0, p1 = 0, n1 = 0;
            std::memcpy(&p0, bp.data() + w * 8, 8);
            std::memcpy(&n0, bn.data() + w * 8, 8);
            std::memcpy(&p1, cp.data() + w * 8, 8);
            std::memcpy(&n1, cn.data() + w * 8, 8);
            if (((p0 & n0) | (p1 & n1)) != 0ULL) {
              disjoint_ok = false;
              break;
            }
            flips += static_cast<uint64_t>(std::popcount((p0 ^ p1) | (n0 ^ n1)));
          }
          if (!disjoint_ok) {
            structured =
                "{\"ok\":false,\"allowed\":false,\"verdict\":\"REJECT_NON_DISJOINT_PLANES\"}";
          } else {
            const double flip_pct =
                100.0 * static_cast<double>(flips) / static_cast<double>(total_weights);
            const bool allowed = (flip_pct <= max_flip_pct);
            std::vector<uint8_t> b_cat = bp;
            b_cat.insert(b_cat.end(), bn.begin(), bn.end());
            std::vector<uint8_t> c_cat = cp;
            c_cat.insert(c_cat.end(), cn.begin(), cn.end());
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(4) << "{\"ok\":" << (allowed ? "true" : "false")
                << ",\"allowed\":" << (allowed ? "true" : "false") << ",\"verdict\":\""
                << (allowed ? "ADMIT_WITHIN_FLIP_BUDGET" : "DENY_EXCEEDS_FLIP_BUDGET")
                << "\",\"total_weights\":" << total_weights << ",\"flipped_weights\":" << flips
                << ",\"flip_pct\":" << flip_pct << ",\"max_flip_pct\":" << max_flip_pct
                << ",\"base_sha3_512\":\"" << sha3_512_hex(b_cat) << "\",\"candidate_sha3_512\":\""
                << sha3_512_hex(c_cat) << "\"}";
            structured = oss.str();
          }
        }
      } else if (name == "t81.memory_search") {
        const std::string ip_path = arg_str(args, "index_pos_path");
        const std::string in_path = arg_str(args, "index_neg_path");
        const std::string qp_path = arg_str(args, "query_pos_path");
        const std::string qn_path = arg_str(args, "query_neg_path");
        uint64_t dim_u64 = 0;
        uint64_t top_k_u64 = 5;
        const bool dims_ok =
            arg_u64(args, "dim", 0, dim_u64) && arg_u64(args, "top_k", 5, top_k_u64);
        const size_t dim = static_cast<size_t>(dim_u64);
        const size_t top_k_req = static_cast<size_t>(top_k_u64);
        std::vector<uint8_t> ip, in, qp, qn;
        if (!dims_ok || dim == 0 || (dim % 64 != 0) || !read_file_bytes(jail, ip_path, ip) ||
            !read_file_bytes(jail, in_path, in) || !read_file_bytes(jail, qp_path, qp) ||
            !read_file_bytes(jail, qn_path, qn)) {
          structured = "{\"ok\":false,\"error\":\"invalid dim or unreadable index/query files\"}";
        } else {
          const size_t row_bytes = (dim / 64) * 8;
          if (ip.empty() || ip.size() != in.size() || (ip.size() % row_bytes != 0) ||
              qp.size() != row_bytes || qn.size() != row_bytes) {
            structured = "{\"ok\":false,\"error\":\"index/query byte size mismatch\"}";
          } else {
            const size_t W64 = dim / 64;
            const size_t docs = ip.size() / row_bytes;
            const size_t top_k = std::min(std::max<size_t>(top_k_req, 1), docs);
            std::vector<uint64_t> qpw(W64), qnw(W64);
            std::memcpy(qpw.data(), qp.data(), row_bytes);
            std::memcpy(qnw.data(), qn.data(), row_bytes);

            std::vector<int32_t> scores(docs, 0);
            for (size_t d = 0; d < docs; ++d) {
              int32_t acc = 0;
              const uint8_t* dp = ip.data() + d * row_bytes;
              const uint8_t* dn = in.data() + d * row_bytes;
              for (size_t w = 0; w < W64; ++w) {
                uint64_t bp = 0, bn = 0;
                std::memcpy(&bp, dp + w * 8, 8);
                std::memcpy(&bn, dn + w * 8, 8);
                acc += std::popcount((qpw[w] & bp) | (qnw[w] & bn)) -
                       std::popcount((qpw[w] & bn) | (qnw[w] & bp));
              }
              scores[d] = acc;
            }

            std::vector<size_t> idx(docs);
            std::iota(idx.begin(), idx.end(), 0);
            std::partial_sort(idx.begin(), idx.begin() + static_cast<std::ptrdiff_t>(top_k),
                              idx.end(), [&](size_t a, size_t b) {
                                if (scores[a] != scores[b]) return scores[a] > scores[b];
                                return a < b;
                              });

            std::vector<uint8_t> idx_cat = ip;
            idx_cat.insert(idx_cat.end(), in.begin(), in.end());
            const std::string idx_sha = sha3_512_hex(idx_cat);

            std::ostringstream res_arr;
            std::string rec_canon = idx_sha + "|";
            res_arr << "[";
            for (size_t i = 0; i < top_k; ++i) {
              if (i > 0) res_arr << ",";
              res_arr << "{\"doc_id\":" << idx[i] << ",\"score\":" << scores[idx[i]] << "}";
              rec_canon += std::to_string(idx[i]) + ":" + std::to_string(scores[idx[i]]) + ",";
            }
            res_arr << "]";
            const std::vector<uint8_t> rec_bytes(rec_canon.begin(), rec_canon.end());
            structured = "{\"ok\":true,\"docs_searched\":" + std::to_string(docs) +
                         ",\"dim\":" + std::to_string(dim) + ",\"top_k\":" + std::to_string(top_k) +
                         ",\"results\":" + res_arr.str() + ",\"index_sha3_512\":\"" + idx_sha +
                         "\",\"retrieval_receipt\":\"" + sha3_512_hex(rec_bytes) + "\"}";
          }
        }
      } else if (name == "t81.adapter_apply") {
        const std::string bp_path = arg_str(args, "base_pos_path");
        const std::string bn_path = arg_str(args, "base_neg_path");
        const std::string dp_path = arg_str(args, "delta_pos_path");
        const std::string dn_path = arg_str(args, "delta_neg_path");
        const std::string sbdr_path = arg_str(args, "delta_sbdr_path");
        const std::string op_path = arg_str(args, "out_pos_path");
        const std::string on_path = arg_str(args, "out_neg_path");
        double max_flip_pct = 100.0;
        const bool max_ok = arg_double(args, "max_flip_pct", 100.0, max_flip_pct) &&
                            max_flip_pct >= 0.0 && max_flip_pct <= 100.0;
        const bool use_sbdr = !sbdr_path.empty();
        std::vector<uint8_t> bp, bn, dp, dn, sbdr;
        bool io_ok = max_ok && !bp_path.empty() && !bn_path.empty() && !op_path.empty() &&
                     !on_path.empty() && read_file_bytes(jail, bp_path, bp) &&
                     read_file_bytes(jail, bn_path, bn);
        if (io_ok) {
          if (use_sbdr) {
            io_ok = read_file_bytes(jail, sbdr_path, sbdr);
          } else {
            io_ok = !dp_path.empty() && !dn_path.empty() && read_file_bytes(jail, dp_path, dp) &&
                    read_file_bytes(jail, dn_path, dn);
          }
        }
        if (!io_ok) {
          structured = "{\"ok\":false,\"applied\":false,\"verdict\":\"REJECT_IO_ERROR\"}";
        } else if (bp.empty() || bp.size() != bn.size() || (bp.size() % 8 != 0) ||
                   (!use_sbdr && (bp.size() != dp.size() || bp.size() != dn.size()))) {
          structured = "{\"ok\":false,\"applied\":false,\"verdict\":\"REJECT_SIZE_MISMATCH\"}";
        } else {
          const size_t words = bp.size() / 8;
          const uint64_t total_weights = static_cast<uint64_t>(words) * 64ULL;
          std::vector<uint8_t> op = bp;
          std::vector<uint8_t> on = bn;
          uint64_t flips = 0;
          uint64_t active_blocks = 0;
          std::string reject_verdict;

          // Validate base planes are disjoint
          for (size_t w = 0; w < words; ++w) {
            uint64_t wp = 0, wn = 0;
            std::memcpy(&wp, bp.data() + w * 8, 8);
            std::memcpy(&wn, bn.data() + w * 8, 8);
            if ((wp & wn) != 0ULL) {
              reject_verdict = "REJECT_NON_DISJOINT_PLANES";
              break;
            }
          }

          if (reject_verdict.empty() && use_sbdr) {
            // Canonical SBDR1 format: "SBDR1\0\0\0" (8B) | total_words (u64) |
            // mask[(W+63)/64] (u64[]) | active_blocks * (pos u64, neg u64)
            static constexpr uint8_t kMagic[8] = {'S', 'B', 'D', 'R', '1', 0, 0, 0};
            const size_t mask_words = (words + 63) / 64;
            const size_t header_bytes = 16 + mask_words * 8;
            if (sbdr.size() < header_bytes || std::memcmp(sbdr.data(), kMagic, 8) != 0) {
              reject_verdict = "REJECT_INVALID_SBDR1_HEADER";
            } else {
              uint64_t file_words = 0;
              std::memcpy(&file_words, sbdr.data() + 8, 8);
              if (file_words != static_cast<uint64_t>(words)) {
                reject_verdict = "REJECT_SIZE_MISMATCH";
              } else {
                for (size_t m = 0; m < mask_words; ++m) {
                  uint64_t mw = 0;
                  std::memcpy(&mw, sbdr.data() + 16 + m * 8, 8);
                  if (m + 1 == mask_words && (words % 64) != 0) {
                    const uint64_t valid_mask = (1ULL << (words % 64)) - 1ULL;
                    if ((mw & ~valid_mask) != 0ULL) {
                      reject_verdict = "REJECT_NON_CANONICAL_SBDR1_MASK";
                      break;
                    }
                  }
                  active_blocks += static_cast<uint64_t>(std::popcount(mw));
                }
                if (reject_verdict.empty() &&
                    sbdr.size() != header_bytes + static_cast<size_t>(active_blocks) * 16) {
                  reject_verdict = "REJECT_INVALID_SBDR1_LENGTH";
                }
              }
            }
            if (reject_verdict.empty()) {
              size_t payload_off = header_bytes;
              for (size_t m = 0; m < mask_words && reject_verdict.empty(); ++m) {
                uint64_t bits = 0;
                std::memcpy(&bits, sbdr.data() + 16 + m * 8, 8);
                while (bits != 0ULL) {
                  const unsigned tz = static_cast<unsigned>(std::countr_zero(bits));
                  const size_t w = (m << 6) + tz;
                  uint64_t wp = 0, wn = 0, ap = 0, an = 0;
                  std::memcpy(&wp, op.data() + w * 8, 8);
                  std::memcpy(&wn, on.data() + w * 8, 8);
                  std::memcpy(&ap, sbdr.data() + payload_off, 8);
                  std::memcpy(&an, sbdr.data() + payload_off + 8, 8);
                  payload_off += 16;
                  if ((ap & an) != 0ULL) {
                    reject_verdict = "REJECT_NON_DISJOINT_PLANES";
                    break;
                  }
                  if ((ap | an) == 0ULL) {
                    reject_verdict = "REJECT_NON_CANONICAL_SBDR1_ZERO_BLOCK";
                    break;
                  }
                  const uint64_t rp = (wp & ~an) | (ap & ~wn);
                  const uint64_t rn = (wn & ~ap) | (an & ~wp);
                  std::memcpy(op.data() + w * 8, &rp, 8);
                  std::memcpy(on.data() + w * 8, &rn, 8);
                  flips += static_cast<uint64_t>(std::popcount((wp ^ rp) | (wn ^ rn)));
                  bits &= (bits - 1ULL);
                }
              }
            }
          } else if (reject_verdict.empty()) {
            for (size_t w = 0; w < words; ++w) {
              uint64_t wp = 0, wn = 0, ap = 0, an = 0;
              std::memcpy(&wp, bp.data() + w * 8, 8);
              std::memcpy(&wn, bn.data() + w * 8, 8);
              std::memcpy(&ap, dp.data() + w * 8, 8);
              std::memcpy(&an, dn.data() + w * 8, 8);
              if ((ap & an) != 0ULL) {
                reject_verdict = "REJECT_NON_DISJOINT_PLANES";
                break;
              }
              if ((ap | an) != 0ULL) ++active_blocks;
              const uint64_t rp = (wp & ~an) | (ap & ~wn);
              const uint64_t rn = (wn & ~ap) | (an & ~wp);
              std::memcpy(op.data() + w * 8, &rp, 8);
              std::memcpy(on.data() + w * 8, &rn, 8);
              flips += static_cast<uint64_t>(std::popcount((wp ^ rp) | (wn ^ rn)));
            }
          }

          if (!reject_verdict.empty()) {
            structured = "{\"ok\":false,\"applied\":false,\"verdict\":\"" + reject_verdict + "\"}";
          } else {
            const double flip_pct =
                100.0 * static_cast<double>(flips) / static_cast<double>(total_weights);
            const bool within_budget = (flip_pct <= max_flip_pct);
            std::vector<uint8_t> b_cat = bp;
            b_cat.insert(b_cat.end(), bn.begin(), bn.end());
            std::vector<uint8_t> m_cat = op;
            m_cat.insert(m_cat.end(), on.begin(), on.end());
            const std::string b_sha = sha3_512_hex(b_cat);
            std::string d_sha;
            if (use_sbdr) {
              d_sha = sha3_512_hex(sbdr);
            } else {
              std::vector<uint8_t> d_cat = dp;
              d_cat.insert(d_cat.end(), dn.begin(), dn.end());
              d_sha = sha3_512_hex(d_cat);
            }
            const std::string m_sha = sha3_512_hex(m_cat);
            const std::string rec_in =
                b_sha + "|" + d_sha + "|" + m_sha + "|" + std::to_string(flips);
            const std::vector<uint8_t> rec_bytes(rec_in.begin(), rec_in.end());
            const std::string adapter_rec = sha3_512_hex(rec_bytes);

            bool wrote_ok = false;
            if (within_budget) {
              wrote_ok = write_file_bytes_atomic(jail, op_path, op) &&
                         write_file_bytes_atomic(jail, on_path, on);
            }
            const bool ok = within_budget && wrote_ok;
            const std::string verdict =
                !within_budget ? "DENY_EXCEEDS_FLIP_BUDGET"
                               : (wrote_ok ? "APPLIED_WITHIN_BUDGET" : "REJECT_WRITE_ERROR");
            const double dense_bytes = static_cast<double>(bp.size() + bn.size());
            const double delta_bytes = use_sbdr ? static_cast<double>(sbdr.size()) : dense_bytes;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(4) << "{\"ok\":" << (ok ? "true" : "false")
                << ",\"applied\":" << (ok ? "true" : "false") << ",\"verdict\":\"" << verdict
                << "\",\"delta_format\":\"" << (use_sbdr ? "sbdr1" : "dense")
                << "\",\"total_weights\":" << total_weights
                << ",\"active_blocks\":" << active_blocks
                << ",\"compression_ratio\":" << (dense_bytes / std::max(1.0, delta_bytes))
                << ",\"effective_flips\":" << flips << ",\"flip_pct\":" << flip_pct
                << ",\"max_flip_pct\":" << max_flip_pct << ",\"base_sha3_512\":\"" << b_sha
                << "\",\"delta_sha3_512\":\"" << d_sha << "\",\"merged_sha3_512\":\"" << m_sha
                << "\",\"adapter_receipt\":\"" << adapter_rec << "\"}";
            structured = oss.str();
          }
        }
      }
      bool is_err = true;
      std::string err_tag = "ERR";
      {
        JsonNode parsed_out;
        ScopedJsonParser out_parser(structured);
        if (out_parser.parse(parsed_out) && parsed_out.kind == JsonNode::Kind::Object) {
          const auto ok_it = parsed_out.obj.find("ok");
          if (ok_it != parsed_out.obj.end() && ok_it->second.kind == JsonNode::Kind::Bool) {
            is_err = (ok_it->second.str != "true");
          } else {
            is_err = false;
          }
          const std::string c = arg_str(parsed_out, "code");
          const std::string v = arg_str(parsed_out, "verdict");
          if (!c.empty())
            err_tag = c;
          else if (!v.empty())
            err_tag = v;
        }
      }
      const bool compact_mode = arg_bool(args, "compact", env_compact_mode());
      const std::string text_payload =
          compact_mode ? (is_err ? ("err:" + err_tag) : "ok") : structured;
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"content\":[{\"type\":\"text\",\"text\":\""
                << json_escape_mcp(text_payload)
                << "\"}],\"isError\":" << (is_err ? "true" : "false")
                << ",\"structured\":" << structured << "}}\n"
                << std::flush;
    } else {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"error\":{\"code\":-32601,\"message\":\"unknown method\"}}\n"
                << std::flush;
    }
  }
  return 0;
}

}  // namespace t81::cli
