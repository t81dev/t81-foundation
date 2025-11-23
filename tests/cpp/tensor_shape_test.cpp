#include <cassert>
#include <iostream>
#include <vector>
#include <climits>
#include "t81/tensor/shape.hpp"
#include "t81/tensor.hpp"

int main() {
  using namespace t81::shape;

  // size_of
  {
    std::vector<int> s{2,3,4};
    assert(size_of(s) == 24);
    std::vector<int> s1{7};
    assert(size_of(s1) == 7);
  }

  // strides_of (row-major)
  {
    auto st = strides_of({2,3,4}); // expect {12,4,1}
    assert(st.size() == 3);
    assert(st[0] == 12 && st[1] == 4 && st[2] == 1);
    auto st2 = strides_of({5}); // {1}
    assert(st2.size() == 1 && st2[0] == 1);
  }

  // can_broadcast_to
  {
    assert(can_broadcast_to({3}, {2,3}));     // vec -> rows
    assert(can_broadcast_to({1,3}, {2,3}));   // row -> matrix
    assert(can_broadcast_to({2,1}, {2,3}));   // col -> matrix
    assert(!can_broadcast_to({2,2}, {2,3}));  // mismatch
  }

  // broadcast_shape
  {
    auto s = broadcast_shape({3}, {2,3});     // -> {2,3}
    assert((s == std::vector<int>{2,3}));
    auto s2 = broadcast_shape({1,3}, {2,3});  // -> {2,3}
    assert((s2 == std::vector<int>{2,3}));
    auto s3 = broadcast_shape({2,1,4}, {1,3,1}); // -> {2,3,4}
    assert((s3 == std::vector<int>{2,3,4}));
    bool threw = false;
    try { (void)broadcast_shape({2,2}, {2,3}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
  }

  // squeeze
  {
    auto q = squeeze({1,2,1,3,1});
    assert((q == std::vector<int>{2,3}));
    auto q2 = squeeze({1,1});
    assert((q2 == std::vector<int>{1})); // keep scalar dim
  }

  // flatten
  {
    auto f = flatten({2,3,4});
    assert((f == std::vector<int>{24}));
  }

  // validate_reshape
  {
    auto v1 = validate_reshape({2,3}, {3,2});
    assert((v1 == std::vector<int>{3,2}));
    auto v2 = validate_reshape({2,3}, {-1});
    assert((v2 == std::vector<int>{6}));
    auto v3 = validate_reshape({2,3}, {2,-1,1}); // -> {2,3,1}
    assert((v3 == std::vector<int>{2,3,1}));

    bool threw = false;
    try { (void)validate_reshape({2,3}, {-1,-1}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw); threw = false;
    try { (void)validate_reshape({2,3}, {4,2}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
  }

  // tensor construction overflow guard
  {
    bool threw = false;
    try {
      t81::T729Tensor huge({INT_MAX, INT_MAX, INT_MAX});
    } catch (const std::overflow_error&) {
      threw = true;
    }
    assert(threw);
  }

  std::cout << "tensor_shape ok\n";
  return 0;
}
