#include <iostream>
#include <memory>
#include "t81/canonfs/performance_analyzer.hpp"
#include "t81/canonfs/ml_optimization_cli.hpp"

int main(int argc, char* argv[]) {
    try {
        // Create performance analyzer (simplified for demo)
        auto analyzer = std::make_shared<t81::canonfs::SimplePerformanceAnalyzer>();
        auto cli = t81::canonfs::MLOptimizationCLI(analyzer);
        
        if (argc == 1) {
            // Interactive mode
            cli.run_interactive_mode();
        } else if (argc == 2) {
            std::string mode = argv[1];
            if (mode == "--train") {
                cli.run_training_mode();
            } else if (mode == "--predict") {
                cli.run_prediction_mode();
            } else if (mode == "--learn") {
                cli.run_continuous_learning();
            } else if (mode == "--status") {
                cli.show_model_status();
            } else if (mode == "--demo") {
                cli.demonstrate_predictions();
            } else if (mode == "--report") {
                cli.generate_ml_report();
            } else if (mode == "--help") {
                std::cout << R"(
🧠 T81 CanonFS ML Optimization System

USAGE:
    canonfs_ml_optimizer [MODE] [OPTIONS]

MODES:
    (no args)              Interactive mode with menu
    --train [points]        Train ML models with historical data
    --predict               Get ML-based optimization recommendations
    --learn                 Enable continuous real-time learning
    --status                Show ML model performance status
    --demo                  Demonstrate ML prediction capabilities
    --report                Generate comprehensive ML report
    --help                  Show this help message

FEATURES:
    🧠 Machine Learning Models: Linear Regression, Decision Trees, Neural Networks, Ensembles
    🔮 Intelligent Predictions: Data-driven optimization recommendations
    🧪 Continuous Learning: Real-time model adaptation and improvement
    📊 Model Evaluation: Accuracy tracking and performance metrics
    🎯 Reinforcement Learning: Learn from optimization outcomes

ML MODELS:
    1. Linear Regression - Predicts optimization effectiveness
    2. Decision Tree - Rule-based optimization selection
    3. Neural Network - Complex pattern recognition
    4. Ensemble Model - Combines multiple models for best accuracy

EXAMPLES:
    canonfs_ml_optimizer                    # Interactive mode
    canonfs_ml_optimizer --train 200           # Train with 200 data points
    canonfs_ml_optimizer --predict              # Get optimization recommendations
    canonfs_ml_optimizer --learn                 # Enable continuous learning
    canonfs_ml_optimizer --status                # Show model performance
    canonfs_ml_optimizer --demo                  # Demonstrate predictions
    canonfs_ml_optimizer --report               # Generate ML report

ADVANCED FEATURES:
    - Feature engineering for optimal predictions
    - Model persistence and loading
    - Real-time performance monitoring integration
    - Automatic model retraining
    - Confidence scoring for predictions
    - Multi-model ensemble predictions
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
