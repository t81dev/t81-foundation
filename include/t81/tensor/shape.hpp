#pragma once
#include <algorithm>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace t81::shape {

// Product of dimensions (size_t). Throws if any dim <= 0.
inline std::size_t size_of(const std::vector<int>& shp) {
  if (shp.empty()) return 0;
  return std::accumulate(shp.begin(), shp.end(), std::size_t{1},
    [](std::size_t a, int b){
      if (b <= 0) throw std::invalid_argument("size_of: non-positive dim");
      return a * static_cast<std::size_t>(b);
    });
}

// Compute row-major strides for a shape (outermost→innermost).
inline std::vector<std::size_t> strides_of(const std::vector<int>& shp) {
  if (shp.empty()) return {};
  std::vector<std::size_t> s(shp.size(), 1);
  for (int i = static_cast<int>(shp.size()) - 2; i >= 0; --i) {
    if (shp[(std::size_t)(i+1)] <= 0 || shp[(std::size_t)i] <= 0)
      throw std::invalid_argument("strides_of: non-positive dim");
    s[(std::size_t)i] = s[(std::size_t)(i+1)] * static_cast<std::size_t>(shp[(std::size_t)(i+1)]);
  }
  return s;
}

// Right-aligned broadcasting compatibility: a can broadcast to b ?
inline bool can_broadcast_to(const std::vector<int>& a, const std::vector<int>& b) {
  int ra = static_cast<int>(a.size());
  int rb = static_cast<int>(b.size());
  int r  = std::max(ra, rb);
  for (int i = 0; i < r; ++i) {
    int da = (i < ra) ? a[(std::size_t)(ra-1-i)] : 1;
    int db = (i < rb) ? b[(std::size_t)(rb-1-i)] : 1;
    if (!(da == db || da == 1 || db == 1)) return false;
  }
  return true;
}

// Join two shapes under NumPy-style broadcasting (right-aligned).
// Throws if they are incompatible.
inline std::vector<int> broadcast_shape(const std::vector<int>& a,
                                        const std::vector<int>& b) {
  int ra = static_cast<int>(a.size());
  int rb = static_cast<int>(b.size());
  int r  = std::max(ra, rb);
  std::vector<int> out((std::size_t)r, 1);
  for (int i = 0; i < r; ++i) {
    int da = (i < ra) ? a[(std::size_t)(ra-1-i)] : 1;
    int db = (i < rb) ? b[(std::size_t)(rb-1-i)] : 1;
    if (da <= 0 || db <= 0) throw std::invalid_argument("broadcast_shape: non-positive dim");
    if (da == db) out[(std::size_t)(r-1-i)] = da;
    else if (da == 1) out[(std::size_t)(r-1-i)] = db;
    else if (db == 1) out[(std::size_t)(r-1-i)] = da;
    else throw std::invalid_argument("broadcast_shape: incompatible shapes");
  }
  return out;
}

// Remove dims that are exactly 1.
inline std::vector<int> squeeze(const std::vector<int>& shp) {
  std::vector<int> out;
  out.reserve(shp.size());
  for (int d : shp) {
    if (d <= 0) throw std::invalid_argument("squeeze: non-positive dim");
    if (d != 1) out.push_back(d);
  }
  if (out.empty()) out.push_back(1); // keep at least a scalar dim
  return out;
}

// Flatten: collapse to a single dimension preserving element count.
inline std::vector<int> flatten(const std::vector<int>& shp) {
  return { static_cast<int>(size_of(shp)) };
}

// Validate reshape counts (supports one -1 to infer). Returns finalized shape.
inline std::vector<int> validate_reshape(const std::vector<int>& old_shape,
                                         std::vector<int> new_shape) {
  std::size_t total = size_of(old_shape);
  int neg1_idx = -1;
  std::size_t known = 1;
  for (int i = 0; i < (int)new_shape.size(); ++i) {
    int d = new_shape[(std::size_t)i];
    if (d == -1) {
      if (neg1_idx != -1) throw std::invalid_argument("validate_reshape: multiple -1");
      neg1_idx = i;
    } else if (d > 0) {
      known *= (std::size_t)d;
    } else {
      throw std::invalid_argument("validate_reshape: non-positive dim (except -1)");
    }
  }
  if (neg1_idx != -1) {
    if (known == 0 || total % known != 0)
      throw std::invalid_argument("validate_reshape: not divisible to infer -1");
    new_shape[(std::size_t)neg1_idx] = static_cast<int>(total / known);
  }
  if (size_of(new_shape) != total)
    throw std::invalid_argument("validate_reshape: element count mismatch");
  return new_shape;
}

} // namespace t81::shape
