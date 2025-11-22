// ternary_base.h — Unified ternary base interface

#ifndef TERNARY_BASE_H
#define TERNARY_BASE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------
// Ternary Base Enumeration
// ------------------------
typedef enum {
    BASE_81 = 81,
    BASE_243 = 243,
    BASE_729 = 729
} TernaryBase;

// ------------------------
// Abstract Opaque Handle
// ------------------------
typedef struct {
    TernaryBase base;
    void* data;           // Cast to actual struct internally
} TernaryHandle;

// ------------------------
// Trait-Like Interface
// ------------------------

/** Generic conversion functions */
TernaryHandle ternary_convert(TernaryHandle src, TernaryBase target_base);

/** Generic arithmetic */
int ternary_add(TernaryHandle a, TernaryHandle b, TernaryHandle* result);
int ternary_mul(TernaryHandle a, TernaryHandle b, TernaryHandle* result);
int ternary_free(TernaryHandle h);

/** Generic to string */
int ternary_to_string(TernaryHandle h, char** out);

// ------------------------
// Forward Stubs for .cweb drop-in modules
// ------------------------

// --- T243BigInt ---
TernaryHandle t243bigint_new_from_string(const char* str);
int t243bigint_add(TernaryHandle a, TernaryHandle b, TernaryHandle* result);
int t243bigint_mul(TernaryHandle a, TernaryHandle b, TernaryHandle* result);
void t243bigint_free(TernaryHandle h);

// --- T729Tensor ---
TernaryHandle t729tensor_new(int rank, const int* shape);
int t729tensor_contract(TernaryHandle a, TernaryHandle b, TernaryHandle* result);
void t729tensor_free(TernaryHandle h);

#ifdef __cplusplus
}
#endif

#endif // TERNARY_BASE_H
