#include "t81/canonfs/performance_cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        // Create CanonFS engine with performance monitoring
        auto engine = t81::canonfs::create_interchange_engine();
        auto cli = t81::canonfs::PerformanceCLI(engine);
        
        if (argc == 1) {
            // Interactive mode
            cli.run_interactive_mode();
        } else if (argc == 2) {
            std::string mode = argv[1];
            if (mode == "--monitor") {
                int interval = argc > 2 ? std::stoi(argv[2]) : 5;
                cli.run_monitoring_mode(interval);
            } else if (mode == "--analyze") {
                std::string output_file = argc > 2 ? argv[2] : "";
                cli.run_analysis_mode(output_file);
            } else if (mode == "--dashboard") {
                std::string output_file = argc > 2 ? argv[2] : "canonfs_performance_dashboard.html";
                cli.generate_performance_dashboard(output_file);
            } else if (mode == "--help") {
                std::cout << R"(
🚀 T81 CanonFS Performance Analysis Tool

USAGE:
    canonfs_performance [MODE] [OPTIONS]

MODES:
    (no args)              Interactive mode with menu
    --monitor [seconds]     Real-time performance monitoring
    --analyze [file]       Comprehensive performance analysis
    --dashboard [file]      Generate HTML performance dashboard
    --help                Show this help message

EXAMPLES:
    canonfs_performance                    # Interactive mode
    canonfs_performance --monitor 10          # Monitor every 10 seconds
    canonfs_performance --analyze report.txt     # Analysis to file
    canonfs_performance --dashboard dashboard.html  # Generate HTML dashboard

FEATURES:
    🔄 Real-time performance metrics
    📊 Comprehensive performance analysis
    💡 Optimization recommendations
    📋 HTML dashboard generation
    🚨 Performance alerting
    📈 Historical trend analysis
)";
            } else {
                std::cout << "❌ Invalid mode. Use --help for usage.\n";
                return 1;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
