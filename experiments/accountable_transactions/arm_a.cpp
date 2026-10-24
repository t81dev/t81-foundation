// Untrusted calculator: witness independently binds and authorizes the effect.
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include "t81/canonfs/axion_hook.hpp"
#include "t81/vm/vm.hpp"
void calculate(int units, int price) {
  if (units < 1 || units > 1000 || price < 1 || price > 1000)
    throw std::runtime_error("bounded inputs required");
  t81::tisc::Program program;
  program.axion_policy_text = "(policy (tier 1) (max-instructions 16))";
  program.insns = {{t81::tisc::Opcode::LoadImm, 40, units, 0},
                   {t81::tisc::Opcode::LoadImm, 41, price, 0},
                   {t81::tisc::Opcode::Mul, 42, 40, 41},
                   {t81::tisc::Opcode::Halt, 0, 0, 0}};
  auto machine = t81::vm::make_interpreter_vm();
  machine->load_program(program);
  const auto start = std::chrono::steady_clock::now();
  if (!machine->run_to_halt(16)) throw std::runtime_error("VM fault");
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start)
          .count();
  std::cout << "{\"amount\":" << machine->state().contexts[0].registers[42]
            << ",\"execution_ns\":" << elapsed << "}\n";
}
int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--stream") {
      int units, price;
      while (std::cin >> units >> price) {
        calculate(units, price);
        std::cout.flush();
      }
    } else {
      if (argc != 3) throw std::runtime_error("usage: accountable_arm_a UNITS PRICE | --stream");
      calculate(std::stoi(argv[1]), std::stoi(argv[2]));
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
