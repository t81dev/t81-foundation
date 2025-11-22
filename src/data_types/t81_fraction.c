#include "t81_fraction.h"

T81Status t81_fraction_init(T81Fraction *f) {
    T81Status st;
    st = t81_bigint_init(&f->num); if (st != T81_OK) return st;
    st = t81_bigint_init(&f->den); if (st != T81_OK) return st;
    return t81_bigint_from_int64(&f->den, 1);
}

T81Status t81_fraction_from_int64(T81Fraction *f, int64_t value) {
    T81Status st = t81_fraction_init(f);
    if (st != T81_OK) return st;
    st = t81_bigint_from_int64(&f->num, value);
    if (st != T81_OK) return st;
    return t81_fraction_normalize(f);
}

T81Status t81_fraction_from_bigints(T81Fraction *f,
                                    const T81BigInt *num,
                                    const T81BigInt *den) {
    // TODO: deep copy num/den, then normalize
    (void)f; (void)num; (void)den;
    return T81_ERR_ALLOC;
}

void t81_fraction_free(T81Fraction *f) {
    t81_bigint_free(&f->num);
    t81_bigint_free(&f->den);
}

T81Status t81_fraction_normalize(T81Fraction *f) {
    // TODO:
    //  - if num == 0 → den = 1
    //  - compute gcd(|num|, den), divide both
    //  - ensure den > 0
    (void)f;
    return T81_OK;
}

int t81_fraction_cmp(const T81Fraction *a, const T81Fraction *b) {
    // TODO: compare a.num * b.den vs b.num * a.den
    (void)a; (void)b;
    return 0;
}

T81Status t81_fraction_add(T81Fraction *out,
                           const T81Fraction *a,
                           const T81Fraction *b) {
    (void)out; (void)a; (void)b;
    // TODO: num = a.num*b.den + b.num*a.den
    //       den = a.den * b.den
    return T81_ERR_ALLOC;
}

/* similarly for sub/mul/div */

