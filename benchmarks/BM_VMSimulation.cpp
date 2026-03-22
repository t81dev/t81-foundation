#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "t81/packed_trit_vector.hpp"

using t81::ComputeTritVector;
using t81::PackedTritVector;

enum Opcode {
  OP_HALT = 0,
  OP_TAND = 1,
};

struct Instruction {
  uint8_t opcode;
  uint8_t arg1;
  uint8_t arg2;
  uint8_t out;
};

// Simulate VM dispatch loop
static void BM_VMSimulation_Dispatch(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<int8_t> t1(len), t2(len);
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  // Create instructions
  std::vector<Instruction> instructions;
  // Loop 1000 times
  for (int i=0; i<1000; ++i) {
      instructions.push_back({OP_TAND, 0, 1, 0});
  }
  instructions.push_back({OP_HALT, 0, 0, 0});

  // VM State
  ComputeTritVector* regs[2] = { &p1, &p2 };

  for (auto _ : state) {
      size_t pc = 0;
      bool running = true;
      while (running) {
          const auto& instr = instructions[pc++];
          switch (instr.opcode) {
              case OP_HALT: running = false; break;
              case OP_TAND:
                  regs[instr.out]->t_and_inplace(*regs[instr.arg2]);
                  break;
          }
      }
      benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_VMSimulation_Dispatch)->Arg(32)->Arg(256);

// Compare against direct loop (Native Call Overhead)
static void BM_NativeCall_Loop(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<int8_t> t1(len), t2(len);
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
      for (int i=0; i<1000; ++i) {
          p1.t_and_inplace(p2);
      }
      benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_NativeCall_Loop)->Arg(32)->Arg(256);
