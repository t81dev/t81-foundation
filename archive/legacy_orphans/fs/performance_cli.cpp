#include "t81/canonfs/performance_cli.hpp"
#include <thread>
#include <iomanip>
#include <sstream>

namespace t81::canonfs {

PerformanceCLI::PerformanceCLI(CanonFSInterchangeEngine* engine)
    : engine_(engine) {
    analyzer_ = std::make_shared<PerformanceAnalyzer>(nullptr);
}

void PerformanceCLI::print_banner() {
    std::cout << "\n";
    std::cout << "🚀 T81 CanonFS Performance Analysis Tool\n";
    std::cout << "=====================================\n";
    std::cout << "Advanced performance monitoring and optimization for CanonFS\n\n";
}

void PerformanceCLI::print_menu() {
    std::cout << "\n📊 Performance Analysis Options:\n\n";
    std::cout << "1. 🔄 Real-time Monitoring - Live performance metrics\n";
    std::cout << "2. 📈 Performance Analysis - Comprehensive analysis report\n";
    std::cout << "3. 💡 Optimization Suggestions - Performance improvement recommendations\n";
    std::cout << "4. 📋 Generate Dashboard - HTML performance dashboard\n";
    std::cout << "5. 📄 Export Report - Save analysis to file\n";
    std::cout << "6. 🚪 Exit - Quit application\n\n";
    std::cout << "Enter option (1-6): ";
}

void PerformanceCLI::execute_command(const std::string& command) {
    switch (command[0]) {
        case '1':
            run_monitoring_mode(5);
            break;
        case '2':
            show_performance_analysis();
            break;
        case '3':
            show_optimization_suggestions();
            break;
        case '4':
            generate_performance_dashboard("canonfs_performance_dashboard.html");
            std::cout << "✅ Dashboard generated: canonfs_performance_dashboard.html\n";
            break;
        case '5': {
            std::string filename;
            std::cout << "Enter output filename: ";
            std::getline(std::cin, filename);
            if (!filename.empty()) {
                export_performance_report(filename);
                std::cout << "✅ Report exported to: " << filename << "\n";
            }
            break;
        }
        case '6':
            std::cout << "👋 Exiting Performance Analysis Tool\n";
            return;
        default:
            std::cout << "❌ Invalid option. Please try again.\n";
            break;
    }
}

void PerformanceCLI::run_interactive_mode() {
    print_banner();
    
    while (true) {
        print_menu();
        std::string command;
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "6" || command == "quit" || command == "exit") {
            break;
        }
        
        execute_command(command);
    }
}

void PerformanceCLI::run_analysis_mode(const std::string& output_file) {
    print_banner();
    
    std::cout << "🔍 Generating comprehensive performance analysis...\n\n";
    
    auto analysis = analyzer_->analyze_current_performance();
    auto report = analyzer_->generate_performance_report();
    
    if (output_file.empty()) {
        std::cout << report;
    } else {
        std::ofstream out_file(output_file);
        if (out_file.is_open()) {
            out_file << report;
            out_file.close();
            std::cout << "✅ Analysis report saved to: " << output_file << "\n";
        } else {
            std::cout << "❌ Failed to create output file: " << output_file << "\n";
        }
    }
}

void PerformanceCLI::run_monitoring_mode(int interval_seconds) {
    print_banner();
    
    std::cout << "📊 Starting real-time performance monitoring...\n";
    std::cout << "Updating every " << interval_seconds << " seconds\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    while (true) {
        show_real_time_metrics();
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
}

void PerformanceCLI::show_real_time_metrics() {
    auto analysis = analyzer_->analyze_current_performance();
    
    // Clear screen and show header
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "📊 T81 CanonFS Real-time Performance Metrics\n";
    std::cout << "=====================================\n";
    std::cout << "Timestamp: " << analysis.timestamp << "\n\n";
    
    // Performance metrics
    std::cout << "🔄 Performance Indicators:\n";
    std::cout << "├─ Throughput: " << std::fixed << std::setprecision(2) 
              << analysis.metrics["operations_per_second"] << " ops/sec\n";
    std::cout << "├─ Avg Operation Time: " << analysis.metrics["average_operation_time_ms"] << " ms\n";
    std::cout << "├─ Policy Denial Rate: " << analysis.metrics["policy_denial_rate_percent"] << "%\n";
    std::cout << "├─ Evidence Log Size: " << analysis.metrics["evidence_log_size"] << " entries\n";
    std::cout << "└─ Memory Usage: " << analysis.metrics["memory_usage_mb"] << " MB\n\n";
    
    // Performance status
    std::cout << "📈 Overall Status: " << analysis.summary << "\n\n";
    
    // Alerts
    auto alerts = analyzer_->get_performance_alerts();
    if (!alerts.empty()) {
        std::cout << "🚨 Performance Alerts:\n";
        for (const auto& alert : alerts) {
            std::cout << "⚠️ " << alert << "\n";
        }
        std::cout << "\n";
    }
    
    // Optimization suggestions
    auto suggestions = analyzer_->get_optimization_suggestions();
    if (!suggestions.empty()) {
        std::cout << "💡 Quick Optimizations:\n";
        for (size_t i = 0; i < std::min(suggestions.size(), static_cast<size_t>(3)); ++i) {
            std::cout << "• " << suggestions[i] << "\n";
        }
        if (suggestions.size() > 3) {
            std::cout << "• ... and " << (suggestions.size() - 3) << " more\n";
        }
    }
    
    std::cout << "\nPress Enter to refresh, Ctrl+C to exit...\n";
    std::cin.get();
}

void PerformanceCLI::show_performance_analysis() {
    print_banner();
    
    std::cout << "🔍 Performing deep performance analysis...\n\n";
    
    auto analysis = analyzer_->analyze_current_performance();
    auto report = analyzer_->generate_performance_report();
    
    std::cout << report;
    
    auto suggestions = analyzer_->get_optimization_suggestions();
    if (!suggestions.empty()) {
        std::cout << "\n💡 Recommended Actions:\n";
        for (const auto& suggestion : suggestions) {
            std::cout << "🎯 " << suggestion << "\n";
        }
    }
}

void PerformanceCLI::show_optimization_suggestions() {
    print_banner();
    
    std::cout << "💡 Generating optimization recommendations...\n\n";
    
    auto suggestions = analyzer_->get_optimization_suggestions();
    
    std::cout << "🚀 Performance Optimization Strategies:\n\n";
    for (size_t i = 0; i < suggestions.size(); ++i) {
        const auto& strategy = suggestions[i];
        std::cout << (i + 1) << ". " << strategy.name << "\n";
        std::cout << "   " << strategy.description << "\n";
        std::cout << "   Expected Improvement: " << strategy.expected_improvement << "%\n";
        std::cout << "   Complexity: " << strategy.implementation_complexity << "\n\n";
    }
}

void PerformanceCLI::generate_performance_dashboard(const std::string& output_file) {
    print_banner();
    
    std::cout << "📋 Generating HTML performance dashboard...\n\n";
    
    auto analysis = analyzer_->analyze_current_performance();
    auto suggestions = analyzer_->get_optimization_suggestions();
    auto alerts = analyzer_->get_performance_alerts();
    
    std::ofstream dashboard(output_file);
    if (!dashboard.is_open()) {
        std::cout << "❌ Failed to create dashboard file: " << output_file << "\n";
        return;
    }
    
    // HTML Dashboard
    dashboard << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>T81 CanonFS Performance Dashboard</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Fira Sans', 'Droid Sans', 'Helvetica Neue', Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
        .dashboard { max-width: 1200px; margin: 0 auto; background: white; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 10px 10px 0 0; text-align: center; }
        .metrics-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 20px; }
        .metric-card { background: white; border-radius: 8px; padding: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .metric-title { font-size: 18px; font-weight: 600; margin-bottom: 10px; color: #333; }
        .metric-value { font-size: 24px; font-weight: 700; color: #667eea; margin-bottom: 5px; }
        .metric-label { font-size: 14px; color: #666; }
        .status-optimal { color: #10b981; }
        .status-warning { color: #f59e0b; }
        .status-critical { color: #ef4444; }
        .alerts { background: #fff3cd; border-radius: 8px; padding: 20px; margin-top: 20px; }
        .alert { background: #fee; border-left: 4px solid #dc3545; padding: 10px; margin-bottom: 10px; border-radius: 4px; }
        .suggestions { background: #e8f5e8; border-radius: 8px; padding: 20px; }
        .suggestion { background: white; border-left: 4px solid #10b981; padding: 15px; margin-bottom: 10px; border-radius: 4px; }
    </style>
</head>
<body>
    <div class="dashboard">
        <div class="header">
            <h1>🚀 T81 CanonFS Performance Dashboard</h1>
            <p>Real-time performance monitoring and optimization for CanonFS interchange operations</p>
        </div>)";
    
    // Metrics Section
    dashboard << R"(
        <div class="metrics-grid">
            <div class="metric-card">
                <div class="metric-title">🔄 Throughput</div>
                <div class="metric-value">)" << analysis.metrics["operations_per_second"] << R"( ops/sec</div>
                <div class="metric-label">Operations per second</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">⏱️ Avg Response Time</div>
                <div class="metric-value">)" << analysis.metrics["average_operation_time_ms"] << R"( ms</div>
                <div class="metric-label">Average operation time</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">🛡️ Policy Denial Rate</div>
                <div class="metric-value">)" << analysis.metrics["policy_denial_rate_percent"] << R"(%</div>
                <div class="metric-label">Policy decisions denied</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">📋 Evidence Log Size</div>
                <div class="metric-value">)" << analysis.metrics["evidence_log_size"] << R"( entries</div>
                <div class="metric-label">Evidence log entries</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">💾 Memory Usage</div>
                <div class="metric-value">)" << analysis.metrics["memory_usage_mb"] << R"( MB</div>
                <div class="metric-label">Memory consumption</div>
            </div>
        </div>)";
    
    // Status Section
    std::string status_class = "status-optimal";
    if (analysis.summary.find("DEGRADED") != std::string::npos) {
        status_class = "status-critical";
    } else if (analysis.summary.find("SUBOPTIMAL") != std::string::npos) {
        status_class = "status-warning";
    }
    
    dashboard << R"(
        <div class="metric-card" style="grid-column: 1 / -1;">
            <div class="metric-title">📊 Overall Status</div>
            <div class="metric-value )" << status_class << R"()">)" << analysis.summary << R"(</div>
            <div class="metric-label">System performance status</div>
        </div>
    </div>)";
    
    // Alerts Section
    if (!alerts.empty()) {
        dashboard << R"(
        <div class="alerts">
            <h3>🚨 Performance Alerts</h3>)";
        for (const auto& alert : alerts) {
            dashboard << R"(<div class="alert">)" << alert << R"(</div>)";
        }
        dashboard << R"(</div>)";
    }
    
    // Suggestions Section
    if (!suggestions.empty()) {
        dashboard << R"(
        <div class="suggestions">
            <h3>💡 Optimization Suggestions</h3>)";
        for (const auto& suggestion : suggestions) {
            dashboard << R"(<div class="suggestion">)";
            dashboard << R"(<strong>)" << suggestion.name << R"(</strong><br>)";
            dashboard << R"(Expected improvement: )" << suggestion.expected_improvement << R"(%<br>)";
            dashboard << R"(Complexity: )" << suggestion.implementation_complexity << R"(</div>)";
        }
        dashboard << R"(</div>)";
    }
    
    dashboard << R"(
    </div>
    
    <script>
        // Auto-refresh every 30 seconds
        setTimeout(() => {
            location.reload();
        }, 30000);
    </script>
</body>
</html>)";
    
    dashboard.close();
    std::cout << "✅ Performance dashboard generated: " << output_file << "\n";
    std::cout << "🌐 Open dashboard: file://" << output_file << "\n";
}

} // namespace t81::canonfs
