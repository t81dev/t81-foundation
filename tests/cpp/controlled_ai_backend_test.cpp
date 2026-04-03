#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "t81/ai_backend/controlled_ai_backend.hpp"
#include "t81/axion/policy_engine.hpp"
#include <fstream>
#include <sstream>

using namespace t81::ai_backend;
using namespace t81::axion;

class ControlledAIBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple policy for testing
        test_policy_text_ = R"(
policy "test.ai.policy" {
    description: "Test policy for controlled AI backend",
    version: "1.0",
    determinism_level: "controlled",
    evidence_collection: true,
    require_explicit_consent: true
}
        )";
        
        auto policy = PolicyEngine::parse_policy(test_policy_text_);
        policy_engine_ = std::make_unique<PolicyEngine>(policy);
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    std::unique_ptr<PolicyEngine> policy_engine_;
    std::string test_policy_text_;
};

// Test deterministic inference (no randomness)
TEST_F(ControlledAIBackendTest, DeterministicInference) {
    AIBackendConfig config;
    config.determinism_level = "strict";
    
    auto backend = create_controlled_ai_backend(std::move(policy_engine_), config);
    
    InferenceRequest request("model123", 10, 0.0, "test input");
    auto result = backend->inference(request);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.output_tokens, 10); // Deterministic: input = output
    EXPECT_EQ(result.output_text, "Deterministic AI response: 10");
    
    // Check evidence
    auto evidence = backend->get_evidence_log();
    EXPECT_EQ(evidence.size(), 1);
    EXPECT_EQ(evidence[0].determinism_level, "strict");
    EXPECT_FALSE(evidence[0].user_consent);
}

// Test controlled non-deterministic inference
TEST_F(ControlledAIBackendTest, ControlledNonDeterministicInference) {
    AIBackendConfig config;
    config.determinism_level = "controlled";
    
    auto backend = create_controlled_ai_backend(std::move(policy_engine_), config);
    
    InferenceRequest request("model123", 10, 0.7, "test input");
    auto result = backend->inference(request);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.output_tokens, 5); // Should vary due to randomness
    EXPECT_LT(result.output_tokens, 15); // But within reasonable bounds
    EXPECT_NE(result.output_text, "Deterministic AI response: 10");
    
    // Check evidence
    auto evidence = backend->get_evidence_log();
    EXPECT_EQ(evidence.size(), 1);
    EXPECT_EQ(evidence[0].determinism_level, "controlled");
    EXPECT_FALSE(evidence[0].user_consent);
}

// Test external AI integration
TEST_F(ControlledAIBackendTest, ExternalAIIntegration) {
    AIBackendConfig config;
    config.determinism_level = "controlled";
    config.external_ai_endpoint = "https://api.test.ai";
    config.external_ai_auth_token = "test_token";
    
    auto backend = create_controlled_ai_backend(std::move(policy_engine_), config);
    
    InferenceRequest request("model123", 10, 0.5, "test input");
    auto result = backend->inference(request);
    
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.output_text, "Deterministic AI response: 10");
    
    // Check evidence
    auto evidence = backend->get_evidence_log();
    EXPECT_EQ(evidence.size(), 1);
    EXPECT_EQ(evidence[0].determinism_level, "controlled");
    EXPECT_TRUE(evidence[0].external_ai_used);
    EXPECT_EQ(evidence[0].metadata.at("ai_service"), "https://api.test.ai");
}

// Test evidence collection
TEST_F(ControlledAIBackendTest, EvidenceCollection) {
    AIBackendConfig config;
    config.determinism_level = "controlled";
    
    auto backend = create_controlled_ai_backend(std::move(policy_engine_), config);
    
    // Run multiple operations
    for (int i = 0; i < 3; ++i) {
        InferenceRequest request("model123", 10, 0.5, "test input " + std::to_string(i));
        backend->inference(request);
    }
    
    auto evidence = backend->get_evidence_log();
    EXPECT_EQ(evidence.size(), 3);
    
    // Check evidence JSON format
    std::string evidence_json = backend->get_evidence_json();
    EXPECT_TRUE(evidence_json.find("\"operation_id\"") != std::string::npos);
    EXPECT_TRUE(evidence_json.find("\"determinism_level\"") != std::string::npos);
    EXPECT_TRUE(evidence_json.find("\"policy_decisions\"") != std::string::npos);
    
    // Clear evidence
    backend->clear_evidence_log();
    EXPECT_EQ(backend->get_evidence_log().size(), 0);
}

// Test policy consent checking
TEST_F(ControlledAIBackendTest, PolicyConsentChecking) {
    // Create a restrictive policy
    std::string restrictive_policy = R"(
policy "restrictive.ai.policy" {
    description: "Restrictive policy for testing",
    version: "1.0",
    determinism_level: "strict",
    require_explicit_consent: true,
    deny_conditions: [
        {
            name: "no_consent",
            description: "User consent not provided",
            trigger_fields: ["user.consent_missing"]
        }
    ]
}
        )";
    
    auto restrictive_policy_engine = PolicyEngine::parse_policy(restrictive_policy);
    auto policy_engine = std::make_unique<PolicyEngine>(restrictive_policy_engine);
    
    AIBackendConfig config;
    config.determinism_level = "controlled";
    config.require_user_consent = false; // No consent provided
    
    auto backend = create_controlled_ai_backend(std::move(policy_engine), config);
    
    InferenceRequest request("model123", 10, 0.5, "test input");
    auto result = backend->inference(request);
    
    // Should fail due to policy denial
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    
    // Check evidence shows policy denial
    auto evidence = backend->get_evidence_log();
    EXPECT_EQ(evidence.size(), 1);
    EXPECT_FALSE(evidence[0].user_consent);
    EXPECT_GT(evidence[0].policy_decisions.size(), 0);
}

// Test determinism level changes
TEST_F(ControlledAIBackendTest, DeterminismLevelChanges) {
    AIBackendConfig config;
    config.determinism_level = "strict";
    
    auto backend = create_controlled_ai_backend(std::move(policy_engine_), config);
    
    InferenceRequest request("model123", 10, 0.5, "test input");
    
    // Test strict determinism
    auto result1 = backend->inference(request);
    EXPECT_TRUE(result1.success);
    EXPECT_EQ(result1.output_text, "Deterministic AI response: 10");
    
    // Change to controlled
    backend->set_determinism_level("controlled");
    auto result2 = backend->inference(request);
    EXPECT_TRUE(result2.success);
    EXPECT_NE(result2.output_text, "Deterministic AI response: 10");
    
    // Change back to strict
    backend->set_determinism_level("strict");
    auto result3 = backend->inference(request);
    EXPECT_TRUE(result3.success);
    EXPECT_EQ(result3.output_text, "Deterministic AI response: 10");
}
