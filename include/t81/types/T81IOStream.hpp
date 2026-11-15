/**
 * @file T81IOStream.hpp
 * @brief Defines the T81IOStream class for entropy-costing, reflective I/O.
 *
 * This file provides the T81IOStream class, which serves as the sole sanctioned
 * channel for input/output operations. All I/O actions, such as reading from
 * or writing to files and standard streams, are designed to be explicit events
 * that cost entropy and are timestamped. This allows for a complete, auditable
 * history of the system's interaction with the outside world.
 */
#pragma once

#include <cstdio>
#include <mutex>
#include <span>
#include <string>
#include <vector>
#include "t81/types/T81Bytes.hpp"
#include "t81/types/T81Entropy.hpp"
#include "t81/types/T81List.hpp"
#include "t81/types/T81Reflection.hpp"
#include "t81/types/T81String.hpp"
#include "t81/types/T81Time.hpp"

namespace t81 {

// ======================================================================
// T81IOStream – The only sanctioned channel to the outside world
// ======================================================================
class T81IOStream {
  enum class Kind { STDIN, STDOUT, STDERR, FILE_READ, FILE_WRITE };
  [[maybe_unused]] Kind kind_;
  FILE* handle_{nullptr};
  T81String path_;  // for files
  mutable T81List<T81Time> timestamps_;
  mutable T81List<T81Entropy> entropy_costs_;
  mutable std::mutex mutex_;

  // Private: only the universe may open a stream
  T81IOStream(Kind k, FILE* h = nullptr, T81String p = {})
      : kind_(k), handle_(h), path_(std::move(p)) {}

public:
  //===================================================================
  // The three sacred streams — exist from genesis
  //===================================================================
  static T81IOStream cin;
  static T81IOStream cout;
  static T81IOStream cerr;

  //===================================================================
  // Open a file — costs entropy and time
  //===================================================================
  [[nodiscard]] static T81IOStream open_read(T81String path, T81Entropy fuel) {
    if (fuel.is_consumed()) return T81IOStream(Kind::FILE_READ, nullptr);
    auto* f = std::fopen(path.str().c_str(), "rb");
    if (!f) return T81IOStream(Kind::FILE_READ, nullptr);
    (void)fuel.consume();
    return T81IOStream(Kind::FILE_READ, f, std::move(path));
  }

  [[nodiscard]] static T81IOStream open_write(T81String path, T81Entropy fuel) {
    if (fuel.is_consumed()) return T81IOStream(Kind::FILE_WRITE, nullptr);
    auto* f = std::fopen(path.str().c_str(), "wb");
    if (!f) return T81IOStream(Kind::FILE_WRITE, nullptr);
    (void)fuel.consume();
    return T81IOStream(Kind::FILE_WRITE, f, std::move(path));
  }

  //===================================================================
  // Destruction — closes handle and records final moment
  //===================================================================
  ~T81IOStream() {
    if (handle_ && handle_ != stdin && handle_ != stdout && handle_ != stderr) {
      std::fclose(handle_);
    }
    record_event(T81Time::now(symbols::IO_CLOSE));
    // Note: we don't acquire entropy in destructor to avoid crashing if pool exhausted.
  }

  //===================================================================
  // Core operations — every I/O costs entropy and time
  //===================================================================
  T81IOStream& write(const T81Bytes& data, T81Entropy fuel) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!valid() || fuel.is_consumed()) return *this;
    std::fwrite(data.data(), 1, data.size(), handle_);
    std::fflush(handle_);
    (void)fuel.consume();
    record_event_locked(T81Time::now(symbols::IO_WRITE));
    return *this;
  }

  [[nodiscard]] T81Bytes read(size_t max_bytes, T81Entropy fuel) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!valid() || fuel.is_consumed()) return {};
    T81Bytes buffer(max_bytes);
    size_t read = std::fread(buffer.data(), 1, max_bytes, handle_);
    buffer = buffer.slice(0, read);
    (void)fuel.consume();
    record_event_locked(T81Time::now(symbols::IO_READ));
    return buffer;
  }

  //===================================================================
  // Stream-like operators — the world speaks in ternary
  //===================================================================
  T81IOStream& operator<<(const T81String& s) {
    std::string stds = s.str();
    return write(T81Bytes::from_string(stds), acquire_entropy());
  }
  T81IOStream& operator<<(const T81Bytes& b) { return write(b, acquire_entropy()); }
  T81IOStream& operator<<(char c) {
    char buf[2] = {c, 0};
    return write(T81Bytes::from_string(std::string_view(buf, 1)), acquire_entropy());
  }
  T81IOStream& operator<<(int64_t n) {
    std::string s = std::to_string(n);
    return write(T81Bytes::from_string(s), acquire_entropy());
  }

  T81IOStream& operator>>(T81String& s) {
    auto bytes = read(1024, acquire_entropy());
    s.assign(bytes.to_utf8());
    return *this;
  }

  //===================================================================
  // Introspection — the stream knows it exists
  //===================================================================
  [[nodiscard]] constexpr bool valid() const noexcept { return handle_ != nullptr; }

  [[nodiscard]] constexpr const T81String& path() const noexcept { return path_; }

  /*
  [[nodiscard]] T81Reflection<T81IOStream> reflect() const {
      return T81Reflection<T81IOStream>(*this, symbols::IOSTREAM, path_.empty() ?
  symbols::SACRED_STREAM : symbols::FILE_STREAM);
  }
  */

private:
  void record_event(T81Time t) const {
    std::lock_guard<std::mutex> lock(mutex_);
    record_event_locked(t);
  }

  void record_event_locked(T81Time t) const {
    timestamps_.push_back(t);
    // cosmic_history.push_back(t);
  }
};

// Inline definitions of static members
inline T81IOStream T81IOStream::cin = T81IOStream(Kind::STDIN, stdin);
inline T81IOStream T81IOStream::cout = T81IOStream(Kind::STDOUT, stdout);
inline T81IOStream T81IOStream::cerr = T81IOStream(Kind::STDERR, stderr);

// ======================================================================
// Global operators — the ternary world speaks
// ======================================================================
// inline T81IOStream& operator<<(T81IOStream& os, const T81String& s)  { return os << s; }
inline T81IOStream& operator<<(T81IOStream& os, const char* s) { return os << T81String(s); }

// ======================================================================
// The first words spoken to the outside world
// ======================================================================
namespace genesis {
inline const bool HELLO_WORLD = [] { return true; }();
}  // namespace genesis

}  // namespace t81
