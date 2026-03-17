#!/bin/bash
# production_qemu_setup.sh - Production QEMU TernaryOS Setup Script

set -e

echo "=== Production QEMU TernaryOS Setup ==="

# Configuration
PRODUCTION_DIR="/opt/ternaryos-qemu"
BUILD_DIR="$PRODUCTION_DIR/build"
SCRIPTS_DIR="$PRODUCTION_DIR/scripts"
DOCS_DIR="$PRODUCTION_DIR/docs"
LOGS_DIR="$PRODUCTION_DIR/logs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root for security reasons"
        exit 1
    fi
}

# Function to check dependencies
check_dependencies() {
    print_header "Checking Dependencies"
    
    local missing_deps=()
    
    # Check for required commands
    for cmd in qemu-system-aarch64 cmake git make; do
        if ! command -v "$cmd" &> /dev/null; then
            missing_deps+=("$cmd")
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Please install missing dependencies and try again"
        exit 1
    fi
    
    print_status "All dependencies found"
}

# Function to create directory structure
create_directories() {
    print_header "Creating Directory Structure"
    
    mkdir -p "$PRODUCTION_DIR"
    mkdir -p "$BUILD_DIR"
    mkdir -p "$SCRIPTS_DIR"
    mkdir -p "$DOCS_DIR"
    mkdir -p "$LOGS_DIR"
    
    print_status "Directory structure created"
}

# Function to setup T81 Foundation source
setup_source() {
    print_header "Setting up T81 Foundation Source"
    
    if [ ! -d "$PRODUCTION_DIR/t81-foundation" ]; then
        print_status "Cloning T81 Foundation repository"
        git clone https://github.com/t81-foundation/t81-foundation.git "$PRODUCTION_DIR/t81-foundation"
    else
        print_status "Updating existing repository"
        cd "$PRODUCTION_DIR/t81-foundation"
        git pull origin main
    fi
    
    print_status "Source code ready"
}

# Function to build production binaries
build_production() {
    print_header "Building Production Binaries"
    
    cd "$PRODUCTION_DIR/t81-foundation"
    
    # Clean build
    rm -rf build
    mkdir build
    cd build
    
    # Configure for production
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # Build all QEMU targets
    print_status "Building QEMU targets..."
    cmake --build . --target t81_ternaryos_qemu_slice6_snapshot
    cmake --build . --target t81_ternaryos_qemu_slice6_efi
    cmake --build . --target t81_ternaryos_qemu_slice6_snapshot_data
    
    print_status "Production build completed"
}

# Function to create production scripts
create_production_scripts() {
    print_header "Creating Production Scripts"
    
    # Production verification script
    cat > "$SCRIPTS_DIR/verify_production.sh" << 'EOF'
#!/bin/bash
# Production verification script

set -e

PRODUCTION_DIR="/opt/ternaryos-qemu"
BUILD_DIR="$PRODUCTION_DIR/build"

echo "=== Production Verification ==="

# Check build artifacts
if [ ! -f "$BUILD_DIR/ternaryos/qemu_slice6/BOOTAA64.EFI" ]; then
    echo "ERROR: BOOTAA64.EFI not found"
    exit 1
fi

# Run verification suite
cd "$PRODUCTION_DIR/t81-foundation"
./experimental/ternaryos/automated_verification.sh

# Check results
if grep -q "VERIFICATION SUCCESSFUL" /tmp/ternaryos_verification/verification_summary.txt; then
    echo "✅ Production verification PASSED"
    exit 0
else
    echo "❌ Production verification FAILED"
    exit 1
fi
EOF

    # Production launch script
    cat > "$SCRIPTS_DIR/launch_production.sh" << 'EOF'
#!/bin/bash
# Production launch script

set -e

PRODUCTION_DIR="/opt/ternaryos-qemu"
BUILD_DIR="$PRODUCTION_DIR/build"

echo "=== Launching Production TernaryOS ==="

# Build disk image
cd "$PRODUCTION_DIR/t81-foundation"
./experimental/ternaryos/scripts/build_qemu_slice6_artifact.sh "$BUILD_DIR" "$PRODUCTION_DIR/production_image"

# Launch QEMU
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file="$PRODUCTION_DIR/production_image/qemu_slice6_guest.img",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -m 256M -smp 2
EOF

    # Production monitoring script
    cat > "$SCRIPTS_DIR/monitor_production.sh" << 'EOF'
#!/bin/bash
# Production monitoring script

set -e

PRODUCTION_DIR="/opt/ternaryos-qemu"
BUILD_DIR="$PRODUCTION_DIR/build"

echo "=== Production Monitoring ==="

# Start QEMU with monitor
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file="$PRODUCTION_DIR/production_image/qemu_slice6_guest.img",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null \
    -monitor telnet:localhost:1234,server,nowait,wait &

QEMU_PID=$!

echo "QEMU started with PID: $QEMU_PID"
echo "Monitor available on localhost:1234"

# Wait for user input
read -p "Press Enter to stop QEMU..."

# Stop QEMU
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

echo "QEMU stopped"
EOF

    # Make scripts executable
    chmod +x "$SCRIPTS_DIR"/*.sh
    
    print_status "Production scripts created"
}

# Function to setup documentation
setup_documentation() {
    print_header "Setting up Documentation"
    
    # Copy documentation
    cp "$PRODUCTION_DIR/t81-foundation/experimental/ternaryos/QEMU_INTEGRATION_ACHIEVEMENTS.md" "$DOCS_DIR/"
    cp "$PRODUCTION_DIR/t81-foundation/experimental/ternaryos/QEMU_DEVELOPMENT_WORKFLOW.md" "$DOCS_DIR/"
    cp "$PRODUCTION_DIR/t81-foundation/experimental/ternaryos/QEMU_SERIAL_CAPTURE_ANALYSIS.md" "$DOCS_DIR/"
    cp "$PRODUCTION_DIR/t81-foundation/experimental/ternaryos/ALTERNATIVE_VERIFICATION_RESULTS.md" "$DOCS_DIR/"
    
    # Create production README
    cat > "$DOCS_DIR/README.md" << 'EOF'
# Production QEMU TernaryOS

## Overview
This is a production-ready QEMU TernaryOS installation for deterministic ternary computing development.

## Quick Start

### 1. Verification
```bash
/opt/ternaryos-qemu/scripts/verify_production.sh
```

### 2. Launch
```bash
/opt/ternaryos-qemu/scripts/launch_production.sh
```

### 3. Monitor
```bash
/opt/ternaryos-qemu/scripts/monitor_production.sh
```

## Directory Structure
- `/opt/ternaryos-qemu/build` - Build artifacts
- `/opt/ternaryos-qemu/scripts` - Production scripts
- `/opt/ternaryos-qemu/docs` - Documentation
- `/opt/ternaryos-qemu/logs` - Log files

## Support
See documentation in `/opt/ternaryos-qemu/docs/` for detailed information.
EOF

    print_status "Documentation setup completed"
}

# Function to setup logging
setup_logging() {
    print_header "Setting up Logging"
    
    # Create log rotation configuration
    cat > "$SCRIPTS_DIR/setup_log_rotation.sh" << 'EOF'
#!/bin/bash
# Setup log rotation for production

LOG_DIR="/opt/ternaryos-qemu/logs"
LOGROTATE_CONF="/etc/logrotate.d/ternaryos-qemu"

# Create logrotate configuration
sudo tee "$LOGROTATE_CONF" > /dev/null << 'EOL'
/opt/ternaryos-qemu/logs/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 $USER $USER
}
EOL

echo "Log rotation configured"
EOF

    chmod +x "$SCRIPTS_DIR/setup_log_rotation.sh"
    
    print_status "Logging setup completed"
}

# Function to create production tests
create_production_tests() {
    print_header "Creating Production Tests"
    
    # Production test suite
    cat > "$SCRIPTS_DIR/run_production_tests.sh" << 'EOF'
#!/bin/bash
# Production test suite

set -e

PRODUCTION_DIR="/opt/ternaryos-qemu"
TEST_LOG="$PRODUCTION_DIR/logs/production_test_$(date +%Y%m%d_%H%M%S).log"

echo "=== Production Test Suite ===" | tee "$TEST_LOG"

# Test 1: Build verification
echo "Test 1: Build verification" | tee -a "$TEST_LOG"
if [ -f "$PRODUCTION_DIR/build/ternaryos/qemu_slice6/BOOTAA64.EFI" ]; then
    echo "✅ Build artifacts present" | tee -a "$TEST_LOG"
else
    echo "❌ Build artifacts missing" | tee -a "$TEST_LOG"
    exit 1
fi

# Test 2: Script verification
echo "Test 2: Script verification" | tee -a "$TEST_LOG"
for script in verify_production.sh launch_production.sh monitor_production.sh; do
    if [ -f "$PRODUCTION_DIR/scripts/$script" ]; then
        echo "✅ $script present" | tee -a "$TEST_LOG"
    else
        echo "❌ $script missing" | tee -a "$TEST_LOG"
        exit 1
    fi
done

# Test 3: Automated verification
echo "Test 3: Automated verification" | tee -a "$TEST_LOG"
cd "$PRODUCTION_DIR/t81-foundation"
if ./experimental/ternaryos/automated_verification.sh > /dev/null 2>&1; then
    echo "✅ Automated verification passed" | tee -a "$TEST_LOG"
else
    echo "❌ Automated verification failed" | tee -a "$TEST_LOG"
    exit 1
fi

# Test 4: Documentation verification
echo "Test 4: Documentation verification" | tee -a "$TEST_LOG"
for doc in README.md QEMU_INTEGRATION_ACHIEVEMENTS.md QEMU_DEVELOPMENT_WORKFLOW.md; do
    if [ -f "$PRODUCTION_DIR/docs/$doc" ]; then
        echo "✅ $doc present" | tee -a "$TEST_LOG"
    else
        echo "❌ $doc missing" | tee -a "$TEST_LOG"
        exit 1
    fi
done

echo "✅ All production tests passed" | tee -a "$TEST_LOG"
echo "Test log: $TEST_LOG"
EOF

    chmod +x "$SCRIPTS_DIR/run_production_tests.sh"
    
    print_status "Production tests created"
}

# Function to setup environment
setup_environment() {
    print_header "Setting up Environment"
    
    # Create environment script
    cat > "$PRODUCTION_DIR/setup_env.sh" << 'EOF'
#!/bin/bash
# Production environment setup

export TERNARYOS_QEMU_HOME="/opt/ternaryos-qemu"
export PATH="$TERNARYOS_QEMU_HOME/scripts:$PATH"

echo "Production environment configured"
echo "TernaryOS QEMU Home: $TERNARYOS_QEMU_HOME"
EOF

    chmod +x "$PRODUCTION_DIR/setup_env.sh"
    
    print_status "Environment setup completed"
}

# Function to verify installation
verify_installation() {
    print_header "Verifying Installation"
    
    # Check directory structure
    if [ ! -d "$PRODUCTION_DIR" ]; then
        print_error "Production directory not created"
        exit 1
    fi
    
    # Check build artifacts
    if [ ! -f "$BUILD_DIR/ternaryos/qemu_slice6/BOOTAA64.EFI" ]; then
        print_error "Build artifacts not found"
        exit 1
    fi
    
    # Check scripts
    for script in verify_production.sh launch_production.sh monitor_production.sh; do
        if [ ! -f "$SCRIPTS_DIR/$script" ]; then
            print_error "Script $script not found"
            exit 1
        fi
    done
    
    # Check documentation
    if [ ! -f "$DOCS_DIR/README.md" ]; then
        print_error "Documentation not found"
        exit 1
    fi
    
    print_status "Installation verified successfully"
}

# Function to display usage information
display_usage() {
    print_header "Production Setup Complete"
    
    echo ""
    echo "🎉 Production QEMU TernaryOS setup completed successfully!"
    echo ""
    echo "Directory Structure:"
    echo "  $PRODUCTION_DIR/          - Production root"
    echo "  $BUILD_DIR/               - Build artifacts"
    echo "  $SCRIPTS_DIR/             - Production scripts"
    echo "  $DOCS_DIR/                - Documentation"
    echo "  $LOGS_DIR/                - Log files"
    echo ""
    echo "Quick Start:"
    echo "  1. Source environment: source $PRODUCTION_DIR/setup_env.sh"
    echo "  2. Run verification: verify_production.sh"
    echo "  3. Launch TernaryOS: launch_production.sh"
    echo "  4. Monitor system: monitor_production.sh"
    echo ""
    echo "For detailed documentation, see: $DOCS_DIR/"
    echo ""
    echo "Production logs will be stored in: $LOGS_DIR/"
    echo ""
}

# Main execution
main() {
    print_header "Production QEMU TernaryOS Setup"
    
    check_root
    check_dependencies
    create_directories
    setup_source
    build_production
    create_production_scripts
    setup_documentation
    setup_logging
    create_production_tests
    setup_environment
    verify_installation
    display_usage
    
    print_status "Production setup completed successfully!"
}

# Run main function
main "$@"
