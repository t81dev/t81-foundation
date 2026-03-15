#include <iostream>
#include <cassert>
#include "t81/ai/cognitive_tiers.hpp"

// Simple test without external testing framework
int main() {
    std::cout << "=== Cognitive Tier Framework Simple Test ===" << std::endl;
    
    try {
        // Test 1: Create tier engine
        auto tier_engine = t81::ai::cognitive::create_cognitive_tier_engine();
        if (!tier_engine) {
            std::cerr << "FAIL: Could not create tier engine" << std::endl;
            return 1;
        }
        std::cout << "✓ Tier engine created successfully" << std::endl;
        
        // Test 2: Get tier capabilities
        auto t1_caps = tier_engine->get_tier_capabilities(t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        if (t1_caps.tier != t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC) {
            std::cerr << "FAIL: T1 capabilities incorrect" << std::endl;
            return 1;
        }
        if (t1_caps.name != "Symbolic") {
            std::cerr << "FAIL: T1 name incorrect" << std::endl;
            return 1;
        }
        if (t1_caps.max_cpu_time_ms != 1000) {
            std::cerr << "FAIL: T1 CPU time limit incorrect" << std::endl;
            return 1;
        }
        std::cout << "✓ T1 capabilities retrieved correctly" << std::endl;
        
        // Test 3: Test T5 (infinite) capabilities
        auto t5_caps = tier_engine->get_tier_capabilities(t81::ai::cognitive::CognitiveTier::TIER5_INFINITE);
        if (t5_caps.tier != t81::ai::cognitive::CognitiveTier::TIER5_INFINITE) {
            std::cerr << "FAIL: T5 capabilities incorrect" << std::endl;
            return 1;
        }
        if (t5_caps.name != "Infinite") {
            std::cerr << "FAIL: T5 name incorrect" << std::endl;
            return 1;
        }
        if (t5_caps.max_cpu_time_ms != 3600000) {  // 1 hour
            std::cerr << "FAIL: T5 CPU time limit incorrect" << std::endl;
            return 1;
        }
        if (!t5_caps.network_access) {
            std::cerr << "FAIL: T5 should have network access" << std::endl;
            return 1;
        }
        if (!t5_caps.human_oversight_required) {
            std::cerr << "FAIL: T5 should require human oversight" << std::endl;
            return 1;
        }
        std::cout << "✓ T5 capabilities retrieved correctly" << std::endl;
        
        // Test 4: Test tier access control
        t81::ai::cognitive::OperationContext t1_context{
            .operation_id = "test_op_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1",
            .project_id = "project_alpha",
            .execution_time_ms = 500,
            .memory_used_mb = 50
        };
        
        bool can_execute_t1 = tier_engine->can_execute_operation(t1_context, "symbolic_reasoning");
        if (!can_execute_t1) {
            std::cerr << "FAIL: T1 should be able to execute symbolic reasoning" << std::endl;
            return 1;
        }
        std::cout << "✓ T1 access control working correctly" << std::endl;
        
        // Test 5: Test tier access denial
        bool can_execute_t3 = tier_engine->can_execute_operation(t1_context, "meta_learning");
        if (can_execute_t3) {
            std::cerr << "FAIL: T1 should NOT be able to execute meta_learning" << std::endl;
            return 1;
        }
        std::cout << "✓ Tier access denial working correctly" << std::endl;
        
        // Test 6: Test security evaluation
        auto security_eval = tier_engine->evaluate_security(t1_context, "symbolic_reasoning");
        if (!security_eval.access_granted) {
            std::cerr << "FAIL: Security evaluation should grant access for T1 symbolic reasoning" << std::endl;
            return 1;
        }
        std::cout << "✓ Security evaluation working correctly" << std::endl;
        
        auto security_eval_denied = tier_engine->evaluate_security(t1_context, "meta_learning");
        if (security_eval_denied.access_granted) {
            std::cerr << "FAIL: Security evaluation should deny access for T1 meta_learning" << std::endl;
            return 1;
        }
        std::cout << "✓ Security denial working correctly" << std::endl;
        
        // Test 7: Test tier constraints
        t81::ai::cognitive::OperationContext valid_context{
            .operation_id = "test_op_002",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1",
            .execution_time_ms = 800,  // Within 1000ms limit
            .memory_used_mb = 80     // Within 100MB limit
        };
        
        bool constraints_ok = tier_engine->verify_tier_constraints(
            valid_context, t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        if (!constraints_ok) {
            std::cerr << "FAIL: Valid context should pass constraints" << std::endl;
            return 1;
        }
        std::cout << "✓ Tier constraints verification working correctly" << std::endl;
        
        // Test 8: Test constraint violation
        t81::ai::cognitive::OperationContext invalid_context{
            .operation_id = "test_op_003",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1",
            .execution_time_ms = 1500,  // Exceeds 1000ms limit
            .memory_used_mb = 150     // Exceeds 100MB limit
        };
        
        bool constraints_invalid = tier_engine->verify_tier_constraints(
            invalid_context, t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        if (constraints_invalid) {
            std::cerr << "FAIL: Invalid context should fail constraints" << std::endl;
            return 1;
        }
        std::cout << "✓ Constraint violation detection working correctly" << std::endl;
        
        // Test 9: Test utility functions
        std::string tier_str = t81::ai::cognitive::tier_to_string(t81::ai::cognitive::CognitiveTier::TIER3_RECURSIVE);
        if (tier_str != "TIER3_RECURSIVE") {
            std::cerr << "FAIL: Tier to string conversion incorrect" << std::endl;
            return 1;
        }
        std::cout << "✓ Utility functions working correctly" << std::endl;
        
        // Test 10: Test promotion path
        auto promotion_path = t81::ai::cognitive::get_promotion_path(
            t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            t81::ai::cognitive::CognitiveTier::TIER3_RECURSIVE);
        if (promotion_path.size() != 2) {
            std::cerr << "FAIL: Promotion path should have 2 steps" << std::endl;
            return 1;
        }
        if (promotion_path[0] != t81::ai::cognitive::CognitiveTier::TIER2_REFLECTIVE ||
            promotion_path[1] != t81::ai::cognitive::CognitiveTier::TIER3_RECURSIVE) {
            std::cerr << "FAIL: Promotion path incorrect" << std::endl;
            return 1;
        }
        std::cout << "✓ Promotion path calculation working correctly" << std::endl;
        
        std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
        std::cout << "Cognitive Tier Framework is working correctly!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FAIL: Unknown exception caught" << std::endl;
        return 1;
    }
}
