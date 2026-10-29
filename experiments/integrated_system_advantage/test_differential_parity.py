#!/usr/bin/env python3
"""Fail-closed, corpus-bounded parity gate. Requires explicit build artifacts.

The probe compiles the same production tensor operations and captures each stage
at its producing instruction, avoiding inference from final labels or pool ordering.
Backend-specific provenance/bundle bytes intentionally differ; result bytes do not.
"""
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
import unittest

from baseline_stack import ConventionalCAS, ConventionalTaskRunner, T81W2Model, MANDATORY_TENSORS, canon_sha3_256_ref

ROOT = Path(__file__).resolve().parents[2]
BUILD = Path(os.environ.get("T81_BUILD_DIR", ROOT / "build"))
EXT = ".exe" if os.name == "nt" else ""


def policy_for(data):
    return ('(policy (tier 1) (allowed-ternary-model-hashes ["sha3-512:'
            + hashlib.sha3_512(data).hexdigest()
            + '"]) (require-axion-event (reason "task:assess_fixed.v1")))\n')


def encode_model(tensors):
    payload = struct.pack('<Q', len(tensors))
    for name, tensor in tensors.items():
        name = name.encode()
        payload += struct.pack('<Q', len(name)) + name + struct.pack('<Q', len(tensor['shape']))
        payload += b''.join(struct.pack('<Q', d) for d in tensor['shape'])
        payload += bytes([tensor['format']]) + struct.pack('<QQ', tensor['trits'], len(tensor['data_bytes']))
        payload += tensor['data_bytes']
    return b'T81W2\n' + hashlib.sha3_512(payload).hexdigest().encode() + b'\n' + payload


class DifferentialParityTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ['T81_CANONFS_READ_VERIFY'] = '1'
        cls.cli = BUILD / ('t81' + EXT)
        cls.probe = BUILD / ('t81_investigation08_probe' + EXT)
        builder = BUILD / ('t81_make_assess_fixed_demo' + EXT)
        for binary in (cls.cli, cls.probe, builder):
            if not binary.is_file():
                raise RuntimeError(f'missing required binary {binary}')
        cls.scratch = tempfile.TemporaryDirectory(prefix='t81-investigation08-')
        cls.root = Path(cls.scratch.name)
        subprocess.run([str(builder), str(cls.root / 'demo')], check=True, capture_output=True)
        cls.model_bytes = (cls.root / 'demo/assess-fixed-demo.t81w').read_bytes()
        cls.tokenizer = json.loads((cls.root / 'demo/tokenizer.json').read_text())
        cls.valid_observations = []

    @classmethod
    def tearDownClass(cls):
        report = os.environ.get('T81_PARITY_REPORT')
        if report:
            Path(report).write_text(json.dumps(dict(valid_scenarios=cls.valid_observations), indent=2))
        cls.scratch.cleanup()

    def run_pair(self, prompt, data=None, tokenizer=None, policy=None, stages=True, expect_valid=True):
        data = self.model_bytes if data is None else data
        tokenizer = self.tokenizer if tokenizer is None else tokenizer
        policy = policy_for(data) if policy is None else policy
        with tempfile.TemporaryDirectory(dir=self.root) as tmp:
            path = Path(tmp)
            model = path / 'model.t81w'; model.write_bytes(data)
            tok = path / 'tokenizer.json'; tok.write_bytes(json.dumps(tokenizer).encode('utf-8'))
            apl = path / 'policy.apl'; apl.write_bytes(policy.encode('utf-8'))
            store = path / 't81-store'
            command = [str(self.cli), 'ai', 'task', 'assess-fixed', '--model-file', str(model),
                       '--model', 'parity', '--policy', str(apl), '--canonfs-root', str(store),
                       '--mode', 'strict_deterministic', '--input', prompt]
            proc = subprocess.run(command, capture_output=True, text=True, timeout=180)
            conventional = ConventionalTaskRunner(str(path / 'conventional'))
            try:
                result = conventional.run_assess_fixed(data, prompt, policy, tok.read_bytes().decode("utf-8"))
                code = 0
            except PermissionError:
                result = {}; code = 13
            except (ValueError, IndexError, KeyError, struct.error):
                result = {}; code = 1
            self.assertEqual(proc.returncode, code, proc.stderr)
            if not expect_valid:
                self.assertNotEqual(code, 0, 'negative scenario unexpectedly accepted')
                # Verify specific expected model rejection error output rather than accepting arbitrary failure/crashes
                self.assertTrue(
                    any(phrase in proc.stderr for phrase in ("error:", "Model architecture admission rejected", "supports only llama-dense-v1")),
                    f"Unexpected CLI stderr for model rejection: {proc.stderr}"
                )
                self.assertFalse(list((path/'conventional/objects').iterdir()), 'denial persisted objects')
                self.assertFalse(list(store.glob('objects/*')), 'reference denial persisted objects')
                return
            self.assertEqual(code, 0, proc.stderr)
            reference = json.loads(proc.stdout)
            for key in ('decision', 'reason_code', 'decision_token_ids', 'model_hash',
                        'input_hash', 'policy_hash', 'policy_result', 'result_ref'):
                self.assertEqual(reference[key], result[key], key)
            # A fresh CLI recovers each referenced artifact; independently hash its raw bytes.
            artifacts = {}
            for key in ('result_ref', 'provenance_ref'):
                target = path / (key + '.json')
                subprocess.run([str(self.cli), 'canonfs', 'get', reference[key], '--out', str(target),
                                '--canonfs-root', str(store)], check=True, capture_output=True, timeout=30)
                raw = target.read_bytes()
                self.assertEqual(canon_sha3_256_ref(raw), reference[key])
                artifacts[key] = json.loads(raw)
            self.assertEqual(conventional.cas.get(result['result_ref']), (path/'result_ref.json').read_bytes())
            evidence = artifacts['provenance_ref']['execution_evidence']
            self.assertEqual(evidence['sampled_token_ids'], [token for _, token in result['candidate_scores']])
            self.assertEqual(evidence['sampled_token_scores'], [score for score, _ in result['candidate_scores']])
            self.assertEqual(evidence['selected_token_id'], result['decision_token_ids'][0])
            self.assertEqual(evidence['input_preview'], result['stages'] and prompt.replace('\r\n','\n').replace('\r','\n').rstrip('\r\n'))
            self.assertEqual(artifacts['provenance_ref']['result_ref'], result['result_ref'])
            self.assertEqual(artifacts['provenance_ref']['model_hash'], result['model_hash'])
            if stages:
                probe = subprocess.run([str(self.probe), str(model), str(tok),
                                        evidence['input_preview']], capture_output=True, text=True, timeout=180)
                self.assertEqual(probe.returncode, 0, probe.stderr)
                trace = json.loads(probe.stdout)
                self.assertEqual(trace['stages'], result['stages'])
                self.assertEqual(trace['sampled_token_scores'], evidence['sampled_token_scores'])
                self.assertEqual(trace['sampled_token_ids'], evidence['sampled_token_ids'])
                self.assertEqual(trace['prompt_token_history'], evidence['prompt_token_history'])
            # New runner; all recovery inputs must come from CAS, no overrides permitted.
            recovered = ConventionalTaskRunner(str(path/'conventional'))
            self.assertTrue(recovered.verify_and_replay(result['bundle_ref'])['valid'])
            self.valid_observations.append(dict(prompt=prompt, model_hash=result['model_hash'],
                                               result_ref=result['result_ref'], stages_compared=stages))

    def test_original_and_normalized_inputs(self):
        for prompt in ('greet hello', 'hello world', 'greet', 'foo bar baz', '123456',
                       'greet hello\r\n', '  greet hello  ', 'greet\nhello', '\tgreet hello\t\r\n\n',
                       'greet hello world greet hello world'):
            with self.subTest(prompt=prompt):
                self.run_pair(prompt)

    def test_valid_nonsymmetric_mutations(self):
        for seed in (7, 19, 41):
            rng = random.Random(seed)
            tensors = copy.deepcopy(T81W2Model(self.model_bytes).tensors)
            # Encode valid u64 limbs (48 base-3 digits, leading digits zero).
            # Nonsymmetric row-major projections distinguish W from W^T.
            for name, tensor in tensors.items():
                count = len(tensor['data_bytes']) // 8
                tensor['data_bytes'] = b''.join(struct.pack('<Q', rng.randrange(3**35)) for _ in range(count))
            q = T81W2Model.unpack_trits(tensors['model.layers.0.self_attn.q_proj.weight'])
            self.assertTrue(any(q[i*16+j] != q[j*16+i] for i in range(16) for j in range(16)))
            self.run_pair('greet hello', encode_model(tensors))

    def test_tokenizer_changes_and_wrapped_ties(self):
        tok = copy.deepcopy(self.tokenizer)
        tok['model']['vocab'].update(greet=63, hello=0, world=12)
        self.run_pair('greet', tokenizer=tok)  # window wraps: first sampled tie != lowest ID
        tok['model']['vocab'] = {'▁': 7, 'g': 11, 'r': 12, 'e': 13, 't': 14, 'h': 15,
                                  'l': 16, 'o': 17, '▁g': 18}
        tok['model']['merges'] = ['▁ g']
        self.run_pair('greet hello', tokenizer=tok)

    def test_policy_denials_and_syntax(self):
        allow = policy_for(self.model_bytes)
        policies = [allow.replace(hashlib.sha3_512(self.model_bytes).hexdigest(), 'deadbeef'),
                    allow.replace('(require-axion-event (reason "task:assess_fixed.v1"))',''),
                    allow.replace('])', ' "sha3-512:other"])'),
                    allow.replace('])', ' "sha3-512:'+hashlib.sha3_512(self.model_bytes).hexdigest()+'"])'),
                    allow[:-2], '(policy (tier nope))', '(policy (unknown true))',
                    '(policy (allowed-ternary-model-hashes [1]))']
        for policy in policies:
            with self.subTest(policy=policy):
                self.run_pair('greet hello', policy=policy, expect_valid=False)

    def test_malformed_models(self):
        good_payload = self.model_bytes.split(b'\n',2)[2]
        def checked(payload):
            return b'T81W2\n'+hashlib.sha3_512(payload).hexdigest().encode()+b'\n'+payload
        for data in (b'', b'\xff\xfe', b'T81W2\n', self.model_bytes[:-8],
                     b'T81W2\n'+b'0'*128+b'\npayload', checked(b'\x01'),
                     checked(struct.pack('<QQ',1,100)+b'name'), checked(good_payload[:100])):
            with self.subTest(size=len(data)):
                self.run_pair('greet hello', data=data, expect_valid=False)
        # Differential negative tests for all 13 mandatory architecture tensors
        base_tensors = T81W2Model(self.model_bytes).tensors
        for tensor_to_remove in sorted(MANDATORY_TENSORS):
            with self.subTest(omitted_tensor=tensor_to_remove):
                tensors = copy.deepcopy(base_tensors)
                if tensor_to_remove in tensors:
                    del tensors[tensor_to_remove]
                self.run_pair('greet hello', data=encode_model(tensors), expect_valid=False)

        tensors = copy.deepcopy(base_tensors)
        tensors['model.embed_tokens.weight']['shape'] = [64, 15]
        self.run_pair('greet hello', data=encode_model(tensors), expect_valid=False)
        self.run_pair('', expect_valid=False)

    def test_trial_isolation_across_invocations(self):
        from ablation_runner import AblationHarness
        with tempfile.TemporaryDirectory() as tmp:
            harness = AblationHarness(ROOT, tmp, BUILD)
            def standin(fixtures, prompt, root):
                Path(root, 'object').write_text('persisted')
                return dict(end_to_end_process_ns=1, result_ref='same', model_hash='same',
                            decision='same', policy_result='allowed')
            harness.run_config_a = standin
            harness.run_config_b = standin
            first = harness.run_paired_benchmarks({}, 'input', iterations=2)
            second = harness.run_paired_benchmarks({}, 'input', iterations=2)
            roots = [root for run in (first, second) for row in run['observations'] for root in row['roots'].values()]
            self.assertEqual(len(roots), len(set(roots)))
            for run in (first,second):
                for row in run['observations']:
                    self.assertEqual(row['initial_entries'], dict(A=0,B=0))
                    self.assertEqual(row['entries_before'], dict(A=0,B=0) if row['condition']=='empty_store' else dict(A=1,B=1))

    def test_binary_identity_tamper_and_independent_recovery(self):
        with tempfile.TemporaryDirectory() as tmp:
            cas = ConventionalCAS(tmp)
            self.assertNotEqual(cas.put(b'\xff'), cas.put(b'\xfe'))
            ref = cas.put(b'\xff')
            self.assertEqual(cas.get(ref), b'\xff')
            # Production CanonFS and the conventional CAS must identify the same bytes.
            source = Path(tmp)/'binary'; source.write_bytes(b'\xff')
            stored = subprocess.run([str(self.cli), 'canonfs', 'put-file', str(source),
                                     '--canonfs-root', str(Path(tmp)/'reference')],
                                    capture_output=True,text=True,check=True,timeout=30)
            self.assertEqual(stored.stdout.strip(), ref)
            target = Path(tmp)/'reference/objects'/(ref.split(':',1)[1]+'.blk')
            target.write_bytes(b'\xfe')
            rejected = subprocess.run([str(self.cli), 'canonfs', 'get', ref,
                                       '--canonfs-root', str(Path(tmp)/'reference')],
                                      capture_output=True,timeout=30)
            self.assertNotEqual(rejected.returncode, 0, 'reference accepted tampered binary object')
            (Path(cas.objects_dir)/ref.split(':',1)[1]).write_bytes(b'\xfe')
            with self.assertRaises(ValueError): cas.get(ref)
            with self.assertRaises(ValueError): cas.put(b'\xff')
        with tempfile.TemporaryDirectory() as tmp:
            runner = ConventionalTaskRunner(tmp)
            result = runner.run_assess_fixed(self.model_bytes, 'greet hello', policy_for(self.model_bytes), self.tokenizer)
            bundle = json.loads(runner.cas.get(result['bundle_ref']))
            self.assertTrue(ConventionalTaskRunner(tmp).verify_and_replay(result['bundle_ref'])['valid'])
            replay = subprocess.run([__import__('sys').executable, str(Path(__file__).with_name('baseline_worker.py')),
                                     'replay','--store',tmp,'--bundle',result['bundle_ref']],
                                    capture_output=True,text=True,check=True,timeout=30)
            self.assertTrue(json.loads(replay.stdout)['valid'])
            for key, value in [('model_hash', 'sha3-512:wrong'), ('source_result_ref', bundle['model_ref']),
                               ('decision', 'DENY'), ('model_ref', 'sha3-256:missing')]:
                altered = dict(bundle, **{key:value})
                ref = runner.cas.put(json.dumps(altered).encode())
                self.assertFalse(ConventionalTaskRunner(tmp).verify_and_replay(ref)['valid'])
            model_path = Path(runner.cas.objects_dir)/bundle['model_ref'].split(':',1)[1]
            model_path.write_bytes(b'invalid binary\xff')
            self.assertFalse(ConventionalTaskRunner(tmp).verify_and_replay(result['bundle_ref'])['valid'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
