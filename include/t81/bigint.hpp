#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace t81 {

// Base-243 big integer, LSB-first digit order, digits in [0..242]
class T243BigInt {
public:
  using digit_t = uint8_t;

  T243BigInt() = default;
  explicit T243BigInt(std::vector<digit_t> digits) : digits_(std::move(digits)) { normalize_(); }

  static T243BigInt from_ascii(const std::string& s) {
    std::vector<digit_t> d(s.size());
    for (size_t i=0;i<s.size();++i) d[i] = static_cast<digit_t>(static_cast<unsigned char>(s[i]) % 243);
    return T243BigInt(std::move(d));
  }

  std::string to_string() const {
    if (digits_.empty()) return "000";
    std::ostringstream oss;
    for (size_t i = digits_.size(); i-- > 0;) {
      oss.width(3); oss.fill('0');
      oss << static_cast<unsigned>(digits_[i]);
    }
    return oss.str();
  }

  static T243BigInt add(const T243BigInt& a, const T243BigInt& b) {
    const size_t max_len = std::max(a.digits_.size(), b.digits_.size());
    std::vector<digit_t> r(max_len + 1, 0);
    for (size_t i=0;i<max_len;++i) {
      if (i < a.digits_.size()) r[i] = static_cast<uint16_t>(r[i]) + a.digits_[i];
      if (i < b.digits_.size()) r[i] = static_cast<uint16_t>(r[i]) + b.digits_[i];
    }
    T243BigInt out(std::move(r));
    out.normalize_();
    return out;
  }

  static T243BigInt mul(const T243BigInt& a, const T243BigInt& b) {
    if (a.digits_.empty() || b.digits_.empty()) return T243BigInt();
    std::vector<uint16_t> tmp(a.digits_.size() + b.digits_.size(), 0);
    for (size_t i=0;i<a.digits_.size();++i)
      for (size_t j=0;j<b.digits_.size();++j)
        tmp[i+j] += static_cast<uint16_t>(a.digits_[i]) * b.digits_[j];

    std::vector<digit_t> r(tmp.size(), 0);
    uint32_t carry = 0;
    for (size_t k=0;k<tmp.size();++k) {
      uint32_t v = tmp[k] + carry;
      r[k] = static_cast<digit_t>(v % 243);
      carry = v / 243;
    }
    if (carry) r.push_back(static_cast<digit_t>(carry % 243));
    T243BigInt out(std::move(r));
    out.normalize_();
    return out;
  }

  const std::vector<digit_t>& digits() const { return digits_; }

private:
  void normalize_() {
    uint16_t carry = 0;
    for (size_t i=0;i<digits_.size();++i) {
      uint16_t v = static_cast<uint16_t>(digits_[i]) + carry;
      digits_[i] = static_cast<digit_t>(v % 243);
      carry = v / 243;
    }
    if (carry) digits_.push_back(static_cast<digit_t>(carry % 243));
    while (!digits_.empty() && digits_.back() == 0) digits_.pop_back();
  }

  std::vector<digit_t> digits_{}; // LSB-first
};

} // namespace t81

