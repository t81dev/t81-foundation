#pragma once
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <initializer_list>
#include <algorithm>
#include <numeric>

namespace t81 {

// Lightweight row-major tensor of float.
class T729Tensor {
public:
  // --- ctors ---
  T729Tensor() = default;

  explicit T729Tensor(std::vector<int> shape)
  : shape_(std::move(shape)), data_(size_from_shape_(shape_)) {
    if (!valid_shape_(shape_)) throw std::invalid_argument("T729Tensor: invalid shape");
  }

  T729Tensor(std::initializer_list<int> shape)
  : T729Tensor(std::vector<int>(shape)) {}

  T729Tensor(std::vector<int> shape, std::vector<float> data)
  : shape_(std::move(shape)), data_(std::move(data)) {
    if (!valid_shape_(shape_)) throw std::invalid_argument("T729Tensor: invalid shape");
    if (data_.size() != size_from_shape_(shape_))
      throw std::invalid_argument("T729Tensor: data size mismatch");
  }

  // --- basics ---
  int rank() const { return static_cast<int>(shape_.size()); }
  const std::vector<int>& shape() const { return shape_; }
  std::vector<float>& data() { return data_; }
  const std::vector<float>& data() const { return data_; }

  std::size_t size() const { return data_.size(); }

  // --- utilities ---
  // Dot-product of two rank-1 tensors → rank-1 {1} tensor.
  static T729Tensor contract_dot(const T729Tensor& a, const T729Tensor& b) {
    if (a.rank() != 1 || b.rank() != 1)
      throw std::invalid_argument("contract_dot: both inputs must be vectors");
    if (a.shape_[0] != b.shape_[0])
      throw std::invalid_argument("contract_dot: size mismatch");
    float s = 0.0f;
    for (std::size_t i = 0; i < a.data_.size(); ++i) s += a.data_[i] * b.data_[i];
    return T729Tensor({1}, std::vector<float>{s});
  }

  // 2D transpose → swaps {rows, cols}.
  T729Tensor transpose2d() const {
    if (rank() != 2) throw std::invalid_argument("transpose2d: rank must be 2");
    const int R = shape_[0], C = shape_[1];
    std::vector<float> out(static_cast<std::size_t>(R) * C);
    for (int r = 0; r < R; ++r) {
      for (int c = 0; c < C; ++c) {
        out[static_cast<std::size_t>(c) * R + r] = data_[static_cast<std::size_t>(r) * C + c];
      }
    }
    return T729Tensor({C, R}, std::move(out));
  }

  // Broadcast (naive repeat) to new shape if compatible (right-aligned).
  T729Tensor broadcast(std::vector<int> new_shape) const {
    if (new_shape.empty()) throw std::invalid_argument("broadcast: empty new_shape");
    // Align shapes from the right
    int nr = static_cast<int>(new_shape.size());
    std::vector<int> cur(nr, 1);
    for (int i = 0; i < rank(); ++i) cur[nr - 1 - i] = shape_[rank() - 1 - i];

    // Check compatibility
    for (int i = 0; i < nr; ++i) {
      if (!(cur[i] == new_shape[i] || cur[i] == 1)) {
        throw std::invalid_argument("broadcast: incompatible shapes");
      }
    }

    // Compute strides
    auto strides = [](const std::vector<int>& s) {
      std::vector<std::size_t> st(s.size(), 1);
      for (int i = (int)s.size() - 2; i >= 0; --i)
        st[(std::size_t)i] = st[(std::size_t)(i + 1)] * (std::size_t)s[(std::size_t)(i + 1)];
      return st;
    };
    auto in_strides  = strides(cur);
    auto out_strides = strides(new_shape);

    const std::size_t out_sz =
      std::accumulate(new_shape.begin(), new_shape.end(), std::size_t{1},
                      [](std::size_t a, int b){ return a * (std::size_t)b; });

    std::vector<float> out(out_sz);
    std::vector<int> idx(nr, 0);

    for (std::size_t flat = 0; flat < out_sz; ++flat) {
      // decode flat -> idx
      std::size_t rem = flat;
      for (int d = 0; d < nr; ++d) {
        idx[(std::size_t)d] = static_cast<int>(rem / out_strides[(std::size_t)d]);
        rem %= out_strides[(std::size_t)d];
      }
      // map to input flat (clamp broadcasted dims to 0)
      std::size_t in_flat = 0;
      for (int d = 0; d < nr; ++d) {
        int dim = cur[(std::size_t)d];
        int ii  = (dim == 1) ? 0 : idx[(std::size_t)d];
        in_flat += (std::size_t)ii * in_strides[(std::size_t)d];
      }
      out[flat] = data_[in_flat];
    }

    return T729Tensor(std::move(new_shape), std::move(out));
  }

private:
  std::vector<int>   shape_;
  std::vector<float> data_;

  static bool valid_shape_(const std::vector<int>& s) {
    return std::all_of(s.begin(), s.end(), [](int d){ return d > 0; });
  }

  static std::size_t size_from_shape_(const std::vector<int>& s) {
    if (s.empty()) return 0;
    std::size_t n = 1;
    for (int d : s) {
      if (d <= 0) throw std::invalid_argument("size_from_shape_: non-positive dim");
      n *= (std::size_t)d;
    }
    return n;
  }
};

} // namespace t81
