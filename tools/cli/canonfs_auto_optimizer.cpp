#include <iostream>
#include <memory>
#include "t81/canonfs/performance_analyzer.hpp"
#include "t81/canonfs/auto_optimization_cli.hpp"

int main(int argc, char* argv[]) {
    try {
        // Create performance analyzer (simplified for demo)
        auto analyzer = std::make_shared<t81::canonfs::PerformanceAnalyzer>(nullptr);
        auto cli = t81::canonfs::AutoOptimizationCLI(analyzer);
        
        if (argc == 1) {
            // Interactive mode
            cli.run_interactive_mode();
        } else if (argc == 2) {
            std::string mode = argv[1];
            if (mode == "--auto") {
                cli.run_auto_mode();
            } else if (mode == "--manual") {
                cli.run_manual_mode();
            } else if (mode == "--status") {
                cli.show_optimization_status();
            } else if (mode == "--report") {
                cli.generate_optimization_report();
            } else if (mode == "--compare") {
                cli.show_performance_comparison();
            } else if (mode == "--help") {
                std::cout << R"(
🤖 T81 CanonFS Auto-Optimization System

USAGE:
    canonfs_auto_optimizer [MODE] [OPTIONS]

MODES:
    (no args)              Interactive mode with menu
    --auto                  Continuous auto-optimization mode
    --manual                Manual optimization mode
    --status                Show optimization status
    --report                Generate optimization report
    --compare               Performance comparison analysis
    --help                  Show this help message

FEATURES:
    🔄 Intelligent auto-optimization with effectiveness measurement
    🔧 Manual optimization application with rollback capability
    📊 Real-time performance monitoring and analysis
    📋 Comprehensive optimization reporting
    📈 Before/after performance comparison
    🎛 Adaptive optimization strategies based on system load

OPTIMIZATION STRATEGIES:
    1. Evidence Log Rotation - Automatic cleanup of old entries
    2. Policy Decision Caching - Cache frequently used policy decisions
    3. Parallel Processing - Enable concurrent CanonFS operations
    4. Memory Pool Management - Optimize memory allocation patterns
    5. Asynchronous Operations - Enable non-blocking operations
    6. Adaptive Throttling - Dynamic rate limiting based on load
    7. Predictive Caching - Pre-cache likely operations
    8. Bulk Operations - Batch processing for efficiency

EXAMPLES:
    canonfs_auto_optimizer                    # Interactive mode
    canonfs_auto_optimizer --auto              # Continuous auto-optimization
    canonfs_auto_optimizer --manual             # Manual optimization selection
    canonfs_auto_optimizer --report             # Generate detailed report
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
