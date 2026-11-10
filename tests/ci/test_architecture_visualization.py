"""
test_architecture_visualization.py
Pytest suite verifying the structural integrity of the T81 architecture visualization datasets.
"""

import subprocess
import sys
import os

def test_architecture_visualization_integrity():
    script = os.path.join("scripts", "ci", "validate_architecture_visualization.py")
    res = subprocess.run([sys.executable, script], capture_output=True, text=True)
    assert res.returncode == 0, f"Validation script failed:\n{res.stdout}\n{res.stderr}"
    assert "All Visualization Data Invariants Verified Successfully!" in res.stdout
