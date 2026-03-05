// T81 AI Experiment Sandbox Manager - RFC-00A0
// Provides infrastructure for safe AI experimentation with core protection

#include <t81/core.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace t81::ai::sandbox {

class ExperimentManager {
private:
    std::filesystem::path experiments_root_;
    std::vector<std::string> active_experiments_;
    
    // Core protection verification
    bool verify_core_protection() {
        const std::vector<std::string> protected_dirs = {
            "/src", "/include/t81", "/spec", "/tests"
        };
        
        for (const auto& dir : protected_dirs) {
            if (std::filesystem::exists(experiments_root_ / dir)) {
                std::cerr << "ERROR: Protected directory " << dir 
                         << " found in experiments directory" << std::endl;
                return false;
            }
        }
        return true;
    }
    
public:
    ExperimentManager(const std::filesystem::path& root) 
        : experiments_root_(root) {
        
        if (!verify_core_protection()) {
            throw std::runtime_error("Core protection verification failed");
        }
    }
    
    // List available experiments
    std::vector<std::string> list_experiments() const {
        std::vector<std::string> experiments;
        
        const std::vector<std::string> experiment_dirs = {
            "determinism", "benchmarks", "model_provenance",
            "quantization", "llm_backend", "policy_hooks",
            "ux_tools", "vm_opcodes"
        };
        
        for (const auto& exp : experiment_dirs) {
            if (std::filesystem::exists(experiments_root_ / exp)) {
                experiments.push_back(exp);
            }
        }
        
        return experiments;
    }
    
    // Enable specific experiment
    bool enable_experiment(const std::string& experiment_name) {
        if (std::find(active_experiments_.begin(), 
                     active_experiments_.end(), 
                     experiment_name) != active_experiments_.end()) {
            active_experiments_.push_back(experiment_name);
            return true;
        }
        return false;
    }
    
    // Validate experiment isolation
    bool validate_isolation() const {
        // Ensure experiments don't depend on core modifications
        for (const auto& exp : active_experiments_) {
            std::filesystem::path exp_dir = experiments_root_ / exp;
            
            // Check for forbidden includes
            for (const auto& entry : std::filesystem::recursive_directory_iterator(exp_dir)) {
                if (entry.path().string().find("include/t81/") != std::string::npos) {
                    std::cerr << "ERROR: Forbidden include found in " 
                             << exp << " experiment" << std::endl;
                    return false;
                }
            }
        }
        return true;
    }
    
    // Generate promotion report
    void generate_promotion_report() const {
        std::cout << "=== T81 AI Experiment Promotion Report ===" << std::endl;
        std::cout << "Active Experiments: ";
        for (size_t i = 0; i < active_experiments_.size(); ++i) {
            std::cout << active_experiments_[i];
            if (i < active_experiments_.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        
        std::cout << "Core Protection: VERIFIED" << std::endl;
        std::cout << "Isolation Status: " 
                  << (validate_isolation() ? "PASS" : "FAIL") << std::endl;
        std::cout << "Ready for Promotion: " 
                  << (validate_isolation() ? "YES" : "NO") << std::endl;
    }
};

} // namespace t81::ai::sandbox

// CLI interface for experiment management
int main(int argc, char* argv[]) {
    try {
        t81::ai::sandbox::ExperimentManager manager(
            std::filesystem::current_path() / "experiments/ai"
        );
        
        if (argc < 2) {
            std::cout << "T81 AI Experiment Sandbox Manager" << std::endl;
            std::cout << "Usage: " << argv[0] << " <command> [options]" << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  list              List available experiments" << std::endl;
            std::cout << "  enable <name>    Enable experiment" << std::endl;
            std::cout << "  validate          Validate experiment isolation" << std::endl;
            std::cout << "  promotion-report  Generate promotion report" << std::endl;
            return 0;
        }
        
        std::string command = argv[1];
        
        if (command == "list") {
            auto experiments = manager.list_experiments();
            std::cout << "Available experiments:" << std::endl;
            for (const auto& exp : experiments) {
                std::cout << "  - " << exp << std::endl;
            }
        } else if (command == "enable" && argc >= 3) {
            if (manager.enable_experiment(argv[2])) {
                std::cout << "Experiment " << argv[2] << " enabled" << std::endl;
            } else {
                std::cerr << "Failed to enable experiment " << argv[2] << std::endl;
                return 1;
            }
        } else if (command == "validate") {
            if (manager.validate_isolation()) {
                std::cout << "Experiment isolation validation: PASS" << std::endl;
            } else {
                std::cerr << "Experiment isolation validation: FAIL" << std::endl;
                return 1;
            }
        } else if (command == "promotion-report") {
            manager.generate_promotion_report();
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
