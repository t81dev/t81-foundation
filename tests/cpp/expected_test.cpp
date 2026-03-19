#include "t81/support/expected.hpp"
#include <cassert>
#include <iostream>
#include <string>

void test_expected_basic() {
  t81::expected<int, std::string> e(42);
  assert(e.has_value());
  assert(e.value() == 42);
  assert(*e == 42);

  t81::expected<int, std::string> err(t81::unexpect, "error");
  assert(!err.has_value());
  assert(err.error() == "error");
}

void test_expected_monadic() {
  t81::expected<int, std::string> e(42);

  // transform
  [[maybe_unused]] auto e2 = e.transform([](int x) { return x * 2; });
  assert(e2.has_value());
  assert(e2.value() == 84);

  // and_then
  auto e3 = e2.and_then(
      [](int x) -> t81::expected<std::string, std::string> { return std::to_string(x); });
  assert(e3.has_value());
  assert(e3.value() == "84");

  // transform_error
  t81::expected<int, int> err(std::unexpect, 1);
  [[maybe_unused]] auto err2 = err.transform_error([](int x) { return std::to_string(x); });
  assert(!err2.has_value());
  assert(err2.error() == "1");

  // or_else
  auto err3 =
      err2.or_else([](const std::string& /*s*/) -> t81::expected<int, std::string> { return 0; });
  assert(err3.has_value());
  assert(err3.value() == 0);
}

void test_expected_void() {
  t81::expected<void, int> e;
  assert(e.has_value());

  [[maybe_unused]] auto e2 = e.transform([]() { return 42; });
  assert(e2.has_value());
  assert(e2.value() == 42);

  [[maybe_unused]] t81::expected<void, int> err(t81::unexpect, 404);
  assert(!err.has_value());
  assert(err.error() == 404);
}

int main() {
  test_expected_basic();
  test_expected_monadic();
  test_expected_void();
  std::cout << "expected_test passed!" << std::endl;
  return 0;
}
