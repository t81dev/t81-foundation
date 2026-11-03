"""Correctness fixtures only. Optional native/raw paths enable actual worker checks."""
import ast
import contextlib
import copy
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parent))
import harness as h
import observers as o
import oracle

NATIVE = os.environ.get("BASE01_FOUNDATION_NATIVE")
RAW = os.environ.get("BASE01_FOUNDATION_RAW")


def elf_fixture():
    names=b"\0.text\0.shstrtab\0.bss\0"
    start=64+4+len(names)
    header=struct.pack("<16sHHIQQQIHHHHHH",b"\x7fELF\x02\x01\x01"+b"\0"*9,1,62,1,0,0,start,0,64,0,0,64,4,2)
    section=lambda *x:struct.pack("<IIQQQQIIQQ",*x)
    return (header+b"DATA"+names+section(0,0,0,0,0,0,0,0,0,0)+
            section(1,1,6,4096,64,4,0,0,4,0)+section(7,3,0,0,68,len(names),0,0,1,0)+
            section(17,8,3,8192,999999,64,0,0,8,0))


def macho_fixture():
    command_size=72+2*80
    offset=32+command_size
    header=struct.pack("<IiiIIIII",0xfeedfacf,0x100000c,0,2,1,command_size,0,0)
    segment=struct.pack("<II16sQQQQiiII",0x19,command_size,b"__DATA",4096,8192,0,offset+4,3,3,2,0)
    section=lambda name,addr,size,off,flags:struct.pack("<16s16sQQIIIIIIII",name,b"__DATA",addr,size,off,2,0,0,flags,0,0,0)
    return header+segment+section(b"__data",4096+offset,4,offset,0)+section(b"__bss",8192,64,0,1)+b"DATA"


class BoundaryTests(unittest.TestCase):
    def test_python_rejects_all_measurement_requests_before_work(self):
        base=["--validate-only","--source","/unused","--source-build","/unused","--build","/unused","--output","/unused"]
        flags=["--measure","--timing","--time","--warmup","--warm-up","--pilot","--trial","--campaign","--permit","--authorize","--me"]
        with patch.object(h,"validate") as validate, patch.object(h,"verify_source") as source, \
             patch.object(h.subprocess,"Popen") as popen, patch.object(h.subprocess,"run") as run, \
             patch.dict(os.environ,{"BASE01_MEASUREMENT_AUTHORIZED":"1","BASE01_PHASE_B1_APPROVED":"1"}):
            for flag in flags:
                with self.subTest(flag=flag),contextlib.redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as e: h.main(base+[flag])
                    self.assertEqual(e.exception.code,2)
            validate.assert_not_called();source.assert_not_called();popen.assert_not_called();run.assert_not_called()

    @unittest.skipUnless(NATIVE,"set BASE01_FOUNDATION_NATIVE for native rejection checks")
    def test_native_rejects_measurement_before_any_output(self):
        env=dict(os.environ,BASE01_MEASUREMENT_AUTHORIZED="1",BASE01_PHASE_B1_APPROVED="1")
        with tempfile.TemporaryDirectory() as tmp:
            for args in [[],["--measure"],["--timing"],["--warmup"],["--pilot"],["--trial"],["--campaign"],["--permit"],
                         ["--validate-only","--measure"],["--validate-only","--permit"]]:
                with self.subTest(args=args):
                    p=subprocess.run([NATIVE,*args],cwd=tmp,env=env,capture_output=True)
                    self.assertEqual(p.returncode,2);self.assertEqual(p.stdout,b"")
                    self.assertEqual(list(Path(tmp).iterdir()),[])

    @unittest.skipUnless(NATIVE,"set BASE01_FOUNDATION_NATIVE for storage gate check")
    def test_native_rejects_persistent_root_before_work(self):
        with tempfile.TemporaryDirectory() as tmp:
            target=Path(tmp)/"must-not-exist"
            p=subprocess.run([NATIVE,"--validate-only"],env=dict(os.environ,T81_CANONFS_ROOT=str(target),T81_CANONFS_READ_VERIFY="1"),capture_output=True)
            self.assertEqual(p.returncode,2);self.assertEqual(p.stdout,b"");self.assertFalse(target.exists())

    def test_unsupported_execution_platform_precedes_source_access(self):
        with patch.object(h.platform,"system",return_value="Windows"),patch.object(h,"verify_source") as source:
            with self.assertRaisesRegex(ValueError,"unsupported execution platform"): h.validate(SimpleNamespace())
            source.assert_not_called()

    def test_stale_native_identity_rejected_before_worker_launch(self):
        with tempfile.TemporaryDirectory() as tmp:
            output=Path(tmp)/"must-not-exist"
            args=SimpleNamespace(source=Path("/source"),source_build=Path("/source-build"),
                                 build=Path("/native-build"),output=output)
            wrong=SimpleNamespace(returncode=0,stdout=json.dumps({"native_identity":"wrong"}))
            with patch.object(h,"verify_source",return_value="source-hash"), \
                 patch.object(h,"verify_builds",return_value="compiler"), \
                 patch.object(h.subprocess,"run",return_value=wrong) as preflight, \
                 patch.object(h.subprocess,"Popen") as worker:
                with self.assertRaisesRegex(ValueError,"stale/unknown native"):
                    h.validate(args)
                worker.assert_not_called()
                self.assertFalse(output.exists())
                self.assertEqual(preflight.call_args.args[0][-1],"--identity-only")

    def test_entire_manifest_is_pinned(self):
        raw=(h.ROOT/"phase-a-manifest.json").read_bytes();h.verify_manifest(raw)
        for key,value in [("measurement_authorized",True),("protocol","other"),("version","other")]:
            data=json.loads(raw);data[key]=value
            with self.assertRaises(ValueError): h.verify_manifest(json.dumps(data).encode())
        with self.assertRaises(ValueError): h.verify_manifest(raw+b"\n")

    def test_dirty_or_wrong_source_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/"data").write_text("original")
            subprocess.run(["git","init",tmp],check=True,capture_output=True)
            subprocess.run(["git","-C",tmp,"add","data"],check=True)
            subprocess.run(["git","-C",tmp,"-c","core.hooksPath=/dev/null","-c","user.name=fixture","-c","user.email=fixture@example.invalid","commit","-m","fixture"],check=True,capture_output=True)
            with self.assertRaises(ValueError):h.verify_source(root)
            revision=h.git(root,"rev-parse","HEAD").decode().strip()
            with patch.object(h,"SOURCE",revision):
                self.assertEqual(len(h.verify_source(root)),64)
                (root/"data").write_text("changed")
                with self.assertRaises(ValueError):h.verify_source(root)

    def test_existing_independent_oracle_is_verbatim(self):
        source=(h.ROOT/"oracle.py").read_text()
        function=next(n for n in ast.parse(source).body if isinstance(n,ast.FunctionDef) and n.name=="verify_codec")
        # Digest verified against function source at accepted B0 commit 91e3a7d7.
        self.assertEqual(hashlib.sha256(ast.get_source_segment(source,function).encode()).hexdigest(),
                         "2cd85af5fe953950ca5da5b2a60672394241e615b5789b4141e7c6996a36a403")


class ArtifactTests(unittest.TestCase):
    def test_elf64_native_names_and_nobits(self):
        raw=elf_fixture();r=o.artifact_metadata(raw)
        self.assertEqual(r["format"],"ELF64-LE")
        self.assertEqual(r["sha256"],hashlib.sha256(raw).hexdigest())
        self.assertEqual(r["sections"][1]["name"],".text")
        self.assertEqual(r["sections"][3]["memory_bytes"],64)
        self.assertEqual(r["sections"][3]["file_bytes"],0)
        self.assertEqual(r["file_bytes"],len(raw))

    def test_elf64_segment_metadata(self):
        header=struct.pack("<16sHHIQQQIHHHHHH",b"\x7fELF\x02\x01\x01"+b"\0"*9,2,62,1,4096,64,0,0,64,56,1,0,0,0)
        segment=struct.pack("<IIQQQQQQ",1,5,120,4096,4096,4,8,4096)
        r=o.artifact_metadata(header+segment+b"DATA")
        self.assertEqual(r["segments"][0]["file_bytes"],4)
        self.assertEqual(r["segments"][0]["memory_bytes"],8)
        bad=bytearray(header+segment+b"DATA");struct.pack_into("<Q",bad,64+40,2)
        with self.assertRaises(ValueError):o.artifact_metadata(bytes(bad))

    def test_elf_malformed_tables_rejected(self):
        raw=elf_fixture()
        variants=[raw[:10],raw[:-1]]
        bad=bytearray(raw);struct.pack_into("<H",bad,60,4097);variants.append(bytes(bad))
        bad=bytearray(raw);struct.pack_into("<H",bad,58,63);variants.append(bytes(bad))
        for raw in variants:
            with self.subTest(length=len(raw)),self.assertRaises(ValueError):o.artifact_metadata(raw)

    def test_macho_native_names_and_zero_fill(self):
        raw=macho_fixture();r=o.artifact_metadata(raw)
        self.assertEqual(r["format"],"Mach-O64-LE")
        self.assertEqual(r["sections"][0]["name"],"__data")
        self.assertEqual(r["sections"][1]["name"],"__bss")
        self.assertEqual(r["sections"][1]["file_bytes"],0)
        self.assertEqual(r["sections"][1]["memory_bytes"],64)

    def test_macho_invalid_commands_and_ranges_rejected(self):
        raw=macho_fixture();variants=[raw[:20],raw[:-1]]
        bad=bytearray(raw);struct.pack_into("<I",bad,32+4,8);variants.append(bytes(bad))
        bad=bytearray(raw);struct.pack_into("<Q",bad,32+72+32,999999);variants.append(bytes(bad))
        for raw in variants:
            with self.subTest(length=len(raw)),self.assertRaises(ValueError):o.artifact_metadata(raw)

    def test_unsupported_formats_are_explicit(self):
        for data in [b"!<arch>\n",b"MZ",b"\xca\xfe\xba\xbe",b"\x7fELF\x01\x01\x01"+b"\0"*9,
                     b"\x7fELF\x02\x02\x01"+b"\0"*9]:
            self.assertEqual(o.artifact_metadata(data)["status"],"UNSUPPORTED")

    def test_parser_memory_bound(self):
        with patch.object(o,"MAX_ARTIFACT",10):
            with self.assertRaises(ValueError):o.artifact_metadata(b"x"*11)

    @unittest.skipUnless(NATIVE,"set BASE01_FOUNDATION_NATIVE for actual artifact parse")
    def test_actual_worker_artifact(self):
        r=o.artifact_metadata(Path(NATIVE).read_bytes())
        self.assertEqual(r["status"],"SUPPORTED");self.assertTrue(r["sections"])
        self.assertEqual(r["sha256"],hashlib.sha256(Path(NATIVE).read_bytes()).hexdigest())


class AccountingTests(unittest.TestCase):
    def test_rss_normalization_and_unsupported(self):
        self.assertEqual(o.rss_kib(2048,"Linux"),2048)
        self.assertEqual(o.rss_kib(2048,"Darwin"),2)
        self.assertEqual(o.rss_kib(1025,"Darwin"),1025/1024)
        for value in [-1,float("inf"),float("nan"),True]:
            with self.assertRaises(ValueError):o.rss_kib(value,"Linux")
        with self.assertRaises(ValueError):o.rss_kib(1,"Windows")

    def test_selector_semantics(self):
        self.assertIn("calling worker",o.selector_scope("RUSAGE_SELF",41,41))
        self.assertIn("reaped worker",o.selector_scope("WAITED_CHILD",41,41,reaped=True))
        for selector,pid,reaped in [("RUSAGE_SELF",42,False),("RUSAGE_CHILDREN",41,True),
                                    ("WAITED_CHILD",41,False),("unknown",41,False)]:
            with self.assertRaises(ValueError):o.selector_scope(selector,41,pid,reaped=reaped)

    def test_lifecycle_identity_and_order(self):
        records=[{"event":"spawned","pid":41,"parent_pid":40},{"event":"exited","pid":41,"returncode":0}]
        self.assertEqual(o.validate_lifecycle(records)["resource_samples"],[])
        for bad in [records[::-1],[records[0],dict(records[1],pid=42)],[records[0]],
                    [dict(records[0],parent_pid=0),records[1]]]:
            with self.assertRaises(ValueError):o.validate_lifecycle(bad)

    def test_constant_bindings_and_no_sampling(self):
        self.assertEqual(o.selector_bindings("Windows")["status"],"UNSUPPORTED")
        if sys.platform not in ("darwin","linux"):
            self.skipTest("actual POSIX constants unavailable")
        import resource
        with patch.object(resource,"getrusage",side_effect=AssertionError("must not sample")):
            self.assertFalse(o.selector_bindings("Darwin" if sys.platform=="darwin" else "Linux")["samples_collected"])
            self.assertEqual(o.rss_kib(1024,"Darwin"),1)
            o.selector_scope("RUSAGE_SELF",41,41)


@unittest.skipUnless(RAW,"set BASE01_FOUNDATION_RAW to correctness-only native evidence")
class WorkloadTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=json.loads(gzip.decompress(Path(RAW).read_bytes()))

    def test_codec_retention_and_independent_oracle(self):
        result=h.verify_codec_record(self.data["codec"])
        self.assertEqual(result["input_sha256"],h.CODEC_INPUT)
        for kind in ("digit","decoded","count"):
            bad=copy.deepcopy(self.data["codec"])
            if kind=="digit":bad["encoded"][0][1][0]=2
            elif kind=="decoded":bad["decoded"][0]+=1
            else:bad["retention"]["encoded_outputs"]-=1
            with self.assertRaises(ValueError):h.verify_codec_record(bad)

    def test_canonfs_independent_hash_and_bytes(self):
        result=h.verify_canonfs(self.data["canonfs"])
        self.assertEqual([x["bytes"] for x in result],list(h.SIZES))
        for kind in ("hash","bytes","count"):
            bad=copy.deepcopy(self.data["canonfs"])
            if kind=="hash":bad[0]["hashes"][0]="00"*32
            elif kind=="bytes":bad[0]["read_outputs_hex"][0]="00"+bad[0]["read_outputs_hex"][0][2:]
            else:bad[0]["retained_reads"]=1
            with self.assertRaises(ValueError):h.verify_canonfs(bad)

    def test_vm_provisional_scope_and_rejections(self):
        self.assertEqual(h.verify_vm(self.data["vm"])["status"],"PROVISIONAL PASS")
        for field,value in [("status","PASS"),("initial_r75",1),("terminal_pc",0),("halted",False),("final_r75",999),("memory_words",999999)]:
            bad=copy.deepcopy(self.data["vm"]);bad[0][field]=value
            with self.assertRaises(ValueError):h.verify_vm(bad)
        bad=copy.deepcopy(self.data["vm"]);bad[5]["tags"][44]=0
        with self.assertRaises(ValueError):h.verify_vm(bad)

    def test_native_source_binding(self):
        self.assertEqual(self.data["native_identity"],h.native_identity())

    def test_no_workload_promoted_to_measurement(self):
        for row in h.readiness().values():
            self.assertFalse(row["eligibility"]["measurement"])
            self.assertFalse(row["eligibility"]["qualification"])
        self.assertEqual(h.readiness()[h.IDS[1]]["engineering"],h.PARTIAL)


if __name__=="__main__":unittest.main()
