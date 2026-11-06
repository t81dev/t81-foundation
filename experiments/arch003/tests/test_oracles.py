"""Two fixed five-record oracle fixtures, checked against Python byte equations."""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import evidence as E


@unittest.skipUnless(os.environ.get('ARCH003_COMPONENT_BIN_DIR'),'set explicit component binary directory')
class OracleWireChecks(unittest.TestCase):
    def test_separate_oracles_match_fixed_reference(self):
        directory=Path(os.environ['ARCH003_COMPONENT_BIN_DIR'])
        streams=[]
        for kind in ('scalar','histogram'):
            run=subprocess.run([directory/f'arch003_{kind}_oracle','--fixed-wire-fixture'],check=True,capture_output=True)
            self.assertEqual(run.stderr,b'')
            self.assertEqual(len(run.stdout),5*88)
            streams.append(run.stdout)
        self.assertEqual(streams[0],streams[1])
        aa=[bytes([255,0,1]),bytes([19]),bytes([129]),bytes([100])+bytes([85])*31,bytes([4,1])]
        bb=[bytes([255,1,1]),bytes([23]),bytes([132]),bytes([104])+bytes([85])*31,bytes([6,1])]
        for f in range(5):
            pair=hashlib.sha256(b'ARCH002-R1-PAIR\0'+struct.pack('<BQQ',f,3,len(aa[f]))+aa[f]+struct.pack('<BQQ',f,3,len(bb[f]))+bb[f]).digest()
            expected=E.EXPECTED.pack(f,0,f,f,0,3,3,len(aa[f]),len(bb[f]),2,0,0,0,sum(1<<i for i,t in enumerate(E.TAGS) if t==f),pair)
            self.assertEqual(streams[0][f*88:(f+1)*88],expected)


if __name__=='__main__':unittest.main()
