#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include "t81/config.hpp"

namespace t81::axion {

// Minimal, header-only façade for Axion. This is a stub that simulates a device
// and returns deterministic placeholder results to keep examples/tests linking.
// Swap in the real backend later without changing the API.

struct Version {
  uint16_t major{0}, minor{0}, patch{0};
  std::string str() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
  }
};

struct Telemetry {
  uint64_t requests{0};
  uint64_t bytes_in{0};
  uint64_t bytes_out{0};
  double   last_ms{0.0};
};

enum class Status : int32_t {
  Ok = 0,
  InvalidArgument = -1,
  BackendUnavailable = -2,
  Internal = -3,
};

struct Buffer {
  std::vector<uint8_t> data;
  explicit Buffer(std::vector<uint8_t> d = {}) : data(std::move(d)) {}
};

struct Signal {
  // A tiny metadata envelope you can expand later.
  uint32_t kind{0};      // e.g., model/op identifier
  uint32_t flags{0};     // bit flags
  uint64_t nonce{0};     // for correlation
};

class Context {
public:
  Context() = default;

  static Version runtime_version() { return Version{1,1,0}; }
  static const char* runtime_name() { return "Axion-Stub"; }

  // Simulate a single-shot request/response exchange.
  Status submit(const Signal& sig, const Buffer& in, Buffer& out) {
    // Deterministic placeholder “processing”:
    // - echo header bytes
    // - append a small trailer encoding kind/flags/nonce
    // - fill telemetry
    constexpr uint8_t trailer_magic[4] = {'A','X','N','\x01'};
    out.data.clear();
    out.data.reserve(in.data.size() + sizeof(trailer_magic) + 16);

    // Echo input
    out.data.insert(out.data.end(), in.data.begin(), in.data.end());
    // Trailer
    out.data.insert(out.data.end(), std::begin(trailer_magic), std::end(trailer_magic));
    append_u32(out.data, sig.kind);
    append_u32(out.data, sig.flags);
    append_u64(out.data, sig.nonce);

    // Update telemetry (fake timings)
    tele_.requests += 1;
    tele_.bytes_in  += static_cast<uint64_t>(in.data.size());
    tele_.bytes_out += static_cast<uint64_t>(out.data.size());
    tele_.last_ms = 0.123; // deterministic stub

    (void)self_test_guard_(); // allow compile-time stripping if unused
    return Status::Ok;
  }

  // Batch form (submits pairwise; lengths must match).
  Status submit_batch(const std::vector<Signal>& sigs,
                      const std::vector<Buffer>& ins,
                      std::vector<Buffer>& outs) {
    if (sigs.size() != ins.size()) return Status::InvalidArgument;
    outs.clear();
    outs.resize(sigs.size());
    for (size_t i = 0; i < sigs.size(); ++i) {
      auto st = submit(sigs[i], ins[i], outs[i]);
      if (st != Status::Ok) return st;
    }
    return Status::Ok;
  }

  const Telemetry& telemetry() const { return tele_; }
  void reset_telemetry() { tele_ = Telemetry{}; }

private:
  Telemetry tele_{};

  static void append_u32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>( x        & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8)  & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
  }
  static void append_u64(std::vector<uint8_t>& v, uint64_t x) {
    for (int i = 0; i < 8; ++i) v.push_back(static_cast<uint8_t>((x >> (8*i)) & 0xFF));
  }

  T81_FORCE_INLINE bool self_test_guard_() const {
#if T81_ENABLE_ASSERTS
    // No-op hook to keep some compilers from stripping everything in certain LTO modes.
    return tele_.requests == tele_.requests;
#else
    return true;
#endif
  }
};

} // namespace t81::axion
