#include "t81/types/cell.hpp"
#include <iostream>
#include <stdexcept>
int main() {
 using t81::core::Cell;
 try {
  auto result = Cell::from_int(50) * Cell::from_int(2);
  std::cout << "50*2=" << result.to_int() << "\n";
  return result.to_int() == 100 ? 0 : 2;
 } catch (const std::overflow_error& error) {
  std::cout << "representable product 100 rejected: " << error.what() << "\n";
  return 0;
 }
}
