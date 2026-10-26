#!/usr/bin/env python3
"""Exercise both governance CLIs in isolated source/summary fixtures.

These tests cover lexical contract enforcement, not VM execution or the truth
of a scientific claim. Expected failures name the contract being violated.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[2]
COHERENCE = "scripts/ci/check_architecture_coherence.py"
CLAIMS = "scripts/governance/check_determinism_claims.py"
INPUTS = (
    "include/t81/isa/opcodes.hpp", "vm/vm.cpp", "vm/tensor_helpers.cpp",
    "include/t81/axion/reasons.hpp", "vm/policy_trace_bridge.cpp",
    "include/t81/tensor/native.hpp", "CMakeLists.txt",
    "src/axion/CMakeLists.txt", ".github/workflows/ci.yml",
    "include/t81/isa/ai_native_opcodes.hpp",
    "scripts/ci/ai_wload_policy_evidence_expectations.json",
)


class GovernanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for name in (*INPUTS, COHERENCE, CLAIMS):
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(REPO / name, target)

    def run_cli(self, script):
        return subprocess.run([sys.executable, str(self.root / script)],
                              cwd=self.root, capture_output=True, text=True,
                              timeout=10)

    def test_current_layout_without_obsolete_core_tree(self):
        self.assertFalse((self.root / "core").exists())
        result = self.run_cli(COHERENCE)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_each_required_contract_remains_enforced(self):
        mutations = [
            ("include/t81/isa/opcodes.hpp", "ATTN", "REMOVED_ATTN", "ISA opcode ATTN"),
            ("vm/vm.cpp", "Opcode::ATTN", "Opcode::REMOVED_ATTN", "VM dispatch for ATTN"),
            ("vm/vm.cpp", "Opcode::QMATMUL", "Opcode::REMOVED_QMATMUL", "VM dispatch for QMATMUL"),
            ("vm/vm.cpp", "Opcode::EMBED", "Opcode::REMOVED_EMBED", "VM dispatch for EMBED"),
            ("include/t81/isa/ai_native_opcodes.hpp", "#pragma once", "#pragma once\n// create_opcode_handler", "placeholder handler"),
            ("src/axion/CMakeLists.txt", "kernel/axion/policy_engine.cpp", "kernel/axion/removed.cpp", "kernel PolicyEngine source"),
            ("src/axion/CMakeLists.txt", "kernel/axion/policy_engine.cpp", "src/axion/policy_engine.cpp", "canonical source must be kernel/axion"),
            (".github/workflows/ci.yml", "- t3k-cross-arch-bit-identity", "- removed-t3k", "quality-gate dependency on t3k"),
            (".github/workflows/ci.yml", "- t81lang-cross-arch-bit-identity", "- removed-lang", "quality-gate dependency on t81lang"),
            ("scripts/ci/ai_wload_policy_evidence_expectations.json", "AI_POLICY_ALLOW_WLOAD_POLICY_GATE", "REMOVED_ALLOW", "WLOAD expectation missing allow"),
            ("scripts/ci/ai_wload_policy_evidence_expectations.json", "AI_POLICY_DENY_WLOAD_UNSUPPORTED", "REMOVED_DENY", "WLOAD expectation missing deny"),
            ("include/t81/axion/reasons.hpp", "canonical_tensor_provenance_reason", "removed_reason", "Axion tensor provenance reason builder"),
            ("vm/policy_trace_bridge.cpp", 'event_type = "tensor_provenance"', 'event_type = "removed"', "structured event emission"),
            ("vm/policy_trace_bridge.cpp", 'reason_code = "VM_TENSOR_PROVENANCE"', 'reason_code = "REMOVED"', "reason code emission"),
            ("vm/vm.cpp", "log_tensor_provenance", "removed_logging", "VM tensor provenance logging hook"),
            ("CMakeLists.txt", "t81_vm_tensor_provenance_trace_test", "removed_trace_test", "tensor provenance regression target"),
            ("CMakeLists.txt", "t81_vm_ai_phase1_shared_helper_parity_test", "removed_parity_test", "phase-1 VM/shared helper parity regression target"),
            ("include/t81/tensor/native.hpp", "namespace t81::tensor_native", "namespace removed", "shared tensor native decode surface"),
            ("vm/tensor_helpers.cpp", "tensor_native::decode", "removed::decode", "VM tensor helpers routing native decode"),
            ("CMakeLists.txt", "t81_tensor_native_decode_test", "removed_decode_test", "shared tensor native decode regression target"),
        ]
        for name, old, new, diagnostic in mutations:
            with self.subTest(contract=diagnostic):
                path = self.root / name
                text = path.read_text()
                self.assertIn(old, text)
                changed = text.replace(old, new)
                if name == "include/t81/axion/reasons.hpp":
                    # Either supported builder surface satisfies this contract;
                    # remove both to construct a real negative fixture.
                    changed = changed.replace("kTensorProvenance", "removed_provenance")
                path.write_text(changed)
                try:
                    result = self.run_cli(COHERENCE)
                    self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                    self.assertIn(diagnostic, result.stdout)
                finally:
                    path.write_text(text)

    def test_old_copy_cannot_replace_missing_current_source(self):
        current = self.root / "vm/vm.cpp"
        stale = self.root / "core/vm/vm.cpp"
        stale.parent.mkdir(parents=True)
        current.rename(stale)
        self.assertNotEqual(self.run_cli(COHERENCE).returncode, 0)

    def test_readme_boundary_and_its_removal(self):
        text = (REPO / "README.md").read_text()
        readme = self.root / "README.md"
        readme.write_text(text)
        self.assertEqual(self.run_cli(CLAIMS).returncode, 0)
        begin = text.index("Determinism guarantees are limited")
        end = text.index("\n\n", begin)
        readme.write_text(text[:begin] + text[end:])
        result = self.run_cli(CLAIMS)
        self.assertEqual(result.returncode, 1)
        self.assertIn("README.md: claims determinism but omits", result.stderr)

    def test_independent_summary_cases(self):
        cases = [
            ("Artifact inspection only.", 0),
            ("Deterministic execution for every backend.", 1),
            ("DETERMINISM guarantees without a boundary.", 1),
            ("Deterministic execution is bounded by the Determinism Surface Registry.", 0),
            ("Deterministic execution: docs/governance/DETERMINISM_SURFACE_REGISTRY.md", 0),
        ]
        for text, expected in cases:
            with self.subTest(summary=text):
                (self.root / "README.md").write_text(text)
                self.assertEqual(self.run_cli(CLAIMS).returncode, expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
