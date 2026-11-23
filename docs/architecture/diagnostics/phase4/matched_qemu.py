#!/usr/bin/env python3
"""Transparent diagnostic argv adapter; no evidence or exit-status filtering."""
import json,os,sys
from pathlib import Path
argv=sys.argv[1:]
if argv.count('-cpu')!=1:raise SystemExit('expected one CPU argument')
argv[argv.index('-cpu')+1]='max'
if os.environ.get('PHASE4_VARS'):
 i=argv.index('-bios');code=argv[i+1];del argv[i:i+2]
 argv+=['-drive',f'if=pflash,format=raw,readonly=on,file={code}',
        '-drive','if=pflash,format=raw,file='+os.environ['PHASE4_VARS']]
argv+=['-nodefaults','-device','virtio-rng-pci','-device','ramfb']
command=['/opt/homebrew/bin/qemu-system-aarch64',*argv]
Path(os.environ['PHASE4_ARGV_RECORD']).write_text(json.dumps(command,indent=2)+'\n')
os.execv(command[0],command)
