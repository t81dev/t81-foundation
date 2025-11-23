#include <t81/codec/base243.hpp>
#include <t81/bigint/divmod.hpp>
#include <sstream>
#include <vector>

namespace t81::codec::base243 {

std::string encode_bigint(const T243BigInt& value) {
    if (value.is_zero()) {
        return "0";
    }

    bool neg = value.is_negative();
    T243BigInt v = neg ? value.abs() : value;

    std::vector<int> digits;
    T243BigInt base(243);

    while (!v.is_zero()) {
        auto dm = divmod(v, base);
        // dm.r is guaranteed 0 <= r < 243 (since base = 243)
        int d = static_cast<int>(dm.r.to_int64()); // assuming you have a safe downcast for small values
        digits.push_back(d);
        v = dm.q;
    }

    // Build string, most significant digit first
    std::ostringstream oss;
    if (neg) {
        oss << '-';
    }

    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (it != digits.rbegin()) {
            oss << '.';
        }
        oss << *it;
    }

    return oss.str();
}

bool decode_bigint(std::string_view s, T243BigInt& out) {
    if (s.empty()) {
        return false;
    }

    bool neg = false;
    std::size_t pos = 0;
    if (s[pos] == '+' || s[pos] == '-') {
        neg = (s[pos] == '-');
        ++pos;
        if (pos >= s.size()) return false;
    }

    // Parse digits separated by '.'
    std::vector<int> digits;
    int current = 0;
    bool have_digit = false;
    for (; pos <= s.size(); ++pos) {
        if (pos == s.size() || s[pos] == '.') {
            if (!have_digit) return false;
            if (current < 0 || current >= 243) return false;
            digits.push_back(current);
            current = 0;
            have_digit = false;
        } else if (s[pos] >= '0' && s[pos] <= '9') {
            have_digit = true;
            current = current * 10 + (s[pos] - '0');
        } else {
            return false; // invalid character
        }
    }

    if (digits.empty()) return false;

    T243BigInt base(243);
    T243BigInt v(0);
    for (int d : digits) {
        v *= base;
        v += T243BigInt(d);
    }

    if (neg) {
        v = -v;
    }

    out = std::move(v);
    return true;
}

} // namespace t81::codec::base243
