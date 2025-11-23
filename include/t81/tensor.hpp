#pragma once
#include <vector>
#include <stdexcept>

namespace t81 {

class T729Tensor {
public:
  T729Tensor() = default;
  T729Tensor(std::initializer_list<int> shape) : rank_(static_cast<int>(shape.size())), shape_(shape) {
    data_.assign(size_(), 0.0f);
  }
  T729Tensor(std::vector<int> shape, std::vector<float> data)
    : rank_(static_cast<int>(shape.size())), shape_(std::move(shape)), data_(std::move(data)) {
    if (data_.size() != size_()) throw std::invalid_argument("data size != product(shape)");
  }

  int rank()  const { return rank_; }
  const std::vector<int>& shape() const { return shape_; }
  std::vector<float>& data() { return data_; }
  const std::vector<float>& data() const { return data_; }
  size_t size() const { return size_(); }

  static T729Tensor contract_dot(const T729Tensor& a, const T729Tensor& b) {
    if (a.rank_ != 1 || b.rank_ != 1 || a.shape_[0] != b.shape_[0])
      throw std::invalid_argument("contract_dot expects two vectors of equal length");
    float dot = 0.0f;
    for (int i=0;i<a.shape_[0];++i) dot += a.data_[i]*b.data_[i];
    return T729Tensor({1}, {dot});
  }

  static T729Tensor transpose(const T729Tensor& m) {
    if (m.rank_ != 2) throw std::invalid_argument("transpose expects rank-2");
    const int rows = m.shape_[0], cols = m.shape_[1];
    std::vector<float> out(rows*cols, 0.0f);
    for (int i=0;i<rows;++i)
      for (int j=0;j<cols;++j)
        out[j*rows + i] = m.data_[i*cols + j];
    return T729Tensor({cols, rows}, std::move(out));
  }

  T729Tensor broadcast(std::vector<int> new_shape) const {
    size_t old_sz = size_();
    size_t new_sz = 1; for (int d: new_shape) new_sz *= static_cast<size_t>(d);
    if (new_sz % old_sz != 0) throw std::invalid_argument("incompatible broadcast");
    std::vector<float> out(new_sz);
    for (size_t i=0;i<new_sz;++i) out[i] = data_[i % old_sz];
    return T729Tensor(std::move(new_shape), std::move(out));
  }

private:
  size_t size_() const { if (rank_ <= 0) return 0; size_t s = 1; for (int d: shape_) s *= static_cast<size_t>(d); return s; }

  int rank_{0};
  std::vector<int> shape_{};
  std::vector<float> data_{};
};

} // namespace t81

