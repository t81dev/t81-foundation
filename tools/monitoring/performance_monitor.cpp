#include "t81/monitoring/performance_monitor.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <thread>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <unistd.h>

namespace t81::monitoring {

// PerformanceCollector implementation
PerformanceCollector::PerformanceCollector() {}

PerformanceCollector::~PerformanceCollector() {
    stop_resource_monitoring();
}

void PerformanceCollector::register_metric(const MetricDefinition& definition) {
    std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto metric_data = std::make_unique<MetricData>();
    metric_data->definition = definition;
    
    metrics_[definition.name] = std::move(metric_data);
}

void PerformanceCollector::unregister_metric(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
    metrics_.erase(name);
}

void PerformanceCollector::collect_metric(const std::string& name, double value,
                                        const std::unordered_map<std::string, std::string>& labels) {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto& metric_data = it->second;
        std::lock_guard<std::mutex> values_lock(metric_data->values_mutex);
        
        metric_data->values.emplace_back(name, value, labels);
        
        // Cleanup old values
        auto cutoff_time = std::chrono::steady_clock::now() - retention_period_;
        metric_data->values.erase(
            std::remove_if(metric_data->values.begin(), metric_data->values.end(),
                          [cutoff_time](const MetricValue& val) {
                              return val.timestamp < cutoff_time;
                          }),
            metric_data->values.end()
        );
        
        // Limit total number of values
        if (metric_data->values.size() > max_metric_values_) {
            metric_data->values.erase(metric_data->values.begin(), 
                                    metric_data->values.begin() + 
                                    (metric_data->values.size() - max_metric_values_));
        }
    }
}

void PerformanceCollector::increment_counter(const std::string& name, double increment,
                                             const std::unordered_map<std::string, std::string>& labels) {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto& metric_data = it->second;
        double new_value = metric_data->counter_value.fetch_add(increment) + increment;
        collect_metric(name, new_value, labels);
    }
}

void PerformanceCollector::set_gauge(const std::string& name, double value,
                                     const std::unordered_map<std::string, std::string>& labels) {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto& metric_data = it->second;
        metric_data->gauge_value.store(value);
        collect_metric(name, value, labels);
    }
}

void PerformanceCollector::record_timer(const std::string& name, std::chrono::nanoseconds duration,
                                       const std::unordered_map<std::string, std::string>& labels) {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto& metric_data = it->second;
        update_timer(*metric_data, duration);
        collect_metric(name, static_cast<double>(duration.count()), labels);
    }
}

void PerformanceCollector::observe_histogram(const std::string& name, double value,
                                            const std::unordered_map<std::string, std::string>& labels) {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto& metric_data = it->second;
        update_histogram(*metric_data, value);
        collect_metric(name, value, labels);
    }
}

ResourceSnapshot PerformanceCollector::collect_resource_snapshot() {
    ResourceSnapshot snapshot;
    snapshot.timestamp = std::chrono::steady_clock::now();
    
    // Get system information
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        snapshot.memory_total_bytes = info.totalram * info.mem_unit;
        snapshot.memory_free_bytes = info.freeram * info.mem_unit;
        snapshot.memory_used_bytes = snapshot.memory_total_bytes - snapshot.memory_free_bytes;
        snapshot.memory_usage_percent = 
            static_cast<double>(snapshot.memory_used_bytes) / snapshot.memory_total_bytes * 100.0;
        
        snapshot.cpu_load_average_1min = info.loads[0] / 65536.0;
        snapshot.cpu_load_average_5min = info.loads[1] / 65536.0;
        snapshot.cpu_load_average_15min = info.loads[2] / 65536.0;
    }
    
    // Get CPU core count
    snapshot.cpu_cores = std::thread::hardware_concurrency();
    
    // Get process-specific information
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // Convert user and system time to CPU usage percentage (simplified)
        auto total_cpu_time = static_cast<double>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) +
                            (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000000.0;
        auto uptime = static_cast<double>(info.uptime);
        if (uptime > 0) {
            snapshot.cpu_usage_percent = (total_cpu_time / uptime) * 100.0;
        }
    }
    
    // Get disk information (simplified)
    // In a real implementation, would use statvfs or similar
    snapshot.disk_total_bytes = 100 * 1024 * 1024 * 1024; // 100GB placeholder
    snapshot.disk_used_bytes = 50 * 1024 * 1024 * 1024;   // 50GB placeholder
    snapshot.disk_free_bytes = snapshot.disk_total_bytes - snapshot.disk_used_bytes;
    snapshot.disk_usage_percent = 
        static_cast<double>(snapshot.disk_used_bytes) / snapshot.disk_total_bytes * 100.0;
    
    return snapshot;
}

void PerformanceCollector::start_resource_monitoring(std::chrono::milliseconds interval) {
    stop_resource_monitoring();
    
    monitoring_interval_ = interval;
    monitoring_active_ = true;
    monitoring_thread_ = std::thread(&PerformanceCollector::monitoring_loop, this);
}

void PerformanceCollector::stop_resource_monitoring() {
    monitoring_active_ = false;
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

std::vector<MetricValue> PerformanceCollector::get_metric_values(
    const std::string& name, std::chrono::steady_clock::time_point since) {
    
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        return {};
    }
    
    std::lock_guard<std::mutex> values_lock(it->second->values_mutex);
    
    std::vector<MetricValue> values;
    for (const auto& value : it->second->values) {
        if (since == std::chrono::steady_clock::time_point{} || value.timestamp >= since) {
            values.push_back(value);
        }
    }
    
    return values;
}

std::vector<ResourceSnapshot> PerformanceCollector::get_resource_history(
    std::chrono::steady_clock::time_point since) {
    
    std::lock_guard<std::mutex> lock(resource_history_mutex_);
    
    std::vector<ResourceSnapshot> history;
    for (const auto& snapshot : resource_history_) {
        if (since == std::chrono::steady_clock::time_point{} || snapshot.timestamp >= since) {
            history.push_back(snapshot);
        }
    }
    
    return history;
}

std::optional<PerformanceCollector::MetricStats> PerformanceCollector::get_metric_stats(
    const std::string& name, std::chrono::steady_clock::time_point since) {
    
    auto values = get_metric_values(name, since);
    if (values.empty()) {
        return std::nullopt;
    }
    
    MetricStats stats;
    stats.count = values.size();
    stats.min = std::numeric_limits<double>::infinity();
    stats.max = -std::numeric_limits<double>::infinity();
    
    for (const auto& value : values) {
        stats.min = std::min(stats.min, value.value);
        stats.max = std::max(stats.max, value.value);
        stats.sum += value.value;
    }
    
    stats.avg = stats.sum / stats.count;
    
    // Calculate variance and standard deviation
    double variance_sum = 0.0;
    for (const auto& value : values) {
        double diff = value.value - stats.avg;
        variance_sum += diff * diff;
    }
    stats.variance = variance_sum / stats.count;
    stats.std_dev = std::sqrt(stats.variance);
    
    return stats;
}

void PerformanceCollector::monitoring_loop() {
    while (monitoring_active_) {
        auto snapshot = collect_resource_snapshot();
        
        {
            std::lock_guard<std::mutex> lock(resource_history_mutex_);
            resource_history_.push_back(snapshot);
            
            // Cleanup old history
            auto cutoff_time = std::chrono::steady_clock::now() - retention_period_;
            resource_history_.erase(
                std::remove_if(resource_history_.begin(), resource_history_.end(),
                              [cutoff_time](const ResourceSnapshot& snap) {
                                  return snap.timestamp < cutoff_time;
                              }),
                resource_history_.end()
            );
        }
        
        // Collect system metrics
        collect_metric("system.memory.usage_percent", snapshot.memory_usage_percent);
        collect_metric("system.cpu.usage_percent", snapshot.cpu_usage_percent);
        collect_metric("system.disk.usage_percent", snapshot.disk_usage_percent);
        
        std::this_thread::sleep_for(monitoring_interval_);
    }
}

void PerformanceCollector::cleanup_old_data() {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    
    auto cutoff_time = std::chrono::steady_clock::now() - retention_period_;
    
    for (auto& [name, metric_data] : metrics_) {
        std::lock_guard<std::mutex> values_lock(metric_data->values_mutex);
        
        metric_data->values.erase(
            std::remove_if(metric_data->values.begin(), metric_data->values.end(),
                          [cutoff_time](const MetricValue& val) {
                              return val.timestamp < cutoff_time;
                          }),
            metric_data->values.end()
        );
    }
    
    // Cleanup resource history
    {
        std::lock_guard<std::mutex> resource_lock(resource_history_mutex_);
        resource_history_.erase(
            std::remove_if(resource_history_.begin(), resource_history_.end(),
                          [cutoff_time](const ResourceSnapshot& snap) {
                              return snap.timestamp < cutoff_time;
                          }),
            resource_history_.end()
        );
    }
}

void PerformanceCollector::update_histogram(MetricData& metric_data, double value) {
    std::lock_guard<std::mutex> lock(metric_data.histogram_mutex);
    
    // Find appropriate bucket
    auto& buckets = metric_data.definition.histogram_buckets;
    auto bucket_it = std::upper_bound(buckets.begin(), buckets.end(), value);
    
    if (bucket_it != buckets.end()) {
        metric_data.histogram_counts[*bucket_it]++;
    } else {
        // Value exceeds all buckets, count in overflow
        metric_data.histogram_counts[std::numeric_limits<double>::infinity()]++;
    }
}

void PerformanceCollector::update_timer(MetricData& metric_data, std::chrono::nanoseconds duration) {
    std::lock_guard<std::mutex> lock(metric_data.timer_mutex);
    
    double duration_ms = static_cast<double>(duration.count()) / 1000000.0; // Convert to milliseconds
    metric_data.timer_values.push_back(duration_ms);
    
    // Limit timer values
    if (metric_data.timer_values.size() > max_metric_values_) {
        metric_data.timer_values.erase(metric_data.timer_values.begin(),
                                     metric_data.timer_values.begin() + 
                                     (metric_data.timer_values.size() - max_metric_values_));
    }
}

// AlertManager implementation
AlertManager::AlertManager(std::shared_ptr<PerformanceCollector> collector)
    : collector_(collector) {}

AlertManager::~AlertManager() {
    stop_monitoring();
}

void AlertManager::configure_alert(const std::string& metric_name, AlertLevel level,
                                  double threshold, const AlertCallback& callback) {
    AlertRule rule;
    rule.metric_name = metric_name;
    rule.level = level;
    rule.threshold = threshold;
    rule.callback = callback;
    
    alert_rules_[metric_name] = rule;
}

void AlertManager::remove_alert(const std::string& metric_name) {
    alert_rules_.erase(metric_name);
}

void AlertManager::start_monitoring() {
    stop_monitoring();
    
    alert_active_ = true;
    alert_thread_ = std::thread(&AlertManager::alert_loop, this);
}

void AlertManager::stop_monitoring() {
    alert_active_ = false;
    if (alert_thread_.joinable()) {
        alert_thread_.join();
    }
}

std::vector<PerformanceAlert> AlertManager::get_alert_history(
    std::chrono::steady_clock::time_point since) {
    
    std::lock_guard<std::mutex> lock(alert_history_mutex_);
    
    std::vector<PerformanceAlert> history;
    for (const auto& alert : alert_history_) {
        if (since == std::chrono::steady_clock::time_point{} || alert.timestamp >= since) {
            history.push_back(alert);
        }
    }
    
    return history;
}

void AlertManager::clear_alert_history() {
    std::lock_guard<std::mutex> lock(alert_history_mutex_);
    alert_history_.clear();
}

void AlertManager::register_alert_callback(AlertLevel level, AlertCallback callback) {
    alert_callbacks_[level].push_back(callback);
}

void AlertManager::unregister_alert_callback(AlertLevel level) {
    alert_callbacks_.erase(level);
}

AlertManager::AlertStats AlertManager::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void AlertManager::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = AlertStats{};
}

void AlertManager::alert_loop() {
    while (alert_active_) {
        check_alerts();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void AlertManager::check_alerts() {
    if (!collector_) {
        return;
    }
    
    for (const auto& [metric_name, rule] : alert_rules_) {
        auto now = std::chrono::steady_clock::now();
        
        // Check cooldown period
        if (now - rule.last_triggered < rule.cooldown_period) {
            continue;
        }
        
        // Get latest metric value
        auto values = collector_->get_metric_values(metric_name, now - std::chrono::seconds(5));
        if (values.empty()) {
            continue;
        }
        
        double latest_value = values.back().value;
        
        // Check threshold
        bool should_alert = false;
        switch (rule.level) {
            case AlertLevel::WARNING:
                should_alert = latest_value >= rule.threshold;
                break;
            case AlertLevel::ERROR:
                should_alert = latest_value >= rule.threshold;
                break;
            case AlertLevel::CRITICAL:
                should_alert = latest_value >= rule.threshold;
                break;
            default:
                break;
        }
        
        if (should_alert) {
            trigger_alert(metric_name, rule.level, latest_value, rule.threshold);
            
            // Update rule's last triggered time
            alert_rules_[metric_name].last_triggered = now;
        }
    }
}

void AlertManager::trigger_alert(const std::string& metric_name, AlertLevel level,
                                double current_value, double threshold) {
    std::string alert_id = metric_name + "_" + std::to_string(level) + "_" + 
                           std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::steady_clock::now().time_since_epoch()).count());
    
    std::string message = monitoring_utils::generate_alert_message(metric_name, level, 
                                                                 current_value, threshold);
    
    PerformanceAlert alert(alert_id, level, metric_name, message, current_value, threshold);
    
    // Store in history
    {
        std::lock_guard<std::mutex> lock(alert_history_mutex_);
        alert_history_.push_back(alert);
        
        // Limit history size
        if (alert_history_.size() > 10000) {
            alert_history_.erase(alert_history_..begin(), 
                               alert_history_.begin() + (alert_history_.size() - 10000));
        }
    }
    
    // Update stats
    update_alert_stats(level);
    
    // Call custom callback for this metric
    auto rule_it = alert_rules_.find(metric_name);
    if (rule_it != alert_rules_.end() && rule_it->second.callback) {
        rule_it->second.callback(alert);
    }
    
    // Call general callbacks for this alert level
    auto callback_it = alert_callbacks_.find(level);
    if (callback_it != alert_callbacks_.end()) {
        for (const auto& callback : callback_it->second) {
            callback(alert);
        }
    }
}

void AlertManager::update_alert_stats(AlertLevel level) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_alerts++;
    stats_.last_alert_time = std::chrono::steady_clock::now();
    
    switch (level) {
        case AlertLevel::INFO:
            stats_.info_alerts++;
            break;
        case AlertLevel::WARNING:
            stats_.warning_alerts++;
            break;
        case AlertLevel::ERROR:
            stats_.error_alerts++;
            break;
        case AlertLevel::CRITICAL:
            stats_.critical_alerts++;
            break;
    }
}

// PerformanceProfiler implementation
PerformanceProfiler::PerformanceProfiler(std::shared_ptr<PerformanceCollector> collector)
    : collector_(collector) {}

PerformanceProfiler::~PerformanceProfiler() {
    // Clean up any remaining sessions
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    for (auto& [session_id, session] : sessions_) {
        session.active = false;
        session.end_time = std::chrono::steady_clock::now();
    }
}

std::string PerformanceProfiler::start_profiling_session(const std::string& session_name) {
    std::string session_id = generate_session_id();
    
    ProfilingSession session;
    session.session_id = session_id;
    session.session_name = session_name.empty() ? "session_" + session_id : session_name;
    session.start_time = std::chrono::steady_clock::now();
    session.active = true;
    session.paused = false;
    
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    sessions_[session_id] = session;
    
    return session_id;
}

void PerformanceProfiler::stop_profiling_session(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.active = false;
        it->second.end_time = std::chrono::steady_clock::now();
    }
}

void PerformanceProfiler::pause_profiling_session(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.paused = true;
    }
}

void PerformanceProfiler::resume_profiling_session(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.paused = false;
    }
}

void PerformanceProfiler::record_operation_start(const std::string& operation_id, 
                                                const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(operations_mutex_);
    
    OperationProfile profile;
    profile.operation_name = operation_name;
    profile.start_time = std::chrono::high_resolution_clock::now();
    profile.session_id = ""; // Default session
    
    active_operations_[operation_id] = profile;
}

void PerformanceProfiler::record_operation_end(const std::string& operation_id) {
    complete_operation(operation_id);
}

void PerformanceProfiler::record_custom_metric(const std::string& metric_name, double value,
                                               const std::unordered_map<std::string, std::string>& labels) {
    if (collector_) {
        collector_->collect_metric(metric_name, value, labels);
    }
}

std::vector<PerformanceProfiler::ProfileData> PerformanceProfiler::get_profile_data(
    const std::string& session_id) {
    
    std::vector<ProfileData> profile_data;
    
    // This is a simplified implementation
    // In a real implementation, would aggregate data from completed operations
    
    return profile_data;
}

std::vector<PerformanceProfiler::ProfileData> PerformanceProfiler::get_top_operations(
    const std::string& session_id, size_t limit) {
    
    auto all_data = get_profile_data(session_id);
    
    // Sort by total time (descending)
    std::sort(all_data.begin(), all_data.end(),
              [](const ProfileData& a, const ProfileData& b) {
                  return a.total_time > b.total_time;
              });
    
    // Return top N
    if (all_data.size() > limit) {
        all_data.resize(limit);
    }
    
    return all_data;
}

void PerformanceProfiler::export_profile(const std::string& session_id, const std::string& filename) {
    auto profile_data = get_profile_data(session_id);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    file << "Operation,Total Time (ns),Min Time (ns),Max Time (ns),Avg Time (ns),Call Count\n";
    
    for (const auto& data : profile_data) {
        file << data.operation_name << ","
             << data.total_time.count() << ","
             << data.min_time.count() << ","
             << data.max_time.count() << ","
             << data.avg_time.count() << ","
             << data.call_count << "\n";
    }
}

void PerformanceProfiler::export_all_profiles(const std::string& directory) {
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    
    for (const auto& [session_id, session] : sessions_) {
        std::string filename = directory + "/profile_" + session.session_name + ".csv";
        export_profile(session_id, filename);
    }
}

std::string PerformanceProfiler::generate_session_id() {
    static std::atomic<uint64_t> counter{0};
    return "session_" + std::to_string(counter++);
}

void PerformanceProfiler::complete_operation(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(operations_mutex_);
    
    auto it = active_operations_.find(operation_id);
    if (it != active_operations_.end()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = end_time - it->second.start_time;
        
        // Record the timing metric
        if (collector_) {
            collector_->record_timer("operation.duration", duration);
        }
        
        active_operations_.erase(it);
    }
}

// ScopedProfiler implementation
PerformanceProfiler::ScopedProfiler::ScopedProfiler(
    PerformanceProfiler* profiler, const std::string& operation_name,
    const std::unordered_map<std::string, std::string>& labels)
    : profiler_(profiler), operation_name_(operation_name), labels_(labels),
      start_time_(std::chrono::high_resolution_clock::now()) {}

PerformanceProfiler::ScopedProfiler::~ScopedProfiler() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = end_time - start_time_;
    
    if (profiler_) {
        profiler_->record_custom_metric("operation.duration", 
                                       static_cast<double>(duration.count()), labels_);
    }
}

// Utility functions implementation
namespace monitoring_utils {

std::string format_bytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    size_t unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

std::string format_duration(std::chrono::nanoseconds duration) {
    auto ns = duration.count();
    
    if (ns < 1000) {
        return std::to_string(ns) + " ns";
    } else if (ns < 1000000) {
        return std::to_string(ns / 1000) + " μs";
    } else if (ns < 1000000000) {
        return std::to_string(ns / 1000000) + " ms";
    } else {
        return std::to_string(ns / 1000000000) + " s";
    }
}

std::string format_percent(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value << "%";
    return oss.str();
}

std::string format_rate(double rate, const std::string& unit) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << rate << " " << unit;
    return oss.str();
}

std::string timestamp_to_string(std::chrono::steady_clock::time_point timestamp) {
    auto time_t = std::chrono::duration_cast<std::chrono::seconds>(
        timestamp.time_since_epoch()).count();
    
    std::ostringstream oss;
    oss << time_t;
    return oss.str();
}

std::chrono::steady_clock::time_point string_to_timestamp(const std::string& timestamp_str) {
    try {
        auto time_t = std::stoll(timestamp_str);
        return std::chrono::steady_clock::time_point(std::chrono::seconds(time_t));
    } catch (...) {
        return std::chrono::steady_clock::time_point{};
    }
}

double calculate_percentile(const std::vector<double>& values, double percentile) {
    if (values.empty()) {
        return 0.0;
    }
    
    std::vector<double> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    
    size_t index = static_cast<size_t>(percentile / 100.0 * (sorted_values.size() - 1));
    index = std::min(index, sorted_values.size() - 1);
    
    return sorted_values[index];
}

double calculate_moving_average(const std::vector<double>& values, size_t window_size) {
    if (values.size() < window_size) {
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    double sum = std::accumulate(values.end() - window_size, values.end(), 0.0);
    return sum / window_size;
}

std::vector<double> smooth_data(const std::vector<double>& values, double alpha) {
    if (values.empty()) {
        return {};
    }
    
    std::vector<double> smoothed(values.size());
    smoothed[0] = values[0];
    
    for (size_t i = 1; i < values.size(); ++i) {
        smoothed[i] = alpha * values[i] + (1.0 - alpha) * smoothed[i - 1];
    }
    
    return smoothed;
}

std::string generate_alert_message(const std::string& metric_name, AlertLevel level,
                                   double current, double threshold) {
    std::ostringstream oss;
    oss << metric_name << " alert: ";
    
    switch (level) {
        case AlertLevel::INFO:
            oss << "INFO";
            break;
        case AlertLevel::WARNING:
            oss << "WARNING";
            break;
        case AlertLevel::ERROR:
            oss << "ERROR";
            break;
        case AlertLevel::CRITICAL:
            oss << "CRITICAL";
            break;
    }
    
    oss << " - Current value: " << current << ", Threshold: " << threshold;
    return oss.str();
}

std::string format_alert_for_logging(const PerformanceAlert& alert) {
    std::ostringstream oss;
    oss << "[" << timestamp_to_string(alert.timestamp) << "] "
        << "ALERT " << alert.alert_id << ": " << alert.message;
    return oss.str();
}

} // namespace monitoring_utils

} // namespace t81::monitoring
