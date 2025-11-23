#include <iostream>
#include <vector>
#include "t81/t81.hpp"

int main() {
  using namespace t81;

  // Build a 2x3 matrix:
  // [1 2 3
  //  4 5 6]
  T729Tensor A({2,3});
  A.data() = {1,2,3, 4,5,6};

  // Transpose -> 3x2
  auto AT = t81::ops::transpose(A);

  // Slice rows[0:2), cols[1:3) -> 2x2
  auto S = t81::ops::slice2d(A, 0, 2, 1, 3);

  // Reshape 2x3 -> 3x2 (row-major reinterpretation)
  auto R = t81::ops::reshape(A, {3,2});

  // Matmul (2x3)·(3x2) -> (2x2)
  auto C = t81::ops::matmul(A, AT);

  // Reductions on A (axis 0: per-col, axis 1: per-row)
  auto sum_cols = t81::ops::reduce_sum_2d(A, 0); // {3}
  auto sum_rows = t81::ops::reduce_sum_2d(A, 1); // {2}

  // Broadcast a row {1,3} -> {2,3}
  T729Tensor row({1,3}); row.data() = {10, 20, 30};
  auto B = t81::ops::broadcast_to(row, {2,3});

  // Print a helper
  auto dump = [](const char* name, const T729Tensor& t) {
    std::cout << name << " [" << t.shape()[0];
    for (int i = 1; i < t.rank(); ++i) std::cout << "x" << t.shape()[i];
    std::cout << "]: {";
    for (size_t i = 0; i < t.data().size(); ++i) {
      if (i) std::cout << ", ";
      std::cout << t.data()[i];
    }
    std::cout << "}\n";
  };

  dump("A    ", A);
  dump("AT   ", AT);
  dump("S    ", S);
  dump("R    ", R);
  dump("C=AAT", C);
  dump("sum0 ", sum_cols);
  dump("sum1 ", sum_rows);
  dump("Brcst", B);

  return 0;
}
