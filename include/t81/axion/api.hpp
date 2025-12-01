/**
 * @file api.hpp
 * @brief Defines the public C++ API for interacting with the Axion kernel.
 *
 * This header provides a minimal, header-only façade for the Axion kernel.
 * It is designed as a stable public interface that allows for different
 * backend implementations to be swapped in without changing client code.
 *
 * @note The current implementation is a stub that simulates a device and
 *       returns deterministic placeholder results to allow examples and tests
 *       to link and run.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include "t81/config.hpp"

namespace t81::axion {

/**
 * @struct Version
 * @brief Represents the version number of a component.
 */
struct Version {
  uint16_t major{0}; ///< Major version number.
  uint16_t minor{0}; ///< Minor version number.
  uint16_t patch{0}; ///< Patch version number.

  /**
   * @brief Converts the version to a string representation.
   * @return A string in the format "major.minor.patch".
   */
  std::string str() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
  }
};

/**
 * @struct Telemetry
 * @brief Holds telemetry data for the Axion context.
 */
struct Telemetry {
  uint64_t requests{0};  ///< Total number of requests processed.
  uint64_t bytes_in{0};    ///< Total number of bytes received.
  uint64_t bytes_out{0};   ///< Total number of bytes sent.
  double   last_ms{0.0};   ///< Processing time of the last request in milliseconds.
};

/**
 * @enum Status
 * @brief Represents the status code of an Axion API call.
 */
enum class Status : int32_t {
  Ok = 0,                 ///< The operation completed successfully.
  InvalidArgument = -1,   ///< An invalid argument was provided.
  BackendUnavailable = -2,///< The backend service is not available.
  Internal = -3,          ///< An internal error occurred.
};

/**
 * @struct Buffer
 * @brief A generic container for binary data.
 */
struct Buffer {
  std::vector<uint8_t> data; ///< The raw byte data.
  explicit Buffer(std::vector<uint8_t> d = {}) : data(std::move(d)) {}
};

/**
 * @struct Signal
 * @brief A metadata envelope for a request to the Axion kernel.
 */
struct Signal {
  uint32_t kind{0};  ///< An identifier for the model or operation.
  uint32_t flags{0}; ///< Bit flags for specifying operation options.
  uint64_t nonce{0}; ///< A unique number for request/response correlation.
};

/**
 * @class Context
 * @brief The main class for interacting with the Axion kernel.
 *
 * Manages the connection and state for submitting requests to Axion and
 * receiving telemetry.
 */
class Context {
public:
  /**
   * @brief Default constructor.
   */
  Context() = default;

  /**
   * @brief Gets the version of the Axion runtime.
   * @return A Version struct.
   */
  static Version runtime_version() { return Version{1,1,0}; }

  /**
   * @brief Gets the name of the Axion runtime.
   * @return A C-string with the runtime name.
   */
  static const char* runtime_name() { return "Axion-Stub"; }

  /**
   * @brief Submits a single request to the Axion kernel.
   * @param[in] sig The signal metadata for the request.
   * @param[in] in The input buffer.
   * @param[out] out The output buffer, which will be populated by the call.
   * @return A Status code indicating the result of the operation.
   */
  Status submit(const Signal& sig, const Buffer& in, Buffer& out) {
    // This stub provides a deterministic placeholder "processing":
    // - Echoes the input buffer.
    // - Appends a trailer encoding the signal metadata.
    // - Fills telemetry with fixed values.
    constexpr uint8_t trailer_magic[4] = {'A','X','N','\x01'};
    out.data.clear();
    out.data.reserve(in.data.size() + sizeof(trailer_magic) + 16);

    // Echo input
    out.data.insert(out.data.end(), in.data.begin(), in.data.end());
    // Append trailer
    out.data.insert(out.data.end(), std::begin(trailer_magic), std::end(trailer_magic));
    append_u32(out.data, sig.kind);
    append_u32(out.data, sig.flags);
    append_u64(out.data, sig.nonce);

    // Update telemetry with fake timings.
    tele_.requests += 1;
    tele_.bytes_in  += static_cast<uint64_t>(in.data.size());
    tele_.bytes_out += static_cast<uint64_t>(out.data.size());
    tele_.last_ms = 0.123; // Deterministic stub value.

    (void)self_test_guard_(); // Allow compile-time stripping if unused.
    return Status::Ok;
  }

  /**
   * @brief Submits a batch of requests to the Axion kernel.
   * @param[in] sigs A vector of signal metadata.
   * @param[in] ins A vector of input buffers.
   * @param[out] outs A vector of output buffers to be populated.
   * @return Status::InvalidArgument if input vectors have different sizes,
   *         otherwise the status of the batch operation.
   */
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

  /**
   * @brief Gets the current telemetry data.
   * @return A const reference to the Telemetry struct.
   */
  const Telemetry& telemetry() const { return tele_; }

  /**
   * @brief Resets the telemetry data to zero.
   */
  void reset_telemetry() { tele_ = Telemetry{}; }

private:
  Telemetry tele_{};

  // Appends a 32-bit integer to a vector of bytes in little-endian order.
  static void append_u32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>( x        & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8)  & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
  }
  // Appends a 64-bit integer to a vector of bytes in little-endian order.
  static void append_u64(std::vector<uint8_t>& v, uint64_t x) {
    for (int i = 0; i < 8; ++i) v.push_back(static_cast<uint8_t>((x >> (8*i)) & 0xFF));
  }

  // A no-op hook to prevent compilers from stripping the class in certain LTO modes.
  T81_FORCE_INLINE bool self_test_guard_() const {
#if T81_ENABLE_ASSERTS
    // Touch telemetry fields to keep the class live in LTO+assert modes.
    return (tele_.requests == tele_.requests) && (tele_.bytes_in >= 0);
#else
    return true;
#endif
  }
};

} // namespace t81::axion
