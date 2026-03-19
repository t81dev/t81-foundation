#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "t81/axion/context.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/experimental/cog/tier4/self_model.hpp"
#include "t81/vm/state.hpp"

namespace t81::cog::v1 {

/**
 * @struct ReflectionTrace
 * @brief Captures cognitive state for auditing and promotion decisions.
 */
struct ReflectionTrace {
  std::string goal;
  float confidence;
  std::string reason;
  std::vector<std::string> history_snapshot;
  
  // Recursive self-improvement tracking
  int recursion_depth{0};
  std::string improvement_type;
  float safety_score{1.0f};
};

/**
 * @struct RecursiveImprovementBounds
 * @brief Safety bounds for recursive self-improvement operations.
 */
struct RecursiveImprovementBounds {
  int max_recursion_depth{5};
  float min_confidence_threshold{0.1f};
  float max_improvement_rate{2.0f};
  int max_consecutive_improvements{3};
  bool requires_human_approval{true};
};

/**
 * @class Tier4Loop
 * @brief Implements the Tier 4 observe-reflect-refine cycle with recursive self-improvement safety.
 */
class Tier4Loop {
public:
  using AxionEvaluator = std::function<t81::axion::Verdict(const t81::axion::SyscallContext&)>;

  Tier4Loop(AxionEvaluator evaluator);

  void observe(const std::string& observation);
  ReflectionTrace reflect();
  void refine();
  
  /**
   * @brief Attempt recursive self-improvement with safety validation.
   */
  bool attempt_recursive_improvement(const std::string& improvement_type);

  /**
   * @brief Consumes a reflection snapshot from the VM to update the internal model.
   */
  void consume_snapshot(const t81::vm::ReflectionSnapshot& snapshot);

  const SelfModel& get_model() const { return model_; }
  
  /**
   * @brief Get mutable access to model for testing purposes.
   */
  SelfModel& get_mutable_model() { return model_; }
  
  /**
   * @brief Get current recursive improvement bounds.
   */
  const RecursiveImprovementBounds& get_improvement_bounds() const { return improvement_bounds_; }

  /**
   * @brief Updates the agent's self-model based on internal reflection.
   */
  void update_self_model(const std::string& belief_key, const std::string& belief_val);
  
  /**
   * @brief Validate recursive improvement against safety bounds.
   */
  bool validate_improvement_safety(const std::string& improvement_type) const;

  /**
   * @brief Apply meta-learning with deterministic guarantees.
   */
  bool apply_deterministic_meta_learning(const std::string& learning_pattern, uint64_t seed);

  /**
   * @brief Get deterministic hash of current learning state for reproducibility.
   */
  uint64_t get_learning_state_hash() const;

private:
  AxionEvaluator evaluator_;
  SelfModel model_;
  RecursiveImprovementBounds improvement_bounds_;
  
  // Recursive improvement tracking
  int consecutive_improvements_{0};
  std::string last_improvement_type_;
  float baseline_confidence_{1.0f};

  void log_reflection(const std::string& reason) const;
  bool check_recursion_depth_limit() const;
  bool check_improvement_rate_limit(float new_confidence) const;
  bool check_consecutive_improvement_limit() const;
  float calculate_safety_score() const;
  uint64_t deterministic_hash_combine(uint64_t seed, uint64_t value) const;
};

}  // namespace t81::cog::v1
