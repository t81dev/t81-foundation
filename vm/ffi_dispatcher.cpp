#include "t81/ffi/ffi.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/context.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/state.hpp"
#include "t81/support/expected.hpp"

#include <chrono>
#include <cstring>
#include <string_view>
#include <unordered_map>
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif
#include <algorithm>

namespace t81::vm {

// Global FFI dispatcher instance
static std::unique_ptr<t81::ffi::FFIDispatcher> g_ffi_dispatcher = nullptr;

// FFI instruction implementations
class FFIInstructions {
public:
    static void append_arg_record(std::vector<std::uint8_t>& encoded,
                                  std::uint8_t tag,
                                  const std::uint8_t* payload,
                                  std::uint32_t size) {
        encoded.push_back(tag);
        const auto* size_bytes = reinterpret_cast<const std::uint8_t*>(&size);
        encoded.insert(encoded.end(), size_bytes, size_bytes + sizeof(size));
        encoded.insert(encoded.end(), payload, payload + size);
    }

    static ThreadContext* current_context(State& state) {
        if (state.current_context >= state.contexts.size()) {
            return nullptr;
        }
        return &state.contexts[state.current_context];
    }

    static std::optional<std::string_view> symbol_text(const State& state, std::int64_t handle) {
        if (handle <= 0) {
            return std::nullopt;
        }
        const std::size_t idx = static_cast<std::size_t>(handle - 1);
        if (idx >= state.symbols.size()) {
            return std::nullopt;
        }
        return std::string_view{state.symbols[idx]};
    }

    static std::expected<std::string, Trap> register_symbol_text(
        const State& state,
        std::int32_t reg_index) {
        auto* ctx = current_context(const_cast<State&>(state));
        if (!ctx ||
            reg_index < 0 ||
            static_cast<std::size_t>(reg_index) >= ctx->registers.size()) {
            return t81::unexpected(Trap::DecodeFault);
        }
        if (ctx->register_tags[reg_index] != ValueTag::SymbolHandle) {
            return t81::unexpected(Trap::TypeFault);
        }
        auto text = symbol_text(state, ctx->registers[reg_index]);
        if (!text.has_value()) {
            return t81::unexpected(Trap::DecodeFault);
        }
        return std::string{text->begin(), text->end()};
    }

    static std::vector<std::uint8_t> serialize_stack_args(const State& state, std::uint64_t arg_count) {
        std::vector<std::uint8_t> encoded;
        if (state.current_context >= state.contexts.size()) {
            return encoded;
        }
        const auto& ctx = state.contexts[state.current_context];
        const std::size_t available = state.layout.stack.limit >= ctx.sp
                                          ? (state.layout.stack.limit - ctx.sp)
                                          : 0;
        const std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(arg_count), available);
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t addr = ctx.sp + (count - 1 - i);
            const auto value = state.memory[addr];
            const auto tag = state.memory_tags[addr];
            if (tag == ValueTag::SymbolHandle) {
                auto text = symbol_text(state, value);
                if (!text.has_value()) {
                    continue;
                }
                append_arg_record(encoded,
                                  static_cast<std::uint8_t>(tag),
                                  reinterpret_cast<const std::uint8_t*>(text->data()),
                                  static_cast<std::uint32_t>(text->size()));
                continue;
            }
            if (tag == ValueTag::FloatHandle) {
                if (value <= 0 || static_cast<std::size_t>(value) > state.floats.size()) {
                    continue;
                }
                const double f = state.floats[static_cast<std::size_t>(value - 1)];
                const auto* bytes = reinterpret_cast<const std::uint8_t*>(&f);
                append_arg_record(encoded,
                                  static_cast<std::uint8_t>(tag),
                                  bytes,
                                  static_cast<std::uint32_t>(sizeof(f)));
                continue;
            }
            if (tag == ValueTag::StringVectorHandle) {
                if (value <= 0 || static_cast<std::size_t>(value) > state.string_vectors.size()) {
                    continue;
                }
                const auto& vec = state.string_vectors[static_cast<std::size_t>(value - 1)];
                std::vector<std::uint8_t> payload;
                const std::uint32_t count = static_cast<std::uint32_t>(vec.size());
                const auto* count_bytes = reinterpret_cast<const std::uint8_t*>(&count);
                payload.insert(payload.end(), count_bytes, count_bytes + sizeof(count));
                for (const auto& item : vec) {
                    const std::uint32_t size = static_cast<std::uint32_t>(item.size());
                    const auto* size_bytes = reinterpret_cast<const std::uint8_t*>(&size);
                    payload.insert(payload.end(), size_bytes, size_bytes + sizeof(size));
                    payload.insert(payload.end(),
                                   reinterpret_cast<const std::uint8_t*>(item.data()),
                                   reinterpret_cast<const std::uint8_t*>(item.data()) + item.size());
                }
                append_arg_record(encoded,
                                  static_cast<std::uint8_t>(tag),
                                  payload.data(),
                                  static_cast<std::uint32_t>(payload.size()));
                continue;
            }
            if (tag == ValueTag::IntVectorHandle) {
                if (value <= 0 || static_cast<std::size_t>(value) > state.int_vectors.size()) {
                    continue;
                }
                const auto& vec = state.int_vectors[static_cast<std::size_t>(value - 1)];
                std::vector<std::uint8_t> payload;
                const std::uint32_t count = static_cast<std::uint32_t>(vec.size());
                const auto* count_bytes = reinterpret_cast<const std::uint8_t*>(&count);
                payload.insert(payload.end(), count_bytes, count_bytes + sizeof(count));
                for (const auto& item : vec) {
                    const auto* item_bytes = reinterpret_cast<const std::uint8_t*>(&item);
                    payload.insert(payload.end(), item_bytes, item_bytes + sizeof(item));
                }
                append_arg_record(encoded,
                                  static_cast<std::uint8_t>(tag),
                                  payload.data(),
                                  static_cast<std::uint32_t>(payload.size()));
                continue;
            }
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
            append_arg_record(encoded,
                              static_cast<std::uint8_t>(tag),
                              bytes,
                              static_cast<std::uint32_t>(sizeof(value)));
        }
        return encoded;
    }

    static std::int64_t intern_symbol(State& state, std::string text) {
        for (std::size_t i = 0; i < state.symbols.size(); ++i) {
            if (state.symbols[i] == text) {
                return static_cast<std::int64_t>(i + 1);
            }
        }
        state.symbols.push_back(std::move(text));
        return static_cast<std::int64_t>(state.symbols.size());
    }

    static std::expected<void, Trap> store_result(State& state,
                                                  std::int32_t result_reg,
                                                  const t81::ffi::FFICallResult& result) {
        auto* ctx = current_context(state);
        if (!ctx ||
            result_reg < 0 ||
            static_cast<std::size_t>(result_reg) >= ctx->registers.size()) {
            return t81::unexpected(Trap::DecodeFault);
        }
        if (result_reg == 0 || (result_reg >= 75 && result_reg <= 80)) {
            return {};
        }

        if (std::holds_alternative<std::uint64_t>(result.result)) {
            ctx->registers[result_reg] =
                static_cast<std::int64_t>(std::get<std::uint64_t>(result.result));
            ctx->register_tags[result_reg] = ValueTag::Int;
            return {};
        }
        if (std::holds_alternative<std::int64_t>(result.result)) {
            ctx->registers[result_reg] = std::get<std::int64_t>(result.result);
            ctx->register_tags[result_reg] = ValueTag::Int;
            return {};
        }
        if (std::holds_alternative<double>(result.result)) {
            state.floats.push_back(std::get<double>(result.result));
            ctx->registers[result_reg] = static_cast<std::int64_t>(state.floats.size());
            ctx->register_tags[result_reg] = ValueTag::FloatHandle;
            return {};
        }
        if (std::holds_alternative<std::string>(result.result)) {
            ctx->registers[result_reg] =
                intern_symbol(state, std::get<std::string>(result.result));
            ctx->register_tags[result_reg] = ValueTag::SymbolHandle;
            return {};
        }
        if (std::holds_alternative<std::vector<std::string>>(result.result)) {
            state.string_vectors.push_back(std::get<std::vector<std::string>>(result.result));
            ctx->registers[result_reg] = static_cast<std::int64_t>(state.string_vectors.size());
            ctx->register_tags[result_reg] = ValueTag::StringVectorHandle;
            return {};
        }
        if (std::holds_alternative<std::vector<std::int64_t>>(result.result)) {
            state.int_vectors.push_back(std::get<std::vector<std::int64_t>>(result.result));
            ctx->registers[result_reg] = static_cast<std::int64_t>(state.int_vectors.size());
            ctx->register_tags[result_reg] = ValueTag::IntVectorHandle;
            return {};
        }
        return t81::unexpected(Trap::TypeFault);
    }

    static Trap map_call_failure(const t81::ffi::FFICallResult& result) {
        switch (result.status) {
            case t81::ffi::FFIResult::Success:
                return Trap::None;
            case t81::ffi::FFIResult::PolicyDenied:
            case t81::ffi::FFIResult::QuarantineRequired:
                return Trap::FFIPolicyDenied;
            case t81::ffi::FFIResult::TypeMismatch:
                return Trap::TypeFault;
            case t81::ffi::FFIResult::ResourceExhausted:
                return Trap::FFIMemoryExhausted;
            case t81::ffi::FFIResult::ExternalError:
                return Trap::FFILoadError;
        }
        return Trap::FFIPolicyDenied;
    }

    // FFI_CALL - Call foreign function with governance
    static std::expected<void, Trap> ffi_call(
        State& state,
        std::int32_t result_reg,
        uint64_t arg_count,
        std::int32_t function_symbol_index
    ) {
        if (!g_ffi_dispatcher) {
            return t81::unexpected(Trap::FFINotInitialized);
        }

        if (function_symbol_index <= 0 ||
            static_cast<std::size_t>(function_symbol_index) > state.symbols.size()) {
            return t81::unexpected(Trap::DecodeFault);
        }

        const std::string& function_name =
            state.symbols[static_cast<std::size_t>(function_symbol_index - 1)];
        auto& registry = t81::ffi::FFILibraryRegistry::instance();
        auto function = registry.lookup_function(function_name);
        if (!function) {
            return t81::unexpected(Trap::FFIPolicyDenied);
        }

        static uint64_t s_call_id_counter = 0;
        t81::ffi::FFICallContext context{
            .function_name = function_name,
            .function_type = function.value()->type,
            .caller_location = "VM_FFI_Call",
            .serialized_args = serialize_stack_args(state, arg_count),
            .call_id = ++s_call_id_counter,
            .resource_cost = static_cast<uint32_t>(arg_count * 8)
        };

        auto result = g_ffi_dispatcher->call(context);
        if (!result) {
            return t81::unexpected(Trap::FFIPolicyDenied);
        }

        if (result->status != t81::ffi::FFIResult::Success) {
            return t81::unexpected(map_call_failure(*result));
        }

        auto stored = store_result(state, result_reg, *result);
        if (!stored) {
            return stored;
        }
        return {};
    }

    // FFI_REGISTER - Register foreign library
    static std::expected<void, Trap> ffi_register(
        State& state,
        std::int32_t library_name_reg,
        std::int32_t version_hash_reg
    ) {
        if (!g_ffi_dispatcher) {
            return t81::unexpected(Trap::FFINotInitialized);
        }

        auto library_name = register_symbol_text(state, library_name_reg);
        if (!library_name) {
            return t81::unexpected(library_name.error());
        }
        auto version_hash = register_symbol_text(state, version_hash_reg);
        if (!version_hash) {
            return t81::unexpected(version_hash.error());
        }

        auto& registry = t81::ffi::FFILibraryRegistry::instance();
        auto result = registry.register_library(*library_name, *version_hash, {});
        if (!result) {
            return t81::unexpected(Trap::FFIRegistrationError);
        }
        return {};
    }

    // FFI_POLICY_SET - Set FFI policy
    static std::expected<void, Trap> ffi_policy_set(
        [[maybe_unused]] State& state,
        uint64_t policy_type,
        uint64_t policy_value
    ) {
        if (!g_ffi_dispatcher) {
            return t81::unexpected(Trap::FFINotInitialized);
        }
        
        // Set resource quotas based on policy type
        switch (policy_type) {
            case 0: // Time quota
                g_ffi_dispatcher->set_resource_quota(policy_value, 1024 * 1024 * 1024);
                break;
            case 1: // Memory quota
                g_ffi_dispatcher->set_resource_quota(1000000000, policy_value);
                break;
            default:
                return t81::unexpected(Trap::FFIPolicyDenied);
        }
        
        return {};
    }
};

// Initialize FFI subsystem
void initialize_ffi_subsystem(t81::axion::Engine& policy_engine) {
    g_ffi_dispatcher = std::make_unique<t81::ffi::FFIDispatcher>(policy_engine);
}

// Get global FFI dispatcher
t81::ffi::FFIDispatcher* get_ffi_dispatcher() {
    return g_ffi_dispatcher.get();
}

// FFI instruction handlers for VM integration
std::expected<void, Trap> ffi_call(
    State& state,
    std::int32_t result_reg,
    uint64_t arg_count,
    std::int32_t function_symbol_index
) {
    return FFIInstructions::ffi_call(state, result_reg, arg_count, function_symbol_index);
}

std::expected<void, Trap> ffi_register(
    State& state,
    std::int32_t library_name_reg,
    std::int32_t version_hash_reg
) {
    return FFIInstructions::ffi_register(state, library_name_reg, version_hash_reg);
}

std::expected<void, Trap> ffi_policy_set(
    State& state,
    uint64_t policy_type,
    uint64_t policy_value
) {
    return FFIInstructions::ffi_policy_set(state, policy_type, policy_value);
}

} // namespace t81::vm
