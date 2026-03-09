/**
 * @file api.hpp
 * @brief Defines the public C++ API for interacting with the Axion kernel.
 *
 * This header provides the main entry point for the Axion kernel façade.
 * It integrates with the underlying policy engine and provides deterministic
 * decision APIs, capability descriptors, and observability telemetry.
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "t81/axion/context.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/config.hpp"

namespace t81::axion {

class Engine;  // Forward declaration

/**
 * @struct Version
 * @brief Represents the version number of a component.
 */
struct Version {
  uint16_t major{0};  ///< Major version number.
  uint16_t minor{0};  ///< Minor version number.
  uint16_t patch{0};  ///< Patch version number.

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
  uint64_t requests{0};
  uint64_t denies{0};
  uint64_t bytes_in{0};
  uint64_t bytes_out{0};
  double last_ms{0.0};
};

/**
 * @struct Capability
 * @brief Describes a kernel capability or privilege.
 */
struct Capability {
  uint32_t id;
  char name[32];
  bool enabled;
};

/**
 * @enum Status
 * @brief Represents the status code of an Axion API call.
 */
enum class Status : int32_t {
  Ok = 0,
  InvalidArgument = -1,
  BackendUnavailable = -2,
  Internal = -3,
  Denied = -4,
};

/**
 * @struct Buffer
 * @brief A generic container for binary data.
 */
struct Buffer {
  std::vector<uint8_t> data;
  explicit Buffer(std::vector<uint8_t> d = {}) : data(std::move(d)) {}
};

/**
 * @struct Signal
 * @brief A metadata envelope for a request to the Axion kernel.
 */
struct Signal {
  uint32_t kind{0};
  uint32_t flags{0};
  uint64_t nonce{0};
};

/**
 * @class AxionContext
 * @brief The main class for interacting with the Axion kernel.
 */
class AxionContext {
public:
  AxionContext();
  explicit AxionContext(std::unique_ptr<Engine> engine);
  ~AxionContext();

  static Version runtime_version() { return Version{1, 2, 0}; }
  static const char* runtime_name() { return "Axion-Façade"; }

  /**
   * @brief Evaluates a syscall against the active policy.
   * @param[in] ctx The syscall context to evaluate.
   * @return A Verdict indicating whether the operation is allowed.
   */
  Verdict evaluate(const SyscallContext& ctx);

  /**
   * @brief Submits a request, subject to policy evaluation.
   */
  Status submit(const Signal& sig, const Buffer& in, Buffer& out);

  /**
   * @brief Returns the list of available capabilities.
   */
  std::vector<Capability> capabilities() const;

  const Telemetry& telemetry() const { return tele_; }
  void reset_telemetry() { tele_ = Telemetry{}; }

private:
  std::unique_ptr<Engine> engine_;
  Telemetry tele_{};
};

/** @brief Compatibility alias for AxionContext. */
using Context = AxionContext;

[[noreturn]] inline void trap_overflow(const char* reason = "Axion overflow") {
  static const bool log_to_stderr = []() {
    if (const char* v = std::getenv("T81_AXION_TRAP_STDERR")) {
      return std::strcmp(v, "0") != 0;
    }
    return true;
  }();
  if (log_to_stderr) {
    std::fprintf(stderr, "Axion trap: %s\n", reason ? reason : "Axion overflow");
  }
  throw std::overflow_error(reason ? reason : "Axion overflow");
}

#ifdef _WIN32
#pragma warning(pop)
#endif

}  // namespace t81::axion
