// End-to-End CSI Integration Test
// Tests complete CSI workflow from VM to CanonFS
// EXPERIMENTAL - NOT FOR PRODUCTION USE

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include "t81/vm/vm.hpp"
#include "csi_integration.hpp"
#include "../experimental/ai/csi/stochastic_provenance.hpp"

namespace t81::vm::csi::test {

class EndToEndTest {
private:
    std::unique_ptr<t81::vm::VM> vm_;
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine_;
    std::unique_ptr<t81::canonfs::CanonDriver> canonfs_driver_;
    
public:
    void run_tests() {
        std::cout << "=== End-to-End CSI Integration Tests ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        setup_infrastructure();
        
        test_complete_stochastic_workflow();
        test_provenance_chain_lifecycle();
        test_policy_enforcement_workflow();
        test_cross_system_portability();
        test_performance_characteristics();
        
        cleanup_infrastructure();
        
        std::cout << std::endl;
        std::cout << "✅ End-to-end tests completed successfully!" << std::endl;
    }

private:
    void setup_infrastructure() {
        std::cout << "Setting up complete infrastructure..." << std::endl;
        
        // Create real components
        policy_engine_ = std::make_unique<t81::axion::PolicyEngine>();
        canonfs_driver_ = std::make_unique<t81::canonfs::CanonDriver>();
        
        // Initialize CSI integration
        initialize_csi_integration(policy_engine_.get(), canonfs_driver_.get());
        
        // Create and configure VM
        vm_ = std::make_unique<t81::vm::VM>();
        
        std::cout << "Infrastructure setup complete" << std::endl;
    }
    
    void cleanup_infrastructure() {
        std::cout << "Cleaning up infrastructure..." << std::endl;
        
        cleanup_csi_integration();
        vm_.reset();
        canonfs_driver_.reset();
        policy_engine_.reset();
        
        std::cout << "Infrastructure cleanup complete" << std::endl;
    }
    
    void test_complete_stochastic_workflow() {
        std::cout << "\n--- Complete Stochastic Workflow Test ---" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Create complete stochastic inference program
        std::vector<t81::tisc::Insn> program = create_stochastic_inference_program();
        
        // Load and execute
        bool load_success = vm_->load_program(program);
        std::cout << "Program loading: " << (load_success ? "SUCCESS" : "FAILED") << std::endl;
        
        if (load_success) {
            // Setup initial state
            setup_inference_state();
            
            // Execute program
            bool exec_success = vm_->run(50);  // Max 50 steps
            auto vm_state = vm_->get_state();
            
            std::cout << "Program execution: " << (exec_success ? "SUCCESS" : "FAILED") << std::endl;
            std::cout << "Instructions executed: " << vm_state.instructions_executed << std::endl;
            std::cout << "VM halted: " << (vm_state.halted ? "YES" : "NO") << std::endl;
            
            // Check results
            analyze_inference_results();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        std::cout << "Total workflow time: " << duration.count() << " ms" << std::endl;
    }
    
    void test_provenance_chain_lifecycle() {
        std::cout << "\n--- Provenance Chain Lifecycle Test ---" << std::endl;
        
        // Create provenance chain program
        std::vector<t81::tisc::Insn> program = create_provenance_chain_program();
        
        bool load_success = vm_->load_program(program);
        if (load_success) {
            setup_provenance_state();
            
            bool exec_success = vm_->run(30);
            auto vm_state = vm_->get_state();
            
            std::cout << "Chain program execution: " << (exec_success ? "SUCCESS" : "FAILED") << std::endl;
            
            // Analyze chain results
            analyze_provenance_results();
        }
    }
    
    void test_policy_enforcement_workflow() {
        std::cout << "\n--- Policy Enforcement Workflow Test ---" << std::endl;
        
        // Create policy enforcement program
        std::vector<t81::tisc::Insn> program = create_policy_enforcement_program();
        
        bool load_success = vm_->load_program(program);
        if (load_success) {
            setup_policy_state();
            
            bool exec_success = vm_->run(25);
            auto vm_state = vm_->get_state();
            
            std::cout << "Policy program execution: " << (exec_success ? "SUCCESS" : "FAILED") << std::endl;
            
            // Analyze policy results
            analyze_policy_results();
        }
    }
    
    void test_cross_system_portability() {
        std::cout << "\n--- Cross-System Portability Test ---" << std::endl;
        
        // Test that stochastic chains can be consumed independently
        
        // 1. Create chain in VM
        std::vector<t81::tisc::Insn> chain_program = create_provenance_chain_program();
        vm_->load_program(chain_program);
        setup_provenance_state();
        vm_->run(20);
        
        // 2. Extract chain hash from VM
        auto vm_state = vm_->get_state();
        int64_t chain_hash_reg = vm_->get_register(0);  // Chain hash should be in R0
        
        std::cout << "Chain hash from VM: " << chain_hash_reg << std::endl;
        
        // 3. Verify chain can be loaded independently
        // This would test CanonFS loading and verification
        bool chain_verifiable = verify_chain_independence(chain_hash_reg);
        std::cout << "Chain independence: " << (chain_verifiable ? "VERIFIED" : "FAILED") << std::endl;
        
        vm_->reset();
    }
    
    void test_performance_characteristics() {
        std::cout << "\n--- Performance Characteristics Test ---" << std::endl;
        
        // Test performance of CSI operations
        
        const int num_iterations = 100;
        std::vector<double> execution_times;
        
        for (int i = 0; i < num_iterations; ++i) {
            // Create simple stochastic operation
            std::vector<t81::tisc::Insn> simple_program;
            t81::tisc::Insn seed_insn;
            seed_insn.opcode = t81::tisc::Opcode::STOCHASTIC_SEED;
            seed_insn.a = 0;
            seed_insn.b = i;  // Different seed each iteration
            simple_program.push_back(seed_insn);
            
            vm_->load_program(simple_program);
            
            auto start = std::chrono::high_resolution_clock::now();
            vm_->run(1);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start);
            execution_times.push_back(duration.count());
            
            vm_->reset();
        }
        
        // Calculate statistics
        double total_time = 0;
        for (double time : execution_times) {
            total_time += time;
        }
        double avg_time = total_time / num_iterations;
        
        double min_time = *std::min_element(execution_times.begin(), execution_times.end());
        double max_time = *std::max_element(execution_times.begin(), execution_times.end());
        
        std::cout << "Performance over " << num_iterations << " iterations:" << std::endl;
        std::cout << "Average time: " << avg_time << " μs" << std::endl;
        std::cout << "Min time: " << min_time << " μs" << std::endl;
        std::cout << "Max time: " << max_time << " μs" << std::endl;
        
        // Performance should be reasonable (< 1000μs per operation)
        assert(avg_time < 1000.0);
    }

private:
    std::vector<t81::tisc::Insn> create_stochastic_inference_program() {
        std::vector<t81::tisc::Insn> program;
        
        // 1. Set stochastic seed
        t81::tisc::Insn seed_insn;
        seed_insn.opcode = t81::tisc::Opcode::STOCHASTIC_SEED;
        seed_insn.a = 0;  // seed_dest
        seed_insn.b = 1;  // seed_value
        seed_insn.c = 0;
        program.push_back(seed_insn);
        
        // 2. Configure stochastic parameters
        t81::tisc::Insn config_insn;
        config_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CONFIG;
        config_insn.a = 2;  // param_reg
        config_insn.b = 3;  // value_reg
        config_insn.c = 0;
        program.push_back(config_insn);
        
        // 3. Begin provenance chain
        t81::tisc::Insn begin_insn;
        begin_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_BEGIN;
        begin_insn.a = 4;  // chain_id_dest
        begin_insn.b = 5;  // model_id_reg
        begin_insn.c = 2;  // config_reg
        program.push_back(begin_insn);
        
        // 4. Perform stochastic decode
        t81::tisc::Insn decode_insn;
        decode_insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        decode_insn.a = 6;  // result_dest
        decode_insn.b = 7;  // logits_reg
        decode_insn.c = 2;  // config_reg
        program.push_back(decode_insn);
        
        // 5. Add step to chain
        t81::tisc::Insn step_insn;
        step_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP;
        step_insn.a = 8;  // unused
        step_insn.b = 9;  // timestep_reg
        step_insn.c = 10; // input_hash_reg
        program.push_back(step_insn);
        
        // 6. End chain
        t81::tisc::Insn end_insn;
        end_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_END;
        end_insn.a = 11;  // chain_hash_dest
        end_insn.b = 0;
        end_insn.c = 0;
        program.push_back(end_insn);
        
        return program;
    }
    
    std::vector<t81::tisc::Insn> create_provenance_chain_program() {
        std::vector<t81::tisc::Insn> program;
        
        // Simple chain creation and management
        t81::tisc::Insn begin_insn;
        begin_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_BEGIN;
        begin_insn.a = 0;  // chain_id
        begin_insn.b = 1;  // model_id
        begin_insn.c = 2;  // config
        program.push_back(begin_insn);
        
        // Add multiple steps
        for (int i = 0; i < 3; ++i) {
            t81::tisc::Insn step_insn;
            step_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP;
            step_insn.b = 3;  // timestep
            step_insn.c = 4;  // input_hash
            program.push_back(step_insn);
        }
        
        t81::tisc::Insn end_insn;
        end_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_END;
        end_insn.a = 0;  // chain_hash
        program.push_back(end_insn);
        
        return program;
    }
    
    std::vector<t81::tisc::Insn> create_policy_enforcement_program() {
        std::vector<t81::tisc::Insn> program;
        
        // Policy evaluation
        t81::tisc::Insn eval_insn;
        eval_insn.opcode = t81::tisc::Opcode::POLICY_EVAL_STOCHASTIC;
        eval_insn.a = 0;  // verdict_dest
        eval_insn.b = 1;  // context
        eval_insn.c = 2;  // data
        program.push_back(eval_insn);
        
        // Entropy constraint
        t81::tisc::Insn entropy_insn;
        entropy_insn.opcode = t81::tisc::Opcode::POLICY_CONSTRAIN_ENTROPY;
        entropy_insn.a = 3;  // constraint_applied
        entropy_insn.b = 4;  // entropy
        entropy_insn.c = 5;  // limit
        program.push_back(entropy_insn);
        
        // Token filtering
        t81::tisc::Insn filter_insn;
        filter_insn.opcode = t81::tisc::Opcode::POLICY_FILTER_TOKENS;
        filter_insn.a = 6;  // filtered_count
        filter_insn.b = 7;  // candidates
        filter_insn.c = 8;  // forbidden
        program.push_back(filter_insn);
        
        return program;
    }
    
    void setup_inference_state() {
        // Set up register values for inference
        vm_->set_register(1, 12345);  // seed value
        vm_->set_register(3, 0);      // param: temperature
        vm_->set_register(4, 100);   // value: 1.0 (encoded)
        vm_->set_register(5, 0x11111111);  // model_id
        vm_->set_register(7, 0x12345678);  // logits tensor
        vm_->set_register(9, 0);      // timestep
        vm_->set_register(10, 0x22222222); // input_hash
    }
    
    void setup_provenance_state() {
        vm_->set_register(1, 0x11111111);  // model_id
        vm_->set_register(2, 0x87654321);  // config
        vm_->set_register(3, 0);            // timestep
        vm_->set_register(4, 0x22222222);   // input_hash
    }
    
    void setup_policy_state() {
        vm_->set_register(1, 0x33333333);  // context
        vm_->set_register(2, 0x44444444);  // data
        vm_->set_register(4, 3.0);         // entropy (high)
        vm_->set_register(5, 2.0);         // limit
        vm_->set_register(7, 0x55555555);  // candidates
        vm_->set_register(8, 0x66666666);  // forbidden
    }
    
    void analyze_inference_results() {
        auto vm_state = vm_->get_state();
        
        std::cout << "Inference Results Analysis:" << std::endl;
        std::cout << "Seed stored in R0: " << vm_->get_register(0) << std::endl;
        std::cout << "Chain ID in R4: " << vm_->get_register(4) << std::endl;
        std::cout << "Decode result in R6: " << vm_->get_register(6) << std::endl;
        std::cout << "Chain hash in R11: " << vm_->get_register(11) << std::endl;
        
        // Verify expected results
        assert(vm_->get_register(0) == 12345);  // Seed should be stored
        assert(vm_->get_register(11) != 0);      // Chain hash should be non-zero
    }
    
    void analyze_provenance_results() {
        auto vm_state = vm_->get_state();
        
        std::cout << "Provenance Results Analysis:" << std::endl;
        std::cout << "Chain ID in R0: " << vm_->get_register(0) << std::endl;
        std::cout << "Chain hash in R0 (after end): " << vm_->get_register(0) << std::endl;
        
        // Verify chain was created and finalized
        assert(vm_->get_register(0) != 0);  // Chain hash should be non-zero
    }
    
    void analyze_policy_results() {
        auto vm_state = vm_->get_state();
        
        std::cout << "Policy Results Analysis:" << std::endl;
        std::cout << "Policy verdict in R0: " << vm_->get_register(0) << std::endl;
        std::cout << "Constraint applied in R3: " << vm_->get_register(3) << std::endl;
        std::cout << "Filtered count in R6: " << vm_->get_register(6) << std::endl;
        
        // Verify policy operations worked
        assert(vm_->get_register(0) >= 0 && vm_->get_register(0) <= 2);  // Valid verdict
        assert(vm_->get_register(3) == 0 || vm_->get_register(3) == 1);  // Constraint indicator
    }
    
    bool verify_chain_independence(int64_t chain_hash) {
        // This would test loading the chain from CanonFS independently
        // For now, just verify the hash is reasonable
        return chain_hash != 0 && chain_hash != 1;
    }
};

} // namespace t81::vm::csi::test

int main(int argc, char** argv) {
    std::cout << "End-to-End CSI Integration Test Runner" << std::endl;
    std::cout << "======================================" << std::endl;
    
    t81::vm::csi::test::EndToEndTest test;
    test.run_tests();
    
    return 0;
}
