#pragma once

// ---------- Core config & traits ----------
#include "t81/config.hpp"
#include "t81/traits.hpp"

// ---------- Primitive types ----------
#include "t81/ternary.hpp"

// ---------- Math types ----------
#include "t81/bigint.hpp"
#include "t81/fraction.hpp"

// ---------- Tensor core & common ops ----------
#include "t81/tensor.hpp"
#include "t81/tensor/shape.hpp"
#include "t81/tensor/ops.hpp"        // transpose/slice/reshape/matmul/reduce

// ---------- IO surfaces (header-only declarations) ----------
#include "t81/io/tensor_loader.hpp"

// ---------- CanonFS surface ----------
#include "t81/canonfs.hpp"
#include "t81/canonfs_io.hpp"

// ---------- Hash/codec stubs (replace with canonical impls later) ----------
#include "t81/hash/base81.hpp"
#include "t81/hash/canonhash.hpp"
#include "t81/codec/base243.hpp"

// ---------- IR surface ----------
#include "t81/ir/opcodes.hpp"
#include "t81/ir/insn.hpp"
#include "t81/ir/encoding.hpp"

// ---------- Axion façade (stub) ----------
#include "t81/axion/api.hpp"

// ---------- Utilities ----------
#include "t81/entropy.hpp"
#include "t81/detail/assert.hpp"
#include "t81/detail/bitops.hpp"
