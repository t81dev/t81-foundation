#pragma once

// Private, single-thread research instrumentation. Never part of VM state or evidence.
#ifdef T81_INVESTIGATION08_TIMING
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
#endif

namespace t81::research::i08 {
#ifdef T81_INVESTIGATION08_TIMING
using Clock = std::chrono::steady_clock;
static_assert(Clock::is_steady);
inline constexpr auto none = std::numeric_limits<std::size_t>::max();
struct Event {
  const char* name;
  std::size_t parent;
  long long start;
  long long end;
};
struct Recorder {
  bool active = false;
  Clock::time_point origin;
  std::size_t parent = none;
  std::vector<Event> events;
  long long now() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - origin).count();
  }
};
inline thread_local Recorder recorder;
class Scope {
  std::size_t index_ = none;

public:
  explicit Scope(const char* name) noexcept {
    if (!recorder.active) return;
    try {
      const auto start = recorder.now();
      index_ = recorder.events.size();
      recorder.events.push_back({name, recorder.parent, start, start});
      recorder.parent = index_;
    } catch (...) {
      recorder.active = false;
      index_ = none;
    }
  }
  void stop() noexcept {
    if (index_ == none || !recorder.active) return;
    auto& event = recorder.events[index_];
    event.end = recorder.now();
    recorder.parent = event.parent;
    index_ = none;
  }
  ~Scope() { stop(); }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
};
class Session {
  bool owner_ = false;
  std::string path_;

public:
  Session() noexcept {
    if (recorder.active) return;
    const char* path = std::getenv("T81_I08_TIMING_OUT");
    if (!path || !*path) return;
    try {
      path_ = path;
      recorder.origin = Clock::now();
      recorder.events.clear();
      recorder.events.reserve(2048);
      recorder.events.push_back({"application", none, 0, 0});
      recorder.parent = 0;
      recorder.active = true;
      owner_ = true;
    } catch (...) {
      recorder.active = false;
    }
  }
  ~Session() noexcept {
    if (!owner_ || !recorder.active) return;
    recorder.events[0].end = recorder.now();
    recorder.active = false;
    try {
      // Sequential unique sidecars only. Recording failures never change task results.
      if (std::filesystem::exists(path_)) return;
      std::ofstream out(path_);
      out << "{\"schema\":\"t81.research.investigation08.phases.v1\","
             "\"clock\":\"steady_clock\",\"unit\":\"ns\","
             "\"startup_ns\":null,\"shutdown_ns\":null,\"events\":[";
      for (std::size_t i = 0; i < recorder.events.size(); ++i) {
        const auto& event = recorder.events[i];
        auto inclusive = event.end - event.start;
        auto exclusive = inclusive;
        for (std::size_t j = i + 1; j < recorder.events.size(); ++j) {
          const auto& child = recorder.events[j];
          if (child.parent == i) exclusive -= child.end - child.start;
        }
        if (i) out << ',';
        out << "{\"id\":" << i << ",\"parent\":";
        if (event.parent == none)
          out << "null";
        else
          out << event.parent;
        out << ",\"name\":\"" << event.name << "\",\"start_ns\":" << event.start
            << ",\"end_ns\":" << event.end << ",\"inclusive_ns\":" << inclusive
            << ",\"exclusive_ns\":" << exclusive << '}';
      }
      out << "]}\n";
    } catch (...) { /* A missing/invalid sidecar is rejected by the research harness. */
    }
  }
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
};
#else
class Scope {
public:
  explicit Scope(const char*) noexcept {}
  void stop() noexcept {}
};
class Session {};
#endif
}  // namespace t81::research::i08

#define T81_I08_JOIN_(a, b) a##b
#define T81_I08_JOIN(a, b) T81_I08_JOIN_(a, b)
#define T81_I08_SCOPE(name) \
  [[maybe_unused]] t81::research::i08::Scope T81_I08_JOIN(i08_scope_, __LINE__)(name)
