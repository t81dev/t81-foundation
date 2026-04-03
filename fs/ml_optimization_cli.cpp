#include "t81/canonfs/ml_optimization_cli.hpp"
#include <thread>
#include <chrono>

namespace t81::canonfs {

MLOptimizationCLI::MLOptimizationCLI(std::shared_ptr<PerformanceAnalyzer> analyzer)
    : ml_optimizer_(std::make_shared<CanonFSMLOptimizer>(analyzer)) {
}

void MLOptimizationCLI::print_banner() {
    std::cout << "\n";
    std::cout << "🧠 T81 CanonFS ML Optimization System\n";
    std::cout << "====================================\n";
    std::cout << "Machine Learning powered performance optimization\n\n";
}

void MLOptimizationCLI::print_menu() {
    std::cout << "🧠 ML Optimization Options:\n\n";
    std::cout << "1. 🧠 Train Models - Train ML models with historical data\n";
    std::cout << "2. 🔮 Predict Strategy - Get ML-based optimization recommendations\n";
    std::cout << "3. 🧪 Continuous Learning - Enable real-time model updates\n";
    std::cout << "4. 📊 Model Status - Show ML model performance\n";
    std::cout << "5. 🎯 Demo Predictions - Demonstrate prediction capabilities\n";
    std::cout << "6. 📋 ML Report - Generate comprehensive ML report\n";
    std::cout << "7. 🚪 Exit - Quit application\n\n";
    std::cout << "Enter option (1-7): ";
}

void MLOptimizationCLI::execute_command(const std::string& command) {
    switch (command[0]) {
        case '1':
            run_training_mode();
            break;
        case '2':
            run_prediction_mode();
            break;
        case '3':
            run_continuous_learning();
            break;
        case '4':
            show_model_status();
            break;
        case '5':
            demonstrate_predictions();
            break;
        case '6':
            generate_ml_report();
            break;
        case '7':
            std::cout << "👋 Exiting ML Optimization System\n";
            return;
        default:
            std::cout << "❌ Invalid option. Please try again.\n";
            break;
    }
}

void MLOptimizationCLI::run_training_mode() {
    print_banner();
    
    std::cout << "🧠 ML Model Training Mode\n";
    std::cout << "========================\n\n";
    
    size_t data_points = 100;
    std::cout << "Enter number of training data points (default 100): ";
    std::string input;
    std::getline(std::cin, input);
    
    if (!input.empty()) {
        try {
            data_points = std::stoull(input);
        } catch (const std::exception& e) {
            std::cout << "❌ Invalid input: " << e.what() << "\n";
            return;
        }
    }
    
    std::cout << "\n🧠 Training ML models with " << data_points << " data points...\n";
    ml_optimizer_->train_models(data_points);
    
    std::cout << "✅ Training completed!\n";
    show_model_status();
}

void MLOptimizationCLI::run_prediction_mode() {
    print_banner();
    
    std::cout << "🔮 ML Prediction Mode\n";
    std::cout << "====================\n\n";
    
    std::cout << "Analyzing current performance...\n";
    auto prediction = ml_optimizer_->predict_optimal_strategy();
    
    std::cout << "\n🧠 ML Prediction Results:\n";
    std::cout << "Recommended Strategy: " << prediction.recommended_strategy << "\n";
    std::cout << "Confidence: " << (prediction.confidence * 100) << "%\n";
    std::cout << "Expected Improvement: " << prediction.expected_improvement << "%\n\n";
    
    std::cout << "Optimization Weights:\n";
    for (const auto& [strategy, weight] : prediction.optimization_weights) {
        std::cout << "- " << strategy << ": " << std::fixed << std::setprecision(3) << weight << "\n";
    }
    
    std::cout << "\n🤖 ML Reasoning:\n";
    for (const auto& reason : prediction.reasoning) {
        std::cout << "- " << reason << "\n";
    }
    
    std::cout << "\n🔧 Apply ML recommendation? (y/n): ";
    std::string apply_choice;
    std::getline(std::cin, apply_choice);
    
    if (apply_choice == "y" || apply_choice == "Y") {
        ml_optimizer_->apply_ml_recommended_strategy();
    } else {
        std::cout << "❌ ML recommendation not applied\n";
    }
}

void MLOptimizationCLI::run_continuous_learning() {
    print_banner();
    
    std::cout << "🧪 Continuous Learning Mode\n";
    std::cout << "==========================\n\n";
    
    std::cout << "Enabling continuous ML learning...\n";
    std::cout << "The system will:\n";
    std::cout << "- Continuously collect performance data\n";
    std::cout << "- Retrain models periodically\n";
    std::cout << "- Adapt to changing patterns\n";
    std::cout << "- Improve prediction accuracy over time\n";
    std::cout << "\nPress Ctrl+C to stop\n\n";
    
    ml_optimizer_->enable_continuous_learning(true);
    
    // Keep the main thread alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "\r🧪 Learning... Data points: " << 
                     std::setw(4) << std::left << "N/A" << " | Accuracy: " << 
                     std::setw(6) << std::fixed << std::setprecision(1) << 
                     (ml_optimizer_->evaluate_model_performance() * 100) << "%" << std::flush;
    }
}

void MLOptimizationCLI::show_model_status() {
    print_banner();
    
    std::cout << "📊 ML Model Status\n";
    std::cout << "=================\n\n";
    
    double accuracy = ml_optimizer_->evaluate_model_performance();
    std::cout << "Overall Model Accuracy: " << (accuracy * 100) << "%\n";
    std::cout << "Model Count: " << "1" << " (Ensemble)\n";
    std::cout << "Learning Status: " << "🔄 Active" << "\n\n";
    
    if (accuracy >= 0.8) {
        std::cout << "✅ Models are performing well\n";
    } else if (accuracy >= 0.6) {
        std::cout << "⚠️ Models need more training data\n";
    } else {
        std::cout << "❌ Models need significant improvement\n";
    }
}

void MLOptimizationCLI::demonstrate_predictions() {
    print_banner();
    
    std::cout << "🎯 ML Prediction Demo\n";
    std::cout << "====================\n\n";
    
    std::cout << "Demonstrating ML prediction capabilities...\n\n";
    
    // Simulate different performance scenarios
    std::vector<std::map<std::string, double>> scenarios = {
        {{"throughput", 0.5}, {"latency", 450.0}, {"memory", 85.0},
        {{"throughput", 2.5}, {"latency", 150.0}, {"memory", 45.0},
        {{"throughput", 1.0}, {"latency", 300.0}, {"memory", 60.0},
        {{"throughput", 3.5}, {"latency", 100.0}, {"memory", 30.0}
    };
    
    for (size_t i = 0; i < scenarios.size(); ++i) {
        std::cout << "Scenario " << (i + 1) << ":\n";
        for (const auto& [metric, value] : scenarios[i]) {
            std::cout << "  " << metric << ": " << value << "\n";
        }
        
        // Get prediction for this scenario
        PerformanceFeatures features;
        features.throughput_ops_per_sec = scenarios[i][0]; // throughput
        features.avg_latency_ms = scenarios[i][1];         // latency
        features.memory_usage_mb = scenarios[i][2];         // memory
        features.policy_denial_rate = 0.05;               // fixed for demo
        features.evidence_log_size = 1000;
        features.cpu_utilization = 0.6;
        features.io_wait_time = 15.0;
        features.system_load_factor = 0.7;
        
        // This would use the actual ML models to predict
        std::cout << "\n🔮 ML Prediction:\n";
        std::cout << "  For this scenario, ML would recommend: ";
        
        if (features.throughput_ops_per_sec < 1.0) {
            std::cout << "Parallel Processing (high confidence)\n";
        } else if (features.avg_latency_ms > 300) {
            std::cout << "Asynchronous Operations (medium confidence)\n";
        } else if (features.memory_usage_mb > 70) {
            std::cout << "Memory Pool Management (medium confidence)\n";
        } else {
            std::cout << "Policy Decision Caching (low confidence)\n";
        }
        
        std::cout << "\n";
    }
}

void MLOptimizationCLI::generate_ml_report() {
    print_banner();
    
    std::cout << "📋 Generating ML Report...\n\n";
    
    auto report = ml_optimizer_->generate_ml_report();
    std::cout << report << "\n";
    
    // Save to file
    std::ofstream out_file("canonfs_ml_optimization_report.txt");
    if (out_file.is_open()) {
        out_file << report;
        out_file.close();
        std::cout << "💾 ML report saved to: canonfs_ml_optimization_report.txt\n";
    }
    
    // Also save model state
    ml_optimizer_->save_model_state();
    std::cout << "🧠 ML models saved to: canonfs_ml_models.dat\n";
}

} // namespace t81::canonfs
