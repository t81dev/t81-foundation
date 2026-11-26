// src/provider/dpe_mcp_coordinator.cpp

#include "provider/dpe_mcp_coordinator.hpp"

#include <algorithm>
#include <cstring>
#include <future>
#include <map>
#include <queue>
#include <span>
#include <unordered_set>

#include "provider/provider_governor.hpp"
#include "t81/crypto/sha3.hpp"

namespace t81::provider {
namespace {

std::string sha3_hex_str(const std::string& s) {
  return "sha3-512:" + t81::crypto::sha3_512_hex(std::span<const uint8_t>(
                           reinterpret_cast<const uint8_t*>(s.data()), s.size()));
}

}  // namespace

DpeMcpCoordinator::DpeMcpCoordinator(std::shared_ptr<t81::axion::PolicyEngine> policy_engine,
                                     std::shared_ptr<t81::canonfs::Driver> canonfs_driver,
                                     std::size_t max_workers)
    : policy_engine_(std::move(policy_engine)),
      canonfs_driver_(std::move(canonfs_driver)),
      max_workers_(std::max<std::size_t>(1, max_workers)),
      signing_kp_(t81::crypto::ed25519_generate_keypair()) {}

DpeMcpCommitOutcome DpeMcpCoordinator::execute_and_commit(const std::vector<DpeAgentTask>& tasks) {
  DpeMcpCommitOutcome out;
  if (!policy_engine_ || !canonfs_driver_) {
    out.code = "ERR_POLICY_DENIED";
    out.error = "missing policy engine or CanonFS driver";
    return out;
  }
  if (tasks.empty()) {
    out.code = "ERR_EMPTY_DAG";
    out.error = "DPE task graph is empty";
    return out;
  }

  // 1. Index tasks and compute topological levels (Kahn's algorithm).
  std::unordered_map<std::string, std::size_t> id_to_idx;
  for (std::size_t i = 0; i < tasks.size(); ++i) {
    if (tasks[i].task_id.empty() || !id_to_idx.emplace(tasks[i].task_id, i).second) {
      out.code = "ERR_DPE_INVALID_DAG";
      out.error = "duplicate or empty task_id";
      return out;
    }
  }
  std::vector<std::size_t> indeg(tasks.size(), 0);
  std::vector<std::vector<std::size_t>> adj(tasks.size());
  for (std::size_t i = 0; i < tasks.size(); ++i) {
    for (const auto& dep : tasks[i].dep_task_ids) {
      auto it = id_to_idx.find(dep);
      if (it == id_to_idx.end()) {
        out.code = "ERR_DPE_MISSING_DEP";
        out.error = "missing dependency " + dep;
        return out;
      }
      adj[it->second].push_back(i);
      ++indeg[i];
    }
  }

  std::vector<std::vector<std::size_t>> levels;
  std::vector<std::size_t> frontier;
  for (std::size_t i = 0; i < tasks.size(); ++i) {
    if (indeg[i] == 0) frontier.push_back(i);
  }
  std::size_t visited = 0;
  while (!frontier.empty()) {
    // Canonical deterministic ordering within each level by (agent_id, task_id).
    std::sort(frontier.begin(), frontier.end(), [&](std::size_t a, std::size_t b) {
      if (tasks[a].agent_id != tasks[b].agent_id) return tasks[a].agent_id < tasks[b].agent_id;
      return tasks[a].task_id < tasks[b].task_id;
    });
    levels.push_back(frontier);
    visited += frontier.size();
    std::vector<std::size_t> next_frontier;
    for (std::size_t u : frontier) {
      for (std::size_t v : adj[u]) {
        if (--indeg[v] == 0) next_frontier.push_back(v);
      }
    }
    frontier = std::move(next_frontier);
  }
  if (visited != tasks.size()) {
    out.code = "ERR_DPE_CYCLE";
    out.error = "cycle detected in DPE task DAG";
    return out;
  }

  // 2. Pre-execution fail-closed Axion policy check & intra-level write-set conflict check.
  const auto* active_pol = policy_engine_->active_policy();
  const std::string policy_sha =
      active_pol ? ProviderGovernor::compute_policy_sha3_512(*active_pol) : "";
  for (const auto& lvl : levels) {
    std::unordered_map<std::string, uint32_t> key_owner;
    for (std::size_t idx : lvl) {
      const auto& t = tasks[idx];
      t81::axion::SyscallContext sctx;
      sctx.caller = "agent-" + std::to_string(t.agent_id);
      sctx.syscall = "dpe:task_commit";
      sctx.payload = t.task_id;
      sctx.final_authorization = true;
      auto verdict = policy_engine_->evaluate(sctx);
      if (verdict.kind != t81::axion::VerdictKind::Allow) {
        out.code = "ERR_POLICY_DENIED";
        out.error = "Axion denied DPE task " + t.task_id + ": " + verdict.reason;
        return out;
      }
      for (const auto& k : t.write_keys) {
        auto [it, inserted] = key_owner.emplace(k, t.agent_id);
        if (!inserted) {
          out.code = "ERR_DPE_TX_CONFLICT";
          out.conflict_winner_agent = std::min(it->second, t.agent_id);
          out.error = "intra-level write-write conflict on key '" + k + "' between agents " +
                      std::to_string(it->second) + " and " + std::to_string(t.agent_id);
          return out;
        }
      }
    }
  }

  // 3. Execute levels concurrently across bounded workers, staging canonical deltas.
  std::map<std::string, std::string> committed_kv;
  std::vector<t81::crypto::AccountableBundleNode> dag_nodes;
  dag_nodes.reserve(tasks.size());

  for (const auto& lvl : levels) {
    std::vector<std::unordered_map<std::string, std::string>> lvl_results(lvl.size());
    std::size_t cursor = 0;
    while (cursor < lvl.size()) {
      const std::size_t batch = std::min(max_workers_, lvl.size() - cursor);
      std::vector<std::future<std::unordered_map<std::string, std::string>>> futs;
      futs.reserve(batch);
      for (std::size_t b = 0; b < batch; ++b) {
        const auto& t = tasks[lvl[cursor + b]];
        futs.push_back(std::async(std::launch::async, [fn = t.compute]() {
          return fn ? fn() : std::unordered_map<std::string, std::string>{};
        }));
      }
      for (std::size_t b = 0; b < batch; ++b) {
        lvl_results[cursor + b] = futs[b].get();
      }
      cursor += batch;
    }

    // Apply level deltas in canonical sorted task order and construct receipt nodes.
    for (std::size_t pos = 0; pos < lvl.size(); ++pos) {
      const auto& t = tasks[lvl[pos]];
      std::map<std::string, std::string> sorted_delta(lvl_results[pos].begin(),
                                                      lvl_results[pos].end());
      std::string delta_serial;
      for (const auto& [k, v] : sorted_delta) {
        committed_kv[k] = v;
        delta_serial += k + "=" + v + ";";
      }
      const std::string delta_sha = sha3_hex_str(delta_serial);
      const std::string meaning_json =
          "{\"action_type\":\"dpe.task_commit\",\"agent_id\":" + std::to_string(t.agent_id) +
          ",\"task_id\":\"" + t.task_id + "\"}";
      const std::string evidence_json = "{\"delta_sha3_512\":\"" + delta_sha + "\"}";
      const std::string authority_json =
          "{\"policy_sha3_512\":\"" + policy_sha + "\",\"verdict\":\"Allow\"}";
      const std::string payload = t81::crypto::rfc00d4_canonical_signing_payload(
          meaning_json, evidence_json, authority_json);
      const std::string sig =
          t81::crypto::sign_rfc00d4_payload_b64(signing_kp_.seed, signing_kp_.public_key, payload);
      dag_nodes.push_back(t81::crypto::AccountableBundleNode{t.task_id, t.dep_task_ids, payload,
                                                             sig, signing_kp_.public_key});
    }
  }

  // 4. Verify complete receipt DAG before committing state into CanonFS (fail-closed).
  auto dag_res = t81::crypto::verify_dag(dag_nodes);
  if (!dag_res.ok) {
    out.code = dag_res.code;
    out.error = dag_res.error;
    return out;
  }

  std::string state_serial;
  for (const auto& [k, v] : committed_kv) state_serial += k + ":" + v + "\n";
  std::vector<std::byte> state_bytes(state_serial.size());
  std::memcpy(state_bytes.data(), state_serial.data(), state_serial.size());
  auto wres = canonfs_driver_->write_object(t81::canonfs::ObjectType::RawBlock, state_bytes);
  if (!wres.has_value()) {
    out.code = "ERR_EVIDENCE_PERSIST_FAILED";
    out.error = "failed persisting unified DPE+MCP committed state to CanonFS";
    return out;
  }

  out.ok = true;
  out.trit = 1;
  out.code = "OK";
  out.levels_executed = levels.size();
  out.tasks_committed = tasks.size();
  out.final_state_sha3_512 = sha3_hex_str(state_serial);
  out.receipt_dag = std::move(dag_nodes);
  return out;
}

}  // namespace t81::provider
