#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <condition_variable>

namespace t81::monitoring {

// Performance metrics types
enum class MetricType {
    COUNTER,        // Cumulative counter
    GAUGE,          // Current value
    HISTOGRAM,      // Distribution of values
    TIMER,          // Duration measurements
    RATE,           // Rate per time unit
    CUSTOM          // Custom metric type
};

// Resource categories
enum class ResourceCategory {
    CPU,
    MEMORY,
    GPU,
    DISK,
    NETWORK,
    TENSOR,
    INFERENCE,
    CUSTOM
};

// Performance alert levels
enum class AlertLevel {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// Metric definition
struct MetricDefinition {
    std::string name;
    std::string description;
    MetricType type;
    ResourceCategory category;
    std::string unit;
    bool enabled = true;
    std::chrono::milliseconds collection_interval{1000};
    
    // For histograms
    std::vector<double> histogram_buckets = {0.1, 0.5, 1.0, 2.5, 5.0, 10.0, 25.0, 50.0, 100.0};
    
    // For alerts
    double warning_threshold = 0.0;
    double critical_threshold = 0.0;
    bool enable_alerts = false;
};

// Metric value
struct MetricValue {
    std::string metric_name;
    std::chrono::steady_clock::time_point timestamp;
    double value;
    std::unordered_map<std::string, std::string> labels;
    
    MetricValue(const std::string& name, double val, 
                const std::unordered_map<std::string, std::string>& lbls = {})
        : metric_name(name), timestamp(std::chrono::steady_clock::now()), 
          value(val), labels(lbls) {}
};

// Performance alert
struct PerformanceAlert {
    std::string alert_id;
    AlertLevel level;
    std::string metric_name;
    std::string message;
    double current_value;
    double threshold_value;
    std::chrono::steady_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> labels;
    
    PerformanceAlert(const std::string& id, AlertLevel lvl, const std::string& metric,
                     const std::string& msg, double current, double threshold)
        : alert_id(id), level(lvl), metric_name(metric), message(msg),
          current_value(current), threshold_value(threshold),
          timestamp(std::chrono::steady_clock::now()) {}
};

// Resource usage snapshot
struct ResourceSnapshot {
    std::chrono::steady_clock::time_point timestamp;
    
    // CPU metrics
    double cpu_usage_percent = 0.0;
    double cpu_load_average_1min = 0.0;
    double cpu_load_average_5min = 0.0;
    double cpu_load_average_15min = 0.0;
    size_t cpu_cores = 0;
    
    // Memory metrics
    size_t memory_total_bytes = 0;
    size_t memory_used_bytes = 0;
    size_t memory_free_bytes = 0;
    size_t memory_cached_bytes = 0;
    double memory_usage_percent = 0.0;
    
    // Tensor-specific metrics
    size_t tensor_memory_allocated = 0;
    size_t tensor_memory_cached = 0;
    size_t tensor_memory_quantized = 0;
    size_t active_tensors = 0;
    float tensor_cache_hit_rate = 0.0;
    
    // Inference metrics
    size_t total_inferences = 0;
    size_t active_inferences = 0;
    double average_inference_latency_ms = 0.0;
    double inference_throughput_per_sec = 0.0;
    size_t queued_inferences = 0;
    
    // Disk metrics
    size_t disk_total_bytes = 0;
    size_t disk_used_bytes = 0;
    size_t disk_free_bytes = 0;
    double disk_usage_percent = 0.0;
    double disk_read_rate_mb_per_sec = 0.0;
    double disk_write_rate_mb_per_sec = 0.0;
    
    // Network metrics
    double network_receive_rate_mb_per_sec = 0.0;
    double network_transmit_rate_mb_per_sec = 0.0;
    
    // Custom metrics
    std::unordered_map<std::string, double> custom_metrics;
};

// Performance collector
class PerformanceCollector {
public:
    explicit PerformanceCollector();
    ~PerformanceCollector();
    
    // Metric registration
    void register_metric(const MetricDefinition& definition);
    void unregister_metric(const std::string& name);
    
    // Data collection
    void collect_metric(const std::string& name, double value, 
                       const std::unordered_map<std::string, std::string>& labels = {});
    void increment_counter(const std::string& name, double increment = 1.0,
                          const std::unordered_map<std::string, std::string>& labels = {});
    void set_gauge(const std::string& name, double value,
                  const std::unordered_map<std::string, std::string>& labels = {});
    void record_timer(const std::string& name, std::chrono::nanoseconds duration,
                     const std::unordered_map<std::string, std::string>& labels = {});
    void observe_histogram(const std::string& name, double value,
                          const std::unordered_map<std::string, std::string>& labels = {});
    
    // Resource monitoring
    ResourceSnapshot collect_resource_snapshot();
    void start_resource_monitoring(std::chrono::milliseconds interval = std::chrono::seconds(1));
    void stop_resource_monitoring();
    
    // Data retrieval
    std::vector<MetricValue> get_metric_values(const std::string& name, 
                                              std::chrono::steady_clock::time_point since = {});
    std::vector<ResourceSnapshot> get_resource_history(
        std::chrono::steady_clock::time_point since = {});
    
    // Statistics
    struct MetricStats {
        double min = std::numeric_limits<double>::infinity();
        double max = -std::numeric_limits<double>::infinity();
        double sum = 0.0;
        double avg = 0.0;
        size_t count = 0;
        double variance = 0.0;
        double std_dev = 0.0;
    };
    
    std::optional<MetricStats> get_metric_stats(const std::string& name,
                                               std::chrono::steady_clock::time_point since = {});
    
    // Configuration
    void set_retention_period(std::chrono::seconds period) { retention_period_ = period; }
    void set_max_metric_values(size_t max_values) { max_metric_values_ = max_values; }
    
private:
    struct MetricData {
        MetricDefinition definition;
        std::vector<MetricValue> values;
        mutable std::mutex values_mutex;
        
        // For counters
        std::atomic<double> counter_value{0.0};
        
        // For gauges
        std::atomic<double> gauge_value{0.0};
        
        // For histograms
        std::unordered_map<double, size_t> histogram_counts;
        mutable std::mutex histogram_mutex;
        
        // For timers
        std::vector<double> timer_values;
        mutable std::mutex timer_mutex;
    };
    
    std::unordered_map<std::string, std::unique_ptr<MetricData>> metrics_;
    mutable std::shared_mutex metrics_mutex_;
    
    std::vector<ResourceSnapshot> resource_history_;
    mutable std::mutex resource_history_mutex_;
    
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_active_{false};
    std::chrono::milliseconds monitoring_interval_{1000};
    
    std::chrono::seconds retention_period_{3600}; // 1 hour default
    size_t max_metric_values_ = 10000;
    
    void monitoring_loop();
    void cleanup_old_data();
    ResourceSnapshot collect_system_metrics();
    void update_histogram(MetricData& metric_data, double value);
    void update_timer(MetricData& metric_data, std::chrono::nanoseconds duration);
};

// Alert manager
class AlertManager {
public:
    using AlertCallback = std::function<void(const PerformanceAlert&)>;
    
    explicit AlertManager(std::shared_ptr<PerformanceCollector> collector);
    ~AlertManager();
    
    // Alert configuration
    void configure_alert(const std::string& metric_name, AlertLevel level, 
                         double threshold, const AlertCallback& callback = nullptr);
    void remove_alert(const std::string& metric_name);
    
    // Alert management
    void start_monitoring();
    void stop_monitoring();
    
    // Alert history
    std::vector<PerformanceAlert> get_alert_history(
        std::chrono::steady_clock::time_point since = {});
    void clear_alert_history();
    
    // Alert callbacks
    void register_alert_callback(AlertLevel level, AlertCallback callback);
    void unregister_alert_callback(AlertLevel level);
    
    // Statistics
    struct AlertStats {
        size_t total_alerts = 0;
        size_t info_alerts = 0;
        size_t warning_alerts = 0;
        size_t error_alerts = 0;
        size_t critical_alerts = 0;
        std::chrono::steady_clock::time_point last_alert_time;
    };
    
    AlertStats get_stats() const;
    void reset_stats();
    
private:
    struct AlertRule {
        std::string metric_name;
        AlertLevel level;
        double threshold;
        AlertCallback callback;
        std::chrono::steady_clock::time_point last_triggered;
        std::chrono::milliseconds cooldown_period{5000}; // 5 seconds default
    };
    
    std::shared_ptr<PerformanceCollector> collector_;
    std::unordered_map<std::string, AlertRule> alert_rules_;
    std::unordered_map<AlertLevel, std::vector<AlertCallback>> alert_callbacks_;
    
    std::vector<PerformanceAlert> alert_history_;
    mutable std::mutex alert_history_mutex_;
    
    std::thread alert_thread_;
    std::atomic<bool> alert_active_{false};
    
    mutable std::mutex stats_mutex_;
    AlertStats stats_;
    
    void alert_loop();
    void check_alerts();
    void trigger_alert(const std::string& metric_name, AlertLevel level, 
                      double current_value, double threshold);
    void update_alert_stats(AlertLevel level);
};

// Performance profiler
class PerformanceProfiler {
public:
    explicit PerformanceProfiler(std::shared_ptr<PerformanceCollector> collector);
    ~PerformanceProfiler();
    
    // Profiling sessions
    std::string start_profiling_session(const std::string& session_name = "");
    void stop_profiling_session(const std::string& session_id);
    void pause_profiling_session(const std::string& session_id);
    void resume_profiling_session(const std::string& session_id);
    
    // Profiling scopes
    class ScopedProfiler {
    public:
        ScopedProfiler(PerformanceProfiler* profiler, const std::string& operation_name,
                      const std::unordered_map<std::string, std::string>& labels = {});
        ~ScopedProfiler();
        
    private:
        PerformanceProfiler* profiler_;
        std::string operation_name_;
        std::unordered_map<std::string, std::string> labels_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };
    
    // Manual profiling
    void record_operation_start(const std::string& operation_id, const std::string& operation_name);
    void record_operation_end(const std::string& operation_id);
    void record_custom_metric(const std::string& metric_name, double value,
                              const std::unordered_map<std::string, std::string>& labels = {});
    
    // Profile analysis
    struct ProfileData {
        std::string operation_name;
        std::chrono::nanoseconds total_time{0};
        std::chrono::nanoseconds min_time{std::chrono::nanoseconds::max()};
        std::chrono::nanoseconds max_time{0};
        std::chrono::nanoseconds avg_time{0};
        size_t call_count = 0;
        double time_per_call = 0.0;
        std::unordered_map<std::string, std::string> labels;
    };
    
    std::vector<ProfileData> get_profile_data(const std::string& session_id = "");
    std::vector<ProfileData> get_top_operations(const std::string& session_id = "", size_t limit = 10);
    
    // Profile export
    void export_profile(const std::string& session_id, const std::string& filename);
    void export_all_profiles(const std::string& directory);
    
private:
    struct ProfilingSession {
        std::string session_id;
        std::string session_name;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
        bool active = true;
        bool paused = false;
    };
    
    struct OperationProfile {
        std::string operation_name;
        std::chrono::high_resolution_clock::time_point start_time;
        std::unordered_map<std::string, std::string> labels;
        std::string session_id;
    };
    
    std::shared_ptr<PerformanceCollector> collector_;
    std::unordered_map<std::string, ProfilingSession> sessions_;
    std::unordered_map<std::string, OperationProfile> active_operations_;
    
    mutable std::shared_mutex sessions_mutex_;
    mutable std::mutex operations_mutex_;
    
    std::string generate_session_id();
    void complete_operation(const std::string& operation_id);
};

// Resource manager
class ResourceManager {
public:
    explicit ResourceManager(std::shared_ptr<PerformanceCollector> collector,
                           std::shared_ptr<AlertManager> alert_manager = nullptr);
    ~ResourceManager();
    
    // Resource limits
    void set_memory_limit(size_t limit_bytes);
    void set_cpu_limit(double max_usage_percent);
    void set_inference_queue_limit(size_t max_size);
    void set_tensor_cache_limit(size_t limit_bytes);
    
    // Resource allocation tracking
    void track_memory_allocation(size_t bytes, const std::string& component);
    void track_memory_deallocation(size_t bytes, const std::string& component);
    void track_tensor_allocation(size_t bytes, const std::string& tensor_name);
    void track_tensor_deallocation(size_t bytes, const std::string& tensor_name);
    
    // Resource enforcement
    bool can_allocate_memory(size_t bytes, const std::string& component);
    bool can_queue_inference();
    bool can_cache_tensor(size_t bytes);
    
    // Resource optimization
    void optimize_memory_usage();
    void optimize_tensor_cache();
    void optimize_inference_queue();
    
    // Resource status
    struct ResourceStatus {
        size_t memory_allocated = 0;
        size_t memory_limit = 0;
        double memory_usage_percent = 0.0;
        size_t tensor_memory_allocated = 0;
        size_t tensor_cache_size = 0;
        size_t inference_queue_size = 0;
        size_t inference_queue_limit = 0;
        bool memory_pressure = false;
        bool cpu_pressure = false;
        std::vector<std::string> recommendations;
    };
    
    ResourceStatus get_resource_status() const;
    std::vector<std::string> get_optimization_recommendations() const;
    
    // Auto-scaling
    void enable_auto_scaling();
    void disable_auto_scaling();
    void set_scaling_policy(std::function<void()> policy);
    
private:
    std::shared_ptr<PerformanceCollector> collector_;
    std::shared_ptr<AlertManager> alert_manager_;
    
    // Resource limits
    std::atomic<size_t> memory_limit_{0};
    std::atomic<double> cpu_limit_{100.0};
    std::atomic<size_t> inference_queue_limit_{1000};
    std::atomic<size_t> tensor_cache_limit_{1024 * 1024 * 1024}; // 1GB default
    
    // Current usage
    std::atomic<size_t> memory_allocated_{0};
    std::atomic<size_t> tensor_memory_allocated_{0};
    std::atomic<size_t> tensor_cache_size_{0};
    std::atomic<size_t> inference_queue_size_{0};
    
    // Component tracking
    std::unordered_map<std::string, size_t> component_memory_usage_;
    std::unordered_map<std::string, size_t> tensor_memory_usage_;
    
    mutable std::shared_mutex usage_mutex_;
    
    // Auto-scaling
    std::atomic<bool> auto_scaling_enabled_{false};
    std::function<void()> scaling_policy_;
    std::thread scaling_thread_;
    std::atomic<bool> scaling_active_{false};
    
    void scaling_loop();
    bool check_memory_pressure();
    bool check_cpu_pressure();
    void enforce_resource_limits();
};

// Performance dashboard (basic implementation)
class PerformanceDashboard {
public:
    explicit PerformanceDashboard(std::shared_ptr<PerformanceCollector> collector,
                                  std::shared_ptr<AlertManager> alert_manager = nullptr);
    ~PerformanceDashboard() = default;
    
    // Dashboard generation
    std::string generate_html_dashboard() const;
    std::string generate_json_summary() const;
    void export_dashboard(const std::string& filename, const std::string& format = "html") const;
    
    // Real-time monitoring
    void start_web_server(int port = 8080);
    void stop_web_server();
    
    // Custom widgets
    void add_custom_widget(const std::string& name, 
                         std::function<std::string()> generator);
    
private:
    std::shared_ptr<PerformanceCollector> collector_;
    std::shared_ptr<AlertManager> alert_manager_;
    
    std::unordered_map<std::string, std::function<std::string()>> custom_widgets_;
    
    std::thread web_server_thread_;
    std::atomic<bool> web_server_active_{false};
    
    void web_server_loop(int port);
    std::string generate_metric_chart(const std::string& metric_name) const;
    std::string generate_resource_panel() const;
    std::string generate_alert_panel() const;
};

// Utility functions
namespace monitoring_utils {
    // Metric formatting
    std::string format_bytes(size_t bytes);
    std::string format_duration(std::chrono::nanoseconds duration);
    std::string format_percent(double value);
    std::string format_rate(double rate, const std::string& unit);
    
    // Time utilities
    std::string timestamp_to_string(std::chrono::steady_clock::time_point timestamp);
    std::chrono::steady_clock::time_point string_to_timestamp(const std::string& timestamp_str);
    
    // System information
    struct SystemInfo {
        std::string hostname;
        std::string os_name;
        std::string os_version;
        std::string cpu_model;
        size_t cpu_cores;
        size_t memory_total;
        std::vector<std::string> network_interfaces;
    };
    
    SystemInfo get_system_info();
    
    // Performance calculations
    double calculate_percentile(const std::vector<double>& values, double percentile);
    double calculate_moving_average(const std::vector<double>& values, size_t window_size);
    std::vector<double> smooth_data(const std::vector<double>& values, double alpha = 0.3);
    
    // Alert helpers
    std::string generate_alert_message(const std::string& metric_name, 
                                      AlertLevel level, double current, double threshold);
    std::string format_alert_for_logging(const PerformanceAlert& alert);
}

} // namespace t81::monitoring
