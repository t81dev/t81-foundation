#include <cmath>
#include <iomanip>
#include <iostream>
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Matrix.hpp"

using namespace t81;

void log(const char* msg) {
  std::cout << "[DEBUG] " << msg << std::endl;
  std::cout.flush();
}

int main() {
  log("Starting debug_matrix_crash...");

  using Scalar = T81Float<72, 9>;

  // 1. Float Construction
  log("1. Constructing scalars");
  Scalar one = Scalar::from_double(1.0);
  Scalar two = Scalar::from_double(2.0);
  Scalar half = Scalar::from_double(0.5);
  log("Scalars constructed. Checking values...");
  if (std::abs(one.to_double() - 1.0) > 1e-9) log("ERROR: one != 1.0");
  if (std::abs(two.to_double() - 2.0) > 1e-9) log("ERROR: two != 2.0");
  if (std::abs(half.to_double() - 0.5) > 1e-9) log("ERROR: half != 0.5");

  // 2. Float Arithmetic
  log("2. Float Arithmetic");
  Scalar res = one + one;
  if (std::abs(res.to_double() - 2.0) > 1e-9) log("ERROR: 1+1 != 2");

  res = one / two;
  if (std::abs(res.to_double() - 0.5) > 1e-9) log("ERROR: 1/2 != 0.5");

  // 3. Matrix Construction (3x3)
  log("3. Matrix 3x3 Construction");
  using Mat3 = T81Matrix<Scalar, 3, 3>;
  Mat3 m3;
  m3(0, 0) = one;
  m3(1, 1) = two;
  m3(2, 2) = one;

  log("3. Matrix 3x3 Determinant");
  Scalar det3 = m3.determinant();
  log("Det3 calculated");
  if (std::abs(det3.to_double() - 2.0) > 1e-9) log("ERROR: det3 != 2.0");

  // 4. Matrix 4x4 Construction
  log("4. Matrix 4x4 Construction");
  using Mat4 = T81Matrix<Scalar, 4, 4>;
  Mat4 m4;  // Identity-like
  m4(0, 0) = one;
  m4(1, 1) = one;
  m4(2, 2) = one;
  m4(3, 3) = one;

  log("4. Matrix 4x4 Determinant");
  Scalar det4 = m4.determinant();
  log("Det4 calculated");
  if (std::abs(det4.to_double() - 1.0) > 1e-9) log("ERROR: det4 != 1.0");

  // 5. Matrix 4x4 Permutation
  log("5. Matrix 4x4 Permutation");
  Mat4 P;
  P(0, 1) = one;
  P(1, 2) = one;
  P(2, 3) = one;
  P(3, 0) = one;

  log("5. Calculating Permutation Det");
  Scalar detP = P.determinant();
  log("Permutation Det calculated");
  if (std::abs(detP.to_double() - (-1.0)) > 1e-9) log("ERROR: detP != -1.0");

  log("5. Calculating Permutation Inverse");
  [[maybe_unused]] Mat4 Pinv = P.inverse();
  log("Permutation Inverse calculated");

  // 6. Matrix 5x5
  log("6. Matrix 5x5 Construction");
  using Mat5 = T81Matrix<Scalar, 5, 5>;
  Mat5 D;
  D(0, 0) = one;
  D(1, 1) = two;
  D(2, 2) = one;
  D(3, 3) = two;
  D(4, 4) = one;

  log("6. Calculating 5x5 Det");
  Scalar det5 = D.determinant();
  log("5x5 Det calculated");
  if (std::abs(det5.to_double() - 4.0) > 1e-9) log("ERROR: det5 != 4.0");

  log("6. Calculating 5x5 Inverse");
  [[maybe_unused]] Mat5 Dinv = D.inverse();
  log("5x5 Inverse calculated");

  log("DONE");
  return 0;
}
