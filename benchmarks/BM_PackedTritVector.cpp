#include <benchmark/benchmark.h>
#include <algorithm>
#include <cstring>
#include <random>
#include <vector>
#include "t81/experimental/packed_trit_vector.hpp"
#include "t81/swar/swar.hpp"

using namespace t81::experimental;

// -----------------------------------------------------------------------------
// SCALAR BENCHMARKS
// -----------------------------------------------------------------------------

static void BM_ScalarTAnd(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v1(len), v2(len), res(len);
  for (size_t i = 0; i < len; ++i) {
    v1[i] = static_cast<int8_t>(dist(rng));
    v2[i] = static_cast<int8_t>(dist(rng));
  }

  for (auto _ : state) {
    for (size_t i = 0; i < len; ++i) {
      res[i] = PackedTritVector::scalar_and(v1[i], v2[i]);
    }
    benchmark::DoNotOptimize(res.data());
  }
}
BENCHMARK(BM_ScalarTAnd)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ScalarTOr(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v1(len), v2(len), res(len);
  for (size_t i = 0; i < len; ++i) {
    v1[i] = static_cast<int8_t>(dist(rng));
    v2[i] = static_cast<int8_t>(dist(rng));
  }

  for (auto _ : state) {
    for (size_t i = 0; i < len; ++i) {
      res[i] = PackedTritVector::scalar_or(v1[i], v2[i]);
    }
    benchmark::DoNotOptimize(res.data());
  }
}
BENCHMARK(BM_ScalarTOr)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ScalarTXor(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v1(len), v2(len), res(len);
  for (size_t i = 0; i < len; ++i) {
    v1[i] = static_cast<int8_t>(dist(rng));
    v2[i] = static_cast<int8_t>(dist(rng));
  }

  for (auto _ : state) {
    for (size_t i = 0; i < len; ++i) {
      res[i] = PackedTritVector::scalar_xor(v1[i], v2[i]);
    }
    benchmark::DoNotOptimize(res.data());
  }
}
BENCHMARK(BM_ScalarTXor)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ScalarTNot(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v1(len), res(len);
  for (size_t i = 0; i < len; ++i) {
    v1[i] = static_cast<int8_t>(dist(rng));
  }

  for (auto _ : state) {
    for (size_t i = 0; i < len; ++i) {
      res[i] = PackedTritVector::scalar_not(v1[i]);
    }
    benchmark::DoNotOptimize(res.data());
  }
}
BENCHMARK(BM_ScalarTNot)->RangeMultiplier(4)->Range(16, 65536);

// -----------------------------------------------------------------------------
// PHASE 1 (PT-5) BENCHMARKS
// -----------------------------------------------------------------------------

static void BM_PackedTAnd(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = PackedTritVector::from_trits(t1).value();
  auto p2 = PackedTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_and(p2).value();
    benchmark::DoNotOptimize(res.packed_data().data());
  }
}
BENCHMARK(BM_PackedTAnd)->RangeMultiplier(4)->Range(16, 65536);

static void BM_PackedTOr(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = PackedTritVector::from_trits(t1).value();
  auto p2 = PackedTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_or(p2).value();
    benchmark::DoNotOptimize(res.packed_data().data());
  }
}
BENCHMARK(BM_PackedTOr)->RangeMultiplier(4)->Range(16, 65536);

static void BM_PackedTXor(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = PackedTritVector::from_trits(t1).value();
  auto p2 = PackedTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_xor(p2).value();
    benchmark::DoNotOptimize(res.packed_data().data());
  }
}
BENCHMARK(BM_PackedTXor)->RangeMultiplier(4)->Range(16, 65536);

static void BM_PackedTNot(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = PackedTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = p1.t_not().value();
    benchmark::DoNotOptimize(res.packed_data().data());
  }
}
BENCHMARK(BM_PackedTNot)->RangeMultiplier(4)->Range(16, 65536);

static void BM_PackPT5(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }

  for (auto _ : state) {
    auto res = PackedTritVector::from_trits(t1).value();
    benchmark::DoNotOptimize(res.packed_data().data());
  }
}
BENCHMARK(BM_PackPT5)->RangeMultiplier(4)->Range(16, 65536);

static void BM_UnpackPT5(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = PackedTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = p1.to_trits().value();
    benchmark::DoNotOptimize(res.data());
  }
}
BENCHMARK(BM_UnpackPT5)->RangeMultiplier(4)->Range(16, 65536);

// -----------------------------------------------------------------------------
// PHASE 2B / 2C / 2D (ComputeTritVector) BENCHMARKS
// -----------------------------------------------------------------------------

// Phase 2C: SWAR Baseline (Explicit Call)
static void BM_ComputeTAnd_Phase2C_SWAR(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::vector<int8_t> t1(len), t2(len);
  std::uniform_int_distribution<int> dist(-1, 1);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = t81::swar::t_and_swar(p1, p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTAnd_Phase2C_SWAR)->RangeMultiplier(4)->Range(16, 65536);

// Phase 2D: AVX2 / Native (via default API)
static void BM_ComputeTAnd_Phase2D_AVX2(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::vector<int8_t> t1(len), t2(len);
  std::uniform_int_distribution<int> dist(-1, 1);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_and(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTAnd_Phase2D_AVX2)->RangeMultiplier(4)->Range(16, 65536);

// Phase 2D: In-Place API
static void BM_ComputeTAnd_Phase2D_InPlace(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::vector<int8_t> t1(len), t2(len);
  std::uniform_int_distribution<int> dist(-1, 1);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    p1.t_and_inplace(p2);
    benchmark::DoNotOptimize(p1.data().data());
  }
}
BENCHMARK(BM_ComputeTAnd_Phase2D_InPlace)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTOr_Phase2C_SWAR(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = t81::swar::t_or_swar(p1, p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTOr_Phase2C_SWAR)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTOr_Phase2D_AVX2(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_or(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTOr_Phase2D_AVX2)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTOr_Phase2D_InPlace(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    p1.t_or_inplace(p2);
    benchmark::DoNotOptimize(p1.data().data());
  }
}
BENCHMARK(BM_ComputeTOr_Phase2D_InPlace)->RangeMultiplier(4)->Range(16, 65536);

// Phase 2C: TXor remains LUT
static void BM_ComputeTXor_Phase2C(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_xor(p2).value();  // Fallback
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTXor_Phase2C)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTNot_Phase2C_SWAR(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = t81::swar::t_not_swar(p1).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTNot_Phase2C_SWAR)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTNot_Phase2D_AVX2(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = p1.t_not().value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTNot_Phase2D_AVX2)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTNot_Phase2D_InPlace(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();

  for (auto _ : state) {
    p1.t_not_inplace();
    benchmark::DoNotOptimize(p1.data().data());
  }
}
BENCHMARK(BM_ComputeTNot_Phase2D_InPlace)->RangeMultiplier(4)->Range(16, 65536);

// Phase 2B: LUT Explicit

static void BM_ComputeTAnd_Phase2B_LUT(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_and_lut(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTAnd_Phase2B_LUT)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTOr_Phase2B_LUT(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_or_lut(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTOr_Phase2B_LUT)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTXor_Phase2B_LUT(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_xor_lut(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTXor_Phase2B_LUT)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTNot_Phase2B_LUT(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = p1.t_not_lut().value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTNot_Phase2B_LUT)->RangeMultiplier(4)->Range(16, 65536);

// -----------------------------------------------------------------------------
// PHASE 2A (Reference / Naive) BENCHMARKS
// -----------------------------------------------------------------------------

static void BM_ComputeTAnd_Phase2A(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_and_ref(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTAnd_Phase2A)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTOr_Phase2A(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_or_ref(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTOr_Phase2A)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTXor_Phase2A(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  for (auto _ : state) {
    auto res = p1.t_xor_ref(p2).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTXor_Phase2A)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeTNot_Phase2A(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = p1.t_not_ref().value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeTNot_Phase2A)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputePack(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }

  for (auto _ : state) {
    auto res = ComputeTritVector::from_trits(t1).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputePack)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeUnpack(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = p1.to_trits().value();
    benchmark::DoNotOptimize(res.data());
  }
}
BENCHMARK(BM_ComputeUnpack)->RangeMultiplier(4)->Range(16, 65536);

static void BM_ComputeFromPhase1(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = PackedTritVector::from_trits(t1).value();

  for (auto _ : state) {
    auto res = ComputeTritVector::from_phase1(p1).value();
    benchmark::DoNotOptimize(res.data().data());
  }
}
BENCHMARK(BM_ComputeFromPhase1)->RangeMultiplier(4)->Range(16, 65536);

// -----------------------------------------------------------------------------
// PHASE 5: ALLOCATION-FREE MICROBENCHMARK
// -----------------------------------------------------------------------------

static void BM_ComputeTritVector_ComputeOnly(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }

  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  // Pre-allocate raw buffers to simulate reuse
  std::vector<uint8_t> dst = p1.data();
  const std::vector<uint8_t>& src_a = p1.data();
  const std::vector<uint8_t>& src_b = p2.data();
  size_t n = dst.size();

  for (auto _ : state) {
    // Raw SWAR TAnd loop implementation (Logic from t_and_swar)
    const uint8_t* a_ptr = src_a.data();
    const uint8_t* b_ptr = src_b.data();
    uint8_t* dst_ptr = dst.data();

    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
      uint64_t a, b;
      std::memcpy(&a, a_ptr + i, 8);
      std::memcpy(&b, b_ptr + i, 8);

      uint64_t H = (a | b) & 0xAAAAAAAAAAAAAAAAULL;
      uint64_t L_content = (a & b) & 0x5555555555555555ULL;
      uint64_t res = H | (H >> 1) | L_content;

      std::memcpy(dst_ptr + i, &res, 8);
    }
    for (; i < n; ++i) {
      uint8_t a = a_ptr[i];
      uint8_t b = b_ptr[i];
      uint8_t H = (a | b) & 0xAA;
      uint8_t L_content = (a & b) & 0x55;
      dst_ptr[i] = H | (H >> 1) | L_content;
    }
    benchmark::DoNotOptimize(dst_ptr);
  }
}
BENCHMARK(BM_ComputeTritVector_ComputeOnly)->RangeMultiplier(4)->Range(16, 65536);

// -----------------------------------------------------------------------------
// PHASE 6: REAL-WORKLOAD BENCHMARK
// -----------------------------------------------------------------------------
// Workload: a = (a AND b); c = (c OR a); d = NOT(d)

static void BM_RealWorkload_Phase2C(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v_a(len), v_b(len), v_c(len), v_d(len);
  for (size_t i = 0; i < len; ++i) {
    v_a[i] = dist(rng);
    v_b[i] = dist(rng);
    v_c[i] = dist(rng);
    v_d[i] = dist(rng);
  }

  auto a = ComputeTritVector::from_trits(v_a).value();
  auto b = ComputeTritVector::from_trits(v_b).value();
  auto c = ComputeTritVector::from_trits(v_c).value();
  auto d = ComputeTritVector::from_trits(v_d).value();

  for (auto _ : state) {
    // We must re-assign to simulate state updates
    // Note: This includes allocation overhead as per current API
    a = a.t_and(b).value();
    c = c.t_or(a).value();
    d = d.t_not().value();
  }
}
BENCHMARK(BM_RealWorkload_Phase2C)->RangeMultiplier(4)->Range(16, 65536);

static void BM_RealWorkload_Phase2B_LUT(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v_a(len), v_b(len), v_c(len), v_d(len);
  for (size_t i = 0; i < len; ++i) {
    v_a[i] = dist(rng);
    v_b[i] = dist(rng);
    v_c[i] = dist(rng);
    v_d[i] = dist(rng);
  }

  auto a = ComputeTritVector::from_trits(v_a).value();
  auto b = ComputeTritVector::from_trits(v_b).value();
  auto c = ComputeTritVector::from_trits(v_c).value();
  auto d = ComputeTritVector::from_trits(v_d).value();

  for (auto _ : state) {
    a = a.t_and_lut(b).value();
    c = c.t_or_lut(a).value();
    d = d.t_not_lut().value();
  }
}
BENCHMARK(BM_RealWorkload_Phase2B_LUT)->RangeMultiplier(4)->Range(16, 65536);

// -----------------------------------------------------------------------------
// PHASE 7: PURE KERNEL BENCHMARKS (Exposed via Public API)
// -----------------------------------------------------------------------------

static void BM_Kernel_TAnd_SWAR(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  // Use raw pointers
  const uint8_t* src_a = p1.data().data();
  const uint8_t* src_b = p2.data().data();
  // Use a mutable buffer for output to avoid allocation in loop
  std::vector<uint8_t> dst = p1.data();
  uint8_t* dst_ptr = dst.data();
  size_t n = dst.size();

  for (auto _ : state) {
    ComputeTritVector::kernel_and_swar(src_a, src_b, dst_ptr, n);
    benchmark::DoNotOptimize(dst_ptr);
  }
}
BENCHMARK(BM_Kernel_TAnd_SWAR)->RangeMultiplier(4)->Range(16, 65536);

#if defined(__x86_64__) && defined(__AVX2__)
static void BM_Kernel_TAnd_AVX2(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  const uint8_t* src_a = p1.data().data();
  const uint8_t* src_b = p2.data().data();
  std::vector<uint8_t> dst = p1.data();
  uint8_t* dst_ptr = dst.data();
  size_t n = dst.size();

  for (auto _ : state) {
    ComputeTritVector::kernel_and_avx2(src_a, src_b, dst_ptr, n);
    benchmark::DoNotOptimize(dst_ptr);
  }
}
BENCHMARK(BM_Kernel_TAnd_AVX2)->RangeMultiplier(4)->Range(16, 65536);
#endif

#if defined(__aarch64__) && defined(__ARM_NEON)
static void BM_Kernel_TAnd_NEON(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> t1(len), t2(len);
  for (size_t i = 0; i < len; ++i) {
    t1[i] = static_cast<int8_t>(dist(rng));
    t2[i] = static_cast<int8_t>(dist(rng));
  }
  auto p1 = ComputeTritVector::from_trits(t1).value();
  auto p2 = ComputeTritVector::from_trits(t2).value();

  const uint8_t* src_a = p1.data().data();
  const uint8_t* src_b = p2.data().data();
  std::vector<uint8_t> dst = p1.data();
  uint8_t* dst_ptr = dst.data();
  size_t n = dst.size();

  for (auto _ : state) {
    ComputeTritVector::kernel_and_neon(src_a, src_b, dst_ptr, n);
    benchmark::DoNotOptimize(dst_ptr);
  }
}
BENCHMARK(BM_Kernel_TAnd_NEON)->RangeMultiplier(4)->Range(16, 65536);
#endif

// Real Workload In-Place
static void BM_RealWorkload_Phase2D_InPlace(benchmark::State& state) {
  size_t len = state.range(0);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);
  std::vector<int8_t> v_a(len), v_b(len), v_c(len), v_d(len);
  for (size_t i = 0; i < len; ++i) {
    v_a[i] = dist(rng);
    v_b[i] = dist(rng);
    v_c[i] = dist(rng);
    v_d[i] = dist(rng);
  }

  auto a = ComputeTritVector::from_trits(v_a).value();
  auto b = ComputeTritVector::from_trits(v_b).value();
  auto c = ComputeTritVector::from_trits(v_c).value();
  auto d = ComputeTritVector::from_trits(v_d).value();

  for (auto _ : state) {
    // a = a AND b
    a.t_and_inplace(b);
    // c = c OR a
    c.t_or_inplace(a);
    // d = NOT d
    d.t_not_inplace();

    benchmark::DoNotOptimize(a.data().data());
    benchmark::DoNotOptimize(c.data().data());
    benchmark::DoNotOptimize(d.data().data());
  }
}
BENCHMARK(BM_RealWorkload_Phase2D_InPlace)->RangeMultiplier(4)->Range(16, 65536);
