#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include "arch003/evidence.hpp"
namespace arch003 {
Bytes oracle_encode(std::uint8_t, std::span<const std::int8_t>);
Result oracle_evaluate(const Request&, std::uint8_t);
Symbols oracle_decode(const Operand&);
}  // namespace arch003
using namespace arch003;
static void require(bool b, const char* message) {
  if (!b) throw std::runtime_error(message);
}
static Result evaluate(const Case& c) {
  std::optional<Result> common;
  for (unsigned k = 0; k < 6; ++k)
    if (c.mask & (1U << k)) {
      auto r = oracle_evaluate(c.request(), registered[k]);
      require(!common || *common == r, "FAIL_ORACLE: nonuniform mask");
      common = r;
    }
  require(common.has_value(), "FAIL_ORACLE: empty mask");
  return *common;
}
static void roundtrip(const Case& c) {
  for (auto o : {c.request().a, c.request().b}) {
    auto s = oracle_decode(o);
    require(oracle_encode(o.format, s) == Bytes(o.payload.begin(), o.payload.end()),
            "FAIL_ORACLE: canonical roundtrip");
    auto bytes = oracle_encode(BYTE, s);
    Operand byte{BYTE, o.n, bytes};
    require(oracle_decode(byte) == s, "FAIL_ORACLE: BYTE roundtrip");
  }
}
int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--fixed-wire-fixture") {
      for (std::uint8_t f = 0; f < 5; ++f) {
        Case c;
        c.seq = f;
        c.fa = c.fb = f;
        c.na = c.nb = 3;
        c.mask = mask_for(f);
        const Symbols a{-1, 0, 1}, b{-1, 1, 1};
        c.a = oracle_encode(f, a);
        c.b = oracle_encode(f, b);
        auto result = evaluate(c);
        require(result == Result{OK, 2}, "fixed wire truth");
        roundtrip(c);
        write_bytes(std::cout, expected_frame(c, result));
      }
      return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--component") {
      const Symbols a{-1, 0, 1, 1, 0}, b{-1, 1, 0, 1, 0};
      for (std::uint8_t f = 0; f < 5; ++f) {
        Case c;
        c.fa = c.fb = f;
        c.na = c.nb = 5;
        c.mask = mask_for(f);
        c.a = oracle_encode(f, a);
        c.b = oracle_encode(f, b);
        require(evaluate(c) == Result{OK, 3}, "component count");
        roundtrip(c);
        auto frame = expected_frame(c, evaluate(c));
        require(frame.size() == 88, "component wire");
      }
      std::cout << "oracle fixed components OK\n";
      return 0;
    }
    require(argc == 2 && std::string_view(argv[1]) == "--campaign" &&
                std::getenv("ARCH003_AUTHORIZED_WORKER"),
            "BLOCKED_AUTHORIZATION: campaign owner required");
    enumerate([](const Descriptor& d) {
      auto c = materialize(d, oracle_encode);
      auto r = evaluate(c);
      if (d.family <= 5) roundtrip(c);
      write_bytes(std::cout, expected_frame(c, r));
    });
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
