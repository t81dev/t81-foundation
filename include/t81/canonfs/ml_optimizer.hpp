#pragma once

#include "t81/canonfs/performance_analyzer.hpp"
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <functional>

namespace t81::canonfs {

// ML Model Types
enum class MLModelType {
    LINEAR_REGRESSION,        // Predict performance based on linear relationships
    DECISION_TREE,          // Tree-based decision making for optimizations
    NEURAL_NETWORK,          // Deep learning for complex patterns
    ENSEMBLE,               // Combine multiple models for better predictions
    REINFORCEMENT_LEARNING   // Learn from optimization outcomes
};

// Performance features for ML
struct PerformanceFeatures {
    double throughput_ops_per_sec;
    double avg_latency_ms;
    double memory_usage_mb;
    double policy_denial_rate;
    size_t evidence_log_size;
    double cpu_utilization;
    double io_wait_time;
    std::chrono::steady_clock::time_point timestamp;
    
    // Feature engineering
    double throughput_trend;           // Rate of change in throughput
    double latency_variance;           // Variability in response times
    double memory_growth_rate;        // Rate of memory usage growth
    double policy_compliance_score;    // Overall policy health metric
    double system_load_factor;        // Current system load (0-1)
    bool is_peak_hour;              // Time-based feature
    bool is_weekend;                // Time-based feature
};

// ML Prediction result
struct MLPrediction {
    std::map<std::string, double> optimization_weights;
    std::string recommended_strategy;
    double confidence_score;
    double expected_improvement;
    std::vector<std::string> reasoning;
};

// ML Model interface
class MLModel {
public:
    virtual ~MLModel() = default;
    
    virtual void train(const std::vector<PerformanceFeatures>& features,
                    const std::vector<std::string>& outcomes) = 0;
    virtual MLPrediction predict(const PerformanceFeatures& current_features) = 0;
    virtual void update_model(const PerformanceFeatures& features, 
                         const std::string& outcome, double effectiveness) = 0;
    virtual std::string get_model_info() const = 0;
    virtual double get_accuracy() const = 0;
};

// Advanced ML optimization engine
class CanonFSMLOptimizer {
public:
    CanonFSMLOptimizer(std::shared_ptr<PerformanceAnalyzer> analyzer);
    ~CanonFSMLOptimizer();
    
    // ML Model Management
    void initialize_models();
    void train_models(size_t historical_data_points = 100);
    void enable_continuous_learning(bool enable = true);
    
    // Prediction and Optimization
    MLPrediction predict_optimal_strategy();
    std::vector<std::string> get_ranked_optimizations();
    bool apply_ml_recommended_strategy();
    
    // Model Evaluation
    double evaluate_model_performance();
    std::string generate_ml_report();
    void save_model_state();
    void load_model_state();

private:
    std::shared_ptr<PerformanceAnalyzer> analyzer_;
    std::vector<std::unique_ptr<MLModel>> models_;
    std::vector<PerformanceFeatures> historical_features_;
    std::vector<std::string> historical_outcomes_;
    std::vector<double> historical_effectiveness_;
    
    bool continuous_learning_enabled_ = true;
    
    // Feature Engineering
    PerformanceFeatures extract_features(const std::map<std::string, double>& metrics);
    std::vector<std::string> generate_feature_importance();
    
    // Model Creation
    std::unique_ptr<MLModel> create_linear_regression_model();
    std::unique_ptr<MLModel> create_decision_tree_model();
    std::unique_ptr<MLModel> create_neural_network_model();
    std::unique_ptr<MLModel> create_ensemble_model();
    
    // Learning Loop
    void continuous_learning_loop();
    void collect_training_data();
    void update_all_models(const PerformanceFeatures& features, 
                         const std::string& outcome, double effectiveness);
};

// Reinforcement Learning for optimization
class OptimizationRLAgent {
public:
    OptimizationRLAgent();
    
    // RL State and Actions
    struct State {
        std::map<std::string, double> current_metrics;
        std::vector<std::string> applied_optimizations;
        double recent_performance_score;
    };
    
    struct Action {
        std::string optimization_strategy;
        std::map<std::string, double> expected_effects;
    };
    
    // RL Methods
    void update_state(const State& new_state);
    Action select_best_action(const State& current_state);
    double calculate_reward(const State& previous_state, const State& current_state);
    void update_q_table(const State& state, const Action& action, double reward);
    Action get_epsilon_greedy_action(const State& state, double epsilon = 0.1);
    
private:
    std::map<std::vector<double>, std::map<std::string, double>> q_table_;
    double learning_rate_ = 0.1;
    double discount_factor_ = 0.95;
    double epsilon_ = 0.1; // Exploration rate
    
    State discretize_state(const std::map<std::string, double>& metrics);
    std::vector<double> encode_state(const State& state);
    std::string decode_action(const Action& action);
};

} // namespace t81::canonfs
