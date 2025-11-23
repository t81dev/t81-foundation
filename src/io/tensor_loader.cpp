#include <sstream>
#include <limits>
#include "t81/io/tensor_loader.hpp"

namespace t81::io {

static void require(bool cond, const char* msg) {
  if (!cond) throw std::runtime_error(msg);
}

T729Tensor load_tensor_txt(std::istream& in) {
  // Read first line: RANK D1 D2 ... DR
  std::string header;
  {
    // Skip leading empty/comment lines
    while (std::getline(in, header)) {
      if (header.empty()) continue;
      // allow comments starting with '#'
      bool only_ws = true;
      for (char c : header) { if (!std::isspace(static_cast<unsigned char>(c))) { only_ws = false; break; } }
      if (only_ws) continue;
      if (!header.empty() && header[0] == '#') continue;
      break;
    }
  }
  require(static_cast<bool>(in) && !header.empty(), "load_tensor_txt: missing header line");

  std::istringstream hs(header);
  int rank = 0;
  hs >> rank;
  require(hs && rank > 0, "load_tensor_txt: invalid rank");
  std::vector<int> shape(rank, 0);
  for (int i = 0; i < rank; ++i) {
    hs >> shape[static_cast<size_t>(i)];
    require(hs && shape[static_cast<size_t>(i)] > 0, "load_tensor_txt: invalid dimension");
  }

  // Compute total size
  std::size_t total = 1;
  for (int d : shape) {
    // guard overflow
    if (d <= 0 || total > (std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(d)))
      throw std::runtime_error("load_tensor_txt: size overflow");
    total *= static_cast<std::size_t>(d);
  }

  // Read remaining floats (allow them to span multiple lines)
  std::vector<float> data;
  data.reserve(total);
  float v;
  while (in >> v) {
    data.push_back(v);
    if (data.size() == total) break;
  }
  require(data.size() == total, "load_tensor_txt: not enough data");
  return T729Tensor(std::move(shape), std::move(data));
}

void save_tensor_txt(std::ostream& out, const T729Tensor& t) {
  // Header
  out << t.rank();
  for (int d : t.shape()) out << ' ' << d;
  out << '\n';

  // Data (single line, space-separated)
  const auto& d = t.data();
  for (std::size_t i = 0; i < d.size(); ++i) {
    if (i) out << ' ';
    out << d[i];
  }
  out << '\n';
}

} // namespace t81::io
