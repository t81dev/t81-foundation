"""Compile the repository's MSVC Debug flags with the actual Windows compiler."""
import argparse
import json
from pathlib import Path
import platform
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--generator", required=True)
    args = parser.parse_args()
    if platform.system() != "Windows":
        parser.error("This regression requires a real Windows/MSVC environment")
    records = []
    with tempfile.TemporaryDirectory(prefix="base01-msvc-debug-") as directory:
        source = Path(directory)
        build = source / "build"
        flags = (args.source.resolve() / "cmake/Compiler.cmake").as_posix()
        (source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.20)\n"
            "project(msvc_debug_regression LANGUAGES CXX)\n"
            "if(NOT MSVC)\n  message(FATAL_ERROR \"MSVC is required\")\nendif()\n"
            "set(T81_USE_CXX23 ON)\n"
            f"include([=[{flags}]=])\n"
            "add_executable(probe main.cpp)\n"
            "target_compile_features(probe PRIVATE ${T81_CXX_STD_FEATURE})\n",
            encoding="utf-8")
        (source / "main.cpp").write_text("int main() { return 0; }\n", encoding="utf-8")
        commands = [
            [args.cmake, "-S", str(source), "-B", str(build), "-G", args.generator,
             "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_CXX_COMPILER=" + args.compiler],
            [args.cmake, "--build", str(build), "--config", "Debug", "--verbose"],
        ]
        for command in commands:
            result = subprocess.run(command, capture_output=True, timeout=60)
            records.append({"argv": command, "returncode": result.returncode,
                            "stdout": result.stdout.decode("utf-8", errors="replace"),
                            "stderr": result.stderr.decode("utf-8", errors="replace")})
            if result.returncode != 0:
                print(json.dumps({"status": "FAIL", "commands": records}))
                return 1
        binary = next((p for p in [build / "probe.exe", build / "Debug/probe.exe"]
                       if p.is_file()), None)
        if binary is None:
            print(json.dumps({"status": "FAIL", "error": "probe not built", "commands": records}))
            return 1
        result = subprocess.run([str(binary)], capture_output=True, timeout=15)
        records.append({"argv": [str(binary)], "returncode": result.returncode})
        print(json.dumps({"status": "PASS" if result.returncode == 0 else "FAIL",
                          "commands": records}))
        return 0 if result.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
