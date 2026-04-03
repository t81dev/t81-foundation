#!/bin/bash

# QEMU/EFI Health Check Script
# Quick health check for common QEMU/EFI development issues

set -e

echo "🔍 QEMU/EFI Health Check"
echo "================================"

# Check basic QEMU installation
echo "Checking QEMU installation..."
if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
    echo "❌ QEMU not found"
    echo "   Fix: brew install qemu (macOS) or apt-get install qemu-system-arm (Linux)"
    exit 1
else
    echo "✅ QEMU found: $(qemu-system-aarch64 --version)"
fi

# Check EFI development tools
echo "Checking EFI tools..."
if ! command -v objcopy >/dev/null 2>&1; then
    echo "❌ objcopy not found"
    echo "   Fix: Install binutils package"
    exit 1
else
    echo "✅ objcopy found"
fi

# Check build environment
echo "Checking build environment..."
if ! command -v cmake >/dev/null 2>&1; then
    echo "❌ CMake not found"
    echo "   Fix: brew install cmake (macOS) or apt-get install cmake (Linux)"
    exit 1
else
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    echo "✅ CMake found: $CMAKE_VERSION"
    
    if [[ "$CMAKE_VERSION" < "3.15" ]]; then
        echo "⚠️  CMake version below recommended 3.15+"
    fi
fi

# Check build directory
echo "Checking build directory..."
if [[ ! -d "build" ]]; then
    echo "⚠️  Build directory not found"
    echo "   Fix: mkdir -p build && cd build && cmake .."
else
    echo "✅ Build directory found"
fi

# Check for disk images
echo "Checking disk images..."
if [[ -d "build" ]]; then
    IMAGE_COUNT=$(find build -name "*.img" -type f | wc -l)
    if [[ $IMAGE_COUNT -gt 0 ]]; then
        echo "✅ Found $IMAGE_COUNT disk images"
        
        # Check for very small images (likely failed builds)
        for img in build/*.img; do
            SIZE=$(stat -f%z "$img" 2>/dev/null || echo "0")
            if [[ $SIZE -lt 1048576 ]]; then  # < 1MB
                echo "⚠️  Very small image: $img ($(numfmt --to=iec-i --suffix=B $SIZE))"
            fi
        done
    else
        echo "ℹ️  No disk images found"
    fi
fi

# Check for EFI firmware
echo "Checking EFI firmware..."
EFI_FIRMWARE_PATHS=(
    "/usr/share/edk2-ovmf/OVMF_CODE.fd"
    "/usr/share/qemu/edk2-x86_64.fd"
    "/usr/local/share/qemu/edk2-x86_64.fd"
)

FIRMWARE_FOUND=false
for path in "${EFI_FIRMWARE_PATHS[@]}"; do
    if [[ -f "$path" ]]; then
        echo "✅ EFI firmware found: $path"
        FIRMWARE_FOUND=true
        break
    fi
done

if [[ "$FIRMWARE_FOUND" == "false" ]]; then
    echo "⚠️  EFI firmware not found in standard locations"
    echo "   Fix: Install qemu-efi package"
fi

# Check T81 QEMU scripts
echo "Checking T81 QEMU scripts..."
T81_SCRIPTS=(
    "ternaryos/scripts/qemu_shell_handoff.py"
    "ternaryos/scripts/qemu_monitor_test.sh"
    "drivers/qemu/scripts/qemu_monitor_test.sh"
)

for script in "${T81_SCRIPTS[@]}"; do
    if [[ -f "$script" ]]; then
        if [[ ! -x "$script" ]]; then
            echo "⚠️  T81 script not executable: $script"
            echo "   Fix: chmod +x $script"
        else
            echo "✅ T81 script found: $script"
        fi
    fi
done

# Memory check
echo "Checking system memory..."
if command -v sysctl >/dev/null 2>&1 && [[ "$(uname)" == "Darwin" ]]; then
    MEMORY_GB=$(sysctl -n hw.memsize | awk '{print int($1/1024/1024/1024)}')
    echo "✅ System memory: ${MEMORY_GB}GB"
    
    if [[ $MEMORY_GB -lt 4 ]]; then
        echo "⚠️  Low memory may affect QEMU performance"
    fi
elif [[ -f "/proc/meminfo" ]]; then
    MEMORY_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    MEMORY_GB=$((MEMORY_KB / 1024))
    echo "✅ System memory: ${MEMORY_GB}GB"
    
    if [[ $MEMORY_GB -lt 4 ]]; then
        echo "⚠️  Low memory may affect QEMU performance"
    fi
fi

# Summary
echo ""
echo "📊 Health Check Summary"
echo "======================"

ERROR_COUNT=0

# Count actual errors (not warnings)
if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
    ((ERROR_COUNT++))
fi

if ! command -v objcopy >/dev/null 2>&1; then
    ((ERROR_COUNT++))
fi

if ! command -v cmake >/dev/null 2>&1; then
    ((ERROR_COUNT++))
fi

if [[ $ERROR_COUNT -eq 0 ]]; then
    echo "✅ No critical issues found"
    echo "💡 Ready for T81 QEMU development"
    exit 0
else
    echo "❌ Found $ERROR_COUNT critical issue(s)"
    echo "🔧 Fix the issues above before proceeding"
    exit 1
fi
