# T81 System Integration AGI Examples

This directory contains executable examples corresponding to the "Comprehensive AGI Example Walkthroughs" found in `docs/how-to/systems-integration-agi.md`.

These examples demonstrate how the T81 Foundation stack—T81Lang, TISC, HanoiVM, Axion, and CanonFS—coalesce to support governed Artificial General Intelligence.

## Examples

1.  **`ethical_accumulator.t81`**: The Ethical Decision Accumulator (Section 6.1).
2.  **`gated_inference.t81`**: Policy-Gated Large Model Inference (Section 6.2).
3.  **`tier4_refining.t81`**: Self-Refining Heuristic Loop (Section 6.3).
4.  **`multi_agent_consensus.t81`**: Multi-Agent Threshold Cognition (Section 6.4).
5.  **`world_model_sim.t81`**: Deterministic World Model Simulation (Section 6.5).
6.  **`categorical_governance.t81`**: Categorical Governance of Intent (Section 6.6).
7.  **`distributed_sharding.t81`**: Distributed Global Intelligence Sharding (Section 7.1).
8.  **`evolutionary_tree.t81`**: Evolutionary Wisdom-Tree Database (Section 7.2).
9.  **`full_stack_agent.t81`**: Auditable Ethics-Enforced Autonomous Agent (Section 7.3).
10. **`supply_chain.t81`**: Self-Assembling Cognitive Supply Chain (Section 7.4).
11. **`recursive_nas.t81`**: Recursive Neural Architecture Search (RNAS) (Section 7.5).
12. **`zk_state_transition.t81`**: Zero-Knowledge Cognitive State Transition (Section 7.6).

## Running the Examples

You can compile and run these examples using the `t81` CLI.

```bash
# Compile
t81 compile examples/system-integration-agi/ethical_accumulator.t81 -o ethical.tisc

# Run with AGI Policy
t81 run ethical.tisc --policy examples/system-integration-agi/agi_safety.apl
```

## Verification

Run the verification script to compile and check all examples:

```bash
./examples/system-integration-agi/verify.sh
```
