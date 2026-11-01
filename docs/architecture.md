# Converter, launcher and resource budgets

## Execution model

The calculator runs a native ARM926 Ndless launcher. The standalone C++17
converter reads the user's Nova Drift installation and exports display-sized
graphics, localization and fonts. Python is used for developer builds and
verification; neither distributed converter nor the calculator requires it.

The supplied reference is PC 1.2.38.4. Its data.win contains sprite metadata and
DXT5 texture pages. Its Windows executable is a native YYC build. Recovered
constants and control flow are implemented in the calculator simulation; the
launcher does not execute the desktop executable. See [coverage.md](coverage.md).

## Files on the calculator

| File or directory | Contents |
| --- | --- |
| `nNova.tns` | ARM launcher |
| `data/nNova.dat.tns` | Shared index and sprite/frame metadata |
| `data/nNova.gfxNN.dat.tns` | Graphics payloads in sprite/frame order |
| `data/nNova.*.font.dat.tns` | Five shared font groups |
| `data/nNova.*.locale.dat.tns` | Eleven independently loaded languages |
| `data/nNova.skins.dat.tns` | Converted cosmetic skin definitions |
| `data/nNova.changelog.dat.tns` | Original scrollable changelog |
| `state/` | Settings, bindings, account progression, scores, current run and local daily records |

The reference format-v3 conversion has **22 data files and 7,579,988 bytes**:
1,292 sprite records, 2,590 animation frames, 2,447 distinct stored frames,
three graphics packs, five font groups and eleven languages. Metadata and
graphics total 5,868,281 bytes. Original font-license notices remain in the
computer deliverable and do not need to be copied to the calculator.

The preceding v2 reference used 23 files and 9,250,199 bytes. This update saves
1,670,211 installed bytes. Regenerating into an existing converter output removes
obsolete nNova.gfxNN.dat.tns fragments. When copying manually, replace the whole
data directory so obsolete fragments do not consume calculator space. Preserve
state separately.

## Image quality and storage

The display adaptation uses RGB565 plus a full eight-bit alpha channel. Source
images are resampled with an area filter in premultiplied-alpha space. Only fully
transparent outside margins are removed. Origins, frame order, frame rate and
original canvas bounds remain in the metadata. Large or thin boss components
have individual resolution rules. All animation frames are preserved by default.

Every frame selects an exact representation: raw pixels, whole-pixel run length,
an exact palette, or separate RGB565 and alpha run-length streams. Identical
frames share one payload. Format v3 introduces separate color/alpha streams;
it is selected only when it saves at least 32 bytes and 5% against the previous
best representation. The format change does not alter decoded pixels.

The complete v2/v3 comparison checks all 2,590 frames and 20,669,277 decoded
pixel bytes, dimensions, origins, frame identifiers and animation rates. The
images are byte-identical after decoding. The new launcher still reads older
supported packs; older launchers cannot read the new v3 representation.

## Runtime cost

Graphics are decoded on a cache miss into a bounded 2 MiB pixel cache. Cache
hits draw directly from decoded pixels. Adjacent animation frames are stored
together to reduce seeks. Fonts and text use a lossless envelope expanded when
a language is loaded, outside the per-frame drawing loop.

Separate color/alpha streams reduce flash reads but require more CPU work to
decode. A desktop memory-only benchmark cannot establish calculator load time.
The [release validation](validation-0.2.0.md) compares both formats on the same ARM launcher and
scenes, including cold-cache frames. Cached drawing runs neither image decoder.

The simulation uses a fixed update step and bounded arrays for enemies,
projectiles, independent boss components and constructs. Enemy recipes retain
pending work when the enemy pool is full, instead of discarding the rest of a
large recipe. Rendering uses integer arithmetic and RGB565. Measurements
separate simulation, rendering and presentation; a 30 FPS cap can hide useful
reductions in rendering time.

## Reproducible builds

GitHub Actions builds the real Ndless launcher, desktop preview, static Linux
converter and Windows converter. It runs gameplay, input and format regressions,
Unicode path checks and a sanitizer build. LLVM-MinGW is pinned by release and
SHA-256. The build-time Zopfli encoder is version-pinned and produces a smaller
compatible zlib stream for the launcher; it is not part of the runtime.

CI uses source code and synthetic fixtures. Converted game data is produced
separately from the supplied installation. Packaging records the source commit,
file sizes and SHA-256 digests and creates an empty state directory.
