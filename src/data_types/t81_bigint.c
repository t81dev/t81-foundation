#include "t81_bigint.h"
#include <stdlib.h>
#include <string.h>

static T81Status t81_bigint_reserve(T81BigInt *n, size_t cap) {
    if (cap <= n->cap) return T81_OK;
    uint8_t *new_digits = realloc(n->digits, cap * sizeof(uint8_t));
    if (!new_digits) return T81_ERR_ALLOC;
    n->digits = new_digits;
    n->cap = cap;
    return T81_OK;
}

T81Status t81_bigint_init(T81BigInt *n) {
    if (!n) return T81_ERR_ALLOC;
    n->sign = 0;
    n->len = 0;
    n->cap = 0;
    n->digits = NULL;
    return T81_OK;
}

T81Status t81_bigint_from_int64(T81BigInt *n, int64_t value) {
    t81_bigint_init(n);
    if (value == 0) {
        n->sign = 0;
        return T81_OK;
    }
    n->sign = (value < 0) ? -1 : 1;
    uint64_t v = (value < 0) ? (uint64_t)(-value) : (uint64_t)value;

    // naive base-81 expansion (unsigned magnitude)
    while (v > 0) {
        if (n->len == n->cap) {
            T81Status st = t81_bigint_reserve(n, n->cap ? n->cap * 2 : 4);
            if (st != T81_OK) return st;
        }
        uint64_t digit = v % 81u;
        n->digits[n->len++] = (uint8_t)digit;
        v /= 81u;
    }
    t81_bigint_normalize(n);
    return T81_OK;
}

T81Status t81_bigint_from_str_base81(T81BigInt *n, const char *s) {
    // stub: parse canonical base-81 string into T81BigInt
    // TODO: implement mapping from characters → 0..80 digits
    (void)n;
    (void)s;
    return T81_ERR_PARSE;
}

void t81_bigint_free(T81BigInt *n) {
    if (!n) return;
    free(n->digits);
    n->digits = NULL;
    n->len = n->cap = 0;
    n->sign = 0;
}

void t81_bigint_normalize(T81BigInt *n) {
    while (n->len > 0 && n->digits[n->len - 1] == 0) {
        n->len--;
    }
    if (n->len == 0) {
        n->sign = 0;
    }
}

int t81_bigint_cmp(const T81BigInt *a, const T81BigInt *b) {
    if (a->sign != b->sign) {
        return (a->sign < b->sign) ? -1 : 1;
    }
    if (a->sign == 0) return 0;

    if (a->len != b->len) {
        if (a->len < b->len) return (a->sign > 0) ? -1 : 1;
        else                 return (a->sign > 0) ? 1 : -1;
    }
    for (size_t i = a->len; i-- > 0; ) {
        if (a->digits[i] == b->digits[i]) continue;
        if (a->digits[i] < b->digits[i]) return (a->sign > 0) ? -1 : 1;
        else                              return (a->sign > 0) ? 1 : -1;
    }
    return 0;
}

/* Arithmetic: TODO real base-81 implementations; these are placeholders */

T81Status t81_bigint_add(T81BigInt *out,
                         const T81BigInt *a,
                         const T81BigInt *b) {
    (void)out; (void)a; (void)b;
    // TODO: implement full base-81 addition with carries
    return T81_ERR_ALLOC;
}

T81Status t81_bigint_sub(T81BigInt *out,
                         const T81BigInt *a,
                         const T81BigInt *b) {
    (void)out; (void)a; (void)b;
    // TODO
    return T81_ERR_ALLOC;
}

T81Status t81_bigint_mul(T81BigInt *out,
                         const T81BigInt *a,
                         const T81BigInt *b) {
    (void)out; (void)a; (void)b;
    // TODO
    return T81_ERR_ALLOC;
}

T81Status t81_bigint_divmod(T81BigInt *q,
                            T81BigInt *r,
                            const T81BigInt *a,
                            const T81BigInt *b) {
    (void)q; (void)r; (void)a; (void)b;
    // TODO
    return T81_ERR_DIV_ZERO;
}

T81Status t81_bigint_to_str_base81(const T81BigInt *n,
                                   char *buf,
                                   size_t buf_size) {
    (void)n; (void)buf; (void)buf_size;
    // TODO: implement canonical string representation
    return T81_ERR_ALLOC;
}

