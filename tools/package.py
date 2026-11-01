#!/usr/bin/env python3
"""Assemble a calculator transfer ZIP; refuse incomplete data installations."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess
import zlib
import zipfile

ROOT = Path(__file__).resolve().parents[1]

LANGUAGES=("eng","deu","spa","fra","jpn","kor","por","rus","pol","zhs","zht")
FONT_LICENSES=("Roboto_LICENSE.txt","03SmartFontUI_IPA_Font_License_Agreement_v1.0.txt",
               "NotoSansKR-Medium_OFL.txt","NotoSansSC-Medium_OFL.txt","Tapei Sans SIL.txt")

def validate_data(directory):
    """Validate the complete transfer boundary, including shared font groups."""
    index_path=directory/"nNova.dat.tns"
    if not index_path.is_file(): raise ValueError("Missing data/nNova.dat.tns; run the converter first.")
    data=index_path.read_bytes()
    if len(data)<64: raise ValueError("Truncated graphics index.")
    h=struct.unpack_from("<8s10I2H3I",data)
    magic,version,sprites,frames,packs,sprite_at,frame_at,total,_,_,crc,width,height,*_=h
    if (magic!=b"NNOVAC1\0" or version not in (1,2,3) or not 0<sprites<=1600 or
        not 0<frames<=4096 or not 0<packs<=32 or sprite_at!=64 or
        frame_at!=64+sprites*76 or total!=frame_at+frames*32 or total!=len(data) or
        (width,height)!=(320,240) or zlib.crc32(data[64:])!=crc):
        raise ValueError("Invalid graphics index or CRC.")
    expected={f"nNova.gfx{i:02}.dat.tns" for i in range(packs)}
    actual={p.name for p in directory.glob("nNova.gfx*.dat.tns")}
    if expected!=actual: raise ValueError("Missing or stale graphics chunks: "+", ".join(sorted(expected^actual)))
    lengths=[]
    for i in range(packs):
        content=(directory/f"nNova.gfx{i:02}.dat.tns").read_bytes()
        if len(content)<32: raise ValueError("Truncated graphics chunk.")
        m,v,number,size,checksum,*_=struct.unpack_from("<8s6I",content)
        if m!=b"NNOVAP1\0" or v!=version or number!=i or size!=len(content) or zlib.crc32(content[32:])!=checksum:
            raise ValueError("Invalid graphics chunk or CRC.")
        lengths.append(len(content))
    for i in range(frames):
        off,stored,decoded,chunk,w,h,_,_,codec,*_=struct.unpack_from("<IIIHHHhhHHHHH",data,frame_at+i*32)
        if (chunk>=packs or not w or not h or w>320 or h>240 or decoded!=w*h*3 or
            codec>({1:1,2:5,3:6}[version]) or off<32 or off>lengths[chunk] or
            not stored or stored>lengths[chunk]-off):
            raise ValueError("Invalid frame reference.")
    languages=[code for code in LANGUAGES if (directory/f"nNova.{code}.locale.dat.tns").is_file()]
    if not languages: raise ValueError("No language text packs; conversion is incomplete.")
    for code in languages:
        choices=[directory/f"nNova.{code}.font.dat.tns"]
        if code not in ("jpn","kor","zhs","zht"):
            choices.extend((directory/"nNova.lat.font.dat.tns",directory/"nNova.eng.font.dat.tns"))
        if not any(p.is_file() for p in choices): raise ValueError("Missing font for language: "+code)
    licenses=FONT_LICENSES if version>=2 else ("Roboto_LICENSE.txt","NotoSansSC-Medium_OFL.txt")
    for name in licenses:
        if not (directory/name).is_file(): raise ValueError("Missing font license: "+name)
    blobs=sorted(directory.glob("*.dat.tns"))
    return blobs,licenses,{"asset_version":version,"sprites":sprites,"frames":frames,
                           "languages":languages,"data_bytes":sum(p.stat().st_size for p in blobs)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--launcher",type=Path,default=ROOT/"dist/nNova.tns")
    parser.add_argument("--data",type=Path,default=ROOT/"dist/data")
    parser.add_argument("--output",type=Path,default=ROOT/"dist/nNova-TI-Nspire.zip")
    parser.add_argument("--converter-linux",type=Path,help="Optional standalone Linux converter binary")
    parser.add_argument("--converter-windows",type=Path,help="Optional standalone Windows converter executable")
    args=parser.parse_args()
    if not args.launcher.is_file(): parser.error("Missing ARM launcher; run make ndless.")
    try: blobs,licenses,metadata=validate_data(args.data)
    except (ValueError,OSError) as error: parser.error(str(error))
    files=[("nNova/nNova.tns",args.launcher)]+[("nNova/data/"+p.name,p) for p in blobs]
    for name in (*licenses,"conversion-report.txt","sprites.csv"):
        if (args.data/name).is_file(): files.append(("nNova/data/"+name,args.data/name))
    for name in ("README.md","CONTROLS.md","LICENSE","THIRD_PARTY.md","THIRD_PARTY_NOTICES.md"):
        if (ROOT/name).is_file(): files.append(("nNova/"+name,ROOT/name))
    for license_file in sorted((ROOT/"third_party/licenses").glob("*.txt")):
        files.append(("nNova/third_party/licenses/"+license_file.name,license_file))
    # Keep the README's evidence links usable in the downloaded archive.
    for document in sorted((ROOT/"docs").rglob("*")):
        if document.is_file() and document.suffix in (".md", ".csv", ".json"):
            files.append(("nNova/"+document.relative_to(ROOT).as_posix(),document))
    converters=[]
    for path,name in ((args.converter_linux,"converter"),(args.converter_windows,"converter.exe")):
        if path is not None:
            if not path.is_file() or not path.stat().st_size: parser.error("Missing or empty converter: "+str(path))
            files.append(("converter/"+name,path));converters.append(name)
    if converters:
        files.extend(("converter/"+name,ROOT/"tools/converter"/name) for name in ("README.md","QUICKSTART.txt"))
        for notice in sorted((ROOT/"tools/converter/third_party").glob("*")):
            if notice.is_file() and ("LICENSE" in notice.name or notice.name=="NOTICE.md"):
                files.append(("converter/third_party/"+notice.name,notice))
    provenance={}
    try:
        revision=subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True,stderr=subprocess.DEVNULL).strip()
        dirty=bool(subprocess.check_output(["git","status","--porcelain"],cwd=ROOT,text=True,stderr=subprocess.DEVNULL).strip())
        provenance={"source_commit":revision,"source_worktree_dirty":dirty}
    except (OSError,subprocess.CalledProcessError):
        pass  # Source archives can be packaged without a local Git checkout.
    manifest={"format":2,"version":"0.2.0","desktop_converters":converters,"launcher":"nNova.tns",**provenance,**metadata,"data_files":[p.name for p in blobs],"sha256":{name:hashlib.sha256(path.read_bytes()).hexdigest() for name,path in files}}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,"w",zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,path in files: archive.write(path,name)
        archive.writestr("nNova/state/",b"")
        archive.writestr("nNova/manifest.json",json.dumps(manifest,indent=2)+"\n")
    with zipfile.ZipFile(args.output) as archive:
        if archive.testzip(): raise RuntimeError("ZIP integrity verification failed")
    print(f"{args.output}: {args.output.stat().st_size:,} bytes; launcher + {len(blobs)} data files + empty state/")

if __name__=="__main__": main()
