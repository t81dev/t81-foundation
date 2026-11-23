#include "tools/cli/core/cmd_vm_tisc_ir.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "t81/cli/driver.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/base81_view.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"
#include "tools/cli/core/cmd_determinism.hpp"
#include "tools/cli/core/execution_policy.hpp"
#include "tools/cli/core/help_text.hpp"
#include "tools/cli/logging.hpp"

namespace fs = std::filesystem;

std::string json_escape(std::string_view text);
std::string trap_explanation_text(t81::vm::Trap trap);
std::string sha3_512_text(std::string_view text);
fs::path discover_canonfs_root();
bool has_extension_ci(const fs::path& path, std::string_view expected_ext);
std::shared_ptr<t81::weights::ModelFile> load_weights_model_optional(
    const std::optional<fs::path>& path);
const char* ir_opcode_name(t81::tisc::ir::Opcode op);
const char* ir_primitive_name(t81::tisc::ir::PrimitiveKind kind);
const char* ir_relation_name(t81::tisc::ir::ComparisonRelation relation);
std::string format_ir_operand(const t81::tisc::ir::Operand& operand);

int run_tensor_command(const char* command_name, const Args& args) {
  (void)command_name;
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    print_help_tensor();
    return 0;
  }

  const std::string action = args.command_args[0];
  if (action == "canonize") {
    if (args.command_args.size() != 2) {
      error("tensor canonize requires exactly one input file.");
      return 1;
    }
    return t81::cli::canonize_tensor(args.command_args[1], discover_canonfs_root());
  }
  if (action == "hash") {
    Args hash_args = args;
    hash_args.command = "determinism";
    return run_determinism_command(command_name, hash_args);
  }
  if (action == "inspect") {
    fs::path path;
    bool as_json = false;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (token == "--json") {
        as_json = true;
      } else if (!token.empty() && token[0] == '-') {
        error("tensor inspect: unknown option '" + token + "'. Run 't81 help tensor inspect'.");
        return 1;
      } else if (path.empty()) {
        path = fs::path(token);
      } else {
        error("tensor inspect: unexpected argument '" + token +
              "'. Run 't81 help tensor inspect'.");
        return 1;
      }
    }
    if (path.empty()) {
      error("tensor inspect requires a .t81w file path.");
      return 1;
    }
    if (!has_extension_ci(path, ".t81w")) {
      error("tensor inspect currently supports .t81w artifacts. Use 't81 tensor hash' for raw "
            "files.");
      return 1;
    }
    try {
      auto mf = t81::weights::load_t81w(path);
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.tensor-inspect.v1\",\n"
                  << "  \"ok\": true,\n"
                  << "  \"model\": \"" << json_escape(path.string()) << "\",\n"
                  << "  \"parameters\": " << mf.total_parameters << ",\n"
                  << "  \"trits\": " << mf.total_trits << ",\n"
                  << "  \"file_size_bytes\": " << mf.file_size << ",\n"
                  << "  \"bits_per_trit\": " << std::fixed << std::setprecision(6)
                  << mf.bits_per_trit << ",\n"
                  << "  \"sparsity\": " << std::fixed << std::setprecision(6) << mf.sparsity
                  << ",\n"
                  << "  \"format\": \"" << json_escape(mf.format) << "\",\n"
                  << "  \"checksum_sha3_512\": \"" << json_escape(mf.checksum) << "\"\n"
                  << "}\n";
        std::cout << std::defaultfloat;
        return 0;
      }
      std::cout << "Tensor artifact: " << path << "\n";
      std::cout << "Parameters:      " << t81::weights::format_count(mf.total_parameters) << "\n";
      std::cout << "Trits:           " << t81::weights::format_count(mf.total_trits) << "\n";
      std::cout << "File size:       " << mf.file_size << " bytes\n";
      std::cout << "Bits per trit:   " << std::fixed << std::setprecision(6) << mf.bits_per_trit
                << "\n";
      std::cout << "Sparsity:        " << mf.sparsity << "\n";
      std::cout << "Format:          " << mf.format << "\n";
      std::cout << "Checksum:        " << mf.checksum << "\n";
      std::cout << std::defaultfloat;
      return 0;
    } catch (const std::exception& ex) {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.tensor-inspect.v1\",\n"
                  << "  \"ok\": false,\n"
                  << "  \"model\": \"" << json_escape(path.string()) << "\",\n"
                  << "  \"error\": \"" << json_escape(ex.what()) << "\"\n"
                  << "}\n";
        return 1;
      }
      error(std::string("tensor inspect: ") + ex.what());
      return 1;
    }
  }

  error("tensor: unknown subcommand '" + action + "'. Run 't81 help tensor'.");
  return 1;
}

int run_vm_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_vm(); });
  }

  const std::string action = args.command_args[0];

  // Validate action before parsing further to give a clear "unknown action" error
  // rather than a misleading "requires a .tisc input file" message.
  static constexpr std::string_view kVmActions[] = {"run",   "debug",   "trace",       "until",
                                                    "step",  "regs",    "stack",       "mem",
                                                    "state", "profile", "explain-trap"};
  const bool known_action = std::any_of(std::begin(kVmActions), std::end(kVmActions),
                                        [&](std::string_view a) { return a == action; });
  if (!known_action) {
    error("vm: unknown action '" + action + "'. Run 't81 help vm'.");
    return 1;
  }

  std::optional<fs::path> policy_path = args.policy;
  std::optional<fs::path> trace_path = args.output;
  std::optional<std::string> display_program;
  bool as_json = false;
  std::size_t step_count = 1;
  std::size_t stack_limit = 10;
  std::optional<std::size_t> memory_addr;
  std::optional<std::size_t> target_pc;
  fs::path input;

  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--policy") {
      if (++i >= args.command_args.size()) {
        error("vm: missing value for --policy.");
        return 1;
      }
      policy_path = fs::path(args.command_args[i]);
    } else if (token == "--display-program") {
      if (++i >= args.command_args.size()) {
        error("vm: missing value for --display-program.");
        return 1;
      }
      display_program = args.command_args[i];
    } else if (token == "--json") {
      as_json = true;
    } else if ((token == "--count" || token == "--steps" || token == "--limit" ||
                token == "--addr" || token == "--pc") &&
               i + 1 < args.command_args.size()) {
      std::size_t parsed = 0;
      try {
        parsed = static_cast<std::size_t>(std::stoull(args.command_args[++i]));
      } catch (...) {
        error("vm: invalid numeric value for " + token + ".");
        return 1;
      }
      if (token == "--count" || token == "--steps") {
        step_count = parsed;
      } else if (token == "--limit") {
        stack_limit = parsed;
      } else if (token == "--pc") {
        target_pc = parsed;
      } else {
        memory_addr = parsed;
      }
    } else if (token == "--out" || token == "--output" || token == "--trace-out" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("vm trace: missing value for " + token + ".");
        return 1;
      }
      trace_path = fs::path(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("vm: unknown option '" + token + "'. Run 't81 help vm'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("vm: unexpected argument '" + token + "'. Run 't81 help vm'.");
      return 1;
    }
  }

  if (input.empty()) {
    error("vm " + action + " requires a .tisc input file.");
    return 1;
  }
  if (input.extension() != ".tisc") {
    error("vm " + action + " expects a .tisc input file.");
    return 1;
  }
  const std::string rendered_program = display_program.value_or(input.string());

  if (action == "run") {
    if (trace_path) {
      return t81::cli::run_tisc(input, policy_path, true, trace_path);
    }
    return t81::cli::run_tisc(input, policy_path, false, std::nullopt);
  }
  if (action == "debug") {
    return t81::cli::debug_tisc(input, policy_path);
  }
  if (action == "trace") {
    if (!trace_path) {
      error("vm trace requires -o <trace.txt> or --out <trace.txt>.");
      return 1;
    }
    return t81::cli::run_tisc(input, policy_path, true, trace_path);
  }
  std::optional<std::string> policy_text;
  if (policy_path) {
    std::string content;
    std::string diagnostic;
    if (!t81::cli::detail::read_execution_policy(*policy_path, content, diagnostic)) {
      error(diagnostic);
      return 1;
    }
    policy_text = std::move(content);
  }
  if (action == "until" || action == "step" || action == "regs" || action == "stack" ||
      action == "mem") {
    auto program = t81::tisc::load_program(input.string());
    if (policy_text) program.axion_policy_text = *policy_text;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    if (policy_path && !vm->state().policy.has_value()) {
      error("Could not apply requested policy: " + policy_path->string());
      return 1;
    }
    std::optional<t81::vm::Trap> trap;
    std::size_t executed_steps = 0;
    bool reached_target_pc = false;
    if (action == "until" && target_pc && vm->state().contexts[0].pc == *target_pc) {
      reached_target_pc = true;
    }
    for (; executed_steps < step_count && !vm->state().halted && !reached_target_pc;
         ++executed_steps) {
      auto result = vm->step();
      if (!result) {
        trap = result.error();
        break;
      }
      if (action == "until" && target_pc && vm->state().contexts[0].pc == *target_pc) {
        reached_target_pc = true;
      }
    }
    const auto& state = vm->state();
    const auto& ctx = state.contexts[0];
    const bool ok = !trap.has_value();

    if (action == "until") {
      if (!target_pc) {
        error("vm until requires --pc <n>.");
        return 1;
      }
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-until.v1\",\n"
                  << "  \"ok\": " << ((ok && reached_target_pc) ? "true" : "false") << ",\n"
                  << "  \"target_pc\": " << *target_pc << ",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"steps_requested\": " << step_count << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"reached_target_pc\": " << (reached_target_pc ? "true" : "false") << ",\n"
                  << "  \"halted\": " << (state.halted ? "true" : "false") << ",\n"
                  << "  \"trap\": ";
        if (trap)
          std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else
          std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Target PC:      " << *target_pc << "\n";
        std::cout << "Steps executed: " << executed_steps << "\n";
        std::cout << "PC:             " << ctx.pc << "\n";
        std::cout << "Reached target: " << (reached_target_pc ? "yes" : "no") << "\n";
        std::cout << "Halted:         " << (state.halted ? "yes" : "no") << "\n";
        if (trap) {
          std::cout << "Trap:           " << t81::vm::to_string(*trap) << "\n";
        }
      }
      return (ok && reached_target_pc) ? 0 : 1;
    }

    if (action == "step") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-step.v1\",\n"
                  << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                  << "  \"steps_requested\": " << step_count << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"halted\": " << (state.halted ? "true" : "false") << ",\n"
                  << "  \"trap\": ";
        if (trap)
          std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else
          std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Steps executed: " << executed_steps << "\n";
        std::cout << "PC:             " << ctx.pc << "\n";
        std::cout << "Halted:         " << (state.halted ? "yes" : "no") << "\n";
        if (trap) {
          std::cout << "Trap:           " << t81::vm::to_string(*trap) << "\n";
        }
      }
      return ok ? 0 : 1;
    }

    if (action == "regs") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-regs.v1\",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"registers\": [\n";
        for (std::size_t reg = 0; reg < ctx.registers.size(); ++reg) {
          std::cout << "    {\"index\": " << reg << ", \"value\": " << ctx.registers[reg]
                    << ", \"tag\": " << static_cast<int>(ctx.register_tags[reg]) << "}";
          if (reg + 1 < ctx.registers.size()) std::cout << ",";
          std::cout << "\n";
        }
        std::cout << "  ],\n  \"trap\": ";
        if (trap)
          std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else
          std::cout << "null\n";
        std::cout << "}\n";
      } else {
        for (std::size_t reg = 0; reg < ctx.registers.size(); ++reg) {
          if (reg < 9 || ctx.registers[reg] != 0) {
            std::cout << "R" << reg << ": " << ctx.registers[reg]
                      << " [tag=" << static_cast<int>(ctx.register_tags[reg]) << "]\n";
          }
        }
        std::cout << "PC: " << ctx.pc << "\n";
      }
      return ok ? 0 : 1;
    }

    if (action == "stack") {
      const std::size_t used = ctx.stack_base >= ctx.sp ? (ctx.stack_base - ctx.sp) : 0;
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-stack.v1\",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"stack_used\": " << used << ",\n"
                  << "  \"entries\": [\n";
        std::size_t emitted = 0;
        for (std::size_t addr = ctx.sp; addr < ctx.stack_base && emitted < stack_limit;
             ++addr, ++emitted) {
          std::cout << "    {\"addr\": " << addr << ", \"value\": " << state.memory[addr] << "}";
          if (addr + 1 < ctx.stack_base && emitted + 1 < stack_limit) std::cout << ",";
          std::cout << "\n";
        }
        std::cout << "  ],\n  \"trap\": ";
        if (trap)
          std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else
          std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Stack used: " << used << "\n";
        std::size_t emitted = 0;
        for (std::size_t addr = ctx.sp; addr < ctx.stack_base && emitted < stack_limit;
             ++addr, ++emitted) {
          std::cout << "[" << addr << "] " << state.memory[addr] << "\n";
        }
      }
      return ok ? 0 : 1;
    }

    if (!memory_addr) {
      error("vm mem requires --addr <n>.");
      return 1;
    }
    if (*memory_addr >= state.memory.size()) {
      error("vm mem: address out of bounds.");
      return 1;
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.vm-mem.v1\",\n"
                << "  \"pc\": " << ctx.pc << ",\n"
                << "  \"steps_executed\": " << executed_steps << ",\n"
                << "  \"addr\": " << *memory_addr << ",\n"
                << "  \"value\": " << state.memory[*memory_addr] << ",\n"
                << "  \"tag\": " << static_cast<int>(state.memory_tags[*memory_addr]) << ",\n"
                << "  \"trap\": ";
      if (trap)
        std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
      else
        std::cout << "null\n";
      std::cout << "}\n";
    } else {
      std::cout << "Memory[" << *memory_addr << "]: " << state.memory[*memory_addr]
                << " [tag=" << static_cast<int>(state.memory_tags[*memory_addr]) << "]\n";
    }
    return ok ? 0 : 1;
  }
  if (action == "state") {
    auto program = t81::tisc::load_program(input.string());
    if (policy_text) program.axion_policy_text = *policy_text;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    if (policy_path && !vm->state().policy.has_value()) {
      error("Could not apply requested policy: " + policy_path->string());
      return 1;
    }
    auto result = vm->run_to_halt();
    const auto& state = vm->state();
    const auto& ctx = state.contexts.empty() ? t81::vm::ThreadContext{} : state.contexts.front();
    const bool ok = static_cast<bool>(result);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.vm-state.v1\",\n";
      std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
      std::cout << "  \"program\": \"" << json_escape(rendered_program) << "\",\n";
      std::cout << "  \"halted\": " << (state.halted ? "true" : "false") << ",\n";
      std::cout << "  \"pc\": " << ctx.pc << ",\n";
      std::cout << "  \"sp\": " << ctx.sp << ",\n";
      std::cout << "  \"memory_words\": " << state.memory.size() << ",\n";
      std::cout << "  \"trace_entries\": " << state.trace.size() << ",\n";
      std::cout << "  \"axion_events\": " << state.axion_log.size() << ",\n";
      std::cout << "  \"printed_lines\": " << state.printed_output.size() << ",\n";
      if (ok) {
        std::cout << "  \"trap\": null\n";
      } else {
        std::cout << "  \"trap\": \"" << json_escape(t81::vm::to_string(result.error())) << "\"\n";
      }
      std::cout << "}\n";
      return ok ? 0 : 1;
    }
    std::cout << "Program:       " << rendered_program << "\n";
    std::cout << "Status:        " << (ok ? "ok" : "trap") << "\n";
    std::cout << "Halted:        " << (state.halted ? "yes" : "no") << "\n";
    std::cout << "PC:            " << ctx.pc << "\n";
    std::cout << "SP:            " << ctx.sp << "\n";
    std::cout << "Memory words:  " << state.memory.size() << "\n";
    std::cout << "Trace entries: " << state.trace.size() << "\n";
    std::cout << "Axion events:  " << state.axion_log.size() << "\n";
    std::cout << "Printed lines: " << state.printed_output.size() << "\n";
    if (!ok) {
      std::cout << "Trap:          " << t81::vm::to_string(result.error()) << "\n";
      return 1;
    }
    return 0;
  }
  if (action == "profile" || action == "explain-trap") {
    auto program = t81::tisc::load_program(input.string());
    if (policy_text) program.axion_policy_text = *policy_text;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    if (policy_path && !vm->state().policy.has_value()) {
      error("Could not apply requested policy: " + policy_path->string());
      return 1;
    }
    auto result = vm->run_to_halt();
    const auto& state = vm->state();
    const auto& ctx = state.contexts.empty() ? t81::vm::ThreadContext{} : state.contexts.front();
    const bool ok = static_cast<bool>(result);

    if (action == "profile") {
      if (as_json) {
        std::cout << "{\n";
        std::cout << "  \"schema\": \"t81.vm-profile.v1\",\n";
        std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
        std::cout << "  \"program\": \"" << json_escape(rendered_program) << "\",\n";
        std::cout << "  \"elapsed_us\": null,\n";
        std::cout << "  \"timing_mode\": \"suppressed_for_determinism\",\n";
        std::cout << "  \"trace_entries\": " << state.trace.size() << ",\n";
        std::cout << "  \"axion_events\": " << state.axion_log.size() << ",\n";
        std::cout << "  \"memory_words\": " << state.memory.size() << ",\n";
        std::cout << "  \"stack_peak_usage\": " << state.memory_stats.stack_peak_usage << ",\n";
        std::cout << "  \"heap_peak_usage\": " << state.memory_stats.heap_peak_usage << ",\n";
        std::cout << "  \"tensor_peak_usage\": " << state.memory_stats.tensor_peak_usage << ",\n";
        std::cout << "  \"meta_peak_usage\": " << state.memory_stats.meta_peak_usage << ",\n";
        std::cout << "  \"pc\": " << ctx.pc << ",\n";
        std::cout << "  \"sp\": " << ctx.sp << ",\n";
        if (ok) {
          std::cout << "  \"trap\": null\n";
        } else {
          std::cout << "  \"trap\": \"" << json_escape(t81::vm::to_string(result.error()))
                    << "\"\n";
        }
        std::cout << "}\n";
        return ok ? 0 : 1;
      }
      std::cout << "Program:          " << rendered_program << "\n";
      std::cout << "Elapsed:          suppressed for deterministic output\n";
      std::cout << "Trace entries:    " << state.trace.size() << "\n";
      std::cout << "Axion events:     " << state.axion_log.size() << "\n";
      std::cout << "Memory words:     " << state.memory.size() << "\n";
      std::cout << "Stack peak usage: " << state.memory_stats.stack_peak_usage << "\n";
      std::cout << "Heap peak usage:  " << state.memory_stats.heap_peak_usage << "\n";
      std::cout << "Tensor peak:      " << state.memory_stats.tensor_peak_usage << "\n";
      std::cout << "Meta peak:        " << state.memory_stats.meta_peak_usage << "\n";
      if (!ok) {
        std::cout << "Trap:             " << t81::vm::to_string(result.error()) << "\n";
        return 1;
      }
      return 0;
    }

    const auto trap = ok ? t81::vm::Trap::None : result.error();
    const std::string explanation = trap_explanation_text(trap);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.vm-trap-explain.v1\",\n";
      std::cout << "  \"program\": \"" << json_escape(rendered_program) << "\",\n";
      std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
      if (ok) {
        std::cout << "  \"trap\": null,\n";
      } else {
        std::cout << "  \"trap\": \"" << json_escape(t81::vm::to_string(trap)) << "\",\n";
      }
      std::cout << "  \"explanation\": \"" << json_escape(explanation) << "\"\n";
      std::cout << "}\n";
      return ok ? 0 : 1;
    }
    std::cout << "Program:      " << input.string() << "\n";
    std::cout << "Result:       " << (ok ? "completed" : "trapped") << "\n";
    if (!ok) {
      std::cout << "Trap:         " << t81::vm::to_string(trap) << "\n";
    }
    std::cout << "Explanation:  " << explanation << "\n";
    return ok ? 0 : 1;
  }

  error("vm: unknown action '" + action + "'. Run 't81 help vm'.");
  return 1;
}

int run_profile_command(const Args& args) {
  if (args.input.empty()) {
    error("profile requires a .t81 or .tisc input file.");
    return 1;
  }
  const bool as_json = std::find(args.command_args.begin(), args.command_args.end(), "--json") !=
                       args.command_args.end();
  if (args.input.extension() == ".t81") {
    auto weights_model_ptr = load_weights_model_optional(args.weights_model);
    if (args.weights_model && !weights_model_ptr) {
      return 1;
    }
    const std::string stable_name =
        "t81-profile-" + sha3_512_text(fs::absolute(args.input).string()).substr(0, 24) + ".tisc";
    const fs::path temp_path = fs::temp_directory_path() / stable_name;
    struct TempCleanup {
      fs::path path;
      ~TempCleanup() {
        std::error_code ec;
        fs::remove(path, ec);
      }
    } cleanup{temp_path};
    const bool old_quiet = g_flags.quiet;
    if (as_json) {
      g_flags.quiet = true;
    }
    int rc = t81::cli::compile(args.input, temp_path, {}, {}, weights_model_ptr);
    g_flags.quiet = old_quiet;
    if (rc != 0) {
      return rc;
    }
    Args vm_args = args;
    vm_args.command = "vm";
    vm_args.command_args = {"profile", temp_path.string(), "--display-program",
                            args.input.string()};
    if (args.policy) {
      vm_args.command_args.push_back("--policy");
      vm_args.command_args.push_back(args.policy->string());
    }
    if (as_json) {
      vm_args.command_args.push_back("--json");
    }
    return run_vm_command(vm_args);
  }
  if (args.input.extension() == ".tisc") {
    Args vm_args = args;
    vm_args.command = "vm";
    vm_args.command_args = {"profile", args.input.string()};
    if (args.policy) {
      vm_args.command_args.push_back("--policy");
      vm_args.command_args.push_back(args.policy->string());
    }
    if (as_json) {
      vm_args.command_args.push_back("--json");
    }
    return run_vm_command(vm_args);
  }
  error("profile expects a .t81 or .tisc input file.");
  return 1;
}

int required_tier_for_program(const t81::tisc::Program& program) {
  std::smatch match;
  if (std::regex_search(program.axion_policy_text, match, std::regex(R"(\(tier\s+([0-9]+)\))")) &&
      match.size() == 2) {
    return std::stoi(match[1].str());
  }
  return 1;
}

int run_tier_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_tier(); });
  }

  const std::string action = args.command_args[0];
  bool as_json = false;
  fs::path input;
  int max_tier = 0;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const auto& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--max-tier") {
      if (i + 1 >= args.command_args.size()) {
        error("tier gate: missing value for --max-tier.");
        return 1;
      }
      try {
        max_tier = std::stoi(args.command_args[++i]);
      } catch (...) {
        error("tier gate: invalid value for --max-tier.");
        return 1;
      }
    } else if (!token.empty() && token[0] == '-') {
      error("tier: unknown option '" + token + "'. Run 't81 help tier'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("tier: unexpected argument '" + token + "'. Run 't81 help tier'.");
      return 1;
    }
  }

  if (action == "info") {
    const std::vector<std::pair<int, std::string>> tiers = {
        {1, "Baseline symbolic execution"}, {2, "Reflective execution"},
        {3, "Recursive reasoning"},         {4, "Distributed planning"},
        {5, "Infinite refinement"},         {6, "Universal cognition"},
        {7, "Governed orchestration"},      {8, "Sovereign optimization"},
        {9, "Reserved experimental apex"}};
    if (as_json) {
      std::cout << "{\n  \"schema\": \"t81.tier-info.v1\",\n  \"tiers\": [\n";
      for (std::size_t i = 0; i < tiers.size(); ++i) {
        std::cout << "    {\"tier\":" << tiers[i].first << ",\"label\":\""
                  << json_escape(tiers[i].second) << "\"}";
        if (i + 1 < tiers.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      for (const auto& [tier, label] : tiers) {
        std::cout << "Tier " << tier << ": " << label << "\n";
      }
    }
    return 0;
  }

  if (input.empty() || input.extension() != ".tisc") {
    error("tier " + action + " expects a .tisc input file.");
    return 1;
  }
  auto program = t81::tisc::load_program(input.string());
  const int required_tier = required_tier_for_program(program);

  if (action == "check") {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tier-check.v1\",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"required_tier\": " << required_tier << "\n"
                << "}\n";
    } else {
      std::cout << "Program:       " << input.string() << "\n";
      std::cout << "Required tier: " << required_tier << "\n";
    }
    return 0;
  }

  if (action == "gate") {
    if (max_tier <= 0) {
      error("tier gate requires --max-tier <n>.");
      return 1;
    }
    const bool ok = required_tier <= max_tier;
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tier-gate.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"required_tier\": " << required_tier << ",\n"
                << "  \"max_tier\": " << max_tier << "\n"
                << "}\n";
      return ok ? 0 : 1;
    }
    std::cout << "Program:       " << input.string() << "\n";
    std::cout << "Required tier: " << required_tier << "\n";
    std::cout << "Max tier:      " << max_tier << "\n";
    std::cout << "Result:        " << (ok ? "allowed" : "blocked") << "\n";
    return ok ? 0 : 1;
  }

  error("tier: unknown action '" + action + "'. Run 't81 help tier'.");
  return 1;
}

int run_tisc_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_tisc(); });
  }

  const std::string action = args.command_args[0];

  if (action == "diff") {
    bool as_json = false;
    fs::path input1, input2;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const auto& tok = args.command_args[i];
      if (tok == "--json")
        as_json = true;
      else if (tok == "-h" || tok == "--help") {
        print_help_tisc_diff();
        return 0;
      } else if (input1.empty())
        input1 = fs::path(tok);
      else if (input2.empty())
        input2 = fs::path(tok);
      else {
        error("tisc diff: unexpected argument '" + tok + "'.");
        return 1;
      }
    }
    if (input1.empty() || input2.empty()) {
      error("tisc diff requires two .tisc input files. Run 't81 help tisc diff'.");
      return 1;
    }
    const auto prog1 = t81::tisc::load_program(input1.string());
    const auto prog2 = t81::tisc::load_program(input2.string());
    auto insn_eq = [](const t81::tisc::Insn& x, const t81::tisc::Insn& y) {
      return x.opcode == y.opcode && x.a == y.a && x.b == y.b && x.c == y.c &&
             x.literal_kind == y.literal_kind;
    };
    const std::size_t max_i = std::max(prog1.insns.size(), prog2.insns.size());
    std::size_t diffs = 0;
    for (std::size_t idx = 0; idx < max_i; ++idx) {
      if (idx >= prog1.insns.size() || idx >= prog2.insns.size() ||
          !insn_eq(prog1.insns[idx], prog2.insns[idx])) {
        ++diffs;
      }
    }
    const bool identical = (diffs == 0);
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-diff.v1\",\n"
                << "  \"lhs\": \"" << json_escape(input1.string()) << "\",\n"
                << "  \"rhs\": \"" << json_escape(input2.string()) << "\",\n"
                << "  \"lhs_instructions\": " << prog1.insns.size() << ",\n"
                << "  \"rhs_instructions\": " << prog2.insns.size() << ",\n"
                << "  \"diff_count\": " << diffs << ",\n"
                << "  \"identical\": " << (identical ? "true" : "false") << "\n"
                << "}\n";
    } else {
      if (identical) {
        std::cout << "identical\n";
      } else {
        for (std::size_t idx = 0; idx < max_i; ++idx) {
          if (idx >= prog1.insns.size()) {
            std::cout << "+ [" << idx << "] <rhs extra>\n";
          } else if (idx >= prog2.insns.size()) {
            std::cout << "- [" << idx << "] <lhs extra>\n";
          } else if (!insn_eq(prog1.insns[idx], prog2.insns[idx])) {
            std::cout << "! [" << idx << "] differs\n";
          }
        }
        std::cout << diffs << " instruction(s) differ\n";
      }
    }
    return identical ? 0 : 1;
  }

  bool as_json = false;
  fs::path input;
  std::optional<fs::path> output = args.output;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--out" || token == "--output" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("tisc encode: missing value for " + token + ".");
        return 1;
      }
      output = fs::path(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("tisc: unknown option '" + token + "'. Run 't81 help tisc'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("tisc: unexpected argument '" + token + "'. Run 't81 help tisc'.");
      return 1;
    }
  }

  if (input.empty()) {
    error("tisc " + action + " requires an input file.");
    return 1;
  }
  if (action != "encode" && input.extension() != ".tisc") {
    error("tisc " + action + " expects a .tisc input file.");
    return 1;
  }

  if (action == "disasm") {
    return t81::cli::disasm_tisc(input);
  }
  if (action == "validate") {
    auto program = t81::tisc::load_program(input.string());
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.tisc-validate.v1\",\n";
      std::cout << "  \"ok\": true,\n";
      std::cout << "  \"program\": \"" << json_escape(input.string()) << "\",\n";
      std::cout << "  \"instructions\": " << program.insns.size() << ",\n";
      std::cout << "  \"float_pool\": " << program.float_pool.size() << ",\n";
      std::cout << "  \"tensor_pool\": " << program.tensor_pool.size() << "\n";
      std::cout << "}\n";
    } else {
      std::cout << "TISC artifact valid: " << input.string() << "\n";
      std::cout << "Instructions: " << program.insns.size() << "\n";
      std::cout << "Float pool:   " << program.float_pool.size() << "\n";
      std::cout << "Tensor pool:  " << program.tensor_pool.size() << "\n";
    }
    return 0;
  }
  if (action == "stats") {
    auto program = t81::tisc::load_program(input.string());
    std::map<std::string, std::size_t> opcode_counts;
    for (const auto& insn : program.insns) {
      ++opcode_counts[std::string(t81::tisc::opcode_name(insn.opcode))];
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-stats.v1\",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"instructions\": " << program.insns.size() << ",\n"
                << "  \"opcodes\": [\n";
      std::size_t emitted = 0;
      for (const auto& [opcode, count] : opcode_counts) {
        std::cout << "    {\"opcode\":\"" << json_escape(opcode) << "\",\"count\":" << count << "}";
        if (++emitted < opcode_counts.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "Program:      " << input.string() << "\n";
      std::cout << "Instructions: " << program.insns.size() << "\n";
      std::cout << "Opcodes:\n";
      for (const auto& [opcode, count] : opcode_counts) {
        std::cout << "  " << opcode << ": " << count << "\n";
      }
    }
    return 0;
  }
  if (action == "encode") {
    std::ifstream in(input, std::ios::binary);
    if (!in) {
      error("tisc encode: could not open input file: " + input.string());
      return 1;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto program_res = t81::tisc::base81_view::parse(text);
    if (!program_res) {
      error("tisc encode: failed to parse Base81 view: " + program_res.error().explain().str());
      return 1;
    }
    const fs::path out_path = output.value_or(input.stem().string() + ".tisc");
    t81::tisc::save_program(program_res.value(), out_path.string());
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-encode.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"output\": \"" << json_escape(out_path.string()) << "\",\n"
                << "  \"instructions\": " << program_res.value().insns.size() << "\n"
                << "}\n";
    } else {
      std::cout << "Encoded TISC artifact: " << out_path.string() << "\n";
    }
    return 0;
  }
  if (action == "decode") {
    const auto program = t81::tisc::load_program(input.string());
    const std::string rendered = t81::tisc::base81_view::render(program);
    if (output) {
      std::ofstream out(*output, std::ios::binary | std::ios::trunc);
      if (!out) {
        error("tisc decode: could not open output file: " + output->string());
        return 1;
      }
      out << rendered;
      if (!out.good()) {
        error("tisc decode: failed writing output file: " + output->string());
        return 1;
      }
    } else if (!as_json) {
      std::cout << rendered;
      if (rendered.empty() || rendered.back() != '\n') {
        std::cout << '\n';
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-decode.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"output\": ";
      if (output) {
        std::cout << "\"" << json_escape(output->string()) << "\",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"instructions\": " << program.insns.size() << "\n"
                << "}\n";
    }
    return 0;
  }

  error("tisc: unknown action '" + action + "'. Run 't81 help tisc'.");
  return 1;
}

int run_ir_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_ir(); });
  }

  const std::string action = args.command_args[0];
  bool as_json = false;
  std::optional<fs::path> output = args.output;
  fs::path input;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--out" || token == "--output" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("ir " + action + ": missing value for " + token + ".");
        return 1;
      }
      output = fs::path(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("ir: unknown option '" + token + "'. Run 't81 help ir'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("ir: unexpected argument '" + token + "'. Run 't81 help ir'.");
      return 1;
    }
  }

  if (action != "show" && action != "dump" && action != "export" && action != "validate") {
    error("ir: unknown action '" + action + "'. Run 't81 help ir'.");
    return 1;
  }
  if (input.empty()) {
    error("ir " + action + " requires exactly one .t81 input file.");
    return 1;
  }
  if (input.extension() != ".t81") {
    error("ir " + action + " expects a .t81 input file.");
    return 1;
  }

  std::ifstream file(input);
  if (!file) {
    error("Could not open source file: " + input.string());
    return 1;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    error("IR generation aborted due to parse errors.");
    return 1;
  }

  t81::frontend::SemanticAnalyzer semantic_analyzer(stmts);
  semantic_analyzer.analyze();
  if (semantic_analyzer.had_error()) {
    error("IR generation aborted due to semantic errors.");
    return 1;
  }

  if (action == "validate") {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.ir-validate.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"source\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"statements\": " << stmts.size() << "\n"
                << "}\n";
    } else {
      std::cout << "IR valid: " << input.string() << "\n";
      std::cout << "Statements: " << stmts.size() << "\n";
    }
    return 0;
  }

  t81::frontend::IRGenerator generator;
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();

  const bool render_json = as_json || action == "export";
  if (render_json) {
    std::ostringstream rendered;
    rendered << "{\n"
             << "  \"schema\": \"t81.ir-export.v1\",\n"
             << "  \"source\": \"" << json_escape(input.string()) << "\",\n"
             << "  \"instruction_count\": " << instructions.size() << ",\n"
             << "  \"instructions\": [\n";
    for (std::size_t i = 0; i < instructions.size(); ++i) {
      const auto& inst = instructions[i];
      rendered << "    {\"index\":" << i << ",\"opcode\":\"" << ir_opcode_name(inst.opcode)
               << "\",\"primitive\":\"" << ir_primitive_name(inst.primitive)
               << "\",\"boolean_result\":" << (inst.boolean_result ? "true" : "false")
               << ",\"relation\":\"" << ir_relation_name(inst.relation) << "\",\"operands\":[";
      for (std::size_t j = 0; j < inst.operands.size(); ++j) {
        rendered << "\"" << json_escape(format_ir_operand(inst.operands[j])) << "\"";
        if (j + 1 != inst.operands.size()) {
          rendered << ",";
        }
      }
      rendered << "]}";
      if (i + 1 != instructions.size()) {
        rendered << ",";
      }
      rendered << "\n";
    }
    rendered << "  ]\n"
             << "}\n";
    if (output) {
      std::ofstream out(*output, std::ios::binary | std::ios::trunc);
      if (!out) {
        error("ir export: could not open output file: " + output->string());
        return 1;
      }
      out << rendered.str();
      if (!out.good()) {
        error("ir export: failed writing output file: " + output->string());
        return 1;
      }
    } else {
      std::cout << rendered.str();
    }
    return 0;
  }

  std::cout << "IR Instructions (" << instructions.size() << " total):\n";
  for (const auto& inst : instructions) {
    std::cout << std::setw(20) << std::left << ir_opcode_name(inst.opcode);
    std::cout << " [" << ir_primitive_name(inst.primitive) << (inst.boolean_result ? " Bool" : "")
              << "]";
    if (inst.boolean_result && inst.relation != t81::tisc::ir::ComparisonRelation::None) {
      std::cout << " <" << ir_relation_name(inst.relation) << ">";
    }
    if (!inst.operands.empty()) {
      std::cout << " | ";
      for (std::size_t i = 0; i < inst.operands.size(); ++i) {
        std::cout << format_ir_operand(inst.operands[i]);
        if (i + 1 < inst.operands.size()) {
          std::cout << ", ";
        }
      }
    }
    std::cout << "\n";
  }
  return 0;
}
