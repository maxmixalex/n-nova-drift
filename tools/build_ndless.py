#!/usr/bin/env python3
"""Compile the real ARM926 launcher with Clang/LLD and package a Zehn .tns.

This target never compiles the host backend. No proprietary SDK, ROM, or binary
toolchain download is needed. Ndless r2022 and an appropriate installed OS are
required on the calculator. The ABI is in platform/ndless/entry.S.
"""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
from zehn import build

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "dist" / "nNova.tns")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build" / "ndless")
    parser.add_argument("--compression", choices=("zlib", "zopfli"), default="zlib")
    args = parser.parse_args()
    compiler = os.environ.get("NOVA_CLANG") or shutil.which("clang++") or shutil.which("clang++-18")
    linker = os.environ.get("NOVA_LLD") or shutil.which("ld.lld") or shutil.which("ld.lld-18")
    if not compiler or not linker:
        parser.error("Install clang and lld (LLVM 18 or newer), or set NOVA_CLANG and NOVA_LLD.")
    directory = args.build_dir.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    sources = sorted((ROOT / "src").glob("*.cpp")) + [ROOT / "platform/ndless/freestanding.cpp", ROOT / "platform/ndless/entry.S"]
    flags = ["--target=arm-none-eabi", "-mcpu=arm926ej-s", "-marm", "-mfloat-abi=soft", "-mno-unaligned-access",
             "-O2", "-ffreestanding", "-fno-builtin", "-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics",
             "-fno-stack-protector", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-ffunction-sections",
             "-fdata-sections", "-fstack-usage", "-Wframe-larger-than=8192", "-Wall", "-Wextra",
             "-DNOVA_NDLESS=1", "-I" + str(ROOT / "src"), "-I" + str(ROOT / "include")]
    objects = []
    for source in sources:
        output = directory / (source.relative_to(ROOT).as_posix().replace("/", "_") + ".o")
        command = [compiler, *flags]
        if source.suffix == ".cpp":
            command += ["-std=c++17"]
        subprocess.run([*command, "-c", str(source), "-o", str(output)], check=True)
        objects.append(str(output))
    elf = directory / "nNova.elf"
    subprocess.run([linker, "-T", str(ROOT / "platform/ndless/link.ld"), "--gc-sections", "--icf=safe",
                    "--emit-relocs", "--no-undefined", "-Map=" + str(directory / "nNova.map"), *objects, "-o", str(elf)], check=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    build(elf, args.output, args.compression)

if __name__ == "__main__":
    main()
