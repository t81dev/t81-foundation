#include "tools/cli/core/mcp_server.hpp"

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace t81::cli {

namespace {

namespace fs = std::filesystem;

struct ActiveTx {
  std::string tx_id;
  fs::path target_path;
  fs::path backup_path;
  std::string before_sha3_512;
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

bool read_file_bytes(const fs::path& path, std::vector<uint8_t>& out) {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs) return false;
  out.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  return true;
}

bool write_file_bytes(const fs::path& path, const std::vector<uint8_t>& data) {
  std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
  if (!ofs) return false;
  if (!data.empty()) {
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
  }
  return ofs.good();
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

std::string trim_ws(std::string s) {
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
  return s;
}

std::string read_git_head_sha(const fs::path& repo_root, std::string& ref_out) {
  const fs::path head_path = repo_root / ".git" / "HEAD";
  std::ifstream ifs(head_path);
  if (!ifs) return "unknown";
  std::string line;
  std::getline(ifs, line);
  line = trim_ws(line);
  if (line.rfind("ref: ", 0) == 0) {
    ref_out = line.substr(5);
    std::ifstream rfs(repo_root / ".git" / ref_out);
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
         "before mutation.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"target_path\",\"backup_dir\"],"
         "\"properties\":{\"target_path\":{\"type\":\"string\"},\"backup_dir\":{\"type\":"
         "\"string\"}}}},"

         "{\"name\":\"t81.tx_commit\","
         "\"description\":\"Commit an active file transaction after optional .apl policy check on "
         "the mutated artifact.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"tx_id\"],"
         "\"properties\":{\"tx_id\":{\"type\":\"string\"},\"policy_path\":{\"type\":\"string\"}}}},"

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
         "\"type\":\"string\"},\"max_flip_pct\":{\"type\":\"string\"}}}},"

         "{\"name\":\"t81.memory_search\","
         "\"description\":\"Zero-float deterministic top-K similarity search over a Dual-Rail "
         "(P,N) agent memory index, returning exact integer scores and a SHA3-512 retrieval "
         "receipt.\","
         "\"inputSchema\":{\"type\":\"object\",\"required\":[\"index_pos_path\","
         "\"index_neg_path\",\"query_pos_path\",\"query_neg_path\",\"dim\"],"
         "\"properties\":{\"index_pos_path\":{\"type\":\"string\"},\"index_neg_path\":{\"type\":"
         "\"string\"},\"query_pos_path\":{\"type\":\"string\"},\"query_neg_path\":{\"type\":"
         "\"string\"},\"dim\":{\"type\":\"string\"},\"top_k\":{\"type\":\"string\"}}}},"

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
         "\"string\"}}}}"
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
         "\"dualrail_crossover\":{\"min_k\":64,\"min_mn\":64,\"reuse_k_star\":6,"
         "\"gpu_fused_min_matmuls\":4}}";
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
  for (const auto& insn : prog.insns) {
    if (saw_halt) return {false, "REJECT_TRAILING_CODE_AFTER_HALT", matmuls};
    switch (insn.opcode) {
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
  if (matmuls == 0) return {false, "REJECT_NO_COMPUTE", 0};
  return {true, "VERIFIED_OK", matmuls};
}

}  // namespace

int run_mcp_serve(const std::vector<std::string>& command_args,
                  const std::function<int(const std::string&)>& run_canonfs_fsck_json) {
  if (command_args.empty() || command_args[0] != "serve") {
    std::cerr << "Usage: t81 mcp serve\n"
              << "Run 't81 help mcp' to list all 15 native MCP tools.\n";
    return 1;
  }

  std::unordered_map<std::string, ActiveTx> active_txs;
  uint64_t tx_counter = 0;

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    const std::string id = extract_id(line);
    const std::string method = extract_str(line, "method");
    // Per JSON-RPC 2.0 / MCP spec, notifications carry no id and must not receive a response.
    if (id == "null" || method.rfind("notifications/", 0) == 0) {
      continue;
    }
    if (method == "initialize") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"protocolVersion\":\"2024-11-05\",\"serverInfo\":{\"name\":\"t81-"
                   "native-mcp\",\"version\":\"2.0.1\"},\"capabilities\":{\"tools\":{}}}}\n"
                << std::flush;
    } else if (method == "ping") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":{}}\n" << std::flush;
    } else if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"tools\":" << build_tools_list_json() << "}}\n"
                << std::flush;
    } else if (method == "tools/call") {
      const std::string name = extract_str(line, "name");
      std::string structured = "{\"ok\":false,\"error\":\"unknown tool\",\"hint\":\"call "
                               "tools/list to inspect valid tool names and inputSchema\"}";

      if (name == "t81.policy_validate" || name == "t81.policy_test" ||
          name == "t81.get_effective_policy") {
        const std::string ppath = extract_str(line, "policy_path");
        if (ppath.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required argument: "
                       "policy_path\",\"expected\":[\"policy_path\"]}";
        } else {
          std::vector<uint8_t> pbytes;
          if (!read_file_bytes(ppath, pbytes)) {
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
              const std::string action_type = extract_str(line, "action_type");
              const std::string floor = resolve_floor(content, action_type);
              structured = "{\"ok\":true,\"policy_hash\":\"" + sha3_512_hex(pbytes) +
                           "\",\"action_type\":\"" + json_escape_mcp(action_type) +
                           "\",\"policy_floor\":\"" + floor + "\",\"allowed_hash_count\":" +
                           std::to_string(policy_allowed_hash_count(res.value())) + "}";
            } else {
              const std::string mhash = extract_str(line, "model_hash");
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
        const std::string apath = extract_str(line, "artifact_path");
        const std::string ppath = extract_str(line, "policy_path");
        const std::string action_type = extract_str(line, "action_type");
        const std::string req_mode = extract_str(line, "mode");
        std::vector<uint8_t> abytes, pbytes;
        if (apath.empty() || ppath.empty()) {
          structured = "{\"ok\":false,\"decision\":\"DENY\",\"blocked\":true,\"error\":\"missing "
                       "required arguments\",\"expected\":[\"artifact_path\",\"policy_path\"]}";
        } else if (!read_file_bytes(apath, abytes) || !read_file_bytes(ppath, pbytes)) {
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
        const std::string croot = extract_str(line, "canonfs_root");
        if (croot.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required argument: "
                       "canonfs_root\",\"expected\":[\"canonfs_root\"]}";
        } else {
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
        }
      } else if (name == "t81.determinism_hash") {
        const std::string fpath = extract_str(line, "path");
        std::vector<uint8_t> bytes;
        if (fpath.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required argument: "
                       "path\",\"expected\":[\"path\"]}";
        } else if (!read_file_bytes(fpath, bytes)) {
          structured = "{\"ok\":false,\"error\":\"could not open file\"}";
        } else {
          structured = "{\"ok\":true,\"sha3_512\":\"" + sha3_512_hex(bytes) + "\"}";
        }
      } else if (name == "t81.session_bootstrap") {
        const std::string root_arg = extract_str(line, "repo_root");
        const fs::path root = root_arg.empty() ? fs::current_path() : fs::path(root_arg);
        std::string git_ref;
        const std::string head_sha = read_git_head_sha(root, git_ref);
        const fs::path lock_path = root / "docs" / "governance" / "PUBLIC_API_LOCK.json";
        const fs::path reg_path = root / "docs" / "governance" / "DETERMINISM_SURFACE_REGISTRY.md";
        std::vector<uint8_t> lock_bytes;
        const bool lock_ok = read_file_bytes(lock_path, lock_bytes);
        const bool reg_ok = fs::exists(reg_path);
        structured =
            "{\"ok\":true,\"version\":\"2.0.1\",\"repo_root\":\"" + json_escape_mcp(root.string()) +
            "\",\"git_ref\":\"" + json_escape_mcp(git_ref) + "\",\"git_head_sha\":\"" +
            json_escape_mcp(head_sha) +
            "\",\"public_api_lock_present\":" + (lock_ok ? "true" : "false") +
            ",\"public_api_lock_sha3_512\":\"" + (lock_ok ? sha3_512_hex(lock_bytes) : "") +
            "\",\"determinism_registry_present\":" + (reg_ok ? "true" : "false") +
            ",\"native_mcp_tools\":15,\"capabilities\":" + detect_host_capabilities_json() + "}";
      } else if (name == "t81.tx_begin") {
        const std::string tpath = extract_str(line, "target_path");
        const std::string bdir = extract_str(line, "backup_dir");
        std::vector<uint8_t> bytes;
        if (tpath.empty() || bdir.empty()) {
          structured = "{\"ok\":false,\"error\":\"missing required "
                       "arguments\",\"expected\":[\"target_path\",\"backup_dir\"]}";
        } else if (!read_file_bytes(tpath, bytes)) {
          structured = "{\"ok\":false,\"error\":\"could not read target_path\"}";
        } else {
          std::error_code ec;
          fs::create_directories(bdir, ec);
          const std::string tx_id = "tx-" + std::to_string(++tx_counter);
          const fs::path bpath = fs::path(bdir) / (tx_id + ".bak");
          if (!write_file_bytes(bpath, bytes)) {
            structured = "{\"ok\":false,\"error\":\"could not write transaction backup\"}";
          } else {
            const std::string before_sha = sha3_512_hex(bytes);
            active_txs[tx_id] = ActiveTx{tx_id, fs::path(tpath), bpath, before_sha};
            structured = "{\"ok\":true,\"tx_id\":\"" + tx_id + "\",\"before_sha3_512\":\"" +
                         before_sha + "\",\"backup_path\":\"" + json_escape_mcp(bpath.string()) +
                         "\"}";
          }
        }
      } else if (name == "t81.tx_abort") {
        const std::string tx_id = extract_str(line, "tx_id");
        auto it = active_txs.find(tx_id);
        if (it == active_txs.end()) {
          structured = "{\"ok\":false,\"error\":\"unknown or closed tx_id\"}";
        } else {
          std::vector<uint8_t> bbytes;
          if (!read_file_bytes(it->second.backup_path, bbytes) ||
              sha3_512_hex(bbytes) != it->second.before_sha3_512) {
            structured = "{\"ok\":false,\"error\":\"backup integrity verification failed (SHA3-512 "
                         "mismatch)\"}";
          } else if (!write_file_bytes(it->second.target_path, bbytes)) {
            structured = "{\"ok\":false,\"error\":\"failed restoring target_path\"}";
          } else {
            std::error_code ec;
            fs::remove(it->second.backup_path, ec);
            const std::string restored_sha = it->second.before_sha3_512;
            active_txs.erase(it);
            structured = "{\"ok\":true,\"tx_id\":\"" + tx_id +
                         "\",\"aborted\":true,\"restored_sha3_512\":\"" + restored_sha + "\"}";
          }
        }
      } else if (name == "t81.tx_commit") {
        const std::string tx_id = extract_str(line, "tx_id");
        const std::string ppath = extract_str(line, "policy_path");
        auto it = active_txs.find(tx_id);
        if (it == active_txs.end()) {
          structured = "{\"ok\":false,\"error\":\"unknown or closed tx_id\"}";
        } else {
          std::vector<uint8_t> after_bytes;
          if (!read_file_bytes(it->second.target_path, after_bytes)) {
            structured = "{\"ok\":false,\"error\":\"could not read mutated target_path\"}";
          } else {
            const std::string after_sha = sha3_512_hex(after_bytes);
            bool policy_ok = true;
            std::string policy_err;
            if (!ppath.empty()) {
              std::vector<uint8_t> pbytes;
              if (!read_file_bytes(ppath, pbytes)) {
                policy_ok = false;
                policy_err = "could not read policy_path";
              } else {
                std::string ptext(pbytes.begin(), pbytes.end());
                auto pres = t81::axion::parse_policy(ptext);
                if (!pres) {
                  policy_ok = false;
                  policy_err = pres.error();
                } else {
                  if (!policy_admits_hash(pres.value(), after_sha)) {
                    policy_ok = false;
                    policy_err = "mutated target_path SHA3-512 denied by policy; call t81.tx_abort "
                                 "to restore";
                  }
                }
              }
            }
            if (!policy_ok) {
              structured = "{\"ok\":false,\"committed\":false,\"tx_id\":\"" + tx_id +
                           "\",\"error\":\"" + json_escape_mcp(policy_err) + "\"}";
            } else {
              std::error_code ec;
              fs::remove(it->second.backup_path, ec);
              const std::string before_sha = it->second.before_sha3_512;
              const bool changed = (before_sha != after_sha);
              active_txs.erase(it);
              structured = "{\"ok\":true,\"committed\":true,\"tx_id\":\"" + tx_id +
                           "\",\"before_sha3_512\":\"" + before_sha + "\",\"after_sha3_512\":\"" +
                           after_sha + "\",\"changed\":" + (changed ? "true" : "false") + "}";
            }
          }
        }
      } else if (name == "t81.verify_subgraph") {
        const std::string bc_path = extract_str(line, "bytecode_path");
        const std::string bc_hex = extract_str(line, "bytecode_hex");
        std::vector<uint8_t> raw;
        bool loaded_ok = false;
        if (!bc_hex.empty()) {
          loaded_ok = decode_hex_bytes(bc_hex, raw);
        } else if (!bc_path.empty()) {
          loaded_ok = read_file_bytes(bc_path, raw);
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
        const std::string bc_path = extract_str(line, "bytecode_path");
        const std::string bc_hex = extract_str(line, "bytecode_hex");
        const std::string ppath = extract_str(line, "policy_path");
        std::vector<uint8_t> raw;
        bool loaded_ok = false;
        if (!bc_hex.empty()) {
          loaded_ok = decode_hex_bytes(bc_hex, raw);
        } else if (!bc_path.empty()) {
          loaded_ok = read_file_bytes(bc_path, raw);
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
            } else {
              t81::axion::Policy pol;
              bool pol_ok = true;
              if (!ppath.empty()) {
                std::vector<uint8_t> pbytes;
                if (!read_file_bytes(ppath, pbytes)) {
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
                auto vm = t81::vm::make_interpreter_vm(std::make_unique<t81::axion::AIHookEngine>(
                    std::make_unique<t81::axion::PolicyEngine>(pol)));
                vm->load_program(dec.value());
                const auto run_res = vm->run_to_halt();
                const bool halted = run_res.has_value();
                const int trap_code = halted ? 0 : static_cast<int>(run_res.error());
                const size_t ax_events = vm->state().axion_log.size();
                const std::string rec_in = prog_sha + "|" + (halted ? "HALT" : "TRAP") + "|" +
                                           std::to_string(trap_code) + "|" +
                                           std::to_string(ax_events);
                const std::vector<uint8_t> rec_bytes(rec_in.begin(), rec_in.end());
                const std::string exec_rec = sha3_512_hex(rec_bytes);
                structured = "{\"ok\":true,\"verdict\":\"VERIFIED_AND_EXECUTED\","
                             "\"executed\":true,\"halted\":" +
                             std::string(halted ? "true" : "false") +
                             ",\"trap_code\":" + std::to_string(trap_code) +
                             ",\"instruction_count\":" + std::to_string(dec.value().insns.size()) +
                             ",\"matmul_count\":" + std::to_string(vr.matmul_count) +
                             ",\"axion_events\":" + std::to_string(ax_events) +
                             ",\"program_sha3_512\":\"" + prog_sha + "\",\"execution_receipt\":\"" +
                             exec_rec + "\"}";
              }
            }
          }
        }
      } else if (name == "t81.diff_check") {
        const std::string bp_path = extract_str(line, "base_pos_path");
        const std::string bn_path = extract_str(line, "base_neg_path");
        const std::string cp_path = extract_str(line, "candidate_pos_path");
        const std::string cn_path = extract_str(line, "candidate_neg_path");
        const std::string max_str = extract_str(line, "max_flip_pct");
        const double max_flip_pct = max_str.empty() ? 1.0 : std::strtod(max_str.c_str(), nullptr);
        std::vector<uint8_t> bp, bn, cp, cn;
        if (bp_path.empty() || bn_path.empty() || cp_path.empty() || cn_path.empty() ||
            !read_file_bytes(bp_path, bp) || !read_file_bytes(bn_path, bn) ||
            !read_file_bytes(cp_path, cp) || !read_file_bytes(cn_path, cn)) {
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
        const std::string ip_path = extract_str(line, "index_pos_path");
        const std::string in_path = extract_str(line, "index_neg_path");
        const std::string qp_path = extract_str(line, "query_pos_path");
        const std::string qn_path = extract_str(line, "query_neg_path");
        const std::string dim_str = extract_str(line, "dim");
        const std::string k_str = extract_str(line, "top_k");
        const size_t dim = static_cast<size_t>(std::strtoull(dim_str.c_str(), nullptr, 10));
        const size_t top_k_req =
            k_str.empty() ? 5 : static_cast<size_t>(std::strtoull(k_str.c_str(), nullptr, 10));
        std::vector<uint8_t> ip, in, qp, qn;
        if (dim == 0 || (dim % 64 != 0) || !read_file_bytes(ip_path, ip) ||
            !read_file_bytes(in_path, in) || !read_file_bytes(qp_path, qp) ||
            !read_file_bytes(qn_path, qn)) {
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
        const std::string bp_path = extract_str(line, "base_pos_path");
        const std::string bn_path = extract_str(line, "base_neg_path");
        const std::string dp_path = extract_str(line, "delta_pos_path");
        const std::string dn_path = extract_str(line, "delta_neg_path");
        const std::string sbdr_path = extract_str(line, "delta_sbdr_path");
        const std::string op_path = extract_str(line, "out_pos_path");
        const std::string on_path = extract_str(line, "out_neg_path");
        const std::string max_str = extract_str(line, "max_flip_pct");
        const double max_flip_pct = max_str.empty() ? 100.0 : std::strtod(max_str.c_str(), nullptr);
        const bool use_sbdr = !sbdr_path.empty();
        std::vector<uint8_t> bp, bn, dp, dn, sbdr;
        bool io_ok = !bp_path.empty() && !bn_path.empty() && !op_path.empty() && !on_path.empty() &&
                     read_file_bytes(bp_path, bp) && read_file_bytes(bn_path, bn);
        if (io_ok) {
          if (use_sbdr) {
            io_ok = read_file_bytes(sbdr_path, sbdr);
          } else {
            io_ok = !dp_path.empty() && !dn_path.empty() && read_file_bytes(dp_path, dp) &&
                    read_file_bytes(dn_path, dn);
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
              wrote_ok = write_file_bytes(op_path, op) && write_file_bytes(on_path, on);
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
      const bool is_err = (structured.find("\"ok\":false") != std::string::npos);
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id
                << ",\"result\":{\"content\":[{\"type\":\"text\",\"text\":\""
                << json_escape_mcp(structured) << "\"}],\"isError\":" << (is_err ? "true" : "false")
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
