# Runtime and conversion architecture

## Hardware boundary

The runtime is freestanding C++ for ARM926EJ-S with the Ndless r2022 syscall ABI.
It owns a 320×240 RGB565 framebuffer and hands the completed buffer to Ndless's
LCD blitter. The platform backend provides monotonic time, key transitions,
absolute touchpad coordinates, bounded file access and temporary/backup save
replacement. Desktop code uses the same game, UI, renderer and pack readers.

No GameMaker runner, GML interpreter, Python, floating-point simulation or
general-purpose allocator runs on the calculator. Fixed arrays bound entities
and image memory. A grid narrows projectile/enemy collision candidates. The
simulation advances at 30 fixed ticks per second, with a bounded catch-up loop;
that is a simulation target, not a statement that every physical device renders
30 frames per second.

## Content pipeline

The C++ converter reads FORM chunks, sprite metadata, texture-page references,
DXT5 DDS pages, UTF-8 localization CSVs and the source installation's font files.
It does not run the PC executable. Bounds, offsets, dimensions and supported
formats are checked before reads and writes.

Different classes of artwork have different display budgets: ships, ordinary
enemies, bosses, projectiles, upgrade icons, menu planets and full-screen
backgrounds do not share a single texture-page downscale factor. Every frame
retains the original pivot, with the offset adjusted after transparent trimming.
The scaled pre-trim canvas is also recorded, so animated reveals do not change
scale as their visible bounds grow.

Frames are converted to straight RGB565 + alpha8. Identical converted frames
share one payload. An individual frame uses raw pixels or a lossless RLE stream,
whichever is smaller. Packs default to approximately 2 MiB shards, with an
independent small index. The default retains every source animation frame.

The runtime resolves sprite names once, looks up frames by index, and expands
pixels only when a frame is first required. A fixed 2 MiB arena bounds the
image cache. Filling it discards the old cache safely; the scene draws a returned
sprite immediately and never retains pointers across a potential cache flush.
There is no mandatory per-frame decompression of the active scene.

Font and localization files are separate from graphics. English and Simplified
Chinese fonts are rasterized from the supplied fonts at the display's UI size.
Localization files preserve source keys; native game descriptions reflect the
port's implemented rules instead of showing unresolved PC parameter macros.

## Game-state persistence

`state/nNova.cfg.tns` stores versioned settings and score records with a CRC.
`state/nNova.sav.tns` stores a versioned, size-checked snapshot of the pointer-free
game state with a CRC and semantic validation. Changing the core layout safely
rejects incompatible saves rather than reading fields at the wrong offsets.
Snapshots reset transient input and timing state on resume.

Writes first go to a temporary sibling, preserving an older generation until
the new file can replace it. An invalidated full-size snapshot records death so
a failed size check cannot accidentally revive the previous backup. Data files
are opened read-only and never rewritten by gameplay.

## Public and private outputs

The public repository and public CI artifacts contain code, conversion tools and
the launcher. The generated playable package is assembled separately from the
provided game files. It includes the real `.tns` launcher and verified data packs,
not a renamed desktop executable or a launcher requiring an online download.

Coverage of image extraction does not establish coverage of game logic. Native
mechanics and outstanding differences are tracked separately in `coverage.md`.
