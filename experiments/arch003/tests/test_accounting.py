"""Deterministic accountant boundary fixtures; no campaign or process tree."""
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from accounting import Accountant,AccountingUnavailable,ResourceExceeded,RSS_CAP


class AccountingChecks(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.now=0
        self.acc=object.__new__(Accountant)
        self.acc.clock=lambda:self.now
        self.acc._start=0;self.acc.last_sample=None;self.acc.peak_bound=0
        self.acc.complete=True;self.acc.directory=Path(self.tmp.name)
        self.acc.jobs=[];self.acc.samples=[]

    def tearDown(self):self.tmp.cleanup()

    def sample(self,rows):
        with patch('accounting.os.getpid',return_value=1),patch.object(self.acc,'process',side_effect=lambda p:rows[p]):
            return self.acc.sample(2)

    def test_aggregate_not_individual_rss(self):
        rows={1:dict(pid=1,rss_bytes=RSS_CAP//2,peak_bytes=RSS_CAP//2,children=[2],threads=1),2:dict(pid=2,rss_bytes=RSS_CAP//2+1,peak_bytes=RSS_CAP//2+1,children=[],threads=1)}
        with self.assertRaises(ResourceExceeded):self.sample(rows)

    def test_no_baseline_subtraction(self):
        row=dict(pid=1,rss_bytes=123,peak_bytes=123,children=[],threads=1)
        self.sample({1:row});self.assertEqual(self.acc.peak_bound,123)

    def test_sample_interval(self):
        self.acc.last_sample=0;self.now=100_000_001
        with self.assertRaises(AccountingUnavailable):self.acc.sample(2)
        self.assertFalse(self.acc.complete)

    def test_deadline(self):
        self.now=1800*10**9+1
        with self.assertRaises(ResourceExceeded):self.sample({1:dict(pid=1,rss_bytes=1,peak_bytes=1,children=[],threads=1)})

    def test_deadline_cannot_be_reset(self):
        with self.assertRaises(AttributeError):self.acc.start=99
        self.assertEqual(self.acc.start,0)

    def test_diagnostic_subbudget(self):
        path=self.acc.directory/'probe.stderr'
        with open(path,'wb') as stream:stream.truncate(8*2**20+1)
        with self.assertRaises(ResourceExceeded):self.acc.check_output()

    def test_unsupported_host_fails_closed(self):
        with patch('accounting.sys.platform','win32'):
            with self.assertRaises(AccountingUnavailable):Accountant(self.tmp.name)


if __name__=='__main__':unittest.main()
