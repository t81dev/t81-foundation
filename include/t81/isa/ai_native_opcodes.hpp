// AI-Native Opcodes Header
// Created for T81 LLM integration

#pragma once

#include <memory>
#include <string>

namespace t81::isa {

// Forward declarations
class OpcodeHandler;

// AI-Native Opcodes (RFC-0026)
enum class AIOpcode {
    ATTN,
    QMATMUL, 
    WLOAD,
    EMBED,
    GATHER,
    SCATTER
};

// Base opcode handler class
class OpcodeHandler {
public:
    virtual ~OpcodeHandler() = default;
    virtual void execute() = 0;
};

// Specific opcode handler classes
class ATTN_Handler : public OpcodeHandler {
public:
    void execute() override;
};

class QMATMUL_Handler : public OpcodeHandler {
public:
    void execute() override;
};

class WLOAD_Handler : public OpcodeHandler {
public:
    void execute() override;
};

class EMBED_Handler : public OpcodeHandler {
public:
    void execute() override;
};

class GATHER_Handler : public OpcodeHandler {
public:
    void execute() override;
};

class SCATTER_Handler : public OpcodeHandler {
public:
    void execute() override;
};

// Factory function
std::unique_ptr<OpcodeHandler> create_opcode_handler(AIOpcode opcode);

} // namespace t81::isa
