#include "t81/inference/batch_inference_engine.hpp"
#include <algorithm>
#include <random>
#include <cmath>
#include <numeric>
#include <sstream>

namespace t81::inference {

// InferenceQueue implementation
InferenceQueue::InferenceQueue(size_t max_size) : max_size_(max_size) {}

bool InferenceQueue::enqueue(const EnhancedInferenceRequest& request) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (size() >= max_size_) {
        stats_.max_size_reached++;
        return false;
    }
    
    get_queue_by_priority(request.priority).push(request);
    update_queue_stats(request, true);
    queue_cv_.notify_one();
    
    return true;
}

std::optional<EnhancedInferenceRequest> InferenceQueue::dequeue() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    // Check queues in priority order
    std::queue<EnhancedInferenceRequest>* queues[] = {
        &realtime_queue_, &high_queue_, &normal_queue_, &low_queue_, &background_queue_
    };
    
    for (auto* queue : queues) {
        if (!queue->empty()) {
            auto request = queue->front();
            queue->pop();
            update_queue_stats(request, false);
            return request;
        }
    }
    
    return std::nullopt;
}

std::vector<EnhancedInferenceRequest> InferenceQueue::dequeue_batch(size_t max_size) {
    std::vector<EnhancedInferenceRequest> batch;
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    // Collect requests from priority queues
    std::queue<EnhancedInferenceRequest>* queues[] = {
        &realtime_queue_, &high_queue_, &normal_queue_, &low_queue_, &background_queue_
    };
    
    for (auto* queue : queues) {
        while (!queue->empty() && batch.size() < max_size) {
            batch.push_back(queue->front());
            queue->pop();
            update_queue_stats(batch.back(), false);
        }
    }
    
    return batch;
}

bool InferenceQueue::enqueue_priority(const EnhancedInferenceRequest& request) {
    return enqueue(request); // Same logic for now
}

std::optional<EnhancedInferenceRequest> InferenceQueue::dequeue_priority() {
    return dequeue(); // Same logic for now
}

void InferenceQueue::clear() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    while (!realtime_queue_.empty()) realtime_queue_.pop();
    while (!high_queue_.empty()) high_queue_.pop();
    while (!normal_queue_.empty()) normal_queue_.pop();
    while (!low_queue_.empty()) low_queue_.pop();
    while (!background_queue_.empty()) background_queue_.pop();
    
    stats_.current_size = 0;
}

size_t InferenceQueue::size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return realtime_queue_.size() + high_queue_.size() + normal_queue_.size() + 
           low_queue_.size() + background_queue_.size();
}

bool InferenceQueue::empty() const {
    return size() == 0;
}

bool InferenceQueue::full() const {
    return size() >= max_size_;
}

InferenceQueue::QueueStats InferenceQueue::get_stats() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    QueueStats stats = stats_;
    stats.current_size = size();
    
    // Count by priority
    stats.priority_counts[InferencePriority::REALTIME] = realtime_queue_.size();
    stats.priority_counts[InferencePriority::HIGH] = high_queue_.size();
    stats.priority_counts[InferencePriority::NORMAL] = normal_queue_.size();
    stats.priority_counts[InferencePriority::LOW] = low_queue_.size();
    stats.priority_counts[InferencePriority::BACKGROUND] = background_queue_.size();
    
    return stats;
}

void InferenceQueue::reset_stats() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    stats_ = QueueStats{};
}

std::vector<EnhancedInferenceRequest> InferenceQueue::remove_expired_requests() {
    std::vector<EnhancedInferenceRequest> expired;
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    std::queue<EnhancedInferenceRequest>* queues[] = {
        &realtime_queue_, &high_queue_, &normal_queue_, &low_queue_, &background_queue_
    };
    
    for (auto* queue : queues) {
        std::queue<EnhancedInferenceRequest> temp_queue;
        
        while (!queue->empty()) {
            auto request = queue->front();
            queue->pop();
            
            if (now > request.deadline) {
                expired.push_back(request);
                stats_.timeout_count++;
            } else {
                temp_queue.push(request);
            }
        }
        
        *queue = std::move(temp_queue);
    }
    
    return expired;
}

std::queue<EnhancedInferenceRequest>& InferenceQueue::get_queue_by_priority(InferencePriority priority) {
    switch (priority) {
        case InferencePriority::REALTIME: return realtime_queue_;
        case InferencePriority::HIGH: return high_queue_;
        case InferencePriority::NORMAL: return normal_queue_;
        case InferencePriority::LOW: return low_queue_;
        case InferencePriority::BACKGROUND: return background_queue_;
        default: return normal_queue_;
    }
}

void InferenceQueue::update_queue_stats(const EnhancedInferenceRequest& request, bool enqueued) {
    if (enqueued) {
        stats_.total_requests++;
        stats_.current_size++;
    } else {
        if (stats_.current_size > 0) {
            stats_.current_size--;
        }
        
        // Update wait time statistics
        auto now = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - request.submit_time);
        
        if (stats_.total_requests == 1) {
            stats_.average_wait_time = wait_time;
            stats_.max_wait_time = wait_time;
        } else {
            // Update average
            auto total_wait = stats_.average_wait_time * (stats_.total_requests - 1);
            stats_.average_wait_time = (total_wait + wait_time) / stats_.total_requests;
            
            // Update max
            if (wait_time > stats_.max_wait_time) {
                stats_.max_wait_time = wait_time;
            }
        }
    }
}

// BatchProcessor implementation
BatchProcessor::BatchProcessor(std::shared_ptr<t81::experimental::LlamaCppAdapter> adapter,
                               std::shared_ptr<t81::memory::TensorMemoryManager> memory_manager,
                               BatchStrategy strategy)
    : adapter_(adapter), memory_manager_(memory_manager), strategy_(strategy) {}

BatchProcessor::~BatchProcessor() = default;

std::future<BatchInferenceResult> BatchProcessor::process_batch_async(const BatchInferenceRequest& batch) {
    return std::async(std::launch::async, [this, batch]() {
        return process_batch(batch);
    });
}

BatchInferenceResult BatchProcessor::process_batch(const BatchInferenceRequest& batch) {
    auto start_time = std::chrono::high_resolution_clock::now();
    BatchInferenceResult result;
    
    // Preprocessing
    auto preprocess_start = std::chrono::high_resolution_clock::now();
    auto llama_requests = preprocess_batch(batch);
    auto preprocess_end = std::chrono::high_resolution_clock::now();
    result.preprocessing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        preprocess_end - preprocess_start);
    
    // Inference
    auto inference_start = std::chrono::high_resolution_clock::now();
    std::vector<LlamaCppInferenceReceipt> receipts;
    
    for (const auto& llama_req : llama_requests) {
        auto receipt = adapter_->infer(llama_req);
        if (receipt.has_value()) {
            receipts.push_back(receipt.value());
        } else {
            // Handle error
            EnhancedInferenceResult error_result;
            error_result.success = false;
            error_result.error_message = receipt.error();
            result.results.push_back(error_result);
        }
    }
    
    auto inference_end = std::chrono::high_resolution_clock::now();
    result.inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        inference_end - inference_start);
    
    // Postprocessing
    auto postprocess_start = std::chrono::high_resolution_clock::now();
    result.results = postprocess_batch(receipts, batch);
    auto postprocess_end = std::chrono::high_resolution_clock::now();
    result.postprocessing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        postprocess_end - postprocess_start);
    
    // Calculate total metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_batch_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // Calculate throughput
    result.total_tokens_generated = 0;
    for (const auto& res : result.results) {
        result.total_tokens_generated += res.token_ids.size();
    }
    
    if (result.total_batch_time.count() > 0) {
        result.throughput_tokens_per_second = 
            static_cast<float>(result.total_tokens_generated) / 
            (result.total_batch_time.count() / 1000.0f);
    }
    
    result.batch_utilization_ratio = static_cast<float>(batch.requests.size()) / batch.max_batch_size;
    
    update_batch_stats(result);
    return result;
}

void BatchProcessor::optimize_batch_size() {
    // Simple optimization based on recent performance
    if (stats_.average_batch_time.count() > 0) {
        float current_throughput = stats_.throughput_tokens_per_second;
        
        // Try different batch sizes and measure performance
        std::vector<size_t> test_sizes = {1, 2, 4, 8, 16};
        size_t best_size = max_batch_size_;
        float best_throughput = current_throughput;
        
        for (size_t test_size : test_sizes) {
            if (test_size > max_batch_size_) continue;
            
            // In a real implementation, would run actual tests
            // For now, use a simple heuristic
            float estimated_throughput = current_throughput * (1.0f + 0.1f * (test_size - 1));
            
            if (estimated_throughput > best_throughput) {
                best_throughput = estimated_throughput;
                best_size = test_size;
            }
        }
        
        max_batch_size_ = best_size;
        stats_.optimal_batch_size = best_size;
    }
}

void BatchProcessor::optimize_for_workload(const std::vector<EnhancedInferenceRequest>& sample_requests) {
    // Analyze request patterns and optimize accordingly
    std::vector<float> prompt_lengths;
    std::vector<float> max_tokens;
    
    for (const auto& req : sample_requests) {
        prompt_lengths.push_back(static_cast<float>(req.prompt.length()));
        max_tokens.push_back(static_cast<float>(req.max_tokens));
    }
    
    // Calculate averages
    float avg_prompt_length = std::accumulate(prompt_lengths.begin(), prompt_lengths.end(), 0.0f) / 
                              prompt_lengths.size();
    float avg_max_tokens = std::accumulate(max_tokens.begin(), max_tokens.end(), 0.0f) / 
                           max_tokens.size();
    
    // Adjust batch size based on workload characteristics
    if (avg_prompt_length > 1000 || avg_max_tokens > 200) {
        // Large prompts/tokens -> smaller batches
        max_batch_size_ = std::max(size_t(1), max_batch_size_ / 2);
    } else if (avg_prompt_length < 100 && avg_max_tokens < 50) {
        // Small prompts/tokens -> larger batches
        max_batch_size_ = std::min(size_t(32), max_batch_size_ * 2);
    }
}

BatchProcessor::BatchStats BatchProcessor::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void BatchProcessor::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = BatchStats{};
}

std::vector<EnhancedInferenceRequest> BatchProcessor::create_optimal_batch(
    const std::vector<EnhancedInferenceRequest>& requests) {
    
    switch (strategy_) {
        case BatchStrategy::DYNAMIC:
            return group_by_similarity(requests);
        case BatchStrategy::FIXED_SIZE:
            // Take first max_batch_size_ requests
            size_t count = std::min(requests.size(), max_batch_size_);
            return std::vector<EnhancedInferenceRequest>(requests.begin(), requests.begin() + count);
        case BatchStrategy::PRIORITY_BASED:
            // Sort by priority and take top requests
            {
                auto sorted_requests = requests;
                std::sort(sorted_requests.begin(), sorted_requests.end(),
                         [](const EnhancedInferenceRequest& a, const EnhancedInferenceRequest& b) {
                             return a.priority < b.priority; // Lower enum value = higher priority
                         });
                
                size_t count = std::min(sorted_requests.size(), max_batch_size_);
                return std::vector<EnhancedInferenceRequest>(sorted_requests.begin(), 
                                                           sorted_requests.begin() + count);
            }
        case BatchStrategy::ADAPTIVE:
            // Adaptive logic based on current load
            return create_optimal_batch(requests); // Simplified for now
        case BatchStrategy::OPTIMIZED:
            // ML-based optimization (placeholder)
            return group_by_similarity(requests);
        default:
            return group_by_similarity(requests);
    }
}

std::vector<EnhancedInferenceRequest> BatchProcessor::group_by_similarity(
    const std::vector<EnhancedInferenceRequest>& requests) {
    
    if (requests.size() <= max_batch_size_) {
        return requests;
    }
    
    // Simple similarity grouping based on prompt length
    std::vector<std::pair<size_t, EnhancedInferenceRequest>> indexed_requests;
    for (size_t i = 0; i < requests.size(); ++i) {
        indexed_requests.emplace_back(i, requests[i]);
    }
    
    // Sort by prompt length
    std::sort(indexed_requests.begin(), indexed_requests.end(),
             [](const auto& a, const auto& b) {
                 return a.second.prompt.length() < b.second.prompt.length();
             });
    
    // Create groups of similar sizes
    std::vector<EnhancedInferenceRequest> batch;
    size_t current_group_size = 0;
    float current_avg_length = 0.0f;
    
    for (const auto& [index, request] : indexed_requests) {
        if (batch.size() >= max_batch_size_) {
            break;
        }
        
        float request_length = static_cast<float>(request.prompt.length());
        
        if (current_group_size == 0) {
            current_avg_length = request_length;
            current_group_size = 1;
            batch.push_back(request);
        } else {
            float new_avg = (current_avg_length * current_group_size + request_length) / 
                           (current_group_size + 1);
            
            // Check if request is similar enough to current group
            float similarity = 1.0f - std::abs(request_length - current_avg_length) / 
                              std::max(request_length, current_avg_length);
            
            if (similarity > 0.7f || batch.size() < max_batch_size_ / 2) {
                batch.push_back(request);
                current_avg_length = new_avg;
                current_group_size++;
            }
        }
    }
    
    return batch;
}

std::vector<LlamaCppInferenceRequest> BatchProcessor::preprocess_batch(const BatchInferenceRequest& batch) {
    std::vector<LlamaCppInferenceRequest> llama_requests;
    llama_requests.reserve(batch.requests.size());
    
    for (const auto& req : batch.requests) {
        LlamaCppInferenceRequest llama_req;
        llama_req.prompt = req.prompt;
        llama_req.max_tokens = req.max_tokens;
        llama_req.temperature = req.temperature;
        llama_req.top_k = req.top_k;
        llama_req.top_p = req.top_p;
        llama_req.expected_model_hash = req.expected_model_hash;
        
        llama_requests.push_back(llama_req);
    }
    
    return llama_requests;
}

std::vector<EnhancedInferenceResult> BatchProcessor::postprocess_batch(
    const std::vector<LlamaCppInferenceReceipt>& receipts,
    const BatchInferenceRequest& original_batch) {
    
    std::vector<EnhancedInferenceResult> results;
    results.reserve(original_batch.requests.size());
    
    for (size_t i = 0; i < original_batch.requests.size() && i < receipts.size(); ++i) {
        EnhancedInferenceResult result;
        const auto& request = original_batch.requests[i];
        const auto& receipt = receipts[i];
        
        result.request_id = request.request_id;
        result.success = receipt.policy_allowed;
        result.text = receipt.text;
        result.token_ids = receipt.token_ids;
        
        if (!receipt.policy_allowed) {
            result.error_message = receipt.policy_reason;
        }
        
        // Calculate timing
        result.completion_time = std::chrono::steady_clock::now();
        result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.completion_time - request.submit_time);
        
        results.push_back(result);
    }
    
    return results;
}

void BatchProcessor::update_batch_stats(const BatchInferenceResult& result) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_batches_processed++;
    stats_.total_requests_processed += result.results.size();
    
    // Update average batch time
    if (stats_.total_batches_processed == 1) {
        stats_.average_batch_time = result.total_batch_time;
    } else {
        auto total_time = stats_.average_batch_time * (stats_.total_batches_processed - 1);
        stats_.average_batch_time = (total_time + result.total_batch_time) / 
                                   stats_.total_batches_processed;
    }
    
    // Update average request time
    if (result.results.size() > 0) {
        auto avg_req_time = result.total_batch_time / result.results.size();
        if (stats_.total_requests_processed == result.results.size()) {
            stats_.average_request_time = avg_req_time;
        } else {
            auto total_req_time = stats_.average_request_time * 
                                 (stats_.total_requests_processed - result.results.size());
            stats_.average_request_time = (total_req_time + avg_req_time * result.results.size()) / 
                                        stats_.total_requests_processed;
        }
    }
    
    // Update utilization
    stats_.average_batch_utilization = 
        (stats_.average_batch_utilization * (stats_.total_batches_processed - 1) + 
         result.batch_utilization_ratio) / stats_.total_batches_processed;
    
    // Update throughput
    stats_.throughput_tokens_per_second = result.throughput_tokens_per_second;
}

// AsyncInferenceEngine implementation
AsyncInferenceEngine::AsyncInferenceEngine(
    std::shared_ptr<t81::experimental::LlamaCppAdapter> adapter,
    std::shared_ptr<t81::memory::TensorMemoryManager> memory_manager)
    : adapter_(adapter), memory_manager_(memory_manager),
      queue_(std::make_unique<InferenceQueue>()),
      batch_processor_(std::make_unique<BatchProcessor>(adapter, memory_manager)) {}

AsyncInferenceEngine::~AsyncInferenceEngine() {
    stop();
}

std::future<EnhancedInferenceResult> AsyncInferenceEngine::infer_async(
    const EnhancedInferenceRequest& request) {
    
    auto promise = std::make_shared<std::promise<EnhancedInferenceResult>>();
    auto future = promise->get_future();
    
    // Add completion callback to request
    EnhancedInferenceRequest modified_request = request;
    modified_request.completion_callback = [promise](const std::string& result, bool success) {
        EnhancedInferenceResult inference_result;
        inference_result.success = success;
        inference_result.text = result;
        inference_result.completion_time = std::chrono::steady_clock::now();
        promise->set_value(inference_result);
    };
    
    if (!queue_->enqueue(modified_request)) {
        // Queue is full, return error result
        EnhancedInferenceResult error_result;
        error_result.success = false;
        error_result.error_message = "Queue is full";
        error_result.completion_time = std::chrono::steady_clock::now();
        promise->set_value(error_result);
    }
    
    return future;
}

std::future<std::vector<EnhancedInferenceResult>> AsyncInferenceEngine::infer_batch_async(
    const std::vector<EnhancedInferenceRequest>& requests) {
    
    return std::async(std::launch::async, [this, requests]() {
        return infer_batch(requests);
    });
}

EnhancedInferenceResult AsyncInferenceEngine::infer(const EnhancedInferenceRequest& request) {
    auto future = infer_async(request);
    return future.get();
}

std::vector<EnhancedInferenceResult> AsyncInferenceEngine::infer_batch(
    const std::vector<EnhancedInferenceRequest>& requests) {
    
    BatchInferenceRequest batch;
    batch.requests = requests;
    batch.max_batch_size = requests.size();
    
    auto batch_result = batch_processor_->process_batch(batch);
    return batch_result.results;
}

void AsyncInferenceEngine::start(size_t worker_count) {
    stop();
    
    for (size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back(&AsyncInferenceEngine::worker_loop, this);
    }
    
    stats_.active_workers = worker_count;
}

void AsyncInferenceEngine::stop() {
    should_stop_ = true;
    
    if (queue_) {
        queue_->queue_cv_.notify_all();
    }
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    stats_.active_workers = 0;
}

void AsyncInferenceEngine::wait_for_completion() {
    while (!queue_->empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AsyncInferenceEngine::set_batch_processor(std::unique_ptr<BatchProcessor> processor) {
    batch_processor_ = std::move(processor);
}

AsyncInferenceEngine::EngineStats AsyncInferenceEngine::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    EngineStats stats;
    stats.queue_stats = queue_->get_stats();
    stats.batch_stats = batch_processor_->get_stats();
    stats.active_workers = stats_.active_workers;
    stats.average_latency = stats_.average_latency;
    stats.requests_per_second = stats_.requests_per_second;
    stats.total_requests_processed = stats_.total_requests_processed;
    
    return stats;
}

void AsyncInferenceEngine::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (queue_) queue_->reset_stats();
    if (batch_processor_) batch_processor_->reset_stats();
    
    stats_ = EngineStats{};
}

void AsyncInferenceEngine::worker_loop() {
    while (!should_stop_) {
        // Try to get a batch of requests
        auto requests = queue_->dequeue_batch(batch_processor_->max_batch_size_);
        
        if (!requests.empty()) {
            process_batch_requests();
        } else {
            // Try to get a single request
            auto request = queue_->dequeue();
            if (request.has_value()) {
                process_single_request(request.value());
            } else {
                // No requests available, wait
                std::unique_lock<std::mutex> lock(queue_->queue_mutex_);
                queue_->queue_cv_.wait_for(lock, std::chrono::milliseconds(100));
            }
        }
    }
}

void AsyncInferenceEngine::process_single_request(const EnhancedInferenceRequest& request) {
    auto result = infer(request);
    
    if (request.completion_callback) {
        request.completion_callback(result.text, result.success);
    }
    
    update_engine_stats(result);
}

void AsyncInferenceEngine::process_batch_requests() {
    auto requests = queue_->dequeue_batch(batch_processor_->max_batch_size_);
    
    if (!requests.empty()) {
        auto results = infer_batch(requests);
        
        for (size_t i = 0; i < requests.size() && i < results.size(); ++i) {
            if (requests[i].completion_callback) {
                requests[i].completion_callback(results[i].text, results[i].success);
            }
        }
        
        // Create batch result for stats
        BatchInferenceResult batch_result;
        batch_result.results = results;
        update_engine_stats(batch_result);
    }
}

void AsyncInferenceEngine::update_engine_stats(const EnhancedInferenceResult& result) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_requests_processed++;
    
    // Update average latency
    if (stats_.total_requests_processed == 1) {
        stats_.average_latency = result.total_time;
    } else {
        auto total_latency = stats_.average_latency * (stats_.total_requests_processed - 1);
        stats_.average_latency = (total_latency + result.total_time) / 
                                 stats_.total_requests_processed;
    }
    
    // Update requests per second
    auto now = std::chrono::steady_clock::now();
    static auto last_update = now;
    static size_t requests_at_last_update = 0;
    
    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);
    if (time_diff.count() > 0) {
        auto requests_diff = stats_.total_requests_processed - requests_at_last_update;
        stats_.requests_per_second = static_cast<float>(requests_diff) / time_diff.count();
        
        last_update = now;
        requests_at_last_update = stats_.total_requests_processed;
    }
}

void AsyncInferenceEngine::update_engine_stats(const BatchInferenceResult& batch_result) {
    for (const auto& result : batch_result.results) {
        update_engine_stats(result);
    }
}

// Utility functions implementation
namespace inference_utils {

bool validate_request(const EnhancedInferenceRequest& request) {
    if (request.prompt.empty()) {
        return false;
    }
    
    if (request.max_tokens <= 0 || request.max_tokens > 4096) {
        return false;
    }
    
    if (request.temperature < 0.0f || request.temperature > 2.0f) {
        return false;
    }
    
    if (request.top_k < 1 || request.top_k > 1000) {
        return false;
    }
    
    if (request.top_p <= 0.0f || request.top_p > 1.0f) {
        return false;
    }
    
    return true;
}

bool validate_batch(const BatchInferenceRequest& batch) {
    if (batch.requests.empty()) {
        return false;
    }
    
    if (batch.requests.size() > batch.max_batch_size) {
        return false;
    }
    
    for (const auto& request : batch.requests) {
        if (!validate_request(request)) {
            return false;
        }
    }
    
    return true;
}

// ScopedInferenceTimer implementation
ScopedInferenceTimer::ScopedInferenceTimer(
    std::function<void(std::chrono::nanoseconds)> callback)
    : callback_(callback), start_time_(std::chrono::high_resolution_clock::now()) {}

ScopedInferenceTimer::~ScopedInferenceTimer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
    callback_(duration);
}

float calculate_request_similarity(const EnhancedInferenceRequest& a, 
                                 const EnhancedInferenceRequest& b) {
    // Simple similarity based on prompt length and parameters
    float length_similarity = 1.0f - std::abs(static_cast<float>(a.prompt.length()) - 
                                             static_cast<float>(b.prompt.length())) / 
                              std::max(static_cast<float>(a.prompt.length()), 
                                      static_cast<float>(b.prompt.length()));
    
    float temp_similarity = 1.0f - std::abs(a.temperature - b.temperature) / 2.0f;
    float tokens_similarity = 1.0f - std::abs(static_cast<float>(a.max_tokens) - 
                                             static_cast<float>(b.max_tokens)) / 
                              std::max(static_cast<float>(a.max_tokens), 
                                      static_cast<float>(b.max_tokens));
    
    return (length_similarity + temp_similarity + tokens_similarity) / 3.0f;
}

ResourceEstimate estimate_resources(const EnhancedInferenceRequest& request) {
    ResourceEstimate estimate;
    
    // Simple estimation based on prompt length and max tokens
    size_t prompt_tokens = request.prompt.length() / 4; // Rough estimate
    size_t total_tokens = prompt_tokens + request.max_tokens;
    
    // Memory estimation (very rough)
    estimate.estimated_memory_mb = (total_tokens * 4) / (1024 * 1024); // 4 bytes per token
    
    // Time estimation (very rough)
    estimate.estimated_time = std::chrono::milliseconds(total_tokens * 10); // 10ms per token
    
    // CPU utilization estimation
    estimate.cpu_utilization = std::min(1.0f, static_cast<float>(total_tokens) / 1000.0f);
    
    return estimate;
}

ResourceEstimate estimate_batch_resources(const BatchInferenceRequest& batch) {
    ResourceEstimate total_estimate;
    
    for (const auto& request : batch.requests) {
        auto request_estimate = estimate_resources(request);
        total_estimate.estimated_memory_mb += request_estimate.estimated_memory_mb;
        total_estimate.estimated_time = std::max(total_estimate.estimated_time, 
                                                request_estimate.estimated_time);
        total_estimate.cpu_utilization += request_estimate.cpu_utilization;
    }
    
    // Batch processing efficiency
    total_estimate.cpu_utilization = std::min(1.0f, total_estimate.cpu_utilization * 0.8f);
    
    return total_estimate;
}

} // namespace inference_utils

} // namespace t81::inference
