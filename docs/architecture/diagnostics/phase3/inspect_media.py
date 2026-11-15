from pathlib import Path
import struct,hashlib,json,zlib
import argparse
a=argparse.ArgumentParser();a.add_argument('--image',default='drivers/qemu/production_image/qemu_slice6_guest.img');a.add_argument('--staged',default='drivers/qemu/production_image/staging/EFI/BOOT/BOOTAA64.EFI');args=a.parse_args()
p=Path(args.image);b=p.read_bytes()
u16=lambda data,o:struct.unpack_from('<H',data,o)[0]
u32=lambda data,o:struct.unpack_from('<I',data,o)[0]
u64=lambda data,o:struct.unpack_from('<Q',data,o)[0]
h=b[512:1024];parts=u64(h,72)*512;count=u32(h,80);entry=u32(h,84)
record={'image_sha256':hashlib.sha256(b).hexdigest(),'gpt_signature':h[:8].decode(),'partitions':[]}
header=bytearray(h[:u32(h,12)]);header[16:20]=bytes(4)
record['header_crc_valid']=zlib.crc32(header)==u32(h,16)
record['entries_crc_valid']=zlib.crc32(b[parts:parts+count*entry])==u32(h,88)
for i in range(count):
 e=b[parts+i*entry:parts+(i+1)*entry]
 if not any(e[:16]):continue
 start,end=u64(e,32),u64(e,40);record['partitions'].append({'start':start,'end':end,'type_guid_raw':e[:16].hex()})
start=record['partitions'][0]['start'];o=start*512;s=b[o:o+512];sector=u16(s,11);cluster=s[13];res=u16(s,14);fats=s[16];fat_size=u32(s,36);root=u32(s,44)
fat=o+res*sector;data=o+(res+fats*fat_size)*sector;size=sector*cluster

def chain(c):
 out=b'';seen=set()
 while 2<=c<0xffffff8:
  if c in seen:raise ValueError('FAT cycle')
  seen.add(c);out+=b[data+(c-2)*size:data+(c-1)*size];c=u32(b,fat+c*4)&0xfffffff
 return out

def listing(c):
 entries=[]
 for idx in range(0,len(chain(c)),32):
  e=chain(c)[idx:idx+32]
  if e[0]==0:break
  if e[0]==0xe5 or e[11]==0xf:continue
  name=e[:8].decode('ascii').rstrip();ext=e[8:11].decode('ascii').rstrip()
  entries.append({'name':name+('.'+ext if ext else ''),'cluster':(u16(e,20)<<16)|u16(e,26),'size':u32(e,28),'directory':bool(e[11]&16)})
 return entries
record['root']=listing(root)
def resolve(path):
 c=root
 for name in path.split('/'):
  e=next(e for e in listing(c) if e['name']==name);c=e['cluster']
 return chain(c)[:e['size']]
efi=resolve('EFI/BOOT/BOOTAA64.EFI');None  # Inspection is read-only; extracted bytes remain in memory.
record['embedded_efi_sha256']=hashlib.sha256(efi).hexdigest();record['embedded_matches_staging']=efi==Path(args.staged).read_bytes()
pe=u32(efi,0x3c);optional=pe+24
record['pe']={'signature':efi[pe:pe+4].hex(),'machine':hex(u16(efi,pe+4)),'optional_magic':hex(u16(efi,optional)),'subsystem':u16(efi,optional+68),'image_base':hex(u64(efi,optional+24)),'relocation_rva':u32(efi,optional+112+5*8),'relocation_bytes':u32(efi,optional+112+5*8+4)}
record['embedded_efi_bytes']=len(efi);record['contains_ready_marker']=b'[axion] t81sh: ready (principal=axion, tier=1)' in efi
record['contains_debug_wfi']=b'DEBUG: WFI loop iteration' in efi
record['profile']=resolve('TERNOS/PROFILE.TXT').decode()
files={}
def walk(c,prefix='',depth=0):
 if depth>16:raise ValueError('unexpected directory depth')
 for entry in listing(c):
  if entry['name'] in ('.','..'):continue
  path=prefix+entry['name']
  if entry['directory']:walk(entry['cluster'],path+'/',depth+1)
  else:files[path]=hashlib.sha256(chain(entry['cluster'])[:entry['size']]).hexdigest()
walk(root)
record['file_tree_sha256']=files
print(json.dumps(record,indent=2))
