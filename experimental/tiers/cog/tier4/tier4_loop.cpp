#include "t81/experimental/cog/tier4/tier4_loop.hpp"
#include <cmath>
#include <sstream>
#include <functional>
#include "t81/axion/context.hpp"

namespace t81::cog::v1 {

Tier4Loop::Tier4Loop(AxionEvaluator evaluator) : evaluator_(std::move(evaluator)) {
  model_.current_goal = "initialize";
  model_.confidence = 1.0f;
  baseline_confidence_ = 1.0f;
}

void Tier4Loop::observe(const std::string& observation) {
  model_.add_history("Observation: " + observation);
  log_reflection("observed state change");
}

ReflectionTrace Tier4Loop::reflect() {
  std::string reason;
  
  // Determine if refinement is needed based on history and confidence
  if (model_.confidence < 0.81f) {
    model_.current_goal = "recalibrate";
    reason = "confidence below threshold, seeking refinement";
    log_reflection(reason);
  } else {
    reason = "reflection complete: state stable";
    log_reflection(reason);
  }

  ReflectionTrace trace;
  trace.goal = model_.current_goal;
  trace.confidence = model_.confidence;
  trace.reason = reason;
  trace.history_snapshot = model_.history;
  trace.recursion_depth = 0; // Updated during recursive improvements
  trace.safety_score = calculate_safety_score();
  
  return trace;
}

void Tier4Loop::refine() {
  if (model_.current_goal == "recalibrate") {
    model_.confidence = 1.0f;
    model_.current_goal = "execute";
    log_reflection("refined model: confidence restored");
  }
}

bool Tier4Loop::attempt_recursive_improvement(const std::string& improvement_type) {
  // Validate safety bounds before attempting improvement
  if (!validate_improvement_safety(improvement_type)) {
    log_reflection("recursive improvement denied: safety bounds violated");
    return false;
  }
  
  // Check if this is a different improvement type
  if (improvement_type != last_improvement_type_) {
    consecutive_improvements_ = 0;
    last_improvement_type_ = improvement_type;
  }
  
  // Attempt the improvement
  float old_confidence = model_.confidence;
  
  if (improvement_type == "meta_learning") {
    // Simulate meta-learning improvement
    model_.confidence = std::min(1.0f, model_.confidence * 1.1f);
    model_.add_history("Meta-learning improvement applied");
  } else if (improvement_type == "self_optimization") {
    // Simulate self-optimization
    model_.confidence = std::min(1.0f, model_.confidence * 1.05f);
    model_.add_history("Self-optimization improvement applied");
  } else if (improvement_type == "cognitive_restructuring") {
    // Simulate cognitive restructuring
    model_.confidence = std::min(1.0f, model_.confidence * 1.15f);
    model_.add_history("Cognitive restructuring improvement applied");
  } else {
    log_reflection("unknown improvement type: " + improvement_type);
    return false;
  }
  
  // Update tracking
  consecutive_improvements_++;
  
  // Log successful improvement
  std::ostringstream ss;
  ss << "recursive improvement successful: " << improvement_type 
     << " (confidence: " << old_confidence << " -> " << model_.confidence << ")";
  log_reflection(ss.str());
  
  return true;
}

void Tier4Loop::consume_snapshot(const t81::vm::ReflectionSnapshot& snapshot) {
  model_.add_history("Consuming VM snapshot at PC=" + std::to_string(snapshot.pc));
  model_.add_history("CODE hash: " + std::to_string(snapshot.code_hash));

  // Heuristic: decrease confidence if we see many traps in recent trace
  int traps = 0;
  for (const auto& entry : snapshot.recent_trace) {
    if (entry.trap.has_value()) traps++;
  }
  if (traps > 0) {
    model_.confidence *= std::pow(0.9f, static_cast<float>(traps));
    log_reflection("detected " + std::to_string(traps) +
                   " traps in recent trace, confidence decreased");
  }
}

void Tier4Loop::update_self_model(const std::string& belief_key, const std::string& belief_val) {
  model_.beliefs[belief_key] = belief_val;
  log_reflection("updated self-model belief: " + belief_key);
}

bool Tier4Loop::validate_improvement_safety(const std::string& improvement_type) const {
  // Check recursion depth limit
  if (!check_recursion_depth_limit()) {
    return false;
  }
  
  // Check confidence threshold
  if (model_.confidence < improvement_bounds_.min_confidence_threshold) {
    return false;
  }
  
  // Check consecutive improvement limit
  if (!check_consecutive_improvement_limit()) {
    return false;
  }
  
  // Check improvement rate limit (simulated)
  if (!check_improvement_rate_limit(model_.confidence)) {
    return false;
  }
  
  // Human approval requirement for certain improvements
  if (improvement_bounds_.requires_human_approval && 
      (improvement_type == "cognitive_restructuring" || improvement_type == "meta_learning")) {
    log_reflection("human approval required for: " + improvement_type);
    return false;
  }
  
  return true;
}

float Tier4Loop::calculate_safety_score() const {
  float score = 1.0f;
  
  // Penalize low confidence
  if (model_.confidence < 0.5f) {
    score *= 0.8f;
  }
  
  // Penalize many consecutive improvements
  if (consecutive_improvements_ > 2) {
    score *= 0.9f;
  }
  
  // Penalize certain risky improvement types
  if (last_improvement_type_ == "cognitive_restructuring") {
    score *= 0.85f;
  }
  
  return score;
}

bool Tier4Loop::check_recursion_depth_limit() const {
  // Simple heuristic based on history length
  return model_.history.size() < static_cast<size_t>(improvement_bounds_.max_recursion_depth * 10);
}

bool Tier4Loop::check_improvement_rate_limit(float new_confidence) const {
  if (baseline_confidence_ == 0.0f) return true;
  
  float improvement_rate = new_confidence / baseline_confidence_;
  return improvement_rate <= improvement_bounds_.max_improvement_rate;
}

bool Tier4Loop::check_consecutive_improvement_limit() const {
  return consecutive_improvements_ < improvement_bounds_.max_consecutive_improvements;
}

void Tier4Loop::log_reflection(const std::string& reason) const {
  std::ostringstream ss;
  ss << "cog:tier4:reflect: " << reason;

  // Create a dummy context for the engine to evaluate.
  // In a real VM integration, this would be part of a syscall.
  t81::axion::SyscallContext ctx;
  ctx.trace_reasons.push_back(ss.str());
  ctx.next_opcode = t81::tisc::Opcode::Nop;

  if (evaluator_) {
    evaluator_(ctx);
  }
}

}  // namespace t81::cog::v1
