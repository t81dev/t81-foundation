#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for BigInt
typedef struct t81_bigint_s* t81_bigint;

// Construct from ASCII (placeholder encoding per BigInt docs).
// Returns NULL on allocation failure.
t81_bigint t81_bigint_from_ascii(const char* s);

// Convert to newly-allocated C string. Caller must free().
// Returns NULL on failure.
char* t81_bigint_to_string(t81_bigint h);

// Release the handle (safe on NULL).
void t81_bigint_free(t81_bigint h);

#ifdef __cplusplus
} // extern "C"
#endif
