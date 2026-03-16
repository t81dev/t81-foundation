#include <gtest/gtest.h>
#include "t81/ffi/ffi.hpp"
#include "t81/axion/engine.hpp"
#include "t81/vm/vm.hpp"

namespace t81::test {

class FFIBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize policy engine for testing
        policy_engine_ = std::make_unique<t81::axion::Engine>();
        
        // Initialize FFI subsystem
        t81::vm::initialize_ffi_subsystem(*policy_engine_);
        
        // Register test functions
        register_test_functions_();
    }
    
    void TearDown() override {
        // Cleanup
        test_functions_.clear();
    }
    
    std::unique_ptr<t81::axion::Engine> policy_engine_;
    std::vector<t81::ffi::FFIFunction> test_functions_;
    
private:
    void register_test_functions_() {
        using namespace t81::ffi;
        
        // Deterministic function
        test_functions_.push_back({
            .name = "deterministic_add",
            .type = FFIType::Deterministic,
            .library_name = "libtest",
            .version_hash = "abc123",
            .param_types = {"uint64_t", "uint64_t"},
            .return_type = "uint64_t",
            .is_variadic = false,
            .policy_reason = "Pure mathematical function",
            .resource_quota = 100,
            .required_capabilities = {}
        });
        
        // Governed function
        test_functions_.push_back({
            .name = "governed_file_read",
            .type = FFIType::Governed,
            .library_name = "libtest",
            .version_hash = "abc123",
            .param_types = {"string", "int"},
            .return_type = "int",
            .is_variadic = false,
            .policy_reason = "File I/O requires audit",
            .resource_quota = 1000,
            .required_capabilities = {"file_read"}
        });
        
        // Quarantined function
        test_functions_.push_back({
            .name = "dangerous_system_call",
            .type = FFIType::Quarantined,
            .library_name = "libtest",
            .version_hash = "abc123",
            .param_types = {"pointer"},
            .return_type = "void",
            .is_variadic = false,
            .policy_reason = "Direct system access is quarantined",
            .resource_quota = 0,
            .required_capabilities = {"system_access"}
        });
        
        // Register with FFI registry
        auto& registry = t81::ffi::FFILibraryRegistry::instance();
        auto result = registry.register_library("libtest", "abc123", test_functions_);
        ASSERT_TRUE(result) << "Failed to register test library: " << result.error();
    }
};

TEST_F(FFIBasicTest, DeterministicFunctionCall) {
    using namespace t81::ffi;
    
    // Test deterministic function call
    FFICallContext context{
        .function_name = "deterministic_add",
        .function_type = FFIType::Deterministic,
        .caller_location = "test_location",
        .serialized_args = {10, 20},  // Serialized arguments
        .call_id = 1,
        .resource_cost = 100
    };
    
    auto dispatcher = t81::vm::get_ffi_dispatcher();
    ASSERT_NE(dispatcher, nullptr) << "FFI dispatcher not initialized";
    
    auto result = dispatcher->call(context);
    
    EXPECT_EQ(result.status, FFIResult::Success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_GT(result.execution_time_ns, 0);
    EXPECT_FALSE(result.audit_events.empty());
    EXPECT_FALSE(result.provenance_hash.empty());
    
    // Check audit events
    bool has_call_event = false;
    bool has_type_event = false;
    for (const auto& event : result.audit_events) {
        if (event.find("FFICall_deterministic_add") != std::string::npos) {
            has_call_event = true;
        }
        if (event.find("FunctionType_0") != std::string::npos) {
            has_type_event = true;
        }
    }
    EXPECT_TRUE(has_call_event);
    EXPECT_TRUE(has_type_event);
}

TEST_F(FFIBasicTest, GovernedFunctionCall) {
    using namespace t81::ffi;
    
    FFICallContext context{
        .function_name = "governed_file_read",
        .function_type = FFIType::Governed,
        .caller_location = "test_location",
        .serialized_args = {},  // Would contain serialized file path and mode
        .call_id = 2,
        .resource_cost = 1000
    };
    
    auto dispatcher = t81::vm::get_ffi_dispatcher();
    auto result = dispatcher->call(context);
    
    // Should succeed for governed functions (assuming policy allows it)
    EXPECT_EQ(result.status, FFIResult::Success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_GT(result.execution_time_ns, 0);
    
    // Check for governance-specific audit events
    bool has_governance_events = false;
    for (const auto& event : result.audit_events) {
        if (event.find("FunctionType_1") != std::string::npos) {
            has_governance_events = true;
        }
    }
    EXPECT_TRUE(has_governance_events);
}

TEST_F(FFIBasicTest, QuarantinedFunctionCall) {
    using namespace t81::ffi;
    
    FFICallContext context{
        .function_name = "dangerous_system_call",
        .function_type = FFIType::Quarantined,
        .caller_location = "test_location",
        .serialized_args = {},
        .call_id = 3,
        .resource_cost = 0
    };
    
    auto dispatcher = t81::vm::get_ffi_dispatcher();
    auto result = dispatcher->call(context);
    
    // Quarantined functions should be denied
    EXPECT_EQ(result.status, FFIResult::QuarantineRequired);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_EQ(result.execution_time_ns, 0);
    
    // Check quarantine-specific audit events
    bool has_quarantine_event = false;
    for (const auto& event : result.audit_events) {
        if (event.find("QuarantineCheck") != std::string::npos) {
            has_quarantine_event = true;
        }
    }
    EXPECT_TRUE(has_quarantine_event);
}

TEST_F(FFIBasicTest, UnknownFunctionCall) {
    using namespace t81::ffi;
    
    FFICallContext context{
        .function_name = "nonexistent_function",
        .function_type = FFIType::Deterministic,
        .caller_location = "test_location",
        .serialized_args = {},
        .call_id = 4,
        .resource_cost = 100
    };
    
    auto dispatcher = t81::vm::get_ffi_dispatcher();
    auto result = dispatcher->call(context);
    
    // Unknown functions should be denied
    EXPECT_EQ(result.status, FFIResult::PolicyDenied);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.error_message.find("not found") != std::string::npos);
    EXPECT_EQ(result.execution_time_ns, 0);
}

TEST_F(FFIBasicTest, ResourceQuotaExceeded) {
    using namespace t81::ffi;
    
    // Set very low resource quota
    auto dispatcher = t81::vm::get_ffi_dispatcher();
    dispatcher->set_resource_quota(100, 1024);  // 100ns, 1KB
    
    FFICallContext context{
        .function_name = "deterministic_add",
        .function_type = FFIType::Deterministic,
        .caller_location = "test_location",
        .serialized_args = {10, 20},
        .call_id = 5,
        .resource_cost = 1000  // Exceeds quota
    };
    
    auto result = dispatcher->call(context);
    
    // Should be denied due to resource quota
    EXPECT_EQ(result.status, FFIResult::ResourceExhausted);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.error_message.find("quota") != std::string::npos);
}

TEST_F(FFIBasicTest, FunctionLookup) {
    using namespace t81::ffi;
    
    auto& registry = t81::ffi::FFILibraryRegistry::instance();
    
    // Test successful lookup
    auto func_result = registry.lookup_function("deterministic_add");
    ASSERT_TRUE(func_result) << "Function lookup failed: " << func_result.error();
    
    auto* func = func_result.value();
    EXPECT_EQ(func->name, "deterministic_add");
    EXPECT_EQ(func->type, FFIType::Deterministic);
    EXPECT_EQ(func->library_name, "libtest");
    
    // Test unsuccessful lookup
    auto missing_result = registry.lookup_function("nonexistent_function");
    EXPECT_FALSE(missing_result);
    EXPECT_TRUE(missing_result.error().find("not found") != std::string::npos);
}

TEST_F(FFIBasicTest, LibraryRegistration) {
    using namespace t81::ffi;
    
    auto& registry = t81::ffi::FFILibraryRegistry::instance();
    
    // Test duplicate registration prevention
    std::vector<FFIFunction> duplicate_functions = {{
        .name = "deterministic_add",
        .type = FFIType::Deterministic,
        .library_name = "libtest",
        .version_hash = "def456",  // Different hash
        .param_types = {"uint64_t", "uint64_t"},
        .return_type = "uint64_t",
        .is_variadic = false,
        .policy_reason = "Test duplicate",
        .resource_quota = 100,
        .required_capabilities = {}
    }};
    
    auto duplicate_result = registry.register_library("libtest", "def456", duplicate_functions);
    EXPECT_FALSE(duplicate_result) << "Duplicate registration should fail";
    EXPECT_TRUE(duplicate_result.error().find("already registered") != std::string::npos);
    
    // Test quarantine status checking
    EXPECT_TRUE(registry.is_quarantined("dangerous_system_call"));
    EXPECT_FALSE(registry.is_quarantined("deterministic_add"));
    EXPECT_TRUE(registry.is_quarantined("nonexistent_function"));  // Unknown functions quarantined by default
}

TEST_F(FFIBasicTest, AuditTrailIntegrity) {
    using namespace t81::ffi;
    
    auto dispatcher = t81::vm::get_ffi_dispatcher();
    
    // Make several FFI calls
    std::vector<FFICallContext> contexts = {
        {
            .function_name = "deterministic_add",
            .function_type = FFIType::Deterministic,
            .caller_location = "test_1",
            .serialized_args = {1, 2},
            .call_id = 10,
            .resource_cost = 100
        },
        {
            .function_name = "governed_file_read",
            .function_type = FFIType::Governed,
            .caller_location = "test_2",
            .serialized_args = {},
            .call_id = 11,
            .resource_cost = 500
        },
        {
            .function_name = "dangerous_system_call",
            .function_type = FFIType::Quarantined,
            .caller_location = "test_3",
            .serialized_args = {},
            .call_id = 12,
            .resource_cost = 0
        }
    };
    
    // Execute calls
    for (const auto& context : contexts) {
        auto result = dispatcher->call(context);
        // Results should be valid (even if denied for quarantine)
        EXPECT_NE(result.status, FFIResult::TypeMismatch);
    }
    
    // Check audit trail
    auto audit_trail = dispatcher->get_audit_trail();
    EXPECT_EQ(audit_trail.size(), contexts.size());
    
    // Verify audit trail contains expected events
    for (size_t i = 0; i < audit_trail.size(); ++i) {
        const auto& result = audit_trail[i];
        EXPECT_FALSE(result.audit_events.empty());
        EXPECT_FALSE(result.provenance_hash.empty());
        
        // Check that call ID matches
        bool has_matching_call_id = false;
        for (const auto& event : result.audit_events) {
            if (event.find(std::to_string(contexts[i].call_id)) != std::string::npos) {
                has_matching_call_id = true;
                break;
            }
        }
        EXPECT_TRUE(has_matching_call_id) << "Call ID " << contexts[i].call_id << " not found in audit events";
    }
}

} // namespace t81::test
