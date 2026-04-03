#include "t81/canonfs/ml_optimizer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace t81::canonfs {

class MLOptimizationCLI {
public:
    MLOptimizationCLI(std::shared_ptr<PerformanceAnalyzer> analyzer);
    void run_interactive_mode();
    void run_training_mode(size_t data_points = 100);
    void run_prediction_mode();
    void run_continuous_learning();
    void generate_ml_report();

private:
    std::shared_ptr<CanonFSMLOptimizer> ml_optimizer_;
    
    void print_banner();
    void print_menu();
    void execute_command(const std::string& command);
    void show_model_status();
    void demonstrate_predictions();
    void show_training_progress();
};

} // namespace t81::canonfs
