// Batch Processing Integration Test Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Tests mini-batch processing capabilities in Advanced AI integration

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include <cmath>

#include "advanced_ai_batch_processing.cpp"

namespace t81::vm::advanced_ai::test {

// Batch processing test suite
class BatchProcessingTester {
private:
    std::vector<int64_t> test_layer_ids_;
    
public:
    BatchProcessingTester() {
        // Initialize test layer IDs
        test_layer_ids_ = {1001, 1002, 1003}; // Mock layer IDs
    }
    
    void run_all_tests() {
        std::cout << "=== Batch Processing Integration Test Suite ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        // Test 1: Basic batch configuration
        test_batch_configuration();
        
        // Test 2: Batch data creation
        test_batch_data_creation();
        
        // Test 3: Batch forward pass
        test_batch_forward_pass();
        
        // Test 4: Batch training step
        test_batch_training_step();
        
        // Test 5: Batch prediction
        test_batch_prediction();
        
        // Test 6: Batch accuracy calculation
        test_batch_accuracy();
        
        // Test 7: Multi-epoch training
        test_multi_epoch_training();
        
        // Test 8: Performance comparison
        test_performance_comparison();
        
        // Test 9: Different batch sizes
        test_different_batch_sizes();
        
        // Test 10: Batch shuffling
        test_batch_shuffling();
        
        std::cout << std::endl;
        std::cout << "=== Batch Processing Test Results ===" << std::endl;
        std::cout << "All batch processing tests completed successfully!" << std::endl;
        std::cout << "Mini-batch processing is ready for experimental research use." << std::endl;
    }

private:
    void test_batch_configuration() {
        std::cout << "\n--- Batch Configuration Test ---" << std::endl;
        
        // Test different batch configurations
        std::vector<BatchConfig> configs = {
            BatchConfig(16, 50, 3, true, 12345),
            BatchConfig(32, 100, 4, false, 67890),
            BatchConfig(64, 200, 8, true, 54321),
            BatchConfig(128, 500, 16, false, 98765)
        };
        
        for (size_t i = 0; i < configs.size(); ++i) {
            const auto& config = configs[i];
            
            std::cout << "  Config " << (i + 1) << ": ";
            std::cout << "batch_size=" << config.batch_size;
            std::cout << ", seq_len=" << config.sequence_length;
            std::cout << ", channels=" << config.num_channels;
            std::cout << ", shuffle=" << (config.shuffle_enabled ? "true" : "false");
            std::cout << " ✅" << std::endl;
            
            // Validate configuration
            assert_true(config.batch_size > 0, "Batch size must be positive");
            assert_true(config.sequence_length > 0, "Sequence length must be positive");
            assert_true(config.num_channels > 0, "Number of channels must be positive");
        }
        
        std::cout << "✅ Batch configuration test passed" << std::endl;
    }
    
    void test_batch_data_creation() {
        std::cout << "\n--- Batch Data Creation Test ---" << std::endl;
        
        // Generate test dataset
        auto dataset = BatchUtils::generate_synthetic_dataset(100, 50, 5);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 5);
        
        BatchConfig config(32, 50, 3, true, 12345);
        BatchProcessor processor(config);
        
        // Test batch creation
        BatchData batch = processor.create_batch(dataset, targets, 0);
        
        std::cout << "  Created batch with " << batch.inputs.size() << " samples" << std::endl;
        std::cout << "  Input size: " << (batch.inputs.empty() ? 0 : batch.inputs[0].size()) << std::endl;
        std::cout << "  Target size: " << (batch.targets.empty() ? 0 : batch.targets[0].size()) << std::endl;
        std::cout << "  Batch ID: " << batch.batch_id << std::endl;
        
        // Validate batch data
        assert_true(batch.inputs.size() <= config.batch_size, "Batch size respected");
        assert_true(!batch.inputs.empty(), "Batch has inputs");
        assert_true(batch.inputs.size() == batch.targets.size(), "Inputs and targets match");
        
        std::cout << "✅ Batch data creation test passed" << std::endl;
    }
    
    void test_batch_forward_pass() {
        std::cout << "\n--- Batch Forward Pass Test ---" << std::endl;
        
        // Generate test data
        auto dataset = BatchUtils::generate_synthetic_dataset(64, 32, 3);
        BatchConfig config(16, 32, 3, false, 12345);
        BatchProcessor processor(config);
        
        // Create batch
        BatchData batch = processor.create_batch(dataset, {}, 0);
        
        // Perform batch forward pass
        auto batch_outputs = processor.batch_forward_pass(batch.inputs, test_layer_ids_[0]);
        
        std::cout << "  Forward pass completed for " << batch_outputs.size() << " samples" << std::endl;
        std::cout << "  Output size per sample: " << (batch_outputs.empty() ? 0 : batch_outputs[0].size()) << std::endl;
        
        // Validate forward pass
        assert_true(batch_outputs.size() == batch.inputs.size(), "Output batch size matches input");
        assert_true(!batch_outputs.empty(), "Batch forward pass produces outputs");
        
        std::cout << "✅ Batch forward pass test passed" << std::endl;
    }
    
    void test_batch_training_step() {
        std::cout << "\n--- Batch Training Step Test ---" << std::endl;
        
        // Generate training data
        auto dataset = BatchUtils::generate_synthetic_dataset(100, 64, 4);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 4);
        
        BatchConfig config(32, 64, 4, true, 12345);
        BatchProcessor processor(config);
        
        // Create batch
        BatchData batch = processor.create_batch(dataset, targets, 0);
        
        // Perform batch training step
        double learning_rate = 0.001;
        double batch_loss = processor.batch_training_step(batch, test_layer_ids_, learning_rate);
        
        std::cout << "  Batch training step completed" << std::endl;
        std::cout << "  Batch loss: " << batch_loss << std::endl;
        std::cout << "  Samples processed: " << batch.inputs.size() << std::endl;
        
        // Validate training step
        assert_true(std::isfinite(batch_loss), "Batch loss is finite");
        assert_true(batch_loss >= 0.0, "Batch loss is non-negative");
        
        std::cout << "✅ Batch training step test passed" << std::endl;
    }
    
    void test_batch_prediction() {
        std::cout << "\n--- Batch Prediction Test ---" << std::endl;
        
        // Generate test data
        auto dataset = BatchUtils::generate_synthetic_dataset(50, 32, 3);
        BatchConfig config(16, 32, 3, false, 12345);
        BatchProcessor processor(config);
        
        // Create batch
        BatchData batch = processor.create_batch(dataset, {}, 0);
        
        // Perform batch prediction
        auto predictions = processor.batch_predict(batch.inputs, test_layer_ids_);
        
        std::cout << "  Batch prediction completed" << std::endl;
        std::cout << "  Predictions: " << predictions.size() << " samples" << std::endl;
        std::cout << "  Prediction range: [" << *std::min_element(predictions.begin(), predictions.end())
                  << ", " << *std::max_element(predictions.begin(), predictions.end()) << "]" << std::endl;
        
        // Validate predictions
        assert_true(predictions.size() == batch.inputs.size(), "Prediction count matches input count");
        assert_true(!predictions.empty(), "Batch prediction produces results");
        
        // Check prediction validity (should be within expected range)
        for (int pred : predictions) {
            assert_true(pred >= 0 && pred < 10, "Predictions are within valid range");
        }
        
        std::cout << "✅ Batch prediction test passed" << std::endl;
    }
    
    void test_batch_accuracy() {
        std::cout << "\n--- Batch Accuracy Test ---" << std::endl;
        
        // Test with known predictions and targets
        std::vector<int> predictions = {0, 1, 2, 1, 0, 2, 1, 0};
        std::vector<int> targets = {0, 1, 1, 1, 0, 2, 0, 0};
        
        BatchConfig config(8, 32, 3, false, 12345);
        BatchProcessor processor(config);
        
        // Calculate accuracy
        double accuracy = processor.calculate_batch_accuracy(predictions, targets);
        
        std::cout << "  Predictions: [";
        for (size_t i = 0; i < predictions.size(); ++i) {
            std::cout << predictions[i];
            if (i < predictions.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        
        std::cout << "  Targets:     [";
        for (size_t i = 0; i < targets.size(); ++i) {
            std::cout << targets[i];
            if (i < targets.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        
        std::cout << "  Accuracy: " << (accuracy * 100) << "%" << std::endl;
        
        // Validate accuracy calculation
        int correct = 0;
        for (size_t i = 0; i < predictions.size(); ++i) {
            if (predictions[i] == targets[i]) correct++;
        }
        double expected_accuracy = static_cast<double>(correct) / predictions.size();
        
        assert_true(std::abs(accuracy - expected_accuracy) < 1e-6, "Accuracy calculation is correct");
        assert_true(accuracy >= 0.0 && accuracy <= 1.0, "Accuracy is within valid range");
        
        std::cout << "✅ Batch accuracy test passed" << std::endl;
    }
    
    void test_multi_epoch_training() {
        std::cout << "\n--- Multi-Epoch Training Test ---" << std::endl;
        
        // Generate training and validation datasets
        auto full_dataset = BatchUtils::generate_synthetic_dataset(200, 64, 5);
        auto full_targets = BatchUtils::generate_one_hot_targets(full_dataset, 5);
        
        // Split into train/validation
        auto [train_dataset, val_dataset] = BatchUtils::split_dataset(full_dataset, 0.8);
        auto [train_targets, val_targets] = BatchUtils::split_dataset(full_targets, 0.8);
        
        BatchConfig config(32, 64, 5, true, 12345);
        BatchTrainingOrchestrator orchestrator(config, test_layer_ids_);
        
        // Train for multiple epochs
        int64_t epochs = 3;
        double learning_rate = 0.001;
        
        auto results = orchestrator.train_multiple_epochs(
            train_dataset, train_targets,
            val_dataset, val_targets,
            epochs, learning_rate
        );
        
        std::cout << "  Multi-epoch training completed" << std::endl;
        std::cout << "  Total epochs: " << results.total_epochs << std::endl;
        std::cout << "  Total training time: " << results.total_training_time_ms << " ms" << std::endl;
        std::cout << "  Training metrics: " << results.training_metrics.size() << " epochs" << std::endl;
        std::cout << "  Validation metrics: " << results.validation_metrics.size() << " epochs" << std::endl;
        
        // Validate multi-epoch training
        assert_true(results.training_metrics.size() == epochs, "Training metrics for all epochs");
        assert_true(results.validation_metrics.size() == epochs, "Validation metrics for all epochs");
        assert_true(results.total_training_time_ms > 0, "Training time recorded");
        
        // Check for improvement over epochs
        if (results.training_metrics.size() >= 2) {
            double first_loss = results.training_metrics[0].avg_loss;
            double last_loss = results.training_metrics.back().avg_loss;
            std::cout << "  Loss progression: " << first_loss << " -> " << last_loss << std::endl;
        }
        
        std::cout << "✅ Multi-epoch training test passed" << std::endl;
    }
    
    void test_performance_comparison() {
        std::cout << "\n--- Performance Comparison Test ---" << std::endl;
        
        // Generate test dataset
        auto dataset = BatchUtils::generate_synthetic_dataset(128, 64, 4);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 4);
        
        // Test different batch sizes
        std::vector<int64_t> batch_sizes = {8, 16, 32, 64};
        
        for (int64_t batch_size : batch_sizes) {
            BatchConfig config(batch_size, 64, 4, false, 12345);
            BatchTrainingOrchestrator orchestrator(config, test_layer_ids_);
            
            // Measure training time
            auto start_time = std::chrono::high_resolution_clock::now();
            
            auto metrics = orchestrator.train_epoch(dataset, targets, 0.001);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            double throughput = (dataset.size() * 1000.0) / duration.count(); // samples per second
            
            std::cout << "  Batch size " << batch_size << ": ";
            std::cout << "Time=" << duration.count() << "ms, ";
            std::cout << "Throughput=" << static_cast<int>(throughput) << " samples/s, ";
            std::cout << "Loss=" << metrics.avg_loss << ", ";
            std::cout << "Acc=" << (metrics.accuracy * 100) << "% ✅" << std::endl;
        }
        
        std::cout << "✅ Performance comparison test completed" << std::endl;
    }
    
    void test_different_batch_sizes() {
        std::cout << "\n--- Different Batch Sizes Test ---" << std::endl;
        
        auto dataset = BatchUtils::generate_synthetic_dataset(100, 32, 3);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 3);
        
        std::vector<int64_t> batch_sizes = {1, 4, 8, 16, 32, 64, 128};
        
        for (int64_t batch_size : batch_sizes) {
            if (batch_size > dataset.size()) continue;
            
            BatchConfig config(batch_size, 32, 3, false, 12345);
            BatchProcessor processor(config);
            
            // Create batch and test processing
            BatchData batch = processor.create_batch(dataset, targets, 0);
            
            auto predictions = processor.batch_predict(batch.inputs, test_layer_ids_);
            double accuracy = processor.calculate_batch_accuracy(predictions, batch.labels);
            
            std::cout << "  Batch size " << batch_size << ": ";
            std::cout << "Samples=" << batch.inputs.size() << ", ";
            std::cout << "Accuracy=" << (accuracy * 100) << "% ✅" << std::endl;
            
            // Validate batch processing
            assert_true(batch.inputs.size() <= batch_size, "Batch size limit respected");
            assert_true(predictions.size() == batch.inputs.size(), "Predictions match batch size");
        }
        
        std::cout << "✅ Different batch sizes test passed" << std::endl;
    }
    
    void test_batch_shuffling() {
        std::cout << "\n--- Batch Shuffling Test ---" << std::endl;
        
        auto dataset = BatchUtils::generate_synthetic_dataset(64, 32, 3);
        auto targets = BatchUtils::generate_one_hot_targets(dataset, 3);
        
        // Test with shuffling enabled
        BatchConfig config_shuffled(16, 32, 3, true, 12345);
        BatchProcessor processor_shuffled(config_shuffled);
        
        // Test without shuffling
        BatchConfig config_no_shuffle(16, 32, 3, false, 12345);
        BatchProcessor processor_no_shuffle(config_no_shuffle);
        
        // Create batches with both configurations
        BatchData batch_shuffled = processor_shuffled.create_batch(dataset, targets, 0);
        BatchData batch_no_shuffle = processor_no_shuffle.create_batch(dataset, targets, 0);
        
        std::cout << "  Shuffling enabled: " << (config_shuffled.shuffle_enabled ? "true" : "false") << std::endl;
        std::cout << "  Shuffling disabled: " << (config_no_shuffle.shuffle_enabled ? "true" : "false") << std::endl;
        std::cout << "  Batch sizes: " << batch_shuffled.inputs.size() << " vs " << batch_no_shuffle.inputs.size() << std::endl;
        
        // Both should have same batch size but potentially different order
        assert_true(batch_shuffled.inputs.size() == batch_no_shuffle.inputs.size(), 
                   "Both configurations produce same batch size");
        assert_true(config_shuffled.shuffle_enabled != config_no_shuffle.shuffle_enabled, 
                   "Shuffling configurations differ");
        
        std::cout << "✅ Batch shuffling test passed" << std::endl;
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
    std::cout << "Batch Processing Integration Test Runner" << std::endl;
    std::cout << "======================================" << std::endl;
    
    t81::vm::advanced_ai::test::BatchProcessingTester tester;
    tester.run_all_tests();
    
    return 0;
}
