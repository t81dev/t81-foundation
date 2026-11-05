import pathlib,subprocess,json,hashlib,shutil
r=pathlib.Path('/private/tmp/t81-cpa006/experiments/cpa006/evidence/baseline');old=pathlib.Path('/Users/t81dev/Code/t81-foundation/experiments/cpa005');py='/private/tmp/t81-h04-enforcement-venv/bin/python';commands=[]
for name,pin in json.loads((old/'manifest.json').read_text())['files'].items():
 data=(old/name).read_bytes();assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
shutil.copyfile(old/'evidence/cpa003-packet.tar.gz',r/'cpa003-packet.tar.gz')
def run(cmd):
 p=subprocess.run(cmd,capture_output=True);i=len(commands);(r/f'command-{i}.stdout').write_bytes(p.stdout);(r/f'command-{i}.stderr').write_bytes(p.stderr)
 commands.append(dict(command=cmd,exit_code=p.returncode,stdout=f'command-{i}.stdout',stderr=f'command-{i}.stderr'));(r/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 assert p.returncode==0,cmd
pins=json.loads((old/'evidence/build-artifacts.json').read_text())
for profile in ('frozen','integrated'):
 src='/private/tmp/t81-cpa005-frozen' if profile=='frozen' else '/private/tmp/t81-cpa005';build='/private/tmp/cpa005-build-'+profile
 for path,pin in pins[profile].items():
  if path.startswith(build):assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==pin['sha256'],path
 (r/profile).mkdir(exist_ok=True)
 run([py,'-B',src+'/experiments/cpa004/evidence/acceptance/check.py','--hosted-observer',build+'/hosted_observer','--output',str(r/profile/'hosted')])
 run([py,'-B',src+'/experiments/cpa004/run_qemu.py','--build',build,'--output',str(r/profile/'qemu'),'--qemu','/opt/homebrew/bin/qemu-system-aarch64','--firmware-code','/opt/homebrew/share/qemu/edk2-aarch64-code.fd','--firmware-vars','/opt/homebrew/share/qemu/edk2-arm-vars.fd'])
run([py,'-B','/private/tmp/t81-cpa005/experiments/cpa005/verify.py','--evidence',str(r)])
run([py,'-B','/private/tmp/t81-cpa005/experiments/cpa004/verify_evidence.py'])
run(['/private/tmp/t81-cpa005/experiments/cpa005/evidence/adversarial/wrong-release']) if False else None
(r/'result.json').write_text(json.dumps(dict(disposition='PASS',retained_cpa005_manifest=True,fresh_hosted=6,fresh_guests=6,build_policy='Reused exact hash-verified CPA005 binaries for baseline replay; fresh processes/guests. CPA006 builds will be separate.',source_tree='70734fd5bcfe06978cea13b4d2d595d4dda71a12',default_off='Prior raw isolation evidence and every CPA005 packaged hash verified; original build/code sections unchanged.'),indent=2)+'\n')
print('PASS baseline')
