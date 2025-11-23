#include <cassert>
#include <iostream>
#include <vector>
#include "t81/tensor/shape.hpp"

int main() {
  using namespace t81::shape;

  // size_of
  assert(size_of({}) == 0);
  assert(size_of({3}) == 3);
  assert(size_of({2, 3, 4}) == 24);
  bool threw = false;
  try { (void)size_of({2, -1}); } catch (const std::invalid_argument&) { threw = true; }
  assert(threw);

  // can_broadcast_to
  assert(can_broadcast_to({3,1}, {3,4}));
  assert(can_broadcast_to({1,4}, {3,4}));
  assert(can_broadcast_to({1,1}, {3,4}));
  assert(!can_broadcast_to({2,3}, {3,4}));
  assert(can_broadcast_to({4}, {3,4}));
  assert(!can_broadcast_to({5}, {3,4}));

  // broadcast_shape
  {
    auto s = broadcast_shape({3,1}, {3,4});
    assert((s == std::vector<int>({3,4})));
  }
  {
    auto s = broadcast_shape({1,4}, {3,4});
    assert((s == std::vector<int>({3,4})));
  }
  {
    auto s = broadcast_shape({1,1}, {3,4});
    assert((s == std::vector<int>({3,4})));
  }
  {
    bool bad = false;
    try { (void)broadcast_shape({2,3}, {3,4}); } catch (const std::invalid_argument&) { bad = true; }
    assert(bad);
  }

  std::cout << "tensor_shape ok\n";
  return 0;
}
