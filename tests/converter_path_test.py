#!/usr/bin/env python3
"""Exercise real converter path handling with an original, synthetic FORM file.

No Nova Drift data or fonts are used. The fixture converts one solid-red 4x4
sprite, then intentionally fails at the missing localization.csv. Creating the
index and graphics pack proves that both the input and output paths were used.
"""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import os


def fixture():
    data = bytearray(b"FORM\0\0\0\0")

    def chunk(name, payload):
        start = len(data) + 8
        data.extend(name.encode("ascii") + struct.pack("<I", len(payload)))
        data.extend(payload)
        return start

    def u32(offset, value):
        struct.pack_into("<I", data, offset, value)

    # A normal GameMaker v3 bitmap sprite with one texture-page reference.
    speed = struct.unpack("<I", struct.pack("<f", 30.0))[0]
    sprite = [0, 4, 4, 0, 3, 3, 0, 0, 0, 0, 0, 0, 2, 2,
              0xffffffff, 3, 0, speed, 0, 0, 0, 1, 0]
    sprt = chunk("SPRT", struct.pack("<2I23I", 1, 0, *sprite))
    u32(sprt + 4, sprt + 8)
    item = struct.pack("<10Hh", 0, 0, 4, 4, 0, 0, 4, 4, 4, 4, 0)
    tpag = chunk("TPAG", struct.pack("<2I", 1, 0) + item)
    u32(tpag + 4, tpag + 8)
    u32(sprt + 8 + 88, tpag + 8)
    name = b"synthetic_path_test"
    strg = chunk("STRG", struct.pack("<3I", 1, 0, len(name)) + name + b"\0")
    u32(strg + 4, strg + 8)
    u32(sprt + 8, strg + 12)

    # One unambiguous DXT5 block: opaque solid red, authored for this test.
    dds = bytearray(128)
    dds[:4] = b"DDS "
    struct.pack_into("<6I", dds, 4, 124, 0x81007, 4, 4, 16, 0)
    struct.pack_into("<2I", dds, 76, 32, 4)
    dds[84:88] = b"DXT5"
    struct.pack_into("<I", dds, 108, 0x1000)
    dds.extend(bytes((255, 255)) + bytes(6) + struct.pack("<HHI", 0xf800, 0xf800, 0))
    txtr = chunk("TXTR", struct.pack("<9I", 1, 0, 0, 0, len(dds), 4, 4, 0, 0) + dds)
    u32(txtr + 4, txtr + 8)
    u32(txtr + 8 + 24, txtr + 36)
    u32(4, len(data) - 8)
    return data


def check_run(executable, arguments, cwd, output):
    result = subprocess.run([str(executable), *map(str, arguments)], cwd=cwd,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            encoding="utf-8", errors="strict", timeout=30)
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    assert "Converting 1 sprites, 1 frames" in result.stdout, result.stdout + result.stderr
    assert "localization.csv" in result.stderr, result.stderr
    index = (output / "nNova.dat.tns").read_bytes()
    assert index[:8] == b"NNOVAC1\0"
    assert struct.unpack_from("<3I", index, 8) == (1, 1, 1)
    assert (output / "nNova.gfx00.dat.tns").is_file()
    verified = subprocess.run([str(executable), "--verify-pack", str(output)], cwd=cwd,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              encoding="utf-8", errors="strict", timeout=30)
    assert verified.returncode == 0, verified.stdout + verified.stderr
    assert "Verified 1 sprites, 1 frames, 1 graphics packs" in verified.stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--converter", type=Path, required=True)
    executable = parser.parse_args().converter.resolve(strict=True)
    if os.name == "nt":
        import ctypes
        print("Windows active code page:", ctypes.windll.kernel32.GetACP())
    with tempfile.TemporaryDirectory(prefix="nnova-path-test-") as temporary:
        root = Path(temporary)
        source = root / "游戏_日本語_🚀"
        output = root / "输出_é_📦"
        cwd = root / "different working directory"
        tool_dir = root / "工具_路径_🌍"
        for path in (source, cwd, tool_dir):
            path.mkdir()
        (source / "data.win").write_bytes(fixture())
        (cwd / "data.win").write_bytes(b"BAD!0000")
        check_run(executable, [source, "--output", output], cwd, output)
        print("PASS: explicit Unicode source/output override the unrelated CWD")

        # Invoke by absolute path from elsewhere: executable-directory fallback.
        adjacent = tool_dir / executable.name
        shutil.copy2(executable, adjacent)
        (tool_dir / "data.win").write_bytes(fixture())
        (cwd / "data.win").unlink()
        check_run(adjacent, [], cwd, tool_dir / "nNova" / "data")
        print("PASS: no-argument discovery beside a Unicode executable path")

        # An actual CWD game must take priority over the executable's directory.
        (tool_dir / "data.win").write_bytes(b"BAD!0000")
        (cwd / "data.win").write_bytes(fixture())
        check_run(adjacent, [], cwd, cwd / "nNova" / "data")
        print("PASS: CWD data.win takes priority over executable-directory data")
    print("Converter Unicode path regression passed; all fixtures were synthetic.")


if __name__ == "__main__":
    main()
