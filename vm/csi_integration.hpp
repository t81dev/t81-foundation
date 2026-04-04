// VM Integration for Controlled Stochastic Inference (CSI)
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Header file for CSI opcode integration

#pragma once

#include <cstdint>
#include <memory>

#include "t81/vm/vm.hpp"
#include "t81/axion/engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace t81::vm::csi {

// CSI opcode family extensions (0xD0-0xDF range)
enum class CSIOpcode : std::uint8_t {
    // Stochastic inference operations
    STOCHASTIC_DECODE = 0xD0,      // Policy-gated stochastic decoding
    STOCHASTIC_SAMPLE = 0xD1,      // Sample from candidate set
    STOCHASTIC_CHAIN_BEGIN = 0xD2, // Begin provenance chain
    STOCHASTIC_CHAIN_STEP = 0xD3,  // Add step to provenance chain
    STOCHASTIC_CHAIN_END = 0xD4,    // Finalize provenance chain
    STOCHASTIC_CONFIG = 0xD5,      // Configure stochastic parameters
    STOCHASTIC_SEED = 0xD6,         // Set stochastic seed
    STOCHASTIC_VERIFY = 0xD7,      // Verify stochastic chain
    
    // Policy enforcement operations
    POLICY_EVAL_STOCHASTIC = 0xD8,  // Evaluate stochastic policy
    POLICY_CONSTRAIN_ENTROPY = 0xD9, // Apply entropy constraints
    POLICY_FILTER_TOKENS = 0xDA,   // Filter forbidden tokens
    POLICY_RECORD_DECISION = 0xDB, // Record policy decision
};

// CSI VM state extension
struct CSIState {
    std::unique_ptr<t81::experimental::csi::ControlledStochasticDecoder> decoder;
    std::unique_ptr<t81::experimental::csi::StochasticProvenanceChain> current_chain;
    std::unique_ptr<t81::experimental::csi::PolicyGatedSampler> sampler;
    
    // Current stochastic configuration
    t81::experimental::csi::StochasticConfig current_config;
    bool stochastic_enabled = false;
    uint64_t current_seed = 12345;
    
    // Policy integration
    t81::axion::PolicyEngine* policy_engine = nullptr;
    t81::canonfs::CanonDriver* canonfs_driver = nullptr;
    
    // Chain management
    std::string current_chain_id;
    std::map<std::string, t81::canonfs::CanonFSHash> completed_chains;
};

// CSI VM integration class
class CSIIntegration {
public:
    // Initialize CSI subsystem
    void initialize(t81::axion::PolicyEngine* policy_engine,
                   t81::canonfs::CanonDriver* canonfs_driver);
    
    // Execute CSI opcode
    Trap execute_csi_opcode(const t81::tisc::Insn& insn, VMContext& ctx);
    
    // Get current CSI state (for debugging/monitoring)
    const CSIState& get_csi_state() const;

private:
    CSIState csi_state_;
    
    // Opcode handlers
    Trap handle_stochastic_decode(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_stochastic_sample(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_chain_begin(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_chain_step(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_chain_end(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_stochastic_config(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_stochastic_seed(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_stochastic_verify(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_policy_eval(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_policy_constrain(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_policy_filter(const t81::tisc::Insn& insn, VMContext& ctx);
    Trap handle_policy_record(const t81::tisc::Insn& insn, VMContext& ctx);
    
    // Helper methods
    bool check_registers(const t81::tisc::Insn& insn, const std::vector<uint8_t>& regs, const VMContext& ctx);
    void set_register_to_string(VMContext& ctx, uint8_t reg, const std::string& value);
    std::string get_string_from_register(int64_t reg_value);
    void reconfigure_stochastic_components();
    std::string generate_chain_id();
};

// Global CSI integration instance
extern std::unique_ptr<CSIIntegration> g_csi_integration;

// VM extension functions
extern "C" {
    // Main dispatch function for CSI opcodes
    Trap execute_csi_opcode_if_enabled(const t81::tisc::Insn& insn, VMContext& ctx);
    
    // Initialization and cleanup
    void initialize_csi_integration(t81::axion::PolicyEngine* policy_engine,
                                  t81::canonfs::CanonDriver* canonfs_driver);
    void cleanup_csi_integration();
}

} // namespace t81::vm::csi
