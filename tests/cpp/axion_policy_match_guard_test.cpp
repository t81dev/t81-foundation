#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string>
#include <string_view>

int main() {
  constexpr std::string_view source = R"(
      enum Color {
          Red;
          Blue(i32);
      };

      fn main() -> i32 {
          match (Color.Blue(42)) {
              Red => 0;
              Blue(v) if v > 10 => v;
          };
          return 0;
      }
  )";

  auto program_opt = t81::cli::build_program_from_source(std::string(source), "<policy-match>");
  if (!program_opt) {
    std::cerr << "Failed to compile policy match program\n";
    return 1;
  }

  auto program = *program_opt;
  program.axion_policy_text = R"(
(policy
  (tier 1)
  (require-match-guard
    (enum Color)
    (variant Blue)
    (payload i32)
    (result pass)))
)";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result) {
    std::cerr << "Guard policy run trapped: " << static_cast<int>(result.error()) << '\n';
    return 1;
  }

  auto program_fail = *program_opt;
  program_fail.axion_policy_text = R"(
(policy
  (tier 1)
  (require-match-guard
    (enum Color)
    (variant Red)
    (result pass)))
)";

  auto vm_fail = t81::vm::make_interpreter_vm();
  vm_fail->load_program(program_fail);
  auto fail_result = vm_fail->run_to_halt();
  if (fail_result.has_value()) {
    std::cerr << "Policy guard failure did not trap\n";
    return 1;
  }
  if (fail_result.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "Expected security fault, got " << static_cast<int>(fail_result.error()) << '\n';
    return 1;
  }

  return 0;
}
