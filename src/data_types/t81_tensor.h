#ifndef T81_TENSOR_H
#define T81_TENSOR_H

#include "t81_bigint.h"
#include <stddef.h>

/*
 * T81Tensor — canonical shape-aware tensor.
 *
 * For now, store:
 *  - rank
 *  - dims[rank]
 *  - total_size = product(dims)
 *  - contiguous data of T81BigInt values
 */

#define T81_TENSOR_MAX_RANK 9

typedef struct {
    uint8_t   rank;
    size_t    dims[T81_TENSOR_MAX_RANK];
    size_t    total_size;
    T81BigInt *data;  // length == total_size
} T81Tensor;

#ifdef __cplusplus
extern "C" {
#endif

T81Status t81_tensor_init(T81Tensor *t,
                          uint8_t rank,
                          const size_t *dims);

void      t81_tensor_free(T81Tensor *t);

T81Status t81_tensor_get(const T81Tensor *t,
                         const size_t *indices,
                         T81BigInt *out);  // copy

T81Status t81_tensor_set(T81Tensor *t,
                         const size_t *indices,
                         const T81BigInt *value);

T81Status t81_tensor_vec_add(T81Tensor *out,
                             const T81Tensor *a,
                             const T81Tensor *b); // rank-1

T81Status t81_tensor_mat_mul(T81Tensor *out,
                             const T81Tensor *a,
                             const T81Tensor *b); // rank-2

#ifdef __cplusplus
}
#endif

#endif // T81_TENSOR_H

