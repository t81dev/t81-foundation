#include "t81/tensor.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

using t81::T729DynamicTensor;

T729DynamicTensor reference_matmul(const T729DynamicTensor& A, const T729DynamicTensor& B) {
  const int m = A.shape()[0];
  const int k = A.shape()[1];
  const int n = B.shape()[1];
  std::vector<float> out(static_cast<size_t>(m) * static_cast<size_t>(n), 0.0f);

  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < n; ++j) {
      float sum = 0.0f;
      for (int p = 0; p < k; ++p) {
        sum += A.data()[static_cast<size_t>(i) * k + p] * B.data()[static_cast<size_t>(p) * n + j];
      }
      out[static_cast<size_t>(i) * n + j] = sum;
    }
  }
  return T729DynamicTensor({m, n}, std::move(out));
}

T729DynamicTensor reference_rmsnorm(const T729DynamicTensor& x, const T729DynamicTensor& w,
                                    float eps = 1e-6f) {
  const int dim = x.shape().back();
  std::vector<float> out = x.data();
  const float* w_ptr = w.data().data();
  for (size_t i = 0; i < out.size(); i += static_cast<size_t>(dim)) {
    float ss = 0.0f;
    for (int j = 0; j < dim; ++j) {
      const float v = out[i + static_cast<size_t>(j)];
      ss += v * v;
    }
    const float inv = 1.0f / std::sqrt(ss / static_cast<float>(dim) + eps);
    for (int j = 0; j < dim; ++j) {
      out[i + static_cast<size_t>(j)] = out[i + static_cast<size_t>(j)] * inv * w_ptr[j];
    }
  }
  return T729DynamicTensor(x.shape(), std::move(out));
}

T729DynamicTensor reference_attention(const T729DynamicTensor& q, const T729DynamicTensor& k,
                                      const T729DynamicTensor& v) {
  auto k_t = k.transpose2d();
  auto scores = t81::ops::matmul(q, k_t);
  const float inv_scale = 1.0f / std::sqrt(static_cast<float>(q.shape()[1]));
  auto scaled = scores.snapshot_values();
  for (auto& value : scaled) {
    value *= inv_scale;
  }
  auto probs = t81::ops::softmax(T729DynamicTensor(scores.shape(), std::move(scaled)));
  return t81::ops::matmul(probs, v);
}

T729DynamicTensor reference_embed(const T729DynamicTensor& table, std::int64_t index) {
  const int dim = table.shape()[1];
  const auto values = table.snapshot_values();
  std::vector<float> out(static_cast<std::size_t>(dim));
  const std::size_t base = static_cast<std::size_t>(index) * static_cast<std::size_t>(dim);
  for (int i = 0; i < dim; ++i) {
    out[static_cast<std::size_t>(i)] = values[base + static_cast<std::size_t>(i)];
  }
  auto result = T729DynamicTensor({dim}, std::move(out));
  result.set_numeric_class(table.strict_core_eligible() ? table.numeric_class()
                                                        : t81::TensorNumericClass::HostFloat);
  return result;
}

void assert_tensor_near(const T729DynamicTensor& a, const T729DynamicTensor& b, float eps) {
  assert(a.shape() == b.shape());
  assert(a.data().size() == b.data().size());
  for (size_t i = 0; i < a.data().size(); ++i) {
    const float da = a.data()[i];
    const float db = b.data()[i];
    const float diff = std::fabs(da - db);
    if (diff > eps) {
      std::cerr << "tensor mismatch at " << i << ": " << da << " vs " << db << " (diff=" << diff
                << ", eps=" << eps << ")\n";
      assert(false);
    }
  }
}

void test_matmul_backend_parity() {
  // Nontrivial shape and values to exercise vectorized/scalar tails.
  T729DynamicTensor A({5, 7});
  T729DynamicTensor B({7, 6});
  for (int i = 0; i < 5 * 7; ++i) {
    A.data()[static_cast<size_t>(i)] = ((i % 11) - 5) * 0.125f;
  }
  for (int i = 0; i < 7 * 6; ++i) {
    B.data()[static_cast<size_t>(i)] = ((i % 13) - 6) * 0.0625f;
  }

  const auto optimized = t81::ops::matmul(A, B);
  const auto reference = reference_matmul(A, B);
  assert(optimized.canonical_fixed_authoritative());
  assert_tensor_near(optimized, reference, 1e-5f);
}

void test_rmsnorm_backend_parity() {
  T729DynamicTensor x({3, 8});
  T729DynamicTensor w({8});
  for (int i = 0; i < 3 * 8; ++i) {
    x.data()[static_cast<size_t>(i)] = ((i % 9) - 4) * 0.3f;
  }
  for (int i = 0; i < 8; ++i) {
    w.data()[static_cast<size_t>(i)] = 0.5f + static_cast<float>(i) * 0.1f;
  }

  const auto optimized = t81::ops::rmsnorm(x, w);
  const auto reference = reference_rmsnorm(x, w);
  assert(optimized.canonical_fixed_authoritative());
  assert_tensor_near(optimized, reference, 1e-5f);
}

void test_attention_backend_parity() {
  T729DynamicTensor q({2, 4});
  T729DynamicTensor k({3, 4});
  T729DynamicTensor v({3, 2});
  for (int i = 0; i < 2 * 4; ++i) {
    q.data()[static_cast<size_t>(i)] = ((i % 7) - 3) * 0.2f;
  }
  for (int i = 0; i < 3 * 4; ++i) {
    k.data()[static_cast<size_t>(i)] = ((i % 5) - 2) * 0.25f;
  }
  for (int i = 0; i < 3 * 2; ++i) {
    v.data()[static_cast<size_t>(i)] = ((i % 4) - 1) * 0.5f;
  }

  const auto optimized = t81::ops::attention(q, k, v);
  const auto reference = reference_attention(q, k, v);
  assert(optimized.canonical_fixed_authoritative());
  assert_tensor_near(optimized, reference, 1e-5f);
}

void test_embed_backend_parity() {
  T729DynamicTensor table({4, 3});
  for (int i = 0; i < 4 * 3; ++i) {
    table.data()[static_cast<std::size_t>(i)] = ((i % 8) - 3) * 0.375f;
  }

  const auto optimized = t81::ops::embed(table, 2);
  const auto reference = reference_embed(table, 2);
  assert(optimized.canonical_fixed_authoritative());
  assert(optimized.numeric_class() == reference.numeric_class());
  assert_tensor_near(optimized, reference, 1e-5f);
}

}  // namespace

int main() {
  test_matmul_backend_parity();
  test_rmsnorm_backend_parity();
  test_attention_backend_parity();
  test_embed_backend_parity();
  std::cout << "tensor backend parity test passed\n";
  return 0;
}
