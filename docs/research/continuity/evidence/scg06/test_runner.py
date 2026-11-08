"""
Process-Safe Deterministic Subprocess Reproduction Battery for F1–F7.
Executes small, deterministic reproductions across Pre-SCG-03 and Post-SCG-03 baselines.
"""

import subprocess
import json
import os
import sys

PRE_INC = "/tmp/scg06_worktrees/pre_scg03/include"
POST_INC = "/tmp/scg06_worktrees/post_scg03/include"
TMP_DIR = "/tmp/scg06_bin"
os.makedirs(TMP_DIR, exist_ok=True)

def compile_and_run(src_code, include_dir, extra_flags=None, timeout_sec=5):
    extra_flags = extra_flags or []
    src_file = os.path.join(TMP_DIR, "test.cpp")
    bin_file = os.path.join(TMP_DIR, "test.bin")
    with open(src_file, "w") as f:
        f.write(src_code)

    cmd_compile = ["g++", "-std=c++20", f"-I{include_dir}"] + extra_flags + [src_file, "-o", bin_file]
    res_c = subprocess.run(cmd_compile, capture_output=True, text=True)
    if res_c.returncode != 0:
        return {
            "status": "COMPILE_ERROR",
            "exit_code": res_c.returncode,
            "stdout": res_c.stdout,
            "stderr": res_c.stderr
        }

    try:
        res_r = subprocess.run([bin_file], capture_output=True, text=True, timeout=timeout_sec)
        return {
            "status": "NORMAL" if res_r.returncode == 0 else "ERROR",
            "exit_code": res_r.returncode,
            "stdout": res_r.stdout,
            "stderr": res_r.stderr
        }
    except subprocess.TimeoutExpired:
        return {
            "status": "TIMEOUT",
            "exit_code": -1,
            "stdout": "",
            "stderr": f"Timed out after {timeout_sec}s"
        }

print("Test runner initialized.")
