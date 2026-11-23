// Model Checkpoint Bundles Test Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Tests model checkpointing capabilities with bundle integration

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include <cmath>

#include "model_checkpoint_bundles.cpp"

namespace t81::vm::advanced_ai::test {

// Model checkpoint test suite
class ModelCheckpointTester {
private:
    std::vector<int64_t> test_layer_ids_;
    
public:
    ModelCheckpointTester() {
        // Initialize test layer IDs
        test_layer_ids_ = {1001, 1002, 1003}; // Mock layer IDs
    }
    
    void run_all_tests() {
        std::cout << "=== Model Checkpoint Bundles Test Suite ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        // Test 1: Checkpoint configuration
        test_checkpoint_configuration();
        
        // Test 2: Basic checkpoint creation
        test_basic_checkpoint_creation();
        
        // Test 3: Checkpoint manager initialization
        test_checkpoint_manager_initialization();
        
        // Test 4: Automatic checkpointing
        test_automatic_checkpointing();
        
        // Test 5: Checkpoint loading and restoration
        test_checkpoint_loading();
        
        // Test 6: Checkpoint statistics
        test_checkpoint_statistics();
        
        // Test 7: Checkpoint comparison
        test_checkpoint_comparison();
        
        // Test 8: Final checkpoint creation
        test_final_checkpoint_creation();
        
        // Test 9: Checkpoint-aware training
        test_checkpoint_aware_training();
        
        // Test 10: Checkpoint export and analysis
        test_checkpoint_export_and_analysis();
        
        std::cout << std::endl;
        std::cout << "=== Model Checkpoint Test Results ===" << std::endl;
        std::cout << "All model checkpoint tests completed successfully!" << std::endl;
        std::cout << "Model checkpoint bundles are ready for experimental research use." << std::endl;
    }

private:
    void test_checkpoint_configuration() {
        std::cout << "\n--- Checkpoint Configuration Test ---" << std::endl;
        
        // Test different checkpoint configurations
        std::vector<CheckpointConfig> configs = {
            CheckpointConfig(true, 5, true, true, "test_checkpoint", 12345, 0.001),
            CheckpointConfig(false, 10, false, false, "no_auto", 67890, 0.005),
            CheckpointConfig(true, 3, true, true, "freq_test", 54321, 0.0005),
            CheckpointConfig(true, 15, false, true, "val_only", 98765, 0.01)
        };
        
        for (size_t i = 0; i < configs.size(); ++i) {
            const auto& config = configs[i];
            
            std::cout << "  Checkpoint Config " << (i + 1) << ": ";
            std::cout << "auto=" << (config.auto_checkpoint_enabled ? "true" : "false");
            std::cout << ", freq=" << config.checkpoint_frequency;
            std::cout << ", val_improvement=" << (config.checkpoint_on_validation_improvement ? "true" : "false");
            std::cout << ", train_completion=" << (config.checkpoint_on_training_completion ? "true" : "false");
            std::cout << ", prefix=" << config.checkpoint_name_prefix;
            std::cout << ", threshold=" << config.min_improvement_threshold;
            std::cout << " ✅" << std::endl;
            
            // Validate configuration
            assert_true(config.checkpoint_frequency > 0, "Checkpoint frequency must be positive");
            assert_true(config.min_improvement_threshold >= 0.0, "Improvement threshold must be non-negative");
            assert_true(!config.checkpoint_name_prefix.empty(), "Checkpoint prefix must not be empty");
        }
        
        std::cout << "✅ Checkpoint configuration test passed" << std::endl;
    }
    
    void test_basic_checkpoint_creation() {
        std::cout << "\n--- Basic Checkpoint Creation Test ---" << std::endl;
        
        CheckpointConfig config(true, 5, true, true, "basic_test", 12345, 0.001);
        ModelCheckpointManager manager(config);
        
        // Create mock training metrics
        TrainingMetrics metrics;
        metrics.avg_loss = 0.234;
        metrics.accuracy = 0.856;
        metrics.batches_processed = 100;
        metrics.training_time_ms = 1250;
        
        // Create checkpoint
        std::string checkpoint_id = manager.create_checkpoint(
            test_layer_ids_, metrics, "parent_bundle_123", {{"learning_rate", "0.001"}, {"batch_size", "32"}});
        
        std::cout << "  Created checkpoint: " << checkpoint_id << std::endl;
        assert_true(!checkpoint_id.empty(), "Checkpoint ID assigned");
        
        // Check checkpoint statistics
        auto stats = manager.get_checkpoint_statistics();
        std::cout << "  Total checkpoints: " << stats.total_checkpoints << std::endl;
        std::cout << "  Best validation accuracy: " << (stats.best_validation_accuracy * 100) << "%" << std::endl;
        
        assert_true(stats.total_checkpoints == 1, "One checkpoint created");
        assert_true(stats.best_validation_accuracy == metrics.accuracy, "Best accuracy updated");
        
        std::cout << "✅ Basic checkpoint creation test passed" << std::endl;
    }
    
    void test_checkpoint_manager_initialization() {
        std::cout << "\n--- Checkpoint Manager Initialization Test ---" << std::endl;
        
        // Test with auto-checkpoint enabled
        CheckpointConfig config_auto(true, 3, true, true, "init_test", 23456, 0.002);
        ModelCheckpointManager manager_auto(config_auto);
        
        std::cout << "  Auto-checkpoint enabled manager initialized ✅" << std::endl;
        std::cout << "  Model version: " << manager_auto.get_model_version() << std::endl;
        
        // Test with auto-checkpoint disabled
        CheckpointConfig config_manual(false, 0, false, false, "manual_test", 34567, 0.003);
        ModelCheckpointManager manager_manual(config_manual);
        
        std::cout << "  Manual checkpoint manager initialized ✅" << std::endl;
        
        // Test model version setting
        manager_auto.set_model_version("v2.1.0");
        std::cout << "  Updated model version: " << manager_auto.get_model_version() << std::endl;
        
        assert_true(manager_auto.get_model_version() == "v2.1.0", "Model version updated");
        
        std::cout << "✅ Checkpoint manager initialization test passed" << std::endl;
    }
    
    void test_automatic_checkpointing() {
        std::cout << "\n--- Automatic Checkpointing Test ---" << std::endl;
        
        CheckpointConfig config(true, 2, true, true, "auto_test", 45678, 0.001);
        ModelCheckpointManager manager(config);
        
        // Simulate training progression
        std::vector<TrainingMetrics> training_progress = {
            {0.8, 0.65, 50, 600},   // Epoch 1
            {0.6, 0.72, 50, 580},   // Epoch 2
            {0.5, 0.78, 50, 590},   // Epoch 3
            {0.4, 0.83, 50, 605},   // Epoch 4
            {0.35, 0.85, 50, 595}   // Epoch 5
        };
        
        std::vector<std::string> created_checkpoints;
        
        for (size_t epoch = 0; epoch < training_progress.size(); ++epoch) {
            const auto& metrics = training_progress[epoch];
            
            // Check if checkpoint should be created
            bool should_create = manager.should_create_checkpoint(metrics, epoch + 1, 50);
            
            if (should_create) {
                std::string checkpoint_id = manager.create_checkpoint(
                    test_layer_ids_, metrics, "training_bundle_" + std::to_string(epoch));
                created_checkpoints.push_back(checkpoint_id);
                
                std::cout << "  Epoch " << (epoch + 1) << ": Checkpoint created (acc=" 
                         << (metrics.accuracy * 100) << "%)" << std::endl;
            } else {
                std::cout << "  Epoch " << (epoch + 1) << ": No checkpoint (acc=" 
                         << (metrics.accuracy * 100) << "%)" << std::endl;
            }
        }
        
        std::cout << "  Total automatic checkpoints: " << created_checkpoints.size() << std::endl;
        
        // Validate automatic checkpointing
        assert_true(created_checkpoints.size() >= 2, "Multiple checkpoints created automatically");
        
        auto stats = manager.get_checkpoint_statistics();
        assert_true(stats.total_checkpoints == created_checkpoints.size(), "All checkpoints tracked");
        
        std::cout << "✅ Automatic checkpointing test passed" << std::endl;
    }
    
    void test_checkpoint_loading() {
        std::cout << "\n--- Checkpoint Loading Test ---" << std::endl;
        
        CheckpointConfig config(true, 5, true, true, "load_test", 56789, 0.001);
        ModelCheckpointManager manager(config);
        
        // Create initial checkpoint
        TrainingMetrics initial_metrics;
        initial_metrics.avg_loss = 0.45;
        initial_metrics.accuracy = 0.78;
        initial_metrics.batches_processed = 100;
        initial_metrics.training_time_ms = 1500;
        
        std::string checkpoint_id = manager.create_checkpoint(
            test_layer_ids_, initial_metrics, "parent_bundle_456");
        
        std::cout << "  Created checkpoint: " << checkpoint_id << std::endl;
        
        // Test loading from checkpoint
        bool load_success = manager.load_from_checkpoint(checkpoint_id, test_layer_ids_);
        
        std::cout << "  Load success: " << (load_success ? "true" : "false") << std::endl;
        
        assert_true(load_success, "Checkpoint loaded successfully");
        
        // Test loading non-existent checkpoint
        bool load_fail = manager.load_from_checkpoint("non_existent_checkpoint", test_layer_ids_);
        
        std::cout << "  Non-existent load success: " << (load_fail ? "true" : "false") << std::endl;
        
        assert_true(!load_fail, "Non-existent checkpoint load failed as expected");
        
        std::cout << "✅ Checkpoint loading test passed" << std::endl;
    }
    
    void test_checkpoint_statistics() {
        std::cout << "\n--- Checkpoint Statistics Test ---" << std::endl;
        
        CheckpointConfig config(true, 1, true, true, "stats_test", 67890, 0.0005);
        ModelCheckpointManager manager(config);
        
        // Create multiple checkpoints with different performance
        std::vector<TrainingMetrics> metrics_list = {
            {0.8, 0.65, 50, 600},
            {0.6, 0.72, 50, 580},
            {0.4, 0.85, 50, 590},
            {0.35, 0.87, 50, 605}
        };
        
        std::vector<std::string> checkpoint_ids;
        
        for (size_t i = 0; i < metrics_list.size(); ++i) {
            std::string cp_id = manager.create_checkpoint(
                test_layer_ids_, metrics_list[i], "training_bundle_" + std::to_string(i));
            checkpoint_ids.push_back(cp_id);
        }
        
        // Get statistics
        auto stats = manager.get_checkpoint_statistics();
        
        std::cout << "  Checkpoint Statistics:" << std::endl;
        std::cout << "    Total checkpoints: " << stats.total_checkpoints << std::endl;
        std::cout << "    Best validation accuracy: " << (stats.best_validation_accuracy * 100) << "%" << std::endl;
        std::cout << "    Best checkpoint ID: " << stats.best_checkpoint_id << std::endl;
        std::cout << "    Total checkpoint size: " << stats.total_checkpoint_size << " bytes" << std::endl;
        std::cout << "    Checkpoint IDs: ";
        for (size_t i = 0; i < stats.checkpoint_ids.size(); ++i) {
            std::cout << stats.checkpoint_ids[i];
            if (i < stats.checkpoint_ids.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // Validate statistics
        assert_true(stats.total_checkpoints == metrics_list.size(), "Correct checkpoint count");
        assert_true(stats.best_validation_accuracy == 0.87, "Best accuracy identified correctly");
        assert_true(!stats.best_checkpoint_id.empty(), "Best checkpoint ID assigned");
        assert_true(stats.checkpoint_ids.size() == metrics_list.size(), "All checkpoint IDs tracked");
        
        std::cout << "✅ Checkpoint statistics test passed" << std::endl;
    }
    
    void test_checkpoint_comparison() {
        std::cout << "\n--- Checkpoint Comparison Test ---" << std::endl;
        
        CheckpointConfig config(true, 5, true, true, "compare_test", 78901, 0.001);
        ModelCheckpointManager manager(config);
        
        // Create two checkpoints with different performance
        TrainingMetrics metrics1 = {0.6, 0.75, 50, 600};
        TrainingMetrics metrics2 = {0.4, 0.82, 50, 580};
        
        std::string cp1_id = manager.create_checkpoint(test_layer_ids_, metrics1, "parent_1");
        std::string cp2_id = manager.create_checkpoint(test_layer_ids_, metrics2, "parent_2");
        
        std::cout << "  Created checkpoints:" << std::endl;
        std::cout << "    " << cp1_id << " (acc=" << (metrics1.accuracy * 100) << "%)" << std::endl;
        std::cout << "    " << cp2_id << " (acc=" << (metrics2.accuracy * 100) << "%)" << std::endl;
        
        // Compare checkpoints
        auto comparison = manager.compare_checkpoints(cp1_id, cp2_id);
        
        std::cout << "  Comparison Results:" << std::endl;
        std::cout << "    Accuracy difference: " << (comparison.accuracy_difference * 100) << "%" << std::endl;
        std::cout << "    Loss difference: " << comparison.loss_difference << std::endl;
        std::cout << "    Time difference: " << comparison.time_difference_ms << "ms" << std::endl;
        std::cout << "    Layer differences: " << comparison.layer_differences.size() << std::endl;
        
        // Validate comparison
        assert_true(comparison.accuracy_difference > 0, "Accuracy difference positive (cp2 better)");
        assert_true(comparison.loss_difference < 0, "Loss difference negative (cp2 better)");
        assert_true(comparison.time_difference_ms > 0, "Time difference positive (cp2 created later)");
        
        std::cout << "✅ Checkpoint comparison test passed" << std::endl;
    }
    
    void test_final_checkpoint_creation() {
        std::cout << "\n--- Final Checkpoint Creation Test ---" << std::endl;
        
        CheckpointConfig config(true, 3, true, true, "final_test", 89012, 0.001);
        ModelCheckpointManager manager(config);
        
        // Create some regular checkpoints first
        TrainingMetrics regular_metrics = {0.5, 0.78, 50, 590};
        manager.create_checkpoint(test_layer_ids_, regular_metrics, "regular_parent");
        
        std::cout << "  Created regular checkpoint ✅" << std::endl;
        
        // Create final checkpoint
        TrainingMetrics final_metrics = {0.35, 0.86, 50, 610};
        std::string final_checkpoint_id = manager.create_final_checkpoint(
            test_layer_ids_, final_metrics, "final_parent");
        
        std::cout << "  Created final checkpoint: " << final_checkpoint_id << std::endl;
        
        // Validate final checkpoint
        assert_true(!final_checkpoint_id.empty(), "Final checkpoint ID assigned");
        assert_true(final_checkpoint_id.find("_final") != std::string::npos, "Final checkpoint marked");
        
        // Check statistics
        auto stats = manager.get_checkpoint_statistics();
        std::cout << "  Total checkpoints: " << stats.total_checkpoints << std::endl;
        std::cout << "  Latest checkpoint: " << stats.latest_checkpoint_id << std::endl;
        
        assert_true(stats.total_checkpoints == 2, "Regular + final checkpoints");
        assert_true(stats.latest_checkpoint_id == final_checkpoint_id, "Final checkpoint is latest");
        
        std::cout << "✅ Final checkpoint creation test passed" << std::endl;
    }
    
    void test_checkpoint_aware_training() {
        std::cout << "\n--- Checkpoint-Aware Training Test ---" << std::endl;
        
        // Configure bundle processing
        BatchConfig base_config(16, 32, 3, true, 12345);
        BundleBatchConfig bundle_config(base_config, true, "training_test", true, 5, 23456);
        
        // Configure checkpointing
        CheckpointConfig checkpoint_config(true, 2, true, true, "aware_test", 34567, 0.001);
        
        CheckpointAwareTrainingOrchestrator orchestrator(bundle_config, test_layer_ids_, checkpoint_config);
        
        // Generate training data
        auto train_dataset = BatchUtils::generate_synthetic_dataset(100, 32, 4);
        auto train_targets = BatchUtils::generate_one_hot_targets(train_dataset, 4);
        auto val_dataset = BatchUtils::generate_synthetic_dataset(20, 32, 4);
        auto val_targets = BatchUtils::generate_one_hot_targets(val_dataset, 4);
        
        std::cout << "  Generated training data: " << train_dataset.size() << " samples" << std::endl;
        std::cout << "  Generated validation data: " << val_dataset.size() << " samples" << std::endl;
        
        // Train with checkpoints
        auto results = orchestrator.train_with_checkpoints(
            train_dataset, train_targets,
            val_dataset, val_targets,
            3, // epochs
            0.001, // learning rate
            {{"optimizer", "adam"}, {"momentum", "0.9"}} // hyperparameters
        );
        
        std::cout << "  Checkpoint-Aware Training Results:" << std::endl;
        std::cout << "    Training epochs: " << results.training_results.training_results.total_epochs << std::endl;
        std::cout << "    Training time: " << results.training_results.training_results.total_training_time_ms << "ms" << std::endl;
        std::cout << "    Checkpoint overhead: " << results.checkpoint_overhead_ms << "ms" << std::endl;
        std::cout << "    Created checkpoints: " << results.created_checkpoint_ids.size() << std::endl;
        std::cout << "    Final checkpoint: " << results.final_checkpoint_id << std::endl;
        std::cout << "    Checkpoint stats:" << std::endl;
        std::cout << "      Total: " << results.checkpoint_stats.total_checkpoints << std::endl;
        std::cout << "      Best accuracy: " << (results.checkpoint_stats.best_validation_accuracy * 100) << "%" << std::endl;
        
        // Validate results
        assert_true(results.training_results.training_results.total_epochs == 3, "Correct epoch count");
        assert_true(!results.final_checkpoint_id.empty(), "Final checkpoint created");
        assert_true(results.checkpoint_stats.total_checkpoints > 0, "Checkpoints created");
        assert_true(results.checkpoint_overhead_ms >= 0, "Checkpoint overhead measured");
        
        std::cout << "✅ Checkpoint-aware training test passed" << std::endl;
    }
    
    void test_checkpoint_export_and_analysis() {
        std::cout << "\n--- Checkpoint Export and Analysis Test ---" << std::endl;
        
        CheckpointConfig config(true, 3, true, true, "export_test", 90123, 0.002);
        ModelCheckpointManager manager(config);
        
        // Create multiple checkpoints
        for (int i = 0; i < 3; ++i) {
            TrainingMetrics metrics = {0.8 - (i * 0.1), 0.7 + (i * 0.05), 50, 600 + (i * 10)};
            manager.create_checkpoint(test_layer_ids_, metrics, "parent_" + std::to_string(i));
        }
        
        // Export checkpoint data
        auto export_data = manager.export_checkpoint_data();
        
        std::cout << "  Export Data (" << export_data.size() << " entries):" << std::endl;
        for (const auto& [key, value] : export_data) {
            std::cout << "    " << key << "=" << value << std::endl;
        }
        
        // List checkpoints
        auto checkpoint_list = manager.list_checkpoints();
        std::cout << "  Available Checkpoints (" << checkpoint_list.size() << "):" << std::endl;
        for (const auto& checkpoint_info : checkpoint_list) {
            std::cout << "    " << checkpoint_info << std::endl;
        }
        
        // Validate export data
        assert_true(!export_data.empty(), "Export data not empty");
        assert_true(export_data.count("auto_checkpoint_enabled") > 0, "Auto checkpoint flag exported");
        assert_true(export_data.count("total_checkpoints_created") > 0, "Checkpoint count exported");
        assert_true(export_data.count("checkpoint_ids") > 0, "Checkpoint IDs exported");
        
        assert_true(checkpoint_list.size() == 3, "All checkpoints listed");
        
        std::cout << "✅ Checkpoint export and analysis test passed" << std::endl;
    }
    
    void assert_true(bool condition, const std::string& test_name) {
        if (condition) {
            std::cout << "✅ " << test_name << std::endl;
        } else {
            std::cout << "❌ " << test_name << std::endl;
            assert(false);
        }
    }
};

} // namespace t81::vm::advanced_ai::test

// Main test runner
int main(int argc, char** argv) {
    std::cout << "Model Checkpoint Bundles Test Runner" << std::endl;
    std::cout << "====================================" << std::endl;
    
    t81::vm::advanced_ai::test::ModelCheckpointTester tester;
    tester.run_all_tests();
    
    return 0;
}
