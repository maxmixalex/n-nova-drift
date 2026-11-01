# Third-party source

`stb_truetype.h` is from the upstream **nothings/stb** repository, commit
`f0569113c93ad095470c54bf34a17b36646bbbb5`:

https://github.com/nothings/stb/blob/f0569113c93ad095470c54bf34a17b36646bbbb5/stb_truetype.h

The header offers a choice of MIT or public-domain licensing and contains both
complete license texts at its end. nNova uses the MIT option, also reproduced in
`STB-LICENSE.txt`. It is used only
by the desktop converter to rasterize fonts supplied by the game owner; it is
not linked into the TI-Nspire launcher.

The supplied Windows executable is built with LLVM-MinGW and statically links
its C++ support. `LLVM-LICENSE.txt` and `MINGW-RUNTIME-LICENSE.txt` reproduce the
runtime notices from the upstream 20261020 toolchain. It requires only Windows
system libraries (including the UCRT supplied with Windows 10 and 11), with no
separate C++ runtime DLL to install.

GameMaker resource layout was checked against the public UndertaleModTool
format implementation, without copying its implementation into this project:

- https://github.com/UnderminersTeam/UndertaleModTool/blob/master/UndertaleModLib/Models/UndertaleSprite.cs
- https://github.com/UnderminersTeam/UndertaleModTool/blob/master/UndertaleModLib/Models/UndertaleTexturePageItem.cs
- https://github.com/UnderminersTeam/UndertaleModTool/blob/master/UndertaleModLib/Models/UndertaleEmbeddedTexture.cs

The converter's FORM parser, DXT5 decoder, area resampler, pack format, and RLE
codec are implemented in this repository. Proprietary game resources are never
bundled with this source.
