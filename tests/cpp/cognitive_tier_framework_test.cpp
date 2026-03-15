#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "t81/ai/cognitive_tiers.hpp"
#include "t81/ai/agi_governance.hpp"
#include "t81/ai/cognitive_monitoring.hpp"

class CognitiveTierFrameworkTest {
protected:
    std::unique_ptr<t81::ai::cognitive::CognitiveTierEngine> tier_engine_;
    std::unique_ptr<t81::ai::agi::AGIGovernance> agi_governance_;
    std::unique_ptr<t81::ai::cognitive::CognitiveMonitoring> cognitive_monitoring_;

public:
    CognitiveTierFrameworkTest() {
        tier_engine_ = t81::ai::cognitive::create_cognitive_tier_engine();
        agi_governance_ = t81::ai::agi::create_agi_governance();
        cognitive_monitoring_ = t81::ai::cognitive::create_cognitive_monitoring();
    }

public:
    // Test cognitive tier engine functionality
    TEST_CASE("CognitiveTierFramework - TierEngineBasicFunctionality", "[cognitive_tier]") {
        // Test tier capability retrieval
        auto t1_caps = tier_engine_->get_tier_capabilities(t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        REQUIRE(t1_caps.tier == t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        REQUIRE(t1_caps.name == "Symbolic");
        REQUIRE(t1_caps.max_cpu_time_ms == 1000);
        REQUIRE(t1_caps.max_memory_mb == 100);
        REQUIRE(t1_caps.network_access == false);
        
        // Test tier 5 (infinite) capabilities
        auto t5_caps = tier_engine_->get_tier_capabilities(t81::ai::cognitive::CognitiveTier::TIER5_INFINITE);
        REQUIRE(t5_caps.tier == t81::ai::cognitive::CognitiveTier::TIER5_INFINITE);
        REQUIRE(t5_caps.name == "Infinite");
        REQUIRE(t5_caps.max_cpu_time_ms == 3600000);  // 1 hour
        REQUIRE(t5_caps.max_memory_mb == 8192);        // 8GB
        REQUIRE(t5_caps.network_access == true);
        REQUIRE(t5_caps.human_oversight_required == true);
        REQUIRE(t5_caps.emergency_shutdown_enabled == true);
    }
    
    TEST_F(CognitiveTierFrameworkTest, TierAccessControl) {
        // Create operation context for T1 user
        t81::ai::cognitive::OperationContext t1_context{
            .operation_id = "test_op_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1",
            .project_id = "project_alpha",
            .execution_time_ms = 500,
            .memory_used_mb = 50
        };
        
        // Test T1 operation (should be allowed)
        bool can_execute_t1 = tier_engine_->can_execute_operation(t1_context, "symbolic_reasoning");
        EXPECT_TRUE(can_execute_t1);
        
        // Test T3 operation with T1 user (should be denied)
        bool can_execute_t3 = tier_engine_->can_execute_operation(t1_context, "meta_learning");
        EXPECT_FALSE(can_execute_t3);
        
        // Test security evaluation
        auto security_eval = tier_engine_->evaluate_security(t1_context, "symbolic_reasoning");
        EXPECT_TRUE(security_eval.access_granted);
        
        auto security_eval_denied = tier_engine_->evaluate_security(t1_context, "meta_learning");
        EXPECT_FALSE(security_eval_denied.access_granted);
        EXPECT_FALSE(security_eval_denied.required_approvals.empty());
    }
    
    TEST_F(CognitiveTierFrameworkTest, TierPromotionProcess) {
        // Create promotion request from T1 to T2
        t81::ai::cognitive::PromotionRequest request{
            .from_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .to_tier = t81::ai::cognitive::CognitiveTier::TIER2_REFLECTIVE,
            .requester_id = "user_t1",
            .project_id = "project_alpha",
            .justification = "User has demonstrated reliable T1 operations for 30 days",
            .performance_evidence = {"100% success rate", "no violations"},
            .safety_assessments = {"low risk profile", "compliant operations"},
            .request_date = "2026-03-14",
            .target_date = "2026-03-21"
        };
        
        // Submit promotion request
        bool submitted = tier_engine_->submit_promotion_request(request);
        EXPECT_TRUE(submitted);
        
        // Check pending promotions
        auto pending = tier_engine_->get_pending_promotions();
        EXPECT_FALSE(pending.empty());
        
        // Find our request
        bool found = false;
        for (const auto& pending_req : pending) {
            if (pending_req.requester_id == "user_t1" && 
                pending_req.from_tier == t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }
    
    TEST_F(CognitiveTierFrameworkTest, TierConstraintVerification) {
        // Create operation context within T1 limits
        t81::ai::cognitive::OperationContext valid_context{
            .operation_id = "test_op_002",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1",
            .execution_time_ms = 800,  // Within 1000ms limit
            .memory_used_mb = 80     // Within 100MB limit
        };
        
        bool constraints_ok = tier_engine_->verify_tier_constraints(
            valid_context, t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        EXPECT_TRUE(constraints_ok);
        
        // Create operation context exceeding T1 limits
        t81::ai::cognitive::OperationContext invalid_context{
            .operation_id = "test_op_003",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1",
            .execution_time_ms = 1500,  // Exceeds 1000ms limit
            .memory_used_mb = 150     // Exceeds 100MB limit
        };
        
        bool constraints_invalid = tier_engine_->verify_tier_constraints(
            invalid_context, t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC);
        EXPECT_FALSE(constraints_invalid);
    }

    // Test AGI governance functionality
    TEST_F(CognitiveTierFrameworkTest, AGIGovernanceSafetyAssessment) {
        // Create AGI proposal for T3 (recursive)
        t81::ai::agi::AGIProposal proposal{
            .proposal_id = "agi_proposal_001",
            .proposer_id = "research_team_alpha",
            .target_tier = t81::ai::cognitive::CognitiveTier::TIER3_RECURSIVE,
            .capabilities = {"meta_learning", "self_improvement", "algorithm_optimization"},
            .intended_uses = {"research", "optimization", "automated_improvement"},
            .scope_description = "Enable safe self-improvement capabilities for research purposes",
            .max_cpu_time = 60000,      // 1 minute
            .max_memory = 2048,          // 2GB
            .network_access = true,
            .network_access_level = "limited",
            .proposed_start_date = "2026-04-01",
            .expected_completion_date = "2026-06-01"
        };
        
        // Assess safety risks
        auto safety_assessment = agi_governance_->assess_safety_risks(proposal);
        
        EXPECT_EQ(safety_assessment.proposal_id, "agi_proposal_001");
        EXPECT_GT(safety_assessment.overall_risk_score, 0.5);  // Should have moderate-high risk
        EXPECT_FALSE(safety_assessment.tier_risks.empty());
        EXPECT_FALSE(safety_assessment.capability_risks.empty());
        
        // Should recommend review or conditions due to T3 level
        EXPECT_TRUE(safety_assessment.approval_recommendation == "REVIEW_REQUIRED" ||
                   safety_assessment.approval_recommendation == "APPROVE_WITH_CONDITIONS");
    }
    
    TEST_F(CognitiveTierFrameworkTest, AGIGovernanceSafetyEnforcement) {
        // Create operation context that violates safety
        t81::ai::cognitive::OperationContext unsafe_context{
            .operation_id = "unsafe_op_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER3_RECURSIVE,
            .user_id = "user_t3",
            .metadata = {
                {"safety_protocol_status", "violated"},
                {"ethical_crisis_detected", "true"}
            }
        };
        
        // Test safety enforcement (should block unsafe operation)
        bool safe_to_execute = agi_governance_->enforce_safety_constraints(
            unsafe_context, "autonomous_research");
        EXPECT_FALSE(safe_to_execute);
    }
    
    TEST_F(CognitiveTierFrameworkTest, AGIEmergencyShutdown) {
        // Test emergency shutdown trigger
        std::string reason = "Critical safety protocol violation detected";
        std::string trigger_id = "safety_monitor_001";
        
        // Trigger emergency shutdown
        agi_governance_->trigger_emergency_shutdown(reason, trigger_id);
        
        // Check shutdown history
        auto shutdown_history = agi_governance_->get_emergency_shutdown_history();
        EXPECT_FALSE(shutdown_history.empty());
        
        // Verify shutdown event
        bool found = false;
        for (const auto& event : shutdown_history) {
            if (event.trigger_id == trigger_id && event.reason == reason) {
                found = true;
                EXPECT_EQ(event.shutdown_type, "immediate");
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    // Test cognitive monitoring functionality
    TEST_F(CognitiveTierFrameworkTest, CognitiveMonitoringSessionManagement) {
        // Create operation context
        t81::ai::cognitive::OperationContext context{
            .operation_id = "monitor_test_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER2_REFLECTIVE,
            .user_id = "user_t2",
            .project_id = "project_beta"
        };
        
        // Start monitoring
        cognitive_monitoring_->start_operation_monitoring("monitor_test_001", context);
        
        // Check active sessions
        auto active_sessions = cognitive_monitoring_->get_active_sessions();
        EXPECT_FALSE(active_sessions.empty());
        
        bool found = false;
        for (const auto& session : active_sessions) {
            if (session.operation_id == "monitor_test_001") {
                found = true;
                EXPECT_EQ(session.context.user_id, "user_t2");
                EXPECT_EQ(session.context.current_tier, t81::ai::cognitive::CognitiveTier::TIER2_REFLECTIVE);
                break;
            }
        }
        EXPECT_TRUE(found);
    }
    
    TEST_F(CognitiveTierFrameworkTest, CognitiveMonitoringMetricsCollection) {
        // Start monitoring session
        t81::ai::cognitive::OperationContext context{
            .operation_id = "metrics_test_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1"
        };
        
        cognitive_monitoring_->start_operation_monitoring("metrics_test_001", context);
        
        // Record metrics
        t81::ai::cognitive::OperationMetrics metrics{
            .timestamp = std::chrono::system_clock::now(),
            .cpu_time_ms = 250,
            .memory_used_mb = 45,
            .network_calls_made = 5,
            .status = "success"
        };
        
        cognitive_monitoring_->record_operation_metrics("metrics_test_001", metrics);
        
        // Complete monitoring
        t81::ai::cognitive::OperationResult result{
            .status = "completed",
            .completion_time = std::chrono::system_clock::now()
        };
        
        cognitive_monitoring_->complete_operation_monitoring("metrics_test_001", result);
        
        // Get session analysis
        auto analysis = cognitive_monitoring_->get_session_analysis("metrics_test_001");
        EXPECT_FALSE(analysis.session_id.empty());
        EXPECT_EQ(analysis.performance_rating, "excellent");  // Low usage, high efficiency
        EXPECT_GT(analysis.resource_efficiency, 0.8);  // Should be high efficiency
    }
    
    TEST_F(CognitiveTierFrameworkTest, CognitiveMonitoringAlertManagement) {
        // Start monitoring with high resource usage
        t81::ai::cognitive::OperationContext context{
            .operation_id = "alert_test_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
            .user_id = "user_t1"
        };
        
        cognitive_monitoring_->start_operation_monitoring("alert_test_001", context);
        
        // Record metrics that exceed thresholds
        t81::ai::cognitive::OperationMetrics high_usage_metrics{
            .timestamp = std::chrono::system_clock::now(),
            .cpu_time_ms = 1200,  // Exceeds T1 limit of 1000ms
            .memory_used_mb = 120,  // Exceeds T1 limit of 100MB
            .network_calls_made = 15,
            .status = "warning"
        };
        
        cognitive_monitoring_->record_operation_metrics("alert_test_001", high_usage_metrics);
        
        // Check for alerts
        auto active_alerts = cognitive_monitoring_->get_active_alerts();
        EXPECT_FALSE(active_alerts.empty());
        
        bool found_cpu_alert = false;
        bool found_memory_alert = false;
        
        for (const auto& alert : active_alerts) {
            if (alert.alert_type == "cpu_threshold_exceeded") {
                found_cpu_alert = true;
            }
            if (alert.alert_type == "memory_threshold_exceeded") {
                found_memory_alert = true;
            }
        }
        
        EXPECT_TRUE(found_cpu_alert);
        EXPECT_TRUE(found_memory_alert);
    }
    
    TEST_F(CognitiveTierFrameworkTest, CognitiveMonitoringDashboard) {
        // Get dashboard data
        auto dashboard = cognitive_monitoring_->get_dashboard_data();
        
        // Verify dashboard structure
        EXPECT_GE(dashboard.current_active_sessions, 0);
        EXPECT_GE(dashboard.total_sessions_today, 0);
        EXPECT_GE(dashboard.avg_session_duration, 0.0);
        
        // Verify tier distribution exists
        EXPECT_FALSE(dashboard.tier_distribution.empty());
        
        // Verify alert counts
        EXPECT_GE(dashboard.active_alerts_count, 0);
        EXPECT_GE(dashboard.critical_alerts_count, 0);
        
        // Verify system health
        EXPECT_FALSE(dashboard.system_health.empty());
        
        // Verify performance trends
        EXPECT_FALSE(dashboard.performance_trends.empty());
    }

    // Integration tests
    TEST_F(CognitiveTierFrameworkTest, IntegrationTierEngineAndMonitoring) {
        // Test integration between tier engine and monitoring
        
        // Create operation context
        t81::ai::cognitive::OperationContext context{
            .operation_id = "integration_test_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER2_REFLECTIVE,
            .user_id = "user_t2"
        };
        
        // Start monitoring
        cognitive_monitoring_->start_operation_monitoring("integration_test_001", context);
        
        // Check tier engine can authorize operation
        bool can_execute = tier_engine_->can_execute_operation(context, "self_monitoring");
        EXPECT_TRUE(can_execute);
        
        // Record metrics and complete
        t81::ai::cognitive::OperationMetrics metrics{
            .timestamp = std::chrono::system_clock::now(),
            .cpu_time_ms = 500,
            .memory_used_mb = 150,
            .network_calls_made = 2,
            .status = "success"
        };
        
        cognitive_monitoring_->record_operation_metrics("integration_test_001", metrics);
        
        t81::ai::cognitive::OperationResult result{
            .status = "completed",
            .completion_time = std::chrono::system_clock::now()
        };
        
        cognitive_monitoring_->complete_operation_monitoring("integration_test_001", result);
        
        // Verify both systems have consistent data
        auto analysis = cognitive_monitoring_->get_session_analysis("integration_test_001");
        EXPECT_EQ(analysis.compliance_status, "compliant");  // Should be within T2 limits
    }
    
    TEST_F(CognitiveTierFrameworkTest, IntegrationAGIGovernanceAndTierEngine) {
        // Test integration between AGI governance and tier engine
        
        // Create high-tier operation requiring AGI oversight
        t81::ai::cognitive::OperationContext context{
            .operation_id = "agi_integration_test_001",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER4_COLLABORATIVE,
            .user_id = "user_t4",
            .metadata = {
                {"authorized_tier", "TIER4_COLLABORATIVE"},
                {"safety_protocol_status", "compliant"}
            }
        };
        
        // Check tier engine authorization
        bool can_execute = tier_engine_->can_execute_operation(context, "multi_agent_coordination");
        EXPECT_TRUE(can_execute);
        
        // Check AGI governance safety enforcement
        bool safe_to_execute = agi_governance_->enforce_safety_constraints(
            context, "multi_agent_coordination");
        EXPECT_TRUE(safe_to_execute);  // Should be safe with proper metadata
        
        // Test unsafe scenario
        t81::ai::cognitive::OperationContext unsafe_context{
            .operation_id = "agi_integration_test_002",
            .current_tier = t81::ai::cognitive::CognitiveTier::TIER4_COLLABORATIVE,
            .user_id = "user_t4",
            .metadata = {
                {"authorized_tier", "TIER2_REFLECTIVE"},  // Lower than actual tier
                {"safety_protocol_status", "violated"}
            }
        };
        
        bool unsafe_can_execute = tier_engine_->can_execute_operation(unsafe_context, "multi_agent_coordination");
        EXPECT_FALSE(unsafe_can_execute);  // Should fail tier access check
        
        bool unsafe_safe_to_execute = agi_governance_->enforce_safety_constraints(
            unsafe_context, "multi_agent_coordination");
        EXPECT_FALSE(unsafe_safe_to_execute);  // Should fail safety check
    }

    // Performance and stress tests
    TEST_F(CognitiveTierFrameworkTest, PerformanceTierEngine) {
        // Test tier engine performance with multiple operations
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000; ++i) {
            t81::ai::cognitive::OperationContext context{
                .operation_id = "perf_test_" + std::to_string(i),
                .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
                .user_id = "perf_user",
                .execution_time_ms = 100,
                .memory_used_mb = 10
            };
            
            bool can_execute = tier_engine_->can_execute_operation(context, "symbolic_reasoning");
            EXPECT_TRUE(can_execute);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Should complete 1000 operations in reasonable time (less than 1 second)
        EXPECT_LT(duration.count(), 1000);
    }
    
    TEST_F(CognitiveTierFrameworkTest, StressMonitoringSystem) {
        // Test monitoring system with multiple concurrent sessions
        const int num_sessions = 100;
        
        // Start multiple monitoring sessions
        for (int i = 0; i < num_sessions; ++i) {
            t81::ai::cognitive::OperationContext context{
                .operation_id = "stress_test_" + std::to_string(i),
                .current_tier = t81::ai::cognitive::CognitiveTier::TIER1_SYMBOLIC,
                .user_id = "stress_user_" + std::to_string(i % 10)
            };
            
            cognitive_monitoring_->start_operation_monitoring(context.operation_id, context);
        }
        
        // Check active sessions
        auto active_sessions = cognitive_monitoring_->get_active_sessions();
        EXPECT_EQ(active_sessions.size(), num_sessions);
        
        // Complete all sessions
        for (int i = 0; i < num_sessions; ++i) {
            t81::ai::cognitive::OperationResult result{
                .status = "completed",
                .completion_time = std::chrono::system_clock::now()
            };
            
            cognitive_monitoring_->complete_operation_monitoring("stress_test_" + std::to_string(i), result);
        }
        
        // Verify all sessions completed
        auto final_active_sessions = cognitive_monitoring_->get_active_sessions();
        EXPECT_TRUE(final_active_sessions.empty());
    }
};

// Test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
