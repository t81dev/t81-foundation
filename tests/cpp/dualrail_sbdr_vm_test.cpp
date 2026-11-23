// Unit & Regression Tests for Dual-Rail TMatMul, Block-Zero Skip, SBDR1T2D CanonFS Load,
// TensorNumericClass propagation, and tensor_canon_hashes_ invalidation on TSet.

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/types/detail/dmath.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"
#include "../../vm/internal/tensor_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

t81::weights::NativeTensor pack_canonical_fixed(const std::vector<int8_t>& trits,
                                                std::vector<uint64_t> shape) {
  using DFixed = t81::core::detail::DFixed;
  t81::weights::NativeTensor nt;
  nt.format = t81::weights::NativeFormat::CanonicalFixed;
  nt.shape = std::move(shape);
  nt.trits = trits.size();

  auto ser = [](const DFixed& f) {
    std::ostringstream oss(std::ios::binary);
    f.v.serialize(oss);
    return oss.str();
  };
  const std::string s_zero = ser(DFixed::zero());
  const std::string s_one = ser(DFixed::one());
  const std::string s_neg = ser(-DFixed::one());

  constexpr size_t kElemBytes = sizeof(uint64_t) + DFixed::Storage::kNumBytes;
  constexpr size_t kElemLimbs = kElemBytes / sizeof(uint64_t);
  nt.data.assign(trits.size() * kElemLimbs, 0ULL);

  uint8_t* dst = reinterpret_cast<uint8_t*>(nt.data.data());
  for (size_t i = 0; i < trits.size(); ++i) {
    const std::string& src = (trits[i] > 0) ? s_one : ((trits[i] < 0) ? s_neg : s_zero);
    std::memcpy(dst, src.data(), kElemBytes);
    dst += kElemBytes;
  }
  return nt;
}

std::vector<std::byte> serialize_canon_tensor(const t81::weights::NativeTensor& tensor) {
  std::vector<std::byte> buffer;
  buffer.reserve(72 + tensor.data.size() * 8);
  buffer.push_back(static_cast<std::byte>(0x20));
  buffer.push_back(static_cast<std::byte>(1));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.shape.size())));
  for (int i = 0; i < 4; ++i) buffer.push_back(static_cast<std::byte>(0));
  for (int i = 0; i < 8; ++i) {
    const uint64_t dim = (i < static_cast<int>(tensor.shape.size())) ? tensor.shape[i] : 0;
    for (int b = 0; b < 8; ++b) {
      buffer.push_back(static_cast<std::byte>((dim >> (b * 8)) & 0xFF));
    }
  }
  for (uint64_t val : tensor.data) {
    for (int b = 0; b < 8; ++b) {
      buffer.push_back(static_cast<std::byte>((val >> (b * 8)) & 0xFF));
    }
  }
  return buffer;
}

std::vector<std::byte> serialize_sbdr1t2d(const std::vector<int8_t>& trits, size_t K, size_t N) {
  const size_t W64 = (K + 63) / 64;
  const size_t total_words = N * W64;
  const size_t mask_words = (total_words + 63) / 64;
  std::vector<uint64_t> Bp(total_words, 0ULL), Bn(total_words, 0ULL);
  for (size_t k = 0; k < K; ++k) {
    const size_t w = k >> 6;
    const uint64_t bit = 1ULL << (k & 63);
    for (size_t n = 0; n < N; ++n) {
      const int8_t v = trits[k * N + n];
      if (v > 0)
        Bp[n * W64 + w] |= bit;
      else if (v < 0)
        Bn[n * W64 + w] |= bit;
    }
  }
  std::vector<uint64_t> mask(mask_words, 0ULL);
  std::vector<uint64_t> payload;
  for (size_t wi = 0; wi < total_words; ++wi) {
    if ((Bp[wi] | Bn[wi]) != 0ULL) {
      mask[wi >> 6] |= (1ULL << (wi & 63));
      payload.push_back(Bp[wi]);
      payload.push_back(Bn[wi]);
    }
  }
  std::vector<std::byte> out(24 + mask_words * 8 + payload.size() * 8);
  static constexpr char kMagic[8] = {'S', 'B', 'D', 'R', '1', 'T', '2', 'D'};
  const uint64_t K64 = static_cast<uint64_t>(K);
  const uint64_t N64 = static_cast<uint64_t>(N);
  std::memcpy(out.data(), kMagic, 8);
  std::memcpy(out.data() + 8, &K64, 8);
  std::memcpy(out.data() + 16, &N64, 8);
  std::memcpy(out.data() + 24, mask.data(), mask_words * 8);
  if (!payload.empty()) {
    std::memcpy(out.data() + 24 + mask_words * 8, payload.data(), payload.size() * 8);
  }
  return out;
}

void test_sbdr1t2d_and_block_zero_skip_and_tset_invalidation() {
  constexpr size_t M = 16, K = 128, N = 16;
  std::mt19937 rng(4242);
  std::uniform_int_distribution<int> dist(-1, 1);

  std::vector<int8_t> A(M * K, 0), B(K * N, 0);
  // Make rows 0..7 have 50% block-zero sparsity (second 64-trit block all zeros) and row 8 all
  // zero.
  for (size_t m = 0; m < M; ++m) {
    if (m == 8) continue;
    const size_t k_lim = (m < 8) ? 64 : K;
    for (size_t k = 0; k < k_lim; ++k) A[m * K + k] = static_cast<int8_t>(dist(rng));
  }
  // Ensure A[0, 0] == 1 so mutating B[0, 0] via TSet visibly changes C[0, 0]
  A[0] = 1;
  for (size_t i = 0; i < K * N; ++i) B[i] = static_cast<int8_t>(dist(rng));
  B[0] = 1;

  auto oracle_matmul = [&](const std::vector<int8_t>& lhs, const std::vector<int8_t>& rhs) {
    std::vector<float> out(M * N, 0.0f);
    for (size_t m = 0; m < M; ++m) {
      for (size_t n = 0; n < N; ++n) {
        int32_t acc = 0;
        for (size_t k = 0; k < K; ++k) {
          acc += static_cast<int32_t>(lhs[m * K + k]) * static_cast<int32_t>(rhs[k * N + n]);
        }
        out[m * N + n] = static_cast<float>(acc);
      }
    }
    return out;
  };

  const auto expected_before = oracle_matmul(A, B);
  std::vector<int8_t> B_mut = B;
  B_mut[0] = -1;  // flip B[0, 0] from +1 to -1 => C[0, 0] must change by -2!
  const auto expected_after = oracle_matmul(A, B_mut);

  auto driver =
      std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  auto ref_a = driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                                    serialize_canon_tensor(pack_canonical_fixed(A, {M, K})));
  auto ref_b =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor, serialize_sbdr1t2d(B, K, N));
  check(ref_a.has_value() && ref_b.has_value(), "CanonFS write_object failed");
  if (!ref_a.has_value() || !ref_b.has_value()) return;

  t81::tisc::Program prog;
  prog.symbol_pool.push_back("sha3-256:" + ref_a->hash.h.to_string());  // symbol 1
  prog.symbol_pool.push_back("sha3-256:" + ref_b->hash.h.to_string());  // symbol 2
  // Load A -> r2, Load SBDR1T2D B -> r4, C1 = TMatMul(r2, r4) -> r5,
  // Mutate B[0] = -1 via TSet(r4, r6=0, r7=-1), C2 = TMatMul(r2, r4) -> r8, Halt
  t81::tisc::Insn ld_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  ld_a.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  t81::tisc::Insn ld_b{t81::tisc::Opcode::LoadImm, 3, 2, 0};
  ld_b.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  prog.insns.push_back(ld_a);
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 2, 1, 0});
  prog.insns.push_back(ld_b);
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 4, 3, 0});
  prog.insns.push_back({t81::tisc::Opcode::TMatMul, 5, 2, 4});
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 6, 0, 0});
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 7, -1, 0});
  prog.insns.push_back({t81::tisc::Opcode::TSet, 4, 6, 7});
  prog.insns.push_back({t81::tisc::Opcode::TMatMul, 8, 2, 4});
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  t81::axion::Policy pol;
  pol.tier = 2;
  pol.allowed_tensor_hashes.push_back(prog.symbol_pool[0]);
  pol.allowed_tensor_hashes.push_back(prog.symbol_pool[1]);
  t81::vm::internal::reset_dualrail_telemetry();
  auto vm = t81::vm::make_interpreter_vm(
      std::make_unique<t81::axion::AIHookEngine>(std::make_unique<t81::axion::PolicyEngine>(pol)));
  vm->set_canonfs_driver(driver);
  vm->load_program(prog);
  auto res = vm->run_to_halt();
  if (!res.has_value()) {
    std::cerr << "  trap=" << static_cast<int>(res.error()) << " pc=" << vm->state().contexts[0].pc
              << "\n";
  }
  check(res.has_value(), "VM failed to halt on SBDR1T2D + TSet invalidation test");
  if (!res.has_value()) return;

  const auto tel = t81::vm::internal::dualrail_telemetry();
  check(tel.fastpath_hits == 2 && tel.fallback_count == 0,
        "Expected 2 Dual-Rail fastpath hits and 0 silent fallbacks on ternary TMatMul");

  const auto& ctx = vm->state().contexts[0];
  const auto& c1 = vm->state().tensors[static_cast<size_t>(ctx.registers[5] - 1)].value();
  const auto& c2 = vm->state().tensors[static_cast<size_t>(ctx.registers[8] - 1)].value();

  check(c1.numeric_class() == t81::TensorNumericClass::ExactInt,
        "TMatMul output with non-trit integer sums must be tagged ExactInt");
  check(c1.data() == expected_before, "SBDR1T2D + block-zero skip TMatMul mismatch vs oracle");
  check(c2.data() == expected_after,
        "TSet mutation did not invalidate CanonDualRailRhsCache (stale RHS hit!)");
}

void test_sbdr1t2d_rejects_malformed_planes() {
  auto driver =
      std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  // Build malformed SBDR1T2D: K=64, N=1, mask=1, active word has pos=1, neg=1 (non-disjoint!)
  std::vector<std::byte> bad(24 + 8 + 16);
  static constexpr char kMagic[8] = {'S', 'B', 'D', 'R', '1', 'T', '2', 'D'};
  const uint64_t K64 = 64, N64 = 1, mask = 1, pos = 1, neg = 1;
  std::memcpy(bad.data(), kMagic, 8);
  std::memcpy(bad.data() + 8, &K64, 8);
  std::memcpy(bad.data() + 16, &N64, 8);
  std::memcpy(bad.data() + 24, &mask, 8);
  std::memcpy(bad.data() + 32, &pos, 8);
  std::memcpy(bad.data() + 40, &neg, 8);

  auto ref = driver->write_object(t81::canonfs::ObjectType::CanonTensor, bad);
  check(ref.has_value(), "CanonFS write_object for malformed SBDR1T2D");
  if (!ref.has_value()) return;

  t81::tisc::Program prog;
  prog.symbol_pool.push_back("sha3-256:" + ref->hash.h.to_string());
  t81::tisc::Insn ld_bad{t81::tisc::Opcode::LoadImm, 0, 1, 0};
  ld_bad.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  prog.insns.push_back(ld_bad);
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 1, 0, 0});
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->set_canonfs_driver(driver);
  vm->load_program(prog);
  auto res = vm->run_to_halt();
  check(!res.has_value() && res.error() == t81::vm::Trap::DecodeFault,
        "Malformed non-disjoint SBDR1T2D must trap with Trap::DecodeFault");
}

void test_sbdr1t2d_regime_aware_sparse128_routing() {
  constexpr size_t M = 8, K = 256, N = 16;
  std::mt19937 rng(9001);
  std::uniform_int_distribution<int> dist(-1, 1);

  std::vector<int8_t> A(M * K, 0), B(K * N, 0);
  for (auto& v : A) v = static_cast<int8_t>(dist(rng));
  // 75% 128-trit block sparsity in B: only first 128-trit block populated on even cols.
  for (size_t n = 0; n < N; n += 2) {
    for (size_t k = 0; k < 128; ++k) B[k * N + n] = static_cast<int8_t>(dist(rng));
  }

  std::vector<float> expected(M * N, 0.0f);
  for (size_t m = 0; m < M; ++m) {
    for (size_t n = 0; n < N; ++n) {
      int32_t acc = 0;
      for (size_t k = 0; k < K; ++k) {
        acc += static_cast<int32_t>(A[m * K + k]) * static_cast<int32_t>(B[k * N + n]);
      }
      expected[m * N + n] = static_cast<float>(acc);
    }
  }

  auto driver =
      std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  auto ref_a =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor, serialize_sbdr1t2d(A, M, K));
  auto ref_b =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor, serialize_sbdr1t2d(B, K, N));
  check(ref_a.has_value() && ref_b.has_value(), "CanonFS write_object failed (sparse128 test)");
  if (!ref_a.has_value() || !ref_b.has_value()) return;

  t81::tisc::Program prog;
  prog.symbol_pool.push_back("sha3-256:" + ref_a->hash.h.to_string());
  prog.symbol_pool.push_back("sha3-256:" + ref_b->hash.h.to_string());
  t81::tisc::Insn ld_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  ld_a.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  t81::tisc::Insn ld_b{t81::tisc::Opcode::LoadImm, 3, 2, 0};
  ld_b.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  prog.insns.push_back(ld_a);
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 2, 1, 0});
  prog.insns.push_back(ld_b);
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 4, 3, 0});
  prog.insns.push_back({t81::tisc::Opcode::TMatMul, 5, 2, 4});
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  t81::axion::Policy pol;
  pol.tier = 2;
  pol.allowed_tensor_hashes.push_back(prog.symbol_pool[0]);
  pol.allowed_tensor_hashes.push_back(prog.symbol_pool[1]);

  t81::vm::internal::reset_dualrail_telemetry();
  auto vm = t81::vm::make_interpreter_vm(
      std::make_unique<t81::axion::AIHookEngine>(std::make_unique<t81::axion::PolicyEngine>(pol)));
  vm->set_canonfs_driver(driver);
  vm->load_program(prog);
  auto res = vm->run_to_halt();
  check(res.has_value(), "VM failed to halt on sparse128 routing test");
  if (!res.has_value()) return;

  const auto tel = t81::vm::internal::dualrail_telemetry();
  check(tel.sparse128_hits == 1 && tel.fallback_count == 0,
        "Expected 1 sparse128 hit and 0 fallbacks on 75%-sparse SBDR1T2D weight matrix");
  const auto& ctx = vm->state().contexts[0];
  const auto& c = vm->state().tensors[static_cast<size_t>(ctx.registers[5] - 1)].value();
  check(c.data() == expected, "sparse128 TMatMul output mismatch vs exact oracle");
}

void test_mixed_dense_and_sparse128_regime_routing_and_repeat_sha3() {
  constexpr size_t M = 8, K = 256, N = 16;
  std::mt19937 rng(20261123);
  std::uniform_int_distribution<int> dist(-1, 1);

  std::vector<int8_t> A(M * K, 0), B_dense(K * N, 0), B_sparse(K * N, 0);
  for (auto& v : A) v = static_cast<int8_t>(dist(rng));
  for (auto& v : B_dense) v = static_cast<int8_t>(dist(rng));
  // 75% 128-trit block sparsity in B_sparse
  for (size_t n = 0; n < N; n += 2) {
    for (size_t k = 0; k < 128; ++k) B_sparse[k * N + n] = static_cast<int8_t>(dist(rng));
  }

  auto oracle = [&](const std::vector<int8_t>& rhs) {
    std::vector<float> out(M * N, 0.0f);
    for (size_t m = 0; m < M; ++m) {
      for (size_t n = 0; n < N; ++n) {
        int32_t acc = 0;
        for (size_t k = 0; k < K; ++k) {
          acc += static_cast<int32_t>(A[m * K + k]) * static_cast<int32_t>(rhs[k * N + n]);
        }
        out[m * N + n] = static_cast<float>(acc);
      }
    }
    return out;
  };
  const auto exp_dense = oracle(B_dense);
  const auto exp_sparse = oracle(B_sparse);

  auto driver =
      std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  auto ref_a =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor, serialize_sbdr1t2d(A, M, K));
  auto ref_bd =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor, serialize_sbdr1t2d(B_dense, K, N));
  auto ref_bs = driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                                     serialize_sbdr1t2d(B_sparse, K, N));
  check(ref_a.has_value() && ref_bd.has_value() && ref_bs.has_value(),
        "CanonFS write_object failed (mixed regime test)");
  if (!ref_a.has_value() || !ref_bd.has_value() || !ref_bs.has_value()) return;

  t81::tisc::Program prog;
  prog.symbol_pool.push_back("sha3-256:" + ref_a->hash.h.to_string());
  prog.symbol_pool.push_back("sha3-256:" + ref_bd->hash.h.to_string());
  prog.symbol_pool.push_back("sha3-256:" + ref_bs->hash.h.to_string());
  auto load_sym = [](int reg, int sym_id) {
    t81::tisc::Insn insn{t81::tisc::Opcode::LoadImm, reg, sym_id, 0};
    insn.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
    return insn;
  };
  prog.insns.push_back(load_sym(1, 1));
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 2, 1, 0});
  prog.insns.push_back(load_sym(3, 2));
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 4, 3, 0});
  prog.insns.push_back(load_sym(5, 3));
  prog.insns.push_back({t81::tisc::Opcode::TLoadHash, 6, 5, 0});
  prog.insns.push_back({t81::tisc::Opcode::TMatMul, 7, 2, 4});
  prog.insns.push_back({t81::tisc::Opcode::TMatMul, 8, 2, 6});
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  t81::axion::Policy pol;
  pol.tier = 2;
  pol.allowed_tensor_hashes = prog.symbol_pool;

  auto run_once = [&]() {
    t81::vm::internal::reset_dualrail_telemetry();
    auto vm = t81::vm::make_interpreter_vm(
        std::make_unique<t81::axion::AIHookEngine>(std::make_unique<t81::axion::PolicyEngine>(pol)));
    vm->set_canonfs_driver(driver);
    vm->load_program(prog);
    auto res = vm->run_to_halt();
    check(res.has_value(), "Mixed dense+sparse128 VM run failed to halt");
    const auto tel = t81::vm::internal::dualrail_telemetry();
    check(tel.fastpath_hits == 2 && tel.dense_hits == 1 && tel.sparse128_hits == 1 &&
              tel.fallback_count == 0,
          "Expected 1 dense_hit + 1 sparse128_hit + 0 fallbacks in mixed-regime VM run");
    const auto& ctx = vm->state().contexts[0];
    const auto& cd = vm->state().tensors[static_cast<size_t>(ctx.registers[7] - 1)].value();
    const auto& cs = vm->state().tensors[static_cast<size_t>(ctx.registers[8] - 1)].value();
    check(cd.data() == exp_dense, "Mixed-regime dense output mismatch vs oracle");
    check(cs.data() == exp_sparse, "Mixed-regime sparse128 output mismatch vs oracle");
    std::vector<uint8_t> raw((cd.data().size() + cs.data().size()) * sizeof(float));
    std::memcpy(raw.data(), cd.data().data(), cd.data().size() * sizeof(float));
    std::memcpy(raw.data() + cd.data().size() * sizeof(float), cs.data().data(),
                cs.data().size() * sizeof(float));
    return t81::crypto::sha3_512(raw);
  };

  const auto h1 = run_once();
  const auto h2 = run_once();
  check(h1 == h2, "Repeat-run SHA3-512 mismatch on mixed dense+sparse128 VM program");
}

}  // namespace

int main() {
  test_sbdr1t2d_and_block_zero_skip_and_tset_invalidation();
  test_sbdr1t2d_rejects_malformed_planes();
  test_sbdr1t2d_regime_aware_sparse128_routing();
  test_mixed_dense_and_sparse128_regime_routing_and_repeat_sha3();
  if (g_failures != 0) {
    std::cerr << "t81_dualrail_sbdr_vm_test: " << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "t81_dualrail_sbdr_vm_test: PASSED\n";
  return 0;
}
