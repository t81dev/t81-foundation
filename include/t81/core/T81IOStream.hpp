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

#include "t81/core/T81Bytes.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Time.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Reflection.hpp"
#include <cstdio>
#include <string>
#include <span>

namespace t81 {

// ======================================================================
// T81IOStream – The only sanctioned channel to the outside world
// ======================================================================
class T81IOStream {
    enum class Kind { STDIN, STDOUT, STDERR, FILE_READ, FILE_WRITE };
    Kind kind_;
    FILE* handle_{nullptr};
    T81String path_;                     // for files
    mutable T81List<T81Time> timestamps_;
    mutable T81List<T81Entropy> entropy_costs_;

    // Private: only the universe may open a stream
    T81IOStream(Kind k, FILE* h = nullptr, T81String p = {})
        : kind_(k), handle_(h), path_(std::move(p)) {}

public:
    //===================================================================
    // The three sacred streams — exist from genesis
    //===================================================================
    static inline const T81IOStream cin  = T81IOStream(Kind::STDIN,  stdin);
    static inline const T81IOStream cout = T81IOStream(Kind::STDOUT, stdout);
    static inline const T81IOStream cerr = T81IOStream(Kind::STDERR, stderr);

    //===================================================================
    // Open a file — costs entropy and time
    //===================================================================
    [[nodiscard]] static T81IOStream open_read(T81String path, T81Entropy fuel) {
        if (!fuel.valid()) return {};
        auto* f = std::fopen(path.str().c_str(), "rb");
        if (!f) return {};
        consume_entropy(fuel);
        return T81IOStream(Kind::FILE_READ, f, std::move(path));
    }

    [[nodiscard]] static T81IOStream open_write(T81String path, T81Entropy fuel) {
        if (!fuel.valid()) return {};
        auto* f = std::fopen(path.str().c_str(), "wb");
        if (!f) return {};
        consume_entropy(fuel);
        return T81IOStream(Kind::FILE_WRITE, f, std::move(path));
    }

    //===================================================================
    // Destruction — closes handle and records final moment
    //===================================================================
    ~T81IOStream() {
        if (handle_ && handle_ != stdin && handle_ != stdout && handle_ != stderr) {
            std::fclose(handle_);
        }
        record_event(T81Time::now(T81Entropy::acquire(), symbols::IO_CLOSE));
    }

    //===================================================================
    // Core operations — every I/O costs entropy and time
    //===================================================================
    T81IOStream& write(const T81Bytes& data, T81Entropy fuel) {
        if (!valid() || !fuel.valid()) return *this;
        std::fwrite(data.data(), 1, data.size(), handle_);
        std::fflush(handle_);
        consume_entropy(fuel);
        record_event(T81Time::now(fuel, symbols::IO_WRITE));
        return *this;
    }

    [[nodiscard]] T81Bytes read(size_t max_bytes, T81Entropy fuel) {
        if (!valid() || !fuel.valid()) return {};
        T81Bytes buffer(max_bytes);
        size_t read = std::fread(buffer.data(), 1, max_bytes, handle_);
        buffer = buffer.subbytes(0, read);
        consume_entropy(fuel);
        record_event(T81Time::now(fuel, symbols::IO_READ));
        return buffer;
    }

    //===================================================================
    // Stream-like operators — the world speaks in ternary
    //===================================================================
    T81IOStream& operator<<(const T81String& s)   { return write(T81Bytes(s), T81Entropy::acquire()); }
    T81IOStream& operator<<(const T81Bytes& b)    { return write(b, T81Entropy::acquire()); }
    T81IOStream& operator<<(char c)               { return write(T81Bytes{static_cast<uint8_t>(c)}, T81Entropy::acquire()); }
    T81IOStream& operator<<(int64_t n)            { return *this << T81String(std::to_string(n)); }

    T81IOStream& operator>>(T81String& s) {
        auto bytes = read(1024, T81Entropy::acquire());
        s = bytes.to_utf8();
        return *this;
    }

    //===================================================================
    // Introspection — the stream knows it exists
    //===================================================================
    [[nodiscard]] constexpr bool valid() const noexcept {
        return handle_ != nullptr;
    }

    [[nodiscard]] constexpr const T81String& path() const noexcept { return path_; }

    [[nodiscard]] T81Reflection<T81IOStream> reflect() const {
        return T81Reflection<T81IOStream>(*this, symbols::IOSTREAM, path_.empty() ? symbols::SACRED_STREAM : symbols::FILE_STREAM);
    }

private:
    void record_event(T81Time t) const {
        timestamps_.push_back(t);
        cosmic_history.push_back(t);  // all I/O is forever
    }
};

// ======================================================================
// Global operators — the ternary world speaks
// ======================================================================
inline T81IOStream& operator<<(T81IOStream& os, const T81String& s)  { return os << s; }
inline T81IOStream& operator<<(T81IOStream& os, const char* s)       { return os << T81String(s); }

// ======================================================================
// The first words spoken to the outside world
// ======================================================================
namespace genesis {
    inline const bool HELLO_WORLD = []{
        cout << "The ternary universe has awakened.\n"_t81;
        cout << "Type count: 84\n"_t81;
        cout << T81Time::now(T81Entropy::acquire(), symbols::HELLO_WORLD).narrate() << "\n";
        return true;
    }();
}
