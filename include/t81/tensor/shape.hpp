#pragma once
#include <vector>
#include <stdexcept>
#include <cstddef>

namespace t81::shape {

// Compute product of dimensions; throws if any dim <= 0.
inline std::size_t size_of(const std::vector<int>& dims) {
  if (dims.empty()) return 0;
  std::size_t s = 1;
  for (int d : dims) {
    if (d <= 0) throw std::invalid_argument("size_of: dims must be positive");
    s *= static_cast<std::size_t>(d);
  }
  return s;
}

// Check if `from` can broadcast to `to` (NumPy-style, right-aligned).
// Returns true if compatible.
inline bool can_broadcast_to(const std::vector<int>& from, const std::vector<int>& to) {
  int i = static_cast<int>(from.size()) - 1;
  int j = static_cast<int>(to.size()) - 1;
  for (; j >= 0; --i, --j) {
    int a = (i >= 0 ? from[static_cast<std::size_t>(i)] : 1);
    int b = to[static_cast<std::size_t>(j)];
    if (!(a == b || a == 1)) return false;
  }
  return true;
}

// Produce the broadcasted shape of two shapes (right-aligned).
// Throws if not broadcastable.
inline std::vector<int> broadcast_shape(std::vector<int> a, std::vector<int> b) {
  int i = static_cast<int>(a.size()) - 1;
  int j = static_cast<int>(b.size()) - 1;
  std::vector<int> out;
  for (; i >= 0 || j >= 0; --i, --j) {
    int da = (i >= 0 ? a[static_cast<std::size_t>(i)] : 1);
    int db = (j >= 0 ? b[static_cast<std::size_t>(j)] : 1);
    if (!(da == db || da == 1 || db == 1))
      throw std::invalid_argument("broadcast_shape: incompatible dims");
    out.push_back(da == 1 ? db : da);
  }
  std::reverse(out.begin(), out.end());
  return out;
}

} // namespace t81::shape
