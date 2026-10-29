#!/usr/bin/env python3
"""Exercise the real checker CLI on isolated source trees, without configuring T81."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


CHECKER = Path(__file__).resolve().parents[2] / "scripts/ci/check_architecture_targets.py"


class TargetSync(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="t81-target-sync-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "repo"
        self.write("scripts/ci/check_architecture_targets.py", CHECKER.read_text(encoding="utf-8"))
        self.write("docs/explanation/ARCHITECTURE.md", "| `t81_core` | STATIC |\n")
        self.write("CMakeLists.txt", "add_subdirectory(src)\nadd_subdirectory(benchmarks)\n")
        self.write("src/CMakeLists.txt", "add_subdirectory(core)\n")
        self.write("src/core/CMakeLists.txt", "add_library(t81_core INTERFACE)\n")
        self.write("benchmarks/CMakeLists.txt", "add_executable(benchmark_runner bench.cpp)\n")

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def run_checker(self, expected, diagnostic=""):
        result = subprocess.run(
            [sys.executable, str(self.root / "scripts/ci/check_architecture_targets.py")],
            cwd=self.root, text=True, capture_output=True, timeout=10,
        )
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        self.assertIn(diagnostic, result.stdout)
        return result.stdout

    def test_nested_source_declarations(self):
        self.run_checker(0, "4 reachable CMakeLists.txt files")

    def test_quoted_multiline_paths_and_case_insensitive_commands(self):
        (self.root / "src/core").rename(self.root / "src/core dir")
        self.write("src/CMakeLists.txt", 'ADD_SUBDIRECTORY (\n "core dir"\n)\n')
        self.write("src/core dir/CMakeLists.txt", 'ADD_LIBRARY (\n "t81_core" INTERFACE\n)\n')
        self.run_checker(0)

    def test_annotated_benchmark_row_is_checked(self):
        self.write("docs/explanation/ARCHITECTURE.md",
                   "| `t81_core` | STATIC |\n| `benchmark_runner` (subdir) | EXECUTABLE |\n")
        self.run_checker(0, "2 documented targets")
        self.write("benchmarks/CMakeLists.txt", "# no benchmark declaration\n")
        self.run_checker(1, "benchmark_runner")

    def test_optional_pybind_module_name_is_not_exempt(self):
        self.write("docs/explanation/ARCHITECTURE.md", "| `_t81` | MODULE (optional) |\n")
        self.write("CMakeLists.txt", "if(T81_BUILD_PYTHON_BINDINGS)\n"
                   "  pybind11_add_module(_t81 bindings.cpp)\nendif()\n"
                   "add_subdirectory(benchmarks)\n")
        self.run_checker(0)
        self.write("CMakeLists.txt", "add_subdirectory(benchmarks)\n")
        self.run_checker(1, "_t81")

    def test_obsolete_python_name_still_fails(self):
        self.write("docs/explanation/ARCHITECTURE.md", "| `t81_python` | MODULE |\n")
        self.write("src/core/CMakeLists.txt", "pybind11_add_module(_t81 bindings.cpp)\n")
        self.run_checker(1, "t81_python")

    def test_removed_target_cannot_be_satisfied_by_disconnected_copy(self):
        self.write("src/core/CMakeLists.txt", "# active declaration removed\n")
        self.write("old-src/CMakeLists.txt", "add_library(t81_core INTERFACE)\n")
        self.write("build/generated/CMakeLists.txt", "add_library(t81_core INTERFACE)\n")
        self.run_checker(1, "t81_core")

    def test_removed_subdirectory_edge_does_not_scan_orphaned_source(self):
        self.write("src/CMakeLists.txt", "# add_subdirectory(core)\n")
        self.run_checker(1, "t81_core")

    def test_comments_cannot_declare_targets(self):
        self.write("src/core/CMakeLists.txt", "# add_library(t81_core INTERFACE)\n"
                   "#[=[\nadd_library(t81_core INTERFACE)\n]=]\n")
        self.run_checker(1, "t81_core")

    def test_quoted_or_bracket_messages_cannot_declare_targets(self):
        self.write("src/core/CMakeLists.txt", 'message("example:\n'
                   'add_library(t81_core INTERFACE)\n")\n'
                   'message([=[\nadd_library(t81_core INTERFACE)\n]=])\n')
        self.run_checker(1, "t81_core")

    def test_uninvoked_function_and_macro_cannot_declare_targets(self):
        self.write("src/core/CMakeLists.txt", "function(example)\n"
                   "add_library(t81_core INTERFACE)\nendfunction()\n"
                   "macro(other)\nadd_library(t81_core INTERFACE)\nendmacro()\n")
        self.run_checker(1, "t81_core")

    def test_computed_target_is_not_guessed(self):
        self.write("src/core/CMakeLists.txt", "set(name t81_core)\nadd_library(${name} INTERFACE)\n")
        self.run_checker(1, "t81_core")

    def test_computed_subdirectory_is_not_guessed(self):
        self.write("src/CMakeLists.txt", "set(name core)\nadd_subdirectory(${name})\n")
        self.run_checker(1, "t81_core")

    def test_missing_reachable_file_is_diagnostic_failure(self):
        (self.root / "src/core/CMakeLists.txt").unlink()
        self.run_checker(1, "cannot inventory CMake declarations")

    def test_missing_required_benchmark_file_still_fails(self):
        (self.root / "benchmarks/CMakeLists.txt").unlink()
        self.run_checker(1, "error: missing")

    def test_source_edge_cannot_escape_repository(self):
        outside = Path(self.temporary.name) / "outside"
        outside.mkdir()
        (outside / "CMakeLists.txt").write_text("add_library(t81_core INTERFACE)\n")
        self.write("CMakeLists.txt", "add_subdirectory(../outside)\n")
        self.run_checker(1, "escapes repository")

    def test_missing_or_empty_expectations_fail(self):
        (self.root / "docs/explanation/ARCHITECTURE.md").unlink()
        self.run_checker(1, "error: missing")
        self.write("docs/explanation/ARCHITECTURE.md", "# empty target table\n")
        self.run_checker(1, "no documented targets")

    def test_renamed_target_fails_with_exact_name(self):
        self.write("src/core/CMakeLists.txt", "add_library(t81_Core INTERFACE)\n")
        self.run_checker(1, "  - t81_core")

    def test_unbalanced_cmake_command_fails(self):
        self.write("src/core/CMakeLists.txt", "add_library(t81_core INTERFACE\n")
        self.run_checker(1, "unclosed CMake command")


if __name__ == "__main__":
    unittest.main(verbosity=2)
