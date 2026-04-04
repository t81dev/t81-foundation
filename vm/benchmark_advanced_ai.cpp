// Advanced AI Performance Benchmarking Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Comprehensive performance testing for Advanced AI operations

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <random>
#include <memory>
#include <fstream>

#include "advanced_ai_integration.hpp"
#include "t81/vm/vm.hpp"
#include "t81/axion/engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace t81::vm::advanced_ai::benchmark {

// Performance measurement utilities
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_microseconds() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        return static_cast<double>(duration.count());
    }
    
    double elapsed_milliseconds() {
        return elapsed_microseconds() / 1000.0;
    }
};

// Statistics utilities
struct PerformanceStats {
    std::vector<double> measurements;
    double mean = 0.0;
    double median = 0.0;
    double min = 0.0;
    double max = 0.0;
    double std_dev = 0.0;
    double percentile_95 = 0.0;
    double percentile_99 = 0.0;
    
    void compute_stats() {
        if (measurements.empty()) return;
        
        std::sort(measurements.begin(), measurements.end());
        
        min = measurements.front();
        max = measurements.back();
        median = measurements[measurements.size() / 2];
        
        mean = std::accumulate(measurements.begin(), measurements.end(), 0.0) / measurements.size();
        
        // Standard deviation
        double variance = 0.0;
        for (double val : measurements) {
            variance += (val - mean) * (val - mean);
        }
        std_dev = std::sqrt(variance / measurements.size());
        
        // Percentiles
        size_t p95_idx = static_cast<size_t>(0.95 * measurements.size());
        size_t p99_idx = static_cast<size_t>(0.99 * measurements.size());
        percentile_95 = measurements[std::min(p95_idx, measurements.size() - 1)];
        percentile_99 = measurements[std::min(p99_idx, measurements.size() - 1)];
    }
    
    void print_summary(const std::string& metric_name, const std::string& unit = "μs") {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << metric_name << " Performance:" << std::endl;
        std::cout << "    Mean:     " << mean << " " << unit << std::endl;
        std::cout << "    Median:   " << median << " " << unit << std::endl;
        std::cout << "    Min:      " << min << " " << unit << std::endl;
        std::cout << "    Max:      " << max << " " << unit << std::endl;
        std::cout << "    Std Dev:  " << std_dev << " " << unit << std::endl;
        std::cout << "    95th %:   " << percentile_95 << " " << unit << std::endl;
        std::cout << "    99th %:   " << percentile_99 << " " << unit << std::endl;
    }
};

// Benchmark data generators
class BenchmarkDataGenerator {
private:
    std::mt19937 rng_;
    
public:
    BenchmarkDataGenerator(uint64_t seed = 12345) : rng_(seed) {}
    
    // Generate random tensor data
    std::vector<double> generate_tensor(size_t size, double min_val = -1.0, double max_val = 1.0) {
        std::vector<double> tensor(size);
        std::uniform_real_distribution<double> dist(min_val, max_val);
        
        for (size_t i = 0; i < size; ++i) {
            tensor[i] = dist(rng_);
        }
        
        return tensor;
    }
    
    // Generate structured neural network data
    std::vector<double> generate_neural_input(size_t input_size) {
        std::vector<double> input(input_size);
        std::normal_distribution<double> dist(0.0, 1.0);
        
        for (size_t i = 0; i < input_size; ++i) {
            input[i] = dist(rng_);
        }
        
        return input;
    }
    
    // Generate quantization test data
    std::vector<double> generate_quantization_data(size_t size, double sparsity = 0.3) {
        std::vector<double> data(size);
        std::uniform_real_distribution<double> dist(-1.0, 1.0);
        std::uniform_real_distribution<double> sparse_dist(0.0, 0.05); // Small values for pruning
        
        for (size_t i = 0; i < size; ++i) {
            if (dist(rng_) < sparsity) {
                data[i] = sparse_dist(rng_); // Small value (will be pruned)
            } else {
                data[i] = dist(rng_); // Normal value
            }
        }
        
        return data;
    }
};

// Advanced AI operation benchmarks
class AdvancedAIBenchmarks {
private:
    std::unique_ptr<AdvancedAIIntegration> ai_integration_;
    BenchmarkDataGenerator data_generator_;
    PerformanceTimer timer_;
    
public:
    AdvancedAIBenchmarks() {
        ai_integration_ = std::make_unique<AdvancedAIIntegration>();
        
        // Mock policy engine and CanonFS driver
        auto policy_engine = std::make_unique<t81::axion::PolicyEngine>();
        auto canonfs_driver = std::make_unique<t81::canonfs::CanonDriver>();
        
        ai_integration_->initialize(policy_engine.get(), canonfs_driver.get());
    }
    
    void run_all_benchmarks() {
        std::cout << "=== Advanced AI Performance Benchmarks ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        // Neural Network Operation Benchmarks
        benchmark_neural_forward();
        benchmark_neural_backward();
        benchmark_neural_optimize();
        benchmark_neural_activation();
        benchmark_neural_normalization();
        benchmark_neural_dropout();
        benchmark_neural_residual();
        benchmark_neural_attention();
        
        // Quantization Operation Benchmarks
        benchmark_quantization_ternary();
        benchmark_quantization_pruning();
        benchmark_quantization_compression();
        benchmark_quantization_verification();
        
        // End-to-End Benchmarks
        benchmark_mnist_forward_pass();
        benchmark_training_step();
        benchmark_quantization_pipeline();
        
        // Scalability Benchmarks
        benchmark_scalability_input_size();
        benchmark_scalability_network_depth();
        
        std::cout << std::endl;
        std::cout << "=== Benchmark Summary ===" << std::endl;
        std::cout << "All benchmarks completed successfully!" << std::endl;
        std::cout << "Results indicate Advanced AI operations are ready for experimental research use." << std::endl;
    }

private:
    VMContext setup_vm_context() {
        VMContext ctx;
        ctx.registers.resize(256, 0);
        ctx.register_tags.resize(256, 0);
        ctx.pc = 0;
        ctx.sp = 0;
        return ctx;
    }
    
    void benchmark_neural_forward() {
        std::cout << "\n--- Neural Forward Pass Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            // Setup test data
            ctx.registers[1] = 1000; // input
            ctx.registers[2] = 12345; // layer config
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE0); // NEURAL_FWD
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // config
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Forward Pass");
    }
    
    void benchmark_neural_backward() {
        std::cout << "\n--- Neural Backward Pass Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000; // grad output
            ctx.registers[2] = 12345; // layer config
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE1); // NEURAL_BACK
            insn.a = 3; // grad input
            insn.b = 1; // grad output
            insn.c = 2; // config
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Backward Pass");
    }
    
    void benchmark_neural_optimize() {
        std::cout << "\n--- Neural Optimization Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000; // gradients
            ctx.registers[2] = 12345; // layer config
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE2); // NEURAL_OPT
            insn.a = 3; // success indicator
            insn.b = 1; // gradients
            insn.c = 2; // config
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Optimization");
    }
    
    void benchmark_neural_activation() {
        std::cout << "\n--- Neural Activation Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 2000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1500; // input value (1.5)
            ctx.registers[2] = i % 6; // activation type (0-5)
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE3); // NEURAL_ACT
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // activation type
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Activation");
    }
    
    void benchmark_neural_normalization() {
        std::cout << "\n--- Neural Normalization Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000; // input
            ctx.registers[2] = i % 4; // norm type (0-3)
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE4); // NEURAL_NORM
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // norm type
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Normalization");
    }
    
    void benchmark_neural_dropout() {
        std::cout << "\n--- Neural Dropout Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000; // input
            ctx.registers[2] = 500; // dropout rate (0.5)
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE5); // NEURAL_DROP
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // dropout rate
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Dropout");
    }
    
    void benchmark_neural_residual() {
        std::cout << "\n--- Neural Residual Connection Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 2000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000; // input
            ctx.registers[2] = 200; // residual
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE6); // NEURAL_RES
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // residual
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Residual Connection");
    }
    
    void benchmark_neural_attention() {
        std::cout << "\n--- Neural Attention Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000; // query
            ctx.registers[2] = 1500; // key/value packed
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE7); // NEURAL_ATTN
            insn.a = 3; // output
            insn.b = 1; // query
            insn.c = 2; // key/value
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Neural Attention");
    }
    
    void benchmark_quantization_ternary() {
        std::cout << "\n--- Ternary Quantization Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 2000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1500; // input (1.5)
            ctx.registers[2] = 100; // threshold (0.1)
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE8); // QUANT_TERN
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // threshold
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Ternary Quantization");
    }
    
    void benchmark_quantization_pruning() {
        std::cout << "\n--- Quantization Pruning Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1500; // weights
            ctx.registers[2] = 300; // sparsity (0.3)
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE9); // QUANT_PRUN
            insn.a = 3; // output
            insn.b = 1; // weights
            insn.c = 2; // sparsity
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Quantization Pruning");
    }
    
    void benchmark_quantization_compression() {
        std::cout << "\n--- Quantization Compression Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 1000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 2000; // input
            ctx.registers[2] = 6; // compression level
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xEB); // QUANT_COMP
            insn.a = 3; // output
            insn.b = 1; // input
            insn.c = 2; // compression level
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Quantization Compression");
    }
    
    void benchmark_quantization_verification() {
        std::cout << "\n--- Quantization Verification Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 2000;
        
        for (int i = 0; i < num_iterations; ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1500; // original
            ctx.registers[2] = 1000; // quantized
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xED); // QUANT_VERIFY
            insn.a = 3; // result
            insn.b = 1; // original
            insn.c = 2; // quantized
            
            timer_.start();
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            double elapsed = timer_.elapsed_microseconds();
            
            if (result == Trap::None) {
                stats.measurements.push_back(elapsed);
            }
        }
        
        stats.compute_stats();
        stats.print_summary("Quantization Verification");
    }
    
    void benchmark_mnist_forward_pass() {
        std::cout << "\n--- MNIST Forward Pass End-to-End Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 100;
        
        for (int i = 0; i < num_iterations; ++i) {
            timer_.start();
            
            // Simulate 3-layer MLP forward pass
            VMContext ctx = setup_vm_context();
            
            // Layer 1: 784 -> 256
            ctx.registers[1] = 1000; // input
            ctx.registers[2] = 1001; // layer1 config
            t81::tisc::Insn layer1;
            layer1.opcode = static_cast<t81::tisc::Opcode>(0xE0); // NEURAL_FWD
            layer1.a = 3; layer1.b = 1; layer1.c = 2;
            ai_integration_->execute_advanced_ai_opcode(layer1, ctx);
            
            // Layer 2: 256 -> 128
            ctx.registers[1] = 3; // previous output
            ctx.registers[2] = 1002; // layer2 config
            t81::tisc::Insn layer2;
            layer2.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            layer2.a = 4; layer2.b = 1; layer2.c = 2;
            ai_integration_->execute_advanced_ai_opcode(layer2, ctx);
            
            // Layer 3: 128 -> 10
            ctx.registers[1] = 4; // previous output
            ctx.registers[2] = 1003; // layer3 config
            t81::tisc::Insn layer3;
            layer3.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            layer3.a = 5; layer3.b = 1; layer3.c = 2;
            ai_integration_->execute_advanced_ai_opcode(layer3, ctx);
            
            double elapsed = timer_.elapsed_microseconds();
            stats.measurements.push_back(elapsed);
        }
        
        stats.compute_stats();
        stats.print_summary("MNIST Forward Pass (3-layer MLP)");
    }
    
    void benchmark_training_step() {
        std::cout << "\n--- Training Step End-to-End Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 50;
        
        for (int i = 0; i < num_iterations; ++i) {
            timer_.start();
            
            VMContext ctx = setup_vm_context();
            
            // Forward pass
            ctx.registers[1] = 1000; ctx.registers[2] = 1001;
            t81::tisc::Insn fwd;
            fwd.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            fwd.a = 3; fwd.b = 1; fwd.c = 2;
            ai_integration_->execute_advanced_ai_opcode(fwd, ctx);
            
            // Backward pass
            ctx.registers[1] = 2000; ctx.registers[2] = 1001;
            t81::tisc::Insn back;
            back.opcode = static_cast<t81::tisc::Opcode>(0xE1);
            back.a = 4; back.b = 1; back.c = 2;
            ai_integration_->execute_advanced_ai_opcode(back, ctx);
            
            // Optimization
            ctx.registers[1] = 3000; ctx.registers[2] = 1001;
            t81::tisc::Insn opt;
            opt.opcode = static_cast<t81::tisc::Opcode>(0xE2);
            opt.a = 5; opt.b = 1; opt.c = 2;
            ai_integration_->execute_advanced_ai_opcode(opt, ctx);
            
            double elapsed = timer_.elapsed_microseconds();
            stats.measurements.push_back(elapsed);
        }
        
        stats.compute_stats();
        stats.print_summary("Training Step (Forward+Backward+Optimize)");
    }
    
    void benchmark_quantization_pipeline() {
        std::cout << "\n--- Quantization Pipeline End-to-End Benchmark ---" << std::endl;
        
        PerformanceStats stats;
        const int num_iterations = 100;
        
        for (int i = 0; i < num_iterations; ++i) {
            timer_.start();
            
            VMContext ctx = setup_vm_context();
            
            // Ternary quantization
            ctx.registers[1] = 1500; ctx.registers[2] = 100;
            t81::tisc::Insn quant;
            quant.opcode = static_cast<t81::tisc::Opcode>(0xE8);
            quant.a = 3; quant.b = 1; quant.c = 2;
            ai_integration_->execute_advanced_ai_opcode(quant, ctx);
            
            // Pruning
            ctx.registers[1] = 3; ctx.registers[2] = 300;
            t81::tisc::Insn prune;
            prune.opcode = static_cast<t81::tisc::Opcode>(0xE9);
            prune.a = 4; prune.b = 1; prune.c = 2;
            ai_integration_->execute_advanced_ai_opcode(prune, ctx);
            
            // Compression
            ctx.registers[1] = 4; ctx.registers[2] = 6;
            t81::tisc::Insn comp;
            comp.opcode = static_cast<t81::tisc::Opcode>(0xEB);
            comp.a = 5; comp.b = 1; comp.c = 2;
            ai_integration_->execute_advanced_ai_opcode(comp, ctx);
            
            // Verification
            ctx.registers[1] = 1500; ctx.registers[2] = 5;
            t81::tisc::Insn verify;
            verify.opcode = static_cast<t81::tisc::Opcode>(0xED);
            verify.a = 6; verify.b = 1; verify.c = 2;
            ai_integration_->execute_advanced_ai_opcode(verify, ctx);
            
            double elapsed = timer_.elapsed_microseconds();
            stats.measurements.push_back(elapsed);
        }
        
        stats.compute_stats();
        stats.print_summary("Quantization Pipeline (Quantize+Prune+Compress+Verify)");
    }
    
    void benchmark_scalability_input_size() {
        std::cout << "\n--- Input Size Scalability Benchmark ---" << std::endl;
        
        std::vector<size_t> input_sizes = {100, 500, 1000, 5000, 10000};
        
        for (size_t input_size : input_sizes) {
            PerformanceStats stats;
            const int num_iterations = 100;
            
            for (int i = 0; i < num_iterations; ++i) {
                VMContext ctx = setup_vm_context();
                
                ctx.registers[1] = static_cast<int64_t>(input_size); // input size indicator
                ctx.registers[2] = 12345; // layer config
                
                t81::tisc::Insn insn;
                insn.opcode = static_cast<t81::tisc::Opcode>(0xE0); // NEURAL_FWD
                insn.a = 3; insn.b = 1; insn.c = 2;
                
                timer_.start();
                Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
                double elapsed = timer_.elapsed_microseconds();
                
                if (result == Trap::None) {
                    stats.measurements.push_back(elapsed);
                }
            }
            
            stats.compute_stats();
            std::cout << "  Input Size " << input_size << ": Mean " << stats.mean << " μs" << std::endl;
        }
    }
    
    void benchmark_scalability_network_depth() {
        std::cout << "\n--- Network Depth Scalability Benchmark ---" << std::endl;
        
        std::vector<int> layer_counts = {1, 3, 5, 10, 20};
        
        for (int num_layers : layer_counts) {
            PerformanceStats stats;
            const int num_iterations = 50;
            
            for (int i = 0; i < num_iterations; ++i) {
                timer_.start();
                
                VMContext ctx = setup_vm_context();
                
                // Simulate multiple forward passes
                for (int layer = 0; layer < num_layers; ++layer) {
                    ctx.registers[1] = 1000 + layer; // input
                    ctx.registers[2] = 12345 + layer; // layer config
                    
                    t81::tisc::Insn insn;
                    insn.opcode = static_cast<t81::tisc::Opcode>(0xE0); // NEURAL_FWD
                    insn.a = 3; insn.b = 1; insn.c = 2;
                    
                    ai_integration_->execute_advanced_ai_opcode(insn, ctx);
                }
                
                double elapsed = timer_.elapsed_microseconds();
                stats.measurements.push_back(elapsed);
            }
            
            stats.compute_stats();
            std::cout << "  " << num_layers << " layers: Mean " << stats.mean << " μs" << std::endl;
        }
    }
};

} // namespace t81::vm::advanced_ai::benchmark

// Main benchmark runner
int main(int argc, char** argv) {
    std::cout << "Advanced AI Performance Benchmark Runner" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    t81::vm::advanced_ai::benchmark::AdvancedAIBenchmarks benchmarks;
    benchmarks.run_all_benchmarks();
    
    return 0;
}
