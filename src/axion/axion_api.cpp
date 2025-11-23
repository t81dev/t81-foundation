#include "t81/axion/api.hpp"

// No implementation needed: the Axion façade is header-only.
// This TU exists to satisfy build systems expecting a .cpp file.
namespace t81::axion {
  // Optional: a tiny anchor to ensure the object file isn't empty.
  static void _t81_axion_anchor() {}
}
