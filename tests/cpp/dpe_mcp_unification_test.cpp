// tests/cpp/dpe_mcp_unification_test.cpp
//
// Wave 4 verification: DPE level-parallel DAG execution unified with MCP
// transactional isolation, fail-closed intra-level conflict detection, and
// native Ed25519 RFC-00D4 receipt DAG verification.

#include "provider/dpe_mcp_coordinator.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

int main() {
  auto parsed = t81::axion::parse_policy("(policy (tier 1))");
  if (!parsed.has_value()) {
    std::cerr << "FAIL: could not parse allow policy\n";
    return 1;
  }
  auto pol = std::make_shared<t81::axion::PolicyEngine>(std::move(parsed.value()));
  auto fs_drv = std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver());

  t81::provider::DpeMcpCoordinator coord(pol, fs_drv, 4);

  // 1. Diamond 4-node multi-agent DAG across 3 levels (disjoint keys -> bit-exact commit).
  std::vector<t81::provider::DpeAgentTask> diamond = {
      {"t0-root",
       1,
       {},
       {"k0"},
       []() { return std::unordered_map<std::string, std::string>{{"k0", "+1"}}; }},
      {"t1-left",
       2,
       {"t0-root"},
       {"k1"},
       []() { return std::unordered_map<std::string, std::string>{{"k1", "-1"}}; }},
      {"t2-right",
       3,
       {"t0-root"},
       {"k2"},
       []() { return std::unordered_map<std::string, std::string>{{"k2", "+1"}}; }},
      {"t3-join",
       4,
       {"t1-left", "t2-right"},
       {"k3"},
       []() { return std::unordered_map<std::string, std::string>{{"k3", "0"}}; }},
  };

  auto r1 = coord.execute_and_commit(diamond);
  auto r2 = coord.execute_and_commit(diamond);
  if (!r1.ok || !r2.ok || r1.levels_executed != 3 || r1.tasks_committed != 4) {
    std::cerr << "FAIL: diamond DAG commit failed (" << r1.code << ": " << r1.error << ")\n";
    return 1;
  }
  if (r1.final_state_sha3_512 != r2.final_state_sha3_512) {
    std::cerr << "FAIL: non-deterministic state hash across repeat parallel runs\n";
    return 1;
  }
  auto dag_check = t81::crypto::verify_dag(r1.receipt_dag);
  if (!dag_check.ok || dag_check.verified_nodes != 4) {
    std::cerr << "FAIL: Ed25519 receipt DAG verification failed\n";
    return 1;
  }

  // 2. Intra-level write-write conflict -> fail closed with canonical lowest-agent-ID winner.
  std::vector<t81::provider::DpeAgentTask> conflict = {
      {"c-a2",
       2,
       {},
       {"shared_key"},
       []() { return std::unordered_map<std::string, std::string>{{"shared_key", "+1"}}; }},
      {"c-a1",
       1,
       {},
       {"shared_key"},
       []() { return std::unordered_map<std::string, std::string>{{"shared_key", "-1"}}; }},
  };
  auto rc = coord.execute_and_commit(conflict);
  if (rc.ok || rc.code != "ERR_DPE_TX_CONFLICT" || rc.conflict_winner_agent != 1) {
    std::cerr << "FAIL: expected ERR_DPE_TX_CONFLICT with winner=1, got " << rc.code
              << " winner=" << rc.conflict_winner_agent << "\n";
    return 1;
  }

  std::cout << "PASS t81_dpe_mcp_unification_test (3 levels, 4 tasks, bit-exact repeat SHA3-512, "
               "Ed25519 DAG verified, conflict winner=1)\n";
  return 0;
}
