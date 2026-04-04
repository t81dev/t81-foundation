// VM CSI Dispatch Integration Test
// Tests the actual VM dispatch with CSI opcodes
// EXPERIMENTAL - NOT FOR PRODUCTION USE

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

#include "t81/vm/vm.hpp"
#include "csi_integration.hpp"

namespace t81::vm::csi::test {

class VMDispatchTest {
private:
    std::unique_ptr<t81::vm::VM> vm_;
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine_;
    std::unique_ptr<t81::canonfs::CanonDriver> canonfs_driver_;
    
public:
    void run_tests() {
        std::cout << "=== VM CSI Dispatch Integration Tests ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        setup_vm();
        
        test_vm_dispatch_integration();
        test_csi_opcode_recognition();
        test_policy_gated_execution();
        test_error_propagation();
        test_deterministic_isolation();
        
        cleanup_vm();
        
        std::cout << std::endl;
        std::cout << "✅ VM dispatch integration tests completed!" << std::endl;
    }

private:
    void setup_vm() {
        std::cout << "Setting up VM with CSI integration..." << std::endl;
        
        // Create mock components
        policy_engine_ = std::make_unique<t81::axion::PolicyEngine>();
        canonfs_driver_ = std::make_unique<t81::canonfs::CanonDriver>();
        
        // Initialize CSI integration
        initialize_csi_integration(policy_engine_.get(), canonfs_driver_.get());
        
        // Create VM instance
        vm_ = std::make_unique<t81::vm::VM>();
        
        std::cout << "VM setup complete" << std::endl;
    }
    
    void cleanup_vm() {
        std::cout << "Cleaning up VM..." << std::endl;
        
        cleanup_csi_integration();
        vm_.reset();
        canonfs_driver_.reset();
        policy_engine_.reset();
        
        std::cout << "VM cleanup complete" << std::endl;
    }
    
    void test_vm_dispatch_integration() {
        std::cout << "\n--- VM Dispatch Integration Test ---" << std::endl;
        
        // Create a simple program with CSI opcodes
        std::vector<t81::tisc::Insn> program;
        
        // Add CSI opcodes to program
        t81::tisc::Insn seed_insn;
        seed_insn.opcode = t81::tisc::Opcode::STOCHASTIC_SEED;
        seed_insn.a = 0;  // dest
        seed_insn.b = 0;  // seed value
        seed_insn.c = 0;  // unused
        program.push_back(seed_insn);
        
        t81::tisc::Insn config_insn;
        config_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CONFIG;
        config_insn.a = 1;  // param_reg
        config_insn.b = 2;  // value_reg
        config_insn.c = 0;  // unused
        program.push_back(config_insn);
        
        t81::tisc::Insn decode_insn;
        decode_insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        decode_insn.a = 3;  // dest
        decode_insn.b = 4;  // logits_reg
        decode_insn.c = 5;  // config_reg
        program.push_back(decode_insn);
        
        // Load program into VM
        bool load_success = vm_->load_program(program);
        std::cout << "Program loading: " << (load_success ? "SUCCESS" : "FAILED") << std::endl;
        
        if (load_success) {
            // Execute program
            auto start_time = std::chrono::high_resolution_clock::now();
            bool exec_success = vm_->run(100);  // Max 100 steps
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time);
            
            std::cout << "Program execution: " << (exec_success ? "SUCCESS" : "FAILED") << std::endl;
            std::cout << "Execution time: " << duration.count() << " μs" << std::endl;
            
            // Check VM state
            auto vm_state = vm_->get_state();
            std::cout << "VM halted: " << (vm_state.halted ? "YES" : "NO") << std::endl;
            std::cout << "Instructions executed: " << vm_state.instructions_executed << std::endl;
        }
    }
    
    void test_csi_opcode_recognition() {
        std::cout << "\n--- CSI Opcode Recognition Test ---" << std::endl;
        
        // Test that all CSI opcodes are recognized by VM
        std::vector<t81::tisc::Opcode> csi_opcodes = {
            t81::tisc::Opcode::STOCHASTIC_DECODE,
            t81::tisc::Opcode::STOCHASTIC_SAMPLE,
            t81::tisc::Opcode::STOCHASTIC_CHAIN_BEGIN,
            t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP,
            t81::tisc::Opcode::STOCHASTIC_CHAIN_END,
            t81::tisc::Opcode::STOCHASTIC_CONFIG,
            t81::tisc::Opcode::STOCHASTIC_SEED,
            t81::tisc::Opcode::STOCHASTIC_VERIFY,
            t81::tisc::Opcode::POLICY_EVAL_STOCHASTIC,
            t81::tisc::Opcode::POLICY_CONSTRAIN_ENTROPY,
            t81::tisc::Opcode::POLICY_FILTER_TOKENS,
            t81::tisc::Opcode::POLICY_RECORD_DECISION
        };
        
        for (auto opcode : csi_opcodes) {
            // Create single instruction program
            std::vector<t81::tisc::Insn> program;
            t81::tisc::Insn insn;
            insn.opcode = opcode;
            insn.a = 0;
            insn.b = 1;
            insn.c = 2;
            program.push_back(insn);
            
            // Try to load and execute
            bool load_success = vm_->load_program(program);
            if (load_success) {
                vm_->reset();
                bool exec_success = vm_->run(1);  // Execute single instruction
                
                auto vm_state = vm_->get_state();
                bool recognized = (vm_state.last_trap != t81::vm::Trap::IllegalInstruction);
                
                std::cout << "Opcode " << static_cast<int>(opcode) << ": "
                          << (recognized ? "RECOGNIZED" : "NOT RECOGNIZED") << std::endl;
            } else {
                std::cout << "Opcode " << static_cast<int>(opcode) << ": LOAD FAILED" << std::endl;
            }
            
            vm_->reset();
        }
    }
    
    void test_policy_gated_execution() {
        std::cout << "\n--- Policy Gated Execution Test ---" << std::endl;
        
        // Create program that should trigger policy evaluation
        std::vector<t81::tisc::Insn> program;
        
        // Set up policy that might deny operations
        t81::tisc::Insn policy_insn;
        policy_insn.opcode = t81::tisc::Opcode::POLICY_EVAL_STOCHASTIC;
        policy_insn.a = 0;  // verdict dest
        policy_insn.b = 1;  // context
        policy_insn.c = 2;  // data
        program.push_back(policy_insn);
        
        // Try stochastic operation that might be denied
        t81::tisc::Insn decode_insn;
        decode_insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        decode_insn.a = 3;  // result dest
        decode_insn.b = 4;  // logits
        decode_insn.c = 5;  // config
        program.push_back(decode_insn);
        
        bool load_success = vm_->load_program(program);
        if (load_success) {
            bool exec_success = vm_->run(10);
            auto vm_state = vm_->get_state();
            
            std::cout << "Policy-gated execution: " << (exec_success ? "SUCCESS" : "FAILED") << std::endl;
            std::cout << "Last trap: " << static_cast<int>(vm_state.last_trap) << std::endl;
            
            // Check if policy evaluation occurred
            // This would require inspecting Axion logs or CSI state
        }
    }
    
    void test_error_propagation() {
        std::cout << "\n--- Error Propagation Test ---" << std::endl;
        
        // Test error conditions and proper trap propagation
        
        // 1. Invalid register access
        std::vector<t81::tisc::Insn> bad_reg_program;
        t81::tisc::Insn bad_reg_insn;
        bad_reg_insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        bad_reg_insn.a = 255;  // Invalid register
        bad_reg_insn.b = 1;
        bad_reg_insn.c = 2;
        bad_reg_program.push_back(bad_reg_insn);
        
        vm_->load_program(bad_reg_program);
        vm_->run(1);
        auto vm_state = vm_->get_state();
        
        std::cout << "Invalid register trap: " << static_cast<int>(vm_state.last_trap) << std::endl;
        assert(vm_state.last_trap == t81::vm::Trap::DecodeFault);
        
        vm_->reset();
        
        // 2. Chain operation without active chain
        std::vector<t81::tisc::Insn> no_chain_program;
        t81::tisc::Insn step_insn;
        step_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP;
        step_insn.b = 1;
        step_insn.c = 2;
        no_chain_program.push_back(step_insn);
        
        vm_->load_program(no_chain_program);
        vm_->run(1);
        vm_state = vm_->get_state();
        
        std::cout << "No chain trap: " << static_cast<int>(vm_state.last_trap) << std::endl;
        assert(vm_state.last_trap == t81::vm::Trap::RuntimeFault);
        
        vm_->reset();
        
        std::cout << "Error propagation working correctly" << std::endl;
    }
    
    void test_deterministic_isolation() {
        std::cout << "\n--- Deterministic Isolation Test ---" << std::endl;
        
        // Test that deterministic opcodes are unaffected by CSI
        
        // Create mixed program
        std::vector<t81::tisc::Insn> mixed_program;
        
        // Deterministic operation
        t81::tisc::Insn add_insn;
        add_insn.opcode = t81::tisc::Opcode::Add;
        add_insn.a = 0;
        add_insn.b = 1;
        add_insn.c = 2;
        mixed_program.push_back(add_insn);
        
        // CSI operation
        t81::tisc::Insn csi_insn;
        csi_insn.opcode = t81::tisc::Opcode::STOCHASTIC_SEED;
        csi_insn.a = 3;
        csi_insn.b = 12345;
        csi_insn.c = 0;
        mixed_program.push_back(csi_insn);
        
        // Another deterministic operation
        t81::tisc::Insn mul_insn;
        mul_insn.opcode = t81::tisc::Opcode::Mul;
        mul_insn.a = 4;
        mul_insn.b = 0;
        mul_insn.c = 2;
        mixed_program.push_back(mul_insn);
        
        bool load_success = vm_->load_program(mixed_program);
        if (load_success) {
            // Set initial register values
            vm_->set_register(1, 5);  // 5 + 2 = 7
            vm_->set_register(2, 2);
            
            bool exec_success = vm_->run(10);
            auto vm_state = vm_->get_state();
            
            std::cout << "Mixed program execution: " << (exec_success ? "SUCCESS" : "FAILED") << std::endl;
            
            // Check deterministic results
            int64_t add_result = vm_->get_register(0);
            int64_t mul_result = vm_->get_register(4);
            
            std::cout << "Add result (5+2): " << add_result << std::endl;
            std::cout << "Mul result (7*2): " << mul_result << std::endl;
            
            // Deterministic operations should work normally
            assert(add_result == 7);
            assert(mul_result == 14);
            
            std::cout << "Deterministic isolation working correctly" << std::endl;
        }
        
        vm_->reset();
    }
};

} // namespace t81::vm::csi::test

int main(int argc, char** argv) {
    std::cout << "VM CSI Dispatch Integration Test Runner" << std::endl;
    std::cout << "======================================" << std::endl;
    
    t81::vm::csi::test::VMDispatchTest test;
    test.run_tests();
    
    return 0;
}
