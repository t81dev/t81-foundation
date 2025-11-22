#ifndef T81_RECURSION_H
#define T81_RECURSION_H

#include "t81.h"

#define T81RECURSE_MAX_DEPTH 1024

typedef TritError (*T81RecursiveCallback)(
    T81BigIntHandle current,
    void* context,
    T81BigIntHandle* result
);

TritError t81bigint_factorial_recursive(T81BigIntHandle n, T81BigIntHandle* result);
TritError t81bigint_fibonacci_tail(T81BigIntHandle n, T81BigIntHandle* result);
TritError t81recurse(T81BigIntHandle start, T81RecursiveCallback cb, void* context, T81BigIntHandle* result);
void t81recursion_reset_depth();

#endif
