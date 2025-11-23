#pragma once
#include <cmath>

namespace t81 {
inline float ternary_entropy(const int* values, int len) {
  if (len <= 0) return 0.0f;
  int c[3] = {0,0,0};
  for (int i=0;i<len;++i) { if (values[i] < 0) ++c[0]; else if (values[i] == 0) ++c[1]; else ++c[2]; }
  auto H = [](float p){ return p > 0.f ? -p*std::log2(p) : 0.f; };
  float p0 = c[0]/static_cast<float>(len);
  float p1 = c[1]/static_cast<float>(len);
  float p2 = c[2]/static_cast<float>(len);
  return H(p0)+H(p1)+H(p2);
}
} // namespace t81

