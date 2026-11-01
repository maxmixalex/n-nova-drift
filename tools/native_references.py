#!/usr/bin/env python3
"""Print PE RIP references to named YYC variable-registration slots.

Uses the same input-only evidence parser as recover_native.py. The matches are
validated through capstone before assigning the containing unwind routine.
"""
import argparse,bisect,re,struct
from recover_native import Native
p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('variables',nargs='+');a=p.parse_args();n=Native(a.exe)
targets={va:name for va,name in n.variables.items() if name in a.variables}
for sec in n.sections:
 if sec.Name.rstrip(b'\0')!=b'.text':continue
 code=n.data[sec.PointerToRawData:sec.PointerToRawData+sec.SizeOfRawData]
 for m in re.finditer(rb'\x8b[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',code):
  off=m.start();va=n.base+sec.VirtualAddress+off
  if off+6>len(code):continue
  target=va+6+struct.unpack_from('<i',code,off+2)[0]
  if target not in targets:continue
  i=bisect.bisect_right(n.starts,va)-1
  if i<0 or va>=n.bounds[i][1]:continue
  start,end=n.bounds[i];print(targets[target],hex(va),n.reverse.get(start,hex(start)))
