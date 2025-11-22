#ifndef T81_BIGINT_H
#define T81_BIGINT_H

#include <stddef.h>
#include <stdint.h>

/*
 * T81BigInt — canonical base-81 integer.
 *
 * Representation:
 *  - sign: -1, 0, +1
 *  - digits: least-significant digit first
 *  - each digit in [0, 80]
 *  - invariant: no leading zero digits except for zero itself
 */

typedef struct {
    int8_t   sign;     // -1, 0, +1
    size_t   len;      // number of digits in use
    size_t   cap;      // capacity of digits array
    uint8_t *digits;   // little-endian base-81 digits
} T81BigInt;

typedef enum {
    T81_OK = 0,
    T81_ERR_ALLOC,
    T81_ERR_PARSE,
    T81_ERR_DIV_ZERO
} T81Status;

#ifdef __cplusplus
extern "C" {
#endif

/* Lifecycle */

T81Status t81_bigint_init(T81BigInt *n);
T81Status t81_bigint_from_int64(T81BigInt *n, int64_t value);
T81Status t81_bigint_from_str_base81(T81BigInt *n, const char *s);
/* canonical string: optional “+” or “-” prefix, digits 0–80 encoded
 * as ASCII 0–9, A–Z, a–z, and additional symbols; mapping TBD.
 */

void      t81_bigint_free(T81BigInt *n);

/* Canonicalization */

void      t81_bigint_normalize(T81BigInt *n);  // enforce sign & strip leading zeros

/* Comparisons */

int       t81_bigint_cmp(const T81BigInt *a, const T81BigInt *b);
/* returns -1, 0, +1 */

/* Arithmetic — all results canonicalized */

T81Status t81_bigint_add(T81BigInt *out,
                         const T81BigInt *a,
                         const T81BigInt *b);

T81Status t81_bigint_sub(T81BigInt *out,
                         const T81BigInt *a,
                         const T81BigInt *b);

T81Status t81_bigint_mul(T81BigInt *out,
                         const T81BigInt *a,
                         const T81BigInt *b);

T81Status t81_bigint_divmod(T81BigInt *q,
                            T81BigInt *r,
                            const T81BigInt *a,
                            const T81BigInt *b);
/* q = a / b (trunc toward zero), r = a mod b
 * if b == 0 → T81_ERR_DIV_ZERO and no output modified
 */

/* Conversion */

T81Status t81_bigint_to_str_base81(const T81BigInt *n,
                                   char *buf,
                                   size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif // T81_BIGINT_H

