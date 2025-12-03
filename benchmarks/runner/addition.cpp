#include <benchmark/benchmark.h>
#include <array>
#include <cstdint>
#include <random>

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Limb.hpp"

namespace {
constexpr size_t kBinaryLimbs = 1024 / 64;

template <typename Value>
Value RandomTernaryValue(std::mt19937_64& rng) {
  Value out;
  for (size_t i = 0; i < Value::num_trits(); ++i) {
    const int trit = static_cast<int>(rng() % 3) - 1;
    out[i] = t81::int_to_trit(trit);
  }
  return out;
}

struct TernaryWide4096 {
  std::array<t81::T81Int<1024>, 4> chunks{};
};

TernaryWide4096 random_wide_ternary(std::mt19937_64& rng) {
  TernaryWide4096 wide;
  for (auto& chunk : wide.chunks) {
    chunk = RandomTernaryValue<t81::T81Int<1024>>(rng);
  }
  return wide;
}

TernaryWide4096 add_wide(const TernaryWide4096& lhs, const TernaryWide4096& rhs) {
  TernaryWide4096 out;
  for (size_t i = 0; i < lhs.chunks.size(); ++i) {
    out.chunks[i] = lhs.chunks[i] + rhs.chunks[i];
  }
  return out;
}
}  // namespace

namespace {
template <size_t LIMBS>
void RunBinaryCarry(benchmark::State& state, uint64_t seed) {
  alignas(64) std::array<uint64_t, LIMBS> a{};
  alignas(64) std::array<uint64_t, LIMBS> b{};
  alignas(64) std::array<uint64_t, LIMBS> result{};
  std::mt19937_64 rng(seed);
  for (auto& v : a) v = rng();
  for (auto& v : b) v = rng();

  for (auto _ : state) {
    uint64_t carry = 0;
    for (size_t i = 0; i < LIMBS; ++i) {
      __uint128_t tmp = static_cast<__uint128_t>(a[i]) + b[i] + carry;
      result[i] = static_cast<uint64_t>(tmp);
      carry = static_cast<uint64_t>(tmp >> 64);
    }
    benchmark::DoNotOptimize(result);
    benchmark::DoNotOptimize(carry);
  }
  state.SetItemsProcessed(state.iterations() * LIMBS);
}
}  // namespace

static void BM_Add_1024_bit_binary_carry_propagate(benchmark::State& state) {
  RunBinaryCarry<kBinaryLimbs>(state, 0xC0FFEE);
}
BENCHMARK(BM_Add_1024_bit_binary_carry_propagate)
    ->Name("BM_Add_1024_bit/binary_carry_propagate");

static void BM_Add_1024_bit_binary_checked(benchmark::State& state) {
  alignas(64) std::array<uint64_t, kBinaryLimbs> a{};
  alignas(64) std::array<uint64_t, kBinaryLimbs> b{};
  alignas(64) std::array<uint64_t, kBinaryLimbs + 1> result{};
  std::mt19937_64 rng(0xC0FFEE);
  for (auto& v : a) v = rng();
  for (auto& v : b) v = rng();

  for (auto _ : state) {
    uint64_t carry = 0;
    for (size_t i = 0; i < kBinaryLimbs; ++i) {
      __uint128_t tmp = static_cast<__uint128_t>(a[i]) + b[i] + carry;
      result[i] = static_cast<uint64_t>(tmp);
      carry = static_cast<uint64_t>(tmp >> 64);
    }
    result[kBinaryLimbs] = carry;
    benchmark::DoNotOptimize(result);
    benchmark::DoNotOptimize(carry);
  }
  state.SetItemsProcessed(state.iterations() * kBinaryLimbs);
}
BENCHMARK(BM_Add_1024_bit_binary_checked)
    ->Name("BM_Add_1024_bit/binary_checked");

static void BM_Add_2048_bit_binary_carry_propagate(benchmark::State& state) {
  RunBinaryCarry<32>(state, 0xBEEFBEEF);
}
BENCHMARK(BM_Add_2048_bit_binary_carry_propagate)
    ->Name("BM_Add_2048_bit/binary_carry_propagate");

static void BM_Add_4096_bit_binary_carry_propagate(benchmark::State& state) {
  RunBinaryCarry<64>(state, 0xFEEDFACE);
}
BENCHMARK(BM_Add_4096_bit_binary_carry_propagate)
    ->Name("BM_Add_4096_bit/binary_carry_propagate");

static void BM_Add_8192_bit_binary_carry_propagate(benchmark::State& state) {
  RunBinaryCarry<128>(state, 0xDEADBEEF);
}
BENCHMARK(BM_Add_8192_bit_binary_carry_propagate)
    ->Name("BM_Add_8192_bit/binary_carry_propagate");

static void BM_Add_1024_bit_ternary_koggestone(benchmark::State& state) {
  std::mt19937_64 rng(0xC0FFEE);
  auto a = RandomTernaryValue<t81::T81Int<1024>>(rng);
  auto b = RandomTernaryValue<t81::T81Int<1024>>(rng);
  for (auto _ : state) {
    auto result = a + b;
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Add_1024_bit_ternary_koggestone)
    ->Name("BM_Add_1024_bit/ternary_koggestone");

static void BM_Add_4096_bit_ternary_koggestone(benchmark::State& state) {
  std::mt19937_64 rng(0xAFFE5ED);
  auto a = random_wide_ternary(rng);
  auto b = random_wide_ternary(rng);
  for (auto _ : state) {
    auto result = add_wide(a, b);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Add_4096_bit_ternary_koggestone)
    ->Name("BM_Add_4096_bit/ternary_koggestone");

static void BM_Add_2048_bit_ternary_koggestone(benchmark::State& state) {
  std::mt19937_64 rng(0x1337BEEF);
  auto a = RandomTernaryValue<t81::T81Int<2048>>(rng);
  auto b = RandomTernaryValue<t81::T81Int<2048>>(rng);
  for (auto _ : state) {
    auto result = a + b;
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Add_2048_bit_ternary_koggestone)
    ->Name("BM_Add_2048_bit/ternary_koggestone");

static void BM_Add_8192_bit_ternary_koggestone(benchmark::State& state) {
  constexpr size_t kChunks = 4;
  struct Block {
    std::array<t81::T81Int<2048>, kChunks> segments{};
  };
  std::mt19937_64 rng(0xDEADBEEF);
  auto make_block = [&]() {
    Block blk;
    for (auto& seg : blk.segments) {
      seg = RandomTernaryValue<t81::T81Int<2048>>(rng);
    }
    return blk;
  };
  auto a = make_block();
  auto b = make_block();
  for (auto _ : state) {
    Block result{};
    for (size_t i = 0; i < kChunks; ++i) {
      result.segments[i] = a.segments[i] + b.segments[i];
    }
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * kChunks);
}
BENCHMARK(BM_Add_8192_bit_ternary_koggestone)
    ->Name("BM_Add_8192_bit/ternary_koggestone");

static void BM_Add_16384_bit_ternary_koggestone(benchmark::State& state) {
  constexpr size_t kChunks = 8;
  struct Block {
    std::array<t81::T81Int<2048>, kChunks> segments{};
  };
  std::mt19937_64 rng(0xB16B00B5);
  auto make_block = [&]() {
    Block blk;
    for (auto& seg : blk.segments) {
      seg = RandomTernaryValue<t81::T81Int<2048>>(rng);
    }
    return blk;
  };
  auto a = make_block();
  auto b = make_block();
  for (auto _ : state) {
    Block result{};
    for (size_t i = 0; i < kChunks; ++i) {
      result.segments[i] = a.segments[i] + b.segments[i];
    }
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * kChunks);
}
BENCHMARK(BM_Add_16384_bit_ternary_koggestone)
    ->Name("BM_Add_16384_bit/ternary_koggestone");

static void BM_Add_16384_bit_binary_carry_propagate(benchmark::State& state) {
 RunBinaryCarry<256>(state, 0xC0DE1234);
}
BENCHMARK(BM_Add_16384_bit_binary_carry_propagate)
    ->Name("BM_Add_16384_bit/binary_carry_propagate");
