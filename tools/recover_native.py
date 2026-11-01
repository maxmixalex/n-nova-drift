#!/usr/bin/env python3
"""Recover named YYC routines and literal assignments from a supplied PE.

Requires pefile and capstone. Does not distribute the original executable.
Function boundaries come from PE exception unwind entries; GML names come
from RIP-relative references to the original runtime trace strings. Variable
names come from the YYC variable registration records (name pointer, slot ID).
This is a conservative literal extractor, not a complete GML decompiler.
"""
import argparse, bisect, csv, hashlib, json, pathlib, re, struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG, X86_REG_RIP

class Native:
    def __init__(self, filename):
        self.path=pathlib.Path(filename); self.data=self.path.read_bytes()
        self.pe=pefile.PE(data=self.data); self.base=self.pe.OPTIONAL_HEADER.ImageBase
        self.sections=self.pe.sections
        self.cs=Cs(CS_ARCH_X86,CS_MODE_64);self.cs.detail=True
        self.bounds=sorted((self.base+x.struct.BeginAddress,self.base+x.struct.EndAddress) for x in self.pe.DIRECTORY_ENTRY_EXCEPTION)
        self.starts=[x[0] for x in self.bounds]
        self.names={};self.functions={};self.variables={}
        self.assets={}
        archive=self.path.with_name('data.win')
        if archive.exists():
            with archive.open('rb') as f:
                f.seek(8)
                while True:
                    head=f.read(8)
                    if len(head)!=8:break
                    tag,size=struct.unpack('<4sI',head);start=f.tell()
                    if tag in (b'OBJT',b'SPRT'):
                        count=struct.unpack('<I',f.read(4))[0];offsets=struct.unpack('<%dI'%count,f.read(count*4));names=[]
                        for offset in offsets:
                            f.seek(offset);name_offset=struct.unpack('<I',f.read(4))[0]
                            f.seek(name_offset);names.append(f.read(180).split(b'\0')[0].decode('utf-8','replace'))
                        self.assets[0 if tag==b'OBJT' else 1]=names
                    f.seek(start+size)
        for m in re.finditer(rb'gml_(?:Script|Object|GlobalScript)_[A-Za-z0-9_@]+\x00',self.data):
            self.names[self.base+self.pe.get_rva_from_offset(m.start())]=m.group()[:-1].decode()
        for sec in self.sections:
            if sec.Name.rstrip(b'\0')!=b'.text': continue
            start=sec.PointerToRawData; code=self.data[start:start+sec.SizeOfRawData]
            for m in re.finditer(rb'[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',code):
                p=m.start();va=self.base+sec.VirtualAddress+p
                target=va+7+struct.unpack_from('<i',code,p+3)[0]
                if target not in self.names: continue
                k=bisect.bisect_right(self.starts,va)-1
                if k>=0 and va<self.bounds[k][1]:
                    begin,end=self.bounds[k];self.functions[self.names[target]]=(begin,end,va)
        for sec in self.sections:
            if sec.Name.rstrip(b'\0')!=b'.data':continue
            for i in range(sec.PointerToRawData,sec.PointerToRawData+sec.SizeOfRawData-16,8):
                ptr,slot=struct.unpack_from('<QQ',self.data,i)
                if slot not in (0,0xffffffff):continue
                s=self.string(ptr)
                if s and re.fullmatch(r'[A-Za-z_$][A-Za-z_0-9.$]*',s):
                    self.variables[self.base+self.pe.get_rva_from_offset(i)+8]=s
        self.reverse={v[0]:k for k,v in self.functions.items()}
    def offset(self,va):
        return self.pe.get_offset_from_rva(va-self.base)
    def read(self,va,size):
        try:
            rva=va-self.base
            for sec in self.sections:
                relative=rva-sec.VirtualAddress
                if 0<=relative<sec.Misc_VirtualSize:
                    if relative>=sec.SizeOfRawData:return b'\0'*size
                    o=sec.PointerToRawData+relative
                    return self.data[o:o+min(size,sec.SizeOfRawData-relative)]
            return b'' 
        except (pefile.PEFormatError,TypeError):return b''
    def string(self,va):
        if not self.base<va<self.base+0x2000000:return None
        raw=self.read(va,200).split(b'\0')[0]
        if not raw or any(x<32 or x>126 for x in raw):return None
        return raw.decode()
    def instructions(self,name):
        a,b,_=self.functions[name]
        return list(self.cs.disasm(self.read(a,b-a),a))
    def references(self,i):
        out=[]
        for o in i.operands:
            if o.type==X86_OP_MEM and o.mem.base==X86_REG_RIP:
                va=i.address+i.size+o.mem.disp
                if va in self.variables:out.append(self.variables[va])
                elif va in self.names:out.append(self.names[va])
                elif 'sd' in i.mnemonic:
                    raw=self.read(va,8)
                    if len(raw)==8:out.append('f64='+str(struct.unpack('<d',raw)[0]))
                elif i.mnemonic.startswith(('movups','movaps','lea')):
                    raw=self.read(va,16)
                    if len(raw)==16:
                        val,flags,kind=struct.unpack('<QII',raw)
                        if flags==0 and kind==15:
                            category=(val>>32)&0xff;index=val&0xffffffff
                            if category in self.assets and index<len(self.assets[category]):out.append(self.assets[category][index])
                        elif flags==0 and kind in (0,7,10):
                            v=struct.unpack('<d',raw[:8])[0] if kind==0 else val
                            if abs(v)<1e9:out.append('RValue='+str(v))
            elif i.mnemonic=='call' and o.type==X86_OP_IMM:
                if o.imm in self.reverse:out.append(self.reverse[o.imm])
            elif i.mnemonic=='movabs' and o.type==X86_OP_IMM:
                v=struct.unpack('<d',struct.pack('<Q',o.imm&0xffffffffffffffff))[0]
                if 1e-10<abs(v)<1e10:out.append('f64='+str(v))
                if (o.imm>>56)==1:
                    category=(o.imm>>32)&0xff;index=o.imm&0xffffffff
                    if category in self.assets and index<len(self.assets[category]):out.append(self.assets[category][index])
        return out
    def annotated(self,name):
        lines=[];resource=None
        for i in self.instructions(name):
            refs=self.references(i)
            if i.mnemonic=='mov' and i.op_str.startswith('edx, ') and len(i.operands)==2 and i.operands[1].type==X86_OP_IMM:
                resource=i.operands[1].imm
            if i.mnemonic=='call' and i.op_str=='0x14002bfd0' and resource is not None:
                if 0<=resource<len(self.assets.get(0,[])):refs.append('object '+self.assets[0][resource])
            lines.append('%016x: %-8s %-55s ; %s'%(i.address,i.mnemonic,i.op_str,', '.join(refs)))
        return '\n'.join(lines)+'\n'
    def literals(self,name):
        # A literal must be attached to an immediately preceding variable
        # setter. Branches, arithmetic and computed RHS are not claimed.
        ins=self.instructions(name); result=[]; snapshots=[]; regs={}
        def regkey(r):
            s=self.cs.reg_name(r) or ""
            return {'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','edi':'rdi','esi':'rsi','ebp':'rbp','esp':'rsp'}.get(s,s[:-1] if re.fullmatch(r'r\d+d',s) else s)
        for i in ins:
            snapshots.append(dict(regs));ops=i.operands
            if i.mnemonic in ('mov','movabs','movsd','movaps','movapd','movdqa','movups') and len(ops)==2 and ops[0].type==X86_OP_REG:
                dest=regkey(ops[0].reg);val=None;o=ops[1]
                if o.type==X86_OP_IMM:val=o.imm
                elif o.type==X86_OP_REG:val=regs.get(regkey(o.reg))
                elif o.type==X86_OP_MEM and o.mem.base==X86_REG_RIP and dest.startswith('xmm'):
                    data=self.read(i.address+i.size+o.mem.disp,8)
                    if len(data)==8:val=struct.unpack('<d',data)[0]
                if val is None:regs.pop(dest,None)
                else:regs[dest]=val
            elif i.mnemonic in ('xor','xorps','xorpd','pxor') and len(ops)==2 and ops[0].type==X86_OP_REG and ops[1].type==X86_OP_REG and ops[0].reg==ops[1].reg:
                regs[regkey(ops[0].reg)]=0
            elif i.mnemonic=='lea' and len(ops)==2 and ops[0].type==X86_OP_REG:
                o=ops[1];base=regs.get(regkey(o.mem.base));dest=regkey(ops[0].reg)
                if base is not None and not o.mem.index:regs[dest]=base+o.mem.disp
                else:regs.pop(dest,None)
            elif i.mnemonic=='call':
                for r in ('rax','rcx','rdx','r8','r9','r10','r11','xmm0','xmm1','xmm2','xmm3','xmm4','xmm5'):regs.pop(r,None)
            elif ops and ops[0].type==X86_OP_REG and i.mnemonic not in ('test','cmp','push','ucomisd','comisd'):
                regs.pop(regkey(ops[0].reg),None)
        for k,i in enumerate(ins):
            if i.mnemonic!='mov' or not i.operands or i.operands[0].type!=X86_OP_REG or i.reg_name(i.operands[0].reg)!='edx':continue
            names=self.references(i)
            if len(names)!=1 or names[0].startswith('gml_'):continue
            var=names[0]
            tail=ins[k+1:k+28]
            setter=next((j for j,t in enumerate(tail[:4]) if t.mnemonic=='call' and ('+ 0x10]' in t.op_str or t.op_str=='0x14002bd20')),None)
            if setter is None:continue
            for j,t in enumerate(tail[setter+1:]):
                if t.mnemonic=='call' and t.op_str in ('0x14002c060','0x1400392c0'):
                    state=snapshots[k+1+setter+1+j];key='rdx' if t.op_str=='0x14002c060' else 'xmm1'
                    if key in state:
                        result.append((var,state[key],'integer' if key=='rdx' else 'real',name,hex(i.address),hex(t.address)))
                    break
                if t.mnemonic=='mov' and len(t.operands)==2 and t.operands[0].type==X86_OP_REG and t.reg_name(t.operands[0].reg)=='edx' and t.operands[1].type==X86_OP_IMM:
                    following=tail[setter+1+j+1:setter+1+j+4]
                    if any(u.mnemonic=='call' and u.op_str=='0x14002c060' for u in following):
                        result.append((var,t.operands[1].imm,'integer',name,hex(i.address),hex(t.address)));break
                if t.mnemonic=='movabs' and t.operands[1].type==X86_OP_IMM:
                    v=struct.unpack('<d',struct.pack('<Q',t.operands[1].imm&0xffffffffffffffff))[0]
                    following=tail[setter+1+j+1:setter+1+j+4]
                    if any(u.mnemonic=='mov' and u.op_str.startswith('qword ptr [rbx],') for u in following):
                        if abs(v)<1e10:result.append((var,v,'real',name,hex(i.address),hex(t.address)))
                        break
                if t.mnemonic=='mov' and self.references(t) and t.op_str.startswith('edx,'):break
        return result

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('--out',required=True)
    p.add_argument('--routine',action='append',default=[]);p.add_argument('--match')
    args=p.parse_args();n=Native(args.exe);out=pathlib.Path(args.out);out.mkdir(parents=True,exist_ok=True)
    with (out/'native_functions.csv').open('w') as f:
        w=csv.writer(f);w.writerow(['routine','start_va','end_va','trace_reference_va','bytes'])
        for name,(start,end,ref) in sorted(n.functions.items()):w.writerow([name,hex(start),hex(end),hex(ref),end-start])
    with (out/'native_variables.csv').open('w') as f:
        w=csv.writer(f);w.writerow(['slot_va','name'])
        for va,name in sorted(n.variables.items()):w.writerow([hex(va),name])
    selected=list(args.routine)
    if args.match:selected += [x for x in n.functions if re.search(args.match,x)]
    literals=[]
    for name in dict.fromkeys(selected):
        if name not in n.functions:raise SystemExit('Unknown routine '+name)
        (out/(name+'.asm')).write_text(n.annotated(name));literals+=n.literals(name)
    with (out/'native_constants.csv').open('w') as f:
        w=csv.writer(f);w.writerow(['variable','value','type','routine','setter_va','literal_va']);w.writerows(literals)
    summary={'sha256':hashlib.sha256(n.data).hexdigest(),'functions':len(n.functions),'variables':len(n.variables),'selected':len(set(selected)),'literal_assignments':len(literals)}
    (out/'manifest.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary))

if __name__=='__main__':main()
