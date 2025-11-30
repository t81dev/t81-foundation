/**
 * @file T81Int.hpp
 * @brief Balanced ternary integer with packed trits.
 *
 * Features:
 *   • Packed 4 trits per byte (2 bits each: 0 = N, 1 = Z, 2 = P)
 *   • Correct balanced ternary arithmetic (+, -, *, /, %)
 *   • Trit proxy, operator[], tritwise shifts
 *   • Safe to_int64() with overflow checking
 *   • kMinValue / kMaxValue as inline static constants
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <limits>
#include <compare>
#include <ostream>
#include <utility>
#include <algorithm>
#include <type_traits>

namespace t81 {

enum class Trit : std::int8_t { N = -1, Z = 0, P = 1 };

constexpr inline int trit_to_int(Trit t) noexcept {
    return static_cast<int>(t);
}

constexpr inline Trit int_to_trit(int v) noexcept {
    if (v < 0) return Trit::N;
    if (v > 0) return Trit::P;
    return Trit::Z;
}

// Forward declarations so floating / fixed / prob types can befriend T81Int
template <std::size_t M, std::size_t E> class T81Float;
template <std::size_t I, std::size_t F> class T81Fixed;
template <std::size_t Trits> class T81Prob;

template <std::size_t N>
class T81Int {
public:
    using size_type = std::size_t;

    static_assert(N > 0 && N <= 2048, "T81Int<N>: N must be in 1..2048");

    static constexpr size_type kNumTrits     = N;
    static constexpr size_type kTritsPerByte = 4;
    static constexpr size_type kNumBytes     = (N + kTritsPerByte - 1) / kTritsPerByte;

private:
    std::array<std::uint8_t, kNumBytes> data_{};

    // --- Low-level encoding helpers (2 bits per trit) ---

    static constexpr std::uint8_t encode_trit(Trit t) noexcept {
        switch (t) {
            case Trit::N: return 0u;
            case Trit::Z: return 1u;
            case Trit::P: return 2u;
        }
        return 1u; // fallback to Z
    }

    static constexpr Trit decode_trit(std::uint8_t v) noexcept {
        switch (v & 0x3u) {
            case 0u: return Trit::N;
            case 1u: return Trit::Z;
            case 2u: return Trit::P;
            default: return Trit::Z;
        }
    }

    constexpr Trit get_trit(size_type idx) const noexcept {
        const size_type byte = idx / kTritsPerByte;
        const size_type off  = (idx % kTritsPerByte) * 2;
        const std::uint8_t raw = static_cast<std::uint8_t>((data_[byte] >> off) & 0x3u);
        return decode_trit(raw);
    }

    constexpr void set_trit(size_type idx, Trit t) noexcept {
        const size_type byte = idx / kTritsPerByte;
        const size_type off  = (idx % kTritsPerByte) * 2;
        const std::uint8_t mask = static_cast<std::uint8_t>(0x3u << off);
        const std::uint8_t enc  = static_cast<std::uint8_t>(encode_trit(t) << off);
        data_[byte] = static_cast<std::uint8_t>((data_[byte] & ~mask) | enc);
    }

    constexpr void clear() noexcept {
        // A byte of all Z trits is 0b01010101 = 0x55
        std::fill(data_.begin(), data_.end(), 0x55u);
    }

    // Friends: allow internal bit-level access from higher-level numeric types
    template <std::size_t M, std::size_t E>
    friend class T81Float;

    template <std::size_t I, std::size_t F>
    friend class T81Fixed;

    template <std::size_t Trits>
    friend class T81Prob;

public:
    // Exposed constants (defined out-of-class as inline variables)
    static const T81Int kMaxValue;
    static const T81Int kMinValue;

    // Helper to construct max value (all P trits)
    static constexpr T81Int make_max_value() noexcept {
        T81Int m;
        for (size_type i = 0; i < kNumTrits; ++i) {
            m.set_trit(i, Trit::P);
        }
        return m;
    }

    // Trit reference proxy
    class TritRef {
        T81Int* owner_;
        size_type idx_;
    public:
        constexpr TritRef(T81Int& owner, size_type idx) noexcept
            : owner_(&owner), idx_(idx) {}

        constexpr TritRef& operator=(Trit t) noexcept {
            owner_->set_trit(idx_, t);
            return *this;
        }

        constexpr TritRef& operator=(const TritRef& other) noexcept {
            return *this = static_cast<Trit>(other);
        }

        constexpr operator Trit() const noexcept {
            return owner_->get_trit(idx_);
        }
    };

    // --- Constructors / assignment ---

    constexpr T81Int() noexcept {
        clear();
    }
    constexpr T81Int(const T81Int&) noexcept = default;
    constexpr T81Int(T81Int&&) noexcept = default;
    constexpr T81Int& operator=(const T81Int&) noexcept = default;
    constexpr T81Int& operator=(T81Int&&) noexcept = default;

    // Construct from 64-bit integer, with range check (not constexpr)
    explicit T81Int(std::int64_t value) {
        assign_from_int64(value);
    }

    explicit T81Int(int value) : T81Int(static_cast<std::int64_t>(value)) {}

    static constexpr size_type num_trits() noexcept { return kNumTrits; }

    // --- Indexing ---

    constexpr Trit operator[](size_type idx) const noexcept {
        return get_trit(idx);
    }

    constexpr TritRef operator[](size_type idx) noexcept {
        return TritRef(*this, idx);
    }

    // --- Raw data access ---

    constexpr const std::array<std::uint8_t, kNumBytes>& raw_data() const noexcept {
        return data_;
    }

    // --- Core numeric helpers ---

    constexpr bool is_zero() const noexcept {
        for (size_type i = 0; i < kNumTrits; ++i) {
            if (get_trit(i) != Trit::Z) return false;
        }
        return true;
    }

    constexpr Trit sign_trit() const noexcept {
        // Most-significant non-zero trit decides sign
        for (size_type i = kNumTrits; i-- > 0; ) {
            Trit t = get_trit(i);
            if (t != Trit::Z) return t;
        }
        return Trit::Z;
    }

private:
    void assign_from_int64(std::int64_t v) {
        clear();
        if (v == 0) return;

        std::int64_t x = v;
        for (size_type i = 0; i < kNumTrits && x != 0; ++i) {
            std::int64_t r = x % 3;
            x /= 3;

            // Normalize remainder into {-1, 0, +1} with carry adjustment
            if (r == 2) {
                r = -1;
                ++x;
            } else if (r == -2) {
                r = 1;
                --x;
            }

            Trit t = int_to_trit(static_cast<int>(r));
            set_trit(i, t);
        }

        if (x != 0) {
            throw std::overflow_error("T81Int: value does not fit in N trits");
        }
    }

public:
    // Convert to int64_t with overflow checking
    std::int64_t to_int64() const {
        // We conservatively only trust the lowest 39 trits (3^39 < 2^63)
        static constexpr size_type kMaxSafeTrits = 39;
        const size_type limit = kNumTrits < kMaxSafeTrits ? kNumTrits : kMaxSafeTrits;

        // For widths > limit, require all higher trits to be zero to avoid overflow
        for (size_type i = limit; i < kNumTrits; ++i) {
            if (get_trit(i) != Trit::Z) {
                throw std::overflow_error("T81Int::to_int64: value too large for int64_t");
            }
        }

        std::int64_t value = 0;
        std::int64_t pow3  = 1;

        for (size_type i = 0; i < limit; ++i) {
            const int d = trit_to_int(get_trit(i));
            if (d != 0) {
                // Check addition overflow: value + d * pow3
                const std::int64_t term = d * pow3;
                if (term > 0 && value > (std::numeric_limits<std::int64_t>::max() - term)) {
                    throw std::overflow_error("T81Int::to_int64: overflow (positive)");
                }
                if (term < 0 && value < (std::numeric_limits<std::int64_t>::min() - term)) {
                    throw std::overflow_error("T81Int::to_int64: overflow (negative)");
                }
                value += term;
            }

            // Next power of 3
            if (i + 1 < limit) {
                if (pow3 > std::numeric_limits<std::int64_t>::max() / 3) {
                    throw std::overflow_error("T81Int::to_int64: overflow in power-of-3");
                }
                pow3 *= 3;
            }
        }

        return value;
    }

    // Convert to a standard binary integer type, leveraging to_int64()
    template <typename T>
    T to_binary() const {
        static_assert(std::is_integral<T>::value, "T must be an integral type.");
        const std::int64_t val = to_int64();
        if (val > std::numeric_limits<T>::max() || val < std::numeric_limits<T>::min()) {
            throw std::overflow_error("T81Int::to_binary: value does not fit in type T");
        }
        return static_cast<T>(val);
    }

    // --- Unary operators ---

    constexpr T81Int operator-() const noexcept {
        T81Int out;
        for (size_type i = 0; i < kNumTrits; ++i) {
            Trit t = get_trit(i);
            Trit r = Trit::Z;
            if (t == Trit::P) r = Trit::N;
            else if (t == Trit::N) r = Trit::P;
            out.set_trit(i, r);
        }
        return out;
    }

    constexpr T81Int& operator++() noexcept {
        *this = *this + T81Int(1);
        return *this;
    }

    constexpr T81Int operator++(int) noexcept {
        T81Int tmp(*this);
        ++(*this);
        return tmp;
    }

    constexpr T81Int& operator--() noexcept {
        *this = *this - T81Int(1);
        return *this;
    }

    constexpr T81Int operator--(int) noexcept {
        T81Int tmp(*this);
        --(*this);
        return tmp;
    }

private:
    // Shifts in trit-space (not power-of-two)
    constexpr void shift_left(size_type k) noexcept {
        if (k == 0) return;
        if (k >= kNumTrits) {
            clear();
            return;
        }
        for (size_type i = kNumTrits; i-- > k; ) {
            set_trit(i, get_trit(i - k));
        }
        for (size_type i = 0; i < k; ++i) {
            set_trit(i, Trit::Z);
        }
    }

    constexpr void shift_right(size_type k) noexcept {
        if (k == 0) return;
        if (k >= kNumTrits) {
            clear();
            return;
        }
        for (size_type i = 0; i + k < kNumTrits; ++i) {
            set_trit(i, get_trit(i + k));
        }
        for (size_type i = kNumTrits - k; i < kNumTrits; ++i) {
            set_trit(i, Trit::Z);
        }
    }

public:
    // Shift operators used by T81Float
    constexpr T81Int& operator<<=(size_type k) noexcept {
        shift_left(k);
        return *this;
    }

    constexpr T81Int& operator>>=(size_type k) noexcept {
        shift_right(k);
        return *this;
    }

    // Optional value-returning shifts
    friend constexpr T81Int operator<<(T81Int v, size_type k) noexcept {
        v <<= k;
        return v;
    }

    friend constexpr T81Int operator>>(T81Int v, size_type k) noexcept {
        v >>= k;
        return v;
    }

    // --- Comparisons ---

    constexpr std::strong_ordering operator<=>(const T81Int& other) const noexcept {
        for (size_type i = kNumTrits; i-- > 0; ) {
            const int self_trit = trit_to_int(get_trit(i));
            const int other_trit = trit_to_int(other.get_trit(i));
            if (self_trit < other_trit) return std::strong_ordering::less;
            if (self_trit > other_trit) return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    constexpr bool operator==(const T81Int&) const noexcept = default;


    // --- Arithmetic (balanced ternary) ---

    friend constexpr T81Int operator+(const T81Int& a, const T81Int& b) noexcept {
        T81Int result;
        int carry = 0;

        for (size_type i = 0; i < kNumTrits; ++i) {
            const int ai = trit_to_int(a.get_trit(i));
            const int bi = trit_to_int(b.get_trit(i));
            int sum = ai + bi + carry; // in [-3,3]

            int digit;
            if (sum > 1) {
                // 2 or 3
                digit = sum - 3;
                carry = 1;
            } else if (sum < -1) {
                // -2 or -3
                digit = sum + 3;
                carry = -1;
            } else {
                digit = sum;
                carry = 0;
            }

            result.set_trit(i, int_to_trit(digit));
        }

        // If carry != 0, we overflow N trits; we ignore extra carry (mod 3^N)
        return result;
    }

    friend constexpr T81Int operator-(const T81Int& a, const T81Int& b) noexcept {
        return a + (-b);
    }

    constexpr T81Int& operator+=(const T81Int& rhs) noexcept {
        *this = *this + rhs;
        return *this;
    }

    constexpr T81Int& operator-=(const T81Int& rhs) noexcept {
        *this = *this - rhs;
        return *this;
    }

    friend constexpr T81Int operator*(const T81Int& a, const T81Int& b) noexcept {
        T81Int result;

        for (size_type i = 0; i < kNumTrits; ++i) {
            Trit tb = b.get_trit(i);
            if (tb == Trit::Z) continue;

            T81Int tmp = a;
            tmp.shift_left(i);

            if (tb == Trit::P) {
                result = result + tmp;
            } else if (tb == Trit::N) {
                result = result - tmp;
            }
        }

        return result;
    }

    constexpr T81Int& operator*=(const T81Int& rhs) noexcept {
        *this = *this * rhs;
        return *this;
    }

    // Division and modulo implemented via int64_t with checks.
    // These are intentionally NOT constexpr.

    friend T81Int operator/(const T81Int& a, const T81Int& b) {
        if (b.is_zero()) {
            throw std::domain_error("T81Int: division by zero");
        }
        const std::int64_t av = a.to_int64();
        const std::int64_t bv = b.to_int64();
        const std::int64_t q  = av / bv;
        return T81Int(q);
    }

    friend T81Int operator%(const T81Int& a, const T81Int& b) {
        if (b.is_zero()) {
            throw std::domain_error("T81Int: modulo by zero");
        }
        const std::int64_t av = a.to_int64();
        const std::int64_t bv = b.to_int64();
        const std::int64_t r  = av % bv;
        return T81Int(r);
    }

    T81Int& operator/=(const T81Int& rhs) {
        *this = *this / rhs;
        return *this;
    }

    T81Int& operator%=(const T81Int& rhs) {
        *this = *this % rhs;
        return *this;
    }

    // --- String conversion (debug) ---

    std::string to_string() const {
        // Simple base-3 string via int64_t path; this is only for debugging.
        std::int64_t v = to_int64();
        if (v == 0) return "0";

        bool neg = v < 0;
        if (neg) v = -v;

        std::string out;
        while (v != 0) {
            int r = static_cast<int>(v % 3);
            v /= 3;
            char c = '?';
            if (r == 0) c = '0';
            else if (r == 1) c = '1';
            else if (r == 2) c = '2';
            out.push_back(c);
        }

        if (neg) out.push_back('-');
        std::reverse(out.begin(), out.end());
        return out;
    }

    friend std::ostream& operator<<(std::ostream& os, const T81Int& v) {
        return os << v.to_int64();
    }

    std::string to_trit_string() const {
        std::string s;
        s.reserve(kNumTrits);
        for (size_type i = kNumTrits; i-- > 0; ) {
            switch (get_trit(i)) {
                case Trit::P: s.push_back('+'); break;
                case Trit::Z: s.push_back('0'); break;
                case Trit::N: s.push_back('-'); break;
            }
        }
        return s;
    }
};

// --- Inline static data member definitions ---

template <std::size_t N>
inline const T81Int<N> T81Int<N>::kMaxValue = T81Int<N>::make_max_value();

template <std::size_t N>
inline const T81Int<N> T81Int<N>::kMinValue = -T81Int<N>::kMaxValue;

// --- Free helpers ---

template <std::size_t N>
inline std::pair<T81Int<N>, T81Int<N>>
div_mod(const T81Int<N>& a, const T81Int<N>& b) {
    if (b.is_zero()) {
        throw std::domain_error("T81Int: division by zero in div_mod");
    }
    const std::int64_t av = a.to_int64();
    const std::int64_t bv = b.to_int64();
    const std::int64_t q  = av / bv;
    const std::int64_t r  = av % bv;
    return { T81Int<N>(q), T81Int<N>(r) };
}

} // namespace t81

namespace std {
    template <size_t N>
    struct hash<t81::T81Int<N>> {
        size_t operator()(const t81::T81Int<N>& val) const {
            // A simple hash combining the raw bytes of the T81Int.
            const auto& data = val.raw_data();
            size_t seed = 0;
            std::hash<uint8_t> hasher;
            for (const auto& byte : data) {
                seed ^= hasher(byte) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
}
