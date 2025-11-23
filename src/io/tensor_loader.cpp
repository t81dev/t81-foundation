#include <fstream>
#include <sstream>
#include <stdexcept>
#include "t81/tensor.hpp"
#include "t81/io/tensor_loader.hpp"

namespace t81::io {

static std::vector<int> read_shape_line(std::istream& in) {
  std::string line;
  if (!std::getline(in, line)) throw std::runtime_error("tensor_loader: missing shape line");
  std::istringstream ss(line);
  int rank = 0;
  if (!(ss >> rank) || rank <= 0) throw std::runtime_error("tensor_loader: invalid rank");
  std::vector<int> shape(rank);
  for (int i = 0; i < rank; ++i) {
    if (!(ss >> shape[i]) || shape[i] <= 0) throw std::runtime_error("tensor_loader: invalid dimension");
  }
  return shape;
}

T729Tensor load_tensor_txt(std::istream& in) {
  auto shape = read_shape_line(in);
  size_t total = 1;
  for (int d : shape) total *= static_cast<size_t>(d);
  std::vector<float> data;
  data.reserve(total);
  float v;
  while (in >> v) {
    data.push_back(v);
    if (data.size() == total) break;
  }
  if (data.size() != total) throw std::runtime_error("tensor_loader: insufficient data values");
  return T729Tensor(std::move(shape), std::move(data));
}

T729Tensor load_tensor_txt_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("tensor_loader: cannot open file: " + path);
  return load_tensor_txt(f);
}

void save_tensor_txt(std::ostream& out, const T729Tensor& t) {
  out << t.rank();
  for (int d : t.shape()) out << ' ' << d;
  out << '\n';
  const auto& d = t.data();
  for (size_t i = 0; i < d.size(); ++i) {
    out << d[i];
    if (i + 1 < d.size()) out << ' ';
  }
  out << '\n';
}

void save_tensor_txt_file(const std::string& path, const T729Tensor& t) {
  std::ofstream f(path);
  if (!f) throw std::runtime_error("tensor_loader: cannot open file for write: " + path);
  save_tensor_txt(f, t);
}

} // namespace t81::io
