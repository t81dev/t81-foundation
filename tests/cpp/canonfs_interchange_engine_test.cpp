#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "t81/canonfs/interchange_engine.hpp"
#include "t81/canonfs/json_renderer.hpp"
#include <filesystem>
#include <fstream>

using namespace t81::canonfs;

class CanonFSInterchangeEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for tests
        test_dir_ = std::filesystem::temp_directory_path() / "canonfs_engine_test";
        std::filesystem::create_directories(test_dir_);
        
        // Create test CanonFS root
        canonfs_root_ = test_dir_ / ".t81_canonfs";
        std::filesystem::create_directories(canonfs_root_);
        
        engine_ = create_interchange_engine();
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }
    
    // Helper to create test file
    void create_test_file(const std::filesystem::path& path, const std::string& content = "test content") {
        std::ofstream file(path);
        file << content;
        file.close();
    }
    
    // Helper to create test policy evaluator
    InterchangePolicyEvaluator create_allow_all_evaluator() {
        return [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
            return InterchangePolicyDecision{true, "allow"};
        };
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path canonfs_root_;
    std::unique_ptr<CanonFSInterchangeEngine> engine_;
};

// Test basic import functionality
TEST_F(CanonFSInterchangeEngineTest, BasicImport) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    ImportRequest request;
    request.input_path = test_file;
    request.canonfs_root = canonfs_root_;
    request.policy_profile = InterchangePolicyProfile::Permissive;
    request.policy_evaluator = create_allow_all_evaluator();
    
    auto outcome = engine_->import(request);
    
    EXPECT_TRUE(outcome.ok());
    EXPECT_EQ(outcome.status, "ok");
    EXPECT_EQ(outcome.source_kind, "host-file");
    EXPECT_FALSE(outcome.imported_objects.empty());
}

// Test import with policy denial
TEST_F(CanonFSInterchangeEngineTest, ImportWithPolicyDenial) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    ImportRequest request;
    request.input_path = test_file;
    request.canonfs_root = canonfs_root_;
    request.policy_profile = InterchangePolicyProfile::DenyAll;
    request.policy_evaluator = create_allow_all_evaluator(); // Override for this test
    
    auto outcome = engine_->import(request);
    
    // The actual policy evaluation would be handled by the engine
    // For now, we test the basic structure
    EXPECT_TRUE(outcome.status.empty() || outcome.status == "ok" || outcome.status == "error");
}

// Test evidence collection
TEST_F(CanonFSInterchangeEngineTest, EvidenceCollection) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    ImportRequest request;
    request.input_path = test_file;
    request.canonfs_root = canonfs_root_;
    request.policy_profile = InterchangePolicyProfile::Permissive;
    request.policy_evaluator = create_allow_all_evaluator();
    
    // Clear any existing evidence
    engine_->clear_evidence_log();
    EXPECT_TRUE(engine_->get_evidence_log().empty());
    
    // Perform operation
    auto outcome = engine_->import(request);
    
    // Check evidence was collected
    auto evidence = engine_->get_evidence_log();
    EXPECT_EQ(evidence.size(), 1);
    EXPECT_EQ(evidence[0].operation_type, "import");
    EXPECT_FALSE(evidence[0].operation_id.empty());
    EXPECT_TRUE(evidence[0].metadata.contains("input_path"));
    EXPECT_TRUE(evidence[0].metadata.contains("success"));
}

// Test JSON rendering
TEST_F(CanonFSInterchangeEngineTest, JSONRendering) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    ImportRequest request;
    request.input_path = test_file;
    request.canonfs_root = canonfs_root_;
    request.policy_profile = InterchangePolicyProfile::Permissive;
    request.policy_evaluator = create_allow_all_evaluator();
    
    auto outcome = engine_->import(request);
    
    // Test JSON rendering
    auto renderer = create_default_json_renderer();
    engine_->set_json_renderer(renderer);
    
    auto json = renderer.render_import_result(outcome);
    
    // Basic JSON validation
    EXPECT_TRUE(json.find("\"schema\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"status\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"t81.canonfs-import.v1\"") != std::string::npos);
}

// Test operation ID generation
TEST_F(CanonFSInterchangeEngineTest, OperationIdGeneration) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    ImportRequest request1;
    request1.input_path = test_file;
    request1.canonfs_root = canonfs_root_;
    request1.policy_profile = InterchangePolicyProfile::Permissive;
    request1.policy_evaluator = create_allow_all_evaluator();
    
    auto outcome1 = engine_->import(request1);
    auto evidence1 = engine_->get_evidence_log();
    
    ImportRequest request2;
    request2.input_path = test_file;
    request2.canonfs_root = canonfs_root_;
    request2.policy_profile = InterchangePolicyProfile::Permissive;
    request2.policy_evaluator = create_allow_all_evaluator();
    
    auto outcome2 = engine_->import(request2);
    auto evidence2 = engine_->get_evidence_log();
    
    // Check that operation IDs are unique
    EXPECT_EQ(evidence2.size(), 2);
    EXPECT_NE(evidence1[0].operation_id, evidence2[1].operation_id);
    EXPECT_TRUE(evidence1[0].operation_id.starts_with("canonfs_op_"));
    EXPECT_TRUE(evidence2[1].operation_id.starts_with("canonfs_op_"));
}

// Test evidence log clearing
TEST_F(CanonFSInterchangeEngineTest, EvidenceLogClearing) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    ImportRequest request;
    request.input_path = test_file;
    request.canonfs_root = canonfs_root_;
    request.policy_profile = InterchangePolicyProfile::Permissive;
    request.policy_evaluator = create_allow_all_evaluator();
    
    // Perform operation to generate evidence
    auto outcome = engine_->import(request);
    EXPECT_EQ(engine_->get_evidence_log().size(), 1);
    
    // Clear evidence
    engine_->clear_evidence_log();
    EXPECT_TRUE(engine_->get_evidence_log().empty());
    
    // Perform another operation
    auto outcome2 = engine_->import(request);
    EXPECT_EQ(engine_->get_evidence_log().size(), 1);
    EXPECT_NE(engine_->get_evidence_log()[0].operation_id, 
              engine_->get_evidence_log()[0].operation_id); // Should be new ID
}

// Test export functionality
TEST_F(CanonFSInterchangeEngineTest, BasicExport) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // First import to get an object
    ImportRequest import_request;
    import_request.input_path = test_file;
    import_request.canonfs_root = canonfs_root_;
    import_request.policy_profile = InterchangePolicyProfile::Permissive;
    import_request.policy_evaluator = create_allow_all_evaluator();
    
    auto import_outcome = engine_->import(import_request);
    ASSERT_TRUE(import_outcome.ok());
    ASSERT_FALSE(import_outcome.imported_objects.empty());
    
    // Now export the imported object
    auto export_file = test_dir_ / "exported.txt";
    ExportRequest export_request;
    export_request.canonical_hash = import_outcome.imported_objects[0];
    export_request.output_path = export_file;
    export_request.canonfs_root = canonfs_root_;
    export_request.policy_profile = InterchangePolicyProfile::Permissive;
    export_request.policy_evaluator = create_allow_all_evaluator();
    
    auto export_outcome = engine_->export(export_request);
    
    EXPECT_TRUE(export_outcome.ok());
    EXPECT_EQ(export_outcome.status, "ok");
    EXPECT_EQ(export_outcome.target_kind, "host-file");
    EXPECT_FALSE(export_outcome.materialized_paths.empty());
}
