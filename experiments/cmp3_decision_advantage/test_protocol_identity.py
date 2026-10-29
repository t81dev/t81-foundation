"""Approved document identities and preserved historical byte contracts."""
import hashlib
import json
from pathlib import Path
import unittest
from runner_r1 import DOCUMENTS, ORIGINAL_BLOB

ROOT = Path(__file__).resolve().parent


class ProtocolIdentity(unittest.TestCase):
    def test_five_approved_document_sha256(self):
        self.assertEqual(len(DOCUMENTS), 5)
        for name, expected in DOCUMENTS.items():
            with self.subTest(name=name):
                self.assertEqual(hashlib.sha256((ROOT/'protocol/r1'/name).read_bytes()).hexdigest(), expected)

    def test_original_pr635_blob_bytes(self):
        raw = (ROOT/'protocol/ORIGINAL_PR635_PROTOCOL.md').read_bytes()
        blob = b'blob '+str(len(raw)).encode('ascii')+b'\0'+raw
        self.assertEqual(hashlib.sha1(blob).hexdigest(), ORIGINAL_BLOB)
        self.assertEqual(hashlib.sha256(raw).hexdigest(), '990f54a57d7ad34cfcd93c45147e81440ac8f2430eae3271d5f676d6947081f1')

    def test_existing_research_protocol_snapshot_unchanged(self):
        raw = (ROOT.parents[1]/'docs/research/TERNARY_ADVANTAGE_CANDIDATES.md').read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(), '2066d1c9f3f3f1831b7317296e551895881f30016b0a9c85547f8c6d537e4c81')

    def test_prospective_approval_is_not_measurement_authorization(self):
        activation = json.loads((ROOT/'protocol/ACTIVATION.json').read_text())
        self.assertEqual(activation['document_sha256'], DOCUMENTS)
        self.assertEqual(activation['original_blob'], ORIGINAL_BLOB)
        self.assertEqual(activation['status'], 'APPROVED_PROSPECTIVE_AMENDMENT')
        self.assertTrue(all(activation[m] is True for m in ('M1','M2','M3','M4')))
        self.assertIs(activation['measurement_authorized'], False)
        self.assertIs(activation['production_signing_configured'], False)
        self.assertIs(activation['signed_production_approval_provisioned'], False)

    def test_provisional_patch_and_report_preserved(self):
        expected = {'PROVISIONAL.patch':'1123bd8f4d52ce212b69ce796cc6c806d87cfbf11b3bfd4342c8323a2d2889f3',
                    'IMPLEMENTATION_REPORT.md':'35f11074821b80ca3b595365dce92944cbb998d29ee11964a147d44ef57929f1'}
        for name, sha in expected.items():
            with self.subTest(name=name):
                self.assertEqual(hashlib.sha256((ROOT/'evidence-r1-publication/provisional'/name).read_bytes()).hexdigest(), sha)


if __name__ == '__main__':
    unittest.main(verbosity=2)
