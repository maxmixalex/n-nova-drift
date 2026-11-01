#!/usr/bin/env python3
"""Strict ELF32 ARM -> Zehn packer for this freestanding LLVM build.

No inferred relocations: every absolute word requires an ELF relocation.
Unknown relocation kinds, TLS, constructors and undefined symbols fail closed.
Format follows the Ndless r2022 zehn.h and genzehn.cpp allocation convention.
"""
# Adapted from nSpire, Copyright (c) 2026 Max Alex; MIT license.
# See third_party/licenses/nSpire-MIT.txt. Format reference: official Ndless
# ndless-sdk/include/zehn.h, commit e52415fb568e92cd36ed049067b3e9c2727b84af.
ALLOCATION_LIMIT = 24 * 1024 * 1024
# Normal CX II CAS OS 6.4/r2022 also needs a sufficiently large *contiguous*
# block after loader metadata has been allocated. Keep headroom below the
# observed limit; the total 24 MiB guard alone did not prevent UI15 reopens
# from failing. This build budget is not a guarantee for arbitrary OS heaps.
EXECUTION_MEMORY_LIMIT = 22_450_000
RELOCATION_ADDRESS_LIMIT = 1 << 24

from pathlib import Path
import struct
import zlib
import argparse

def compress_image(image:bytes,compression:str='zlib')->bytes:
    """Keep the loader's zlib stream; spend release-build time on encoding.

    Zopfli changes neither executable bytes nor the calculator decoder. Use
    two fixed, bounded profiles and retain the smallest exact stream. The
    second profile improves splitting without removing the established one.
    """
    if compression not in ('zlib','zopfli'):
        raise ValueError('Unknown Zehn compression: '+compression)
    encoded=zlib.compress(image,9)
    if compression=='zopfli':
        try:
            from zopfli.zlib import compress
        except ImportError as error:
            raise RuntimeError('Release compression needs: python3 -m pip install -r requirements-release.txt') from error
        raw=bytes(image)
        for iterations,blocks in ((5,15),(20,64)):
            candidate=compress(raw,numiterations=iterations,blocksplitting=True,blocksplittingmax=blocks)
            if len(candidate)>=len(encoded):continue
            decoder=zlib.decompressobj()
            try:
                decoded=decoder.decompress(candidate,len(raw)+1)
            except zlib.error as error:
                raise ValueError('Release compressor produced an invalid zlib stream') from error
            if decoded!=raw or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
                raise ValueError('Release compressor changed loaded bytes or left trailing data')
            encoded=candidate
    return encoded

def build(source:Path,target:Path,compression:str='zlib')->None:
    b=source.read_bytes()
    if b[:7]!=b'\x7fELF\x01\x01\x01':raise ValueError('Expected ELF32 little-endian')
    et,machine,ver,entry,phoff,shoff,flags,ehsize,phsize,phnum,shsize,shnum,shstr=struct.unpack_from('<HH5I6H',b,16)
    if et!=2 or machine!=40 or shsize!=40:raise ValueError('Expected linked ARM executable')
    sec=[struct.unpack_from('<10I',b,shoff+i*40) for i in range(shnum)]
    names=b[sec[shstr][4]:sec[shstr][4]+sec[shstr][5]]
    def name(s):return names[s[0]:].split(b'\0',1)[0].decode()
    alloc=[s for s in sec if s[2]&2 and s[5]]
    if not alloc or min(s[3] for s in alloc)!=0:raise ValueError('Image must be linked at zero')
    for s in alloc:
        if s[2]&0x400 or name(s) in ('.init_array','.fini_array','.ctors'):raise ValueError('TLS/constructors unsupported')
    end=max(s[3]+s[5] for s in alloc if s[1]!=8)
    memory_end=max(s[3]+s[5] for s in alloc)
    if memory_end>ALLOCATION_LIMIT or entry>=end:raise ValueError('Invalid image size or entry')
    if memory_end>EXECUTION_MEMORY_LIMIT:
        raise ValueError(f'Zehn executable/BSS exceeds contiguous-memory budget: {memory_end} > {EXECUTION_MEMORY_LIMIT}')
    image=bytearray(end);covered=bytearray(end)
    for s in alloc:
        if s[1]==8:
            if s[3]<end:raise ValueError('Non-tail NOBITS section unsupported')
            continue
        if s[4]+s[5]>len(b) or any(covered[s[3]:s[3]+s[5]]):raise ValueError('Section bounds/overlap')
        image[s[3]:s[3]+s[5]]=b[s[4]:s[4]+s[5]];covered[s[3]:s[3]+s[5]]=b'\1'*s[5]
    relocs=set()
    for s in sec:
        if s[1]==2: # symtab
            for off in range(s[4],s[4]+s[5],s[9]):
                stname,value,size,info,other,section=struct.unpack_from('<IIIBBH',b,off)
                if section==0 and stname and info>>4!=2:raise ValueError('Unresolved strong symbol')
        if s[1]!=9:continue
        if not sec[s[7]][2]&2:continue
        symtab=sec[s[6]]
        for off in range(s[4],s[4]+s[5],8):
            addr,info=struct.unpack_from('<2I',b,off);kind=info&255;symbol=info>>8
            sym=struct.unpack_from('<IIIBBH',b,symtab[4]+symbol*symtab[9])
            if kind in (2,38): # R_ARM_ABS32 / TARGET1
                if sym[5]==0:raise ValueError('Absolute relocation against undefined symbol')
                if sym[5]==0xfff1:continue # Absolute linker constant, not an image pointer.
                if addr%4 or addr+4>end or addr>=RELOCATION_ADDRESS_LIMIT:
                    raise ValueError('Unaligned/out-of-range relocation')
                value=struct.unpack_from('<I',image,addr)[0]
                if value>memory_end:raise ValueError(f'Absolute word outside image: {value:x}')
                relocs.add(addr)
            elif kind not in (0,3,10,28,29,30,42):
                raise ValueError(f'Unsupported relocation type {kind} at {addr:x}')
    name=b'nNova\0';author=b'Max Alex\0';notice=b'Nova Drift inspired TI-Nspire adaptation; requires external data.\0'
    extra=name+author+notice
    extra+=bytes((-len(extra))%4)
    flag_pairs=[(0,31),(2,2022),(4,1),(5,0),(6,1),(7,0),(8,0),(9,len(name)),(10,1),(11,len(name)+len(author)),(12,1),(13,1)]
    reloc_data=struct.pack('<I',3)+b''.join(struct.pack('<I',r<<8) for r in sorted(relocs))
    flag_data=b''.join(struct.pack('<I',kind|(data<<8)) for kind,data in flag_pairs)
    prefix=32+len(reloc_data)+len(flag_data)+len(extra)
    allocation=prefix+memory_end
    # Ndless allocates the Zehn prefix as well as the loaded image/BSS. An
    # image below 24 MiB can still exceed the budget once relocations and
    # flags are included. Relocation *locations* remain separately limited
    # to 24 bits by Zehn; values may point into the larger zeroed BSS.
    if allocation>ALLOCATION_LIMIT:raise ValueError(f'Zehn allocation exceeds 24 MiB: {allocation}')
    compressed=compress_image(image,compression)
    result=struct.pack('<8I',0x6e68655A,1,prefix+len(compressed),len(relocs)+1,len(flag_pairs),len(extra),allocation,entry)+reloc_data+flag_data+extra+compressed
    # Independent round trip validates bytes, allocation and every rebase word.
    decoded=zlib.decompress(result[prefix:]);assert decoded==image
    for base in (0x10100000,0x11000000):
        relocated=bytearray(decoded)+bytearray(memory_end-end)
        for r in relocs:
            v=struct.unpack_from('<I',relocated,r)[0];struct.pack_into('<I',relocated,r,v+base)
        for r in relocs:assert struct.unpack_from('<I',relocated,r)[0]-base==struct.unpack_from('<I',image,r)[0]
    target.write_bytes(result)
    print(f'{target}: {len(result):,} bytes; allocation {allocation:,}; {len(relocs)} absolute relocations; entry 0x{entry:x}; compression {compression}')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('input',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--compression',choices=('zlib','zopfli'),default='zlib',help='zopfli spends offline build time on a smaller compatible zlib stream')
    a=p.parse_args();build(a.input,a.output,a.compression)
