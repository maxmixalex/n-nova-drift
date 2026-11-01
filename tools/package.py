#!/usr/bin/env python3
"""Assemble a calculator transfer ZIP; refuse incomplete data installations."""
from pathlib import Path
import argparse
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--launcher",type=Path,default=ROOT/"dist/nNova.tns")
    parser.add_argument("--data",type=Path,default=ROOT/"dist/data")
    parser.add_argument("--output",type=Path,default=ROOT/"dist/nNova-TI-Nspire.zip")
    args=parser.parse_args()
    if not args.launcher.is_file(): parser.error("Missing ARM launcher; run make ndless.")
    if not (args.data/"nNova.dat.tns").is_file(): parser.error("Missing data/nNova.dat.tns; run the converter first.")
    blobs=sorted(args.data.glob("*.dat.tns"))
    if not any(p.name.startswith("nNova.gfx") for p in blobs): parser.error("No graphics chunks; conversion is incomplete.")
    files=[("nNova/nNova.tns",args.launcher)]+[("nNova/data/"+p.name,p) for p in blobs]
    for name in ("Roboto_LICENSE.txt","NotoSansSC-Medium_OFL.txt","conversion-report.txt","sprites.csv"):
        if (args.data/name).is_file(): files.append(("nNova/data/"+name,args.data/name))
    for license_name in ("Roboto_LICENSE.txt","NotoSansSC-Medium_OFL.txt"):
        if not (args.data/license_name).is_file(): parser.error("Missing font license: "+license_name)
    for name in ("README.md","CONTROLS.md","LICENSE","THIRD_PARTY.md","THIRD_PARTY_NOTICES.md"):
        if (ROOT/name).is_file(): files.append(("nNova/"+name,ROOT/name))
    for license_file in sorted((ROOT/"third_party/licenses").glob("*.txt")):
        files.append(("nNova/third_party/licenses/"+license_file.name,license_file))
    manifest={"format":1,"launcher":"nNova.tns","data_files":[p.name for p in blobs],"sha256":{name:hashlib.sha256(path.read_bytes()).hexdigest() for name,path in files}}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,"w",zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,path in files: archive.write(path,name)
        archive.writestr("nNova/state/",b"")
        archive.writestr("nNova/manifest.json",json.dumps(manifest,indent=2)+"\n")
    with zipfile.ZipFile(args.output) as archive:
        if archive.testzip(): raise RuntimeError("ZIP integrity verification failed")
    print(f"{args.output}: {args.output.stat().st_size:,} bytes; launcher + {len(blobs)} data files + empty state/")

if __name__=="__main__": main()
