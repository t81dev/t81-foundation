#ifndef T81_CORE_CELL_HPP
#define T81_CORE_CELL_HPP

#include <cstdint>
#include <array>
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <algorithm>

namespace t81::core {

class Cell {
public:
    static constexpr int TRIT_CAPACITY = 12;

    Cell() : trits_{}, overflow_(false) { trits_.fill(0); }
    explicit Cell(int64_t value) { from_i64(value); }

    void from_i64(int64_t value) {
        trits_.fill(0);
        overflow_ = false;
        if (value > max_value().to_i64() || value < -max_value().to_i64()) {
            overflow_ = true;
            return;
        }

        int64_t current = value;
        for (size_t i = 0; i < TRIT_CAPACITY; ++i) {
            int64_t remainder = current % 3;
            current /= 3;
            if (remainder == 2) {
                trits_[i] = -1;
                current++;
            } else if (remainder == -2) {
                trits_[i] = 1;
                current--;
            } else {
                trits_[i] = static_cast<int8_t>(remainder);
            }
        }
    }

    int64_t to_i64() const {
        int64_t value = 0;
        int64_t power_of_3 = 1;
        for (int i = 0; i < TRIT_CAPACITY; ++i) {
            value += trits_[i] * power_of_3;
            power_of_3 *= 3;
        }
        return value;
    }

    static Cell max_value() {
        Cell c;
        c.trits_.fill(1);
        return c;
    }

    Cell operator-() const {
        Cell result;
        for (int i = 0; i < TRIT_CAPACITY; ++i) {
            result.trits_[i] = -this->trits_[i];
        }
        return result;
    }

    Cell operator+(const Cell& other) const {
        Cell result;
        int8_t carry = 0;
        for (int i = 0; i < TRIT_CAPACITY; ++i) {
            int8_t sum = this->trits_[i] + other.trits_[i] + carry;
            if (sum > 1) {
                result.trits_[i] = sum - 3;
                carry = 1;
            } else if (sum < -1) {
                result.trits_[i] = sum + 3;
                carry = -1;
            } else {
                result.trits_[i] = sum;
                carry = 0;
            }
        }
        if (carry != 0) result.overflow_ = true;
        return result;
    }

    Cell operator-(const Cell& other) const { return *this + (-other); }

    Cell operator*(const Cell& other) const {
        Cell result;
        for (int i = 0; i < TRIT_CAPACITY; ++i) {
            Cell partial_product;
            if (other.trits_[i] == 1) {
                partial_product = *this;
            } else if (other.trits_[i] == -1) {
                partial_product = -(*this);
            } else {
                continue;
            }

            // Shift partial product
            std::rotate(partial_product.trits_.begin(), partial_product.trits_.begin() + TRIT_CAPACITY - i, partial_product.trits_.end());
            for(int j=0; j < i; ++j) partial_product.trits_[j] = 0;

            result = result + partial_product;
        }
        return result;
    }

    Cell operator/(const Cell& other) const {
        if (other.to_i64() == 0) throw std::runtime_error("Division by zero");
        int64_t a = this->to_i64();
        int64_t b = other.to_i64();
        return Cell(a / b); // Placeholder, real ternary division is complex
    }

    bool has_overflowed() const { return overflow_; }
    bool operator!=(const Cell& other) const { return to_i64() != other.to_i64(); }

private:
    std::array<int8_t, TRIT_CAPACITY> trits_;
    bool overflow_;
};

} // namespace t81::core

#endif // T81_CORE_CELL_HPP
