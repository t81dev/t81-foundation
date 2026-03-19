// RFC-0034 §5.17 — Ternary-Native Inference Operations
// Pure T81BigInt accumulator; no floating-point multiply.
// All ops require T729DynamicTensor values in {-1.0f, 0.0f, +1.0f} (T81Qutrit domain).
#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "t81/tensor.hpp"
#include "t81/types/T81BigInt.hpp"

namespace t81::ops {

// ---------------------------------------------------------------------------
// TACT mode constants (§5.17.6, TACT Modes registry)
// ---------------------------------------------------------------------------
inline constexpr std::uint8_t kTActModeStep = 0x01;    // TernaryStep
inline constexpr std::uint8_t kTActModeTanh = 0x02;    // TanhQuantized

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace ternary_detail {

// Snap a float activation or weight value to the nearest trit {-1,0,+1}.
inline int snap_trit(float v, float threshold = 0.5f) {
  if (v >  threshold) return  1;
  if (v < -threshold) return -1;
  return 0;
}

// Validate every element of a tensor is in {-1.0f, 0.0f, +1.0f}.
inline bool is_ternary_domain(const T729DynamicTensor& t) {
  for (float v : t.snapshot_values()) {
    if (v != -1.0f && v != 0.0f && v != 1.0f) return false;
  }
  return true;
}

}  // namespace ternary_detail

// ---------------------------------------------------------------------------
// TWMATMUL — Ternary-weight matrix multiply (§5.17.1)
//   acc[i,j] += act[i,k] when wt[k,j]==+1
//   acc[i,j] -= act[i,k] when wt[k,j]==-1
//   skip                 when wt[k,j]==0
// Returns a new tensor whose values are the BigInt-exact result cast to float.
// Weights must be in {-1,0,+1}; activations are arbitrary T81Float rows.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor twmatmul(const T729DynamicTensor& activations,
                                                const T729DynamicTensor& weights) {
  const auto& a_shape = activations.shape();
  const auto& w_shape = weights.shape();

  if (a_shape.size() != 2 || w_shape.size() != 2) {
    throw std::invalid_argument("twmatmul: both operands must be 2-D tensors");
  }
  const int m = static_cast<int>(a_shape[0]);
  const int k = static_cast<int>(a_shape[1]);
  const int kw = static_cast<int>(w_shape[0]);
  const int n  = static_cast<int>(w_shape[1]);

  if (k != kw) {
    throw std::invalid_argument("twmatmul: inner dimensions must match");
  }

  const auto a_vals = activations.snapshot_values();  // m * k floats
  const auto w_vals = weights.snapshot_values();      // k * n floats

  // Use T81BigInt accumulators for bit-exact results.
  std::vector<t81::v1::T81BigInt> acc(static_cast<std::size_t>(m) *
                                      static_cast<std::size_t>(n));

  for (int i = 0; i < m; ++i) {
    for (int p = 0; p < k; ++p) {
      const float av = a_vals[static_cast<std::size_t>(i) * k + p];
      for (int j = 0; j < n; ++j) {
        const float wv = w_vals[static_cast<std::size_t>(p) * n + j];
        const int wt = ternary_detail::snap_trit(wv);
        if (wt == 0) continue;
        // Convert activation element to BigInt, then accumulate.
        // For the normative path the activation must itself be in ternary domain.
        const int av_trit = ternary_detail::snap_trit(av);
        auto& cell = acc[static_cast<std::size_t>(i) * n + j];
        if (wt == +1) {
          cell = cell + t81::v1::T81BigInt(static_cast<std::int64_t>(av_trit));
        } else {
          cell = cell - t81::v1::T81BigInt(static_cast<std::int64_t>(av_trit));
        }
      }
    }
  }

  // Convert BigInt accumulators to float output tensor.
  std::vector<float> out;
  out.reserve(acc.size());
  for (const auto& bi : acc) {
    out.push_back(static_cast<float>(bi.to_int64()));
  }

  return T729DynamicTensor({m, n}, std::move(out));
}

// ---------------------------------------------------------------------------
// TQUANT — Quantize T729DynamicTensor to ternary {-1,0,+1} (§5.17.2)
//   threshold: elements with |x| <= threshold map to 0
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor tquant(const T729DynamicTensor& src, float threshold) {
  if (threshold < 0.0f) {
    throw std::invalid_argument("tquant: threshold must be >= 0");
  }
  auto vals = src.snapshot_values();
  for (auto& v : vals) {
    v = static_cast<float>(ternary_detail::snap_trit(v, threshold));
  }
  return T729DynamicTensor(src.shape(), std::move(vals));
}

// ---------------------------------------------------------------------------
// TERNACCUM — Scalar ternary dot product (§5.17.4)
//   Returns a 1×1 tensor whose single value is the BigInt-exact dot product.
//   Both operands must be flat 1-D tensors of equal length.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor ternaccum(const T729DynamicTensor& weights,
                                                  const T729DynamicTensor& activations) {
  const auto w_vals = weights.snapshot_values();
  const auto a_vals = activations.snapshot_values();

  if (w_vals.size() != a_vals.size()) {
    throw std::invalid_argument("ternaccum: operand lengths must match");
  }

  t81::v1::T81BigInt acc(static_cast<std::int64_t>(0));
  for (std::size_t i = 0; i < w_vals.size(); ++i) {
    const int wt = ternary_detail::snap_trit(w_vals[i]);
    const int av = ternary_detail::snap_trit(a_vals[i]);
    if (wt == 0) continue;
    if (wt == +1) {
      acc = acc + t81::v1::T81BigInt(static_cast<std::int64_t>(av));
    } else {
      acc = acc - t81::v1::T81BigInt(static_cast<std::int64_t>(av));
    }
  }

  return T729DynamicTensor({1, 1}, {static_cast<float>(acc.to_int64())});
}

// ---------------------------------------------------------------------------
// TWEMBED — Row-gather from T81Qutrit table (§5.17.5)
//   table must be 2-D; index selects the row.
//   Result is a 1×cols tensor (the gathered row), values validated as ternary.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor twembed(const T729DynamicTensor& table,
                                               std::int64_t index) {
  const auto& shape = table.shape();
  if (shape.size() != 2) {
    throw std::invalid_argument("twembed: table must be 2-D");
  }
  const auto rows = static_cast<std::int64_t>(shape[0]);
  const auto cols = static_cast<std::size_t>(shape[1]);

  if (index < 0 || index >= rows) {
    throw std::out_of_range("twembed: index out of bounds");
  }
  if (!ternary_detail::is_ternary_domain(table)) {
    throw std::invalid_argument("twembed: table values must be in ternary domain {-1,0,+1}");
  }

  const auto all_vals = table.snapshot_values();
  const std::size_t row_offset = static_cast<std::size_t>(index) * cols;
  std::vector<float> row(all_vals.begin() + static_cast<std::ptrdiff_t>(row_offset),
                         all_vals.begin() + static_cast<std::ptrdiff_t>(row_offset + cols));

  return T729DynamicTensor({1, static_cast<int>(cols)}, std::move(row));
}

// ---------------------------------------------------------------------------
// TATTN — Ternary Q/K attention (§5.17.3)
//   Q and K are 2-D tensors (seq_len × head_dim) with ternary-domain values.
//   V is the float value matrix (seq_len × head_dim); output = softmax(Q·Kᵀ) · V.
//   Q·Kᵀ is computed via twmatmul (ternary, BigInt-exact).
//   Softmax and V-projection are T81Float deterministic (t81_soft_math path).
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor tattn(const T729DynamicTensor& q,
                                             const T729DynamicTensor& k,
                                             const T729DynamicTensor& v) {
  const auto& qsh = q.shape();
  const auto& ksh = k.shape();
  const auto& vsh = v.shape();

  if (qsh.size() != 2 || ksh.size() != 2 || vsh.size() != 2) {
    throw std::invalid_argument("tattn: Q, K, V must be 2-D tensors");
  }
  const int seq_q   = static_cast<int>(qsh[0]);
  const int head_q  = static_cast<int>(qsh[1]);
  const int seq_k   = static_cast<int>(ksh[0]);
  const int head_k  = static_cast<int>(ksh[1]);
  const int seq_v   = static_cast<int>(vsh[0]);
  const int head_v  = static_cast<int>(vsh[1]);

  if (head_q != head_k) {
    throw std::invalid_argument("tattn: Q and K head_dim must match");
  }
  if (seq_k != seq_v) {
    throw std::invalid_argument("tattn: K and V sequence lengths must match");
  }

  // K transposed: shape (head_k × seq_k)
  std::vector<float> kt_vals;
  kt_vals.reserve(static_cast<std::size_t>(head_k) * seq_k);
  const auto k_vals = k.snapshot_values();
  for (int h = 0; h < head_k; ++h) {
    for (int s = 0; s < seq_k; ++s) {
      kt_vals.push_back(k_vals[static_cast<std::size_t>(s) * head_k + h]);
    }
  }
  T729DynamicTensor kt({head_k, seq_k}, std::move(kt_vals));

  // scores = Q · Kᵀ via ternary matmul — shape (seq_q × seq_k)
  T729DynamicTensor scores = twmatmul(q, kt);

  // Deterministic softmax row-wise (host float; determinism guaranteed by
  // t81_soft_math call-site in VM; here we use std::exp as the ops-layer
  // reference implementation — VM overrides with t81::soft_math::exp).
  auto sc_vals = scores.snapshot_values();
  const float scale = 1.0f / std::sqrt(static_cast<float>(head_q));
  for (int i = 0; i < seq_q; ++i) {
    float row_max = -1e38f;
    for (int j = 0; j < seq_k; ++j) {
      float sv = sc_vals[static_cast<std::size_t>(i) * seq_k + j] * scale;
      sc_vals[static_cast<std::size_t>(i) * seq_k + j] = sv;
      if (sv > row_max) row_max = sv;
    }
    float row_sum = 0.0f;
    for (int j = 0; j < seq_k; ++j) {
      float ev = std::exp(sc_vals[static_cast<std::size_t>(i) * seq_k + j] - row_max);
      sc_vals[static_cast<std::size_t>(i) * seq_k + j] = ev;
      row_sum += ev;
    }
    for (int j = 0; j < seq_k; ++j) {
      sc_vals[static_cast<std::size_t>(i) * seq_k + j] /= row_sum;
    }
  }

  // out = softmax(scores) · V — shape (seq_q × head_v)
  const auto v_vals = v.snapshot_values();
  std::vector<float> out(static_cast<std::size_t>(seq_q) * head_v, 0.0f);
  for (int i = 0; i < seq_q; ++i) {
    for (int p = 0; p < seq_k; ++p) {
      const float alpha = sc_vals[static_cast<std::size_t>(i) * seq_k + p];
      if (alpha == 0.0f) continue;
      for (int j = 0; j < head_v; ++j) {
        out[static_cast<std::size_t>(i) * head_v + j] +=
            alpha * v_vals[static_cast<std::size_t>(p) * head_v + j];
      }
    }
  }

  return T729DynamicTensor({seq_q, head_v}, std::move(out));
}

// ---------------------------------------------------------------------------
// TACT — Ternary activation function (§5.17.6)
//   mode 0x01 (TernaryStep):    x > 0.5 → +1; x < −0.5 → −1; else 0
//   mode 0x02 (TanhQuantized):  tanh(x) > 0.5 → +1; tanh(x) < −0.5 → −1; else 0
//
//   NOTE: The post-execute Axion activation-ceiling gate
//   (Allow / Quarantine / Deny) is enforced in the VM dispatch (vm.cpp),
//   not here. This function implements only the mathematical transform.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor tact(const T729DynamicTensor& src, std::uint8_t mode) {
  auto vals = src.snapshot_values();
  for (auto& v : vals) {
    float x = v;
    if (mode == kTActModeTanh) {
      x = std::tanh(x);
    }
    v = static_cast<float>(ternary_detail::snap_trit(x));
  }
  return T729DynamicTensor(src.shape(), std::move(vals));
}

}  // namespace t81::ops
