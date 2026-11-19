#include "tools/cli/core/mcp_server.hpp"

#include "t81/axion/policy.hpp"
#include "t81/crypto/sha3.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string_view>

namespace t81::cli {

namespace {

std::string json_escape_mcp(std::string_view input) {
  std::string out;
  out.reserve(input.size() + 8);
  for (char c : input) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string extract_str(const std::string& json, const std::string& key) {
  const std::string pat = "\"" + key + "\"";
  const size_t kpos = json.find(pat);
  if (kpos == std::string::npos) return {};
  const size_t colon = json.find(':', kpos + pat.size());
  if (colon == std::string::npos) return {};
  const size_t q1 = json.find('"', colon + 1);
  if (q1 == std::string::npos) return {};
  std::string out;
  for (size_t i = q1 + 1; i < json.size(); ++i) {
    if (json[i] == '\\' && i + 1 < json.size()) {
      out.push_back(json[++i]);
    } else if (json[i] == '"') {
      break;
    } else {
      out.push_back(json[i]);
    }
  }
  return out;
}

std::string extract_id(const std::string& json) {
  const size_t kpos = json.find("\"id\"");
  if (kpos == std::string::npos) return "null";
  const size_t colon = json.find(':', kpos + 4);
  if (colon == std::string::npos) return "null";
  size_t i = colon + 1;
  while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) ++i;
  size_t end = i;
  while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ' ') ++end;
  return end > i ? json.substr(i, end - i) : "null";
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

int mode_rank(const std::string& m) {
  if (m == "observe-only") return 0;
  if (m == "warn") return 1;
  if (m == "fail-closed") return 2;
  return -1;
}

std::string resolve_floor(const std::string& policy_text, const std::string& action_type) {
  if (!action_type.empty()) {
    for (const char* m : {"observe-only", "warn", "fail-closed"}) {
      const std::string needle = "action-policy:" + action_type + "=" + m;
      if (policy_text.find(needle) != std::string::npos) return m;
    }
  }
  for (const char* m : {"observe-only", "warn", "fail-closed"}) {
    const std::string needle = std::string("enforcement-mode:") + m;
    if (policy_text.find(needle) != std::string::npos) return m;
  }
  return "fail-closed";
}

}  // namespace

int run_mcp_serve(const std::vector<std::string>& command_args,
                  const std::function<int(const std::string&)>& run_canonfs_fsck_json) {
  if (command_args.empty() || command_args[0] != "serve") {
    std::cerr << "Usage: t81 mcp serve\n";
    return 1;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    const std::string id = extract_id(line);
    const std::string method = extract_str(line, "method");
    if (method == "initialize") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"protocolVersion\":\"2024-11-05\",\"serverInfo\":{\"name\":\"t81-native-mcp\",\"version\":\"2.0.0\"},\"capabilities\":{\"tools\":{}}}}\n"
                << std::flush;
    } else if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"tools\":["
                << "{\"name\":\"t81.policy_validate\",\"description\":\"Validate an Axion .apl policy in-process.\"},"
                << "{\"name\":\"t81.policy_test\",\"description\":\"Evaluate a SHA3-512 hash against an Axion .apl policy in-process.\"},"
                << "{\"name\":\"t81.get_effective_policy\",\"description\":\"Introspect authoritative .apl policy floor and SHA3-512 digest in-process.\"},"
                << "{\"name\":\"t81.gate_check\",\"description\":\"Evaluate artifact admission with anti-downgrade per-action enforcement floor in-process.\"},"
                << "{\"name\":\"t81.canonfs_fsck\",\"description\":\"Verify CanonFS object hashes and snapshots in-process.\"},"
                << "{\"name\":\"t81.determinism_hash\",\"description\":\"Compute SHA3-512 digest of a file in-process.\"}"
                << "]}}\n"
                << std::flush;
    } else if (method == "tools/call") {
      const std::string name = extract_str(line, "name");
      std::string structured = "{\"ok\":false,\"error\":\"unknown tool\"}";
      if (name == "t81.policy_validate" || name == "t81.policy_test" ||
          name == "t81.get_effective_policy") {
        const std::string ppath = extract_str(line, "policy_path");
        std::ifstream ifs(ppath, std::ios::binary);
        if (!ifs) {
          structured = "{\"ok\":false,\"valid\":false,\"error\":\"could not open policy file\"}";
        } else {
          std::vector<uint8_t> pbytes((std::istreambuf_iterator<char>(ifs)),
                                      std::istreambuf_iterator<char>());
          std::string content(pbytes.begin(), pbytes.end());
          auto res = t81::axion::parse_policy(content);
          if (!res) {
            structured = "{\"ok\":false,\"valid\":false,\"error\":\"" +
                         json_escape_mcp(res.error()) + "\"}";
          } else if (name == "t81.policy_validate") {
            structured = "{\"ok\":true,\"valid\":true,\"tier\":" +
                         std::to_string(res.value().tier) + "}";
          } else if (name == "t81.get_effective_policy") {
            const std::string action_type = extract_str(line, "action_type");
            const std::string floor = resolve_floor(content, action_type);
            structured = "{\"ok\":true,\"policy_hash\":\"" + sha3_512_hex(pbytes) +
                         "\",\"action_type\":\"" + json_escape_mcp(action_type) +
                         "\",\"policy_floor\":\"" + floor +
                         "\",\"allowed_hash_count\":" +
                         std::to_string(res.value().allowed_tensor_hashes.size()) + "}";
          } else {
            const std::string mhash = extract_str(line, "model_hash");
            bool allowed = false;
            for (const auto& h : res.value().allowed_tensor_hashes) {
              if (h == mhash) {
                allowed = true;
                break;
              }
            }
            structured = std::string("{\"ok\":") + (allowed ? "true" : "false") +
                         ",\"admitted\":" + (allowed ? "true" : "false") + "}";
          }
        }
      } else if (name == "t81.gate_check") {
        const std::string apath = extract_str(line, "artifact_path");
        const std::string ppath = extract_str(line, "policy_path");
        const std::string action_type = extract_str(line, "action_type");
        const std::string req_mode = extract_str(line, "mode");
        std::ifstream aifs(apath, std::ios::binary);
        std::ifstream pifs(ppath, std::ios::binary);
        if (!aifs || !pifs) {
          structured = "{\"ok\":false,\"decision\":\"DENY\",\"blocked\":true,\"error\":\"missing artifact or policy file\"}";
        } else {
          std::vector<uint8_t> abytes((std::istreambuf_iterator<char>(aifs)),
                                      std::istreambuf_iterator<char>());
          std::vector<uint8_t> pbytes((std::istreambuf_iterator<char>(pifs)),
                                      std::istreambuf_iterator<char>());
          std::string content(pbytes.begin(), pbytes.end());
          auto res = t81::axion::parse_policy(content);
          if (!res) {
            structured = "{\"ok\":false,\"decision\":\"DENY\",\"policy_floor\":\"fail-closed\",\"mode_applied\":\"fail-closed\",\"downgrade_attempt_blocked\":false,\"blocked\":true,\"error\":\"" +
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
            bool admitted = false;
            for (const auto& h : res.value().allowed_tensor_hashes) {
              if (h == asha) {
                admitted = true;
                break;
              }
            }
            const bool blocked = (!admitted && eff == "fail-closed");
            const std::string decision =
                admitted ? "ALLOW" : (eff == "warn" ? "WARN" : "DENY");
            structured = std::string("{\"ok\":") + (blocked ? "false" : "true") +
                         ",\"decision\":\"" + decision +
                         "\",\"policy_floor\":\"" + floor +
                         "\",\"mode_applied\":\"" + eff +
                         "\",\"downgrade_attempt_blocked\":" +
                         (downgrade_blocked ? "true" : "false") +
                         ",\"blocked\":" + (blocked ? "true" : "false") +
                         ",\"artifact_sha3_512\":\"" + asha +
                         "\",\"policy_hash\":\"" + psha + "\"}";
          }
        }
      } else if (name == "t81.canonfs_fsck") {
        const std::string croot = extract_str(line, "canonfs_root");
        std::ostringstream cap;
        std::streambuf* old_buf = std::cout.rdbuf(cap.rdbuf());
        const int rc = run_canonfs_fsck_json(croot);
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
      } else if (name == "t81.determinism_hash") {
        const std::string fpath = extract_str(line, "path");
        std::ifstream ifs(fpath, std::ios::binary);
        if (!ifs) {
          structured = "{\"ok\":false,\"error\":\"could not open file\"}";
        } else {
          std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
          structured = "{\"ok\":true,\"sha3_512\":\"" + sha3_512_hex(bytes) + "\"}";
        }
      }
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"structured\":" << structured << "}}\n"
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
