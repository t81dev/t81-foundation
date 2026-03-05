#pragma once

#include <vector>
#include <memory>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <unordered_map>
#include "t81/experimental/llama_cpp_adapter.hpp"
#include "t81/codec/enhanced_gguf_parser.hpp"
#include "t81/codec/advanced_ternary_quantization.hpp"
#include "t81/memory/advanced_memory_manager.hpp"

namespace t81::inference {

// Batch processing strategies
enum class BatchStrategy {
    DYNAMIC,        // Dynamically group requests by size and timing
    FIXED_SIZE,     // Fixed batch size with timeout
    PRIORITY_BASED, // Prioritize certain requests
    ADAPTIVE,       // Adapt based on system load
    OPTIMIZED       // ML-based optimization
};

// Inference request types
enum class InferenceType {
    TEXT_GENERATION,
    EMBEDDING,
    CLASSIFICATION,
    SEQUENCE_TO_SEQUENCE,
    CUSTOM
};

// Priority levels for inference requests
enum class InferencePriority {
    REALTIME,       // Lowest latency (e.g., interactive chat)
    HIGH,           // High priority (e.g., user requests)
    NORMAL,         // Normal priority (e.g., batch processing)
    LOW,            // Low priority (e.g., background tasks)
    BACKGROUND      // Background processing only
};

// Enhanced inference request
struct EnhancedInferenceRequest {
    std::string request_id;
    InferenceType type;
    InferencePriority priority;
    std::string prompt;
    int max_tokens = 150;
    float temperature = 0.7f;
    int top_k = 40;
    float top_p = 0.9f;
    int reasoning_level = 3;
    bool enable_ternary_quantization = true;
    bool enable_batching = true;
    std::string expected_model_hash;
    
    // Timing constraints
    std::chrono::milliseconds timeout{30000};
    std::chrono::steady_clock::time_point submit_time;
    std::chrono::steady_clock::time_point deadline;
    
    // Callback for async completion
    std::function<void(const std::string&, bool)> completion_callback;
    
    // Additional metadata
    std::unordered_map<std::string, std::string> metadata;
    
    EnhancedInferenceRequest() : submit_time(std::chrono::steady_clock::now()) {
        deadline = submit_time + timeout;
    }
};

// Enhanced inference result
struct EnhancedInferenceResult {
    std::string request_id;
    bool success = false;
    std::string text;
    std::vector<int> token_ids;
    std::string error_message;
    
    // Performance metrics
    std::chrono::milliseconds total_time{0};
    std::chrono::milliseconds queue_time{0};
    std::chrono::milliseconds processing_time{0};
    std::chrono::milliseconds quantization_time{0};
    std::chrono::milliseconds inference_time{0};
    
    // Resource usage
    size_t memory_used = 0;
    size_t tensors_processed = 0;
    float cache_hit_rate = 0.0f;
    
    // Quality metrics
    float perplexity = 0.0f;
    float coherence_score = 0.0f;
    
    std::chrono::steady_clock::time_point completion_time;
};

// Batch inference request
struct BatchInferenceRequest {
    std::vector<EnhancedInferenceRequest> requests;
    std::chrono::steady_clock::time_point batch_start_time;
    std::chrono::milliseconds batch_timeout{100};
    size_t max_batch_size = 8;
    
    BatchInferenceRequest() : batch_start_time(std::chrono::steady_clock::now()) {}
};

// Batch inference result
struct BatchInferenceResult {
    std::vector<EnhancedInferenceResult> results;
    std::chrono::milliseconds total_batch_time{0};
    std::chrono::milliseconds preprocessing_time{0};
    std::chrono::milliseconds inference_time{0};
    std::chrono::milliseconds postprocessing_time{0};
    
    // Batch efficiency metrics
    float throughput_tokens_per_second = 0.0f;
    float batch_utilization_ratio = 0.0f;
    size_t total_tokens_generated = 0;
};

// Request queue manager
class InferenceQueue {
public:
    explicit InferenceQueue(size_t max_size = 1000);
    ~InferenceQueue() = default;
    
    // Queue operations
    bool enqueue(const EnhancedInferenceRequest& request);
    std::optional<EnhancedInferenceRequest> dequeue();
    std::vector<EnhancedInferenceRequest> dequeue_batch(size_t max_size);
    
    // Priority queue operations
    bool enqueue_priority(const EnhancedInferenceRequest& request);
    std::optional<EnhancedInferenceRequest> dequeue_priority();
    
    // Queue management
    void clear();
    size_t size() const;
    bool empty() const;
    bool full() const;
    
    // Statistics
    struct QueueStats {
        size_t total_requests = 0;
        size_t current_size = 0;
        size_t max_size_reached = 0;
        std::chrono::milliseconds average_wait_time{0};
        std::chrono::milliseconds max_wait_time{0};
        size_t timeout_count = 0;
        std::unordered_map<InferencePriority, size_t> priority_counts;
    };
    
    QueueStats get_stats() const;
    void reset_stats();
    
    // Timeout handling
    std::vector<EnhancedInferenceRequest> remove_expired_requests();
    
private:
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Multiple queues for different priorities
    std::queue<EnhancedInferenceRequest> realtime_queue_;
    std::queue<EnhancedInferenceRequest> high_queue_;
    std::queue<EnhancedInferenceRequest> normal_queue_;
    std::queue<EnhancedInferenceRequest> low_queue_;
    std::queue<EnhancedInferenceRequest> background_queue_;
    
    size_t max_size_;
    mutable QueueStats stats_;
    
    std::queue<EnhancedInferenceRequest>& get_queue_by_priority(InferencePriority priority);
    void update_queue_stats(const EnhancedInferenceRequest& request, bool enqueued);
};

// Batch processor
class BatchProcessor {
public:
    explicit BatchProcessor(std::shared_ptr<t81::experimental::LlamaCppAdapter> adapter,
                           std::shared_ptr<t81::memory::TensorMemoryManager> memory_manager,
                           BatchStrategy strategy = BatchStrategy::ADAPTIVE);
    ~BatchProcessor();
    
    // Batch processing
    std::future<BatchInferenceResult> process_batch_async(const BatchInferenceRequest& batch);
    BatchInferenceResult process_batch(const BatchInferenceRequest& batch);
    
    // Configuration
    void set_batch_strategy(BatchStrategy strategy) { strategy_ = strategy; }
    void set_max_batch_size(size_t size) { max_batch_size_ = size; }
    void set_batch_timeout(std::chrono::milliseconds timeout) { batch_timeout_ = timeout; }
    
    // Optimization
    void optimize_batch_size();
    void optimize_for_workload(const std::vector<EnhancedInferenceRequest>& sample_requests);
    
    // Statistics
    struct BatchStats {
        size_t total_batches_processed = 0;
        size_t total_requests_processed = 0;
        std::chrono::milliseconds average_batch_time{0};
        std::chrono::milliseconds average_request_time{0};
        float average_batch_utilization = 0.0f;
        float throughput_tokens_per_second = 0.0f;
        size_t optimal_batch_size = 1;
    };
    
    BatchStats get_stats() const;
    void reset_stats();
    
private:
    std::shared_ptr<t81::experimental::LlamaCppAdapter> adapter_;
    std::shared_ptr<t81::memory::TensorMemoryManager> memory_manager_;
    BatchStrategy strategy_;
    
    size_t max_batch_size_ = 8;
    std::chrono::milliseconds batch_timeout_{100};
    
    mutable std::mutex stats_mutex_;
    BatchStats stats_;
    
    // Batch optimization
    std::vector<EnhancedInferenceRequest> create_optimal_batch(
        const std::vector<EnhancedInferenceRequest>& requests);
    std::vector<EnhancedInferenceRequest> group_by_similarity(
        const std::vector<EnhancedInferenceRequest>& requests);
    
    // Processing strategies
    BatchInferenceResult process_dynamic_batch(const BatchInferenceRequest& batch);
    BatchInferenceResult process_fixed_size_batch(const BatchInferenceRequest& batch);
    BatchInferenceResult process_priority_batch(const BatchInferenceRequest& batch);
    BatchInferenceResult process_adaptive_batch(const BatchInferenceRequest& batch);
    
    // Preprocessing and postprocessing
    std::vector<LlamaCppInferenceRequest> preprocess_batch(const BatchInferenceRequest& batch);
    std::vector<EnhancedInferenceResult> postprocess_batch(
        const std::vector<LlamaCppInferenceReceipt>& receipts,
        const BatchInferenceRequest& original_batch);
    
    void update_batch_stats(const BatchInferenceResult& result);
};

// Async inference engine
class AsyncInferenceEngine {
public:
    AsyncInferenceEngine(std::shared_ptr<t81::experimental::LlamaCppAdapter> adapter,
                         std::shared_ptr<t81::memory::TensorMemoryManager> memory_manager);
    ~AsyncInferenceEngine();
    
    // Async inference
    std::future<EnhancedInferenceResult> infer_async(const EnhancedInferenceRequest& request);
    std::future<std::vector<EnhancedInferenceResult>> infer_batch_async(
        const std::vector<EnhancedInferenceRequest>& requests);
    
    // Synchronous inference
    EnhancedInferenceResult infer(const EnhancedInferenceRequest& request);
    std::vector<EnhancedInferenceResult> infer_batch(
        const std::vector<EnhancedInferenceRequest>& requests);
    
    // Engine control
    void start(size_t worker_count = 4);
    void stop();
    void wait_for_completion();
    
    // Configuration
    void set_queue_size(size_t size) { queue_->max_size_ = size; }
    void set_batch_processor(std::unique_ptr<BatchProcessor> processor);
    
    // Statistics
    struct EngineStats {
        InferenceQueue::QueueStats queue_stats;
        BatchProcessor::BatchStats batch_stats;
        size_t active_workers = 0;
        std::chrono::milliseconds average_latency{0};
        float requests_per_second = 0.0f;
        size_t total_requests_processed = 0;
    };
    
    EngineStats get_stats() const;
    void reset_stats();
    
private:
    std::shared_ptr<t81::experimental::LlamaCppAdapter> adapter_;
    std::shared_ptr<t81::memory::TensorMemoryManager> memory_manager_;
    std::unique_ptr<InferenceQueue> queue_;
    std::unique_ptr<BatchProcessor> batch_processor_;
    
    std::vector<std::thread> workers_;
    std::atomic<bool> should_stop_{false};
    
    mutable std::mutex stats_mutex_;
    EngineStats stats_;
    
    void worker_loop();
    void process_single_request(const EnhancedInferenceRequest& request);
    void process_batch_requests();
    
    void update_engine_stats(const EnhancedInferenceResult& result);
    void update_engine_stats(const BatchInferenceResult& batch_result);
};

// Inference optimizer
class InferenceOptimizer {
public:
    explicit InferenceOptimizer(std::shared_ptr<AsyncInferenceEngine> engine);
    ~InferenceOptimizer() = default;
    
    // Optimization strategies
    void optimize_for_latency();
    void optimize_for_throughput();
    void optimize_for_memory();
    void optimize_balanced();
    
    // Auto-tuning
    void enable_auto_tuning();
    void disable_auto_tuning();
    void tune_periodically(std::chrono::seconds interval);
    
    // Performance monitoring
    struct OptimizationMetrics {
        float current_latency = 0.0f;
        float current_throughput = 0.0f;
        float current_memory_usage = 0.0f;
        float target_latency = 100.0f;  // ms
        float target_throughput = 100.0f; // requests/sec
        float target_memory_usage = 0.8f; // 80%
        
        std::vector<std::pair<std::chrono::steady_clock::time_point, float>> latency_history;
        std::vector<std::pair<std::chrono::steady_clock::time_point, float>> throughput_history;
        std::vector<std::pair<std::chrono::steady_clock::time_point, float>> memory_history;
    };
    
    OptimizationMetrics get_metrics() const;
    void reset_metrics();
    
    // Recommendations
    struct OptimizationRecommendation {
        std::string description;
        float expected_improvement = 0.0f;
        std::chrono::milliseconds implementation_time{0};
        int priority = 0; // 0 = low, 10 = high
    };
    
    std::vector<OptimizationRecommendation> get_recommendations() const;
    void apply_recommendation(const OptimizationRecommendation& recommendation);
    
private:
    std::shared_ptr<AsyncInferenceEngine> engine_;
    std::atomic<bool> auto_tuning_enabled_{false};
    std::thread tuning_thread_;
    std::atomic<bool> should_stop_tuning_{false};
    
    mutable std::mutex metrics_mutex_;
    OptimizationMetrics metrics_;
    
    void tuning_loop();
    void collect_metrics();
    void analyze_performance();
    void apply_optimizations();
    
    // Specific optimization functions
    void optimize_batch_size();
    void optimize_queue_sizes();
    void optimize_memory_allocation();
    void optimize_worker_count();
    
    // Analysis helpers
    float calculate_latency_trend() const;
    float calculate_throughput_trend() const;
    float calculate_memory_trend() const;
    bool is_performance_degrading() const;
};

// Utility functions
namespace inference_utils {
    // Request validation
    bool validate_request(const EnhancedInferenceRequest& request);
    bool validate_batch(const BatchInferenceRequest& batch);
    
    // Performance measurement
    class ScopedInferenceTimer {
    public:
        explicit ScopedInferenceTimer(std::function<void(std::chrono::nanoseconds)> callback);
        ~ScopedInferenceTimer();
        
    private:
        std::chrono::steady_clock::time_point start_;
        std::function<void(std::chrono::nanoseconds)> callback_;
    };
    
    // Request clustering
    std::vector<std::vector<EnhancedInferenceRequest>> cluster_requests(
        const std::vector<EnhancedInferenceRequest>& requests);
    
    // Similarity calculation
    float calculate_request_similarity(const EnhancedInferenceRequest& a, 
                                     const EnhancedInferenceRequest& b);
    
    // Load balancing
    std::vector<size_t> calculate_load_distribution(size_t total_requests, 
                                                   size_t num_workers);
    
    // Resource estimation
    struct ResourceEstimate {
        size_t estimated_memory_mb = 0;
        std::chrono::milliseconds estimated_time{0};
        float cpu_utilization = 0.0f;
        bool can_process = true;
    };
    
    ResourceEstimate estimate_resources(const EnhancedInferenceRequest& request);
    ResourceEstimate estimate_batch_resources(const BatchInferenceRequest& batch);
    
    // Quality metrics
    float calculate_perplexity(const std::string& text);
    float calculate_coherence(const std::string& text);
    float calculate_relevance(const std::string& text, const std::string& prompt);
}

} // namespace t81::inference
