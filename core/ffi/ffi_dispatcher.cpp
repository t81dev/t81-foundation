#include "t81/ffi/ffi.hpp"
#include "t81/axion/engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

#include <chrono>
#include <unordered_map>
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif
#include <cstring>
#include <algorithm>

namespace t81::ffi {

// Implementation of FFILibraryRegistry
FFILibraryRegistry& FFILibraryRegistry::instance() {
    static FFILibraryRegistry registry;
    return registry;
}

t81::expected<void, std::string> FFILibraryRegistry::register_library(
    const std::string& library_name,
    const std::string& version_hash,
    const std::vector<FFIFunction>& functions
) {
    // Validate library name and hash
    if (library_name.empty() || version_hash.empty()) {
        return t81::unexpected(std::string("Library name and version hash cannot be empty"));
    }
    
    // Check for duplicate registration
    for (const auto& func : registered_functions_) {
        if (func.library_name == library_name) {
            return t81::unexpected(std::string("Library already registered: " + library_name));
        }
    }
    
    // Register all functions
    for (const auto& func : functions) {
        function_index_[func.name] = registered_functions_.size();
        registered_functions_.push_back(func);
    }
    
    return {};
}

t81::expected<FFIFunction*, std::string> FFILibraryRegistry::lookup_function(
    const std::string& function_name
) {
    auto it = function_index_.find(function_name);
    if (it == function_index_.end()) {
        return t81::unexpected(std::string("Function not found: " + function_name));
    }
    
    return &registered_functions_[it->second];
}

std::vector<FFIFunction> FFILibraryRegistry::list_functions() const {
    return registered_functions_;
}

bool FFILibraryRegistry::is_quarantined(const std::string& function_name) const {
    auto it = function_index_.find(function_name);
    if (it == function_index_.end()) {
        return true; // Unknown functions are quarantined by default
    }
    
    return registered_functions_[it->second].type == FFIType::Quarantined;
}

// Implementation of FFIDispatcher
FFIDispatcher::FFIDispatcher(axion::Engine& policy_engine)
    : policy_engine_(policy_engine),
      max_call_time_ns_(1000000000),  // 1 second default
      max_memory_usage_(1024 * 1024 * 1024) {  // 1GB default
}

t81::expected<FFICallResult, std::string> FFIDispatcher::call(
    const FFICallContext& context
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Lookup function
    auto& registry = FFILibraryRegistry::instance();
    auto func_result = registry.lookup_function(context.function_name);
    if (!func_result) {
        return FFICallResult{
            .status = FFIResult::PolicyDenied,
            .result = {},
            .error_message = "Function not found or not registered: " + context.function_name,
            .execution_time_ns = 0,
            .audit_events = {"FunctionLookup"},
            .provenance_hash = ""
        };
    }
    
    const FFIFunction& function = *func_result.value();
    
    // Check quarantine status
    if (function.type == FFIType::Quarantined) {
        return FFICallResult{
            .status = FFIResult::QuarantineRequired,
            .result = {},
            .error_message = "Function is quarantined: " + context.function_name,
            .execution_time_ns = 0,
            .audit_events = {"QuarantineCheck"},
            .provenance_hash = ""
        };
    }
    
    // Policy check
    auto policy_result = check_policy_(context, function);
    if (!policy_result) {
        return FFICallResult{
            .status = FFIResult::PolicyDenied,
            .result = {},
            .error_message = policy_result.error(),
            .execution_time_ns = 0,
            .audit_events = {"PolicyCheck"},
            .provenance_hash = ""
        };
    }
    
    // Resource quota check
    if (context.resource_cost > max_memory_usage_) {
        return FFICallResult{
            .status = FFIResult::ResourceExhausted,
            .result = {},
            .error_message = "Resource quota exceeded for FFI call",
            .execution_time_ns = 0,
            .audit_events = {"ResourceQuotaCheck"},
            .provenance_hash = ""
        };
    }
    
    // Execute the call
    auto call_result = execute_call_(context, function);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    call_result.execution_time_ns = 
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    
    // Generate audit events
    generate_audit_events_(context, function, call_result);
    
    // Store in audit trail
    audit_trail_.push_back(call_result);
    
    return call_result;
}

void FFIDispatcher::set_resource_quota(uint64_t max_time_ns, uint64_t max_memory) {
    max_call_time_ns_ = max_time_ns;
    max_memory_usage_ = max_memory;
}

std::vector<FFICallResult> FFIDispatcher::get_audit_trail() const {
    return audit_trail_;
}

t81::expected<void, std::string> FFIDispatcher::check_policy_(
    const FFICallContext& context,
    [[maybe_unused]] const FFIFunction& function
) {
    // Create Axion syscall context
    axion::SyscallContext syscall_context{
        .snapshot = {},  // No snapshot for FFI calls
        .caller = context.caller_location,
        .syscall = "FFI_Call",
        .payload = context.function_name,
        .policy = nullptr,
        .trace_reasons = {},
        .pc = 0,
        .next_opcode = t81::tisc::Opcode::FFICall,
        .recursion_depth = 0,
        .instruction_count = 0,
        .stack_usage = 0,
        .reflection_count = 0,
        .meta_write_count = 0,
        .current_tier = 0
    };
    
    // Check with policy engine
    auto policy_result = policy_engine_.evaluate(syscall_context);
    
    if (policy_result.kind != axion::VerdictKind::Allow) {
        return t81::unexpected(std::string("Policy denied: " + policy_result.reason));
    }
    
    // TODO: Check required capabilities when Engine interface supports it
    // For now, assume all capabilities are available
    
    return {};
}

FFICallResult FFIDispatcher::execute_call_(
    const FFICallContext& context,
    const FFIFunction& function
) {
    // This is a simplified implementation
    // In practice, this would involve:
    // 1. Type marshaling from T81 to C types
    // 2. Calling the actual foreign function
    // 3. Type marshaling from C back to T81 types
    // 4. Error handling and cleanup
    
    try {
        // Load library if not already loaded
#ifndef _WIN32
        void* handle = dlopen(function.library_name.c_str(), RTLD_LAZY);
        if (!handle) {
            return FFICallResult{
                .status = FFIResult::ExternalError,
                .result = {},
                .error_message = "Failed to load library: " + std::string(dlerror()),
                .execution_time_ns = 0,
                .audit_events = {"LibraryLoad"},
                .provenance_hash = ""
            };
        }

        // Get function pointer
        void* func_ptr = dlsym(handle, function.name.c_str());
        if (!func_ptr) {
            dlclose(handle);
            return FFICallResult{
                .status = FFIResult::ExternalError,
                .result = {},
                .error_message = "Function not found in library: " + function.name,
                .execution_time_ns = 0,
                .audit_events = {"FunctionLookup"},
                .provenance_hash = ""
            };
        }
#else
        HMODULE handle = LoadLibraryA(function.library_name.c_str());
        if (!handle) {
            return FFICallResult{
                .status = FFIResult::ExternalError,
                .result = {},
                .error_message = "Failed to load library",
                .execution_time_ns = 0,
                .audit_events = {"LibraryLoad"},
                .provenance_hash = ""
            };
        }
        
        // Get function pointer
        void* func_ptr = (void*)GetProcAddress(handle, function.name.c_str());
        if (!func_ptr) {
            FreeLibrary(handle);
            return FFICallResult{
                .status = FFIResult::ExternalError,
                .result = {},
                .error_message = "Function not found in library: " + function.name,
                .execution_time_ns = 0,
                .audit_events = {"FunctionLookup"},
                .provenance_hash = ""
            };
        }
#endif
        
        // For deterministic functions, we could cache results
        if (function.type == FFIType::Deterministic) {
            // Check cache first
            auto cache_key = generate_cache_key_(context, function);
            auto cached_result = lookup_cache_(cache_key);
            if (cached_result) {
                return *cached_result;
            }
        }
        
        // Execute function (simplified - actual implementation depends on calling convention)
        auto result = call_foreign_function_(func_ptr, context, function);
        
        // Cache deterministic results
        if (function.type == FFIType::Deterministic) {
            auto cache_key = generate_cache_key_(context, function);
            cache_result_(cache_key, result);
        }
        
#ifndef _WIN32
        dlclose(handle);
#else
        FreeLibrary(handle);
#endif
        
        return result;
        
    } catch (const std::exception& e) {
        return FFICallResult{
            .status = FFIResult::ExternalError,
            .result = {},
            .error_message = "Exception in FFI call: " + std::string(e.what()),
            .execution_time_ns = 0,
            .audit_events = {"Exception"},
            .provenance_hash = ""
        };
    }
}

void FFIDispatcher::generate_audit_events_(
    const FFICallContext& context,
    const FFIFunction& function,
    const FFICallResult& result
) {
    // Generate provenance hash
    std::string provenance_data = context.function_name + ":" + 
                              std::to_string(context.call_id) + ":" +
                              std::to_string(static_cast<int>(result.status)) + ":" +
                              result.error_message;
    std::string provenance_hash = compute_provenance_hash_(provenance_data);
    
    // Create audit events
    std::vector<std::string> events = {
        "FFICall_" + context.function_name,
        "FunctionType_" + std::to_string(static_cast<int>(function.type)),
        "Library_" + function.library_name,
        "Status_" + std::to_string(static_cast<int>(result.status)),
        "ExecutionTime_" + std::to_string(result.execution_time_ns) + "ns"
    };
    
    // Add error events if applicable
    if (result.status != FFIResult::Success) {
        events.push_back("Error_" + result.error_message);
    }
    
    // Store events in result
    const_cast<FFICallResult&>(result).audit_events = events;
    const_cast<FFICallResult&>(result).provenance_hash = provenance_hash;
}

// Helper functions
std::string FFIDispatcher::generate_cache_key_(
    const FFICallContext& context,
    [[maybe_unused]] const FFIFunction& function
) {
    return context.function_name + ":" + 
           std::to_string(context.call_id) + ":" +
           std::to_string(context.serialized_args.size());
}

std::optional<FFICallResult> FFIDispatcher::lookup_cache_(const std::string& key) {
    auto it = deterministic_cache_.find(key);
    if (it != deterministic_cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void FFIDispatcher::cache_result_(
    const std::string& key,
    const FFICallResult& result
) {
    deterministic_cache_[key] = result;
}

std::string FFIDispatcher::compute_provenance_hash_(const std::string& data) {
    // Simple hash implementation - in practice would use CanonFS
    std::hash<std::string> hasher;
    return std::to_string(hasher(data));
}

FFICallResult FFIDispatcher::call_foreign_function_(
    void* func_ptr,
    [[maybe_unused]] const FFICallContext& context,
    const FFIFunction& function
) {
    // This is a placeholder implementation
    // Actual implementation would depend on function signature and calling convention
    
    if (function.return_type == "uint64_t") {
        uint64_t result = 0;
        // Call function with appropriate casting based on signature
        auto typed_func = reinterpret_cast<uint64_t(*)()>(func_ptr);
        result = typed_func();
        
        return FFICallResult{
            .status = FFIResult::Success,
            .result = result,
            .error_message = "",
            .execution_time_ns = 0,  // Will be set by caller
            .audit_events = {},
            .provenance_hash = ""
        };
    }
    
    // Add more type handling as needed
    return FFICallResult{
        .status = FFIResult::TypeMismatch,
        .result = {},
        .error_message = "Unsupported return type: " + function.return_type,
        .execution_time_ns = 0,
        .audit_events = {"TypeMismatch"},
        .provenance_hash = ""
    };
}

} // namespace t81::ffi
