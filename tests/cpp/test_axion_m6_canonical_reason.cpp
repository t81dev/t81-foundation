// Test for AX-M6 verbatim reason-string concatenation
// Verifies that StructuredEvent::to_canonical_reason_string() produces correct format

#include <iostream>
#include <string>
#include "t81/axion/reasons.hpp"
#include "t81/axion/verdict.hpp"

using namespace t81::axion;

int main() {
    std::cout << "=== AX-M6 Canonical Reason String Test ===" << std::endl;
    
    int test_count = 0;
    int passed_count = 0;
    
    // Test 1: Already canonical format should pass through
    {
        test_count++;
        StructuredEvent event;
        event.reason = "segment=stack addr=42 action=Read";
        event.storage_class = "heap";
        event.handle_id = 128;
        event.event_type = "Write";
        
        std::string canonical = event.to_canonical_reason_string();
        
        // Should pass through unchanged if already in canonical format
        if (canonical == "segment=stack addr=42 action=Read") {
            std::cout << "✓ Test 1 PASSED: Canonical format pass-through" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ Test 1 FAILED: Expected 'segment=stack addr=42 action=Read', got '" << canonical << "'" << std::endl;
        }
    }
    
    // Test 2: Construct canonical from fields
    {
        test_count++;
        StructuredEvent event;
        event.storage_class = "tensor";
        event.handle_id = 256;
        event.event_type = "Allocate";
        
        std::string canonical = event.to_canonical_reason_string();
        
        // Should construct canonical format from individual fields
        if (canonical == "segment=tensor addr=256 action=Allocate") {
            std::cout << "✓ Test 2 PASSED: Construct from fields" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ Test 2 FAILED: Expected 'segment=tensor addr=256 action=Allocate', got '" << canonical << "'" << std::endl;
        }
    }
    
    // Test 3: Partial fields fallback
    {
        test_count++;
        StructuredEvent event;
        event.storage_class = "meta";
        event.reason_code = "Write";
        
        std::string canonical = event.to_canonical_reason_string();
        
        // Should use reason_code if event_type is empty
        if (canonical == "segment=meta action=Write") {
            std::cout << "✓ Test 3 PASSED: Partial fields fallback" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ Test 3 FAILED: Expected 'segment=meta action=Write', got '" << canonical << "'" << std::endl;
        }
    }
    
    // Test 4: Unknown fields fallback
    {
        test_count++;
        StructuredEvent event;
        
        std::string canonical = event.to_canonical_reason_string();
        
        // Should use "unknown" for missing fields
        if (canonical == "segment=unknown action=unknown") {
            std::cout << "✓ Test 4 PASSED: Unknown fields fallback" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ Test 4 FAILED: Expected 'segment=unknown action=unknown', got '" << canonical << "'" << std::endl;
        }
    }
    
    // Test 5: Extract address from reason
    {
        test_count++;
        StructuredEvent event;
        event.reason = "some operation addr=1234 other info";
        event.storage_class = "stack";
        
        std::string canonical = event.to_canonical_reason_string();
        
        // Should extract address from existing reason
        if (canonical == "segment=stack addr=1234 action=unknown") {
            std::cout << "✓ Test 5 PASSED: Extract address from reason" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ Test 5 FAILED: Expected 'segment=stack addr=1234 action=unknown', got '" << canonical << "'" << std::endl;
        }
    }
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Passed: " << passed_count << "/" << test_count << " tests" << std::endl;
    
    if (passed_count == test_count) {
        std::cout << "✓ ALL TESTS PASSED - AX-M6 implementation working correctly!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
