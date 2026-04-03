#include "t81/canonfs/performance_analyzer.hpp"
#include "t81/canonfs/interchange_engine.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

namespace t81::canonfs {

class PerformanceCLI {
public:
    PerformanceCLI(CanonFSInterchangeEngine* engine);
    void run_interactive_mode();
    void run_analysis_mode(const std::string& output_file);
    void run_monitoring_mode(int interval_seconds);
    void generate_performance_dashboard(const std::string& output_file);

private:
    CanonFSInterchangeEngine* engine_;
    std::shared_ptr<PerformanceAnalyzer> analyzer_;
    
    void print_banner();
    void print_menu();
    void execute_command(const std::string& command);
    void show_real_time_metrics();
    void show_performance_analysis();
    void show_optimization_suggestions();
    void export_performance_report(const std::string& filename);
};

} // namespace t81::canonfs
