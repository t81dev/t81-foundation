// BM_InferenceComparison.cpp
//
// Reproducible inference benchmark: T81 ternary ops vs FP32 and simulated
// FP16 baselines for dot product, matrix multiply, and attention.
//
// Comparison scope (see label field of each benchmark):
//   path=fp32-mul-acc   — dense float matmul: MUL + ADD per weight element
//   path=fp16-sim       — FP16 storage, float compute (CPU w/o hw FP16 MACs)
//   path=t81-ternary    — T81 ops-layer: conditional ADD/SUB, zero multiply
//
// Memory efficiency per weight element:
//   FP32    : 4 bytes  (32 bits)
//   FP16    : 2 bytes  (16 bits)
//   Ternary : ~0.25 bytes (2 bits packed; 4 trits/byte)
//
// Throughput metric: weight elements processed per second (items/s).
// The SetItemsProcessed call drives the items/s column in benchmark output.
//
// NOTE: T81 ops-layer benchmarks use the pure reference path (snapshot_values).
// The VM dispatch path (packed trit storage, TISC opcodes) is separately
// benchmarked in BM_NativeWeightsExecution.cpp.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

#include <benchmark/benchmark.h>

#include "t81/tensor.hpp"
#include "t81/tensor/ternary_native.hpp"

// ── Portable FP16 simulation ────────────────────────────────────────────────
// On CPU there are no scalar FP16 MACs without AVX512-FP16 (Sapphire Rapids+).
// The realistic FP16 inference path on commodity CPUs loads uint16_t weights
// and promotes them to float before arithmetic.  We simulate this faithfully.

namespace {

static uint16_t float_to_fp16(float f) noexcept {
  uint32_t x = 0;
  std::memcpy(&x, &f, sizeof(x));
  const uint32_t sign  = (x >> 31) & 0x1u;
  const int32_t  exp32 = static_cast<int32_t>((x >> 23) & 0xFFu) - 127;
  const uint32_t frac  = x & 0x7FFFFFu;
  if (exp32 > 15)  return static_cast<uint16_t>((sign << 15) | 0x7C00u);  // inf
  if (exp32 < -14) return static_cast<uint16_t>((sign << 15));              // zero/denorm
  const uint32_t exp16  = static_cast<uint32_t>(exp32 + 15) & 0x1Fu;
  const uint32_t frac16 = frac >> 13;
  return static_cast<uint16_t>((sign << 15) | (exp16 << 10) | frac16);
}

static float fp16_to_float(uint16_t h) noexcept {
  const uint32_t sign  = (h >> 15) & 0x1u;
  const uint32_t exp16 = (h >> 10) & 0x1Fu;
  const uint32_t frac  = h & 0x3FFu;
  uint32_t x = 0;
  if (exp16 == 0) {
    x = (sign << 31) | (frac << 13);  // denormal
  } else if (exp16 == 31) {
    x = (sign << 31) | 0x7F800000u | (frac << 13);  // inf/nan
  } else {
    x = (sign << 31) | ((exp16 + 112u) << 23) | (frac << 13);
  }
  float out = 0.0f;
  std::memcpy(&out, &x, sizeof(out));
  return out;
}

// ── Tensor construction helpers ─────────────────────────────────────────────

// Ternary-valued floats distributed ~50% zeros (Bernoulli with p=0.5).
// index % 3: 0→-1, 1→0, 2→+1  gives roughly equal trit density.
static std::vector<float> make_ternary_weights(int n) {
  std::vector<float> v(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    const int r = i % 3;
    v[static_cast<std::size_t>(i)] = (r == 0) ? -1.0f : (r == 1) ? 0.0f : 1.0f;
  }
  return v;
}

// Float activations in ternary domain (same values, for fair comparison).
static std::vector<float> make_ternary_activations(int n) {
  return make_ternary_weights(n);
}

// FP16-packed weights: convert ternary floats → uint16_t storage.
static std::vector<uint16_t> make_fp16_weights(int n) {
  const auto fvals = make_ternary_weights(n);
  std::vector<uint16_t> h(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    h[static_cast<std::size_t>(i)] = float_to_fp16(fvals[static_cast<std::size_t>(i)]);
  }
  return h;
}

// T729DynamicTensor wrapper.
static T729DynamicTensor make_tensor(std::vector<int> shape, std::vector<float> data) {
  return T729DynamicTensor(std::move(shape), std::move(data));
}

}  // namespace

// ============================================================================
// 1. Dot product: FP32 MUL-ACC vs FP16-sim vs T81 TERNACCUM
// ============================================================================
// Work unit: N weight elements.  Each op is weight×activation.

static void BM_DotProduct_FP32(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  const auto w   = make_ternary_weights(N);
  const auto act = make_ternary_activations(N);

  for (auto _ : state) {
    float acc = 0.0f;
    for (int i = 0; i < N; ++i) {
      acc += w[static_cast<std::size_t>(i)] * act[static_cast<std::size_t>(i)];
    }
    benchmark::DoNotOptimize(acc);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * N);
  state.SetLabel("path=fp32-mul-acc; op=multiply-accumulate");
  state.counters["N"]               = static_cast<double>(N);
  state.counters["weight_bytes_fp32"] = static_cast<double>(N) * 4.0;
}
BENCHMARK(BM_DotProduct_FP32)->RangeMultiplier(4)->Range(64, 16384)->Repetitions(2);

// ─────────────────────────────────────────────────────────────────────────────

static void BM_DotProduct_FP16Sim(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  const auto w16  = make_fp16_weights(N);
  const auto act  = make_ternary_activations(N);

  for (auto _ : state) {
    float acc = 0.0f;
    for (int i = 0; i < N; ++i) {
      const float wf = fp16_to_float(w16[static_cast<std::size_t>(i)]);
      acc += wf * act[static_cast<std::size_t>(i)];
    }
    benchmark::DoNotOptimize(acc);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * N);
  state.SetLabel("path=fp16-sim; op=load-promote-multiply-accumulate");
  state.counters["N"]               = static_cast<double>(N);
  state.counters["weight_bytes_fp16"] = static_cast<double>(N) * 2.0;
}
BENCHMARK(BM_DotProduct_FP16Sim)->RangeMultiplier(4)->Range(64, 16384)->Repetitions(2);

// ─────────────────────────────────────────────────────────────────────────────

static void BM_DotProduct_T81Ternary(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  const auto wv  = make_ternary_weights(N);
  const auto av  = make_ternary_activations(N);
  auto wt  = make_tensor({1, N}, wv);
  auto act = make_tensor({1, N}, av);

  for (auto _ : state) {
    auto result = t81::ops::ternaccum(wt, act);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * N);
  state.SetLabel("path=t81-ternary; op=conditional-add-sub; zero-multiply");
  state.counters["N"]                   = static_cast<double>(N);
  state.counters["weight_bytes_ternary"] = static_cast<double>((N + 3) / 4);  // 2 bits/trit
}
BENCHMARK(BM_DotProduct_T81Ternary)->RangeMultiplier(4)->Range(64, 16384)->Repetitions(2);

// ============================================================================
// 2. Matrix multiply (M=1 inference vector): FP32 vs FP16-sim vs TWMATMUL
// ============================================================================
// Matrix: 1 × K activation @ K × K weight → 1 × K output.
// Work unit: K*K weight elements (ops performed).

static void BM_MatMul_FP32(benchmark::State& state) {
  const int K = static_cast<int>(state.range(0));
  const auto w   = make_ternary_weights(K * K);
  const auto act = make_ternary_activations(K);
  std::vector<float> out(static_cast<std::size_t>(K), 0.0f);

  for (auto _ : state) {
    std::fill(out.begin(), out.end(), 0.0f);
    for (int k = 0; k < K; ++k) {
      const float av = act[static_cast<std::size_t>(k)];
      for (int n = 0; n < K; ++n) {
        out[static_cast<std::size_t>(n)] += av * w[static_cast<std::size_t>(k) * K + n];
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=fp32-mul-acc; op=vector-matrix-multiply");
  state.counters["K"]               = static_cast<double>(K);
  state.counters["weight_bytes_fp32"] = static_cast<double>(K) * K * 4.0;
}
BENCHMARK(BM_MatMul_FP32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Repetitions(2);

// ─────────────────────────────────────────────────────────────────────────────

static void BM_MatMul_FP16Sim(benchmark::State& state) {
  const int K    = static_cast<int>(state.range(0));
  const auto w16 = make_fp16_weights(K * K);
  const auto act = make_ternary_activations(K);
  std::vector<float> out(static_cast<std::size_t>(K), 0.0f);

  for (auto _ : state) {
    std::fill(out.begin(), out.end(), 0.0f);
    for (int k = 0; k < K; ++k) {
      const float av = act[static_cast<std::size_t>(k)];
      for (int n = 0; n < K; ++n) {
        const float wf = fp16_to_float(w16[static_cast<std::size_t>(k) * K + n]);
        out[static_cast<std::size_t>(n)] += av * wf;
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=fp16-sim; op=vector-matrix-multiply");
  state.counters["K"]               = static_cast<double>(K);
  state.counters["weight_bytes_fp16"] = static_cast<double>(K) * K * 2.0;
}
BENCHMARK(BM_MatMul_FP16Sim)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Repetitions(2);

// ─────────────────────────────────────────────────────────────────────────────

static void BM_MatMul_T81Ternary(benchmark::State& state) {
  const int K   = static_cast<int>(state.range(0));
  const auto wv = make_ternary_weights(K * K);
  const auto av = make_ternary_activations(K);
  // activations: 1 × K;  weights: K × K
  auto weights = make_tensor({K, K}, wv);
  auto acts    = make_tensor({1, K}, av);

  for (auto _ : state) {
    auto result = t81::ops::twmatmul(acts, weights);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=t81-ternary; op=twmatmul-cond-add-sub; zero-multiply");
  state.counters["K"]                   = static_cast<double>(K);
  state.counters["weight_bytes_ternary"] = static_cast<double>((K * K + 3) / 4);
}
BENCHMARK(BM_MatMul_T81Ternary)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Repetitions(2);

// ============================================================================
// 3. Attention: FP32 vs T81 TATTN
// ============================================================================
// Shape: seq_len × head_dim for Q, K, V.  head_dim fixed at 64.
// Work unit: seq_len * head_dim weight elements in Q and K.

static constexpr int kHeadDim = 64;

static void BM_Attention_FP32(benchmark::State& state) {
  const int S  = static_cast<int>(state.range(0));
  const int D  = kHeadDim;
  const auto q_data = make_ternary_activations(S * D);
  const auto k_data = make_ternary_activations(S * D);
  const auto v_data = make_ternary_activations(S * D);  // V kept as float

  for (auto _ : state) {
    // Q · Kᵀ  →  S × S scores
    std::vector<float> scores(static_cast<std::size_t>(S) * S, 0.0f);
    for (int i = 0; i < S; ++i) {
      for (int j = 0; j < S; ++j) {
        float dot = 0.0f;
        for (int d = 0; d < D; ++d) {
          dot += q_data[static_cast<std::size_t>(i) * D + d] *
                 k_data[static_cast<std::size_t>(j) * D + d];
        }
        scores[static_cast<std::size_t>(i) * S + j] = dot / std::sqrt(static_cast<float>(D));
      }
    }
    // Softmax rows
    for (int i = 0; i < S; ++i) {
      float mx = *std::max_element(scores.begin() + i * S,
                                   scores.begin() + (i + 1) * S);
      float sm = 0.0f;
      for (int j = 0; j < S; ++j) {
        scores[static_cast<std::size_t>(i) * S + j] =
            std::exp(scores[static_cast<std::size_t>(i) * S + j] - mx);
        sm += scores[static_cast<std::size_t>(i) * S + j];
      }
      for (int j = 0; j < S; ++j) {
        scores[static_cast<std::size_t>(i) * S + j] /= sm;
      }
    }
    // scores · V  →  S × D output
    std::vector<float> out(static_cast<std::size_t>(S) * D, 0.0f);
    for (int i = 0; i < S; ++i) {
      for (int p = 0; p < S; ++p) {
        const float alpha = scores[static_cast<std::size_t>(i) * S + p];
        for (int d = 0; d < D; ++d) {
          out[static_cast<std::size_t>(i) * D + d] +=
              alpha * v_data[static_cast<std::size_t>(p) * D + d];
        }
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  // Work unit: Q·Kᵀ dominates at S*S*D multiplies
  state.SetItemsProcessed(state.iterations() * S * S * D);
  state.SetLabel("path=fp32-mul-acc; op=qkt-softmax-v; head_dim=64");
  state.counters["seq_len"]             = static_cast<double>(S);
  state.counters["weight_bytes_fp32_qk"] = static_cast<double>(S) * D * 4.0 * 2.0;  // Q+K
}
BENCHMARK(BM_Attention_FP32)->Arg(32)->Arg(64)->Arg(128)->Repetitions(2);

// ─────────────────────────────────────────────────────────────────────────────

static void BM_Attention_T81Ternary(benchmark::State& state) {
  const int S = static_cast<int>(state.range(0));
  const int D = kHeadDim;

  auto q = make_tensor({S, D}, make_ternary_activations(S * D));
  auto k = make_tensor({S, D}, make_ternary_activations(S * D));
  auto v = make_tensor({S, D}, make_ternary_activations(S * D));

  for (auto _ : state) {
    auto result = t81::ops::tattn(q, k, v);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * S * S * D);
  state.SetLabel("path=t81-ternary; op=tattn-twmatmul-softmax-v; zero-multiply-in-qkt; head_dim=64");
  state.counters["seq_len"]                  = static_cast<double>(S);
  state.counters["weight_bytes_ternary_qk"]  = static_cast<double>((S * D * 2 + 3) / 4);
}
BENCHMARK(BM_Attention_T81Ternary)->Arg(32)->Arg(64)->Arg(128)->Repetitions(2);

// ============================================================================
// 4. Memory-efficiency snapshot: weight loading throughput by format
// ============================================================================
// Measures how fast N weight values can be loaded into a float accumulator.
// Isolates the memory-bandwidth effect from the compute difference.

static void BM_WeightLoad_FP32(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  const auto w = make_ternary_weights(N);
  for (auto _ : state) {
    float s = 0.0f;
    for (int i = 0; i < N; ++i) s += w[static_cast<std::size_t>(i)];
    benchmark::DoNotOptimize(s);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * N * 4);
  state.SetLabel("path=fp32; op=sequential-load");
  state.counters["weight_bytes_fp32"] = static_cast<double>(N) * 4.0;
}
BENCHMARK(BM_WeightLoad_FP32)->RangeMultiplier(4)->Range(1024, 65536)->Repetitions(2);

static void BM_WeightLoad_FP16Sim(benchmark::State& state) {
  const int N    = static_cast<int>(state.range(0));
  const auto w16 = make_fp16_weights(N);
  for (auto _ : state) {
    float s = 0.0f;
    for (int i = 0; i < N; ++i) s += fp16_to_float(w16[static_cast<std::size_t>(i)]);
    benchmark::DoNotOptimize(s);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * N * 2);
  state.SetLabel("path=fp16-sim; op=load-promote");
  state.counters["weight_bytes_fp16"] = static_cast<double>(N) * 2.0;
}
BENCHMARK(BM_WeightLoad_FP16Sim)->RangeMultiplier(4)->Range(1024, 65536)->Repetitions(2);

static void BM_WeightLoad_TernaryPacked(benchmark::State& state) {
  // Simulate 2-bit packed trit storage: 4 trits per byte.
  const int N      = static_cast<int>(state.range(0));
  const int nbytes = (N + 3) / 4;
  // Pack: trit i lives in bits [2*(i%4), 2*(i%4)+1] of byte i/4.
  // Values: 0b00=-1, 0b01=0, 0b10=+1
  std::vector<uint8_t> packed(static_cast<std::size_t>(nbytes));
  for (int i = 0; i < N; ++i) {
    const int r = i % 3;  // same distribution as make_ternary_weights
    const uint8_t bits = (r == 0) ? 0b00u : (r == 1) ? 0b01u : 0b10u;
    packed[static_cast<std::size_t>(i / 4)] |=
        static_cast<uint8_t>(bits << (2 * (i % 4)));
  }
  static constexpr float kTritTable[4] = {-1.0f, 0.0f, 1.0f, 0.0f};

  for (auto _ : state) {
    float s = 0.0f;
    for (int i = 0; i < N; ++i) {
      const uint8_t bits =
          (packed[static_cast<std::size_t>(i / 4)] >> (2 * (i % 4))) & 0b11u;
      s += kTritTable[bits];
    }
    benchmark::DoNotOptimize(s);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * nbytes);
  state.SetLabel("path=ternary-packed; op=unpack-2bit-trit; 4-trits-per-byte");
  state.counters["weight_bytes_ternary"] = static_cast<double>(nbytes);
  state.counters["memory_ratio_vs_fp32"] = static_cast<double>(N) * 4.0 / nbytes;
}
BENCHMARK(BM_WeightLoad_TernaryPacked)->RangeMultiplier(4)->Range(1024, 65536)->Repetitions(2);
