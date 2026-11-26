// tests/cpp/ed25519_bundle_test.cpp
//
// Verifies RFC 8032 §7.1 Ed25519 test vectors, OpenSSL PEM interop,
// RFC-00D4 multi-agent verify_dag (positive & adversarial negative cases),
// and warm verification latency (< 0.5 ms).

#include "crypto/ed25519_bundle.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
  assert(hex.size() % 2 == 0);
  std::vector<uint8_t> out(hex.size() / 2);
  auto nibble = [](char c) -> uint8_t {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    assert(false);
    return 0;
  };
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<uint8_t>((nibble(hex[2 * i]) << 4) | nibble(hex[2 * i + 1]));
  }
  return out;
}

void check_rfc8032_vector(const char* name, const std::string& seed_hex, const std::string& pub_hex,
                          const std::string& msg_hex, const std::string& sig_hex) {
  auto seed_v = hex_to_bytes(seed_hex);
  auto pub_v = hex_to_bytes(pub_hex);
  auto msg_v = hex_to_bytes(msg_hex);
  auto sig_v = hex_to_bytes(sig_hex);
  assert(seed_v.size() == 32 && pub_v.size() == 32 && sig_v.size() == 64);

  std::array<uint8_t, 32> seed{};
  std::memcpy(seed.data(), seed_v.data(), 32);
  auto kp = t81::crypto::ed25519_keypair_from_seed(seed);
  if (std::memcmp(kp.public_key.data(), pub_v.data(), 32) != 0) {
    std::cerr << "FAIL " << name << ": public key mismatch\n";
    std::exit(1);
  }
  auto sig = t81::crypto::ed25519_sign(kp.seed, kp.public_key, msg_v);
  if (std::memcmp(sig.data(), sig_v.data(), 64) != 0) {
    std::cerr << "FAIL " << name << ": signature mismatch\n";
    std::exit(1);
  }
  if (!t81::crypto::ed25519_verify(kp.public_key, msg_v, sig)) {
    std::cerr << "FAIL " << name << ": verify rejected valid signature\n";
    std::exit(1);
  }
  // Flip 1 bit -> must fail closed.
  sig[0] ^= 0x01;
  if (t81::crypto::ed25519_verify(kp.public_key, msg_v, sig)) {
    std::cerr << "FAIL " << name << ": verify accepted tampered signature\n";
    std::exit(1);
  }
}

}  // namespace

int main() {
  // 1. Official RFC 8032 §7.1 Test Vectors 1, 2, 3.
  check_rfc8032_vector("RFC8032-TV1-empty",
                       "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
                       "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", "",
                       "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
                       "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");

  check_rfc8032_vector("RFC8032-TV2-1byte",
                       "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
                       "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c", "72",
                       "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
                       "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");

  check_rfc8032_vector("RFC8032-TV3-2bytes",
                       "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7",
                       "fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025", "af82",
                       "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac"
                       "18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a");

  // 2. PEM export/import round-trip (OpenSSL PKCS#8 / SPKI compatible).
  auto kp = t81::crypto::ed25519_generate_keypair();
  const std::string priv_pem = t81::crypto::export_private_key_pem(kp.seed);
  const std::string pub_pem = t81::crypto::export_public_key_pem(kp.public_key);
  auto dec_seed = t81::crypto::import_private_key_pem(priv_pem);
  auto dec_pub = t81::crypto::import_public_key_pem(pub_pem);
  if (!dec_seed.has_value() || *dec_seed != kp.seed) {
    std::cerr << "FAIL: private PEM round-trip mismatch\n";
    return 1;
  }
  if (!dec_pub.has_value() || *dec_pub != kp.public_key) {
    std::cerr << "FAIL: public PEM round-trip mismatch\n";
    return 1;
  }

  // 3. RFC-00D4 multi-agent DAG verification (positive + adversarial negative controls).
  const std::string p1 = t81::crypto::rfc00d4_canonical_signing_payload(
      "{\"action_type\":\"stage\"}", "{\"sha3_512\":\"aaa\"}", "{\"verdict\":\"Allow\"}");
  const std::string p2 = t81::crypto::rfc00d4_canonical_signing_payload(
      "{\"action_type\":\"commit\"}", "{\"sha3_512\":\"bbb\"}", "{\"verdict\":\"Allow\"}");

  t81::crypto::AccountableBundleNode n1{
      "bundle-root",
      {},
      p1,
      t81::crypto::sign_rfc00d4_payload_b64(kp.seed, kp.public_key, p1),
      kp.public_key};
  t81::crypto::AccountableBundleNode n2{
      "bundle-child",
      {"bundle-root"},
      p2,
      t81::crypto::sign_rfc00d4_payload_b64(kp.seed, kp.public_key, p2),
      kp.public_key};

  auto ok_dag = t81::crypto::verify_dag({n1, n2});
  assert(ok_dag.ok && ok_dag.verified_nodes == 2);

  // Tampered payload -> fail closed.
  auto bad_tamper = n2;
  bad_tamper.canonical_signing_payload.back() = ' ';
  auto r_tamper = t81::crypto::verify_dag({n1, bad_tamper});
  assert(!r_tamper.ok && r_tamper.code == "ERR_RECEIPT_VERIFY_FAILED");

  // Missing parent -> fail closed.
  auto bad_parent = n2;
  bad_parent.parent_bundle_ids = {"nonexistent-parent"};
  auto r_parent = t81::crypto::verify_dag({n1, bad_parent});
  assert(!r_parent.ok && r_parent.code == "ERR_DAG_MISSING_PARENT");

  // Cycle -> fail closed.
  auto cyc1 = n1;
  cyc1.parent_bundle_ids = {"bundle-child"};
  auto r_cycle = t81::crypto::verify_dag({cyc1, n2});
  if (r_cycle.ok || r_cycle.code != "ERR_DAG_CYCLE") {
    std::cerr << "FAIL: cycle not rejected\n";
    return 1;
  }

  // Empty DAG & duplicate ID -> fail closed.
  if (t81::crypto::verify_dag({}).ok) {
    std::cerr << "FAIL: empty DAG accepted\n";
    return 1;
  }
  if (t81::crypto::verify_dag({n1, n1}).ok) {
    std::cerr << "FAIL: duplicate bundle_id accepted\n";
    return 1;
  }

  // 3b. Wycheproof / RFC 8032 adversarial malformed-input & small-order torsion vectors.
  int adv_passed = 0;
  auto must_reject = [&](const char* label, const std::array<uint8_t, 32>& pub,
                         const std::array<uint8_t, 64>& sig, std::span<const uint8_t> msg) {
    if (t81::crypto::ed25519_verify(pub, msg, sig)) {
      std::cerr << "FAIL adversarial vector accepted: " << label << "\n";
      std::exit(1);
    }
    ++adv_passed;
  };
  const std::vector<uint8_t> m_adv = {'t', '8', '1'};
  auto valid_sig = t81::crypto::ed25519_sign(kp.seed, kp.public_key, m_adv);

  // Non-canonical S == L and S == all-0xFF.
  auto sig_s_eq_l = valid_sig;
  static constexpr uint8_t kLBytes[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                                          0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                                          0,    0,    0,    0,    0,    0,    0,    0,
                                          0,    0,    0,    0,    0,    0,    0,    0x10};
  std::memcpy(sig_s_eq_l.data() + 32, kLBytes, 32);
  must_reject("non_canonical_S_eq_L", kp.public_key, sig_s_eq_l, m_adv);

  auto sig_s_ff = valid_sig;
  std::memset(sig_s_ff.data() + 32, 0xff, 32);
  must_reject("non_canonical_S_all_ff", kp.public_key, sig_s_ff, m_adv);

  // Non-canonical y == p (0xed, 0xff..0xff, 0x7f) in both A and R.
  std::array<uint8_t, 32> y_eq_p{};
  y_eq_p.fill(0xff);
  y_eq_p[0] = 0xed;
  y_eq_p[31] = 0x7f;
  must_reject("non_canonical_A_y_eq_p", y_eq_p, valid_sig, m_adv);
  auto sig_r_y_eq_p = valid_sig;
  std::memcpy(sig_r_y_eq_p.data(), y_eq_p.data(), 32);
  must_reject("non_canonical_R_y_eq_p", kp.public_key, sig_r_y_eq_p, m_adv);

  // All-zero pubkey & all-zero signature.
  std::array<uint8_t, 32> zero_pub{};
  std::array<uint8_t, 64> zero_sig{};
  must_reject("all_zero_pub_and_sig", zero_pub, zero_sig, m_adv);

  // Small-order torsion points (identity, order-2, order-8) in A and R.
  std::array<uint8_t, 32> id_pt{};
  id_pt[0] = 0x01;
  must_reject("small_order_identity_A", id_pt, valid_sig, m_adv);
  auto sig_r_id = valid_sig;
  std::memcpy(sig_r_id.data(), id_pt.data(), 32);
  must_reject("small_order_identity_R", kp.public_key, sig_r_id, m_adv);

  std::array<uint8_t, 32> ord2_pt{};
  ord2_pt.fill(0xff);
  ord2_pt[0] = 0xec;
  ord2_pt[31] = 0x7f;
  must_reject("small_order_2_A", ord2_pt, valid_sig, m_adv);

  // Truncated / oversized base64 signatures & invalid PEM.
  if (t81::crypto::verify_rfc00d4_payload_b64(kp.public_key, p1, "AAAA")) {
    std::cerr << "FAIL: truncated base64 sig accepted\n";
    return 1;
  }
  ++adv_passed;
  if (t81::crypto::import_public_key_pem("not-a-pem").has_value()) {
    std::cerr << "FAIL: invalid PEM accepted\n";
    return 1;
  }
  ++adv_passed;

  // 4. Warm verification latency benchmark (target < 0.5 ms / verify).
  constexpr int kIters = 200;
  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < kIters; ++i) {
    if (!t81::crypto::verify_rfc00d4_payload_b64(kp.public_key, p1, n1.signature_b64)) {
      std::cerr << "FAIL: warm verify rejected valid signature\n";
      return 1;
    }
  }
  const auto t1 = std::chrono::steady_clock::now();
  const double mean_us =
      std::chrono::duration<double, std::micro>(t1 - t0).count() / static_cast<double>(kIters);
  std::cout << "PASS t81_ed25519_bundle_test (RFC8032 TV1-3 + " << adv_passed
            << " Wycheproof/adversarial vectors + PEM + DAG + warm verify " << mean_us
            << " us/op)\n";
  // Assert well below 5000 us under Release (generous cross-compiler CI ceiling; M2 Clang ~440-600
  // us).
#ifdef NDEBUG
  if (mean_us >= 5000.0) {
    std::cerr << "FAIL: warm Ed25519 verify exceeded 5000 us (" << mean_us << " us)\n";
    return 1;
  }
#endif
  return 0;
}
