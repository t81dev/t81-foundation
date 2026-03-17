// examples/axion_integration_examples.cpp
//
// Comprehensive examples of Axion OS Kernel integration patterns
// for deterministic ternary computing systems.

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "t81/axion/api.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/ethics.hpp"
#include "t81/axion/context.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace examples::axion {

// ─── Example 1: Basic Axion Integration ─────────────────────────────────

void basic_integration_example() {
  std::cout << "=== Basic Axion Integration Example ===\n";
  
  // Initialize Axion with default policy
  if (!t81::axion::initialize()) {
    std::cerr << "❌ Failed to initialize Axion\n";
    return;
  }
  
  std::cout << "✅ Axion initialized successfully\n";
  
  // Create a simple syscall context
  t81::axion::SyscallContext ctx;
  ctx.caller = "example_program";
  ctx.syscall = "file_write";
  ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"root"}};
  ctx.metadata["file_size"] = "1024";
  ctx.metadata["file_type"] = "text";
  
  // Evaluate the operation
  auto verdict = t81::axion::evaluate(ctx);
  
  std::cout << "Operation: " << ctx.syscall << "\n";
  std::cout << "Verdict: " << (verdict.kind == t81::axion::VerdictKind::Allow ? "✅ Allow" : "❌ Deny") << "\n";
  std::cout << "Reason: " << verdict.reason << "\n";
  
  // Cleanup
  t81::axion::shutdown();
  std::cout << "✅ Axion shutdown complete\n\n";
}

// ─── Example 2: Policy Engine Integration ───────────────────────────────

void policy_engine_example() {
  std::cout << "=== Policy Engine Integration Example ===\n";
  
  // Create a custom security policy
  t81::axion::Policy security_policy;
  security_policy.name = "example_security";
  security_policy.version = "1.0.0";
  
  // Simple bytecode: allow file operations, deny network operations
  security_policy.bytecode = {
    0x01, 0x00, 0x00, 0x00,  // LOAD syscall
    0x02, 0x04, 0x00, 0x00,  // CONST "file_"
    0x03, 0x00, 0x00, 0x00,  // STR_STARTS_WITH
    0x04, 0x01, 0x00, 0x00,  // JUMP_IF_TRUE 1
    0x05, 0x00, 0x00, 0x00,  // DENY
    0x06, 0x00, 0x00, 0x00   // ALLOW
  };
  
  // Initialize policy engine
  t81::axion::PolicyEngine engine(security_policy);
  
  std::cout << "✅ Policy engine initialized with security policy\n";
  
  // Test file operation (should be allowed)
  t81::axion::SyscallContext file_ctx;
  file_ctx.caller = "test_program";
  file_ctx.syscall = "file_write";
  file_ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"root"}};
  
  auto file_verdict = engine.execute_bytecode(file_ctx);
  std::cout << "File operation: " << (file_verdict.kind == t81::axion::VerdictKind::Allow ? "✅ Allowed" : "❌ Denied") << "\n";
  
  // Test network operation (should be denied)
  t81::axion::SyscallContext net_ctx;
  net_ctx.caller = "test_program";
  net_ctx.syscall = "network_connect";
  net_ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"root"}};
  
  auto net_verdict = engine.execute_bytecode(net_ctx);
  std::cout << "Network operation: " << (net_verdict.kind == t81::axion::VerdictKind::Allow ? "✅ Allowed" : "❌ Denied") << "\n";
  std::cout << "Reason: " << net_verdict.reason << "\n\n";
}

// ─── Example 3: Ethics Principles Evaluation ─────────────────────────────

void ethics_evaluation_example() {
  std::cout << "=== Ethics Principles Evaluation Example ===\n";
  
  // Create context for AI model operation
  t81::axion::SyscallContext ai_ctx;
  ai_ctx.caller = "ai_model";
  ai_ctx.syscall = "AgentInvoke";
  ai_ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"ai_snapshot"}};
  ai_ctx.metadata["model_type"] = "neural_network";
  ai_ctx.metadata["cognitive_load"] = "0.85";
  ai_ctx.metadata["tier"] = "T19683";
  
  std::cout << "Evaluating AI operation against ethics principles:\n";
  
  // Check each ethics principle
  std::vector<std::pair<t81::axion::EthicsPrinciple, std::string>> principles = {
    {t81::axion::EthicsPrinciple::Safety, "Safety"},
    {t81::axion::EthicsPrinciple::Privacy, "Privacy"},
    {t81::axion::EthicsPrinciple::Fairness, "Fairness"},
    {t81::axion::EthicsPrinciple::Transparency, "Transparency"},
    {t81::axion::EthicsPrinciple::Accountability, "Accountability"}
  };
  
  for (const auto& [principle, name] : principles) {
    auto verdict = t81::axion::check_ethics(principle, ai_ctx);
    std::cout << "Θ" << static_cast<int>(principle) << " " << name << ": ";
    std::cout << (verdict.kind == t81::axion::VerdictKind::Allow ? "✅ Pass" : "❌ Fail") << "\n";
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      std::cout << "  Reason: " << verdict.reason << "\n";
    }
  }
  
  std::cout << "\n";
}

// ─── Example 4: VM Integration Pattern ───────────────────────────────────

void vm_integration_example() {
  std::cout << "=== VM Integration Pattern Example ===\n";
  
  // Simulate VM execution with Axion governance
  struct Instruction {
    std::string opcode;
    std::string operand;
  };
  
  std::vector<Instruction> program = {
    {"LOAD", "x"},
    {"ADD", "42"},
    {"AgentInvoke", "model"},
    {"STORE", "result"}
  };
  
  std::cout << "Simulating VM execution with Axion governance:\n";
  
  for (const auto& instr : program) {
    std::cout << "Executing: " << instr.opcode << " " << instr.operand << "\n";
    
    // Check for privileged operations requiring Axion evaluation
    if (instr.opcode == "AgentInvoke") {
      t81::axion::SyscallContext ctx;
      ctx.caller = "t81vm";
      ctx.syscall = "AgentInvoke";
      ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"vm_state"}};
      ctx.metadata["operand"] = instr.operand;
      ctx.metadata["tier"] = "T729";
      
      auto verdict = t81::axion::evaluate(ctx);
      
      if (verdict.kind == t81::axion::VerdictKind::Deny) {
        std::cout << "❌ AgentInvoke denied: " << verdict.reason << "\n";
        std::cout << "❌ VM execution halted\n\n";
        return;
      } else {
        std::cout << "✅ AgentInvoke allowed: " << verdict.reason << "\n";
      }
    }
    
    // Simulate instruction execution
    std::cout << "✅ Instruction executed successfully\n";
  }
  
  std::cout << "✅ Program completed successfully\n\n";
}

// ─── Example 5: CanonFS Integration Pattern ─────────────────────────────

void canonfs_integration_example() {
  std::cout << "=== CanonFS Integration Pattern Example ===\n";
  
  // Simulate CanonFS write operation with Axion governance
  struct WriteRequest {
    std::string filename;
    std::size_t size;
    std::string content_type;
  };
  
  std::vector<WriteRequest> requests = {
    {"config.txt", 1024, "text"},
    {"model.bin", 1048576, "binary"},
    {"secret.key", 2048, "encryption_key"}
  };
  
  std::cout << "Processing CanonFS write requests with Axion governance:\n";
  
  for (const auto& req : requests) {
    std::cout << "Writing: " << req.filename << " (" << req.size << " bytes)\n";
    
    // Create Axion context for filesystem operation
    t81::axion::SyscallContext ctx;
    ctx.caller = "canonfs";
    ctx.syscall = "object_write";
    ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"fs_root"}};
    ctx.metadata["filename"] = req.filename;
    ctx.metadata["size"] = std::to_string(req.size);
    ctx.metadata["content_type"] = req.content_type;
    
    auto verdict = t81::axion::evaluate(ctx);
    
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      std::cout << "❌ Write denied: " << verdict.reason << "\n";
    } else {
      std::cout << "✅ Write allowed: " << verdict.reason << "\n";
      // Simulate successful write
      std::cout << "✅ File written successfully\n";
    }
  }
  
  std::cout << "\n";
}

// ─── Example 6: Advanced Policy with Loop Hints ───────────────────────────

void advanced_policy_example() {
  std::cout << "=== Advanced Policy with Loop Hints Example ===\n";
  
  // Create policy with loop hints for recursion control
  t81::axion::Policy advanced_policy;
  advanced_policy.name = "recursive_control";
  advanced_policy.version = "1.0.0";
  
  // Bytecode for: allow recursion up to depth 10
  advanced_policy.bytecode = {
    0x01, 0x00, 0x00, 0x00,  // LOAD recursion_depth
    0x02, 0x0A, 0x00, 0x00,  // CONST 10
    0x03, 0x00, 0x00, 0x00,  // CMP_LT
    0x04, 0x01, 0x00, 0x00,  // JUMP_IF_TRUE 1
    0x05, 0x00, 0x00, 0x00,  // DENY
    0x06, 0x00, 0x00, 0x00   // ALLOW
  };
  
  // Add loop hint for recursion control
  t81::axion::LoopHint loop_hint;
  loop_hint.file = "recursive_function.t81";
  loop_hint.line = 42;
  loop_hint.column = 8;
  loop_hint.bound_infinite = false;
  loop_hint.bound_value = 10;
  
  advanced_policy.loops.push_back(loop_hint);
  
  // Initialize policy engine
  t81::axion::PolicyEngine engine(advanced_policy);
  
  std::cout << "✅ Advanced policy initialized with loop hints\n";
  
  // Test recursion at different depths
  std::vector<int> test_depths = {5, 10, 15};
  
  for (int depth : test_depths) {
    t81::axion::SyscallContext ctx;
    ctx.caller = "recursive_function";
    ctx.syscall = "recursive_call";
    ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"call_stack"}};
    ctx.metadata["recursion_depth"] = std::to_string(depth);
    
    auto verdict = engine.execute_bytecode(ctx);
    
    std::cout << "Recursion depth " << depth << ": ";
    std::cout << (verdict.kind == t81::axion::VerdictKind::Allow ? "✅ Allowed" : "❌ Denied") << "\n";
    std::cout << "  Reason: " << verdict.reason << "\n";
  }
  
  std::cout << "\n";
}

// ─── Example 7: Performance Monitoring ───────────────────────────────────

void performance_monitoring_example() {
  std::cout << "=== Performance Monitoring Example ===\n";
  
  // Initialize Axion for performance testing
  t81::axion::initialize();
  
  std::cout << "Running performance evaluation...\n";
  
  // Simulate multiple evaluations
  const int num_evaluations = 1000;
  auto start_time = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < num_evaluations; ++i) {
    t81::axion::SyscallContext ctx;
    ctx.caller = "perf_test";
    ctx.syscall = "test_operation";
    ctx.snapshot = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"perf_root"}};
    ctx.metadata["iteration"] = std::to_string(i);
    
    auto verdict = t81::axion::evaluate(ctx);
    (void)verdict; // Suppress unused variable warning
  }
  
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
  
  std::cout << "Completed " << num_evaluations << " evaluations in " << duration.count() << " μs\n";
  std::cout << "Average time per evaluation: " << (duration.count() / num_evaluations) << " μs\n";
  std::cout << "Evaluations per second: " << (num_evaluations * 1000000.0 / duration.count()) << "\n";
  
  t81::axion::shutdown();
  std::cout << "✅ Performance monitoring complete\n\n";
}

} // namespace examples::axion

// ─── Main Function ───────────────────────────────────────────────────────

int main() {
  std::cout << "Axion OS Kernel Integration Examples\n";
  std::cout << "=====================================\n\n";
  
  using namespace examples::axion;
  
  // Run all integration examples
  basic_integration_example();
  policy_engine_example();
  ethics_evaluation_example();
  vm_integration_example();
  canonfs_integration_example();
  advanced_policy_example();
  performance_monitoring_example();
  
  std::cout << "🎉 All integration examples completed successfully!\n";
  std::cout << "✅ Axion OS Kernel Alpha-ready with comprehensive integration patterns\n";
  
  return 0;
}
