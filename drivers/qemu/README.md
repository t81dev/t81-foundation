# Local Production QEMU TernaryOS

## Overview
This is a local production-ready QEMU TernaryOS installation for deterministic ternary computing development.

## Quick Start

### 1. Verification
```bash
./scripts/verify_production.sh
```

### 2. Launch
```bash
./scripts/launch_production.sh
```

### 3. Monitor
```bash
./scripts/monitor_production.sh
```

## Directory Structure
- `build/` - Build artifacts
- `scripts/` - Production scripts
- `docs/` - Documentation
- `logs/` - Log files
- `production_image/` - Generated disk images

## Usage Examples

### Basic Launch
```bash
./scripts/launch_production.sh
```

### Monitoring with Debug
```bash
./scripts/monitor_production.sh
# Then in another terminal:
telnet localhost 1234
```

### Verification
```bash
./scripts/verify_production.sh
```

## Advanced Usage

### Custom Memory Configuration
```bash
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=production_image/qemu_slice6_guest.img,format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -m 512M -smp 4
```

### Network Support
```bash
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=production_image/qemu_slice6_guest.img,format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -netdev user,id=net0 -device virtio-net-device,netdev=net0
```

### Debug Mode
```bash
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=production_image/qemu_slice6_guest.img,format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -d int,cpu,exec -D debug.log
```

## Troubleshooting

### Common Issues

#### Build Artifacts Missing
```bash
# Rebuild from source
cd ../
cmake --build . --target t81_ternaryos_qemu_slice6_efi
cp -r build/ternaryos production_qemu/build/
```

#### QEMU Not Found
```bash
# Install QEMU (macOS)
brew install qemu

# Install QEMU (Linux)
sudo apt-get install qemu-system-arm
```

#### Permission Issues
```bash
# Make scripts executable
chmod +x production_qemu/scripts/*.sh
```

### Debug Mode

#### Enable Debug Logging
```bash
# Run with debug output
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=production_image/qemu_slice6_guest.img,format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -d int,cpu,exec -D production_debug.log
```

#### Memory Analysis
```bash
# Create memory dump
echo "dump-guest-memory /tmp/memory.dump" | nc localhost 1234

# Analyze memory
../userland/experimental/memory_analyzer.sh /tmp/memory.dump
```

## Performance Tuning

### Optimize Boot Time
```bash
# Fast boot configuration
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=production_image/qemu_slice6_guest.img,format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -no-reboot -nodefaults
```

### Optimize Performance
```bash
# High performance configuration
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=production_image/qemu_slice6_guest.img,format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -smp 4 -m 512M -enable-kvm
```

## Integration with Development Workflow

### CI/CD Integration
```bash
# Add to CI pipeline
./scripts/verify_production.sh
```

### Development Testing
```bash
# Quick test during development
./scripts/verify_production.sh && ./scripts/monitor_production.sh
```

### Automated Testing
```bash
# Run full test suite
./scripts/verify_production.sh
```

## Support

### Documentation
- `docs/QEMU_INTEGRATION_ACHIEVEMENTS.md` - Complete achievement record
- `docs/QEMU_DEVELOPMENT_WORKFLOW.md` - Development workflow guide
- `docs/QEMU_SERIAL_CAPTURE_ANALYSIS.md` - Problem analysis
- `docs/ALTERNATIVE_VERIFICATION_RESULTS.md` - Solution documentation

### Community Support
- GitHub Issues: Report bugs and request features
- Documentation: Refer to comprehensive guides
- Examples: See usage examples above

### Advanced Topics
- Memory dump analysis
- QEMU monitor integration
- Automated verification
- Performance profiling

## Security Considerations

### Isolation
- QEMU provides hardware isolation
- Network access can be controlled
- File system access is limited

### Best Practices
- Use dedicated user account
- Limit network access when not needed
- Regular security updates
- Monitor resource usage

## Future Enhancements

### Planned Features
- GUI debugging interface
- Advanced profiling tools
- Network testing capabilities
- Cloud integration

### Extension Points
- Custom verification scripts
- Additional monitoring tools
- Performance analysis plugins
- Debugging extensions

---

**For advanced usage and troubleshooting, refer to the comprehensive documentation in the `docs/` directory.**
