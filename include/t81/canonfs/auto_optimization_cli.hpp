#include "t81/canonfs/auto_optimizer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace t81::canonfs {

class AutoOptimizationCLI {
public:
    AutoOptimizationCLI(std::shared_ptr<PerformanceAnalyzer> analyzer);
    void run_interactive_mode();
    void run_auto_mode();
    void run_manual_mode();
    void generate_optimization_report();

private:
    std::shared_ptr<CanonFSAutoOptimizer> optimizer_;
    
    void print_banner();
    void print_menu();
    void execute_command(const std::string& command);
    void show_optimization_status();
    void list_available_optimizations();
    void apply_optimization_interactive();
    void show_performance_comparison();
};

} // namespace t81::canonfs
