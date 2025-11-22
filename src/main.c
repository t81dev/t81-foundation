#include <stdio.h>
#include "t81_core.h"

int main(void) {
    T81BigInt n;
    if (t81_bigint_from_int64(&n, 12345) != T81_OK) {
        fprintf(stderr, "failed to init bigint\n");
        return 1;
    }
    printf("Initialized T81BigInt from 12345 (len=%zu, sign=%d)\n",
           n.len, n.sign);
    t81_bigint_free(&n);
    return 0;
}

