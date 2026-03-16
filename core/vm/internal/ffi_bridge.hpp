// RFC-00B8 — VM-level FFI bridge: forward declarations for free functions
// defined in core/vm/ffi_dispatcher.cpp.
#pragma once

#include <cstdint>
#include "t81/ffi/ffi.hpp"
#include "t81/support/expected.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

void initialize_ffi_subsystem(t81::axion::Engine& policy_engine);
t81::ffi::FFIDispatcher* get_ffi_dispatcher();

std::expected<void, Trap> ffi_call(State& state, uint8_t function_index,
                                   uint64_t arg_count, uint64_t result_addr);
std::expected<void, Trap> ffi_register(State& state, uint64_t library_name_addr,
                                       uint64_t version_hash_addr);
std::expected<void, Trap> ffi_policy_set(State& state, uint64_t policy_type,
                                          uint64_t policy_value);

}  // namespace t81::vm
