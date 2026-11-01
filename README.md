# nNova — Nova Drift for TI-Nspire

[![Build and verify](https://github.com/maxmixalex/n-nova-drift/actions/workflows/build.yml/badge.svg)](https://github.com/maxmixalex/n-nova-drift/actions/workflows/build.yml)

A native C++ reimplementation of **Nova Drift** for the 320×240 color TI-Nspire,
using an offline **converter + launcher** pipeline. The calculator runs a small
ARM926 Ndless executable; a standalone desktop converter reads your own PC game
installation and creates the separate content packs.

**Development status:** this is an early playable port, not a claim of complete
equivalence to the PC release. The supplied PC build uses GameMaker YYC and has
no GML bytecode in `data.win`. Game rules are reimplemented in native C++; numeric
balance, enemy patterns, rendering and supported modes must be checked against
the [coverage report](docs/coverage.md). Original asset extraction and native
gameplay coverage are different things.

## Install a converted package

Requires a color TI-Nspire CX / CX II with a compatible Ndless installation
(r2022 or later). The hardware backend is verified in a CX II CAS emulator;
physical calculator performance is not inferred from desktop frame rates.

Keep this layout together in a calculator documents folder:

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
  state/
```

Open **nNova.tns**. The launcher creates `state/` when needed. Settings, the best
score and the current run are stored there; content packs remain read-only.
Use **Save and quit** from the pause menu before closing the application.

See [CONTROLS.md](CONTROLS.md) for keyboard and touchpad controls.

## Convert your own installation

Download the standalone Windows converter from the workflow artifacts, put
`nova-converter.exe` beside your own Nova Drift `data.win`, and run:

```bat
nova-converter.exe
```

No Python installation, GameMaker runtime, network access or execution of
`NovaDrift.exe` is needed for conversion. The default output is `nNova/data/`.
Copy the separately built `nNova.tns` launcher into `nNova/`.

To choose a directory and validate every converted frame:

```bat
nova-converter.exe "C:\Games\Nova Drift" --output "C:\nNova\data" --verify
```

On Linux, use the same arguments with `./nova-converter`. The supported source
format is the GameMaker build represented by the provided game files: DXT5 DDS
texture pages and the corresponding SPRT/TPAG metadata. Unsupported layouts
fail with an error instead of silently creating empty sprites.

The default converter preserves all animation frames, crops transparent margins,
shares identical frames, and chooses raw or lossless run-length storage per
frame. Pixel conversion to the native RGB565 display and category-specific
resizing are the visual adaptations; run-length compression adds no further
loss. `--compact-animation` is an explicit opt-in and is not used for the
default delivery. See [docs/architecture.md](docs/architecture.md).

## Build

Desktop preview and converter, with a C++17 compiler:

```sh
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Release
cmake --build build/host --parallel
```

SDL2 development headers enable the interactive desktop preview. A headless
preview is available without SDL2:

```sh
build/host/nova-drift --data /path/to/nNova/data --headless --demo \
  --frames 1200 --screenshot build/game.ppm
```

The ARM launcher uses Clang 18, LLD 18 and Python 3 **at build time** for the
strict ELF-to-Zehn packager. There is no Python dependency on the calculator or
in the converter executable:

```sh
python3 tools/build_ndless.py
```

The result is `dist/nNova.tns`. GitHub Actions builds the real launcher, the
Linux preview/converter, and a static Windows converter. CI never contains or
downloads the commercial game assets. Generated packs and local game files are
excluded from version control.

## Source layout

| Location | Purpose |
| --- | --- |
| `src/game.*`, `src/content.*` | Fixed-step, allocation-free simulation and content rules |
| `src/app.*`, `src/render.*`, `src/draw.*` | Menus, save flow, game rendering and RGB565 drawing |
| `src/assets.*`, `src/font.*`, `src/localization.*` | Bounded binary-pack readers |
| `src/platform.*`, `platform/ndless/` | Desktop and real Ndless platform backends |
| `tools/converter/` | Standalone C++17 GameMaker asset converter |
| `tools/build_ndless.py`, `tools/zehn.py` | ARM compilation and checked Zehn packaging |
| `tests/` | Simulation, pack-format and launcher regression tests |

## Credits and rights

Nova Drift is the work of Chimeric / Pixeljam. This is an unofficial fan port.
The repository contains the port's source and conversion tools, not the
commercial game's textures, fonts, localization files, executable or audio.
Create content packs from a copy of the game you own; generated packs retain
the rights of their respective original materials.

New port code is MIT-licensed. Third-party components retain their original
licenses; see [THIRD_PARTY.md](THIRD_PARTY.md).
