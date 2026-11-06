#include <bit>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include "arch003/adapters.hpp"
#include "arch003/evidence.hpp"
using namespace arch003;
static void check(bool b, const char* m) {
  if (!b) throw std::runtime_error(m);
}
static std::string hex(Hash h) {
  constexpr char d[] = "0123456789abcdef";
  std::string s;
  for (auto b : h) {
    s += d[b >> 4];
    s += d[b & 15];
  }
  return s;
}
int main() {
  try {
    Sha256 hash;
    const std::uint8_t abc[]{'a', 'b', 'c'};
    hash.update(abc);
    check(hex(hash.finish()) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA256 abc");
    Sha256 empty;
    check(hex(empty.finish()) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "SHA256 empty");
    Sha256 multi;
    Bytes block(1000, 'a');
    multi.update(block);
    check(hex(multi.finish()) == "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3",
          "SHA256 blocks");
    std::array<std::uint64_t, 12> records{}, calls{};
    std::uint64_t seq = 0;
    std::array<std::uint64_t, 12> first{};
    enumerate([&](const Descriptor& d) {
      check(d.seq == seq++, "sequence");
      if (!records[d.family]) first[d.family] = d.seq;
      ++records[d.family];
      calls[d.family] += std::popcount(d.mask);
      check(d.mask && d.mask < 64, "mask");
    });
    check(records == family_records && calls == family_calls, "exact schedule arithmetic");
    check(seq == 408034, "total");
    const Symbols symbols{-1, 0, 1};
    const std::array<Bytes, 5> golden{{{255, 0, 1}, {19}, {129}, Bytes(32, 85), {4, 1}}};
    for (std::uint8_t f = 0; f < 5; ++f) {
      auto a = encode(f, symbols), b = encode(f, symbols);
      auto want = golden[f];
      if (f == NATIVE) want[0] = 100;
      check(a == want, "encoding golden");
      Request r{{f, 3, a}, {f, 3, b}};
      check(admit(r, f) == OK, "admission");
      for (unsigned i = 0; i < 3; ++i) check(decode_active(r.a, i) == symbols[i], "decode");
      Request zero{{f, 0, {}}, {f, 0, {}}};
      check(admit(zero, f) == OK, "null empty");
      r.a.format = 255;
      r.b.n = limit + 1;
      check(admit(r, f) == ERR_FORMAT, "format precedence");
      r.a.format = f;
      check(admit(r, f) == ERR_LENGTH, "length precedence");
      r.a.n = r.b.n;
      check(admit(r, f) == ERR_LIMIT, "limit precedence");
    }
    for (std::uint8_t f = 0; f < 5; ++f) {
      Descriptor d;
      d.family = 11;
      d.format = f;
      d.mask = mask_for(f);
      d.args = {4, 5, 0, 0, 0, 0};
      if (f == BYTE) continue;
      auto c = materialize(d, encode);
      check(admit(c.request(), f) == ERR_SYMBOL, "B active outranks A padding");
    }
    SplitMix64 rng(0);
    check(rng.next() == 0xe220a8397b1dcdafULL, "SplitMix known vector");
    Case c;
    c.fa = c.fb = C2;
    c.na = c.nb = 1;
    c.a = {0};
    c.b = {0};
    c.mask = 1;
    check(expected_frame(c, {OK, 1}).size() == 88, "expected layout");
    check(
        observed_frame(0, 0, {ERR_SYMBOL, {}}, consume(digest_initial, {ERR_SYMBOL, {}})).size() ==
            32,
        "observed layout");
    // No normal candidate or guard execution: oracle/verifier acceptance remains a review gate.
    std::cout << "bounded components OK; schedule metadata only; no candidate invocations\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
