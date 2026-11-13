"""Correctness validation only. Synthetic durations are never experiment evidence."""
import ast
import base64
from collections import Counter
import copy
from fractions import Fraction
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import analysis
import apparatus
import schedule

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def records(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()]


def synthetic_dataset():
    rows = []
    for item in schedule.generate()["pairs"]:
        rows.append(dict(item, purpose="synthetic_unit_test", status="complete", equivalent=True,
                         arms={a: dict(elapsed_ns=100, verified=True) for a in ("A", "B")}))
    return dict(purpose="synthetic_unit_test", schedule_sha256=analysis.SCHEDULE_SHA256, pairs=rows)


class ScheduleTests(unittest.TestCase):
    def test_exact_sample_and_balance(self):
        planned = schedule.generate()
        self.assertEqual(planned, schedule.generate())
        self.assertEqual(planned["seed"], 62708)
        self.assertFalse(planned["measurement_authorized"])
        pairs = planned["pairs"]
        self.assertEqual(len(pairs), 360)
        self.assertEqual(len({(r["cell"], r["block"]) for r in pairs}), 360)
        for cell in schedule.CELLS:
            sample = [r for r in pairs if r["cell"] == cell]
            self.assertEqual(len(sample), 60)
            self.assertEqual(Counter(tuple(r["order"]) for r in sample), {("A", "B"):30, ("B", "A"):30})
        for block in range(60):
            sample = [r for r in pairs if r["block"] == block]
            self.assertEqual({r["cell"] for r in sample}, set(schedule.CELLS))
            self.assertEqual([r["position"] for r in sample], list(range(6)))
            for row in sample:
                self.assertEqual(row["prompt"], dict(schedule.SCENARIOS)[row["cell"].split("/")[0]])

    def test_frozen_bytes_and_no_overwrite(self):
        expected = HERE/"schedule.json"
        self.assertEqual(hashlib.sha256(expected.read_bytes()).hexdigest(), analysis.SCHEDULE_SHA256)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"schedule.json"
            self.assertEqual(schedule.write(path), analysis.SCHEDULE_SHA256)
            self.assertEqual(path.read_bytes(), expected.read_bytes())
            with self.assertRaises(FileExistsError):
                schedule.write(path)


class StatisticalTests(unittest.TestCase):
    def test_indices_with_independent_pascal_oracle(self):
        # Independent integer Pascal construction; does not call math.comb/CDF code.
        row = [1]
        for _ in range(60):
            row = [1] + [a+b for a,b in zip(row, row[1:])] + [1]
        self.assertEqual(analysis.interval_indices(), (20, 41))
        self.assertLessEqual(Fraction(sum(row[:20]), 2**60), Fraction(1, 240))
        self.assertGreater(Fraction(sum(row[:21]), 2**60), Fraction(1, 240))
        self.assertLessEqual(Fraction(12*sum(row[:20]), 2**60), Fraction(1,20))

    def test_exact_boundaries_and_neighbors(self):
        low, high = Fraction(10,11), Fraction(11,10)
        eps = Fraction(1,10**30)
        self.assertEqual(analysis.decision(low-eps, low-eps), analysis.ADVANTAGE)
        self.assertEqual(analysis.decision(low, low), analysis.EQUIVALENT)
        self.assertEqual(analysis.decision(high, high), analysis.EQUIVALENT)
        self.assertEqual(analysis.decision(high+eps, high+eps), analysis.DISADVANTAGE)
        self.assertEqual(analysis.decision(low-eps, low), analysis.INCONCLUSIVE)
        self.assertEqual(analysis.decision(high, high+eps), analysis.INCONCLUSIVE)
        self.assertEqual(analysis.decision(low, high), analysis.EQUIVALENT)
        with self.assertRaises(ValueError):
            analysis.decision(2,1)

    def test_overall_exhaustive_mixed_and_unresolved(self):
        states = (analysis.ADVANTAGE, analysis.DISADVANTAGE, analysis.EQUIVALENT, analysis.INCONCLUSIVE)
        for values in itertools.product(states, repeat=6):
            expected = values[0] if all(v == values[0] for v in values) else analysis.INCONCLUSIVE
            self.assertEqual(analysis.overall(dict(zip(analysis.CELLS, values))), expected)

    def test_order_statistics_not_zero_based_or_unpaired(self):
        data = synthetic_dataset()
        for row in data["pairs"]:
            # CI20 and CI41 both equal1; CI19=.5 and CI42=2 expose off-by-one errors.
            block = row["block"]
            a = 50 if block < 19 else 100 if block < 41 else 200
            row["arms"]["A"]["elapsed_ns"] = a
        result = analysis.analyze(data, synthetic=True)
        for cell in result["cells"].values():
            self.assertEqual(cell["simultaneous_interval"]["lower"]["value"], 1)
            self.assertEqual(cell["simultaneous_interval"]["upper"]["value"], 1)
            self.assertIn("serial_dependence_warning",cell["warnings"])
            self.assertEqual(cell["disposition"],analysis.INCONCLUSIVE)
        self.assertFalse(result["eligible_for_scientific_claim"])

    def test_synthetic_inference_and_environment_guard(self):
        data = synthetic_dataset()
        self.assertEqual(analysis.analyze(data, synthetic=True)["overall_disposition"], analysis.EQUIVALENT)
        data["pairs"][0]["environment_regime_changed"] = True
        self.assertEqual(analysis.analyze(data, synthetic=True)["overall_disposition"], analysis.INCONCLUSIVE)

    def test_refuse_missing_duplicate_failed_validation_and_bad_durations(self):
        mutations = [lambda d:d["pairs"].pop(),
                     lambda d:d["pairs"].reverse(),
                     lambda d:d["pairs"].__setitem__(0,copy.deepcopy(d["pairs"][1])),
                     lambda d:d["pairs"][0].update(status="incomplete"),
                     lambda d:d["pairs"][0].update(equivalent=False),
                     lambda d:d["pairs"][0]["arms"]["A"].update(elapsed_ns=0),
                     lambda d:d["pairs"][0]["arms"]["A"].update(elapsed_ns=float('nan')),
                     lambda d:d["pairs"][0]["arms"]["A"].update(elapsed_ns=True),
                     lambda d:d["pairs"][0]["arms"]["A"].update(verified=False),
                     lambda d:d.update(schedule_sha256="wrong"),
                     lambda d:d["pairs"][0].update(order=list(reversed(d["pairs"][0]["order"])))]
        for mutate in mutations:
            data = synthetic_dataset(); mutate(data)
            with self.assertRaises(ValueError):
                analysis.analyze(data, synthetic=True)
        with self.assertRaises(ValueError):
            analysis.analyze(dict(purpose=apparatus.PURPOSE, pairs=[]))
        data = synthetic_dataset(); data["purpose"] = "confirmatory"
        with self.assertRaises(ValueError):
            analysis.analyze(data)

    def test_analysis_does_not_import_execution_or_comparator(self):
        tree = ast.parse((HERE/"analysis.py").read_text())
        imports = [n.module for n in ast.walk(tree) if isinstance(n,ast.ImportFrom)]
        imports += [a.name for n in ast.walk(tree) if isinstance(n,ast.Import) for a in n.names]
        self.assertFalse(set(imports) & {"apparatus", "baseline_stack", "ablation_runner", "subprocess"})


class RetentionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name)/"attempts.jsonl"
        self.journal = apparatus.Journal(self.path,create=True)

    def test_missing_executable_nonzero_and_raw_output(self):
        with self.assertRaises(FileNotFoundError):
            apparatus.launch([str(Path(self.tmp.name)/"missing")],self.journal)
        with self.assertRaises(apparatus.LaunchFailure) as error:
            apparatus.launch([sys.executable,"-c","import os; os.write(1,b'\\xff'); raise SystemExit(7)"],self.journal)
        self.assertEqual(error.exception.returncode,7)
        events=records(self.path)
        self.assertEqual([e["kind"] for e in events], ["launch_start","launch_failure","launch_start","launch_finish"])
        self.assertEqual(base64.b64decode(events[-1]["stdout_base64"]),b'\xff')
        self.assertTrue(all(e["purpose"]==apparatus.PURPOSE and e["confirmatory"] is False for e in events))

    def test_timeout_is_retained(self):
        with self.assertRaises(subprocess.TimeoutExpired):
            apparatus.launch([sys.executable,"-c","import time; print('started',flush=True); time.sleep(10)"],self.journal,timeout=.2)
        last=records(self.path)[-1]
        self.assertEqual(last["kind"],"launch_failure")
        self.assertEqual(last["error_type"],"TimeoutExpired")
        self.assertIn(b'started',base64.b64decode(last["stdout_base64"]))

    def test_disk_failure_prevents_spawn(self):
        with patch.object(apparatus.os,"fsync",side_effect=OSError("disk unavailable")), patch.object(apparatus.subprocess,"Popen") as spawn:
            with self.assertRaises(OSError):
                apparatus.launch([sys.executable,"-c","pass"],self.journal)
            spawn.assert_not_called()

    def test_partial_record_preserved_and_stops(self):
        self.path.write_bytes(b'{"incomplete":')
        with self.assertRaises(ValueError):
            self.journal.append("never")
        self.assertEqual(self.path.read_bytes(),b'{"incomplete":')

    def test_incomplete_pair_keeps_successful_first_arm(self):
        harness=apparatus.ControlledHarness(ROOT,self.tmp.name,"unused-build")
        roles={r:"ref-"+r for r in apparatus.ROLES}
        def launch(command,journal,**kwargs):
            context=kwargs["context"]
            journal.append("launch_start",context=context)
            if context["arm"]=="B":
                journal.append("launch_failure",error="injected missing worker")
                raise FileNotFoundError("injected missing worker")
            result=dict(purpose=apparatus.PURPOSE,measurement_eligible=False,verified=True,artifact_roles=roles)
            journal.append("launch_finish",returncode=0,stdout_base64=base64.b64encode(json.dumps(result).encode()).decode())
            return json.dumps(result).encode()
        fixtures=dict(model_path="unused-model",apl_policy_path="unused-policy",tokenizer_path="unused-tokenizer")
        with patch.object(apparatus,"launch",side_effect=launch):
            with self.assertRaises(FileNotFoundError):
                harness.validate_pair(fixtures)
        path=next(Path(self.tmp.name).glob('validation-*/attempts.jsonl'))
        events=records(path)
        self.assertEqual(events[-1]["kind"],"pair_incomplete")
        self.assertEqual(events[-1]["completed_arms"],["A"])
        self.assertTrue(any(e["kind"]=="launch_finish" for e in events))


@unittest.skipUnless(os.environ.get("T81_BUILD_DIR"),"set T81_BUILD_DIR for real correctness/instrumentation checks")
class RealWorkflowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix='t81-i08-apparatus-validation-')
        cls.path=Path(cls.tmp.name)
        cls.harness=apparatus.ControlledHarness(ROOT,cls.path,os.environ["T81_BUILD_DIR"])
        cls.fixtures=cls.harness.prepare_fixtures()
        cls.model=Path(cls.fixtures['model_path']).read_bytes()
        cls.policy=Path(cls.fixtures['apl_policy_path']).read_text()
        cls.tokenizer=Path(cls.fixtures['tokenizer_path']).read_text()

    @classmethod
    def tearDownClass(cls):
        if os.environ.get("T81_VALIDATION_EVIDENCE_DIR"):
            out=Path(os.environ["T81_VALIDATION_EVIDENCE_DIR"])
            out.mkdir(parents=True,exist_ok=False)
            for i,path in enumerate(sorted(cls.path.rglob('attempts.jsonl'))):
                shutil.copyfile(path,out/f'{i:02d}-{path.parent.name}.jsonl')
        cls.tmp.cleanup()

    def test_real_seven_obligations_and_populated_condition(self):
        for _,prompt in schedule.SCENARIOS:
            for condition in schedule.CONDITIONS:
                with self.subTest(prompt=prompt,condition=condition):
                    result=self.harness.validate_pair(self.fixtures,prompt,condition,order=('B','A'))
                    row=result['record']
                    self.assertTrue(row['equivalent'])
                    if condition=='populated_store':
                        self.assertTrue(all(n>0 for n in row['entries_before'].values()))
                    else:
                        self.assertEqual(row['entries_before'],dict(A=0,B=0))
                    for arm in 'AB':
                        value=row['arms'][arm]
                        self.assertEqual(set(value['artifact_roles']),set(apparatus.ROLES))
                        self.assertTrue(value['verified'])
                        self.assertFalse(value['measurement_eligible'])
                        self.assertNotIn('elapsed_ns',value)
                        self.assertEqual(set(value['storage']['logical_role_bytes']),set(apparatus.ROLES))
                    self.assertTrue(all(not e['confirmatory'] for e in records(result['journal'])))

    def test_files_removed_before_new_process_recovery_and_tamper(self):
        for arm in 'AB':
            with self.subTest(arm=arm):
                path=self.path/('recovery-'+arm); path.mkdir()
                fixture=path/'original'; fixture.mkdir()
                (fixture/'model').write_bytes(self.model)
                (fixture/'policy').write_text(self.policy)
                (fixture/'tokenizer').write_text(self.tokenizer)
                journal=apparatus.Journal(path/'attempts.jsonl',create=True)
                base=[sys.executable,str(HERE/'apparatus.py'),'worker','--arm',arm,
                      '--store',str(path/'cas'),'--cli',self.harness.t81_bin,
                      '--journal',str(journal.path),'--scratch',str(path)]
                result=json.loads(apparatus.launch(base+['--operation','run','--model',str(fixture/'model'),
                    '--policy',str(fixture/'policy'),'--tokenizer',str(fixture/'tokenizer'),'--input','greet hello'],journal))
                for file in fixture.iterdir(): file.unlink()
                fixture.rmdir()
                ref=result['artifact_roles']['bundle']
                replay=json.loads(apparatus.launch(base+['--operation','replay','--bundle',ref],journal))
                self.assertEqual(result['artifact_roles'],replay['artifact_roles'])
                # Hash-valid false provenance must fail the full recovery/re-execution check.
                store=apparatus.Store(arm,path/'cas',self.harness.t81_bin,journal,path)
                bundle=json.loads(store.get(ref)); prov=json.loads(store.get(bundle['source_provenance_ref']))
                prov['execution_evidence']['sampled_token_scores'][0]+=1
                bundle['source_provenance_ref']=store.put(apparatus.json_bytes(prov))
                altered=store.put(apparatus.json_bytes(bundle))
                with self.assertRaises(apparatus.LaunchFailure):
                    apparatus.launch(base+['--operation','replay','--bundle',altered],journal)
                # Byte tamper must fail hash-verified readback too.
                digest=ref.split(':',1)[1]
                object_path=path/'cas/objects'/(digest+('.blk' if arm=='A' else ''))
                object_path.write_bytes(b'corrupted')
                with self.assertRaises(apparatus.LaunchFailure):
                    apparatus.launch(base+['--operation','lookup','--bundle',ref],journal)

    def test_policy_denial_and_invalid_model_leave_no_objects(self):
        for arm in 'AB':
            for kind in ('denial','invalid'):
                with self.subTest(arm=arm,kind=kind):
                    path=self.path/(arm+'-'+kind); path.mkdir()
                    journal=apparatus.Journal(path/'attempts.jsonl',create=True)
                    store=apparatus.Store(arm,path/'cas',self.harness.t81_bin,journal,path)
                    model=self.model if kind=='denial' else b'invalid model'
                    policy=self.policy.replace('task:assess_fixed.v1','wrong') if kind=='denial' else self.policy.replace(hashlib.sha3_512(self.model).hexdigest(),hashlib.sha3_512(model).hexdigest())
                    with self.assertRaises((apparatus.LaunchFailure,PermissionError,ValueError)):
                        apparatus.execute(store,model,'greet hello',policy,self.tokenizer)
                    self.assertFalse(list((path/'cas').glob('objects/*')))


if __name__=='__main__':
    unittest.main(verbosity=2)
