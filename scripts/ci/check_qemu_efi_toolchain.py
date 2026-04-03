#!/usr/bin/env python3
"""
QEMU/EFI Toolchain Fail-Fast Checks

This script provides early detection of QEMU/EFI toolchain issues
and configuration problems that commonly cause development friction.

Usage: python3 check_qemu_efi_toolchain.py [--verbose] [--fix]
"""

import argparse
import json
import os
import subprocess
import sys
import shutil
import platform
from pathlib import Path
from typing import List, Dict, Tuple, Optional

class QEMUEFIChecker:
    def __init__(self, verbose: bool = False, auto_fix: bool = False):
        self.verbose = verbose
        self.auto_fix = auto_fix
        self.issues = []
        self.fixes = []
        self.root_dir = Path.cwd()
        
    def log(self, message: str, level: str = "INFO"):
        if self.verbose or level in ["ERROR", "WARNING"]:
            print(f"[{level}] {message}")
    
    def add_issue(self, category: str, severity: str, description: str, 
                 fix_cmd: Optional[str] = None, auto_fixable: bool = False):
        issue = {
            "category": category,
            "severity": severity,
            "description": description,
            "fix_command": fix_cmd,
            "auto_fixable": auto_fixable
        }
        self.issues.append(issue)
        self.log(f"{category}: {description}", severity)
        
        if auto_fixable and self.auto_fix and fix_cmd:
            try:
                subprocess.run(fix_cmd, shell=True, check=True, cwd=self.root_dir)
                self.fixes.append(fix_cmd)
                self.log(f"Auto-fixed: {description}", "INFO")
            except subprocess.CalledProcessError as e:
                self.log(f"Auto-fix failed: {e}", "ERROR")
    
    def check_qemu_installation(self):
        """Check if QEMU is properly installed and accessible."""
        self.log("Checking QEMU installation...")
        
        # Check for QEMU binaries
        qemu_binaries = [
            "qemu-system-aarch64",
            "qemu-system-x86_64", 
            "qemu-img"
        ]
        
        for binary in qemu_binaries:
            if not shutil.which(binary):
                self.add_issue(
                    "qemu_installation",
                    "ERROR" if binary in ["qemu-system-aarch64", "qemu-system-x86_64"] else "WARNING",
                    f"Missing QEMU binary: {binary}",
                    f"brew install qemu" if platform.system() == "Darwin" else 
                    f"apt-get install qemu-system-{binary.split('-')[-1]}"
                )
    
    def check_efi_toolchain(self):
        """Check EFI development tools and environment."""
        self.log("Checking EFI toolchain...")
        
        # Check for required EFI development tools
        efi_tools = {
            "gcc": "gcc",
            "objcopy": "objcopy", 
            "ld": "ld",
            "python3": "python3"
        }
        
        for tool_name, binary in efi_tools.items():
            if not shutil.which(binary):
                self.add_issue(
                    "efi_toolchain",
                    "ERROR",
                    f"Missing EFI development tool: {tool_name} ({binary})",
                    f"brew install {binary}" if platform.system() == "Darwin" else 
                    f"apt-get install {binary}"
                )
    
    def check_build_environment(self):
        """Check build environment and dependencies."""
        self.log("Checking build environment...")
        
        # Check CMake
        cmake_version = self._get_version("cmake --version", r"cmake version (\d+\.\d+)")
        if cmake_version and tuple(map(int, cmake_version.split('.'))) < (3, 15):
            self.add_issue(
                "build_environment",
                "WARNING",
                f"CMake version {cmake_version} is below recommended 3.15+",
                "brew upgrade cmake" if platform.system() == "Darwin" else 
                "apt-get install cmake"
            )
        
        # Check Make
        if not shutil.which("make"):
            self.add_issue(
                "build_environment",
                "ERROR",
                "Missing make tool",
                "brew install make" if platform.system() == "Darwin" else 
                "apt-get install make"
            )
        
        # Check for required build directories
        build_dir = self.root_dir / "build"
        if not build_dir.exists():
            self.add_issue(
                "build_environment", 
                "WARNING",
                "Build directory does not exist",
                "mkdir -p build",
                auto_fixable=True
            )
    
    def check_qemu_configuration(self):
        """Check QEMU configuration and common issues."""
        self.log("Checking QEMU configuration...")
        
        # Check for QEMU configuration files
        qemu_configs = [
            self.root_dir / ".qemu",
            Path.home() / ".qemu",
            "/etc/qemu"
        ]
        
        config_issues = []
        for config_path in qemu_configs:
            if config_path.exists():
                try:
                    with open(config_path, 'r') as f:
                        config_content = f.read()
                        # Check for common problematic configurations
                        if "enable-kvm" in config_content and platform.system() == "Darwin":
                            config_issues.append("KVM enabled on macOS (not supported)")
                        if "memory" in config_content and "256M" in config_content:
                            config_issues.append("Low memory configuration (256M)")
                except Exception as e:
                    self.add_issue(
                        "qemu_configuration",
                        "WARNING",
                        f"Cannot read QEMU config {config_path}: {e}"
                    )
        
        for issue in config_issues:
            self.add_issue(
                "qemu_configuration",
                "WARNING",
                f"QEMU configuration issue: {issue}"
            )
    
    def check_efi_boot_environment(self):
        """Check EFI boot environment and firmware."""
        self.log("Checking EFI boot environment...")
        
        # Check for EFI firmware availability
        efi_firmware_paths = [
            "/usr/share/edk2-ovmf/OVMF_CODE.fd",
            "/usr/share/qemu/edk2-x86_64.fd",
            "/usr/local/share/qemu/edk2-x86_64.fd"
        ]
        
        firmware_available = False
        for firmware_path in efi_firmware_paths:
            if Path(firmware_path).exists():
                firmware_available = True
                break
        
        if not firmware_available:
            self.add_issue(
                "efi_boot_environment",
                "WARNING",
                "EFI firmware not found in standard locations",
                "brew install qemu" if platform.system() == "Darwin" else 
                "apt-get install qemu-efi"
            )
        
        # Check for EFI variable support
        try:
            result = subprocess.run(
                ["efivar", "--list"], 
                capture_output=True, 
                text=True,
                timeout=5
            )
            if result.returncode != 0 and shutil.which("efivar"):
                self.add_issue(
                    "efi_boot_environment",
                    "WARNING",
                    "EFI variables not accessible (may need root/sudo)"
                )
        except (subprocess.TimeoutExpired, FileNotFoundError):
            # Expected on many systems
            pass
    
    def check_t81_qemu_integration(self):
        """Check T81-specific QEMU integration issues."""
        self.log("Checking T81 QEMU integration...")
        
        # Check for T81 QEMU scripts
        t81_qemu_scripts = [
            "ternaryos/scripts/qemu_shell_handoff.py",
            "ternaryos/scripts/qemu_monitor_test.sh",
            "drivers/qemu/scripts/qemu_monitor_test.sh"
        ]
        
        for script in t81_qemu_scripts:
            script_path = self.root_dir / script
            if script_path.exists():
                # Check script permissions
                if not os.access(script_path, os.X_OK):
                    self.add_issue(
                        "t81_qemu_integration",
                        "WARNING",
                        f"T81 QEMU script not executable: {script}",
                        f"chmod +x {script}",
                        auto_fixable=True
                    )
                
                # Check script dependencies
                if script.endswith(".py"):
                    try:
                        result = subprocess.run(
                            ["python3", "-m", "py_compile", str(script_path)],
                            capture_output=True,
                            text=True,
                            timeout=10
                        )
                        if result.returncode != 0:
                            self.add_issue(
                                "t81_qemu_integration",
                                "WARNING",
                                f"T81 QEMU script has syntax errors: {script}"
                            )
                    except subprocess.TimeoutExpired:
                        self.add_issue(
                            "t81_qemu_integration",
                            "WARNING",
                            f"T81 QEMU script compilation timeout: {script}"
                        )
    
    def check_disk_images(self):
        """Check for disk image issues and permissions."""
        self.log("Checking disk images...")
        
        # Look for common disk image issues
        build_dir = self.root_dir / "build"
        if build_dir.exists():
            for img_file in build_dir.rglob("*.img"):
                # Check file permissions
                if not os.access(img_file, os.R_OK):
                    self.add_issue(
                        "disk_images",
                        "WARNING",
                        f"Disk image not readable: {img_file.name}"
                    )
                
                # Check for extremely small images (likely failed builds)
                if img_file.stat().st_size < 1024 * 1024: # < 1MB
                    self.add_issue(
                        "disk_images",
                        "WARNING",
                        f"Very small disk image (possible failed build): {img_file.name}"
                    )
                
                # Check for orphaned images
                if "qemu" in img_file.name.lower() and img_file.stat().st_mtime < (
                    (Path.cwd().stat().st_mtime - 7 * 24 * 3600)  # 7 days ago
                ):
                    self.add_issue(
                        "disk_images",
                        "INFO",
                        f"Old QEMU disk image found: {img_file.name}"
                    )
    
    def check_network_configuration(self):
        """Check QEMU network configuration issues."""
        self.log("Checking network configuration...")
        
        # Check for common network port conflicts
        common_ports = [2222, 5555, 8080, 9090]
        for port in common_ports:
            try:
                result = subprocess.run(
                    ["lsof", "-i", f":{port}"],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    self.add_issue(
                        "network_configuration",
                        "WARNING",
                        f"Port {port} already in use (may conflict with QEMU)"
                    )
            except (subprocess.TimeoutExpired, FileNotFoundError):
                # lsof not available is common
                pass
    
    def check_memory_configuration(self):
        """Check memory configuration and limits."""
        self.log("Checking memory configuration...")
        
        # Check system memory
        try:
            if platform.system() == "Darwin":
                result = subprocess.run(
                    ["sysctl", "hw.memsize"],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    memory_gb = int(result.stdout.strip()) // (1024 * 1024 * 1024)
                    if memory_gb < 4:
                        self.add_issue(
                            "memory_configuration",
                            "WARNING",
                            f"Low system memory ({memory_gb}GB) may affect QEMU performance"
                        )
            elif platform.system() == "Linux":
                with open("/proc/meminfo", "r") as f:
                    for line in f:
                        if line.startswith("MemTotal:"):
                            memory_kb = int(line.split()[1])
                            memory_gb = memory_kb // 1024
                            if memory_gb < 4:
                                self.add_issue(
                                    "memory_configuration",
                                    "WARNING",
                                    f"Low system memory ({memory_gb}GB) may affect QEMU performance"
                                )
                            break
        except Exception:
            pass  # Memory checks are best-effort
    
    def _get_version(self, command: str, pattern: str) -> Optional[str]:
        """Extract version from command output using regex pattern."""
        try:
            result = subprocess.run(
                command.split(),
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode == 0:
                import re
                match = re.search(pattern, result.stdout)
                return match.group(1) if match else None
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        return None
    
    def run_all_checks(self):
        """Run all QEMU/EFI checks."""
        self.log("Starting QEMU/EFI toolchain checks...")
        
        # Run all check categories
        self.check_qemu_installation()
        self.check_efi_toolchain()
        self.check_build_environment()
        self.check_qemu_configuration()
        self.check_efi_boot_environment()
        self.check_t81_qemu_integration()
        self.check_disk_images()
        self.check_network_configuration()
        self.check_memory_configuration()
        
        return self.issues, self.fixes
    
    def generate_report(self) -> str:
        """Generate a comprehensive report of all issues found."""
        if not self.issues:
            return "✅ No QEMU/EFI toolchain issues detected!"
        
        report = ["🔍 QEMU/EFI Toolchain Check Report", "=" * 50, ""]
        
        # Group issues by severity
        error_issues = [i for i in self.issues if i["severity"] == "ERROR"]
        warning_issues = [i for i in self.issues if i["severity"] == "WARNING"]
        info_issues = [i for i in self.issues if i["severity"] == "INFO"]
        
        if error_issues:
            report.append(f"🚨 ERRORS ({len(error_issues)}):")
            for issue in error_issues:
                report.append(f"  ❌ {issue['description']}")
                if issue["fix_command"]:
                    report.append(f"     Fix: {issue['fix_command']}")
            report.append("")
        
        if warning_issues:
            report.append(f"⚠️  WARNINGS ({len(warning_issues)}):")
            for issue in warning_issues:
                report.append(f"  ⚠️  {issue['description']}")
                if issue["fix_command"]:
                    report.append(f"     Fix: {issue['fix_command']}")
            report.append("")
        
        if info_issues:
            report.append(f"ℹ️  INFO ({len(info_issues)}):")
            for issue in info_issues:
                report.append(f"  ℹ️  {issue['description']}")
            report.append("")
        
        if self.fixes:
            report.append(f"🔧 Auto-applied Fixes ({len(self.fixes)}):")
            for fix in self.fixes:
                report.append(f"  ✅ {fix}")
            report.append("")
        
        # Add recommendations
        report.append("💡 Recommendations:")
        if error_issues:
            report.append("  - Fix all ERROR issues before proceeding")
        if warning_issues:
            report.append("  - Review WARNING issues for optimal development experience")
        report.append("  - Run with --fix flag to auto-fix applicable issues")
        report.append("  - Use --verbose flag for detailed diagnostic information")
        
        return "\n".join(report)

def main():
    parser = argparse.ArgumentParser(description="QEMU/EFI toolchain fail-fast checks")
    parser.add_argument("--verbose", "-v", action="store_true", 
                       help="Enable verbose output")
    parser.add_argument("--fix", "-f", action="store_true",
                       help="Auto-fix applicable issues")
    parser.add_argument("--json", action="store_true",
                       help="Output results in JSON format")
    
    args = parser.parse_args()
    
    checker = QEMUEFIChecker(verbose=args.verbose, auto_fix=args.fix)
    issues, fixes = checker.run_all_checks()
    
    if args.json:
        result = {
            "issues": issues,
            "fixes_applied": fixes,
            "summary": {
                "total_issues": len(issues),
                "errors": len([i for i in issues if i["severity"] == "ERROR"]),
                "warnings": len([i for i in issues if i["severity"] == "WARNING"]),
                "info": len([i for i in issues if i["severity"] == "INFO"])
            }
        }
        print(json.dumps(result, indent=2))
    else:
        print(checker.generate_report())
    
    # Exit with error code if there are any ERROR issues
    error_count = len([i for i in issues if i["severity"] == "ERROR"])
    sys.exit(1 if error_count > 0 else 0)

if __name__ == "__main__":
    main()
