# Third-party notices

This repository contains the original adaptation code and a resource converter.
The commercial Nova Drift executable, GameMaker data, graphics, fonts, and
localization files are not included in the public repository. The converter
reads an owner's local installation. Converted game assets retain their
original rights; the repository's software license does not relicense them.

## Ndless interface and executable format

The calculator launcher targets the public Ndless r2022 system-call ABI and the
Zehn executable format. The implementation was checked against the official
[Ndless source](https://github.com/ndless-nspire/Ndless) at commit
`e52415fb568e92cd36ed049067b3e9c2727b84af`, especially
`ndless-sdk/include/syscall-list.h`, `keys.h`, `zehn.h`, and the public LCD and
touchpad interfaces. No proprietary calculator OS, ROM, or TI library is
included in this repository or the launcher. Ndless itself is installed
separately; its upstream source and license notices remain with that project.

The ARM EABI helpers, memory primitives, and strict ELF-to-Zehn packaging tools
are adapted from the nSpire project, copyright (c) 2026 Max Alex, under the MIT
license. The complete notice is in
[third_party/licenses/nSpire-MIT.txt](third_party/licenses/nSpire-MIT.txt).
The applicable source files carry attribution comments.

## Converter and Windows runtime

The converter uses Sean Barrett's `stb_truetype.h` for offline rasterization of
the owner's original fonts. It is taken from
[nothings/stb](https://github.com/nothings/stb) commit
`f0569113c93ad095470c54bf34a17b36646bbbb5` and used under its MIT option.
The complete notice is
[STB-LICENSE.txt](tools/converter/third_party/STB-LICENSE.txt).
The header is not compiled into the calculator launcher.

The Windows x64 executable is built with
[LLVM-MinGW 20261020](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261020)
(LLVM 23.1.4) and statically links its C++ runtime. Binary distributions must
include [LLVM-LICENSE.txt](tools/converter/third_party/LLVM-LICENSE.txt) and
[MINGW-RUNTIME-LICENSE.txt](tools/converter/third_party/MINGW-RUNTIME-LICENSE.txt),
along with the stb notice above. Additional details are recorded in the
[converter third-party notice](tools/converter/third_party/NOTICE.md).

## Converted fonts

The converter preserves the original installation's font license files beside
the derived bitmap font data. A complete private resource package includes
`data/Roboto_LICENSE.txt` and `data/NotoSansSC-Medium_OFL.txt`. The packaging
script rejects a data directory that is missing either notice.

## Desktop preview and emulator

The optional desktop preview links the system's
[SDL2](https://github.com/libsdl-org/SDL/tree/SDL2) library, distributed under
the zlib license. SDL2 is not part of the calculator launcher.

The hardware validation process uses an independently supplied
[Firebird emulator](https://github.com/nspire-emus/firebird), licensed under
GPL-3.0, and the owner's calculator images. Neither the emulator nor the
images are distributed by this repository. `tools/prepare_firebird.py` only
prepares an isolated copy of a matching user-supplied NAND/snapshot pair; it
does not change OS code, application code, or emulated RAM.

