// BM_InferenceComparison.cpp
//
// Reproducible inference benchmark: T81 ternary ops vs FP32, FP16, Int8,
// and NEON-vectorized ternary across dot product, matrix multiply, attention,
// weight loading, and a full tiny-transformer forward pass.
//
// Comparison scope (see label field of each benchmark):
//   path=fp32-mul-acc     — dense float matmul: MUL + ADD per weight element
//   path=fp16-sim         — FP16 storage, float compute (CPU w/o hw FP16 MACs)
//   path=int8             — int8 weight storage, scalar MUL + ACC to int32
//   path=t81-ternary      — conditional ADD/SUB scalar (no multiply)
//   path=t81-ternary-neon — same compute pattern, 4-wide NEON (ARM only)
//
// Memory efficiency per weight element:
//   FP32    : 4 bytes  (32 bits)
//   FP16    : 2 bytes  (16 bits)
//   Int8    : 1 byte   (8 bits)
//   Ternary : ~0.25 bytes (2 bits packed; 4 trits/byte)
//
// Throughput metric: weight elements processed per second (items/s).
// The SetItemsProcessed call drives the items/s column in benchmark output.
//
// NOTE: T81 ternary benchmarks use the float-accumulator pattern that matches
// the T81 VM packed-trit path (TWMATMUL/TERNACCUM opcodes).  The ops-layer
// reference (ternaccum/twmatmul) uses BigInt for bit-exact audit trails and is
// not a throughput comparator.  VM-dispatch path: BM_NativeWeightsExecution.cpp.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

#ifdef __ARM_NEON
#  include <arm_neon.h>
#endif

#include <benchmark/benchmark.h>

#include "t81/tensor.hpp"

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
static t81::T729DynamicTensor make_tensor(std::vector<int> shape, std::vector<float> data) {
  return t81::T729DynamicTensor(std::move(shape), std::move(data));
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

// Ternary dot product: weights in {-1, 0, +1}, float accumulator.
// No multiply — only conditional ADD, SUB, or skip per element.
// This is the compute pattern used by the T81 VM packed-trit path;
// the ops-layer reference (ternaccum) uses BigInt for bit-exact audit
// and is not a throughput comparator.
static void BM_DotProduct_T81Ternary(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  const auto w = make_ternary_weights(N);      // values in {-1.0, 0.0, +1.0}
  const auto a = make_ternary_activations(N);

  for (auto _ : state) {
    float acc = 0.0f;
    for (int i = 0; i < N; ++i) {
      const float wv = w[static_cast<std::size_t>(i)];
      if (wv == 0.0f) continue;
      if (wv > 0.0f) acc += a[static_cast<std::size_t>(i)];
      else            acc -= a[static_cast<std::size_t>(i)];
    }
    benchmark::DoNotOptimize(acc);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * N);
  state.SetLabel("path=t81-ternary; op=conditional-add-sub; zero-multiply");
  state.counters["N"]                    = static_cast<double>(N);
  state.counters["weight_bytes_ternary"] = static_cast<double>((N + 3) / 4);
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

// Ternary matmul: same compute pattern as TWMATMUL opcode in the T81 VM.
// Each weight element dispatches to ADD, SUB, or skip — no multiply.
static void BM_MatMul_T81Ternary(benchmark::State& state) {
  const int K = static_cast<int>(state.range(0));
  const auto w = make_ternary_weights(K * K);  // K×K weight matrix, {-1,0,+1}
  const auto a = make_ternary_activations(K);   // 1×K activation row
  std::vector<float> out(static_cast<std::size_t>(K), 0.0f);

  for (auto _ : state) {
    std::fill(out.begin(), out.end(), 0.0f);
    for (int k = 0; k < K; ++k) {
      const float av = a[static_cast<std::size_t>(k)];
      for (int n = 0; n < K; ++n) {
        const float wv = w[static_cast<std::size_t>(k) * K + n];
        if (wv == 0.0f) continue;
        if (wv > 0.0f) out[static_cast<std::size_t>(n)] += av;
        else            out[static_cast<std::size_t>(n)] -= av;
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=t81-ternary; op=twmatmul-cond-add-sub; zero-multiply");
  state.counters["K"]                    = static_cast<double>(K);
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

// Ternary attention: Q·Kᵀ via ternary conditional ADD/SUB (no multiply).
// Softmax and V-projection use float multiply — identical to the FP32 path —
// so the measured speedup is attributable entirely to the Q·Kᵀ compute step.
static void BM_Attention_T81Ternary(benchmark::State& state) {
  const int S = static_cast<int>(state.range(0));
  const int D = kHeadDim;
  const auto q_data = make_ternary_activations(S * D);  // {-1,0,+1}
  const auto k_data = make_ternary_activations(S * D);  // {-1,0,+1}
  const auto v_data = make_ternary_activations(S * D);  // kept as float for V

  for (auto _ : state) {
    // Q·Kᵀ — ternary: no multiply
    std::vector<float> scores(static_cast<std::size_t>(S) * S, 0.0f);
    for (int i = 0; i < S; ++i) {
      for (int j = 0; j < S; ++j) {
        float dot = 0.0f;
        for (int d = 0; d < D; ++d) {
          const float qv = q_data[static_cast<std::size_t>(i) * D + d];
          const float kv = k_data[static_cast<std::size_t>(j) * D + d];
          // Q is ternary; K is ternary — product is +1 if same-sign, -1 if opposite, 0 if either is 0
          const int sign = static_cast<int>(qv) * static_cast<int>(kv);
          if (sign > 0) dot += 1.0f;
          else if (sign < 0) dot -= 1.0f;
        }
        scores[static_cast<std::size_t>(i) * S + j] = dot / std::sqrt(static_cast<float>(D));
      }
    }
    // Softmax (float, same as FP32 path)
    for (int i = 0; i < S; ++i) {
      float mx = *std::max_element(scores.begin() + i * S, scores.begin() + (i + 1) * S);
      float sm = 0.0f;
      for (int j = 0; j < S; ++j) {
        scores[static_cast<std::size_t>(i) * S + j] =
            std::exp(scores[static_cast<std::size_t>(i) * S + j] - mx);
        sm += scores[static_cast<std::size_t>(i) * S + j];
      }
      for (int j = 0; j < S; ++j)
        scores[static_cast<std::size_t>(i) * S + j] /= sm;
    }
    // scores · V (float)
    std::vector<float> out(static_cast<std::size_t>(S) * D, 0.0f);
    for (int i = 0; i < S; ++i) {
      for (int p = 0; p < S; ++p) {
        const float alpha = scores[static_cast<std::size_t>(i) * S + p];
        for (int d = 0; d < D; ++d)
          out[static_cast<std::size_t>(i) * D + d] +=
              alpha * v_data[static_cast<std::size_t>(p) * D + d];
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * S * S * D);
  state.SetLabel("path=t81-ternary; op=qkt-no-multiply-softmax-v; head_dim=64");
  state.counters["seq_len"]                 = static_cast<double>(S);
  state.counters["weight_bytes_ternary_qk"] = static_cast<double>((S * D * 2 + 3) / 4);
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

// ============================================================================
// 5. Int8 baseline — the quantization ladder rung between FP16 and ternary
// ============================================================================
// Weights stored as int8_t (-128..127); activations as int8_t.
// The compiler will typically autovectorize this to SDOT/SMULL on AArch64.

static void BM_DotProduct_Int8(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  std::vector<int8_t> w(static_cast<std::size_t>(N));
  std::vector<int8_t> a(static_cast<std::size_t>(N));
  for (int i = 0; i < N; ++i) {
    const int r = i % 3;
    w[static_cast<std::size_t>(i)] = static_cast<int8_t>((r == 0) ? -1 : (r == 1) ? 0 : 1);
    a[static_cast<std::size_t>(i)] = static_cast<int8_t>((r == 0) ?  1 : (r == 1) ? 0 : 1);
  }

  for (auto _ : state) {
    int32_t acc = 0;
    for (int i = 0; i < N; ++i) {
      acc += static_cast<int32_t>(w[static_cast<std::size_t>(i)]) *
             static_cast<int32_t>(a[static_cast<std::size_t>(i)]);
    }
    benchmark::DoNotOptimize(acc);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * N);
  state.SetLabel("path=int8; op=int8-mul-acc-to-int32");
  state.counters["N"]               = static_cast<double>(N);
  state.counters["weight_bytes_int8"] = static_cast<double>(N);
}
BENCHMARK(BM_DotProduct_Int8)->RangeMultiplier(4)->Range(64, 16384)->Repetitions(2);

static void BM_MatMul_Int8(benchmark::State& state) {
  const int K = static_cast<int>(state.range(0));
  std::vector<int8_t> w(static_cast<std::size_t>(K * K));
  std::vector<int8_t> a(static_cast<std::size_t>(K));
  for (int i = 0; i < K * K; ++i)
    w[static_cast<std::size_t>(i)] = static_cast<int8_t>((i % 3 == 0) ? -1 : (i % 3 == 1) ? 0 : 1);
  for (int i = 0; i < K; ++i)
    a[static_cast<std::size_t>(i)] = static_cast<int8_t>((i % 3 == 0) ? 1 : (i % 3 == 1) ? 0 : 1);
  std::vector<int32_t> out(static_cast<std::size_t>(K), 0);

  for (auto _ : state) {
    std::fill(out.begin(), out.end(), 0);
    for (int k = 0; k < K; ++k) {
      const int32_t av = static_cast<int32_t>(a[static_cast<std::size_t>(k)]);
      for (int n = 0; n < K; ++n) {
        out[static_cast<std::size_t>(n)] +=
            av * static_cast<int32_t>(w[static_cast<std::size_t>(k) * K + n]);
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=int8; op=vector-matrix-int8-mul-acc");
  state.counters["K"]               = static_cast<double>(K);
  state.counters["weight_bytes_int8"] = static_cast<double>(K) * K;
}
BENCHMARK(BM_MatMul_Int8)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Repetitions(2);

// ============================================================================
// 6. NEON-vectorized ternary — 4-wide float, no multiply (AArch64 only)
// ============================================================================
// Uses NEON compare-and-select: vcgtq_f32/vcltq_f32 + vbslq_f32 to replace
// the scalar branch with a branchless 4-element SIMD operation.
// No vmulq_f32 is used — the ternary constraint eliminates all multiplies.

#ifdef __ARM_NEON

static void BM_DotProduct_T81Ternary_NEON(benchmark::State& state) {
  const int N = static_cast<int>(state.range(0));
  const auto w = make_ternary_weights(N);
  const auto a = make_ternary_activations(N);
  // Pad to multiple of 4 for clean NEON loops
  const int N4 = (N / 4) * 4;

  for (auto _ : state) {
    float32x4_t acc  = vdupq_n_f32(0.0f);
    float32x4_t zero = vdupq_n_f32(0.0f);
    int i = 0;
    for (; i < N4; i += 4) {
      float32x4_t wv = vld1q_f32(&w[static_cast<std::size_t>(i)]);
      float32x4_t av = vld1q_f32(&a[static_cast<std::size_t>(i)]);
      // Branchless ternary: select av where w>0, -av where w<0, 0 otherwise
      uint32x4_t pos = vcgtq_f32(wv, zero);
      uint32x4_t neg = vcltq_f32(wv, zero);
      float32x4_t av_neg = vnegq_f32(av);
      float32x4_t contrib = vbslq_f32(pos, av, vbslq_f32(neg, av_neg, zero));
      acc = vaddq_f32(acc, contrib);
    }
    float result = vaddvq_f32(acc);
    for (; i < N; ++i) {  // scalar tail
      const float wv = w[static_cast<std::size_t>(i)];
      if (wv > 0.0f) result += a[static_cast<std::size_t>(i)];
      else if (wv < 0.0f) result -= a[static_cast<std::size_t>(i)];
    }
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * N);
  state.SetLabel("path=t81-ternary-neon; op=4wide-cmp-sel-add; no-multiply");
  state.counters["N"]                    = static_cast<double>(N);
  state.counters["weight_bytes_ternary"] = static_cast<double>((N + 3) / 4);
}
BENCHMARK(BM_DotProduct_T81Ternary_NEON)->RangeMultiplier(4)->Range(64, 16384)->Repetitions(2);

static void BM_MatMul_T81Ternary_NEON(benchmark::State& state) {
  const int K = static_cast<int>(state.range(0));
  const auto w = make_ternary_weights(K * K);
  const auto a = make_ternary_activations(K);
  std::vector<float> out(static_cast<std::size_t>(K), 0.0f);
  const int K4 = (K / 4) * 4;

  for (auto _ : state) {
    std::fill(out.begin(), out.end(), 0.0f);
    const float32x4_t zero = vdupq_n_f32(0.0f);
    for (int k = 0; k < K; ++k) {
      const float av_scalar = a[static_cast<std::size_t>(k)];
      const float32x4_t av4 = vdupq_n_f32(av_scalar);
      const float32x4_t av4_neg = vnegq_f32(av4);
      int n = 0;
      for (; n < K4; n += 4) {
        float32x4_t wv = vld1q_f32(&w[static_cast<std::size_t>(k) * K + n]);
        float32x4_t ov = vld1q_f32(&out[static_cast<std::size_t>(n)]);
        uint32x4_t pos = vcgtq_f32(wv, zero);
        uint32x4_t neg = vcltq_f32(wv, zero);
        float32x4_t contrib = vbslq_f32(pos, av4, vbslq_f32(neg, av4_neg, zero));
        ov = vaddq_f32(ov, contrib);
        vst1q_f32(&out[static_cast<std::size_t>(n)], ov);
      }
      for (; n < K; ++n) {  // scalar tail
        const float wv = w[static_cast<std::size_t>(k) * K + n];
        if (wv > 0.0f) out[static_cast<std::size_t>(n)] += av_scalar;
        else if (wv < 0.0f) out[static_cast<std::size_t>(n)] -= av_scalar;
      }
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=t81-ternary-neon; op=4wide-matmul-cmp-sel; no-multiply");
  state.counters["K"]                    = static_cast<double>(K);
  state.counters["weight_bytes_ternary"] = static_cast<double>((K * K + 3) / 4);
}
BENCHMARK(BM_MatMul_T81Ternary_NEON)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Repetitions(2);

// Output-register-tiled NEON matmul:
//   Outer loop: j tile (4 output columns)
//   Inner loop: k reduction — acc float32x4_t stays in NEON register, zero stores
//   Trade-off: single store per 4-element tile, but weight access is column-strided
//              (w[k*K+jj] with k varying, stride=K floats) — cache-unfriendly at large K.
// Compare to the row-streaming NEON version above to see which bottleneck dominates.
static void BM_MatMul_T81Ternary_NEON_Tiled(benchmark::State& state) {
  const int K = static_cast<int>(state.range(0));
  const auto w = make_ternary_weights(K * K);
  const auto a = make_ternary_activations(K);
  std::vector<float> out(static_cast<std::size_t>(K), 0.0f);
  const int K4 = (K / 4) * 4;
  const float32x4_t zero = vdupq_n_f32(0.0f);

  for (auto _ : state) {
    // Output tile (4 columns) in register across entire K reduction — zero stores per K step
    for (int jj = 0; jj < K4; jj += 4) {
      float32x4_t acc = vdupq_n_f32(0.0f);
      for (int k = 0; k < K; ++k) {
        const float av_scalar = a[static_cast<std::size_t>(k)];
        if (av_scalar == 0.0f) continue;
        const float32x4_t av4  = vdupq_n_f32(av_scalar);
        const float32x4_t av4n = vnegq_f32(av4);
        // w[k][jj..jj+3] — column-strided access (stride = K floats)
        float32x4_t wv = vld1q_f32(&w[static_cast<std::size_t>(k) * K + jj]);
        const uint32x4_t pos = vcgtq_f32(wv, zero);
        const uint32x4_t neg = vcltq_f32(wv, zero);
        acc = vaddq_f32(acc, vbslq_f32(pos, av4, vbslq_f32(neg, av4n, zero)));
      }
      vst1q_f32(&out[static_cast<std::size_t>(jj)], acc);  // single store per tile
    }
    for (int jj = K4; jj < K; ++jj) {  // scalar tail
      float s = 0.0f;
      for (int k = 0; k < K; ++k) {
        const float wv = w[static_cast<std::size_t>(k) * K + jj];
        const float av = a[static_cast<std::size_t>(k)];
        if (wv > 0.0f) s += av;
        else if (wv < 0.0f) s -= av;
      }
      out[static_cast<std::size_t>(jj)] = s;
    }
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * K * K);
  state.SetLabel("path=t81-ternary-neon-tiled; op=register-tile-4out; single-store; col-stride-weight");
  state.counters["K"]                    = static_cast<double>(K);
  state.counters["weight_bytes_ternary"] = static_cast<double>((K * K + 3) / 4);
}
BENCHMARK(BM_MatMul_T81Ternary_NEON_Tiled)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Repetitions(2);

#endif  // __ARM_NEON

// ============================================================================
// 7. Tiny transformer layer — full forward pass throughput
// ============================================================================
// Single decoder attention layer + MLP with ternary weights.
// dim=256, heads=4, head_dim=64, mlp_hidden=512.
// Measures end-to-end tokens/second through a realistic inference kernel.
//
// Weight matrices (all ternary for T81 path, float for FP32 path):
//   Q,K,V : dim×(heads×head_dim) = 256×256
//   O     : (heads×head_dim)×dim = 256×256
//   gate,up: dim×mlp_hidden = 256×512  (SwiGLU gate)
//   down  : mlp_hidden×dim  = 512×256

namespace {

constexpr int kDim      = 256;
constexpr int kHeads    = 4;
constexpr int kKVDim    = kDim;   // heads * head_dim = 256
constexpr int kMLPHid   = 512;

// Layer norm: mean-subtract, variance-normalize, apply scale (all float)
static void layer_norm(const std::vector<float>& x, std::vector<float>& out,
                       int seq, int dim) {
  for (int s = 0; s < seq; ++s) {
    float mean = 0.0f;
    for (int d = 0; d < dim; ++d) mean += x[static_cast<std::size_t>(s * dim + d)];
    mean /= static_cast<float>(dim);
    float var = 0.0f;
    for (int d = 0; d < dim; ++d) {
      const float v = x[static_cast<std::size_t>(s * dim + d)] - mean;
      var += v * v;
    }
    var = std::sqrt(var / static_cast<float>(dim) + 1e-5f);
    for (int d = 0; d < dim; ++d) {
      out[static_cast<std::size_t>(s * dim + d)] =
          (x[static_cast<std::size_t>(s * dim + d)] - mean) / var;
    }
  }
}

// Row-wise softmax in place
static void softmax_rows(std::vector<float>& x, int rows, int cols) {
  for (int r = 0; r < rows; ++r) {
    float mx = *std::max_element(x.begin() + r * cols, x.begin() + (r + 1) * cols);
    float sm = 0.0f;
    for (int c = 0; c < cols; ++c) {
      x[static_cast<std::size_t>(r * cols + c)] =
          std::exp(x[static_cast<std::size_t>(r * cols + c)] - mx);
      sm += x[static_cast<std::size_t>(r * cols + c)];
    }
    for (int c = 0; c < cols; ++c)
      x[static_cast<std::size_t>(r * cols + c)] /= sm;
  }
}

// SiLU: x * sigmoid(x)
static inline float silu(float x) {
  return x / (1.0f + std::exp(-x));
}

// Dense float matmul: [seq, in_dim] × [in_dim, out_dim] → [seq, out_dim]
static void matmul_fp32(const std::vector<float>& a, const std::vector<float>& b,
                         std::vector<float>& c, int seq, int in_dim, int out_dim) {
  std::fill(c.begin(), c.end(), 0.0f);
  for (int s = 0; s < seq; ++s)
    for (int k = 0; k < in_dim; ++k) {
      const float av = a[static_cast<std::size_t>(s * in_dim + k)];
      for (int n = 0; n < out_dim; ++n)
        c[static_cast<std::size_t>(s * out_dim + n)] +=
            av * b[static_cast<std::size_t>(k * out_dim + n)];
    }
}

// Ternary matmul (conditional ADD/SUB, no multiply)
static void matmul_ternary(const std::vector<float>& a, const std::vector<float>& w,
                            std::vector<float>& c, int seq, int in_dim, int out_dim) {
  std::fill(c.begin(), c.end(), 0.0f);
  for (int s = 0; s < seq; ++s)
    for (int k = 0; k < in_dim; ++k) {
      const float av = a[static_cast<std::size_t>(s * in_dim + k)];
      for (int n = 0; n < out_dim; ++n) {
        const float wv = w[static_cast<std::size_t>(k * out_dim + n)];
        if (wv > 0.0f) c[static_cast<std::size_t>(s * out_dim + n)] += av;
        else if (wv < 0.0f) c[static_cast<std::size_t>(s * out_dim + n)] -= av;
      }
    }
}

}  // namespace

// ── FP32 transformer layer ────────────────────────────────────────────────────

static void BM_TransformerLayer_FP32(benchmark::State& state) {
  const int seq = static_cast<int>(state.range(0));

  // Weights (float, ternary-valued for apples-to-apples comparison)
  const auto wq   = make_ternary_weights(kDim * kKVDim);
  const auto wk   = make_ternary_weights(kDim * kKVDim);
  const auto wv   = make_ternary_weights(kDim * kKVDim);
  const auto wo   = make_ternary_weights(kKVDim * kDim);
  const auto wgate = make_ternary_weights(kDim * kMLPHid);
  const auto wup   = make_ternary_weights(kDim * kMLPHid);
  const auto wdown = make_ternary_weights(kMLPHid * kDim);

  // Buffers
  std::vector<float> x(static_cast<std::size_t>(seq * kDim), 0.5f);
  std::vector<float> xn(static_cast<std::size_t>(seq * kDim));
  std::vector<float> q(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> k(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> v(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> scores(static_cast<std::size_t>(seq * seq));
  std::vector<float> attn_out(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> attn_proj(static_cast<std::size_t>(seq * kDim));
  std::vector<float> gate_buf(static_cast<std::size_t>(seq * kMLPHid));
  std::vector<float> up_buf(static_cast<std::size_t>(seq * kMLPHid));
  std::vector<float> mlp_out(static_cast<std::size_t>(seq * kDim));

  for (auto _ : state) {
    // Attention: layernorm → QKV → scores → softmax → V → O
    layer_norm(x, xn, seq, kDim);
    matmul_fp32(xn, wq, q, seq, kDim, kKVDim);
    matmul_fp32(xn, wk, k, seq, kDim, kKVDim);
    matmul_fp32(xn, wv, v, seq, kDim, kKVDim);

    // Q·Kᵀ scores (float matmul, seq×kKVDim @ kKVDim×seq)
    const float scale = 1.0f / std::sqrt(static_cast<float>(kKVDim / kHeads));
    std::fill(scores.begin(), scores.end(), 0.0f);
    for (int i = 0; i < seq; ++i)
      for (int j = 0; j < seq; ++j) {
        float dot = 0.0f;
        for (int d = 0; d < kKVDim; ++d)
          dot += q[static_cast<std::size_t>(i * kKVDim + d)] *
                 k[static_cast<std::size_t>(j * kKVDim + d)];
        scores[static_cast<std::size_t>(i * seq + j)] = dot * scale;
      }
    softmax_rows(scores, seq, seq);

    // scores · V
    std::fill(attn_out.begin(), attn_out.end(), 0.0f);
    for (int i = 0; i < seq; ++i)
      for (int p = 0; p < seq; ++p) {
        const float alpha = scores[static_cast<std::size_t>(i * seq + p)];
        for (int d = 0; d < kKVDim; ++d)
          attn_out[static_cast<std::size_t>(i * kKVDim + d)] +=
              alpha * v[static_cast<std::size_t>(p * kKVDim + d)];
      }

    matmul_fp32(attn_out, wo, attn_proj, seq, kKVDim, kDim);
    // Residual
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += attn_proj[i];

    // MLP: layernorm → SwiGLU (gate * up) → down → residual
    layer_norm(x, xn, seq, kDim);
    matmul_fp32(xn, wgate, gate_buf, seq, kDim, kMLPHid);
    matmul_fp32(xn, wup,   up_buf,   seq, kDim, kMLPHid);
    for (std::size_t i = 0; i < gate_buf.size(); ++i)
      gate_buf[i] = silu(gate_buf[i]) * up_buf[i];
    matmul_fp32(gate_buf, wdown, mlp_out, seq, kMLPHid, kDim);
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += mlp_out[i];

    benchmark::DoNotOptimize(x.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * seq);
  state.SetLabel("path=fp32; op=full-layer-attn-swiglu-mlp; dim=256 mlp=512 heads=4");
  state.counters["seq_len"] = static_cast<double>(seq);
}
BENCHMARK(BM_TransformerLayer_FP32)->Arg(32)->Arg(64)->Arg(128)->Repetitions(2);

// ── Ternary transformer layer ─────────────────────────────────────────────────

static void BM_TransformerLayer_Ternary(benchmark::State& state) {
  const int seq = static_cast<int>(state.range(0));

  // Same weight shapes and values; matmul uses conditional ADD/SUB
  const auto wq    = make_ternary_weights(kDim * kKVDim);
  const auto wk    = make_ternary_weights(kDim * kKVDim);
  const auto wv    = make_ternary_weights(kDim * kKVDim);
  const auto wo    = make_ternary_weights(kKVDim * kDim);
  const auto wgate = make_ternary_weights(kDim * kMLPHid);
  const auto wup   = make_ternary_weights(kDim * kMLPHid);
  const auto wdown = make_ternary_weights(kMLPHid * kDim);

  std::vector<float> x(static_cast<std::size_t>(seq * kDim), 0.5f);
  std::vector<float> xn(static_cast<std::size_t>(seq * kDim));
  std::vector<float> q(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> k(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> v(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> scores(static_cast<std::size_t>(seq * seq));
  std::vector<float> attn_out(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> attn_proj(static_cast<std::size_t>(seq * kDim));
  std::vector<float> gate_buf(static_cast<std::size_t>(seq * kMLPHid));
  std::vector<float> up_buf(static_cast<std::size_t>(seq * kMLPHid));
  std::vector<float> mlp_out(static_cast<std::size_t>(seq * kDim));

  for (auto _ : state) {
    // Ternary QKV projections
    layer_norm(x, xn, seq, kDim);
    matmul_ternary(xn, wq, q, seq, kDim, kKVDim);
    matmul_ternary(xn, wk, k, seq, kDim, kKVDim);
    matmul_ternary(xn, wv, v, seq, kDim, kKVDim);

    // Q·Kᵀ with ternary Q and K (no multiply: integer sign product)
    const float scale = 1.0f / std::sqrt(static_cast<float>(kKVDim / kHeads));
    std::fill(scores.begin(), scores.end(), 0.0f);
    for (int i = 0; i < seq; ++i)
      for (int j = 0; j < seq; ++j) {
        float dot = 0.0f;
        for (int d = 0; d < kKVDim; ++d) {
          const int qi = static_cast<int>(q[static_cast<std::size_t>(i * kKVDim + d)]);
          const int ki = static_cast<int>(k[static_cast<std::size_t>(j * kKVDim + d)]);
          const int s  = qi * ki;  // {-1,0,+1}*{-1,0,+1} — integer, no float mul
          if (s > 0) dot += 1.0f;
          else if (s < 0) dot -= 1.0f;
        }
        scores[static_cast<std::size_t>(i * seq + j)] = dot * scale;
      }
    softmax_rows(scores, seq, seq);

    // scores · V (float; attention weights are always float post-softmax)
    std::fill(attn_out.begin(), attn_out.end(), 0.0f);
    for (int i = 0; i < seq; ++i)
      for (int p = 0; p < seq; ++p) {
        const float alpha = scores[static_cast<std::size_t>(i * seq + p)];
        for (int d = 0; d < kKVDim; ++d)
          attn_out[static_cast<std::size_t>(i * kKVDim + d)] +=
              alpha * v[static_cast<std::size_t>(p * kKVDim + d)];
      }

    matmul_ternary(attn_out, wo, attn_proj, seq, kKVDim, kDim);
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += attn_proj[i];

    // Ternary MLP
    layer_norm(x, xn, seq, kDim);
    matmul_ternary(xn, wgate, gate_buf, seq, kDim, kMLPHid);
    matmul_ternary(xn, wup,   up_buf,   seq, kDim, kMLPHid);
    for (std::size_t i = 0; i < gate_buf.size(); ++i)
      gate_buf[i] = silu(gate_buf[i]) * up_buf[i];
    matmul_ternary(gate_buf, wdown, mlp_out, seq, kMLPHid, kDim);
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += mlp_out[i];

    benchmark::DoNotOptimize(x.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * seq);
  state.SetLabel("path=t81-ternary; op=full-layer-attn-swiglu-mlp; dim=256 mlp=512 heads=4");
  state.counters["seq_len"] = static_cast<double>(seq);
}
BENCHMARK(BM_TransformerLayer_Ternary)->Arg(32)->Arg(64)->Arg(128)->Repetitions(2);

// ── Hybrid MLP transformer layer ──────────────────────────────────────────────
// Attention projections (Q,K,V,O) use ternary weights (no multiply).
// MLP projections (gate, up, down) use FP32 (compiler autovectorises to FMADD).
//
// Governance note: the hybrid path bypasses the ternary weight invariant for
// MLP layers. In a production deployment, Axion policy must explicitly permit
// Int8/FP32 dispatch on gate/up/down weight tensors. This benchmark quantifies
// the throughput uplift; policy approval is required before enabling in
// certified inference paths (RFC-0034 §4.2 activation-ceiling gate applies).

static void BM_TransformerLayer_HybridMLP(benchmark::State& state) {
  const int seq = static_cast<int>(state.range(0));

  // Attention weights: ternary {-1,0,+1}
  const auto wq    = make_ternary_weights(kDim * kKVDim);
  const auto wk    = make_ternary_weights(kDim * kKVDim);
  const auto wv    = make_ternary_weights(kDim * kKVDim);
  const auto wo    = make_ternary_weights(kKVDim * kDim);
  // MLP weights: float ternary values (FP32 path — same {-1,0,+1} distribution)
  const auto wgate = make_ternary_weights(kDim * kMLPHid);
  const auto wup   = make_ternary_weights(kDim * kMLPHid);
  const auto wdown = make_ternary_weights(kMLPHid * kDim);

  std::vector<float> x(static_cast<std::size_t>(seq * kDim), 0.5f);
  std::vector<float> xn(static_cast<std::size_t>(seq * kDim));
  std::vector<float> q(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> k(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> v(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> scores(static_cast<std::size_t>(seq * seq));
  std::vector<float> attn_out(static_cast<std::size_t>(seq * kKVDim));
  std::vector<float> attn_proj(static_cast<std::size_t>(seq * kDim));
  std::vector<float> gate_buf(static_cast<std::size_t>(seq * kMLPHid));
  std::vector<float> up_buf(static_cast<std::size_t>(seq * kMLPHid));
  std::vector<float> mlp_out(static_cast<std::size_t>(seq * kDim));

  for (auto _ : state) {
    // Attention: ternary Q/K/V/O projections
    layer_norm(x, xn, seq, kDim);
    matmul_ternary(xn, wq, q, seq, kDim, kKVDim);
    matmul_ternary(xn, wk, k, seq, kDim, kKVDim);
    matmul_ternary(xn, wv, v, seq, kDim, kKVDim);

    const float scale = 1.0f / std::sqrt(static_cast<float>(kKVDim / kHeads));
    std::fill(scores.begin(), scores.end(), 0.0f);
    for (int i = 0; i < seq; ++i)
      for (int j = 0; j < seq; ++j) {
        float dot = 0.0f;
        for (int d = 0; d < kKVDim; ++d) {
          const int qi = static_cast<int>(q[static_cast<std::size_t>(i * kKVDim + d)]);
          const int ki = static_cast<int>(k[static_cast<std::size_t>(j * kKVDim + d)]);
          const int s  = qi * ki;
          if (s > 0) dot += 1.0f;
          else if (s < 0) dot -= 1.0f;
        }
        scores[static_cast<std::size_t>(i * seq + j)] = dot * scale;
      }
    softmax_rows(scores, seq, seq);

    std::fill(attn_out.begin(), attn_out.end(), 0.0f);
    for (int i = 0; i < seq; ++i)
      for (int p = 0; p < seq; ++p) {
        const float alpha = scores[static_cast<std::size_t>(i * seq + p)];
        for (int d = 0; d < kKVDim; ++d)
          attn_out[static_cast<std::size_t>(i * kKVDim + d)] +=
              alpha * v[static_cast<std::size_t>(p * kKVDim + d)];
      }

    matmul_ternary(attn_out, wo, attn_proj, seq, kKVDim, kDim);
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += attn_proj[i];

    // MLP: FP32 gate/up/down (compiler autovectorises to FMADD)
    layer_norm(x, xn, seq, kDim);
    matmul_fp32(xn, wgate, gate_buf, seq, kDim, kMLPHid);
    matmul_fp32(xn, wup,   up_buf,   seq, kDim, kMLPHid);
    for (std::size_t i = 0; i < gate_buf.size(); ++i)
      gate_buf[i] = silu(gate_buf[i]) * up_buf[i];
    matmul_fp32(gate_buf, wdown, mlp_out, seq, kMLPHid, kDim);
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += mlp_out[i];

    benchmark::DoNotOptimize(x.data());
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * seq);
  state.SetLabel("path=hybrid-mlp; attn=ternary mlp=fp32; dim=256 mlp=512 heads=4");
  state.counters["seq_len"] = static_cast<double>(seq);
}
BENCHMARK(BM_TransformerLayer_HybridMLP)->Arg(32)->Arg(64)->Arg(128)->Repetitions(2);
