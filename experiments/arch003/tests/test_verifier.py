"""Fixed evidence fixtures only; no candidate process, oracle stream, or campaign."""
import hashlib
import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import evidence as E
import verifier as V


class ParserChecks(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.root=Path(self.tmp.name)
        # Family-0 empty BYTE request selects only B-BYTE (implementation 1).
        self.row=(0,0,0,0,0,0,0,0,0,0,0,0,0,2,bytes(32))
        self.expected=self.root/'expected.bin'
        self.expected.write_bytes(E.HEADER.pack(b'A2R1',0,255,0,0,1)+E.EXPECTED.pack(*self.row))
        self.observed=(0,1,0,1,0,0,E.fold(E.D0,0,0),0)

    def tearDown(self):
        self.tmp.cleanup()

    def verify(self,row=None,tail=b''):
        return V.verify(self.expected,io.BytesIO(E.OBSERVED.pack(*(row or self.observed))+tail),full=False)

    def test_valid_fixed_frame(self):
        self.assertEqual(self.verify()[0],1)

    def test_rejections(self):
        mutations={0:1,1:0,2:7,3:0,4:1,6:0,7:1}
        for field,value in mutations.items():
            with self.subTest(field=field):
                row=list(self.observed);row[field]=value
                with self.assertRaises(V.Failure):self.verify(row)
        with self.assertRaises(V.Failure):self.verify(tail=b'x')
        with self.assertRaises(V.Failure):V.verify(self.expected,io.BytesIO(b'\0'),full=False)
        with self.assertRaises(V.Failure):V.verify(self.expected,io.BytesIO(b''),full=False)

    def test_wrong_count_with_correct_digest_fails_semantics(self):
        row=list(self.observed);row[5]=1;row[6]=E.fold(E.D0,0,1)
        with self.assertRaises(V.Failure) as exc:self.verify(row)
        self.assertEqual(exc.exception.status,65)

    def test_error_null_and_unknown_status(self):
        row=list(self.observed);row[2]=5;row[3]=0;row[5]=1;row[6]=E.fold(E.D0,5,1)
        with self.assertRaises(V.Failure):self.verify(row)

    def test_expected_malformed(self):
        for field,value in [(0,1),(1,12),(4,7),(9,1),(10,32),(12,2),(13,64)]:
            with self.subTest(field=field):
                row=list(self.row);row[field]=value
                self.expected.write_bytes(E.HEADER.pack(b'A2R1',0,255,0,0,1)+E.EXPECTED.pack(*row))
                with self.assertRaises(V.Failure):self.verify()

    def test_header(self):
        for idx in range(5):
            args=[b'A2R1',0,255,0,0,1]
            args[idx]=b'FAIL' if idx==0 else (args[idx]+1)%256
            with self.assertRaises(V.Failure):V.header(io.BytesIO(E.HEADER.pack(*args)),0,255,0)

    def test_full_campaign_cannot_pass_prefix(self):
        with self.assertRaises(V.Failure):V.verify(self.expected,io.BytesIO(E.OBSERVED.pack(*self.observed)))

    def test_canonical_json(self):
        self.assertEqual(E.canonical({'z':'é','a':1}),b'{"a":1,"z":"\\u00e9"}\n')
        for value in [1.0,float('nan'),-1,2**53,{'é':1}]:
            with self.assertRaises(ValueError):E.canonical(value)

    def test_index_root_and_tamper(self):
        E.write_json(self.root/'receipt.json',{'ok':True})
        E.seal(self.root);self.assertIn('expected.bin',E.verify_index(self.root))
        (self.root/'receipt.json').write_bytes(b'{}\n')
        with self.assertRaises(ValueError):E.verify_index(self.root)

    def test_unsafe_paths(self):
        for path in ['/abs','../x','a/../x','a//b','a\\b','./x','']:
            with self.assertRaises(ValueError):E.safe_path(path)

    def test_manifest_cannot_claim_complete_without_receipts(self):
        obj=E.manifest_template('a'*64,'b'*64,'c'*40)
        E.validate_manifest(obj,self.root)
        obj.update(status_id=64,status='PASS_CORRECTNESS_CORE',complete=True,expectations_sealed=True,actual_expected_records=408034,actual_calls=[483553]*3)
        with self.assertRaises(ValueError):E.validate_manifest(obj,self.root)

    def test_guard_fixture_rejects_without_candidate(self):
        pair=hashlib.sha256(b'ARCH002-R1-PAIR\0'+struct.pack('<BQQ',1,1,1)+b'\0'+struct.pack('<BQQ',1,1,1)+b'\0').digest()
        row=(2**64-1,255,1,1,0,1,1,1,1,1,0,0,0,1,pair)
        self.expected.write_bytes(E.HEADER.pack(b'A2R1',0,255,255,0,1)+E.EXPECTED.pack(*row))
        raw=E.OBSERVED.pack(2**64-1,0,0,1,0,2,E.fold(E.D0,0,2),0)
        with self.assertRaises(V.Failure) as exc:V.verify(self.expected,io.BytesIO(raw),guard=True)
        self.assertEqual(exc.exception.status,65)


if __name__=='__main__':unittest.main()
