#include "debugger.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "internal/tooling/logging.hpp"
#include "t81/isa/opcodes.hpp"

namespace t81::cli {

namespace {
const char* tier_id_name(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
      return "Tier0";
    case t81::cog::TierId::Tier1:
      return "Tier1";
    case t81::cog::TierId::Tier2:
      return "Tier2";
    case t81::cog::TierId::Tier3:
      return "Tier3";
    case t81::cog::TierId::Tier4:
      return "Tier4";
    case t81::cog::TierId::Tier5:
      return "Tier5";
  }
  return "Unknown";
}

bool should_print_tier(std::optional<int> filter, int tier) {
  return !filter.has_value() || *filter == tier;
}
}  // namespace

Debugger::Debugger(std::unique_ptr<t81::vm::IVirtualMachine> vm, t81::tisc::Program program)
    : vm_(std::move(vm)), program_(std::move(program)) {}

void Debugger::run() {
  info("T81VM Debugger active. Type 'h' for help.");

  std::string line;
  while (!quit_ && !vm_->state().halted) {
    print_current_instruction();
    std::cout << "dbg> " << std::flush;
    if (!std::getline(std::cin, line)) break;

    if (line.empty()) continue;

    std::istringstream iss(line);
    char cmd;
    iss >> cmd;

    switch (cmd) {
      case 'h':
        print_help();
        break;
      case 's': {
        auto res = vm_->step();
        if (!res) {
          error("Execution trapped: " + std::string(t81::vm::to_string(res.error())));
        }
        break;
      }
      case 'p': {
        std::string pattern;
        if (iss >> pattern) {
          policy_breakpoints_.insert(pattern);
          info("Policy breakpoint set for: " + pattern);
        } else {
          error("Usage: p <pattern>");
        }
        break;
      }
      case 'c': {
        while (!vm_->state().halted) {
          auto res = vm_->step();
          if (!res) {
            error("Execution trapped: " + std::string(t81::vm::to_string(res.error())));
            break;
          }
          if (breakpoints_.count(vm_->state().contexts[0].pc)) {
            info("Breakpoint hit at PC=" + std::to_string(vm_->state().contexts[0].pc));
            break;
          }
          bool stop = false;
          const auto& log = vm_->state().axion_log;
          if (!log.empty()) {
            const auto& last_event = log.back();
            for (const auto& pat : policy_breakpoints_) {
              if (last_event.verdict.reason.find(pat) != std::string::npos) {
                info("Policy breakpoint hit: " + pat);
                info("Reason: " + last_event.verdict.reason);
                stop = true;
                break;
              }
            }
          }
          // Check watchpoints
          for (auto& [addr, last_val] : watchpoints_) {
            if (addr < vm_->state().memory.size()) {
              auto current_val = vm_->state().memory[addr];
              if (current_val != last_val) {
                info("Watchpoint hit at address " + std::to_string(addr) + ": " +
                     std::to_string(last_val) + " -> " + std::to_string(current_val));
                last_val = current_val;  // Update
                stop = true;
                break;
              }
            }
          }
          if (stop) break;
        }
        break;
      }
      case 'w': {
        std::size_t addr;
        if (iss >> addr) {
          if (addr < vm_->state().memory.size()) {
            watchpoints_[addr] = vm_->state().memory[addr];
            info("Watchpoint set at address " + std::to_string(addr));
          } else {
            error("Address out of bounds");
          }
        } else {
          error("Usage: w <addr>");
        }
        break;
      }
      case 'b': {
        std::size_t pc;
        if (iss >> pc) {
          breakpoints_.insert(pc);
          info("Breakpoint set at PC=" + std::to_string(pc));
        } else {
          error("Usage: b <pc>");
        }
        break;
      }
      case 'r':
        print_registers();
        break;
      case 'k':
        print_stack();
        break;
      case 'm': {
        std::size_t addr;
        if (iss >> addr) {
          print_memory(addr);
        } else {
          error("Usage: m <addr>");
        }
        break;
      }
      case 'l':
        print_list();
        break;
      case 't': {
        std::string arg;
        if (!(iss >> arg) || arg == "all") {
          print_tier_state(std::nullopt);
          break;
        }
        int tier = 0;
        try {
          tier = std::stoi(arg);
        } catch (...) {
          error("Usage: t [1|2|3|4|5|all]");
          break;
        }
        if (tier < 1 || tier > 5) {
          error("Usage: t [1|2|3|4|5|all]");
          break;
        }
        print_tier_state(tier);
        break;
      }
      case 'q':
        quit_ = true;
        break;
      default:
        error("Unknown command. Type 'h' for help.");
    }
  }

  if (vm_->state().halted) {
    info("Program halted.");
  }
}

void Debugger::print_help() {
  std::cout << "Debugger commands:\n"
            << "  s          Step one instruction\n"
            << "  c          Continue execution\n"
            << "  b <pc>     Set breakpoint at PC\n"
            << "  w <addr>   Set watchpoint at address\n"
            << "  p <pat>    Set policy breakpoint (on trace string match)\n"
            << "  r          Print registers\n"
            << "  k          Print stack\n"
            << "  m <addr>   Print memory at address\n"
            << "  t [tier]   Inspect cognitive tiers (1-5 or all)\n"
            << "  l          List surrounding instructions\n"
            << "  h          Show this help\n"
            << "  q          Quit debugger\n";
}

void Debugger::print_registers() {
  const auto& state = vm_->state();
  std::cout << "Registers:\n";
  // Print common registers
  for (int i = 0; i < 9; ++i) {
    std::cout << "  R" << std::setw(2) << std::left << i << ": " << state.contexts[0].registers[i]
              << "\n";
  }
  // Print non-zero others
  for (int i = 9; i < 243; ++i) {
    if (state.contexts[0].registers[i] != 0) {
      std::cout << "  R" << i << ": " << state.contexts[0].registers[i] << "\n";
    }
  }
  std::cout << "  PC: " << state.contexts[0].pc << "\n";
  std::cout << "  SP: " << state.contexts[0].sp << "\n";
  std::cout << "  Flags: " << (state.contexts[0].flags.zero ? "Z" : "")
            << (state.contexts[0].flags.negative ? "N" : "")
            << (state.contexts[0].flags.positive ? "P" : "") << "\n";
}

void Debugger::print_stack() {
  const auto& state = vm_->state();
  std::cout << "Stack (top 10):\n";
  std::size_t count = 0;

  if (state.contexts[0].sp == 0) {
    std::cout << "  <empty>\n";
    return;
  }

  for (std::size_t i = state.contexts[0].sp; i > 0 && count < 10; --i, ++count) {
    std::size_t addr = i - 1;
    if (addr < state.memory.size()) {
      std::cout << "  [" << addr << "]: " << state.memory[addr] << "\n";
    }
  }
}

void Debugger::print_current_instruction() {
  const auto& state = vm_->state();
  if (state.halted) return;

  if (state.contexts[0].pc < program_.insns.size()) {
    const auto& insn = program_.insns[state.contexts[0].pc];
    std::cout << "[" << std::setw(4) << state.contexts[0].pc << "] "
              << t81::tisc::opcode_name(insn.opcode);
    std::cout << " " << insn.a << ", " << insn.b << ", " << insn.c;
    if (insn.literal_kind != t81::tisc::LiteralKind::Int) {
      std::cout << " (lit=" << static_cast<int>(insn.literal_kind) << ")";
    }
    std::cout << "\n";
  } else {
    std::cout << "[" << std::setw(4) << state.contexts[0].pc << "] <invalid PC>\n";
  }
}

void Debugger::print_list() {
  const auto& state = vm_->state();
  std::size_t start = (state.contexts[0].pc >= 5) ? state.contexts[0].pc - 5 : 0;
  std::size_t end = std::min(state.contexts[0].pc + 6, program_.insns.size());

  for (std::size_t i = start; i < end; ++i) {
    const auto& insn = program_.insns[i];
    if (i == state.contexts[0].pc)
      std::cout << "-> ";
    else
      std::cout << "   ";

    std::cout << "[" << std::setw(4) << i << "] " << t81::tisc::opcode_name(insn.opcode) << " "
              << insn.a << ", " << insn.b << ", " << insn.c << "\n";
  }
}

void Debugger::print_memory(std::size_t addr) {
  const auto& state = vm_->state();
  if (addr >= state.memory.size()) {
    error("Address out of bounds");
    return;
  }
  std::cout << "Memory[" << addr << "]: " << state.memory[addr] << "\n";
}

void Debugger::print_tier_state(std::optional<int> tier_filter) {
  const auto& state = vm_->state();
  const auto& ctx = state.contexts[0];
  std::cout << "Cognitive tier state:\n";
  std::cout << "  Active tier: " << tier_id_name(ctx.tier_status.current);
  if (!ctx.tier_status.label.empty()) {
    std::cout << " (" << ctx.tier_status.label << ")";
  }
  std::cout << "\n";

  if (should_print_tier(tier_filter, 1)) {
    std::cout << "  Tier1 symbolic: graphs=" << state.symbolic_graphs.size()
              << ", total_nodes=" << state.total_symbolic_nodes << "\n";
  }
  if (should_print_tier(tier_filter, 2)) {
    std::size_t sealed = 0;
    for (const auto& frame : ctx.tier2_frames) {
      if (frame.sealed) ++sealed;
    }
    std::cout << "  Tier2 reflective: frames=" << ctx.tier2_frames.size()
              << ", sealed=" << sealed << "\n";
  }
  if (should_print_tier(tier_filter, 3)) {
    std::cout << "  Tier3 recursive: depth=" << ctx.tier3_recursor.current_depth << "/"
              << ctx.tier3_recursor.max_depth
              << ", proofs=" << ctx.tier3_recursor.proofs.size() << "\n";
  }
  if (should_print_tier(tier_filter, 4)) {
    std::cout << "  Tier4 distributed: tick=" << state.tier4_state.vector.global_tick
              << ", inbox=" << state.tier4_state.inbox.size()
              << ", outbox=" << state.tier4_state.outbox.size() << "\n";
  }
  if (should_print_tier(tier_filter, 5)) {
    std::size_t active_forms = 0;
    std::size_t convergent = 0;
    std::size_t lazy_terms = 0;
    for (const auto& form : state.infinite_forms) {
      if (!form.has_value()) continue;
      ++active_forms;
      if (form->is_convergent) ++convergent;
      lazy_terms += form->lazy_terms.size();
    }
    std::cout << "  Tier5 infinite: forms=" << active_forms << ", convergent=" << convergent
              << ", lazy_terms=" << lazy_terms << "\n";
  }
}

}  // namespace t81::cli
