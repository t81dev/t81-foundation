// VM CSI Integration Test Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <memory>
#include <chrono>

#include "csi_integration.hpp"
#include "t81/vm/vm.hpp"
#include "t81/axion/engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace t81::vm::csi::test {

class CSIVMIntegrationTestSuite {
private:
    std::unique_ptr<CSIIntegration> csi_integration_;
    std::unique_ptr<t81::axion::PolicyEngine> mock_policy_engine_;
    std::unique_ptr<t81::canonfs::CanonDriver> mock_canonfs_driver_;
    
    int tests_run = 0;
    int tests_passed = 0;
    
public:
    void run_all_tests() {
        std::cout << "=== VM CSI Integration Test Suite ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        setup_mocks();
        
        // Basic integration tests
        test_csi_initialization();
        test_opcode_dispatch();
        test_stochastic_decode_opcode();
        test_chain_management_opcodes();
        test_policy_enforcement_opcodes();
        test_error_handling();
        
        // Advanced integration tests
        test_vm_integration();
        test_deterministic_guarantees();
        test_provenance_integration();
        
        cleanup_mocks();
        
        std::cout << std::endl;
        std::cout << "=== Test Results ===" << std::endl;
        std::cout << "Tests Run: " << tests_run << std::endl;
        std::cout << "Tests Passed: " << tests_passed << std::endl;
        std::cout << "Success Rate: " << (tests_passed * 100.0 / tests_run) << "%" << std::endl;
        
        if (tests_passed == tests_run) {
            std::cout << "✅ All tests passed!" << std::endl;
        } else {
            std::cout << "❌ Some tests failed!" << std::endl;
        }
    }

private:
    void assert_true(bool condition, const std::string& test_name) {
        tests_run++;
        if (condition) {
            tests_passed++;
            std::cout << "✅ " << test_name << std::endl;
        } else {
            std::cout << "❌ " << test_name << std::endl;
        }
    }
    
    void setup_mocks() {
        std::cout << "Setting up mock components..." << std::endl;
        
        // Create mock policy engine
        mock_policy_engine_ = std::make_unique<t81::axion::PolicyEngine>();
        
        // Create mock CanonFS driver
        mock_canonfs_driver_ = std::make_unique<t81::canonfs::CanonDriver>();
        
        // Initialize CSI integration
        csi_integration_ = std::make_unique<CSIIntegration>();
        csi_integration_->initialize(mock_policy_engine_.get(), mock_canonfs_driver_.get());
    }
    
    void cleanup_mocks() {
        std::cout << "Cleaning up mock components..." << std::endl;
        csi_integration_.reset();
        mock_canonfs_driver_.reset();
        mock_policy_engine_.reset();
    }
    
    // Basic integration tests
    
    void test_csi_initialization() {
        std::cout << "\n--- CSI Initialization Tests ---" << std::endl;
        
        assert_true(csi_integration_ != nullptr, "CSI integration created");
        assert_true(csi_integration_->get_csi_state().stochastic_enabled, "CSI enabled after initialization");
        assert_true(csi_integration_->get_csi_state().current_seed == 12345, "Default seed set correctly");
        assert_true(csi_integration_->get_csi_state().policy_engine != nullptr, "Policy engine linked");
        assert_true(csi_integration_->get_csi_state().canonfs_driver != nullptr, "CanonFS driver linked");
    }
    
    void test_opcode_dispatch() {
        std::cout << "\n--- Opcode Dispatch Tests ---" << std::endl;
        
        // Create mock VM context
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        // Test each CSI opcode dispatch
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
            t81::tisc::Insn insn;
            insn.opcode = opcode;
            insn.a = 0;
            insn.b = 1;
            insn.c = 2;
            
            Trap result = csi_integration_->execute_csi_opcode(insn, ctx);
            
            // Should not be IllegalInstruction (opcode recognized)
            assert_true(result != Trap::IllegalInstruction, 
                       "Opcode " + std::to_string(static_cast<int>(opcode)) + " dispatched successfully");
        }
    }
    
    void test_stochastic_decode_opcode() {
        std::cout << "\n--- Stochastic Decode Opcode Tests ---" << std::endl;
        
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        // Test successful stochastic decode
        t81::tisc::Insn insn;
        insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        insn.a = 0;  // dest
        insn.b = 1;  // logits_reg
        insn.c = 2;  // config_reg
        
        // Setup mock data
        setup_mock_logits(ctx, 1);
        setup_mock_config(ctx, 2);
        
        Trap result = csi_integration_->execute_csi_opcode(insn, ctx);
        
        // Should succeed or have policy-related trap
        assert_true(result == Trap::None || result == Trap::SecurityFault, 
                   "Stochastic decode executes without decode fault");
        
        if (result == Trap::None) {
            assert_true(ctx.registers[0] != 0, "Result stored in destination register");
        }
    }
    
    void test_chain_management_opcodes() {
        std::cout << "\n--- Chain Management Opcode Tests ---" << std::endl;
        
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        // Test chain begin
        t81::tisc::Insn begin_insn;
        begin_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_BEGIN;
        begin_insn.a = 0;  // dest (chain_id)
        begin_insn.b = 1;  // model_id_reg
        begin_insn.c = 2;  // config_reg
        
        setup_mock_model_id(ctx, 1);
        setup_mock_config(ctx, 2);
        
        Trap result = csi_integration_->execute_csi_opcode(begin_insn, ctx);
        assert_true(result == Trap::None, "Chain begin succeeds");
        assert_true(ctx.registers[0] != 0, "Chain ID stored in destination");
        
        // Test chain step
        t81::tisc::Insn step_insn;
        step_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP;
        step_insn.b = 1;  // timestep_reg
        step_insn.c = 2;  // input_hash_reg
        
        ctx.registers[1] = 42;  // timestep
        setup_mock_input_hash(ctx, 2);
        
        result = csi_integration_->execute_csi_opcode(step_insn, ctx);
        assert_true(result == Trap::None, "Chain step succeeds");
        
        // Test chain end
        t81::tisc::Insn end_insn;
        end_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_END;
        end_insn.a = 0;  // dest (chain_hash)
        
        result = csi_integration_->execute_csi_opcode(end_insn, ctx);
        assert_true(result == Trap::None, "Chain end succeeds");
        assert_true(ctx.registers[0] != 0, "Chain hash stored in destination");
    }
    
    void test_policy_enforcement_opcodes() {
        std::cout << "\n--- Policy Enforcement Opcode Tests ---" << std::endl;
        
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        // Test policy evaluation
        t81::tisc::Insn eval_insn;
        eval_insn.opcode = t81::tisc::Opcode::POLICY_EVAL_STOCHASTIC;
        eval_insn.a = 0;  // dest (verdict)
        eval_insn.b = 1;  // context_reg
        eval_insn.c = 2;  // data_reg
        
        setup_mock_policy_context(ctx, 1);
        setup_mock_policy_data(ctx, 2);
        
        Trap result = csi_integration_->execute_csi_opcode(eval_insn, ctx);
        assert_true(result == Trap::None, "Policy evaluation succeeds");
        assert_true(ctx.registers[0] >= 0 && ctx.registers[0] <= 2, "Valid verdict stored");
        
        // Test entropy constraint
        t81::tisc::Insn entropy_insn;
        entropy_insn.opcode = t81::tisc::Opcode::POLICY_CONSTRAIN_ENTROPY;
        entropy_insn.a = 0;  // dest (constraint_applied)
        entropy_insn.b = 1;  // entropy_reg
        entropy_insn.c = 2;  // limit_reg
        
        ctx.registers[1] = 3.0;  // high entropy
        ctx.registers[2] = 2.0;  // limit
        
        result = csi_integration_->execute_csi_opcode(entropy_insn, ctx);
        assert_true(result == Trap::None, "Entropy constraint succeeds");
        assert_true(ctx.registers[0] == 0 || ctx.registers[0] == 1, "Constraint indicator valid");
    }
    
    void test_error_handling() {
        std::cout << "\n--- Error Handling Tests ---" << std::endl;
        
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        // Test invalid register access
        t81::tisc::Insn bad_insn;
        bad_insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        bad_insn.a = 255;  // invalid register
        bad_insn.b = 1;
        bad_insn.c = 2;
        
        Trap result = csi_integration_->execute_csi_opcode(bad_insn, ctx);
        assert_true(result == Trap::DecodeFault, "Invalid register handled correctly");
        
        // Test unrecognized CSI opcode (if we add one)
        // This would require modifying the test to use an invalid opcode value
        
        // Test operations without active chain
        t81::tisc::Insn step_without_chain;
        step_without_chain.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP;
        step_without_chain.b = 1;
        step_without_chain.c = 2;
        
        result = csi_integration_->execute_csi_opcode(step_without_chain, ctx);
        assert_true(result == Trap::RuntimeFault, "Chain step without chain handled correctly");
    }
    
    // Advanced integration tests
    
    void test_vm_integration() {
        std::cout << "\n--- VM Integration Tests ---" << std::endl;
        
        // Test that VM can dispatch CSI opcodes
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        t81::tisc::Insn insn;
        insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        insn.a = 0;
        insn.b = 1;
        insn.c = 2;
        
        setup_mock_logits(ctx, 1);
        setup_mock_config(ctx, 2);
        
        // Test the external dispatch function
        Trap result = execute_csi_opcode_if_enabled(insn, ctx);
        
        // Should not be IllegalInstruction (CSI integration working)
        assert_true(result != Trap::IllegalInstruction, "VM dispatches CSI opcodes correctly");
    }
    
    void test_deterministic_guarantees() {
        std::cout << "\n--- Deterministic Guarantees Tests ---" << std::endl;
        
        VMContext ctx1, ctx2;
        setup_mock_vm_context(ctx1);
        setup_mock_vm_context(ctx2);
        
        // Setup identical contexts
        setup_mock_logits(ctx1, 1);
        setup_mock_logits(ctx2, 1);
        setup_mock_config(ctx1, 2);
        setup_mock_config(ctx2, 2);
        
        // Use same seed for deterministic results
        t81::tisc::Insn seed_insn;
        seed_insn.opcode = t81::tisc::Opcode::STOCHASTIC_SEED;
        seed_insn.a = 3;  // seed_reg
        ctx1.registers[3] = 99999;
        ctx2.registers[3] = 99999;
        
        Trap result1 = csi_integration_->execute_csi_opcode(seed_insn, ctx1);
        Trap result2 = csi_integration_->execute_csi_opcode(seed_insn, ctx2);
        
        assert_true(result1 == Trap::None && result2 == Trap::None, "Seed setting succeeds");
        
        // Run identical stochastic decode
        t81::tisc::Insn decode_insn;
        decode_insn.opcode = t81::tisc::Opcode::STOCHASTIC_DECODE;
        decode_insn.a = 0;
        decode_insn.b = 1;
        decode_insn.c = 2;
        
        result1 = csi_integration_->execute_csi_opcode(decode_insn, ctx1);
        result2 = csi_integration_->execute_csi_opcode(decode_insn, ctx2);
        
        // Should produce same results with same seed
        if (result1 == Trap::None && result2 == Trap::None) {
            assert_true(ctx1.registers[0] == ctx2.registers[0], "Same seed produces same result");
        }
    }
    
    void test_provenance_integration() {
        std::cout << "\n--- Provenance Integration Tests ---" << std::endl;
        
        VMContext ctx;
        setup_mock_vm_context(ctx);
        
        // Create a complete provenance chain
        t81::tisc::Insn begin_insn;
        begin_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_BEGIN;
        begin_insn.a = 0;
        begin_insn.b = 1;
        begin_insn.c = 2;
        
        setup_mock_model_id(ctx, 1);
        setup_mock_config(ctx, 2);
        
        Trap result = csi_integration_->execute_csi_opcode(begin_insn, ctx);
        assert_true(result == Trap::None, "Chain creation succeeds");
        
        // Add multiple steps
        for (int i = 0; i < 3; ++i) {
            t81::tisc::Insn step_insn;
            step_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP;
            step_insn.b = 1;
            step_insn.c = 2;
            
            ctx.registers[1] = i;  // timestep
            setup_mock_input_hash(ctx, 2);
            
            result = csi_integration_->execute_csi_opcode(step_insn, ctx);
            assert_true(result == Trap::None, "Chain step " + std::to_string(i) + " succeeds");
        }
        
        // Finalize chain
        t81::tisc::Insn end_insn;
        end_insn.opcode = t81::tisc::Opcode::STOCHASTIC_CHAIN_END;
        end_insn.a = 0;
        
        result = csi_integration_->execute_csi_opcode(end_insn, ctx);
        assert_true(result == Trap::None, "Chain finalization succeeds");
        assert_true(ctx.registers[0] != 0, "Chain hash generated");
        
        // Verify chain
        t81::tisc::Insn verify_insn;
        verify_insn.opcode = t81::tisc::Opcode::STOCHASTIC_VERIFY;
        verify_insn.a = 0;  // chain_hash_reg (already has hash from end)
        
        result = csi_integration_->execute_csi_opcode(verify_insn, ctx);
        assert_true(result == Trap::None, "Chain verification succeeds");
        assert_true(ctx.registers[0] == 1, "Chain verification passes");
    }

    // Helper methods
    
    void setup_mock_vm_context(VMContext& ctx) {
        ctx.registers.resize(256, 0);
        ctx.register_tags.resize(256, t81::ValueTag::Int);
        ctx.pc = 0;
        ctx.sp = 0;
    }
    
    void setup_mock_logits(VMContext& ctx, uint8_t reg) {
        // Mock logits tensor handle
        ctx.registers[reg] = 0x12345678;  // Mock tensor handle
        ctx.register_tags[reg] = t81::ValueTag::TensorHandle;
    }
    
    void setup_mock_config(VMContext& ctx, uint8_t reg) {
        // Mock configuration handle
        ctx.registers[reg] = 0x87654321;  // Mock config handle
        ctx.register_tags[reg] = t81::ValueTag::BigIntHandle;
    }
    
    void setup_mock_model_id(VMContext& ctx, uint8_t reg) {
        // Mock model ID
        ctx.registers[reg] = 0x11111111;  // Mock model ID
        ctx.register_tags[reg] = t81::ValueTag::StringHandle;
    }
    
    void setup_mock_input_hash(VMContext& ctx, uint8_t reg) {
        // Mock input hash
        ctx.registers[reg] = 0x22222222;  // Mock input hash
        ctx.register_tags[reg] = t81::ValueTag::StringHandle;
    }
    
    void setup_mock_policy_context(VMContext& ctx, uint8_t reg) {
        // Mock policy context
        ctx.registers[reg] = 0x33333333;  // Mock context
        ctx.register_tags[reg] = t81::ValueTag::StringHandle;
    }
    
    void setup_mock_policy_data(VMContext& ctx, uint8_t reg) {
        // Mock policy data
        ctx.registers[reg] = 0x44444444;  // Mock data
        ctx.register_tags[reg] = t81::ValueTag::StringHandle;
    }
};

} // namespace t81::vm::csi::test

// Main test runner
int main(int argc, char** argv) {
    std::cout << "VM CSI Integration Test Runner" << std::endl;
    std::cout << "================================" << std::endl;
    
    t81::vm::csi::test::CSIVMIntegrationTestSuite test_suite;
    test_suite.run_all_tests();
    
    return 0;
}
