"""Controlled single-file substitution on a disposable GPT/FAT32 image copy.

Investigation fixture, not a production image builder or acceptance verifier.
"""
import argparse,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('image',type=Path);p.add_argument('efi',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
if a.image.resolve()==a.output.resolve():raise SystemExit('output must be a separate disposable copy')
b=bytearray(a.image.read_bytes());payload=a.efi.read_bytes();u16=lambda o:struct.unpack_from('<H',b,o)[0];u32=lambda o:struct.unpack_from('<I',b,o)[0]
entries=struct.unpack_from('<Q',b,512+72)[0]*512;part=struct.unpack_from('<Q',b,entries+32)[0]*512
sector=u16(part+11);size=sector*b[part+13];reserved=u16(part+14);fats=b[part+16];fat_size=u32(part+36)*sector;fat=part+reserved*sector;data=fat+fats*fat_size
if sector!=512 or fats!=2 or not payload.startswith(b'MZ'):raise SystemExit('fixture requires expected FAT32/PE inputs')
def chain(c):
 values=[]
 while 2<=c<0xffffff8:
  if c in values:raise SystemExit('FAT cycle')
  values.append(c);c=u32(fat+4*c)&0xfffffff
 return values
c=u32(part+44)
for name in ('EFI','BOOT','BOOTAA64.EFI'):
 found=None
 for cluster in chain(c):
  start=data+(cluster-2)*size
  for off in range(start,start+size,32):
   if b[off] in (0,0xe5) or b[off+11]==0xf:continue
   n=bytes(b[off:off+8]).decode().rstrip();ext=bytes(b[off+8:off+11]).decode().rstrip();n+=('.'+ext if ext else '')
   if n==name:found=off;break
  if found is not None:break
 if found is None:raise SystemExit('missing '+name)
 c=(u16(found+20)<<16)|u16(found+26)
old=chain(c);old_size=u32(found+28)
old_payload=b''.join(b[data+(c-2)*size:data+(c-1)*size] for c in old)[:old_size]
needed=(len(payload)+size-1)//size;selected=old[:needed]
for cluster in range(2,fat_size//4):
 if len(selected)>=needed:break
 if u32(fat+cluster*4)&0xfffffff==0:selected.append(cluster)
if len(selected)!=needed:raise SystemExit('not enough free clusters')
for copy in range(fats):
 for cluster in old:struct.pack_into('<I',b,fat+copy*fat_size+cluster*4,0)
 for i,cluster in enumerate(selected):
  struct.pack_into('<I',b,fat+copy*fat_size+cluster*4,selected[i+1] if i+1<len(selected) else 0xfffffff)
for i,cluster in enumerate(selected):
 start=data+(cluster-2)*size;block=payload[i*size:(i+1)*size];b[start:start+size]=block+bytes(size-len(block))
struct.pack_into('<I',b,found+28,len(payload));a.output.write_bytes(b)
h=lambda v:hashlib.sha256(v).hexdigest()
print(json.dumps({'input_image_sha256':h(a.image.read_bytes()),'old_efi_sha256':h(old_payload),'new_efi_sha256':h(payload),'output_image_sha256':h(b),'old_bytes':old_size,'new_bytes':len(payload),'only_intended_file':'EFI/BOOT/BOOTAA64.EFI','acceptance_claim':False},indent=2))
