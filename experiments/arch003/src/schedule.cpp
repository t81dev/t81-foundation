#include "arch003/schedule.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace arch003 {
std::uint64_t SplitMix64::next() {
  auto z = (state_ += 0x9e3779b97f4a7c15ULL);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}
std::uint64_t SplitMix64::bounded(std::uint64_t m) {
  if (!m) throw std::invalid_argument("zero modulus");
  const auto rem = (0ULL - m) % m;
  for (;;) {
    auto u = next();
    if (!rem || u <= std::numeric_limits<std::uint64_t>::max() - rem) return u % m;
  }
}
std::vector<std::uint64_t> positions(std::uint64_t n) {
  if (!n) return {};
  std::vector<std::uint64_t> p{0, n - 1};
  for (unsigned k : {4, 5, 8, 16, 32, 64, 128, 256, 1024})
    for (auto v : {k - 1, k})
      if (v < n) p.push_back(v);
  std::sort(p.begin(), p.end());
  p.erase(std::unique(p.begin(), p.end()), p.end());
  return p;
}
void enumerate(const Visitor& visit) {
  std::uint64_t seq = 0;
  auto emit = [&](unsigned family, unsigned f, std::array<std::uint64_t, 6> args,
                  std::uint8_t mask = 0) {
    visit({seq++, static_cast<std::uint8_t>(family), static_cast<std::uint8_t>(f),
           mask ? mask : mask_for(f), args});
  };
  auto formats = [&](unsigned family, std::array<std::uint64_t, 6> a) {
    for (unsigned f = 0; f < 5; ++f) emit(family, f, a);
  };
  formats(0, {0, 0, 0, 0, 0, 0});
  for (unsigned a = 0; a < 3; ++a)
    for (unsigned b = 0; b < 3; ++b) formats(0, {1, a, b, 0, 0, 0});
  std::uint64_t power = 1;
  for (unsigned n = 1; n <= 5; ++n) {
    power *= 3;
    for (std::uint64_t a = 0; a < power; ++a)
      for (std::uint64_t b = 0; b < power; ++b) formats(1, {n, a, b, 0, 0, 0});
  }
  for (auto n : lengths) {
    for (unsigned pattern = 0; pattern < 14; ++pattern) formats(2, {n, pattern, 0, 0, 0, 0});
    for (auto p : positions(n))
      for (unsigned m = 0; m < 2; ++m) formats(2, {n, 14, p, m, 0, 0});
  }
  for (auto n : lengths)
    for (unsigned d = 0; d < 2; ++d)
      for (unsigned j = 0; j < 32; ++j) formats(3, {n, d, j, 0, 0, 0});
  for (unsigned endpoint = 0; endpoint < 4; ++endpoint) formats(4, {endpoint, 0, 0, 0, 0, 0});
  for (auto n : lengths)
    for (unsigned o = 0; o < 32; ++o)
      for (unsigned mode = 0; mode < 2; ++mode) formats(5, {n, o, mode, 0, 0, 0});
  for (unsigned f = 0; f < 5; ++f)
    for (unsigned op = 0; op < 2; ++op)
      for (unsigned v = 0; v < 256; ++v) emit(6, f, {op, v, 0, 0, 0, 0});
  for (unsigned op = 0; op < 2; ++op)
    for (unsigned i = 0; i < 128; ++i) emit(7, NATIVE, {op, i, 0, 0, 0, 0});
  for (unsigned f : {C2, PT5, NATIVE, PLANES}) {
    unsigned end = f == C2 ? 4 : f == PT5 ? 5 : f == NATIVE ? 128 : 8;
    for (unsigned n = 1; n < end; ++n)
      for (unsigned op = 0; op < 2; ++op)
        for (unsigned j = n; j < end; ++j)
          for (unsigned code = 0; code < (f == PT5 ? 2U : 3U); ++code)
            emit(8, f, {n, op, j, code, 0, 0});
  }
  for (unsigned f = 0; f < 5; ++f)
    for (auto pair : std::array<std::array<unsigned, 2>, 4>{{{0, 1}, {1, 0}, {4, 5}, {5, 4}}})
      emit(9, f, {0, pair[0], pair[1], 0, 0, 0});
  for (unsigned f = 0; f < 5; ++f)
    for (auto n : {limit + 1, std::numeric_limits<std::uint64_t>::max()})
      emit(9, f, {1, n, 0, 0, 0, 0});
  for (unsigned f = 0; f < 5; ++f)
    for (auto n : lengths)
      for (unsigned op = 0; op < 2; ++op) {
        if (n) emit(9, f, {2, n, op, 0, 0, 0});
        emit(9, f, {2, n, op, 1, 0, 0});
      }
  for (unsigned f = 0; f < 5; ++f)
    for (unsigned g = 0; g < 5; ++g)
      if (f != g) emit(10, f, {f, g, 0, 0, 0, 0}, 63);
  for (unsigned u = 5; u < 256; ++u) {
    emit(10, u, {u, BYTE, 0, 0, 0, 0}, 63);
    emit(10, BYTE, {BYTE, u, 0, 0, 0, 0}, 63);
    emit(10, u, {u, u, 0, 0, 0, 0}, 63);
  }
  for (unsigned f = 0; f < 5; ++f) emit(10, f, {f, f, 0, 0, 0, 0}, 63 ^ mask_for(f));
  for (unsigned f = 0; f < 5; ++f) {
    unsigned last = f == BYTE ? 5 : 6;
    for (unsigned a = 0; a < last; ++a)
      for (unsigned b = a + 1; b < last; ++b) emit(11, f, {a, b, 0, 0, 0, 0});
    emit(11, f, {6, 0, 0, 0, 0, 0});
    if (f != BYTE) {
      emit(11, f, {7, 0, 0, 0, 0, 0});
      emit(11, f, {8, 0, 0, 0, 0, 0});
    }
  }
}
static void field(Bytes& b, std::uint64_t i, unsigned code) {
  auto sh = 2 * (i % 4);
  b[i / 4] = static_cast<std::uint8_t>((b[i / 4] & ~(3U << sh)) | (code << sh));
}
Case materialize(const Descriptor& d, Encoder encoder) {
  const auto x = d.args;
  auto f = d.format;
  Case c;
  c.seq = d.seq;
  c.family = d.family;
  c.fa = c.fb = f;
  c.mask = d.mask;
  Symbols a, b;
  auto zeros = [&](std::uint64_t na, std::uint64_t nb) {
    c.na = na;
    c.nb = nb;
    a.assign(na, 0);
    b.assign(nb, 0);
  };
  auto cycle = [&](std::uint64_t n) {
    zeros(n, n);
    for (std::uint64_t i = 0; i < n; ++i) a[i] = b[i] = static_cast<std::int8_t>(i % 3) - 1;
  };
  auto seeded = [&](std::uint64_t n, unsigned endpoint, unsigned j) {
    zeros(n, n);
    SplitMix64 rng(0x4152434830303200ULL + static_cast<std::uint64_t>(endpoint) * (1ULL << 24) + j);
    auto draw = [&]() {
      auto v = rng.bounded(endpoint % 2 ? 20 : 3);
      return static_cast<std::int8_t>(endpoint % 2 ? (v < 18    ? 0
                                                      : v == 18 ? -1
                                                                : 1)
                                                   : static_cast<int>(v) - 1);
    };
    for (std::uint64_t i = 0; i < n; ++i) {
      a[i] = draw();
      b[i] = draw();
    }
  };
  if (d.family == 0) {
    zeros(x[0], x[0]);
    if (x[0]) {
      a[0] = static_cast<std::int8_t>(x[1]) - 1;
      b[0] = static_cast<std::int8_t>(x[2]) - 1;
    }
  } else if (d.family == 1) {
    zeros(x[0], x[0]);
    auto va = x[1], vb = x[2];
    for (std::uint64_t i = 0; i < x[0]; ++i) {
      a[i] = static_cast<std::int8_t>(va % 3) - 1;
      b[i] = static_cast<std::int8_t>(vb % 3) - 1;
      va /= 3;
      vb /= 3;
    }
  } else if (d.family == 2) {
    zeros(x[0], x[0]);
    for (std::uint64_t i = 0; i < x[0]; ++i) {
      if (x[1] < 9) {
        a[i] = static_cast<std::int8_t>(x[1] / 3) - 1;
        b[i] = static_cast<std::int8_t>(x[1] % 3) - 1;
      } else if (x[1] < 12) {
        a[i] = static_cast<std::int8_t>(i % 3) - 1;
        b[i] = static_cast<std::int8_t>((i + x[1] - 9) % 3) - 1;
      } else if (x[1] < 14) {
        a[i] = i % 2 ? 1 : -1;
        b[i] = (i + x[1] - 12) % 2 ? 1 : -1;
      } else
        b[i] = x[3] ? 0 : 1;
    }
    if (x[1] == 14) b[x[2]] = x[3] ? 1 : 0;
  } else if (d.family == 3)
    seeded(x[0], x[1], x[2]);
  else if (d.family == 4)
    seeded(x[0] < 2 ? 1ULL << 20 : limit, x[0], 0);
  else if (d.family == 5) {
    cycle(x[0]);
    if (x[0]) b.back() = b.back() == 1 ? -1 : static_cast<std::int8_t>(b.back() + 1);
    c.oa = x[1];
    c.ob = (7 * x[1] + 3) % 32;
    c.mode = x[2];
  } else if (d.family == 6) {
    auto n = f == BYTE ? 1 : f == C2 ? 4 : f == PT5 ? 5 : f == NATIVE ? 4 : 8;
    zeros(n, n);
  } else if (d.family == 7)
    zeros(128, 128);
  else if (d.family == 8)
    zeros(x[0], x[0]);
  else if (d.family == 9) {
    if (x[0] == 0)
      zeros(x[1], x[2]);
    else if (x[0] == 1) {
      c.na = c.nb = x[1];
    } else
      zeros(x[1], x[1]);
  } else if (d.family == 10) {
    c.fa = x[0];
    c.fb = x[1];
    return c;
  } else if (d.family == 11)
    zeros(1, 1);
  else
    throw std::invalid_argument("unknown schedule family");
  c.a = encoder(f, a);
  c.b = encoder(f, b);
  if (d.family == 6) {
    auto& p = x[0] ? c.b : c.a;
    p[0] = x[1];
    if (f == PLANES) p[1] = x[1];
  }
  if (d.family == 7) field(x[0] ? c.b : c.a, x[1], 3);
  if (d.family == 8) {
    auto& p = x[1] ? c.b : c.a;
    if (f == C2)
      field(p, x[2], x[3] + 1);
    else if (f == NATIVE) {
      constexpr unsigned codes[]{0, 2, 3};
      field(p, x[2], codes[x[3]]);
    } else if (f == PT5) {
      unsigned power = 1;
      for (unsigned j = 0; j < x[2]; ++j) power *= 3;
      p[0] = static_cast<std::uint8_t>(p[0] + (x[3] ? 1 : -1) * static_cast<int>(power));
    } else {
      unsigned code = x[3] + 1;
      if (code & 2) p[0] |= 1U << x[2];
      if (code & 1) p[1] |= 1U << x[2];
    }
  }
  if (d.family == 9 && x[0] == 2) {
    auto& p = x[2] ? c.b : c.a;
    if (x[3])
      p.push_back(0);
    else
      p.pop_back();
  }
  if (d.family == 11) {
    auto badsymbol = [&](Bytes& p) {
      if (f == BYTE)
        p[0] = 2;
      else if (f == C2)
        field(p, 0, 2);
      else if (f == PT5)
        p[0] = 243;
      else if (f == NATIVE)
        field(p, 0, 3);
      else {
        p[0] |= 1;
        p[1] |= 1;
      }
    };
    auto badpad = [&](Bytes& p) {
      if (f == C2)
        field(p, 1, 1);
      else if (f == PT5)
        p[0] -= 3;
      else if (f == NATIVE)
        field(p, 1, 0);
      else if (f == PLANES)
        p[0] |= 2;
    };
    if (x[0] < 6) {
      auto has = [&](unsigned fault) { return x[0] == fault || x[1] == fault; };
      if (has(2)) c.na = c.nb = limit + 1;
      if (has(1)) c.nb = c.na + 1;
      if (has(0)) c.fa = 255;
      if (has(3)) c.a.push_back(0);
      if (has(4)) badsymbol(c.b);
      if (has(5)) badpad(c.a);
    } else if (x[0] == 6) {
      badsymbol(c.a);
      badsymbol(c.b);
    } else if (x[0] == 7) {
      badsymbol(c.a);
      badpad(c.b);
    } else {
      badpad(c.a);
      badpad(c.b);
    }
  }
  return c;
}
}  // namespace arch003
