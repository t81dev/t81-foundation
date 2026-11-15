import hashlib,json,os,shutil,subprocess
from pathlib import Path
base=Path('/private/tmp/t81-phase4');image=base/'guest-image/qemu_slice6_guest.img'
rows=[]
for name,firmware,matched,vars_template in [
 ('supplied-original','/opt/homebrew/share/qemu/edk2-aarch64-code.fd',False,None),
 ('supplied-matched','/opt/homebrew/share/qemu/edk2-aarch64-code.fd',True,'/opt/homebrew/share/qemu/edk2-arm-vars.fd'),
 ('diagnostic-original',str(base/'firmware/diagnostic-code.fd'),False,None),
 ('diagnostic-matched',str(base/'firmware/diagnostic-code.fd'),True,str(base/'firmware/diagnostic-vars.fd'))]:
 out=base/name;out.mkdir(exist_ok=False)
 env=dict(os.environ,QEMU_IMG=str(image),BIOS=firmware,OUTPUT_DIR=str(out),VERIFICATION_TIMEOUT='40')
 if matched:
  variables=out/'vars.fd';shutil.copyfile(vars_template,variables)
  env.update(QEMU_BIN=str(base/'matched_qemu.py'),PHASE4_VARS=str(variables),PHASE4_ARGV_RECORD=str(out/'qemu-argv.json'))
 cmd=['bash','drivers/qemu/scripts/automated_verification.sh']
 p=subprocess.run(cmd,env=env,text=True,capture_output=True,timeout=45)
 attempt=Path(p.stdout.splitlines()[0].removeprefix('Results: '))
 raw={n:(attempt/n).read_bytes().decode() for n in ('command.json','qmp.log','serial.log','qemu.log','verification_summary.txt') if (attempt/n).exists()}
 record={'name':name,'kind':'unchanged verifier','argv':cmd,'environment':{k:v for k,v in env.items() if k in ('QEMU_IMG','BIOS','OUTPUT_DIR','VERIFICATION_TIMEOUT','QEMU_BIN') or k.startswith('PHASE4_')},'exit':p.returncode,'stdout':p.stdout,'stderr':p.stderr,'firmware_sha256':hashlib.sha256(Path(firmware).read_bytes()).hexdigest(),'image_sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'raw':raw,'executed_qemu_argv':json.loads((out/'qemu-argv.json').read_text()) if matched else json.loads(raw['command.json']),'vars_template_sha256':hashlib.sha256(Path(vars_template).read_bytes()).hexdigest() if vars_template else None}
 rows.append(record);print(name,p.returncode,flush=True)
 (base/'matrix.json').write_text(json.dumps(rows,indent=2)+'\n')
