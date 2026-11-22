"""Exercise the AArch64 demo entrypoint with stubbed firmware and QEMU."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "drivers/qemu/scripts/boot_demo.sh"


class BootDemoTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="t81-boot-demo-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        for name in ("basename", "cp", "cut", "dd", "dirname", "du", "git", "grep",
                     "mktemp", "rm", "rmdir", "tee"):
            (self.bin / name).symlink_to(shutil.which(name))
        (self.bin / "python3").symlink_to(sys.executable)
        for name in ("parted", "mformat", "mmd", "mcopy"):
            self.stub(name, "exit 0")
        self.stub("cmake", "exit 23")
        self.stub("timeout", 'shift\nexec "$@"')
        self.stub(
            "qemu-system-aarch64",
            r"""printf '%s\n' "$@" > "$BUILD_DIR/qemu.args"
printf 'firmware output\r\n'
if [ "$PROMPT" = yes ]; then printf '[axion@T81 tier=1]$ '; fi
if [ "$QEMU_STATUS" != 0 ]; then printf 'qemu diagnostic\n' >&2; fi
exit "$QEMU_STATUS"
""",
        )
        self.build = self.root / "build directory"
        self.efi = self.build / "ternaryos/qemu_slice6/BOOTAA64.EFI"
        self.efi.parent.mkdir(parents=True)
        self.efi.write_bytes(b"fixture EFI binary")
        firmware = self.root / "firmware.fd"
        firmware.write_bytes(b"fixture firmware")
        self.env = dict(os.environ, PATH=str(self.bin), BUILD_DIR=str(self.build),
                        EDK2_CODE=str(firmware), EDK2_VARS=str(firmware),
                        TMPDIR=str(self.root), PROMPT="yes", QEMU_STATUS="0")
        self.env.pop("CANON_IMG", None)
        self.bash = shutil.which("bash")

    def stub(self, name, commands):
        executable = self.bin / name
        executable.write_text("#!/bin/sh\n" + commands + "\n")
        executable.chmod(0o755)

    def run_demo(self, **overrides):
        return subprocess.run(
            [self.bash, str(SCRIPT)], env=dict(self.env, **overrides),
            cwd=self.root, capture_output=True, text=True, timeout=15,
        )

    def test_image_and_prompt_confirm_success(self):
        result = self.run_demo()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Boot verified.", result.stdout)
        image = self.build / "canon_store.img"
        self.assertEqual(image.stat().st_size, 4 * 1024 * 1024)
        with image.open("rb") as store:
            self.assertEqual(store.read(729), b"CST1" + bytes(725))
        self.assertEqual((self.build / "boot-demo.serial.log").read_bytes(),
                         b"firmware output\r\n[axion@T81 tier=1]$ ")
        qemu_args = (self.build / "qemu.args").read_text().splitlines()
        self.assertIn("virt,gic-version=3", qemu_args)
        self.assertIn("virtio-mmio.force-legacy=off", qemu_args)
        self.assertIn("virtio-blk-device,drive=boot0,bootindex=0", qemu_args)
        self.assertIn("virtio-blk-device,drive=canon0,bootindex=1", qemu_args)
        self.assertEqual(list(self.root.glob("t81-qemu-demo.*")), [])

    def test_checks_mtools_executables_individually(self):
        self.stub("mtools", "exit 0")
        for missing in ("mformat", "mmd", "mcopy"):
            with self.subTest(missing=missing):
                (self.bin / missing).unlink()
                result = self.run_demo()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(f"Missing: {missing}", result.stderr)
                self.assertFalse((self.build / "canon_store.img").exists())
                self.stub(missing, "exit 0")

    def test_qemu_error_and_timeout_are_not_masked_by_prompt(self):
        for status in (37, 124):
            with self.subTest(status=status):
                result = self.run_demo(QEMU_STATUS=str(status))
                self.assertEqual(result.returncode, status, result.stderr)
                self.assertIn("qemu diagnostic", result.stderr)
                self.assertIn(f"exit status {status}", result.stderr)
                self.assertNotIn("Boot verified.", result.stdout)

    def test_zero_exit_without_prompt_does_not_use_stale_log(self):
        log = self.build / "boot-demo.serial.log"
        log.write_text("[axion@T81 tier=1]$ ")
        result = self.run_demo(PROMPT="no")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Guest shell prompt was not observed", result.stderr)
        self.assertNotIn("Boot verified.", result.stdout)
        self.assertNotIn("[axion@T81", log.read_text())

    def test_failed_configure_does_not_attempt_boot(self):
        self.efi.unlink()
        result = self.run_demo()
        self.assertEqual(result.returncode, 23)
        self.assertNotIn("Building BOOTAA64.EFI", result.stdout)
        self.assertFalse((self.build / "canon_store.img").exists())


if __name__ == "__main__":
    unittest.main()
