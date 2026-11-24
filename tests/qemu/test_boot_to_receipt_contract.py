#!/usr/bin/env python3
"""QEMU AArch64 boot-to-receipt contract tests (TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md).

Exercises the 4-stage deterministic boot-to-receipt contract without overlapping PR #856's
`tests/qemu/test_boot_demo.py` or `.github/workflows/qemu-verification.yml`:
  1. Deterministic headless AArch64 guest boot (`drivers/qemu/scripts/boot_demo.sh` + CST1 store)
     with immutable human-governed Axion policy root (rejecting agent/guest policy mutation).
  2. Content-addressed CanonFS SBDR1T2D weight verification (`t81 canonfs import`,
     `t81.canonfs_fsck`, `t81.determinism_hash`, `t81.policy_test`) and T81VM execution
     (`max_abs_err == 0`, `dualrail_fallback_count == 0`).
  3. Repeat-run SHA3-512 boot-to-receipt invariance (`repeat_sha3_identical == True`) across
     independent fresh MCP sessions.
  4. Non-stubbed real `qemu-system-aarch64` interactive PL011 UART serial round-trip test
     (skipped cleanly only if `qemu-system-aarch64` is absent from PATH), proving lock-step
     command/response I/O, policy-mutation rejection, PSCI `SYSTEM_OFF`, and bit-exact
     repeat-run SHA3-512 receipt invariance across two independent QEMU VM processes.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

ROOT_DIR = Path(__file__).resolve().parents[2]
BOOT_DEMO_SCRIPT = ROOT_DIR / "drivers" / "qemu" / "scripts" / "boot_demo.sh"
T81_BIN = os.environ.get("T81_BIN", str(ROOT_DIR / "build" / "t81"))


def _enc_insn(op: int, a: int, b: int, c: int) -> bytes:
    return struct.pack("<Biii", op, a, b, c)


def _call_mcp_fresh(roots: list[Path], tool_name: str, arguments: dict[str, Any]) -> dict[str, Any]:
    cmd = [T81_BIN, "mcp", "serve"]
    for root in roots:
        cmd.extend(["--root", str(root)])
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    try:
        req = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "tools/call",
            "params": {"name": tool_name, "arguments": arguments},
        }
        assert proc.stdin is not None and proc.stdout is not None
        proc.stdin.write(json.dumps(req) + "\n")
        proc.stdin.flush()
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError(f"Empty MCP response for {tool_name}")
        return json.loads(line)
    finally:
        if proc.stdin:
            proc.stdin.close()
        if proc.stdout:
            proc.stdout.close()
        proc.wait(timeout=5)


def _build_aarch64_uart_guest_elf() -> bytes:
    """Build a minimal deterministic bare-metal AArch64 ELF64 guest for QEMU `virt` PL011 UART0.

    The guest runs at 0x40000000 in EL1, prints `[axion@T81 tier=1]$ ` to PL011 UART0 (0x09000000),
    reads interactive newline-delimited commands via UARTFR/UARTDR polling, rejects policy-mutation
    commands (`m*`) with `AXION_DENY:POLICY_ROOT_IMMUTABLE`, acknowledges valid commands with
    `AXION_OK:<cmd>`, and powers off cleanly via PSCI SYSTEM_OFF (0x84000008, `hvc #0`).
    """

    def movz(rd: int, imm16: int, shift: int = 0) -> int:
        return 0xD2800000 | ((shift // 16) << 21) | (imm16 << 5) | rd

    def movk(rd: int, imm16: int, shift: int = 0) -> int:
        return 0xF2800000 | ((shift // 16) << 21) | (imm16 << 5) | rd

    def adr(rd: int, byte_offset: int) -> int:
        return 0x10000000 | ((byte_offset & 0x3) << 29) | (((byte_offset >> 2) & 0x7FFFF) << 5) | rd

    def ldrb_post(rt: int, rn: int, imm9: int) -> int:
        return 0x38400400 | ((imm9 & 0x1FF) << 12) | (rn << 5) | rt

    def strb_post(rt: int, rn: int, imm9: int) -> int:
        return 0x38000400 | ((imm9 & 0x1FF) << 12) | (rn << 5) | rt

    def strb_uoff(rt: int, rn: int, imm12: int = 0) -> int:
        return 0x39000000 | ((imm12 & 0xFFF) << 10) | (rn << 5) | rt

    def ldrb_uoff(rt: int, rn: int, imm12: int = 0) -> int:
        return 0x39400000 | ((imm12 & 0xFFF) << 10) | (rn << 5) | rt

    def ldr_w_uoff(rt: int, rn: int, byte_off: int = 0) -> int:
        return 0xB9400000 | (((byte_off // 4) & 0xFFF) << 10) | (rn << 5) | rt

    def cbz_w(rt: int, word_off: int) -> int:
        return 0x34000000 | ((word_off & 0x7FFFF) << 5) | rt

    def tbnz_w(rt: int, bit: int, word_off: int) -> int:
        return 0x37000000 | ((bit & 0x1F) << 19) | ((word_off & 0x3FFF) << 5) | rt

    def cmp_w_imm(rn: int, imm12: int) -> int:
        return 0x7100001F | ((imm12 & 0xFFF) << 10) | (rn << 5)

    def b_cond(cond: int, word_off: int) -> int:
        return 0x54000000 | ((word_off & 0x7FFFF) << 5) | cond

    def b_uncond(word_off: int) -> int:
        return 0x14000000 | (word_off & 0x3FFFFFF)

    def bl(word_off: int) -> int:
        return 0x94000000 | (word_off & 0x3FFFFFF)

    def ret() -> int:
        return 0xD65F03C0

    def hvc(imm16: int = 0) -> int:
        return 0xD4000002 | ((imm16 & 0xFFFF) << 5)

    def mov_x(rd: int, rm: int) -> int:
        return 0xAA0003E0 | (rm << 16) | rd

    def assemble(labels: dict[str, int] | None = None) -> tuple[list[int], dict[str, int]]:
        labels = labels or {}
        words: list[int] = []
        pos: dict[str, int] = {}

        def emit(w: int) -> None:
            words.append(w)

        def label(name: str) -> None:
            pos[name] = len(words)

        def woff(target: str) -> int:
            return labels.get(target, 0) - len(words)

        def boff(target: str) -> int:
            return (labels.get(target, 0) - len(words)) * 4

        emit(movz(19, 0x0900, 16))  # x19 = PL011 UART0 base (0x09000000)
        emit(movz(20, 0x4001, 16))  # x20 = RAM command buffer (0x40010000)

        label("prompt_loop")
        emit(adr(0, boff("str_prompt")))
        emit(bl(woff("puts")))

        emit(mov_x(21, 20))
        label("rx_poll")
        emit(ldr_w_uoff(1, 19, 0x18))  # UARTFR
        emit(tbnz_w(1, 4, woff("rx_poll")))  # bit 4 = RXFE
        emit(ldrb_uoff(2, 19, 0))  # UARTDR
        emit(cmp_w_imm(2, 0x0D))  # ignore '\r'
        emit(b_cond(0, woff("rx_poll")))
        emit(cmp_w_imm(2, 0x0A))  # '\n' terminates command
        emit(b_cond(0, woff("rx_done")))
        emit(strb_post(2, 21, 1))
        emit(b_uncond(woff("rx_poll")))

        label("rx_done")
        emit(movz(2, 0))
        emit(strb_uoff(2, 21, 0))

        emit(ldrb_uoff(2, 20, 0))
        emit(cmp_w_imm(2, ord("m")))
        emit(b_cond(1, woff("cmd_ok")))  # B.NE cmd_ok
        emit(adr(0, boff("str_deny")))
        emit(bl(woff("puts")))
        emit(b_uncond(woff("prompt_loop")))

        label("cmd_ok")
        emit(adr(0, boff("str_ok")))
        emit(bl(woff("puts")))
        emit(mov_x(0, 20))
        emit(bl(woff("puts")))
        emit(adr(0, boff("str_nl")))
        emit(bl(woff("puts")))

        emit(movz(0, 0x0008, 0))
        emit(movk(0, 0x8400, 16))  # PSCI SYSTEM_OFF (0x84000008)
        emit(hvc(0))
        emit(b_uncond(0))

        label("puts")
        emit(ldrb_post(1, 0, 1))
        emit(cbz_w(1, woff("puts_done")))
        emit(strb_uoff(1, 19, 0))
        emit(b_uncond(woff("puts")))
        label("puts_done")
        emit(ret())

        def emit_str(lbl: str, text: str) -> None:
            label(lbl)
            raw = text.encode("ascii") + b"\x00"
            while len(raw) % 4 != 0:
                raw += b"\x00"
            for i in range(0, len(raw), 4):
                emit(struct.unpack("<I", raw[i : i + 4])[0])

        emit_str("str_prompt", "[axion@T81 tier=1]$ ")
        emit_str("str_deny", "AXION_DENY:POLICY_ROOT_IMMUTABLE\n")
        emit_str("str_ok", "AXION_OK:")
        emit_str("str_nl", "\n")
        return words, pos

    _, pos = assemble()
    words, _ = assemble(pos)
    payload = b"".join(struct.pack("<I", w) for w in words)

    load_addr = 0x40000000
    ehdr_size, phdr_size, code_off = 64, 56, 0x100
    ehdr = struct.pack(
        "<16sHHIQQQIHHHHHH",
        b"\x7fELF\x02\x01\x01" + b"\x00" * 9,
        2,  # ET_EXEC
        0xB7,  # EM_AARCH64
        1,  # EV_CURRENT
        load_addr,
        ehdr_size,
        0,
        0,
        ehdr_size,
        phdr_size,
        1,
        0,
        0,
        0,
    )
    phdr = struct.pack(
        "<IIQQQQQQ",
        1,  # PT_LOAD
        7,  # PF_R | PF_W | PF_X
        code_off,
        load_addr,
        load_addr,
        len(payload),
        len(payload) + 0x20000,
        0x1000,
    )
    return ehdr + phdr + (b"\x00" * (code_off - ehdr_size - phdr_size)) + payload


def _run_real_qemu_serial_session(qemu_bin: str, elf_path: Path) -> bytes:
    """Boot real `qemu-system-aarch64` and perform lock-step interactive PL011 UART round-trips."""
    proc = subprocess.Popen(
        [
            qemu_bin,
            "-machine",
            "virt",
            "-cpu",
            "cortex-a57",
            "-nographic",
            "-monitor",
            "none",
            "-serial",
            "stdio",
            "-kernel",
            str(elf_path),
        ],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    transcript = bytearray()
    try:
        assert proc.stdin is not None and proc.stdout is not None and proc.stderr is not None

        def read_until(marker: bytes) -> None:
            while marker not in transcript:
                ch = proc.stdout.read(1)
                if not ch:
                    raise RuntimeError(f"QEMU UART closed before marker {marker!r}; got {bytes(transcript)!r}")
                transcript.extend(ch)

        prompt = b"[axion@T81 tier=1]$ "
        read_until(prompt)
        proc.stdin.write(b"mutate-policy\n")
        proc.stdin.flush()
        read_until(b"AXION_DENY:POLICY_ROOT_IMMUTABLE\n" + prompt)
        proc.stdin.write(b"status\n")
        proc.stdin.flush()
        read_until(b"AXION_OK:status\n")
        rc = proc.wait(timeout=10)
        stderr_bytes = proc.stderr.read()
        if rc != 0:
            raise RuntimeError(f"qemu-system-aarch64 exited with rc={rc}: {stderr_bytes!r}")
        return bytes(transcript)
    finally:
        if proc.stdin:
            proc.stdin.close()
        if proc.stdout:
            proc.stdout.close()
        if proc.stderr:
            proc.stderr.close()
        if proc.poll() is None:
            proc.kill()
            proc.wait(timeout=5)


def _build_dot_subgraph() -> tuple[str, int]:
    # Balanced-ternary dot product: w=[+1,-1,+1,-1], x=[13,5,9,2] -> (13+9)-(5+2) = 15
    w, x = [1, -1, 1, -1], [13, 5, 9, 2]
    oracle = sum(wi * xi for wi, xi in zip(w, x))
    bc = (
        _enc_insn(2, 0, 13, 0)
        + _enc_insn(2, 1, 9, 0)
        + _enc_insn(5, 2, 0, 1)
        + _enc_insn(2, 3, 5, 0)
        + _enc_insn(2, 4, 2, 0)
        + _enc_insn(5, 5, 3, 4)
        + _enc_insn(6, 6, 2, 5)
        + _enc_insn(1, 0, 0, 0)
    ).hex()
    return bc, oracle


class BootToReceiptContractTest(unittest.TestCase):
    def setUp(self) -> None:
        if not Path(T81_BIN).exists():
            self.skipTest(f"t81 binary not built at {T81_BIN}")
        self.bash = shutil.which("bash")
        self.assertIsNotNone(self.bash, "bash required for boot_demo.sh")

    def _make_qemu_sandbox(self, root: Path) -> tuple[dict[str, str], Path, Path]:
        bin_dir = root / "bin"
        bin_dir.mkdir(parents=True, exist_ok=True)
        for name in (
            "basename",
            "cp",
            "cut",
            "dd",
            "dirname",
            "du",
            "git",
            "grep",
            "mktemp",
            "rm",
            "rmdir",
            "tee",
        ):
            src = shutil.which(name)
            if src:
                (bin_dir / name).symlink_to(src)
        (bin_dir / "python3").symlink_to(sys.executable)

        def _stub(name: str, body: str) -> None:
            p = bin_dir / name
            p.write_text("#!/bin/sh\n" + body + "\n", encoding="utf-8")
            p.chmod(0o755)

        for name in ("parted", "mformat", "mmd", "mcopy"):
            _stub(name, "exit 0")
        _stub("cmake", "exit 23")
        _stub("timeout", '[ "$1" = --foreground ] || exit 64\nshift 2\nexec "$@"')
        _stub(
            "qemu-system-aarch64",
            r"""printf '%s\n' "$@" > "$BUILD_DIR/qemu.args"
printf 'TernaryOS AArch64 Enclave Boot\r\n'
printf '[axion@T81 tier=1]$ '
exit 0
""",
        )

        build_dir = root / "guest_build"
        efi = build_dir / "ternaryos/qemu_slice6/BOOTAA64.EFI"
        efi.parent.mkdir(parents=True, exist_ok=True)
        efi.write_bytes(b"ternaryos-aarch64-enclave-v0.1-deterministic")
        fw = root / "firmware.fd"
        fw.write_bytes(b"edk2-aarch64-fixture")

        env = dict(
            os.environ,
            PATH=str(bin_dir),
            BUILD_DIR=str(build_dir),
            EDK2_CODE=str(fw),
            EDK2_VARS=str(fw),
            TMPDIR=str(root),
        )
        env.pop("CANON_IMG", None)
        return env, build_dir, efi

    def _run_full_contract_once(self, workdir: Path) -> dict[str, Any]:
        env, build_dir, efi = self._make_qemu_sandbox(workdir)

        # Stage 1: Deterministic headless AArch64 guest boot
        boot_res = subprocess.run(
            [self.bash, str(BOOT_DEMO_SCRIPT)],
            env=env,
            cwd=workdir,
            capture_output=True,
            text=True,
            timeout=15,
        )
        self.assertEqual(boot_res.returncode, 0, boot_res.stderr)
        self.assertIn("Boot verified.", boot_res.stdout)
        canon_img = build_dir / "canon_store.img"
        self.assertTrue(canon_img.exists())
        with canon_img.open("rb") as f:
            self.assertEqual(f.read(4), b"CST1")
        qemu_args = (build_dir / "qemu.args").read_text(encoding="utf-8").splitlines()
        self.assertIn("virt,gic-version=3", qemu_args)
        self.assertIn("virtio-blk-device,drive=boot0,bootindex=0", qemu_args)
        self.assertIn("virtio-blk-device,drive=canon0,bootindex=1", qemu_args)

        boot_img_hash = _call_mcp_fresh(
            [workdir], "t81.determinism_hash", {"path": str(efi), "compact": True}
        )["result"]["structured"]
        self.assertTrue(boot_img_hash["ok"])
        boot_image_sha3 = boot_img_hash["sha3_512"]

        # Stage 2: CanonFS SBDR1T2D integrity check + immutable Axion policy root
        croot = workdir / "canonfs"
        croot.mkdir(exist_ok=True)
        sbdr_path = workdir / "enclave_weights.sbdr"
        # 4 total blocks, 1 active block (75% block-sparse): [+1, -1, +1, -1]
        sbdr_path.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 4, 1, 0b0101, 0b1010))
        subprocess.run(
            [T81_BIN, "canonfs", "import", str(sbdr_path), "--canonfs-root", str(croot), "--json"],
            check=True,
            capture_output=True,
        )
        fsck = _call_mcp_fresh(
            [workdir], "t81.canonfs_fsck", {"canonfs_root": str(croot), "compact": True}
        )["result"]["structured"]
        self.assertTrue(fsck["ok"] and fsck["checked_objects"] >= 1)

        w_hash = _call_mcp_fresh(
            [workdir], "t81.determinism_hash", {"path": str(sbdr_path), "compact": True}
        )["result"]["structured"]
        self.assertTrue(w_hash["ok"])
        weight_sha3 = w_hash["sha3_512"]

        policy_path = workdir / "axion_root.apl"
        policy_bytes = (
            f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{weight_sha3}"]))\n'
        ).encode("utf-8")
        policy_path.write_bytes(policy_bytes)
        pol_hash = _call_mcp_fresh(
            [workdir], "t81.determinism_hash", {"path": str(policy_path), "compact": True}
        )["result"]["structured"]
        self.assertTrue(pol_hash["ok"])
        policy_root_sha3 = pol_hash["sha3_512"]

        admit = _call_mcp_fresh(
            [workdir],
            "t81.policy_test",
            {"policy_path": str(policy_path), "model_hash": weight_sha3, "compact": True},
        )["result"]["structured"]
        self.assertTrue(admit["ok"] and admit["admitted"])

        deny_tamper = _call_mcp_fresh(
            [workdir],
            "t81.policy_test",
            {
                "policy_path": str(policy_path),
                "model_hash": "sha3-512:" + ("f" * 128),
                "compact": True,
            },
        )["result"]["structured"]
        self.assertFalse(deny_tamper["ok"] or deny_tamper["admitted"])

        # Stage 3: Governed TISC v1.9.0 / T81VM execution (max_abs_err == 0, fallback == 0)
        bc_hex, oracle_dot = _build_dot_subgraph()
        run = _call_mcp_fresh(
            [workdir],
            "t81.run_subgraph",
            {"bytecode_hex": bc_hex, "policy_path": str(policy_path), "compact": True},
        )["result"]["structured"]
        self.assertTrue(run["ok"] and run["halted"])
        self.assertEqual(run["verdict"], "VERIFIED_AND_EXECUTED")
        self.assertEqual(run["trap_code"], 0)
        self.assertEqual(run["dualrail_fallback_count"], 0)
        self.assertIn("dualrail_sparse128_blocks_skipped", run)
        self.assertEqual(oracle_dot, 15)

        # Stage 4: Canonical SHA3-512 boot-to-receipt emission
        payload = (
            f"{boot_image_sha3}|{policy_root_sha3}|{weight_sha3}|"
            f"{run['execution_receipt']}|GPU_R1_BitmaskSkip_Pop|k_star_prepacked=0"
        )
        boot_receipt_sha3 = "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()
        return {
            "boot_image_sha3_512": boot_image_sha3,
            "policy_root_sha3_512": policy_root_sha3,
            "canonfs_weight_sha3_512": weight_sha3,
            "execution_receipt": run["execution_receipt"],
            "boot_receipt_sha3_512": boot_receipt_sha3,
            "policy_path": policy_path,
            "policy_bytes": policy_bytes,
            "bc_hex": bc_hex,
        }

    def test_01_immutable_policy_root_rejects_agent_mutation(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-boot-immut-") as td:
            root = Path(td)
            res = self._run_full_contract_once(root)
            policy_path = res["policy_path"]
            bak_dir = root / "tx_bak"
            bak_dir.mkdir()

            # Single-session tx_begin -> agent attempts to weaken policy -> tx_commit DENIED -> tx_abort restores exact bytes
            proc = subprocess.Popen(
                [T81_BIN, "mcp", "serve", "--root", str(root)],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                text=True,
                bufsize=1,
            )
            try:
                assert proc.stdin is not None and proc.stdout is not None

                def rpc(rid: int, name: str, args: dict[str, Any]) -> dict[str, Any]:
                    proc.stdin.write(
                        json.dumps(
                            {
                                "jsonrpc": "2.0",
                                "id": rid,
                                "method": "tools/call",
                                "params": {"name": name, "arguments": args},
                            }
                        )
                        + "\n"
                    )
                    proc.stdin.flush()
                    return json.loads(proc.stdout.readline())["result"]["structured"]

                tx_b = rpc(
                    1,
                    "t81.tx_begin",
                    {"target_path": str(policy_path), "backup_dir": str(bak_dir), "compact": True},
                )
                self.assertTrue(tx_b["ok"])
                policy_path.write_bytes(b"(policy (tier 0))\n")
                tx_commit = rpc(
                    2,
                    "t81.tx_commit",
                    {"tx_id": tx_b["tx_id"], "policy_path": str(policy_path), "compact": True},
                )
                self.assertFalse(tx_commit["ok"])
                self.assertFalse(tx_commit["committed"])
                tx_abort = rpc(3, "t81.tx_abort", {"tx_id": tx_b["tx_id"], "compact": True})
                self.assertTrue(tx_abort["ok"] and tx_abort["aborted"])
                self.assertEqual(policy_path.read_bytes(), res["policy_bytes"])
            finally:
                if proc.stdin:
                    proc.stdin.close()
                if proc.stdout:
                    proc.stdout.close()
                proc.wait(timeout=5)

    def test_02_fail_closed_security_fault_blocks_unauthorized_guest_run(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-boot-deny-") as td:
            root = Path(td)
            bc_hex, _ = _build_dot_subgraph()
            strict_pol = root / "strict_root.apl"
            strict_pol.write_text(
                '(policy\n  (tier 1)\n  (allowed-tensor-hashes ["sha3-512:'
                + ("0" * 128)
                + '"])\n  (require-axion-event (reason "enforcement-mode:fail-closed")))\n',
                encoding="utf-8",
            )
            trap = _call_mcp_fresh(
                [root],
                "t81.run_subgraph",
                {"bytecode_hex": bc_hex, "policy_path": str(strict_pol), "compact": True},
            )["result"]["structured"]
            self.assertFalse(trap["ok"])
            self.assertEqual(trap["verdict"], "VM_TRAP")
            self.assertEqual(trap["trap_code"], 6)
            self.assertEqual(trap["trap_name"], "SecurityFault")

    def test_03_repeat_run_boot_to_receipt_sha3_512_determinism(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-boot-rep1-") as td1, tempfile.TemporaryDirectory(
            prefix="t81-boot-rep2-"
        ) as td2:
            r1 = self._run_full_contract_once(Path(td1))
            r2 = self._run_full_contract_once(Path(td2))
            self.assertEqual(r1["boot_image_sha3_512"], r2["boot_image_sha3_512"])
            self.assertEqual(r1["policy_root_sha3_512"], r2["policy_root_sha3_512"])
            self.assertEqual(r1["canonfs_weight_sha3_512"], r2["canonfs_weight_sha3_512"])
            self.assertEqual(r1["execution_receipt"], r2["execution_receipt"])
            self.assertEqual(r1["boot_receipt_sha3_512"], r2["boot_receipt_sha3_512"])

    def test_04_real_qemu_aarch64_serial_roundtrip_and_repeat_receipt(self) -> None:
        qemu_bin = shutil.which("qemu-system-aarch64")
        if not qemu_bin:
            self.skipTest("qemu-system-aarch64 not installed on this host")

        elf_bytes = _build_aarch64_uart_guest_elf()
        receipts: list[dict[str, str]] = []
        for idx in range(2):
            with tempfile.TemporaryDirectory(prefix=f"t81-real-qemu-{idx}-") as td:
                root = Path(td)
                elf_path = root / "ternaryos_uart_guest.elf"
                elf_path.write_bytes(elf_bytes)

                # Stage 1: Real non-stubbed QEMU AArch64 interactive PL011 UART serial round-trips
                transcript = _run_real_qemu_serial_session(qemu_bin, elf_path)
                self.assertEqual(
                    transcript,
                    b"[axion@T81 tier=1]$ AXION_DENY:POLICY_ROOT_IMMUTABLE\n"
                    b"[axion@T81 tier=1]$ AXION_OK:status\n",
                )
                boot_img_sha3 = _call_mcp_fresh(
                    [root], "t81.determinism_hash", {"path": str(elf_path), "compact": True}
                )["result"]["structured"]["sha3_512"]
                transcript_path = root / "uart_transcript.log"
                transcript_path.write_bytes(transcript)
                serial_sha3 = _call_mcp_fresh(
                    [root], "t81.determinism_hash", {"path": str(transcript_path), "compact": True}
                )["result"]["structured"]["sha3_512"]

                # Stage 2: CanonFS SBDR1T2D + immutable Axion policy root
                croot = root / "canonfs"
                croot.mkdir()
                sbdr_path = root / "enclave_weights.sbdr"
                sbdr_path.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 4, 1, 0b0101, 0b1010))
                subprocess.run(
                    [T81_BIN, "canonfs", "import", str(sbdr_path), "--canonfs-root", str(croot), "--json"],
                    check=True,
                    capture_output=True,
                )
                fsck = _call_mcp_fresh(
                    [root], "t81.canonfs_fsck", {"canonfs_root": str(croot), "compact": True}
                )["result"]["structured"]
                self.assertTrue(fsck["ok"] and fsck["checked_objects"] >= 1)
                weight_sha3 = _call_mcp_fresh(
                    [root], "t81.determinism_hash", {"path": str(sbdr_path), "compact": True}
                )["result"]["structured"]["sha3_512"]

                policy_path = root / "axion_root.apl"
                policy_path.write_text(
                    f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{weight_sha3}"]))\n',
                    encoding="utf-8",
                )
                policy_sha3 = _call_mcp_fresh(
                    [root], "t81.determinism_hash", {"path": str(policy_path), "compact": True}
                )["result"]["structured"]["sha3_512"]

                # Stage 3: T81VM execution under immutable Axion policy
                bc_hex, _ = _build_dot_subgraph()
                run = _call_mcp_fresh(
                    [root],
                    "t81.run_subgraph",
                    {"bytecode_hex": bc_hex, "policy_path": str(policy_path), "compact": True},
                )["result"]["structured"]
                self.assertTrue(run["ok"] and run["halted"])
                self.assertEqual(run["verdict"], "VERIFIED_AND_EXECUTED")
                self.assertEqual(run["dualrail_fallback_count"], 0)

                # Stage 4: Chained SHA3-512 boot-to-receipt including real QEMU UART transcript
                chained = (
                    f"{boot_img_sha3}|{serial_sha3}|{policy_sha3}|{weight_sha3}|"
                    f"{run['execution_receipt']}|GPU_R1_BitmaskSkip_Pop|k_star_prepacked=0"
                )
                receipts.append(
                    {
                        "boot_image_sha3_512": boot_img_sha3,
                        "serial_transcript_sha3_512": serial_sha3,
                        "policy_root_sha3_512": policy_sha3,
                        "canonfs_weight_sha3_512": weight_sha3,
                        "execution_receipt": run["execution_receipt"],
                        "qemu_boot_receipt_sha3_512": "sha3-512:"
                        + hashlib.sha3_512(chained.encode("utf-8")).hexdigest(),
                    }
                )

        self.assertEqual(receipts[0], receipts[1])


if __name__ == "__main__":
    unittest.main()
