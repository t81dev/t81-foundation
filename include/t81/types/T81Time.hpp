/**
 * @file T81Time.hpp
 * @brief Lightweight time utility used by T81Result and related APIs.
 *
 * This version intentionally avoids coupling to entropy or cognitive
 * machinery. It provides:
 *   • T81Time::now() for capturing a timestamp.
 *   • duration / micros_since(other) helpers.
 *   • narrate() for human-readable description.
 *   • reflect() -> T81Reflection<T81Time> for introspection.
 */

#pragma once

#include "t81/types/T81Reflection.hpp"
#include "t81/types/T81String.hpp"
#include "t81/types/T81Symbol.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace t81 {

class T81Time {
public:
  using clock = std::chrono::steady_clock;
  using duration = std::chrono::microseconds;

private:
  clock::time_point tp_;
  T81Symbol event_id_;

  static inline std::optional<clock::time_point> deterministic_override_;
  static inline std::mutex override_mutex_;

  static clock::time_point current_time() {
    std::lock_guard<std::mutex> lock(override_mutex_);
    return deterministic_override_.value_or(clock::now());
  }

public:
  T81Time() : tp_(current_time()), event_id_(T81Symbol::intern("TIME_EVENT")) {}

  explicit T81Time(clock::time_point tp, T81Symbol id = T81Symbol::intern("TIME_EVENT"))
      : tp_(tp), event_id_(id) {}

  [[nodiscard]] static T81Time now(T81Symbol id = T81Symbol::intern("TIME_EVENT")) {
    return T81Time(current_time(), id);
  }

  static void set_deterministic_time(clock::time_point tp) {
    std::lock_guard<std::mutex> lock(override_mutex_);
    deterministic_override_ = tp;
  }

  [[nodiscard]] duration since(const T81Time& other) const noexcept {
    return std::chrono::duration_cast<duration>(tp_ - other.tp_);
  }

  [[nodiscard]] std::uint64_t micros_since(const T81Time& other) const noexcept {
    return static_cast<std::uint64_t>(since(other).count());
  }

  [[nodiscard]] const T81Symbol& event_id() const noexcept { return event_id_; }

  // Human-readable description for logging / error reporting.
  [[nodiscard]] T81String narrate() const {
    using namespace std::chrono;
    auto us = duration_cast<microseconds>(tp_.time_since_epoch()).count();
    return T81String("t+") + T81String(std::to_string(us)) + T81String("us@") +
           T81String(event_id_.to_string());
  }

  [[nodiscard]] T81Reflection<T81Time> reflect() const {
    return T81Reflection<T81Time>(*this, T81Symbol::intern("TIME"), event_id_);
  }
  [[nodiscard]] std::string serialize_canonical() const { return "Time()"; }
};

}  // namespace t81
