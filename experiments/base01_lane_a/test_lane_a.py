"""Contract tests use fixture values, never benchmark samples or timing."""
import contextlib
import importlib.util
import io
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("lane_a", Path(__file__).with_name("lane_a.py"))
lane = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lane)


class Contract(unittest.TestCase):
    def test_linux_macos_rss_units(self):
        self.assertEqual(lane.rss_kib(2048, "Linux"), 2048)
        self.assertEqual(lane.rss_kib(2048, "Darwin"), 2)
        self.assertEqual(lane.rss_kib(1025, "Darwin"), 1025/1024)
        for raw, system in [(-1,"Linux"),(1,"Windows")]:
            with self.assertRaises(ValueError): lane.rss_kib(raw, system)

    def test_statistics_definitions(self):
        result=lane.summarize(list(range(1,31)))  # mathematical fixture, no measurements
        self.assertEqual(result["median_ns"],15.5)
        self.assertEqual(result["iqr_ns"],14.5)
        self.assertAlmostEqual(result["cv_pct"],56.79618342470648)
        self.assertEqual(result["status"],"UNSTABLE / INCONCLUSIVE")
        self.assertEqual(lane.summarize([10]*30)["cv_pct"],0)
        for bad in ([1]*29,[0]*30,[math.inf]*30,[math.nan]*30,[-1]*30):
            with self.assertRaises(ValueError): lane.summarize(bad)

    def test_measure_gate_precedes_all_work(self):
        with patch.object(sys,"argv",["lane_a.py","--measure"]), \
             patch.object(lane,"validate") as validate, \
             patch.object(lane,"command") as command, contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error: lane.main()
            self.assertEqual(error.exception.code,2)
            validate.assert_not_called()
            command.assert_not_called()

    def test_no_environment_override(self):
        with patch.dict(lane.os.environ, {"BASE01_MEASUREMENT_AUTHORIZED":"1"}):
            result=subprocess.run([sys.executable,str(lane.ROOT/"lane_a.py"),"--measure"],capture_output=True)
            self.assertEqual(result.returncode,2)
            self.assertIn(b"disabled in B0",result.stderr)

    def test_frozen_and_unknown_flags_rejected(self):
        for arg in ("--cmp3","--h04-r03-phase-c","--authorize-base01-measurement"):
            result=subprocess.run([sys.executable,str(lane.ROOT/"lane_a.py"),"--validate-only",arg],capture_output=True)
            self.assertEqual(result.returncode,2)

    def test_codec_oracle_rejects_tampering(self):
        with self.assertRaises(ValueError): lane.verify_codec([[0,0,[0]]])

    def test_source_identity_fails_wrong_revision_and_dirty_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            subprocess.run(["git","init",tmp],check=True,capture_output=True)
            file=root/"data"
            file.write_text("original")
            subprocess.run(["git","-C",tmp,"add","data"],check=True)
            subprocess.run(["git","-C",tmp,"-c","user.name=fixture","-c","user.email=fixture@example.invalid",
                            "commit","-m","fixture"],check=True,capture_output=True)
            with self.assertRaises(ValueError): lane.verify_source(root)
            fixture_sha=lane.command(["git","-C",root,"rev-parse","HEAD"]).strip()
            with patch.object(lane,"SOURCE_SHA",fixture_sha):
                self.assertEqual(len(lane.verify_source(root)),64)
                file.write_text("modified")
                with self.assertRaises(ValueError): lane.verify_source(root)


if __name__=="__main__": unittest.main()
