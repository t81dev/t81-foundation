#include <iostream>

#include <t81/types/T81Int.hpp>

int main() {
  t81::T81Int<9> value(42);
  std::cout << "t81-consumer: value=" << value.to_int64() << "\n";
  return 0;
}
