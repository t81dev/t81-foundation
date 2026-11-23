// Full-Stack T81 Integration Pipeline:
//   CanonFS (content-addressed packed ternary tensors)
//   -> Axion (AIHookEngine allowlist enforcement + fail-closed SecurityFault test)
//   -> TISC (LoadImm -> TLoadHash -> TLoadHash -> TMatMul -> Halt)
//   -> T81VM (executing Seam 1 Dual-Rail NEON/popcount tensor_matmul_2d)
//   -> SHA3-512 execution receipt verified bit-exact against independent oracle.

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/types/detail/dmath.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

// Pack balanced trits {-1, 0, +1} into NativeFormat::CanonicalFixed so CanonFS -> TLoadHash
// constructs T729DynamicTensor via from_canonical_fixed() with zero float/div_mod overhead.
t81::weights::NativeTensor pack_ternary_native_tensor(const std::vector<int8_t>& trits,
                                                      std::vector<uint64_t> shape) {
  using DFixed = t81::core::detail::DFixed;
  t81::weights::NativeTensor nt;
  nt.format = t81::weights::NativeFormat::CanonicalFixed;
  nt.shape = std::move(shape);
  nt.trits = trits.size();

  const DFixed d_zero = DFixed::zero();
  const DFixed d_one = DFixed::one();
  const DFixed d_neg = -DFixed::one();

  auto ser = [](const DFixed& f) {
    std::ostringstream oss(std::ios::binary);
    f.v.serialize(oss);
    return oss.str();
  };
  const std::string s_zero = ser(d_zero);
  const std::string s_one = ser(d_one);
  const std::string s_neg = ser(d_neg);

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
  const size_t total_bytes = 24 + mask_words * 8 + payload.size() * 8;
  std::vector<std::byte> out(total_bytes);
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

std::string sha3_hex(const void* data, size_t bytes) {
  return t81::crypto::sha3_512_hex(
      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data), bytes));
}

}  // namespace

int main() {
  constexpr size_t M = 64, K = 128, N = 64;
  std::mt19937 rng(202611);
  std::uniform_int_distribution<int> dist(-1, 1);

  std::vector<int8_t> A_trits(M * K), B_trits(K * N);
  for (auto& v : A_trits) v = static_cast<int8_t>(dist(rng));
  for (auto& v : B_trits) v = static_cast<int8_t>(dist(rng));

  // Independent exact scalar oracle
  std::vector<float> C_oracle(M * N, 0.0f);
  for (size_t m = 0; m < M; ++m) {
    for (size_t n = 0; n < N; ++n) {
      int32_t acc = 0;
      for (size_t k = 0; k < K; ++k) {
        acc += static_cast<int32_t>(A_trits[m * K + k]) * static_cast<int32_t>(B_trits[k * N + n]);
      }
      C_oracle[m * N + n] = static_cast<float>(acc);
    }
  }

  // 1. Write packed ternary tensors into live CanonFS driver (A=CanonicalFixed, B=SBDR1T2D!)
  auto driver =
      std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  const auto bytes_a = serialize_canon_tensor(pack_ternary_native_tensor(A_trits, {M, K}));
  const auto bytes_b = serialize_sbdr1t2d(B_trits, K, N);

  auto ref_a = driver->write_object(t81::canonfs::ObjectType::CanonTensor, bytes_a);
  auto ref_b = driver->write_object(t81::canonfs::ObjectType::CanonTensor, bytes_b);
  if (!ref_a.has_value() || !ref_b.has_value()) {
    std::cerr << "FAIL: CanonFS write_object failed\n";
    return 1;
  }
  const std::string hash_a = "sha3-256:" + ref_a->hash.h.to_string();
  const std::string hash_b = "sha3-256:" + ref_b->hash.h.to_string();

  // 2. Build TISC program: TLoadHash(A) -> TLoadHash(B) -> TMatMul(A, B) -> Halt
  t81::tisc::Program prog;
  prog.symbol_pool.push_back(hash_a);  // symbol 1
  prog.symbol_pool.push_back(hash_b);  // symbol 2

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

  const auto bc = t81::tisc::encode(prog);
  const std::string prog_sha = sha3_hex(bc.data(), bc.size());

  // 3. Authorized T81VM execution (CanonFS + Axion AIHookEngine + Seam 1 Dual-Rail TMatMul)
  t81::axion::Policy allow_pol;
  allow_pol.allowed_tensor_hashes = {hash_a, hash_b};
  auto vm_ok = t81::vm::make_interpreter_vm(std::make_unique<t81::axion::AIHookEngine>(
      std::make_unique<t81::axion::PolicyEngine>(allow_pol)));
  vm_ok->set_canonfs_driver(driver);
  vm_ok->load_program(prog);

  const auto t0 = Clock::now();
  const auto res_ok = vm_ok->run_to_halt();
  const double vm_us = std::chrono::duration<double, std::micro>(Clock::now() - t0).count();
  if (!res_ok.has_value()) {
    std::cerr << "FAIL: Authorized T81VM execution trapped\n";
    return 1;
  }

  const int64_t out_handle = vm_ok->state().contexts[0].registers[5];
  const auto& tensors = vm_ok->state().tensors;
  const size_t out_idx = static_cast<size_t>(out_handle - 1);
  if (out_handle <= 0 || out_idx >= tensors.size() || !tensors[out_idx].has_value() ||
      tensors[out_idx]->data() != C_oracle) {
    std::cerr << "FAIL: T81VM TMatMul output did not match independent scalar oracle\n";
    return 1;
  }
  const auto& out_tensor = *tensors[out_idx];

  // 4. Unauthorized T81VM execution -> fail-closed Trap::SecurityFault before TMatMul
  t81::axion::Policy deny_pol;
  deny_pol.allowed_tensor_hashes = {hash_a};  // hash_b NOT allowlisted!
  auto vm_deny = t81::vm::make_interpreter_vm(std::make_unique<t81::axion::AIHookEngine>(
      std::make_unique<t81::axion::PolicyEngine>(deny_pol)));
  vm_deny->set_canonfs_driver(driver);
  vm_deny->load_program(prog);
  const auto res_deny = vm_deny->run_to_halt();
  if (res_deny.has_value() || res_deny.error() != t81::vm::Trap::SecurityFault) {
    std::cerr << "FAIL: Unauthorized T81VM execution did not fail-closed with SecurityFault\n";
    return 1;
  }

  const std::string out_sha =
      sha3_hex(out_tensor.data().data(), out_tensor.data().size() * sizeof(float));
  const std::string canonical = prog_sha + "|" + hash_a + "|" + hash_b + "|" + out_sha;
  const std::string receipt = "sha3-512:" + sha3_hex(canonical.data(), canonical.size());

  std::cout << std::fixed << std::setprecision(1);
  std::cout << "{\n"
            << "  \"schema\": \"t81.full-stack-pipeline.v1\",\n"
            << "  \"ok\": true,\n"
            << "  \"layers_connected\": [\"CanonFS\", \"Axion\", \"TISC\", \"T81VM\", "
               "\"DualRail_TMatMul\"],\n"
            << "  \"shape\": \"64x128x64\",\n"
            << "  \"oracle_exact_match\": true,\n"
            << "  \"unauthorized_trapped_security_fault\": true,\n"
            << "  \"vm_e2e_us\": " << vm_us << ",\n"
            << "  \"canonfs_hash_a\": \"" << hash_a << "\",\n"
            << "  \"canonfs_hash_b\": \"" << hash_b << "\",\n"
            << "  \"execution_receipt\": \"" << receipt << "\"\n"
            << "}\n";
  return 0;
}
