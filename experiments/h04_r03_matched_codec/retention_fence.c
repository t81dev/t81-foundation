/* Ordering fence only: does not erase bytes, evict cache lines, or reclaim RSS. */
#include <stdatomic.h>
void h04_retention_fence(void) {
    atomic_thread_fence(memory_order_seq_cst);
}
