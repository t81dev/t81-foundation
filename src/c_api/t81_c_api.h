#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// forward-compatible C handles
typedef struct t81_bigint_s* t81_bigint;

t81_bigint t81_bigint_from_ascii(const char* s);
void       t81_bigint_free(t81_bigint h);
char*      t81_bigint_to_string(t81_bigint h); // caller free

#ifdef __cplusplus
}
#endif
