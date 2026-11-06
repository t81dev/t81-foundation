#include <cstdint>
#include <string_view>
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  std::string_view mode = argv[1];
  if (mode == "heap") {
    volatile std::uint8_t* p = new std::uint8_t[1];
    volatile unsigned i = 1;
    p[i] = 1;
    delete[] p;
  } else if (mode == "shift") {
    volatile unsigned n = sizeof(unsigned) * 8;
    volatile unsigned one = 1;
    volatile unsigned result = one << n;
    (void)result;
  } else
    return 2;
  return 0;
}
