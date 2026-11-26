#pragma once

// src/provider/dpe_mcp_coordinator.hpp
//
// Wave 4: Unifies RFC-DPE level-parallel DAG execution with MCP transactional
// isolation and native Ed25519 RFC-00D4 receipt sealing.
// Staged under src/provider/ to preserve docs/governance/PUBLIC_API_LOCK.json (v2.2.0).

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "crypto/ed25519_bundle.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace t81::provider {

struct DpeAgentTask {
  std::string task_id;
  uint32_t agent_id{1};  // Canonical positive integer agent ID (matches TernaryOSTransactions.tla)
  std::vector<std::string> dep_task_ids;
  std::vector<std::string> write_keys;
  // Pure deterministic execution callback producing key -> payload bytes.
  std::function<std::unordered_map<std::string, std::string>()> compute;
};

struct DpeMcpCommitOutcome {
  bool ok{false};
  int trit{-1};
  std::string code;   // "OK", "ERR_DPE_CYCLE", "ERR_DPE_TX_CONFLICT",
                      // "ERR_POLICY_DENIED", "ERR_EVIDENCE_PERSIST_FAILED"
  std::string error;
  uint32_t conflict_winner_agent{0};
  std::size_t levels_executed{0};
  std::size_t tasks_committed{0};
  std::string final_state_sha3_512;
  std::vector<t81::crypto::AccountableBundleNode> receipt_dag;
};

class DpeMcpCoordinator {
public:
  DpeMcpCoordinator(std::shared_ptr<t81::axion::PolicyEngine> policy_engine,
                    std::shared_ptr<t81::canonfs::Driver> canonfs_driver,
                    std::size_t max_workers = 4);

  // Executes DAG tasks level-by-level across bounded worker threads, checks
  // intra-level write-set disjointness (fail-closed ERR_DPE_TX_CONFLICT with
  // canonical lowest-agent-ID winner), commits deltas atomically into CanonFS,
  // and seals a verified Ed25519 RFC-00D4 receipt DAG.
  [[nodiscard]] DpeMcpCommitOutcome execute_and_commit(const std::vector<DpeAgentTask>& tasks);

private:
  std::shared_ptr<t81::axion::PolicyEngine> policy_engine_;
  std::shared_ptr<t81::canonfs::Driver> canonfs_driver_;
  std::size_t max_workers_;
  t81::crypto::Ed25519KeyPair signing_kp_;
};

}  // namespace t81::provider
