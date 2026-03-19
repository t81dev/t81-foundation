/**
 * @file t81.hpp
 * @brief Master include file for the T81 C++ library.
 *
 * This header aggregates all the necessary components of the T81 Foundation's
 * C++ library, providing a single entry point for users. Including this file
 * grants access to core data types, the TISC instruction set, the VM, the
 * T81Lang compiler frontend, and other utilities.
 */

#pragma once

#include <array>
#include <cstring>
#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif
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
#include "t81/tensor/ops.hpp"  // transpose/slice/reshape/matmul/reduce
#include "t81/tensor/shape.hpp"

// ---------- IO surfaces (header-only declarations) ----------
#include "t81/io/tensor_loader.hpp"

// ---------- CanonFS surface ----------
#include "t81/canonfs.hpp"
#include "t81/canonfs_io.hpp"

// ---------- Hash/codec stubs (replace with canonical impls later) ----------
#include "t81/codec/base243.hpp"
#include "t81/tracing/base81.hpp"
#include "t81/tracing/canonhash.hpp"

// ---------- IR surface ----------
#include "t81/ir/encoding.hpp"
#include "t81/ir/insn.hpp"
#include "t81/ir/opcodes.hpp"

// ---------- Axion façade (stub) ----------
#include "t81/axion/api.hpp"

// ---------- Utilities ----------
#include "t81/entropy.hpp"

// ---------- Canonical v1.1 surface ----------
#include "t81/canonical.hpp"

// ---------- Modern core (spec-driven, canonical for new code) ----------
#include "t81/support/expected.hpp"
#include "t81/types/base81.hpp"
#include "t81/types/bigint.hpp"
#include "t81/types/fraction.hpp"
#include "t81/types/ids.hpp"
#include "t81/types/tensor.hpp"

// NOTE:
// New code should prefer canonical numeric headers (`t81/bigint.hpp`,
// `t81/fraction.hpp`) and `t81::v1` types. The `t81::core::{BigInt,Fraction}`
// surface remains as a compatibility shim.

// ---------- TISC ISA ----------
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

// ---------- VM ----------
#include "t81/vm/state.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

// ---------- T81Lang ----------
#include "t81/lang/ast.hpp"
#include "t81/lang/compiler.hpp"
#include "t81/lang/types.hpp"

// ---------- CanonFS ----------
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"

// ---------- Hanoi microkernel ----------
#include "t81/experimental/hanoi/error.hpp"
#include "t81/experimental/hanoi/kernel.hpp"
#include "t81/experimental/hanoi/types.hpp"

#include "t81/axion/context.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/cog/v1/symbolic_graph.hpp"
#include "t81/conversion.hpp"
#include "t81/native.hpp"

// ---------- Cognitive tiers ----------
#include "t81/experimental/cog/metrics.hpp"
#include "t81/experimental/cog/promotion.hpp"
#include "t81/experimental/cog/tier.hpp"
