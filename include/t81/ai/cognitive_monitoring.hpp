#pragma once

#include "t81/ai/cognitive_tiers.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace t81::ai::cognitive {

// Operation context for monitoring
struct OperationContext {
    std::string operation_id;
    CognitiveTier current_tier;
    std::string user_id;
    std::string project_id;
    std::map<std::string, std::string> metadata;
    
    // Runtime constraints
    size_t execution_time_ms;
    size_t memory_used_mb;
    size_t network_calls_made;
    std::vector<std::string> operations_executed;
};

// Operation metrics for monitoring
struct OperationMetrics {
    std::chrono::system_clock::time_point timestamp;
    size_t cpu_time_ms;
    size_t memory_used_mb;
    size_t network_calls_made;
    std::string status;
    std::map<std::string, std::string> additional_metrics;
};

// Operation result for monitoring
struct OperationResult {
    std::string status;
    std::string error_message;
    std::map<std::string, std::string> result_data;
    std::chrono::system_clock::time_point completion_time;
};

// Monitoring session
struct MonitoringSession {
    std::string session_id;
    std::string operation_id;
    OperationContext context;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::string status;
    
    // Metrics collection
    std::vector<OperationMetrics> metrics;
    size_t total_cpu_time;
    size_t total_memory_used;
    size_t total_network_calls;
    
    // Alerts
    std::vector<std::string> alerts;
    
    // Result
    OperationResult result;
    
    // Performance tracking
    struct PerformanceMetrics {
        std::chrono::system_clock::time_point start_time;
        std::vector<std::pair<std::chrono::system_clock::time_point, double>> cpu_usage_history;
        std::vector<std::pair<std::chrono::system_clock::time_point, size_t>> memory_usage_history;
        std::vector<std::pair<std::chrono::system_clock::time_point, double>> network_usage_history;
        size_t error_count;
        size_t warning_count;
    } performance_metrics;
    
    // Final metrics
    struct FinalMetrics {
        size_t total_duration_ms;
        double avg_cpu_time;
        size_t peak_memory_usage;
        size_t total_network_calls;
        double error_rate;
        double efficiency_score;
    } final_metrics;
};

// Session analysis results
struct SessionAnalysis {
    std::string session_id;
    std::chrono::system_clock::time_point analysis_time;
    
    // Performance analysis
    std::string performance_rating;  // "excellent", "good", "fair", "poor"
    double resource_efficiency;
    double stability_score;
    
    // Behavioral analysis
    std::vector<std::string> behavioral_patterns;
    std::vector<std::string> anomaly_detection;
    std::vector<std::string> recommendations;
    
    // Risk assessment
    std::vector<std::string> risk_factors;
    std::string compliance_status;  // "compliant", "non_compliant", "requires_review"
};

// Cognitive alert
struct CognitiveAlert {
    std::string alert_id;
    std::string session_id;
    std::string alert_type;
    std::string message;
    std::string details;
    std::string severity;  // "info", "warning", "critical"
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point acknowledged_time;
    std::string acknowledged_by;
    std::string status;  // "active", "acknowledged", "resolved"
    double value;  // Numeric value for threshold-based alerts
};

// Monitoring thresholds
struct MonitoringThresholds {
    size_t max_cpu_time_ms;
    size_t max_memory_mb;
    size_t max_network_calls;
    size_t max_session_duration_ms;
    double alert_cpu_threshold;      // Percentage of max
    double alert_memory_threshold;    // Percentage of max
    double alert_error_rate_threshold;
    std::chrono::seconds alert_cooldown;
};

// Monitoring dashboard data
struct MonitoringDashboard {
    size_t current_active_sessions;
    size_t total_sessions_today;
    double avg_session_duration;
    size_t total_cpu_time_today;
    size_t total_memory_used_today;
    
    // Tier distribution
    std::map<CognitiveTier, size_t> tier_distribution;
    
    // Alert summary
    size_t active_alerts_count;
    size_t critical_alerts_count;
    
    // Performance trends
    std::vector<std::string> performance_trends;
    
    // System health
    std::string system_health;
    
    // Timestamp
    std::chrono::system_clock::time_point last_updated;
};

// Main cognitive monitoring interface
class CognitiveMonitoring {
public:
    virtual ~CognitiveMonitoring() = default;
    
    // Session management
    virtual void start_operation_monitoring(
        const std::string& operation_id,
        const OperationContext& context) = 0;
    
    virtual void record_operation_metrics(
        const std::string& operation_id,
        const OperationMetrics& metrics) = 0;
    
    virtual void complete_operation_monitoring(
        const std::string& operation_id,
        const OperationResult& result) = 0;
    
    // Session queries
    virtual std::vector<MonitoringSession> get_active_sessions() = 0;
    
    virtual std::vector<MonitoringSession> get_completed_sessions(
        const std::chrono::system_clock::time_point& since) = 0;
    
    virtual SessionAnalysis get_session_analysis(
        const std::string& session_id) = 0;
    
    // Alert management
    virtual std::vector<CognitiveAlert> get_active_alerts() = 0;
    
    virtual void acknowledge_alert(
        const std::string& alert_id,
        const std::string& acknowledged_by) = 0;
    
    // Dashboard
    virtual MonitoringDashboard get_dashboard_data() = 0;
    
    // Threshold management
    virtual void set_monitoring_thresholds(
        const MonitoringThresholds& thresholds) = 0;
    
    virtual MonitoringThresholds get_monitoring_thresholds() = 0;
    
    // Data export
    virtual void export_monitoring_data(
        const std::string& export_path,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time) = 0;
    
    // Real-time monitoring
    virtual bool enable_real_time_monitoring() = 0;
    virtual bool disable_real_time_monitoring() = 0;
    virtual bool is_real_time_monitoring_active() = 0;
    
    // Performance profiling
    virtual void start_performance_profiling(const std::string& operation_id) = 0;
    virtual void stop_performance_profiling(const std::string& operation_id) = 0;
    virtual std::map<std::string, double> get_performance_profile(const std::string& operation_id) = 0;
    
    // Behavioral analysis
    virtual std::vector<std::string> analyze_behavioral_patterns(
        const std::string& user_id,
        const std::chrono::system_clock::time_point& since) = 0;
    
    virtual std::vector<std::string> detect_anomalies(
        const std::string& operation_id) = 0;
    
    virtual std::vector<std::string> generate_recommendations(
        const std::string& session_id) = 0;
    
    // Compliance checking
    virtual bool check_compliance_status(const std::string& session_id) = 0;
    virtual std::vector<std::string> get_compliance_violations(const std::string& session_id) = 0;
    
    // Health monitoring
    virtual std::string get_system_health_status() = 0;
    virtual std::map<std::string, double> get_system_metrics() = 0;
    virtual std::vector<std::string> get_health_warnings() = 0;
    
    // Alert configuration
    virtual void configure_alert_rules(
        const std::vector<std::string>& alert_types,
        const std::vector<std::string>& recipients) = 0;
    
    virtual void set_alert_severity_thresholds(
        const std::map<std::string, double>& thresholds) = 0;
    
    virtual std::vector<std::string> get_alert_recipients(const std::string& alert_type) = 0;
};

// Cognitive monitoring factory
std::unique_ptr<CognitiveMonitoring> create_cognitive_monitoring();

// Utility functions
namespace monitoring_utils {
    std::string alert_severity_to_string(const std::string& severity);
    std::string session_status_to_string(const std::string& status);
    std::string performance_rating_to_string(const std::string& rating);
    double calculate_efficiency_score(size_t cpu_time, size_t memory_used, size_t operations_completed);
    std::vector<std::string> get_default_alert_types();
    MonitoringThresholds get_default_thresholds(CognitiveTier tier);
    bool is_threshold_violation(double value, double threshold, double alert_percentage);
    std::string format_duration_ms(size_t duration_ms);
    std::string format_memory_size(size_t memory_mb);
    std::string generate_session_report(const MonitoringSession& session);
}

} // namespace t81::ai::cognitive
