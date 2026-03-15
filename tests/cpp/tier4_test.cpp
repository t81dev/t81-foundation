#include <cassert>
#include <iostream>
#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/experimental/cog/promotion.hpp"
#include "t81/experimental/cog/tier4/planner.hpp"
#include "t81/experimental/cog/tier4/tier4_loop.hpp"

int main() {
  [[maybe_unused]] auto engine = t81::axion::make_policy_engine(std::nullopt);
  t81::cog::v1::Tier4Loop loop(
      [&](const t81::axion::SyscallContext& ctx) { return engine->evaluate(ctx); });

  std::cout << "Starting Tier 4 Reflection Loop test...\n";

  // Test basic observe-reflect-refine cycle
  loop.observe("initial contact");
  assert(loop.get_model().history.size() == 1);

  [[maybe_unused]] auto trace = loop.reflect();
  assert(loop.get_model().current_goal == "initialize");
  assert(trace.goal == "initialize");
  assert(!t81::cog::should_promote_to_tier4(trace));

  // Test recursive self-improvement safety
  std::cout << "Testing recursive self-improvement safety...\n";
  
  // Test valid improvement
  bool improvement_result = loop.attempt_recursive_improvement("self_optimization");
  assert(improvement_result);
  assert(loop.get_model().confidence > 1.0f);
  
  // Test safety bounds - low confidence should block improvement
  loop.get_mutable_model().confidence = 0.05f; // Below minimum threshold
  improvement_result = loop.attempt_recursive_improvement("meta_learning");
  assert(!improvement_result); // Should be denied
  std::cout << "Safety bounds validation: PASS\n";
  
  // Test consecutive improvement limits
  loop.get_mutable_model().confidence = 0.9f; // Reset confidence
  int successful_improvements = 0;
  for (int i = 0; i < 5; ++i) {
    if (loop.attempt_recursive_improvement("self_optimization")) {
      successful_improvements++;
    }
  }
  assert(successful_improvements <= 3); // Should be limited
  std::cout << "Consecutive improvement limits: PASS (" << successful_improvements << " allowed)\n";

  // Simulate low confidence for promotion test
  loop.observe("uncertain environment");
  
  // For the sake of testing the promotion heuristic, let's assume we want to test it.
  [[maybe_unused]] t81::cog::v1::ReflectionTrace low_conf_trace;
  low_conf_trace.confidence = 0.5f;
  low_conf_trace.goal = "recalibrate";
  assert(t81::cog::should_promote_to_tier4(low_conf_trace));

  // Test TierAwarePlanner
  using namespace t81::cog::v1;
  TaskMetadata simple_task{"1", 5, 100, false};
  assert(TierAwarePlanner::select_tier(simple_task) == t81::cog::TierId::Tier1);

  TaskMetadata complex_task{"2", 60, 1000, false};
  assert(TierAwarePlanner::select_tier(complex_task) == t81::cog::TierId::Tier4);

  TaskMetadata reflection_task{"3", 10, 500, true};
  assert(TierAwarePlanner::select_tier(reflection_task) == t81::cog::TierId::Tier4);

  // Test safety score calculation
  auto safe_trace = loop.reflect();
  assert(safe_trace.safety_score > 0.0f);
  assert(safe_trace.safety_score <= 1.0f);
  std::cout << "Safety score calculation: PASS (" << safe_trace.safety_score << ")\n";

  std::cout << "\n🎉 Tier 4 Loop Closure validation completed!\n";
  std::cout << "✅ Observe-Reflect-Refine cycle validated\n";
  std::cout << "✅ Recursive self-improvement safety enforced\n";
  std::cout << "✅ Safety bounds and limits verified\n";
  std::cout << "✅ Integration with promotion system confirmed\n";
  std::cout << "✅ Tier 4 Loop successfully closed!\n";
  
  return 0;
}
