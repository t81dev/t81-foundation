// Advanced AI Opcode Extensions for T81 VM
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// RFC-00E2 - Advanced Neural Network Operations (0xE0-0xEF range)

#pragma once

#include <cstdint>
#include "t81/isa/opcodes.hpp"

namespace t81::isa {

// Advanced AI Opcode Extensions (0xE0-0xEF range)
// These opcodes extend beyond the basic AI-native operations to provide
// comprehensive neural network capabilities while maintaining deterministic
// execution and policy governance.

enum class AdvancedAIOpcode : std::uint8_t {
    // Advanced Neural Network Operations (0xE0-0xE7)
    NEURAL_FWD = 0xE0,     // Forward pass with configurable layers
    NEURAL_BACK = 0xE1,    // Backward pass (for training/research)
    NEURAL_OPT = 0xE2,     // Optimizer step (SGD, Adam, etc.)
    NEURAL_ACT = 0xE3,     // Advanced activation functions
    NEURAL_NORM = 0xE4,    // Layer/Batch/Group normalization
    NEURAL_DROP = 0xE5,    // Dropout with deterministic seeding
    NEURAL_RES = 0xE6,     // Residual connections
    NEURAL_ATTN = 0xE7,    // Advanced attention variants
    
    // Advanced Quantization & Compression (0xE8-0xEF)
    QUANT_TERN = 0xE8,     // Ternary quantization (beyond T3_K)
    QUANT_PRUN = 0xE9,     // Structured pruning
    QUANT_DIST = 0xEA,     // Distribution-aware quantization
    QUANT_COMP = 0xEB,     // Compression algorithms
    QUANT_DECOMP = 0xEC,   // Decompression with verification
    QUANT_VERIFY = 0xED,   // Quantization integrity checks
    QUANT_ADAPT = 0xEE,    // Adaptive quantization
    QUANT_MIXED = 0xEF,    // Mixed-precision operations
};

// Convert to TISC opcode for VM dispatch
[[nodiscard]] constexpr t81::tisc::Opcode to_tisc_opcode(AdvancedAIOpcode op) {
    switch (op) {
        case AdvancedAIOpcode::NEURAL_FWD:    return static_cast<t81::tisc::Opcode>(0xE0);
        case AdvancedAIOpcode::NEURAL_BACK:   return static_cast<t81::tisc::Opcode>(0xE1);
        case AdvancedAIOpcode::NEURAL_OPT:    return static_cast<t81::tisc::Opcode>(0xE2);
        case AdvancedAIOpcode::NEURAL_ACT:    return static_cast<t81::tisc::Opcode>(0xE3);
        case AdvancedAIOpcode::NEURAL_NORM:   return static_cast<t81::tisc::Opcode>(0xE4);
        case AdvancedAIOpcode::NEURAL_DROP:   return static_cast<t81::tisc::Opcode>(0xE5);
        case AdvancedAIOpcode::NEURAL_RES:    return static_cast<t81::tisc::Opcode>(0xE6);
        case AdvancedAIOpcode::NEURAL_ATTN:   return static_cast<t81::tisc::Opcode>(0xE7);
        case AdvancedAIOpcode::QUANT_TERN:   return static_cast<t81::tisc::Opcode>(0xE8);
        case AdvancedAIOpcode::QUANT_PRUN:   return static_cast<t81::tisc::Opcode>(0xE9);
        case AdvancedAIOpcode::QUANT_DIST:   return static_cast<t81::tisc::Opcode>(0xEA);
        case AdvancedAIOpcode::QUANT_COMP:   return static_cast<t81::tisc::Opcode>(0xEB);
        case AdvancedAIOpcode::QUANT_DECOMP: return static_cast<t81::tisc::Opcode>(0xEC);
        case AdvancedAIOpcode::QUANT_VERIFY: return static_cast<t81::tisc::Opcode>(0xED);
        case AdvancedAIOpcode::QUANT_ADAPT:  return static_cast<t81::tisc::Opcode>(0xEE);
        case AdvancedAIOpcode::QUANT_MIXED:  return static_cast<t81::tisc::Opcode>(0xEF);
    }
    return t81::tisc::Opcode::Nop;
}

// Check if opcode is in advanced AI range
[[nodiscard]] constexpr bool is_advanced_ai_opcode(t81::tisc::Opcode op) {
    auto opcode_val = static_cast<std::uint8_t>(op);
    return opcode_val >= 0xE0 && opcode_val <= 0xEF;
}

// Get opcode category for policy enforcement
[[nodiscard]] constexpr const char* get_advanced_ai_category(t81::tisc::Opcode op) {
    auto opcode_val = static_cast<std::uint8_t>(op);
    if (opcode_val >= 0xE0 && opcode_val <= 0xE7) {
        return "neural_network";
    } else if (opcode_val >= 0xE8 && opcode_val <= 0xEF) {
        return "quantization";
    }
    return "unknown";
}

// Determinism level requirements
enum class DeterminismLevel {
    STRICT,          // Bit-exact reproducibility required
    CONFIGURABLE,    // Deterministic with configuration
    STATISTICAL,     // Within tolerance bounds
    REPRODUCIBLE     // Documented randomness with seeds
};

[[nodiscard]] constexpr DeterminismLevel get_determinism_level(t81::tisc::Opcode op) {
    auto opcode_val = static_cast<std::uint8_t>(op);
    switch (opcode_val) {
        case 0xE0: // NEURAL_FWD
        case 0xE3: // NEURAL_ACT
        case 0xE4: // NEURAL_NORM
        case 0xE6: // NEURAL_RES
        case 0xE8: // QUANT_TERN
        case 0xEB: // QUANT_COMP
        case 0xEC: // QUANT_DECOMP
        case 0xED: // QUANT_VERIFY
            return DeterminismLevel::STRICT;
            
        case 0xE1: // NEURAL_BACK
        case 0xE2: // NEURAL_OPT
        case 0xE5: // NEURAL_DROP
        case 0xE7: // NEURAL_ATTN
        case 0xE9: // QUANT_PRUN
        case 0xEA: // QUANT_DIST
        case 0xEE: // QUANT_ADAPT
            return DeterminismLevel::CONFIGURABLE;
            
        case 0xEF: // QUANT_MIXED
            return DeterminismLevel::STATISTICAL;
    }
    return DeterminismLevel::STRICT;
}

// Policy tier requirements
[[nodiscard]] constexpr int get_required_tier(t81::tisc::Opcode op) {
    auto opcode_val = static_cast<std::uint8_t>(op);
    switch (opcode_val) {
        case 0xE0: // NEURAL_FWD
        case 0xE3: // NEURAL_ACT
        case 0xE4: // NEURAL_NORM
        case 0xE6: // NEURAL_RES
        case 0xE8: // QUANT_TERN
        case 0xEB: // QUANT_COMP
        case 0xEC: // QUANT_DECOMP
        case 0xED: // QUANT_VERIFY
            return 2; // Tier 2 for basic operations
            
        case 0xE1: // NEURAL_BACK
        case 0xE2: // NEURAL_OPT
        case 0xE5: // NEURAL_DROP
        case 0xE7: // NEURAL_ATTN
        case 0xE9: // QUANT_PRUN
        case 0xEA: // QUANT_DIST
        case 0xEE: // QUANT_ADAPT
            return 3; // Tier 3 for advanced operations
            
        case 0xEF: // QUANT_MIXED
            return 4; // Tier 4 for experimental operations
    }
    return 2;
}

} // namespace t81::isa
