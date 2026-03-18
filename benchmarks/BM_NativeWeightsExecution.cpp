#include <benchmark/benchmark.h>

#include <cmath>
#include <compare>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/tensor/native.hpp"
#include "t81/tensor/unary.hpp"
#include "t81/types/detail/dmath.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace {

t81::weights::NativeTensor make_native_tensor_for_elements(uint64_t elements) {
  t81::weights::NativeTensor tensor;
  const uint64_t side = static_cast<uint64_t>(std::sqrt(static_cast<double>(elements)));
  tensor.shape = {side, side};
  tensor.trits = side * side;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  const uint64_t limb_count = (tensor.trits + 47) / 48;
  tensor.data.reserve(static_cast<size_t>(limb_count));
  for (uint64_t i = 0; i < limb_count; ++i) {
    tensor.data.push_back((i % 2 == 0) ? 0x123456789abcdef0ULL : 0x0fedcba987654321ULL);
  }
  return tensor;
}

t81::T729DynamicTensor make_host_tensor_for_elements(uint64_t elements) {
  const uint64_t side = static_cast<uint64_t>(std::sqrt(static_cast<double>(elements)));
  std::vector<float> data;
  data.reserve(static_cast<std::size_t>(side * side));
  static constexpr float kPattern[] = {-1.25f, -0.25f, 0.5f, 1.25f};
  for (uint64_t i = 0; i < side * side; ++i) {
    data.push_back(kPattern[i % 4]);
  }
  return t81::T729DynamicTensor::from_host_float_data(
      {static_cast<int>(side), static_cast<int>(side)}, std::move(data),
      t81::TensorNumericClass::HostFloat);
}

t81::tisc::Program make_weights_load_loop_program(uint64_t elements) {
  t81::tisc::Program program;
  program.symbol_pool = {"tensorA"};

  auto model = std::make_shared<t81::weights::ModelFile>();
  model->native["tensorA"] = make_native_tensor_for_elements(elements);
  program.weights_model = model;

  t81::tisc::Insn load_w;
  load_w.opcode = t81::tisc::Opcode::WeightsLoad;
  load_w.a = 1;
  load_w.b = 1;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {load_w, jump};
  return program;
}

t81::tisc::Program make_host_tensor_unary_loop_program(uint64_t elements, t81::tisc::Opcode opcode) {
  t81::tisc::Program program;
  program.tensor_pool.push_back(make_host_tensor_for_elements(elements));

  t81::tisc::Insn load_t;
  load_t.opcode = t81::tisc::Opcode::LoadImm;
  load_t.a = 1;
  load_t.b = 1;
  load_t.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn unary;
  unary.opcode = opcode;
  unary.a = 2;
  unary.b = 1;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {load_t, unary, jump};
  return program;
}

t81::T729DynamicTensor make_host_rmsnorm_weights(uint64_t elements) {
  const uint64_t side = static_cast<uint64_t>(std::sqrt(static_cast<double>(elements)));
  std::vector<float> data;
  data.reserve(static_cast<std::size_t>(side));
  static constexpr float kPattern[] = {0.5f, 0.75f, 1.0f, 1.25f};
  for (uint64_t i = 0; i < side; ++i) {
    data.push_back(kPattern[i % 4]);
  }
  return t81::T729DynamicTensor::from_host_float_data(
      {static_cast<int>(side)}, std::move(data), t81::TensorNumericClass::HostFloat);
}

t81::tisc::Program make_weights_rmsnorm_loop_program(uint64_t elements) {
  t81::tisc::Program program = make_weights_load_loop_program(elements);
  program.tensor_pool.push_back(make_host_rmsnorm_weights(elements));

  t81::tisc::Insn load_w;
  load_w.opcode = t81::tisc::Opcode::LoadImm;
  load_w.a = 2;
  load_w.b = 1;
  load_w.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn trmsnorm;
  trmsnorm.opcode = t81::tisc::Opcode::TRMSNorm;
  trmsnorm.a = 3;
  trmsnorm.b = 1;
  trmsnorm.c = 2;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {program.insns[0], load_w, trmsnorm, jump};
  return program;
}

t81::tisc::Program make_host_tensor_rmsnorm_loop_program(uint64_t elements) {
  t81::tisc::Program program;
  program.tensor_pool.push_back(make_host_tensor_for_elements(elements));
  program.tensor_pool.push_back(make_host_rmsnorm_weights(elements));

  t81::tisc::Insn load_x;
  load_x.opcode = t81::tisc::Opcode::LoadImm;
  load_x.a = 1;
  load_x.b = 1;
  load_x.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn load_w;
  load_w.opcode = t81::tisc::Opcode::LoadImm;
  load_w.a = 2;
  load_w.b = 2;
  load_w.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn trmsnorm;
  trmsnorm.opcode = t81::tisc::Opcode::TRMSNorm;
  trmsnorm.a = 3;
  trmsnorm.b = 1;
  trmsnorm.c = 2;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {load_x, load_w, trmsnorm, jump};
  return program;
}

t81::tisc::Program make_weights_rope_loop_program(uint64_t elements) {
  t81::tisc::Program program = make_weights_load_loop_program(elements);

  t81::tisc::Insn load_pos;
  load_pos.opcode = t81::tisc::Opcode::LoadImm;
  load_pos.a = 2;
  load_pos.b = 3;

  t81::tisc::Insn trope;
  trope.opcode = t81::tisc::Opcode::TRoPE;
  trope.a = 3;
  trope.b = 1;
  trope.c = 2;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {program.insns[0], load_pos, trope, jump};
  return program;
}

t81::tisc::Program make_host_tensor_rope_loop_program(uint64_t elements) {
  t81::tisc::Program program;
  program.tensor_pool.push_back(make_host_tensor_for_elements(elements));

  t81::tisc::Insn load_x;
  load_x.opcode = t81::tisc::Opcode::LoadImm;
  load_x.a = 1;
  load_x.b = 1;
  load_x.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn load_pos;
  load_pos.opcode = t81::tisc::Opcode::LoadImm;
  load_pos.a = 2;
  load_pos.b = 3;

  t81::tisc::Insn trope;
  trope.opcode = t81::tisc::Opcode::TRoPE;
  trope.a = 3;
  trope.b = 1;
  trope.c = 2;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {load_x, load_pos, trope, jump};
  return program;
}

t81::tisc::Program make_weights_expand_loop_program(uint64_t elements) {
  t81::tisc::Program program = make_weights_load_loop_program(elements);

  t81::tisc::Insn texp;
  texp.opcode = t81::tisc::Opcode::TExp;
  texp.a = 2;
  texp.b = 1;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {program.insns[0], texp, jump};
  return program;
}

t81::tisc::Program make_weights_silu_loop_program(uint64_t elements) {
  t81::tisc::Program program = make_weights_load_loop_program(elements);

  t81::tisc::Insn tsilu;
  tsilu.opcode = t81::tisc::Opcode::TSiLU;
  tsilu.a = 2;
  tsilu.b = 1;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {program.insns[0], tsilu, jump};
  return program;
}

t81::tisc::Program make_weights_softmax_loop_program(uint64_t elements) {
  t81::tisc::Program program = make_weights_load_loop_program(elements);

  t81::tisc::Insn tsoftmax;
  tsoftmax.opcode = t81::tisc::Opcode::TSoftmax;
  tsoftmax.a = 2;
  tsoftmax.b = 1;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {program.insns[0], tsoftmax, jump};
  return program;
}

t81::tisc::Program make_weights_promote_loop_program(uint64_t elements) {
  t81::tisc::Program program = make_weights_load_loop_program(elements);

  t81::tisc::Insn tid;
  tid.opcode = t81::tisc::Opcode::TID;
  tid.a = 2;
  tid.b = 1;

  t81::tisc::Insn jump;
  jump.opcode = t81::tisc::Opcode::Jump;
  jump.a = 0;

  program.insns = {program.insns[0], tid, jump};
  return program;
}

t81::T729DynamicTensor exp_balanced_trits_direct(const t81::weights::NativeTensor& native) {
  using DFixed = t81::core::detail::DFixed;

  if (native.format != t81::weights::NativeFormat::BalancedTernary) {
    throw std::invalid_argument("direct packed exp requires BalancedTernary input");
  }

  static const DFixed kExpNegOne = t81::core::detail::exp(-DFixed::one());
  static const DFixed kExpZero = DFixed::one();
  static const DFixed kExpOne = t81::core::detail::exp(DFixed::one());

  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }

  std::vector<DFixed> fixed_data;
  fixed_data.reserve(static_cast<size_t>(remaining));
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<DFixed> block(static_cast<size_t>(count), kExpZero);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<size_t>(i)] = kExpNegOne;
          break;
        case 1:
          block[static_cast<size_t>(i)] = kExpZero;
          break;
        case 2:
          block[static_cast<size_t>(i)] = kExpOne;
          break;
        default:
          throw std::runtime_error("invalid balanced ternary digit");
      }
    }
    fixed_data.insert(fixed_data.end(), block.begin(), block.end());
    remaining -= count;
    if (remaining == 0) {
      break;
    }
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (uint64_t dim : native.shape) {
    shape.push_back(static_cast<int>(dim));
  }
  return t81::T729DynamicTensor::from_canonical_fixed(
      std::move(shape), std::move(fixed_data), t81::TensorNumericClass::HostFloat);
}

std::vector<float> exp_balanced_trits_raw_float(const t81::weights::NativeTensor& native) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary) {
    throw std::invalid_argument("raw float packed exp requires BalancedTernary input");
  }

  static const float kExpNegOne = std::exp(-1.0f);
  static const float kExpZero = 1.0f;
  static const float kExpOne = std::exp(1.0f);

  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }

  std::vector<float> out;
  out.reserve(static_cast<size_t>(remaining));
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<float> block(static_cast<size_t>(count), kExpZero);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<size_t>(i)] = kExpNegOne;
          break;
        case 1:
          block[static_cast<size_t>(i)] = kExpZero;
          break;
        case 2:
          block[static_cast<size_t>(i)] = kExpOne;
          break;
        default:
          throw std::runtime_error("invalid balanced ternary digit");
      }
    }
    out.insert(out.end(), block.begin(), block.end());
    remaining -= count;
    if (remaining == 0) {
      break;
    }
  }
  return out;
}

void benchmark_program_loop(benchmark::State& state, const t81::tisc::Program& program, int steps_per_iter) {
  for (auto _ : state) {
    state.PauseTiming();
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    state.ResumeTiming();
    for (int i = 0; i < steps_per_iter; ++i) {
      auto res = vm->step();
      if (!res.has_value()) {
        state.SkipWithError("VM step failed in native weights benchmark");
        return;
      }
    }
    benchmark::ClobberMemory();
  }
}

}  // namespace

static void BM_NativeWeightsLoad_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_load_loop_program(elements);
  benchmark_program_loop(state, program, 2);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoad_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsPromote_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_promote_loop_program(elements);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsPromote_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndExp_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_expand_loop_program(elements);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndExp_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndExp_Binary(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_host_tensor_unary_loop_program(elements, t81::tisc::Opcode::TExp);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndExp_Binary)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndSiLU_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_silu_loop_program(elements);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndSiLU_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndSiLU_Binary(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_host_tensor_unary_loop_program(elements, t81::tisc::Opcode::TSiLU);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndSiLU_Binary)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndSoftmax_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_softmax_loop_program(elements);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndSoftmax_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndSoftmax_Binary(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_host_tensor_unary_loop_program(elements, t81::tisc::Opcode::TSoftmax);
  benchmark_program_loop(state, program, 3);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndSoftmax_Binary)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndRMSNorm_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_rmsnorm_loop_program(elements);
  benchmark_program_loop(state, program, 4);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndRMSNorm_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndRMSNorm_Binary(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_host_tensor_rmsnorm_loop_program(elements);
  benchmark_program_loop(state, program, 4);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndRMSNorm_Binary)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndRoPE_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_weights_rope_loop_program(elements);
  benchmark_program_loop(state, program, 4);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndRoPE_T81Native)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsLoadAndRoPE_Binary(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto program = make_host_tensor_rope_loop_program(elements);
  benchmark_program_loop(state, program, 4);
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["trits"] = static_cast<double>(elements);
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=structural-advantage; work: trits/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsLoadAndRoPE_Binary)->Arg(64)->RangeMultiplier(4)->Range(256, 65536);

static void BM_NativeWeightsDecode_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto native = make_native_tensor_for_elements(elements);
  for (auto _ : state) {
    auto decoded = t81::tensor_native::decode(native, t81::tensor_native::DecodeMode::StrictCanonical);
    if (!decoded.has_value()) {
      state.SkipWithError("native decode failed");
      return;
    }
    benchmark::DoNotOptimize(decoded->canonical_fixed_data().data());
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=systems-path; work: ops/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsDecode_T81Native)->Arg(8192)->Arg(65536);

static void BM_NativeWeightsDecodeAndExp_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto native = make_native_tensor_for_elements(elements);
  for (auto _ : state) {
    auto decoded = t81::tensor_native::decode(native, t81::tensor_native::DecodeMode::StrictCanonical);
    if (!decoded.has_value()) {
      state.SkipWithError("native decode failed");
      return;
    }
    auto expd = t81::ops::exp(*decoded);
    benchmark::DoNotOptimize(expd.canonical_fixed_data().data());
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=systems-path; work: ops/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsDecodeAndExp_T81Native)->Arg(8192)->Arg(65536);

static void BM_NativeWeightsPackedExp_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto native = make_native_tensor_for_elements(elements);
  for (auto _ : state) {
    auto expd = exp_balanced_trits_direct(native);
    benchmark::DoNotOptimize(expd.canonical_fixed_data().data());
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=systems-path; work: ops/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsPackedExp_T81Native)->Arg(8192)->Arg(65536);

static void BM_NativeWeightsPackedExpRawFloat_T81Native(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto native = make_native_tensor_for_elements(elements);
  for (auto _ : state) {
    auto out = exp_balanced_trits_raw_float(native);
    benchmark::DoNotOptimize(out.data());
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(elements));
  state.counters["work_per_iter"] = static_cast<double>(elements);
  state.SetLabel("comparison=systems-path; work: ops/iter=" + std::to_string(elements));
}
BENCHMARK(BM_NativeWeightsPackedExpRawFloat_T81Native)->Arg(8192)->Arg(65536);
