"""Synthetic negative trust examples, never production approval evidence."""
import importlib.util,json,pathlib,subprocess,sys
repo=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[4]
s=importlib.util.spec_from_file_location('root_tests',repo/'tests/ci/check_root_structure_test.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
T=m.RootLayout;T.setUpClass();t=T('test_valid_approved_layout');t.setUp();rows=[]
def capture(label,r):
 rows.append({'case':label,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
try:
 t.approve_fixture()
 t.contract_edit(lambda x:x.update(entries=[e for e in x['entries'] if e['path']!='LICENSE']))
 (t.repo/'LICENSE').unlink();t.commit()
 capture('contract loosening alone is structurally consistent, not authorization',t.run_check('--structural'))
 capture('original external approval rejects altered candidate',t.run_check('--authorization','--trusted-approval',str(t.trust)))
 t.approve_fixture()  # FORGED test data, no human or signature authenticated.
 local=t.repo/'docs/forged-approval.json';local.write_bytes(t.trust.read_bytes())
 capture('candidate-local forged approval is rejected',t.run_check('--authorization','--trusted-approval',str(local)))
 capture('forged external approval accepted if operator trust assumption is violated',t.run_check('--authorization','--trusted-approval',str(t.trust)))
 checker=t.repo/m.module.CHECKER;checker.write_text('print("forged candidate evaluator success")\n')
 t.commit()
 r=subprocess.run([sys.executable,str(checker),'--authorization'],capture_output=True,text=True)
 capture('candidate-controlled evaluator can forge its own apparent success',r)
 capture('trusted evaluator with original external data rejects altered candidate evaluator',t.run_check('--authorization','--trusted-approval',str(t.trust)))
 assert [x['exit']==0 for x in rows]==[True,False,False,True,True,False]
finally:t.doCleanups();T.tearDownClass()
print(json.dumps({'synthetic_fixture_only':True,'results':rows},indent=2))
