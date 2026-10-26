"""Characterize documented non-interpreter limits; no production artifact edits."""
import importlib.util,json,pathlib,subprocess,tempfile,sys
repo=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[4]
s=importlib.util.spec_from_file_location('root_tests',repo/'tests/ci/check_root_structure_test.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
T=m.RootLayout;T.setUpClass();rows=[]
try:
 for name,fragment in [
 ('included-file','include(cmake/review-source.cmake)\n'),
 ('macro-invocation','macro(hidden_sources)\n target_sources(direct PRIVATE ${PROJECT_SOURCE_DIR}/market-demos/customer_demo_system.cpp)\nendmacro()\nhidden_sources()\n'),
 ('indirect-source-variable','set(HIDDEN_SOURCE ${PROJECT_SOURCE_DIR}/market-demos/customer_demo_system.cpp)\ntarget_sources(direct PRIVATE ${HIDDEN_SOURCE})\n')]:
  t=T('test_valid_approved_layout');t.setUp()
  try:
   if name=='included-file':(t.repo/'cmake/review-source.cmake').write_text('target_sources(direct PRIVATE ${PROJECT_SOURCE_DIR}/market-demos/customer_demo_system.cpp)\n')
   original=(t.repo/'CMakeLists.txt').read_text()
   (t.repo/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.16)\nproject(ReviewProbe LANGUAGES CXX)\n'+original+fragment+'\nget_target_property(observed_sources direct SOURCES)\nmessage(STATUS "REVIEW_SOURCES=${observed_sources}")\n')
   t.commit();r=t.run_check('--structural');auth=t.run_check('--authorization')
   with tempfile.TemporaryDirectory() as build:
    args=['cmake','-S',str(t.repo),'-B',build]
    cm=subprocess.run(args,capture_output=True,text=True,timeout=60)
   rows.append({'name':name,'fragment':fragment,'structural_exit':r.returncode,'structural_stdout':r.stdout,'authorization_without_trust_exit':auth.returncode,'cmake_exit':cm.returncode,'cmake_command':args,'cmake_stdout':cm.stdout,'cmake_stderr':cm.stderr,'classification':'explicitly outside literal add_subdirectory/source-path inventory; not complete CMake certification'})
  finally:t.doCleanups()
finally:T.tearDownClass()
print(json.dumps(rows,indent=2))
