#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace t81::cli {

class Debugger {
public:
  Debugger(std::unique_ptr<t81::vm::IVirtualMachine> vm, t81::tisc::Program program);
  void run();

private:
  void print_help();
  void print_registers();
  void print_stack();
  void print_current_instruction();
  void print_list();
  void print_memory(std::size_t addr);
  void print_tier_state(std::optional<int> tier_filter);

  std::unique_ptr<t81::vm::IVirtualMachine> vm_;
  t81::tisc::Program program_;
  std::set<std::size_t> breakpoints_;
  std::set<std::string> policy_breakpoints_;
  std::map<std::size_t, std::int64_t> watchpoints_;
  bool quit_{false};
};

}  // namespace t81::cli
