# nNova 0.2.0 — Nova Drift for TI-Nspire

[![Build and verify](https://github.com/maxmixalex/n-nova-drift/actions/workflows/build.yml/badge.svg)](https://github.com/maxmixalex/n-nova-drift/actions/workflows/build.yml)

A native C++ reimplementation of **Nova Drift** for the 320×240 color TI-Nspire,
using an offline **converter + launcher** pipeline. The calculator runs a small
ARM926 Ndless executable; a standalone desktop converter reads your own PC game
installation and creates the separate content packs.

**0.2.0 includes** the original menu structure, all 11 languages, calculator-first
controls, account progression, reconstructed native mechanics, and independent
enemy and boss components. The supplied PC **1.2.38.4** executable is used as a reference
for constants and control flow; its original UI also runs separately for direct
comparison. The [UI comparison](docs/ui-parity.md) and
[gameplay coverage](docs/coverage.md) distinguish implemented behavior from
remaining differences. A functional entry in a catalog is not proof of exact
PC behavior.

## 中文快速开始

将 ZIP 中 **`nNova/nNova.tns`** 和 **`nNova/data/` 下全部 `*.dat.tns`** 传到已安装兼容 Ndless 的彩色 TI-Nspire，保留 `nNova/data/` 的目录关系，然后打开 `nNova.tns`。`state/` 用于保存进度，由程序创建。说明文档、许可证和电脑用的 `converter/` 留在电脑即可；转换器可放在您自己的 PC 原游戏目录中运行，重新生成全部素材和语言，**不需要 Python**。

默认按计算器数字键操作：**8 推进、2 刹车、4/6 转向、5 开火、7 技能、9 升级**。菜单用 8/2 与 4/6 选择，5 或 Enter 确定，Esc 返回或暂停。**0 在数字键优先和触摸瞄准之间切换**；触摸瞄准时可同时按 8 推进、用触摸板瞄准并按下开火。设置 → 控制中可重绑定战斗按键。详情见 [CONTROLS.md](CONTROLS.md)。

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
    nNova.lat.font.dat.tns
    nNova.zhs.font.dat.tns
    nNova.eng.locale.dat.tns
    nNova.zhs.locale.dat.tns
    ...
  state/
```

Open **nNova.tns**. The launcher creates `state/` when needed. Settings, the best
score and the current run are stored there; content packs remain read-only.
Use **Save and Quit** in Pause to save and return to the main menu; use Exit
to close the application. The current run, account rank, ten scores, language,
physical key bindings, daily date selection and sixteen local daily scores have
separate checked persistent state. When updating an older package, replace the
entire `data/` directory so obsolete graphics chunks do not remain; preserve
your existing `state/` directory.

The default physical keys are **8** thrust, **2** brake, **4 / 6** turn,
**5** fire / confirm, **7** ability, **9** upgrades, and **Menu / Esc** pause.
**0** explicitly switches between keypad and touch aim. Hovering never changes
focus. In touch aim, **8** thrust and touch aim / press-to-fire work together;
held input cannot click through a newly opened page. Nine actions can be
rebound to physical calculator keys. See [CONTROLS.md](CONTROLS.md).

All 11 original languages are included by default. The first launch follows
the calculator's system language, with checked fallbacks. Font groups are
shared by Latin / Cyrillic languages, while Japanese, Korean, simplified
Chinese and traditional Chinese use their original game fonts.

## Convert your own installation

The release ZIP includes the standalone Windows converter at
`converter/converter.exe` and the Linux converter at `converter/converter`.
Copy `converter.exe` beside your own Nova Drift `data.win`, and run:

```bat
converter.exe
```

The standalone workflow artifact uses the name `nova-converter.exe`; use that
filename instead if you downloaded the CI artifact rather than the release ZIP.

No Python installation, GameMaker runtime, network access or execution of
`NovaDrift.exe` is needed for conversion. The default output is `nNova/data/`.
Copy the separately built `nNova.tns` launcher into `nNova/`.

To choose a directory and validate every converted frame:

```bat
converter.exe "C:\Games\Nova Drift" --output "C:\nNova\data" --verify
```

On Linux, use the ZIP's `./converter` with the same arguments (`./nova-converter`
for a standalone workflow artifact). The supported source
format is the supplied PC 1.2.38.4 layout: DXT5 DDS
texture pages and the corresponding SPRT/TPAG metadata. Unsupported layouts
fail with an error instead of silently creating empty sprites.

The default converter preserves all animation frames and all eight alpha
bits, crops transparent margins, and shares identical frames. Each image uses
the smallest exact raw, run-length, palette, or planar RGB565/alpha representation;
fonts and text
use a checked lossless wrapper decoded only when loading a language. Graphics
are stored in sprite / frame order to reduce calculator file seeks. Pixel
conversion to RGB565 and display-sized source resampling are the visual
adaptations. Thirteen large or thin boss component sprite groups have additional resolution so
restoring their original attachments does not enlarge a blurred small image. `--compact-animation` is an explicit opt-in and is not used for the
default delivery. The verified reference conversion contains **22 data files**,
**1,292 sprite records**, **2,590 animation frames**, five font groups and all
11 languages, totaling **7,579,988 bytes** (7.23 MiB). This is **1,670,211 bytes
smaller (18.06%)** than the preceding 0.2 candidate, with the same decoded frame
pixels, dimensions, timing and origins. Images decode into a bounded cache when
loaded; the frame loop does not decompress the whole pack. See
[docs/architecture.md](docs/architecture.md).

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

## Gameplay and verification scope

The catalog contains **13 weapons, 10 shields, 14 bodies and 180 mod keys**.
The enemy catalog has **65 identities**, including 11 boss controllers; 64
actor slots, 192 independent component slots and 448 projectile slots bound the
live simulation. The current pointer-free Game state is **52,792 bytes**.

Source reconstruction includes native prerequisites, career ranks, opening and
curated waves, weapon and charge formulas, independent destructible components,
boss phases, construct weapon inheritance and shield interactions. Dedicated
regressions test these behaviors. The item audit distinguishes implementations
awaiting clause-level verification from restored source formulas and known
platform adaptations; these categories are not a claim of complete PC parity.
Daily Challenge includes ten locally seeded modes, selectable calendar dates,
rules and per-date local best scores. Comets, cargo trains and black holes have
simulation and collision behavior. These local challenges do not reproduce
Steam's daily seed or online ranking. The built-in speakerless calculator target
does not output PC audio. Steam services and online score services are not
implemented. See
[coverage](docs/coverage.md) for exact budgets, evidence and remaining boundaries.

## Source layout

| Location | Purpose |
| --- | --- |
| `src/game.*`, `src/content.*`, `src/progression.inc` | Fixed-step simulation, source-derived rules and account progression |
| `src/boss_ai.inc`, `src/boss_parts.*` | Boss phases, independent components and arena hazards |
| `src/input_controller.*`, `src/input_bindings.*` | Calculator key / touch isolation and persistent rebinding |
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
