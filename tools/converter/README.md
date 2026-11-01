# nNova resource converter

The converter turns a personally owned Nova Drift installation into the small
resource packs used by the TI-Nspire launcher. It is a native C++17 command-line
program. Python, the GameMaker runner, and an Internet connection are not needed
to run it. The original game files are read without being changed.

## Run

Put `converter.exe` next to `NovaDrift.exe` and `data.win`, open a terminal in that
folder, and run:

```console
converter.exe
```

The supplied Windows binary targets 64-bit Windows 10/11. The Linux binary is
invoked as `./converter`. Both create `nNova/data` inside the
game folder. When launched from another working directory, it automatically
checks for `data.win` beside the executable after checking the current directory.
An explicit game path always takes precedence. Copy that data directory beside the separately supplied `nNova.tns`
launcher. On the calculator the resulting layout is:

```text
nNova/
  nNova.tns
  data/
    nNova.dat.tns
    nNova.gfx00.dat.tns
    nNova.gfx01.dat.tns
    ...
    nNova.lat.font.dat.tns
    nNova.jpn.font.dat.tns
    nNova.kor.font.dat.tns
    nNova.zhs.font.dat.tns
    nNova.zht.font.dat.tns
    nNova.eng.locale.dat.tns
    nNova.zhs.locale.dat.tns
    ...
    nNova.changelog.dat.tns
```

An explicit source or destination can be given. Quote paths containing spaces:

```console
converter.exe "C:\Games\Nova Drift" --output "C:\Transfer\nNova\data" --verify
./converter "/games/Nova Drift" --output ./transfer/nNova/data --verify
```

`--verify` checks the compressed pixel streams while converting and validates
the completed packs. `--verify-pack DIRECTORY` independently checks an existing
data directory, including its CRCs, frame boundaries, fonts, and language packs.
`--self-test` runs small synthetic codec tests without copyrighted game files.

The default output includes all 11 supported languages: English, German,
Spanish, French, Japanese, Korean, Portuguese, Russian, Polish, Simplified
Chinese, and Traditional Chinese. The launcher initially selects the calculator's
system language and lets you change it in Settings. Seven Latin/Cyrillic
languages share one font; Japanese, Korean, and both Chinese variants have their
own fonts. Language names remain readable before you select a language.

For a smaller installation, explicitly select the languages you need:

```console
converter.exe --languages eng,zhs --verify
```

`--all-locales` is retained as an alias for the default selection. To update text
and fonts in an existing data directory without converting the texture pages
again, use `--localization-only --output EXISTING_DATA_DIRECTORY`. The shared original English changelog is exported when `changelog.txt` is present.
The converter reads the source game's bundled fonts and copies their licenses.

All original animation frames are retained by default. The optional
`--compact-animation 12` samples long animations down to at most 12 frames;
that explicitly trades animation fidelity for space. It is unnecessary for the
supplied 320×240 profile and is not used for the standard package.

## What is converted

The supported source build uses a GameMaker `FORM` file with regular bitmap
sprites and embedded DDS DXT5/BC3 texture pages. Every referenced sprite frame
is decoded from the original texture. Unsupported sprite/image types fail with
a clear error instead of being silently replaced.

Sprites are resampled individually for their use on a 320×240 display. Ships,
enemies, bosses, projectiles, upgrade icons, menu planets, and the title have
separate size policies. Area filtering uses premultiplied alpha to avoid dark
halos. The output keeps full 8-bit transparency and RGB565 color, then trims only
fully transparent borders while preserving the original pivot and scaled canvas
dimensions. Cropping therefore does not make animation frames change scale.

Adaptive raw/RLE, exact-palette and separate-plane RLE encoding are lossless after this intentional
resolution/color conversion. The encoder compares raw pixels, repeated-pixel
runs, exact 4/8-bit RGBA palettes, and exact 4/8-bit RGB palettes with unchanged
8-bit alpha. It also tries separate RGB565 and alpha8 RLE streams, which exploit
flat color and opacity independently without discarding either. It selects a
candidate only when every original pixel is represented exactly and the payload
is smaller. Separate-plane RLE additionally needs to save at least 32 bytes and
5%, retaining the earlier codecs where a second plane traversal would buy very
little space. There is no color quantization,
alpha reduction, frame removal, or additional resizing in this compression step.

Identical frames share stored data. Graphics are split into packs of at most
2 MiB and arranged in sprite/frame order, so an animation's consecutive frames
can be read with fewer long file seeks. The launcher expands a frame only on a
cache miss and uses a bounded 2 MiB pixel cache; cached drawing does not decode
images. Version 2 packs introduced the exact palette codecs. Version 3 adds the
two RLE planes, decoded directly into the existing color/alpha cache without an
intermediate image buffer or additional persistent memory. The launcher still
accepts version 1 and version 2 graphics packs. New version 3 data needs the
launcher supplied with this release.

For the supplied source build, separate-plane RLE reduces the 2,447 unique
image payloads from 7,351,611 to 5,687,049 bytes (22.64%). All 2,590 animation
frames, 20,669,277 decoded pixel bytes, dimensions, origins, frame IDs and sprite
animation rates are checked for exact equality by the image-codec audit. This
is a storage and cache-miss read reduction; it does not claim that steady-state
frame rendering becomes 22.64% faster.

The bundled Roboto Condensed, SmartFont UI, Noto Sans KR, Noto Sans SC, and
Taipei Sans TC fonts are rasterized at a 14-pixel em size with 16-pixel line
spacing. Fonts contain the characters needed by the selected source strings,
native UI labels, language names, and the changelog. Missing visible glyphs fail
conversion instead of silently producing blank text. Where a primary font lacks
a character, another of the supplied original fonts provides that glyph.

Selected language files preserve the original CSV keys and UTF-8 values, add
translated calculator UI labels, and use English for empty source translations.
The complete original credits are also extracted from `data.win`, preserving
the native draw order, original names, and localized section headings. The current
credits layout is verified against source version 1.2.38.4 with a checksum; a
different layout fails explicitly rather than assigning the wrong names to roles.
The converter does not require or execute `NovaDrift.exe` for this extraction.

A bounded lossless LZ envelope compresses fonts, dictionaries, and the changelog.
The launcher decodes each file only when opening it or changing language, checks
its CRC, and then reads the decoded data directly. It also accepts the previous
uncompressed font and locale formats. Source description formatting and recovered
numeric parameters are interpreted by the launcher; asset conversion itself does
not implement game rules. Music and sound are not emitted because this calculator
target has no corresponding built-in game audio output.

`conversion-report.txt` records the input size/CRC, counts, output sizes and
conversion choices. `sprites.csv` lists the resource names, animation rates and
size categories. Font license files accompany the generated font packs.

This is an asset converter. It does not emulate the PC executable, decompile
YYC code, or claim that converting assets reproduces every PC gameplay rule.

## Build

From the repository root, GCC or Clang can build the converter directly:

```sh
g++ -O3 -std=c++17 -Iinclude tools/converter/main.cpp -o converter
```

With a MinGW-w64 cross compiler, a self-contained Windows executable is:

```sh
x86_64-w64-mingw32-g++ -O3 -std=c++17 -static -static-libgcc \
  -static-libstdc++ -Iinclude tools/converter/main.cpp -o converter.exe
```

LLVM-MinGW is also supported:

```sh
x86_64-w64-mingw32-clang++ -O3 -std=c++17 -static -Iinclude \
  tools/converter/main.cpp -o converter.exe
```

For MSVC, use `/std:c++17 /O2 /EHsc /utf-8 /Iinclude` in place of the GCC flags.
The repository CMake target is `nova-converter`.

The bundled third-party sources are `stb_truetype.h` for original fonts and
the PNG-only build of `stb_image.h` for custom skins. They run on the computer
when converting assets and are available under MIT or public-domain terms; see
`third_party/NOTICE.md`. No proprietary game images, fonts, language files, or
generated data packs belong in the public repository.


### Original custom skins

When the game directory includes `custom_skins/custom_skins.csv`, the converter also reads its original PNG body, force and optional drone images and emits `nNova.skins.dat.tns`. The runtime uses this optional pack for the original Custom Skins setting. PNG decoding is compiled into the C++ executable; no Python or external image program is required. Images use premultiplied-alpha area resampling with a 72-pixel maximum side and the same exact RGB565/alpha image codecs as the main pack. Original canvas/origin metadata, bounds, decoded CRCs and whole-pack CRC are retained and checked. Skin names participate in font glyph collection. Omitting the source skin directory produces a valid game pack without the optional skin file.

`--localization-only` refreshes language/font packs, the optional skin pack and the language/font statistics in `conversion-report.txt`, while retaining the existing graphics/index data. `--verify-pack` checks all present skin images as well as ordinary sprites, fonts and dictionaries.
