#ifndef T81_FRACTION_H
#define T81_FRACTION_H

#include "t81_bigint.h"

/*
 * T81Fraction — canonical rational number:
 *  - numerator, denominator are T81BigInt
 *  - denominator > 0 unless value == 0
 *  - gcd(|num|, den) == 1
 */

typedef struct {
    T81BigInt num;
    T81BigInt den;
} T81Fraction;

#ifdef __cplusplus
extern "C" {
#endif

T81Status t81_fraction_init(T81Fraction *f);
T81Status t81_fraction_from_int64(T81Fraction *f, int64_t value);
T81Status t81_fraction_from_bigints(T81Fraction *f,
                                    const T81BigInt *num,
                                    const T81BigInt *den);

void      t81_fraction_free(T81Fraction *f);

T81Status t81_fraction_normalize(T81Fraction *f); // enforce gcd + sign rules

int       t81_fraction_cmp(const T81Fraction *a, const T81Fraction *b);

T81Status t81_fraction_add(T81Fraction *out,
                           const T81Fraction *a,
                           const T81Fraction *b);

T81Status t81_fraction_sub(T81Fraction *out,
                           const T81Fraction *a,
                           const T81Fraction *b);

T81Status t81_fraction_mul(T81Fraction *out,
                           const T81Fraction *a,
                           const T81Fraction *b);

T81Status t81_fraction_div(T81Fraction *out,
                           const T81Fraction *a,
                           const T81Fraction *b);

#ifdef __cplusplus
}
#endif

#endif // T81_FRACTION_H

