# CanonFS Interchange Refactoring Plan

## Current State Analysis

### Current Architecture
```
CLI Layer (tools/cli/driver.cpp)
    ├── Calls import_path() / export_ref() directly
    ├── Renders JSON responses using interchange module
    └── Handles policy evaluation and error formatting

Core Interchange Module (fs/canonfs_interchange_ops.cpp)
    ├── Implements import_path() and export_ref()
    ├── Contains business logic for CanonFS operations
    └── Provides JSON rendering functions

JSON Rendering Module (fs/canonfs_interchange.cpp)
    ├── render_import_result() and render_export_result()
    ├── JSON schema validation functions
    └── Utility functions for JSON formatting
```

### Issues with Current Design
1. **Tight Coupling** - CLI directly calls core interchange functions
2. **Limited Reusability** - JSON rendering tied to CLI-specific output
3. **Testing Complexity** - Hard to test interchange logic independently
4. **Code Duplication** - Similar patterns in import/export flows

## Refactoring Strategy

### Phase 1: Extract Core Interchange Library

#### 1.1 Create Reusable Interchange Engine
```cpp
// New: include/t81/canonfs/interchange_engine.hpp
class CanonFSInterchangeEngine {
public:
    ImportOutcome import(const ImportRequest& request);
    ExportOutcome export(const ExportRequest& request);
    
    // Policy evaluation
    std::optional<InterchangePolicyDecision> evaluate_policy(
        const PolicyContext& context);
        
    // Evidence collection
    void collect_evidence(const OperationContext& context);
};
```

#### 1.2 Define Request/Response Structures
```cpp
// New: include/t81/canonfs/interchange_types.hpp
struct ImportRequest {
    std::filesystem::path input_path;
    std::filesystem::path canonfs_root;
    InterchangePolicyProfile policy_profile;
    InterchangePolicyEvaluator policy_evaluator;
    std::optional<std::string> policy_file;
};

struct ExportRequest {
    std::string canonical_hash;
    std::filesystem::path output_path;
    std::filesystem::path canonfs_root;
    InterchangePolicyProfile policy_profile;
    InterchangePolicyEvaluator policy_evaluator;
    std::optional<std::string> policy_file;
};
```

#### 1.3 Separate JSON Rendering
```cpp
// New: include/t81/canonfs/json_renderer.hpp
class InterchangeJSONRenderer {
public:
    std::string render_import_result(const ImportOutcome& outcome);
    std::string render_export_result(const ExportOutcome& outcome);
    
    // Validation
    bool validate_import_schema(const std::string& json);
    bool validate_export_schema(const std::string& json);
};
```

### Phase 2: Refactor CLI Integration

#### 2.1 Update CLI to Use New Engine
```cpp
// tools/cli/driver.cpp - Updated import/export functions
int canonfs_import(const fs::path& input, const fs::path& canonfs_root,
                   const std::optional<fs::path>& policy_path,
                   const std::optional<std::string>& policy_profile, bool as_json) {
    
    // Create request object
    ImportRequest request{input, canonfs_root, policy_profile, policy_evaluator, policy_path};
    
    // Use new engine
    CanonFSInterchangeEngine engine;
    auto outcome = engine.import(request);
    
    // Use new renderer
    if (as_json) {
        InterchangeJSONRenderer renderer;
        auto json = renderer.render_import_result(outcome);
        // Output JSON with validation
        if (!renderer.validate_import_schema(json)) {
            error("canonfs import: internal JSON validation failed");
            return 1;
        }
        std::cout << json;
    } else {
        // Human-readable output
        format_human_readable_import_result(outcome);
    }
    
    return outcome.ok() ? 0 : 1;
}
```

#### 2.2 Maintain Backward Compatibility
- Keep existing function signatures for external users
- Add adapter functions for gradual migration
- Ensure CLI behavior remains identical

### Phase 3: Enhanced Testing

#### 3.1 Unit Test Interchange Engine
```cpp
// tests/cpp/canonfs_interchange_engine_test.cpp
class CanonFSInterchangeEngineTest : public ::testing::Test {
protected:
    std::unique_ptr<CanonFSInterchangeEngine> engine_;
    
    void SetUp() override {
        engine_ = std::make_unique<CanonFSInterchangeEngine>();
    }
};

TEST_F(CanonFSInterchangeEngineTest, ImportWithPolicy) {
    ImportRequest request;
    request.policy_profile = InterchangePolicyProfile::DenyAll;
    
    auto outcome = engine_->import(request);
    EXPECT_FALSE(outcome.ok());
    EXPECT_FALSE(outcome.errors.empty());
}
```

#### 3.2 Integration Tests
```cpp
// tests/cpp/canonfs_integration_test.cpp
class CanonFSIntegrationTest : public ::testing::Test {
    // Test CLI + Engine integration
    // Verify backward compatibility
    // Test error handling paths
};
```

## Implementation Benefits

### 1. Improved Separation of Concerns
- **CLI Layer**: Argument parsing, user interface, output formatting
- **Engine Layer**: Business logic, policy evaluation, file operations
- **Renderer Layer**: JSON formatting, schema validation

### 2. Enhanced Reusability
- Engine can be used by multiple frontends (CLI, library, web API)
- JSON renderer can be used independently
- Policy evaluation separated from file operations

### 3. Better Testing
- Unit tests for engine logic without CLI dependencies
- Integration tests for end-to-end scenarios
- Mock-friendly interfaces for testing

### 4. Future Extensibility
- Easy to add new interchange formats
- Pluggable policy evaluators
- Configurable JSON renderers

## Migration Path

### Step 1: Create New Interfaces
1. Create `interchange_engine.hpp` with engine interface
2. Create `interchange_types.hpp` with request/response types
3. Create `json_renderer.hpp` with rendering interface

### Step 2: Implement Engine
1. Move core logic from `canonfs_interchange_ops.cpp` to new engine
2. Implement policy evaluation separation
3. Add evidence collection hooks

### Step 3: Update CLI
1. Modify `driver.cpp` to use new engine
2. Update JSON rendering to use new renderer
3. Maintain existing CLI behavior

### Step 4: Add Tests
1. Create unit tests for engine
2. Create integration tests for CLI
3. Update existing tests to work with new structure

### Step 5: Documentation
1. Update API documentation
2. Add migration guide for external users
3. Update examples to use new interfaces

## Risk Mitigation

### Technical Risks
- **Breaking Changes**: Minimize by maintaining function signatures
- **Performance**: Ensure no regression in operation speed
- **Complexity**: Keep interfaces simple and well-documented

### Mitigation Strategies
- **Adaptor Pattern**: Provide compatibility layer during transition
- **Comprehensive Testing**: Test all existing use cases
- **Gradual Migration**: Allow old and new code to coexist
- **Documentation**: Clear migration path for external users

## Success Metrics

### Code Quality
- Reduced coupling between CLI and core logic
- Improved testability of interchange operations
- Better separation of concerns

### Functionality
- 100% backward compatibility maintained
- Enhanced error reporting and validation
- Improved extensibility for future features

### Developer Experience
- Clearer API boundaries
- Better debugging capabilities
- Easier unit testing

This refactoring will make CanonFS interchange more maintainable, testable, and extensible while preserving all existing functionality.
