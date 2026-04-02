#!/usr/bin/env bash
# team_onboarding_checklist.sh - Team Onboarding Verification Script

set -euo pipefail

echo "=== Team Onboarding Verification Checklist ==="

ONBOARDING_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTION_DIR="$(dirname "$ONBOARDING_DIR")"
TMP_BASE="${TMPDIR:-/tmp}"
LOG_DIR="${LOG_DIR:-$(mktemp -d "$TMP_BASE/t81-onboarding-logs.XXXXXX")}"
LAUNCH_LOG="$LOG_DIR/launch.log"
VERIFY_LOG="$LOG_DIR/verification.log"
MONITOR_LOG="$LOG_DIR/monitor.log"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Checklist items
checklist=(
    "Environment Setup:Verify QEMU installation"
    "Environment Setup:Verify build tools"
    "Environment Setup:Verify source code access"
    "Basic Usage:Launch TernaryOS successfully"
    "Basic Usage:Run verification suite"
    "Basic Usage:Access production scripts"
    "Monitoring:Connect to QEMU monitor"
    "Monitoring:Use basic monitor commands"
    "Development:Make and test changes"
    "Development:Build and update artifacts"
    "Troubleshooting:Resolve common issues"
    "Documentation:Access key documentation"
)

resolve_timeout_cmd() {
    if command -v timeout &> /dev/null; then
        echo "timeout"
        return
    fi
    if command -v gtimeout &> /dev/null; then
        echo "gtimeout"
        return
    fi
    echo ""
}

TIMEOUT_CMD="$(resolve_timeout_cmd)"

cleanup() {
    if [[ -d "$LOG_DIR" ]]; then
        rm -rf "$LOG_DIR"
    fi
}

trap cleanup EXIT

# Function to check QEMU installation
check_qemu() {
    if command -v qemu-system-aarch64 &> /dev/null; then
        echo -e "${GREEN}✅${NC} QEMU is installed"
        qemu-system-aarch64 -version | head -1
        return 0
    else
        echo -e "${RED}❌${NC} QEMU is not installed"
        echo "Install with: brew install qemu (macOS) or apt-get install qemu-system-arm (Linux)"
        return 1
    fi
}

# Function to check build tools
check_build_tools() {
    local missing_tools=()
    
    for tool in cmake git make; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done
    
    if [ ${#missing_tools[@]} -eq 0 ]; then
        echo -e "${GREEN}✅${NC} All build tools are installed"
        return 0
    else
        echo -e "${RED}❌${NC} Missing tools: ${missing_tools[*]}"
        return 1
    fi
}

# Function to check source code access
check_source_access() {
    if [ -f "$PRODUCTION_DIR/../experimental/ternaryos/hal/qemu_slice6_bare_kernel.c" ]; then
        echo -e "${GREEN}✅${NC} Source code is accessible"
        return 0
    else
        echo -e "${RED}❌${NC} Source code not found"
        echo "Ensure you're in the correct directory structure"
        return 1
    fi
}

# Function to check build artifacts
check_build_artifacts() {
    if [ -f "$PRODUCTION_DIR/build/ternaryos/qemu_slice6/BOOTAA64.EFI" ]; then
        echo -e "${GREEN}✅${NC} Build artifacts are present"
        echo "BOOTAA64.EFI size: $(wc -c < "$PRODUCTION_DIR/build/ternaryos/qemu_slice6/BOOTAA64.EFI") bytes"
        return 0
    else
        echo -e "${RED}❌${NC} Build artifacts not found"
        echo "Run: cmake --build . --target t81_ternaryos_qemu_slice6_efi"
        return 1
    fi
}

# Function to check production scripts
check_production_scripts() {
    local scripts=("launch_production.sh" "verify_production.sh" "monitor_production.sh")
    local missing_scripts=()
    
    for script in "${scripts[@]}"; do
        if [ ! -f "$PRODUCTION_DIR/scripts/$script" ]; then
            missing_scripts+=("$script")
        fi
    done
    
    if [ ${#missing_scripts[@]} -eq 0 ]; then
        echo -e "${GREEN}✅${NC} All production scripts are present"
        return 0
    else
        echo -e "${RED}❌${NC} Missing scripts: ${missing_scripts[*]}"
        return 1
    fi
}

# Function to test basic launch
test_basic_launch() {
    echo "Testing basic launch (10 second timeout)..."

    if [ -z "$TIMEOUT_CMD" ]; then
        echo -e "${RED}❌${NC} Missing timeout command (install coreutils on macOS)"
        return 1
    fi

    "$TIMEOUT_CMD" 10 "$PRODUCTION_DIR/scripts/launch_production.sh" > "$LAUNCH_LOG" 2>&1 &
    LAUNCH_PID=$!
    
    sleep 3
    
    if kill -0 "$LAUNCH_PID" 2>/dev/null; then
        kill "$LAUNCH_PID" 2>/dev/null || true
        wait "$LAUNCH_PID" 2>/dev/null || true
        
        if grep -q "qemu-system-aarch64" "$LAUNCH_LOG"; then
            echo -e "${GREEN}✅${NC} Basic launch test passed"
            return 0
        else
            echo -e "${RED}❌${NC} QEMU did not start properly"
            echo "Check $LAUNCH_LOG for details"
            return 1
        fi
    else
        echo -e "${RED}❌${NC} Launch failed immediately"
        echo "Check $LAUNCH_LOG for details"
        return 1
    fi
}

# Function to test verification suite
test_verification_suite() {
    echo "Testing verification suite..."

    if "$PRODUCTION_DIR/scripts/verify_production.sh" > "$VERIFY_LOG" 2>&1; then
        echo -e "${GREEN}✅${NC} Verification suite passed"
        return 0
    else
        echo -e "${RED}❌${NC} Verification suite failed"
        echo "Check $VERIFY_LOG for details"
        return 1
    fi
}

# Function to test monitor connection
test_monitor_connection() {
    echo "Testing monitor connection..."

    if [ -z "$TIMEOUT_CMD" ]; then
        echo -e "${RED}❌${NC} Missing timeout command (install coreutils on macOS)"
        return 1
    fi
    if ! command -v nc &> /dev/null; then
        echo -e "${RED}❌${NC} Missing nc (netcat); required for monitor connectivity checks"
        return 1
    fi

    # Start QEMU with monitor
    "$TIMEOUT_CMD" 8 "$PRODUCTION_DIR/scripts/monitor_production.sh" > "$MONITOR_LOG" 2>&1 &
    MONITOR_PID=$!
    
    sleep 3
    
    # Test monitor connection
    if echo "info version" | nc localhost 1234 2>/dev/null | grep -q "QEMU"; then
        echo -e "${GREEN}✅${NC} Monitor connection test passed"
        
        # Stop monitor
        echo "quit" | nc localhost 1234 2>/dev/null || true
        kill "$MONITOR_PID" 2>/dev/null || true
        wait "$MONITOR_PID" 2>/dev/null || true
        
        return 0
    else
        echo -e "${RED}❌${NC} Monitor connection failed"
        echo "Check $MONITOR_LOG for details"
        
        # Cleanup
        kill "$MONITOR_PID" 2>/dev/null || true
        wait "$MONITOR_PID" 2>/dev/null || true
        
        return 1
    fi
}

# Function to check documentation access
check_documentation() {
    local docs=("README.md" "TEAM_ONBOARDING.md" "QEMU_INTEGRATION_ACHIEVEMENTS.md")
    local missing_docs=()
    
    for doc in "${docs[@]}"; do
        if [ ! -f "$PRODUCTION_DIR/docs/$doc" ] && [ ! -f "$PRODUCTION_DIR/$doc" ]; then
            missing_docs+=("$doc")
        fi
    done
    
    if [ ${#missing_docs[@]} -eq 0 ]; then
        echo -e "${GREEN}✅${NC} Key documentation is accessible"
        return 0
    else
        echo -e "${RED}❌${NC} Missing documentation: ${missing_docs[*]}"
        return 1
    fi
}

# Main execution
main() {
    echo "Team Onboarding Verification Checklist"
    echo "======================================"
    echo ""
    echo "Logs will be written under: $LOG_DIR"
    echo ""
    
    local passed=0
    local total=0
    
    # Environment Setup checks
    echo -e "${YELLOW}Environment Setup${NC}"
    echo "------------------"
    
    if check_qemu; then ((passed++)); fi
    ((total++))
    
    if check_build_tools; then ((passed++)); fi
    ((total++))
    
    if check_source_access; then ((passed++)); fi
    ((total++))
    
    echo ""
    
    # Basic Usage checks
    echo -e "${YELLOW}Basic Usage${NC}"
    echo "------------"
    
    if check_build_artifacts; then ((passed++)); fi
    ((total++))
    
    if check_production_scripts; then ((passed++)); fi
    ((total++))
    
    if test_verification_suite; then ((passed++)); fi
    ((total++))
    
    echo ""
    
    # Advanced Usage checks
    echo -e "${YELLOW}Advanced Usage${NC}"
    echo "---------------"
    
    if test_basic_launch; then ((passed++)); fi
    ((total++))
    
    if test_monitor_connection; then ((passed++)); fi
    ((total++))
    
    echo ""
    
    # Documentation checks
    echo -e "${YELLOW}Documentation${NC}"
    echo "-------------"
    
    if check_documentation; then ((passed++)); fi
    ((total++))
    
    echo ""
    echo "======================================"
    echo "Onboarding Checklist Results"
    echo "======================================"
    echo "Passed: $passed/$total checks"
    
    if [ $passed -eq $total ]; then
        echo -e "${GREEN}🎉 ALL CHECKS PASSED!${NC}"
        echo "You're ready to start contributing to the QEMU TernaryOS integration!"
        echo ""
        echo "Next steps:"
        echo "1. Read TEAM_ONBOARDING.md for detailed guidance"
        echo "2. Try making a small change and testing it"
        echo "3. Join team discussions and code reviews"
        return 0
    else
        echo -e "${YELLOW}⚠️  SOME CHECKS FAILED${NC}"
        echo "Please resolve the failed checks before proceeding."
        echo ""
        echo "Common solutions:"
        echo "- Install missing tools (QEMU, CMake, Git)"
        echo "- Build the project: cmake --build . --target t81_ternaryos_qemu_slice6_efi"
        echo "- Check directory structure"
        echo "- Review documentation for troubleshooting"
        return 1
    fi
}

# Run main function
main "$@"
