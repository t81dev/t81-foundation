// Bundle and Batch Processing Integration Test Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Tests the integration of CanonFS bundle system with mini-batch processing

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include <cmath>

#include "bundle_batch_integration.cpp"

namespace t81::vm::advanced_ai::test {

// Bundle batch processing test suite
class BundleBatchIntegrationTester {
private:
    std::vector<int64_t> test_layer_ids_;
    
public:
    BundleBatchIntegrationTester() {
        // Initialize test layer IDs
        test_layer_ids_ = {1001, 1002, 1003}; // Mock layer IDs
    }
    
    void run_all_tests() {
        std::cout << "=== Bundle and Batch Processing Integration Test Suite ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        // Test 1: Bundle batch configuration
        test_bundle_batch_configuration();
        
        // Test 2: Bundle creation and management
        test_bundle_creation_and_management();
        
        // Test 3: Bundle-aware batch processing
        test_bundle_aware_batch_processing();
        
        // Test 4: Bundle provenance tracking
        test_bundle_provenance_tracking();
        
        // Test 5: Bundle training with provenance
        test_bundle_training_with_provenance();
        
        // Test 6: Bundle statistics and monitoring
        test_bundle_statistics();
        
        // Test 7: Bundle frequency management
        test_bundle_frequency_management();
        
        // Test 8: Bundle export and analysis
        test_bundle_export_and_analysis();
        
        // Test 9: Bundle performance impact
        test_bundle_performance_impact();
        
        // Test 10: Multi-epoch bundle training
        test_multi_epoch_bundle_training();
        
        std::cout << std::endl;
        std::cout << "=== Bundle Batch Integration Test Results ===" << std::endl;
        std::cout << "All bundle-batch integration tests completed successfully!" << std::endl;
        std::cout << "Bundle-enhanced batch processing is ready for experimental research use." << std::endl;
    }

private:
    void test_bundle_batch_configuration() {
        std::cout << "\n--- Bundle Batch Configuration Test ---" << std::endl;
        
        // Test different bundle configurations
        BatchConfig base_config(32, 50, 3, true, 12345);
        
        std::vector<BundleBatchConfig> bundle_configs = {
            BundleBatchConfig(base_config, true, "ai_batch", true, 10, 54321),
            BundleBatchConfig(base_config, false, "no_bundle", false, 5, 67890),
            BundleBatchConfig(base_config, true, "research_batch", true, 20, 98765),
            BundleBatchConfig(base_config, true, "prod_batch", false, 50, 11111)
        };
        
        for (size_t i = 0; i < bundle_configs.size(); ++i) {
            const auto& config = bundle_configs[i];
            
            std::cout << "  Bundle Config " << (i + 1) << ": ";
            std::cout << "enabled=" << (config.bundle_enabled ? "true" : "false");
            std::cout << ", prefix=" << config.bundle_name_prefix;
            std::cout << ", auto_create=" << (config.auto_bundle_creation ? "true" : "false");
            std::cout << ", frequency=" << config.bundle_frequency;
            std::cout << " ✅" << std::endl;
            
            // Validate configuration
            assert_true(config.bundle_frequency > 0, "Bundle frequency must be positive");
            assert_true(!config.bundle_name_prefix.empty(), "Bundle prefix must not be empty");
        }
        
        std::cout << "✅ Bundle batch configuration test passed" << std::endl;
    }
    
    void test_bundle_creation_and_management() {
        std::cout << "\n--- Bundle Creation and Management Test ---" << std::endl;
        
        BatchConfig base_config(16, 32, 2, true, 12345);
        BundleBatchConfig bundle_config(base_config, true, "test_bundle", true, 5, 23456);
        
        BundleBatchProcessor processor(bundle_config);
        
        // Test initial bundle creation
        std::string initial_bundle_id = processor.get_current_bundle_id();
        std::cout << "  Initial bundle ID: " << initial_bundle_id << std::endl;
        assert_true(!initial_bundle_id.empty() && initial_bundle_id != "no_bundle", 
                   "Initial bundle created successfully");
        
        // Test bundle statistics
        auto stats = processor.get_bundle_statistics();
        std::cout << "  Bundle statistics:" << std::endl;
        std::cout << "    Total batches processed: " << stats.total_batches_processed << std::endl;
        std::cout << "    Completed bundles count: " << stats.completed_bundles_count << std::endl;
        std::cout << "    Bundle IDs count: " << stats.bundle_ids.size() << std::endl;
        
        assert_true(stats.bundle_ids.size() >= 1, "At least one bundle ID available");
        
        // Test bundle export
        auto export_data = processor.export_bundle_data();
        std::cout << "  Export data keys: ";
        for (const auto& [key, value] : export_data) {
            std::cout << key << " ";
        }
        std::cout << std::endl;
        
        assert_true(!export_data.empty(), "Export data not empty");
        assert_true(export_data.count("bundle_enabled") > 0, "Bundle enabled flag exported");
        
        std::cout << "✅ Bundle creation and management test passed" << std::endl;
    }
    
    void test_bundle_aware_batch_processing() {
        std::cout << "\n--- Bundle-Aware Batch Processing Test ---" << std::endl;
        
        // Generate test dataset
        auto dataset = BatchUtils::generate_synthetic_dataset(64, 32, 4);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 4);
        
        BatchConfig base_config(16, 32, 4, false, 34567);
        BundleBatchConfig bundle_config(base_config, true, "processing_test", true, 3, 45678);
        
        BundleBatchProcessor processor(bundle_config);
        
        // Create bundle-aware batch
        BundleBatchData bundle_batch = processor.create_bundle_batch(dataset, targets, 0);
        
        std::cout << "  Bundle batch created:" << std::endl;
        std::cout << "    Batch ID: " << bundle_batch.batch_id << std::endl;
        std::cout << "    Bundle ID: " << bundle_batch.bundle_id << std::endl;
        std::cout << "    Samples: " << bundle_batch.inputs.size() << std::endl;
        std::cout << "    Provenance IDs: " << bundle_batch.sample_provenance_ids.size() << std::endl;
        std::cout << "    Metadata entries: " << bundle_batch.batch_metadata.size() << std::endl;
        std::cout << "    Bundle timestamp: " << bundle_batch.bundle_timestamp << std::endl;
        
        // Validate bundle batch
        assert_true(!bundle_batch.bundle_id.empty(), "Bundle ID assigned");
        assert_true(bundle_batch.sample_provenance_ids.size() == bundle_batch.inputs.size(), 
                   "Provenance IDs match sample count");
        assert_true(!bundle_batch.batch_metadata.empty(), "Batch metadata created");
        assert_true(bundle_batch.bundle_timestamp > 0, "Bundle timestamp set");
        
        // Test bundle-aware forward pass
        auto outputs = processor.bundle_forward_pass(bundle_batch, test_layer_ids_[0]);
        std::cout << "  Bundle forward pass: " << outputs.size() << " outputs ✅" << std::endl;
        
        // Test bundle-aware prediction
        auto predictions = processor.bundle_predict(bundle_batch, test_layer_ids_);
        std::cout << "  Bundle prediction: " << predictions.size() << " predictions ✅" << std::endl;
        
        // Test bundle-aware training step
        double loss = processor.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
        std::cout << "  Bundle training step: loss=" << loss << " ✅" << std::endl;
        
        assert_true(std::isfinite(loss), "Bundle training loss is finite");
        assert_true(outputs.size() == bundle_batch.inputs.size(), "Outputs match input count");
        assert_true(predictions.size() == bundle_batch.inputs.size(), "Predictions match input count");
        
        std::cout << "✅ Bundle-aware batch processing test passed" << std::endl;
    }
    
    void test_bundle_provenance_tracking() {
        std::cout << "\n--- Bundle Provenance Tracking Test ---" << std::endl;
        
        // Generate test data
        auto dataset = BatchUtils::generate_synthetic_dataset(32, 24, 3);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 3);
        
        BatchConfig base_config(8, 24, 3, true, 56789);
        BundleBatchConfig bundle_config(base_config, true, "provenance_test", true, 2, 67890);
        
        BundleBatchProcessor processor(bundle_config);
        
        // Process multiple batches to generate provenance
        for (int batch_idx = 0; batch_idx < 3; ++batch_idx) {
            BundleBatchData bundle_batch = processor.create_bundle_batch(dataset, targets, batch_idx * 8);
            
            // Process batch with provenance tracking
            processor.bundle_forward_pass(bundle_batch, test_layer_ids_[0]);
            processor.bundle_predict(bundle_batch, test_layer_ids_);
            processor.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
            
            std::cout << "  Batch " << (batch_idx + 1) << ": Bundle ID=" << bundle_batch.bundle_id;
            std::cout << ", Samples=" << bundle_batch.inputs.size() << std::endl;
        }
        
        // Check bundle statistics after processing
        auto stats = processor.get_bundle_statistics();
        std::cout << "  Provenance tracking results:" << std::endl;
        std::cout << "    Total batches processed: " << stats.total_batches_processed << std::endl;
        std::cout << "    Total samples processed: " << stats.total_samples_processed << std::endl;
        std::cout << "    Completed bundles: " << stats.completed_bundles_count << std::endl;
        std::cout << "    Current bundle: " << processor.get_current_bundle_id() << std::endl;
        
        assert_true(stats.total_batches_processed > 0, "Batches processed with provenance");
        assert_true(stats.total_samples_processed > 0, "Samples tracked with provenance");
        
        std::cout << "✅ Bundle provenance tracking test passed" << std::endl;
    }
    
    void test_bundle_training_with_provenance() {
        std::cout << "\n--- Bundle Training with Provenance Test ---" << std::endl;
        
        // Generate training data
        auto train_dataset = BatchUtils::generate_synthetic_dataset(100, 48, 5);
        auto train_targets = BatchUtils::generate_one_hot_targets(train_dataset, 5);
        auto val_dataset = BatchUtils::generate_synthetic_dataset(20, 48, 5);
        auto val_targets = BatchUtils::generate_one_hot_targets(val_dataset, 5);
        
        BatchConfig base_config(16, 48, 5, true, 78901);
        BundleBatchConfig bundle_config(base_config, true, "training_bundle", true, 4, 89012);
        
        BundleBatchTrainingOrchestrator orchestrator(bundle_config, test_layer_ids_);
        
        // Train with bundle provenance
        auto results = orchestrator.train_multiple_epochs_with_bundles(
            train_dataset, train_targets,
            val_dataset, val_targets,
            2, // epochs
            0.001 // learning rate
        );
        
        std::cout << "  Bundle training results:" << std::endl;
        std::cout << "    Training epochs: " << results.training_results.total_epochs << std::endl;
        std::cout << "    Total training time: " << results.training_results.total_training_time_ms << "ms" << std::endl;
        std::cout << "    Bundle overhead: " << results.total_bundle_overhead_ms << "ms" << std::endl;
        std::cout << "    Created bundles: " << results.created_bundle_ids.size() << std::endl;
        std::cout << "    Bundle stats batches: " << results.bundle_stats.total_batches_processed << std::endl;
        std::cout << "    Bundle stats samples: " << results.bundle_stats.total_samples_processed << std::endl;
        
        // Validate training results
        assert_true(results.training_results.total_epochs == 2, "Correct number of epochs");
        assert_true(!results.created_bundle_ids.empty(), "Bundles created during training");
        assert_true(results.bundle_stats.total_batches_processed > 0, "Batches processed");
        assert_true(results.bundle_stats.total_samples_processed > 0, "Samples processed");
        
        // Test bundle data export
        auto export_data = orchestrator.export_bundle_training_data();
        std::cout << "  Export data entries: " << export_data.size() << std::endl;
        
        assert_true(export_data.count("bundle_integration_enabled") > 0, "Bundle integration flag exported");
        assert_true(export_data.count("provenance_tracking") > 0, "Provenance tracking flag exported");
        
        std::cout << "✅ Bundle training with provenance test passed" << std::endl;
    }
    
    void test_bundle_statistics() {
        std::cout << "\n--- Bundle Statistics Test ---" << std::endl;
        
        BatchConfig base_config(12, 36, 4, false, 11122);
        BundleBatchConfig bundle_config(base_config, true, "stats_test", true, 3, 33445);
        
        BundleBatchProcessor processor(bundle_config);
        
        // Process some batches to generate statistics
        auto dataset = BatchUtils::generate_synthetic_dataset(60, 36, 4);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 4);
        
        for (int i = 0; i < 5; ++i) {
            BundleBatchData bundle_batch = processor.create_bundle_batch(dataset, targets, i * 12);
            processor.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
        }
        
        // Get detailed statistics
        auto stats = processor.get_bundle_statistics();
        
        std::cout << "  Bundle Statistics:" << std::endl;
        std::cout << "    Total batches processed: " << stats.total_batches_processed << std::endl;
        std::cout << "    Total samples processed: " << stats.total_samples_processed << std::endl;
        std::cout << "    Average processing time: " << stats.average_batch_processing_time << "μs" << std::endl;
        std::cout << "    Total bundle time: " << stats.total_bundle_creation_time << "ms" << std::endl;
        std::cout << "    Completed bundles: " << stats.completed_bundles_count << std::endl;
        std::cout << "    Bundle IDs: ";
        for (size_t i = 0; i < stats.bundle_ids.size(); ++i) {
            std::cout << stats.bundle_ids[i];
            if (i < stats.bundle_ids.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // Validate statistics
        assert_true(stats.total_batches_processed > 0, "Batches processed");
        assert_true(stats.total_samples_processed > 0, "Samples processed");
        assert_true(stats.average_batch_processing_time >= 0, "Processing time non-negative");
        assert_true(stats.total_bundle_creation_time >= 0, "Bundle time non-negative");
        assert_true(!stats.bundle_ids.empty(), "Bundle IDs available");
        
        std::cout << "✅ Bundle statistics test passed" << std::endl;
    }
    
    void test_bundle_frequency_management() {
        std::cout << "\n--- Bundle Frequency Management Test ---" << std::endl;
        
        // Test different bundle frequencies
        std::vector<int64_t> frequencies = {2, 5, 10};
        
        for (int64_t frequency : frequencies) {
            BatchConfig base_config(8, 32, 3, false, 44556);
            BundleBatchConfig bundle_config(base_config, true, "freq_test", true, frequency, 55667);
            
            BundleBatchProcessor processor(bundle_config);
            
            auto dataset = BatchUtils::generate_synthetic_dataset(40, 32, 3);
            auto targets = BatchUtils::generate_one_hot_targets(dataset, 3);
            
            // Process batches to trigger bundle creation
            int batches_to_process = frequency + 2; // Process more than frequency to trigger creation
            
            for (int i = 0; i < batches_to_process; ++i) {
                BundleBatchData bundle_batch = processor.create_bundle_batch(dataset, targets, i * 8);
                processor.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
            }
            
            auto stats = processor.get_bundle_statistics();
            
            std::cout << "  Frequency " << frequency << ": ";
            std::cout << "batches=" << stats.total_batches_processed;
            std::cout << ", bundles=" << stats.completed_bundles_count;
            std::cout << " ✅" << std::endl;
            
            // Validate frequency behavior
            assert_true(stats.total_batches_processed == batches_to_process, "Correct batch count");
            assert_true(stats.completed_bundles_count >= 1, "At least one bundle created");
        }
        
        std::cout << "✅ Bundle frequency management test passed" << std::endl;
    }
    
    void test_bundle_export_and_analysis() {
        std::cout << "\n--- Bundle Export and Analysis Test ---" << std::endl;
        
        BatchConfig base_config(20, 64, 6, true, 66778);
        BundleBatchConfig bundle_config(base_config, true, "export_test", true, 5, 77889);
        
        BundleBatchProcessor processor(bundle_config);
        BundleBatchTrainingOrchestrator orchestrator(bundle_config, test_layer_ids_);
        
        // Generate some activity to export
        auto dataset = BatchUtils::generate_synthetic_dataset(80, 64, 6);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 6);
        
        for (int i = 0; i < 3; ++i) {
            BundleBatchData bundle_batch = processor.create_bundle_batch(dataset, targets, i * 20);
            processor.bundle_forward_pass(bundle_batch, test_layer_ids_[0]);
            processor.bundle_predict(bundle_batch, test_layer_ids_);
            processor.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
        }
        
        // Export processor data
        auto processor_export = processor.export_bundle_data();
        std::cout << "  Processor export data (" << processor_export.size() << " entries):" << std::endl;
        for (const auto& [key, value] : processor_export) {
            std::cout << "    " << key << "=" << value << std::endl;
        }
        
        // Export orchestrator data
        auto orchestrator_export = orchestrator.export_bundle_training_data();
        std::cout << "  Orchestrator export data (" << orchestrator_export.size() << " entries):" << std::endl;
        for (const auto& [key, value] : orchestrator_export) {
            std::cout << "    " << key << "=" << value << std::endl;
        }
        
        // Validate export data
        assert_true(!processor_export.empty(), "Processor export data not empty");
        assert_true(!orchestrator_export.empty(), "Orchestrator export data not empty");
        assert_true(processor_export.count("bundle_enabled") > 0, "Bundle enabled in processor export");
        assert_true(orchestrator_export.count("bundle_integration_enabled") > 0, "Bundle integration in orchestrator export");
        
        std::cout << "✅ Bundle export and analysis test passed" << std::endl;
    }
    
    void test_bundle_performance_impact() {
        std::cout << "\n--- Bundle Performance Impact Test ---" << std::endl;
        
        auto dataset = BatchUtils::generate_synthetic_dataset(64, 32, 4);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 4);
        
        // Test with bundles enabled
        BatchConfig base_config(16, 32, 4, false, 88990);
        BundleBatchConfig bundle_config_enabled(base_config, true, "perf_test", true, 4, 99001);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        BundleBatchProcessor processor_with_bundles(bundle_config_enabled);
        
        for (int i = 0; i < 4; ++i) {
            BundleBatchData bundle_batch = processor_with_bundles.create_bundle_batch(dataset, targets, i * 16);
            processor_with_bundles.bundle_forward_pass(bundle_batch, test_layer_ids_[0]);
            processor_with_bundles.bundle_predict(bundle_batch, test_layer_ids_);
            processor_with_bundles.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_with_bundles = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Test without bundles
        BundleBatchConfig bundle_config_disabled(base_config, false, "no_bundle", false, 0, 99002);
        
        start_time = std::chrono::high_resolution_clock::now();
        
        BundleBatchProcessor processor_without_bundles(bundle_config_disabled);
        
        for (int i = 0; i < 4; ++i) {
            BundleBatchData bundle_batch = processor_without_bundles.create_bundle_batch(dataset, targets, i * 16);
            processor_without_bundles.bundle_forward_pass(bundle_batch, test_layer_ids_[0]);
            processor_without_bundles.bundle_predict(bundle_batch, test_layer_ids_);
            processor_without_bundles.bundle_training_step(bundle_batch, test_layer_ids_, 0.001);
        }
        
        end_time = std::chrono::high_resolution_clock::now();
        auto duration_without_bundles = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "  Performance comparison:" << std::endl;
        std::cout << "    With bundles: " << duration_with_bundles.count() << "ms" << std::endl;
        std::cout << "    Without bundles: " << duration_without_bundles.count() << "ms" << std::endl;
        
        double overhead = static_cast<double>(duration_with_bundles.count() - duration_without_bundles.count());
        double overhead_percent = (overhead / duration_without_bundles.count()) * 100.0;
        
        std::cout << "    Bundle overhead: " << overhead << "ms (" << overhead_percent << "%)" << std::endl;
        
        // Check that overhead is reasonable (should be less than 50%)
        assert_true(overhead_percent < 50.0, "Bundle overhead is reasonable");
        
        std::cout << "✅ Bundle performance impact test passed" << std::endl;
    }
    
    void test_multi_epoch_bundle_training() {
        std::cout << "\n--- Multi-Epoch Bundle Training Test ---" << std::endl;
        
        // Generate larger dataset for multi-epoch training
        auto train_dataset = BatchUtils::generate_synthetic_dataset(200, 56, 6);
        auto train_targets = BatchUtils::generate_one_hot_targets(train_dataset, 6);
        auto val_dataset = BatchUtils::generate_synthetic_dataset(40, 56, 6);
        auto val_targets = BatchUtils::generate_one_hot_targets(val_dataset, 6);
        
        BatchConfig base_config(32, 56, 6, true, 11223);
        BundleBatchConfig bundle_config(base_config, true, "multi_epoch_test", true, 6, 22334);
        
        BundleBatchTrainingOrchestrator orchestrator(bundle_config, test_layer_ids_);
        
        // Multi-epoch training
        auto results = orchestrator.train_multiple_epochs_with_bundles(
            train_dataset, train_targets,
            val_dataset, val_targets,
            3, // epochs
            0.0005 // learning rate
        );
        
        std::cout << "  Multi-epoch bundle training results:" << std::endl;
        std::cout << "    Epochs completed: " << results.training_results.total_epochs << std::endl;
        std::cout << "    Training metrics: " << results.training_results.training_metrics.size() << " epochs" << std::endl;
        std::cout << "    Validation metrics: " << results.training_results.validation_metrics.size() << " epochs" << std::endl;
        std::cout << "    Total training time: " << results.training_results.total_training_time_ms << "ms" << std::endl;
        std::cout << "    Bundle overhead: " << results.total_bundle_overhead_ms << "ms" << std::endl;
        std::cout << "    Created bundles: " << results.created_bundle_ids.size() << std::endl;
        std::cout << "    Bundle statistics:" << std::endl;
        std::cout << "      Total batches: " << results.bundle_stats.total_batches_processed << std::endl;
        std::cout << "      Total samples: " << results.bundle_stats.total_samples_processed << std::endl;
        std::cout << "      Completed bundles: " << results.bundle_stats.completed_bundles_count << std::endl;
        
        // Validate multi-epoch results
        assert_true(results.training_results.total_epochs == 3, "Correct epoch count");
        assert_true(results.training_results.training_metrics.size() == 3, "Training metrics for all epochs");
        assert_true(results.training_results.validation_metrics.size() == 3, "Validation metrics for all epochs");
        assert_true(!results.created_bundle_ids.empty(), "Bundles created across epochs");
        assert_true(results.bundle_stats.total_batches_processed > 0, "Batches processed across epochs");
        
        // Check training progress
        if (results.training_results.training_metrics.size() >= 2) {
            double first_loss = results.training_results.training_metrics[0].avg_loss;
            double last_loss = results.training_results.training_metrics.back().avg_loss;
            std::cout << "      Loss progression: " << first_loss << " -> " << last_loss << std::endl;
        }
        
        std::cout << "✅ Multi-epoch bundle training test passed" << std::endl;
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
    std::cout << "Bundle and Batch Processing Integration Test Runner" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    t81::vm::advanced_ai::test::BundleBatchIntegrationTester tester;
    tester.run_all_tests();
    
    return 0;
}
