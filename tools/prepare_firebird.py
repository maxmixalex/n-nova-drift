#!/usr/bin/env python3
"""Make an isolated Firebird test session from user-supplied calculator images.

Firebird version-3 snapshots contain their own Boot1/NAND paths and override
the command line paths. The snapshot and NAND MUST be a matching pair from the
same emulator checkpoint; if uncertain, boot the flash normally and install
Ndless again instead of resuming a stale snapshot. This copies the NAND and changes only those documented
header path fields; no emulated RAM, executable, OS code, or flags are changed.
The input files are never modified. No images are downloaded or distributed.
"""
from pathlib import Path
import argparse
import gzip
import shutil
import struct

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--boot1",type=Path,required=True)
    parser.add_argument("--flash",type=Path,required=True)
    parser.add_argument("--snapshot",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    boot=args.boot1.resolve(strict=True)
    original=args.flash.resolve(strict=True)
    source=args.snapshot.resolve(strict=True)
    target=args.output.resolve()
    target.mkdir(parents=True,exist_ok=True)
    flash=target/"nand.bin";snapshot=target/"installed.snapshot"
    if flash.exists() or snapshot.exists(): parser.error("Output already contains a session; choose a new empty directory.")
    raw=bytearray(gzip.decompress(source.read_bytes()))
    if len(raw)<1032 or struct.unpack_from("<II",raw)!=(0xcafebee0,3): parser.error("Expected a Firebird version-3 snapshot.")
    for offset,path in ((8,boot),(520,flash)):
        value=str(path).encode()
        if len(value)>=512: parser.error("Snapshot paths must fit 511 bytes.")
        raw[offset:offset+512]=value+b"\0"*(512-len(value))
    shutil.copyfile(original,flash)
    snapshot.write_bytes(gzip.compress(raw,compresslevel=1))
    (target/"probe.commands").write_text(f'status\nscreen "{target}/initial.ppm"\ndir "/"\nquit\n')
    print(f"Prepared {target}\nUse firebird-control --boot1 '{boot}' --flash '{flash}' --snapshot '{snapshot}' --commands '{target}/probe.commands'")

if __name__=="__main__": main()
