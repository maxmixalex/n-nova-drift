"""The calculator allocation includes the Zehn prefix, not only ELF memory.

Adapted from nSpire, Copyright (c) 2026 Max Alex; MIT license.
See third_party/licenses/nSpire-MIT.txt.
"""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import importlib.util
import zlib
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import zehn
from zehn import ALLOCATION_LIMIT, EXECUTION_MEMORY_LIMIT, build, compress_image


def image(memory_end: int) -> bytes:
    """A four-byte ARM text image with a tail BSS of the requested extent."""
    names = b'\0.text\0.bss\0.shstrtab\0'
    names_offset = 56
    sections_offset = (names_offset + len(names) + 3) & ~3
    header = b'\x7fELF\x01\x01\x01' + bytes(9)
    header += struct.pack('<HH5I6H', 2, 40, 1, 0, 0, sections_offset, 0,
                          52, 0, 0, 40, 4, 3)
    payload = header + struct.pack('<I', 0xe12fff1e) + names
    payload += bytes(sections_offset - len(payload))
    sections = [
        (0,) * 10,
        (1, 1, 6, 0, 52, 4, 0, 0, 4, 0),
        (7, 8, 3, 8, 0, memory_end - 8, 0, 0, 8, 0),
        (12, 3, 0, 0, names_offset, len(names), 0, 0, 1, 0),
    ]
    return payload + b''.join(struct.pack('<10I', *row) for row in sections)


class ZehnAllocationTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.source = Path(self.directory.name) / 'input.elf'
        self.target = Path(self.directory.name) / 'output.tns'
        self.make(128)
        # Read the actual serialized allocation convention. This remains
        # valid if the flag list or metadata length changes later.
        self.prefix = struct.unpack_from('<I', self.target.read_bytes(), 24)[0] - 128
        self.assertGreater(self.prefix, 0)

    def make(self, memory_end):
        self.source.write_bytes(image(memory_end))
        with redirect_stdout(StringIO()):
            build(self.source, self.target)

    def test_total_allocation_at_limit_is_accepted(self):
        # Isolate the total-allocation guard. The ordinary production
        # contiguous-execution limit is deliberately stricter than this one.
        with patch.object(zehn, 'EXECUTION_MEMORY_LIMIT', ALLOCATION_LIMIT):
            self.make(ALLOCATION_LIMIT - self.prefix)
        self.assertEqual(struct.unpack_from('<I', self.target.read_bytes(), 24)[0], ALLOCATION_LIMIT)

    def test_prefix_pushes_valid_image_over_limit_without_replacing_output(self):
        previous = self.target.read_bytes()
        memory_end = ALLOCATION_LIMIT - self.prefix + 1
        self.assertLess(memory_end, ALLOCATION_LIMIT)
        with patch.object(zehn, 'EXECUTION_MEMORY_LIMIT', ALLOCATION_LIMIT):
            with self.assertRaisesRegex(ValueError, 'Zehn allocation exceeds 24 MiB'):
                self.make(memory_end)
        self.assertEqual(self.target.read_bytes(), previous)

    def test_contiguous_execution_limit_is_independently_enforced(self):
        self.make(EXECUTION_MEMORY_LIMIT)
        previous = self.target.read_bytes()
        self.assertEqual(struct.unpack_from('<I', previous, 24)[0], EXECUTION_MEMORY_LIMIT + self.prefix)
        with self.assertRaisesRegex(ValueError, 'contiguous-memory budget'):
            self.make(EXECUTION_MEMORY_LIMIT + 1)
        self.assertEqual(self.target.read_bytes(), previous)

    @unittest.skipUnless(importlib.util.find_spec('zopfli'), 'optional release encoder not installed')
    def test_release_compression_preserves_all_loaded_bytes_and_headers(self):
        original = self.target.read_bytes()
        with redirect_stdout(StringIO()):
            build(self.source, self.target, 'zopfli')
        packed = self.target.read_bytes()
        self.assertLessEqual(len(packed), len(original))
        a, b = list(struct.unpack_from('<8I', original)), list(struct.unpack_from('<8I', packed))
        a[2] = b[2] = 0  # Only stored file size can differ in the Zehn header.
        self.assertEqual(a, b)
        self.assertEqual(original[32:self.prefix], packed[32:self.prefix])
        self.assertEqual(zlib.decompress(original[self.prefix:]), zlib.decompress(packed[self.prefix:]))

    @unittest.skipUnless(importlib.util.find_spec('zopfli'), 'optional release encoder not installed')
    def test_invalid_encoded_payload_never_replaces_previous_launcher(self):
        previous = self.target.read_bytes()
        with patch('zopfli.zlib.compress', return_value=zlib.compress(b'')):
            with self.assertRaisesRegex(ValueError, 'loaded bytes'), redirect_stdout(StringIO()):
                build(self.source, self.target, 'zopfli')
        self.assertEqual(self.target.read_bytes(), previous)


class ZehnCompressionTest(unittest.TestCase):
    def test_development_encoding_preserves_existing_zlib_stream(self):
        raw = bytes(range(256)) * 31 + b'\x00' * 4096
        self.assertEqual(compress_image(raw), zlib.compress(raw, 9))
        with self.assertRaisesRegex(ValueError, 'Unknown Zehn compression'):
            compress_image(raw, 'unknown')

    @unittest.skipUnless(importlib.util.find_spec('zopfli'), 'optional release encoder not installed')
    def test_release_encoding_is_exact_and_never_larger(self):
        for raw in (b'', bytes(range(256)), bytes(range(256)) * 257, b'\0' * 70000,
                    (b'Native calculator instructions and read-only tables\0' * 137)):
            with self.subTest(bytes=len(raw)):
                encoded = compress_image(raw, 'zopfli')
                self.assertLessEqual(len(encoded), len(zlib.compress(raw, 9)))
                self.assertEqual(zlib.decompress(encoded), raw)


    @unittest.skipUnless(importlib.util.find_spec('zopfli'), 'optional release encoder not installed')
    def test_fixed_profiles_keep_earlier_winner_and_never_silently_use_bad_output(self):
        raw = b'ABC' * 10000
        fallback, best, larger = (zlib.compress(raw, level) for level in (1, 9, 0))
        self.assertLess(len(best), len(fallback))
        for candidates in ((best, larger), (larger, best)):
            with self.subTest(order=[len(c) for c in candidates]):
                with patch.object(zehn.zlib, 'compress', return_value=fallback), \
                     patch('zopfli.zlib.compress', side_effect=candidates) as encoder:
                    self.assertEqual(compress_image(raw, 'zopfli'), best)
                    self.assertEqual(encoder.call_count, 2)
                    self.assertEqual([call.kwargs for call in encoder.call_args_list], [
                        dict(numiterations=5, blocksplitting=True, blocksplittingmax=15),
                        dict(numiterations=20, blocksplitting=True, blocksplittingmax=64)])
        for invalid in (b'bad', best+b'trailing', best[:-1], zlib.compress(b'wrong')):
            with self.subTest(invalid=invalid[:8]):
                with patch('zopfli.zlib.compress', return_value=invalid):
                    # Force selection so the exact independent decoder runs.
                    with patch.object(zehn.zlib, 'compress', return_value=larger):
                        with self.assertRaisesRegex(ValueError, 'Release compressor'):
                            compress_image(raw, 'zopfli')


if __name__ == '__main__':
    unittest.main()
