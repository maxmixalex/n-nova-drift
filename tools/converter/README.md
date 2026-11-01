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
game folder. Copy that data directory beside the separately supplied `nNova.tns`
launcher. On the calculator the resulting layout is:

```text
nNova/
  nNova.tns
  data/
    nNova.dat.tns
    nNova.gfx00.dat.tns
    nNova.gfx01.dat.tns
    ...
    nNova.eng.font.dat.tns
    nNova.zhs.font.dat.tns
    nNova.eng.locale.dat.tns
    nNova.zhs.locale.dat.tns
    ...
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

Adaptive raw/RLE encoding is lossless after this intentional resolution/color
conversion. Identical frames share stored data. Graphics are split into packs
of at most 2 MiB. The launcher expands a frame only on a cache miss and uses a
bounded 2 MiB pixel cache, so normal draws do not perform decompression.

The original Roboto Condensed and Noto Sans SC fonts are rasterized at 12 pixels
for English and Simplified Chinese. The language files preserve all original
CSV keys and UTF-8 values. Original GameMaker formatting tags and variable
placeholders remain in full descriptions; the launcher decides how to display
the subset it uses. Music and sound are omitted because the calculator target
does not provide the game's audio output.

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

The only bundled third-party source is `stb_truetype.h`, used on the computer
when converting fonts. It is available under MIT or public-domain terms; see
`third_party/NOTICE.md`. No proprietary game images, fonts, language files, or
generated data packs belong in the public repository.
