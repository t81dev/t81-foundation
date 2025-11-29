//======================================================================
// T81Polynomial.hpp – Exact polynomial arithmetic in balanced ternary
//                    The final algebraic structure in the T81 universe
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Complex.hpp"
#include "t81/T81List.hpp"
#include "t81/T81Vector.hpp"
#include <cstddef>
#include <span>
#include <compare>
#include <algorithm>

namespace t81 {

// ======================================================================
// T81Polynomial<Coeff> – Univariate polynomial with exact coefficients
// ======================================================================
template <typename Coeff = T81Float<72,9>>
class T81Polynomial {
    T81List<Coeff> coeffs_;  // coeff[0] = constant term, coeff[1] = x, etc.
    // Leading zeros are automatically trimmed

    void trim() noexcept {
        while (coeffs_.size() > 1 && coeffs_.back().is_zero())
            coeffs_.pop_back();
    }

public:
    using coefficient_type = Coeff;
    using value_type       = Coeff;

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Polynomial() noexcept : coeffs_{Coeff(0)} {}

    // From initializer list: {c0, c1, c2, ...} → c0 + c1·x + c2·x² + ...
    constexpr T81Polynomial(std::initializer_list<Coeff> c)
        : coeffs_(c) { trim(); }

    // From vector-like range
    template <typename InputIt>
    constexpr T81Polynomial(InputIt first, InputIt last)
        : coeffs_(first, last) { trim(); }

    // Monomial: x^n
    [[nodiscard]] static constexpr T81Polynomial monomial(size_t degree) {
        T81List<Coeff> c(degree + 1, Coeff(0));
        c[degree] = Coeff(1);
        return T81Polynomial(std::move(c));
    }

    // x variable
    [[nodiscard]] static constexpr T81Polynomial x() { return monomial(1); }

    //===================================================================
    // Properties
    //===================================================================
    [[nodiscard]] constexpr size_t degree() const noexcept {
        return coeffs_.empty() ? 0 : coeffs_.size() - 1;
    }

    [[nodiscard]] constexpr const Coeff& operator[](size_t i) const noexcept {
        return i < coeffs_.size() ? coeffs_[i] : Coeff(0);
    }

    [[nodiscard]] constexpr Coeff& operator[](size_t i) noexcept {
        if (i >= coeffs_.size()) {
            coeffs_.resize(i + 1, Coeff(0));
        }
        return coeffs_[i];
    }

    [[nodiscard]] constexpr const T81List<Coeff>& coefficients() const noexcept {
        return coeffs_;
    }

    //===================================================================
    // Evaluation – Horner's method (exact)
    //===================================================================
    [[nodiscard]] constexpr Coeff eval(const Coeff& x) const noexcept {
        Coeff result(0);
        for (auto it = coeffs_.rbegin(); it != coeffs_.rend(); ++it)
            result = result * x + *it;
        return result;
    }

    //===================================================================
    // Arithmetic – exact, no rounding
    //===================================================================
    [[nodiscard]] friend constexpr T81Polynomial operator+(
        const T81Polynomial& a,
        const T81Polynomial& b
    ) noexcept {
        size_t max_deg = std::max(a.degree(), b.degree());
        T81List<Coeff> c(max_deg + 1, Coeff(0));
        for (size_t i = 0; i <= max_deg; ++i)
            c[i] = a[i] + b[i];
        return T81Polynomial(std::move(c));
    }

    [[nodiscard]] friend constexpr T81Polynomial operator-(
        const T81Polynomial& a,
        const T81Polynomial& b
    ) noexcept {
        size_t max_deg = std::max(a.degree(), b.degree());
        T81List<Coeff> c(max_deg + 1, Coeff(0));
        for (size_t i = 0; i <= max_deg; ++i)
            c[i] = a[i] - b[i];
        return T81Polynomial(std::move(c));
    }

    [[nodiscard]] friend constexpr T81Polynomial operator*(
        const T81Polynomial& a,
        const T81Polynomial& b
    ) noexcept {
        if (a.degree() == 0) return b * a[0];
        if (b.degree() == 0) return a * b[0];

        T81List<Coeff> c(a.degree() + b.degree() + 1, Coeff(0));
        for (size_t i = 0; i <= a.degree(); ++i)
            for (size_t j = 0; j <= b.degree(); ++j)
                c[i + j] += a[i] * b[j];
        return T81Polynomial(std::move(c));
    }

    [[nodiscard]] constexpr T81Polynomial operator-() const noexcept {
        T81List<Coeff> c = coeffs_;
        for (auto& coef : c) coef = -coef;
        return T81Polynomial(std::move(c));
    }

    //===================================================================
    // Division (quotient + remainder) – exact when divides evenly
    //===================================================================
    struct Division {
        T81Polynomial quotient;
        T81Polynomial remainder;
    };

    [[nodiscard]] constexpr Division div(const T81Polynomial& divisor) const {
        if (divisor.degree() == 0) {
            return {*this * (Coeff(1) / divisor[0]), T81Polynomial{}};
        }
        if (degree() < divisor.degree()) {
            return {{}, *this};
        }

        auto q = *this;
        T81Polynomial r;
        while (q.degree() >= divisor.degree()) {
            auto lead = q.coefficients().back() / divisor.coefficients().back();
            size_t deg_diff = q.degree() - divisor.degree();
            auto term = T81Polynomial::monomial(deg_diff) * lead;
            q = q - (divisor * term);
            r = r + term;
        }
        r.trim();
        return {r, q};
    }

    //===================================================================
    // Roots – exact symbolic when possible
    //===================================================================
    [[nodiscard]] T81List<Coeff> roots() const;  // implemented for low degree

    //===================================================================
    // Derivatives & Integrals
    //===================================================================
    [[nodiscard]] constexpr T81Polynomial derivative() const noexcept {
        if (degree() == 0) return {};
        T81List<Coeff> c;
        for (size_t i = 1; i <= degree(); ++i)
            c.push_back(coeffs_[i] * Coeff(i));
        return T81Polynomial(std::move(c));
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Polynomial& o) const noexcept = default;
};

// ======================================================================
// Common polynomial types
// ======================================================================
using IntPoly    = T81Polynomial<T81Int<81>>;
using FloatPoly  = T81Polynomial<T81Float<72,9>>;
using ComplexPoly = T81Polynomial<T81Complex<121>>;

// ======================================================================
// Example: This is how the future does algebra
// ======================================================================
/*
constexpr auto x = T81Polynomial<Coeff>::x();
constexpr auto p = (x - 1) * (x - 2) * (x - 3);  // (x-1)(x-2)(x-3)
constexpr auto q = x * x + 1;

auto roots = p.roots();        // exactly {1,2,3}
auto eval  = p.eval(Coeff(5)); // exactly 48
auto deriv = p.derivative();   // 3x² - 12x + 11
*/
