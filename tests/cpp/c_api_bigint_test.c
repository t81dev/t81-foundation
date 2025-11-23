#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "src/c_api/t81_c_api.h"

int main(void) {
  // Construct from canonical base-81 digit string (digits 0..80 separated by '.')
  t81_bigint h = t81_bigint_from_ascii("1.23.5");
  assert(h != NULL);

  // Convert back to string (implementation-dependent debug format)
  char* s = t81_bigint_to_string(h);
  assert(s != NULL);

  // Should be non-empty; we just sanity-check roundtrip produced something.
  assert(s[0] != '\0');

  // Clean up
  free(s);
  t81_bigint_free(h);

  puts("c_api_bigint ok");
  return 0;
}
