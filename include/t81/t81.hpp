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

// ---------- Canonical v1.1 surface ----------
#include "t81/canonical.hpp"

// ---------- Modern core (spec-driven) ----------
#include "t81/support/expected.hpp"
#include "t81/core/base81.hpp"
#include "t81/core/bigint.hpp"
#include "t81/core/fraction.hpp"
#include "t81/core/tensor.hpp"
#include "t81/core/ids.hpp"

// ---------- TISC ISA ----------
#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/program.hpp"
#include "t81/tisc/encoding.hpp"

// ---------- VM ----------
#include "t81/vm/state.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

// ---------- T81Lang ----------
#include "t81/lang/types.hpp"
#include "t81/lang/ast.hpp"
#include "t81/lang/compiler.hpp"

// ---------- CanonFS ----------
#include "t81/canonfs/canon_types.hpp"
#include "t81/canonfs/canon_driver.hpp"

// ---------- Hanoi microkernel ----------
#include "t81/hanoi/types.hpp"
#include "t81/hanoi/error.hpp"
#include "t81/hanoi/kernel.hpp"

// ---------- Axion ----------
#include "t81/axion/verdict.hpp"
#include "t81/axion/context.hpp"
#include "t81/axion/engine.hpp"

// ---------- Cognitive tiers ----------
#include "t81/cog/tier.hpp"
#include "t81/cog/promotion.hpp"
#include "t81/cog/metrics.hpp"
