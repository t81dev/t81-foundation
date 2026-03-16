#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <memory>
#include <functional>

#include "t81/support/expected.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/axion/engine.hpp"

namespace t81::ffi {

// FFI function classification levels
enum class FFIType : uint8_t {
    Deterministic = 0x01,  // Pure functions, same input -> same output
    Governed = 0x02,       // Stateful operations with audit trail
    Quarantined = 0x03     // High-risk operations requiring explicit override
};

// FFI call result status
enum class FFIResult : uint8_t {
    Success = 0x00,
    PolicyDenied = 0x01,
    TypeMismatch = 0x02,
    ResourceExhausted = 0x03,
    ExternalError = 0x04,
    QuarantineRequired = 0x05
};

// FFI function descriptor
struct FFIFunction {
    std::string name;
    FFIType type;
    std::string library_name;
    std::string version_hash;
    std::vector<std::string> param_types;
    std::string return_type;
    bool is_variadic;
    
    // Governance metadata
    std::string policy_reason;
    uint64_t resource_quota;
    std::vector<std::string> required_capabilities;
};

// FFI call context for governance
struct FFICallContext {
    std::string function_name;
    FFIType function_type;
    std::string caller_location;  // Source location in T81Lang
    std::vector<uint8_t> serialized_args;
    uint64_t call_id;
    uint32_t resource_cost;
};

// FFI call result with audit data
struct FFICallResult {
    FFIResult status;
    std::variant<uint64_t, int64_t, double, std::string, void*> result;
    std::string error_message;
    uint64_t execution_time_ns;
    std::vector<std::string> audit_events;
    std::string provenance_hash;
};

// FFI library registry
class FFILibraryRegistry {
public:
    static FFILibraryRegistry& instance();
    
    // Register a foreign library with governance checks
    t81::expected<void, std::string> register_library(
        const std::string& library_name,
        const std::string& version_hash,
        const std::vector<FFIFunction>& functions
    );
    
    // Lookup function by name
    t81::expected<FFIFunction*, std::string> lookup_function(
        const std::string& function_name
    );
    
    // List all registered functions
    std::vector<FFIFunction> list_functions() const;
    
    // Check if function is quarantined
    bool is_quarantined(const std::string& function_name) const;
    
private:
    FFILibraryRegistry() = default;
    std::vector<FFIFunction> registered_functions_;
    std::unordered_map<std::string, size_t> function_index_;
};

// FFI dispatcher with governance integration
class FFIDispatcher {
public:
    explicit FFIDispatcher(axion::Engine& policy_engine);
    
    // Execute FFI call with full governance
    t81::expected<FFICallResult, std::string> call(
        const FFICallContext& context
    );
    
    // Set global resource quotas
    void set_resource_quota(uint64_t max_time_ns, uint64_t max_memory);
    
    // Get audit trail for all FFI calls
    std::vector<FFICallResult> get_audit_trail() const;
    
private:
    axion::Engine& policy_engine_;
    uint64_t max_call_time_ns_;
    uint64_t max_memory_usage_;
    std::vector<FFICallResult> audit_trail_;
    std::unordered_map<std::string, FFICallResult> deterministic_cache_;
    
    // Internal governance checks
    t81::expected<void, std::string> check_policy_(
        const FFICallContext& context,
        const FFIFunction& function
    );
    
    // Execute the actual FFI call
    FFICallResult execute_call_(
        const FFICallContext& context,
        const FFIFunction& function
    );
    
    // Generate audit events
    void generate_audit_events_(
        const FFICallContext& context,
        const FFIFunction& function,
        const FFICallResult& result
    );
    
    // Helper functions
    std::string generate_cache_key_(
        const FFICallContext& context,
        const FFIFunction& function
    );
    std::optional<FFICallResult> lookup_cache_(const std::string& key);
    void cache_result_(
        const std::string& key,
        const FFICallResult& result
    );
    std::string compute_provenance_hash_(const std::string& data);
    FFICallResult call_foreign_function_(
        void* func_ptr,
        const FFICallContext& context,
        const FFIFunction& function
    );
};

} // namespace t81::ffi
