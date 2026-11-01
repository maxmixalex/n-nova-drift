"""Reject incomplete or stale calculator transfers, including shared fonts."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("nnova_package",ROOT/"tools/package.py")
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)

def make_data(root):
    pixels=b"\xff\xff\xff"
    chunk=struct.pack("<8s6I",b"NNOVAP1\0",2,0,35,zlib.crc32(pixels),0,0)+pixels
    (root/"nNova.gfx00.dat.tns").write_bytes(chunk)
    sprite=struct.pack("<I4H64s",0,1,1,1,0,b"sTest")
    frame=struct.pack("<IIIHHHhhHHHHH",32,3,3,0,1,1,0,0,0,1,1,0,0)
    body=sprite+frame
    header=struct.pack("<8s10I2H3I",b"NNOVAC1\0",2,1,1,1,64,140,172,0,0,zlib.crc32(body),320,240,0,0,0)
    (root/"nNova.dat.tns").write_bytes(header+body)
    (root/"nNova.eng.locale.dat.tns").write_bytes(b"test")
    (root/"nNova.lat.font.dat.tns").write_bytes(b"test")
    for name in package.FONT_LICENSES:(root/name).write_text("Test license fixture\n")

class PackageTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix="nnova-package-")
        self.root=Path(self.temp.name);make_data(self.root)
    def tearDown(self): self.temp.cleanup()
    def test_shared_latin_font_and_all_notices(self):
        (self.root/"nNova.deu.locale.dat.tns").write_bytes(b"test")
        blobs,licenses,meta=package.validate_data(self.root)
        self.assertEqual(meta["languages"],["eng","deu"])
        self.assertEqual(meta["asset_version"],2)
        self.assertEqual(len(licenses),5)
        self.assertIn("nNova.lat.font.dat.tns",[p.name for p in blobs])
        (self.root/"NotoSansKR-Medium_OFL.txt").unlink()
        with self.assertRaisesRegex(ValueError,"Missing font license"): package.validate_data(self.root)
    def test_locale_needs_its_script_font(self):
        (self.root/"nNova.jpn.locale.dat.tns").write_bytes(b"test")
        with self.assertRaisesRegex(ValueError,"Missing font for language: jpn"): package.validate_data(self.root)
        (self.root/"nNova.jpn.font.dat.tns").write_bytes(b"test")
        package.validate_data(self.root)
    def test_stale_missing_and_corrupt_graphics(self):
        extra=self.root/"nNova.gfx04.dat.tns";extra.write_bytes(b"stale")
        with self.assertRaisesRegex(ValueError,"Missing or stale graphics"): package.validate_data(self.root)
        extra.unlink()
        graphics=self.root/"nNova.gfx00.dat.tns";raw=graphics.read_bytes();graphics.unlink()
        with self.assertRaisesRegex(ValueError,"Missing or stale graphics"): package.validate_data(self.root)
        graphics.write_bytes(raw[:-1]+b"\0")
        with self.assertRaisesRegex(ValueError,"CRC"): package.validate_data(self.root)

if __name__=="__main__": unittest.main()
