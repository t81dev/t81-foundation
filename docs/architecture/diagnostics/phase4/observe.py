import argparse,json,os,shutil,socket,subprocess,tempfile,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('name');p.add_argument('--image',required=True);p.add_argument('--bios',required=True);p.add_argument('--vars');p.add_argument('--ramfb',action='store_true');p.add_argument('--rng',action='store_true');p.add_argument('--cpu',default='cortex-a57');p.add_argument('--no-disk',action='store_true');p.add_argument('--pflash',action='store_true');p.add_argument('--nodefaults',action='store_true');p.add_argument('--seconds',type=float,default=20);a=p.parse_args()
out=Path('/private/tmp/t81-phase4')/a.name;out.mkdir(exist_ok=True)
shutil.copyfile(a.image,out/'guest.img')
bios=a.bios
with tempfile.TemporaryDirectory(prefix='p3-',dir='/tmp') as tmp:
 qmp=Path(tmp)/'qmp';serial=Path(tmp)/'serial'
 cmd=['qemu-system-aarch64','-machine','virt,gic-version=3','-cpu',a.cpu,'-m','512','-smp','1','-nographic','-monitor','none','-qmp',f'unix:{qmp},server=on,wait=off','-chardev',f'socket,id=s,path={serial},server=on,wait=off,logfile={out}/serial.log','-serial','chardev:s','-drive',f'file={out}/guest.img,format=raw,if=none,id=hd0','-device','virtio-blk-device,drive=hd0']
 if a.rng:cmd+=['-device','virtio-rng-pci']
 if a.ramfb:cmd+=['-device','ramfb']
 if a.no_disk:
  start=cmd.index('-drive');del cmd[start:start+4]
 if a.pflash:
  shutil.copyfile(a.vars or '/opt/homebrew/share/qemu/edk2-arm-vars.fd',out/'vars.fd')
  cmd+=['-drive',f'if=pflash,format=raw,readonly=on,file={bios}','-drive',f'if=pflash,format=raw,file={out}/vars.fd']
 else: cmd+=['-bios',bios]
 if a.nodefaults:cmd+=['-nodefaults']
 (out/'command.json').write_text(json.dumps(cmd,indent=2)+'\n')
 process=None; events=[]; sent=False;offset=None
 try:
  with (out/'qemu.log').open('wb') as log:
   process=subprocess.Popen(cmd,stdin=subprocess.DEVNULL,stdout=log,stderr=log)
   deadline=time.monotonic()+a.seconds
   while not qmp.exists() and process.poll() is None and time.monotonic()<deadline:time.sleep(.05)
   with socket.socket(socket.AF_UNIX) as connection:
    connection.settimeout(2);connection.connect(str(qmp)); stream=connection.makefile('rb')
    def receive():
     line=stream.readline(); events.append(line.decode());return json.loads(line)
    receive()
    def command(name):
     connection.sendall((json.dumps({'execute':name})+'\n').encode())
     while True:
      r=receive()
      if 'return' in r or 'error' in r:return r
    command('qmp_capabilities');command('query-status')
    while process.poll() is None and time.monotonic()<deadline:
     text=(out/'serial.log').read_text(errors='replace') if (out/'serial.log').exists() else ''
     if '[axion] t81sh: ready (principal=axion, tier=1)' in text and not sent:
      with socket.socket(socket.AF_UNIX) as console:
       console.settimeout(2);console.connect(str(serial));offset=len(text);console.sendall(b'help\r');sent=True
     time.sleep(.1)
    if process.poll() is None:command('query-status');command('quit')
    code=process.wait(timeout=3)
 except Exception as exc:
  events.append(repr(exc)+'\n');code=None
 finally:
  if process and process.poll() is None:
   process.terminate()
   try:process.wait(timeout=2)
   except subprocess.TimeoutExpired:process.kill();process.wait()
 (out/'qmp.log').write_text(''.join(events))
 text=(out/'serial.log').read_text(errors='replace') if (out/'serial.log').exists() else ''
 record={'name':a.name,'qemu_exit':code,'help_sent':sent,'ready':'[axion] t81sh: ready (principal=axion, tier=1)' in text,'fresh_help':sent and 'show RFC-00B9 shell commands' in text[offset:],'observation_only':True}
 (out/'result.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record),flush=True)
