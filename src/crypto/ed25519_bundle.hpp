#pragma once

// src/crypto/ed25519_bundle.hpp
//
// Zero-dependency RFC 8032 Ed25519 signing/verification and RFC-00D4
// Accountable Decision Bundle detached-signature & multi-agent DAG verifier.
// Staged under src/crypto/ to preserve docs/governance/PUBLIC_API_LOCK.json (v2.2.0).

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace t81::crypto {

struct Ed25519KeyPair {
  std::array<uint8_t, 32> seed{};
  std::array<uint8_t, 32> public_key{};
};

// Deterministic key derivation from a 32-byte RFC 8032 seed.
[[nodiscard]] Ed25519KeyPair ed25519_keypair_from_seed(std::span<const uint8_t, 32> seed) noexcept;

// Generate a fresh keypair using std::random_device.
[[nodiscard]] Ed25519KeyPair ed25519_generate_keypair();

// RFC 8032 PureEdDSA Ed25519 sign & verify (constant-time scalar mul).
[[nodiscard]] std::array<uint8_t, 64> ed25519_sign(std::span<const uint8_t, 32> seed,
                                                   std::span<const uint8_t, 32> public_key,
                                                   std::span<const uint8_t> message) noexcept;

[[nodiscard]] bool ed25519_verify(std::span<const uint8_t, 32> public_key,
                                  std::span<const uint8_t> message,
                                  std::span<const uint8_t, 64> signature) noexcept;

// OpenSSL-compatible PKCS#8 / SPKI PEM encoding & decoding for bit-exact interop
// with examples/research/agent-receipt-sidecar/t81_receipt_sidecar.py.
[[nodiscard]] std::string export_private_key_pem(std::span<const uint8_t, 32> seed);
[[nodiscard]] std::string export_public_key_pem(std::span<const uint8_t, 32> public_key);
[[nodiscard]] std::optional<std::array<uint8_t, 32>> import_private_key_pem(std::string_view pem);
[[nodiscard]] std::optional<std::array<uint8_t, 32>> import_public_key_pem(std::string_view pem);

// Strict Base64 helpers.
[[nodiscard]] std::string base64_encode(std::span<const uint8_t> data);
[[nodiscard]] std::optional<std::vector<uint8_t>> base64_decode(std::string_view b64);

// Build RFC-00D4 canonical signing bytes matching t81_receipt_sidecar.py:
// {"authority":<authority_json>,"evidence":<evidence_json>,"meaning":<meaning_json>}
[[nodiscard]] std::string rfc00d4_canonical_signing_payload(std::string_view meaning_json,
                                                            std::string_view evidence_json,
                                                            std::string_view authority_json);

[[nodiscard]] std::string sign_rfc00d4_payload_b64(std::span<const uint8_t, 32> seed,
                                                   std::span<const uint8_t, 32> public_key,
                                                   std::string_view canonical_payload);

[[nodiscard]] bool verify_rfc00d4_payload_b64(std::span<const uint8_t, 32> public_key,
                                              std::string_view canonical_payload,
                                              std::string_view signature_b64);

struct AccountableBundleNode {
  std::string bundle_id;
  std::vector<std::string> parent_bundle_ids;
  std::string canonical_signing_payload;
  std::string signature_b64;
  std::array<uint8_t, 32> public_key{};
};

struct DagVerificationResult {
  bool ok{false};
  std::string code;  // "OK", "ERR_DAG_EMPTY", "ERR_DAG_DUPLICATE_ID",
                     // "ERR_DAG_MISSING_PARENT", "ERR_DAG_CYCLE", "ERR_RECEIPT_VERIFY_FAILED"
  std::string error;
  std::size_t verified_nodes{0};
};

// Fail-closed multi-agent RFC-00D4 receipt DAG verifier:
// checks unique IDs, parent existence, acyclicity, and Ed25519 signature on every node.
[[nodiscard]] DagVerificationResult verify_dag(const std::vector<AccountableBundleNode>& nodes);

}  // namespace t81::crypto
