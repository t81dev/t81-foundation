#pragma once
#include "t81/config.hpp"

#include "t81/ternary.hpp"
#include "t81/bigint.hpp"
#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include "t81/tensor/ops.hpp"

#include "t81/canonfs.hpp"
#include "t81/canonfs_io.hpp"
#include "t81/entropy.hpp"

namespace t81 {
// Surface aliases for T81Lang parity
using T81BigInt = T243BigInt;
using T81Tensor = T729Tensor;
}
