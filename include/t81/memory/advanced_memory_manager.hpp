#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <thread>
#include <functional>
#include "t81/codec/enhanced_gguf_parser.hpp"
#include "t81/codec/advanced_ternary_quantization.hpp"

namespace t81::memory {

// Memory allocation strategies
enum class AllocationStrategy {
    LAZY,           // Allocate on demand
    PREEMPTIVE,     // Pre-allocate based on usage patterns
    POOL_BASED,     // Use fixed-size pools
    TIERED,         // Multiple memory tiers (fast/slow)
    ADAPTIVE        // Adapt to usage patterns
};

// Memory priority levels
enum class MemoryPriority {
    CRITICAL,       // Cannot be evicted
    HIGH,           // Evicted only under memory pressure
    NORMAL,         // Normal eviction policy
    LOW,            // First to be evicted
    TEMPORARY       // Very short lifetime
};

// Memory block metadata
struct MemoryBlock {
    void* data;
    size_t size;
    size_t alignment;
    MemoryPriority priority;
    std::chrono::steady_clock::time_point last_access;
    std::chrono::steady_clock::time_point creation_time;
    uint32_t access_count;
    std::string tag;
    bool is_pinned;
    bool is_quantized;
    
    MemoryBlock() : data(nullptr), size(0), alignment(0), 
                   priority(MemoryPriority::NORMAL), access_count(0),
                   is_pinned(false), is_quantized(false) {}
};

// Advanced memory pool with tiered allocation
class TieredMemoryPool {
public:
    struct TierConfig {
        size_t pool_size;
        size_t block_size;
        size_t max_blocks;
        bool allow_growth;
        float growth_factor;
    };
    
    explicit TieredMemoryPool(const std::vector<TierConfig>& tiers);
    ~TieredMemoryPool();
    
    // Memory allocation
    std::shared_ptr<MemoryBlock> allocate(size_t size, size_t alignment = 16,
                                         MemoryPriority priority = MemoryPriority::NORMAL,
                                         const std::string& tag = "");
    
    // Deallocation (automatic through shared_ptr)
    void deallocate(MemoryBlock* block);
    
    // Memory management
    void compact();
    void defragment();
    void garbage_collect();
    
    // Statistics
    struct PoolStats {
        size_t total_allocated = 0;
        size_t total_used = 0;
        size_t total_free = 0;
        size_t fragmentation_ratio = 0;
        size_t allocation_count = 0;
        size_t deallocation_count = 0;
        float average_allocation_time = 0.0f;
        float peak_usage_ratio = 0.0f;
    };
    
    PoolStats get_stats() const;
    void reset_stats();
    
    // Configuration
    void set_allocation_strategy(AllocationStrategy strategy) { strategy_ = strategy; }
    AllocationStrategy get_allocation_strategy() const { return strategy_; }
    
private:
    struct MemoryTier {
        std::vector<uint8_t> memory_pool;
        std::vector<bool> allocation_map;
        TierConfig config;
        std::mutex tier_mutex;
        size_t used_blocks = 0;
        
        MemoryTier(const TierConfig& cfg) : config(cfg) {
            memory_pool.resize(cfg.pool_size);
            allocation_map.resize(cfg.max_blocks, false);
        }
    };
    
    std::vector<std::unique_ptr<MemoryTier>> tiers_;
    AllocationStrategy strategy_;
    mutable std::shared_mutex pool_mutex_;
    PoolStats stats_;
    
    // Tier selection
    size_t select_tier(size_t size) const;
    
    // Internal allocation
    void* allocate_from_tier(MemoryTier& tier, size_t size, size_t alignment);
    void deallocate_from_tier(MemoryTier& tier, void* ptr);
    
    // Statistics tracking
    void update_allocation_stats(size_t size, std::chrono::nanoseconds duration);
    void update_deallocation_stats();
};

// Tensor memory manager with caching and optimization
class TensorMemoryManager {
public:
    explicit TensorMemoryManager(std::shared_ptr<TieredMemoryPool> pool = nullptr);
    ~TensorMemoryManager() = default;
    
    // Tensor allocation
    std::shared_ptr<MemoryBlock> allocate_tensor(const std::vector<uint32_t>& dimensions,
                                                bool quantized = false,
                                                MemoryPriority priority = MemoryPriority::NORMAL,
                                                const std::string& name = "");
    
    std::shared_ptr<MemoryBlock> allocate_tensor(size_t element_count,
                                                bool quantized = false,
                                                MemoryPriority priority = MemoryPriority::NORMAL,
                                                const std::string& name = "");
    
    // Tensor caching
    void cache_tensor(const std::string& name, std::shared_ptr<MemoryBlock> tensor);
    std::shared_ptr<MemoryBlock> get_cached_tensor(const std::string& name);
    void remove_cached_tensor(const std::string& name);
    void clear_tensor_cache();
    
    // Batch operations
    std::vector<std::shared_ptr<MemoryBlock>> allocate_tensor_batch(
        const std::vector<std::vector<uint32_t>>& dimensions_list,
        bool quantized = false,
        MemoryPriority priority = MemoryPriority::NORMAL);
    
    // Memory optimization
    void optimize_layout();
    void prefetch_tensors(const std::vector<std::string>& tensor_names);
    void evict_low_priority_tensors();
    
    // Statistics
    struct TensorStats {
        size_t total_tensors = 0;
        size_t cached_tensors = 0;
        size_t total_memory_used = 0;
        size_t quantized_memory_used = 0;
        size_t cached_memory_used = 0;
        float cache_hit_rate = 0.0f;
        float quantization_ratio = 0.0f;
        size_t cache_evictions = 0;
    };
    
    TensorStats get_stats() const;
    void reset_stats();
    
    // Configuration
    void set_cache_size_limit(size_t limit) { cache_size_limit_ = limit; }
    void set_quantization_threshold(size_t threshold) { quantization_threshold_ = threshold; }
    
private:
    std::shared_ptr<TieredMemoryPool> memory_pool_;
    std::unordered_map<std::string, std::shared_ptr<MemoryBlock>> tensor_cache_;
    mutable std::shared_mutex cache_mutex_;
    TensorStats stats_;
    
    size_t cache_size_limit_ = 1024 * 1024 * 1024; // 1GB default
    size_t quantization_threshold_ = 1024 * 1024;  // 1MB default
    
    // Cache management
    void evict_from_cache_if_needed();
    void update_cache_stats(const std::string& tensor_name, bool hit);
    
    // Layout optimization
    void reorganize_memory_layout();
    std::vector<std::string> get_tensor_access_order() const;
};

// Asynchronous memory manager for background operations
class AsyncMemoryManager {
public:
    using TaskCallback = std::function<void()>;
    
    explicit AsyncMemoryManager(std::shared_ptr<TensorMemoryManager> tensor_manager);
    ~AsyncMemoryManager();
    
    // Asynchronous operations
    std::future<std::shared_ptr<MemoryBlock>> allocate_tensor_async(
        const std::vector<uint32_t>& dimensions,
        bool quantized = false,
        MemoryPriority priority = MemoryPriority::NORMAL,
        const std::string& name = "");
    
    std::future<void> cache_tensor_async(const std::string& name, 
                                        std::shared_ptr<MemoryBlock> tensor);
    
    std::future<std::shared_ptr<MemoryBlock>> load_tensor_async(
        const std::string& file_path,
        const t81::codec::GGUTensorInfo& tensor_info);
    
    // Background tasks
    void schedule_task(TaskCallback task, MemoryPriority priority = MemoryPriority::NORMAL);
    void schedule_garbage_collection();
    void schedule_memory_optimization();
    
    // Control
    void start_workers(size_t worker_count = 4);
    void stop_workers();
    void wait_for_all_tasks();
    
    // Statistics
    struct AsyncStats {
        size_t pending_tasks = 0;
        size_t completed_tasks = 0;
        size_t failed_tasks = 0;
        float average_task_time = 0.0f;
        size_t worker_utilization = 0;
    };
    
    AsyncStats get_stats() const;
    
private:
    struct Task {
        TaskCallback callback;
        MemoryPriority priority;
        std::chrono::steady_clock::time_point creation_time;
        std::promise<void> promise;
        
        Task(TaskCallback cb, MemoryPriority p) 
            : callback(std::move(cb)), priority(p), 
              creation_time(std::chrono::steady_clock::now()) {}
    };
    
    std::shared_ptr<TensorMemoryManager> tensor_manager_;
    std::vector<std::thread> workers_;
    std::priority_queue<std::shared_ptr<Task>, 
                       std::vector<std::shared_ptr<Task>>,
                       std::function<bool(const std::shared_ptr<Task>&, const std::shared_ptr<Task>&)>> task_queue_;
    
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> should_stop_{false};
    
    mutable std::mutex stats_mutex_;
    AsyncStats stats_;
    
    void worker_loop();
    void update_task_stats(std::chrono::nanoseconds duration, bool success);
};

// Memory monitoring and profiling
class MemoryProfiler {
public:
    explicit MemoryProfiler(std::shared_ptr<TensorMemoryManager> tensor_manager);
    ~MemoryProfiler() = default;
    
    // Profiling
    void start_profiling();
    void stop_profiling();
    void reset_profile();
    
    // Memory tracking
    struct MemorySnapshot {
        std::chrono::steady_clock::time_point timestamp;
        size_t total_allocated;
        size_t total_used;
        size_t cached_memory;
        size_t quantized_memory;
        float fragmentation_ratio;
        size_t active_tensors;
        size_t cache_hits;
        size_t cache_misses;
    };
    
    std::vector<MemorySnapshot> get_profile_history() const;
    MemorySnapshot get_current_snapshot() const;
    
    // Analysis
    struct MemoryAnalysis {
        float average_usage = 0.0f;
        float peak_usage = 0.0f;
        float memory_efficiency = 0.0f;
        float cache_effectiveness = 0.0f;
        size_t fragmentation_events = 0;
        std::vector<std::string> recommendations;
    };
    
    MemoryAnalysis analyze_memory_usage() const;
    std::vector<std::string> get_optimization_recommendations() const;
    
    // Reporting
    void generate_report(const std::string& filename) const;
    void print_summary() const;
    
private:
    std::shared_ptr<TensorMemoryManager> tensor_manager_;
    std::vector<MemorySnapshot> profile_history_;
    std::atomic<bool> profiling_{false};
    std::thread profiling_thread_;
    
    void profiling_loop();
    MemorySnapshot capture_snapshot() const;
};

// Memory leak detection and validation
class MemoryValidator {
public:
    explicit MemoryValidator(std::shared_ptr<TensorMemoryManager> tensor_manager);
    ~MemoryValidator();
    
    // Validation
    bool validate_memory_integrity() const;
    bool detect_memory_leaks() const;
    bool validate_tensor_dimensions() const;
    
    // Leak tracking
    void track_allocation(void* ptr, size_t size, const std::string& tag);
    void track_deallocation(void* ptr);
    
    // Reporting
    struct LeakReport {
        size_t total_leaks = 0;
        size_t total_leaked_bytes = 0;
        std::vector<std::pair<void*, std::string>> leak_details;
    };
    
    LeakReport generate_leak_report() const;
    void print_leak_report() const;
    
private:
    std::shared_ptr<TensorMemoryManager> tensor_manager_;
    std::unordered_map<void*, std::pair<size_t, std::string>> allocations_;
    mutable std::mutex validation_mutex_;
    
    bool is_valid_pointer(void* ptr) const;
    void cleanup_tracking();
};

// Utility functions
namespace memory_utils {
    // Size calculations
    size_t calculate_tensor_size(const std::vector<uint32_t>& dimensions, bool quantized = false);
    size_t calculate_aligned_size(size_t size, size_t alignment);
    
    // Memory alignment
    void* aligned_alloc(size_t size, size_t alignment);
    void aligned_free(void* ptr);
    
    // Memory utilities
    void memset_zero(void* ptr, size_t size);
    void memcpy_optimized(void* dest, const void* src, size_t size);
    
    // Performance measurement
    class ScopedTimer {
    public:
        explicit ScopedTimer(std::function<void(std::chrono::nanoseconds)> callback);
        ~ScopedTimer();
        
    private:
        std::chrono::steady_clock::time_point start_;
        std::function<void(std::chrono::nanoseconds)> callback_;
    };
    
    // Memory pressure detection
    bool is_memory_pressure_high();
    size_t get_available_memory();
    float get_memory_usage_ratio();
}

} // namespace t81::memory
