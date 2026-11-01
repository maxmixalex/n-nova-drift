# Gameplay coverage and original-game evidence

This document describes the native 0.2.0 implementation. The supplied PC
**1.2.38.4** installation is the reference for game resources, language text,
numeric constants and recovered control flow. The calculator runs its own C++
simulation; it does not run the PC executable or GameMaker bytecode.

## What is implemented

A run includes initial equipment choices, steering and firing, ordinary and boss
waves, experience and levelling, seven-offer upgrades, prerequisites, rerolls,
Research banish, banked upgrades, constructs, player and enemy status effects,
account progression, save/resume, defeat, the final encounter, ending and results.
The interface uses the original artwork and descriptions in all eleven supplied
languages. The separate [interface inventory](ui-parity.md) lists the screens,
reference captures and calculator input behavior.

The content catalogue contains **13 weapons, 10 shields, 14 bodies, 180 mod
keys and 65 enemy identities**. Eleven identities have dedicated boss controllers.
A catalogue count is not a claim that every possible interaction has been
verified against the PC executable. The audit currently distinguishes 177
implemented mod behaviors, one blank original entry and two unavailable source
placeholder entries. The executable audit also checks that the 180 source keys
are unique and correspond to the original language catalogue when it is supplied.

| Area | Implemented behavior and evidence | Main implementation / checks |
| --- | --- | --- |
| Equipment and upgrades | Native weapon, shield and body selection; source prerequisites and exclusion rules; normal, wild and super mods; recursive prerequisites, rerolls, Research banish and gear-decline flow | `content.cpp`, `progression.inc`, `mod_source_rules.inc`; content, game and application tests |
| Weapons and persistent damage | Charge and projectile formulas, homing, bursts, blast and burn interactions, Vortex, electrical bolts, Scorching Wake and inherited construct weapons | `source_values.inc`, `vortex.inc`, `electrical_bolts.inc`, `scorching_wake.inc`, `inherited_weapons.inc`; native-rule and feature tests |
| Shields and constructs | Electrical and damaging shields, independent drones/mines/turrets/allies, deployment and rebuilding, interception, weapon inheritance, advanced construct interactions and individual health | `electrical_shields.inc`, `constructs.inc`, `advanced_constructs.inc`; construct, shield and inherited-weapon tests |
| Enemy controllers | Separate movement, fire, summons, destruction effects and projectiles for the recovered enemy families | `enemy_ai*.inc`, `enemy_projectiles*.inc`, `enemy_death*.inc`; flight, hazards, minor-enemy and family tests |
| Bosses and components | Eleven boss controllers; independent destructible attachments, phase/state changes, shields, beams, explosions, summons and arena interactions | `boss_ai.inc`, `boss_parts.inc`, `enemy_parts*.inc`; boss-flow, component, Myrmidon, Eclipse, Celestial, Snapper, Constructor and Dweller tests |
| Waves | Source opening and curated schedules, enemy eligibility/weights, dynamic quantity distribution and deferred spawns when the fixed actor pool is full | `source_waves.cpp`, `waves.inc`; original native golden values and source-wave tests |
| Cosmic objects | Comets cause impact damage; cargo train cars have individual collision, health and drops; black holes apply attraction and interact with pickups | `daily.inc` and the cosmic-object state; daily tests and actual rendered objects |
| Career and persistence | Source rank/reward progression, unlock gating, ten ordinary scores, current-run snapshot, separate settings and physical bindings, checksums and backup recovery | `progression.inc`, `app.cpp`, `input_bindings.cpp`; input and application tests |
| Ending | Original sampled motion, original sprites and masks, timed ending, held-key isolation, fresh-confirm skip and results transition | `ending_samples.inc`, `app_ending.inc`; application ending tests and eleven-language captures |

The enemy implementation is divided into small family files so an implementation
can be compared with a recovered source branch without mixing unrelated bosses.
The rendering composites use their original component sprite groups and
attachments. They are not substitutes for independent collision and damage
state: those are simulated separately.

## Daily Challenge

All **ten daily variants** and **23 modifiers** have a local implementation.
The calendar accepts a selected date, displays the full applicable rules and
keeps best results for sixteen recent dates. Repeating a date repeats this port's
seed and challenge configuration. A usable calculator RTC supplies the initial
date; otherwise the saved/manual date remains available.

Daily modes include the 20/45-upgrade Draft starts, the seven-mod Purity limit,
Cosmic Egg defence, sword and escort variants, the eleven-boss gauntlet and the
other original daily rule sets. The egg has the recovered 15,000 health, its
failure path and the recovered victory reward. Draft begins at the corresponding
wave 40/200. Brothers in Arms shares damage, applies the recovered survivor
hull/shield reduction after an escort dies and uses the recovered integrated
weapon delay modifier. Sword includes its weapon restrictions, invulnerability,
attack-speed and attraction changes. Turbo uses the source odd/even wave advance
rule in addition to its speed, experience and score modifiers. Cosmic Ballet
uses the more frequent curated-wave schedule, including its Turbo interaction.

The local date rotation is deliberately described as **offline**. It does not
claim to reproduce the PC service's date-to-challenge schedule or upload to its
leaderboard. The gauntlet uses a fixed local boss order. Cosmic-event scheduling
and geometry are adapted to the native arena and object budgets, rather than
being advertised as a replay of the PC's random-number stream.

## Runtime and display budgets

| Item | Native implementation |
| --- | --- |
| Simulation and presentation target | Fixed 30 Hz simulation with a 30 FPS presentation limit |
| Coordinates and heading | Integer Q8 positions and 256 heading units |
| Catch-up | At most six simulation steps per advance call; input elapsed time clamped to 200 ms |
| Display | 320 × 240 RGB565 framebuffer |
| Ordinary logical arena | 320 × 240; Far Sight expands the logical world and projects it onto the display |
| Enemy actors | 64 simultaneous slots |
| Independent enemy/boss components | 192 slots |
| Projectiles | 448 slots: 192 friendly and 256 hostile |
| Pickups | 72 slots; experience combines on saturation |
| Constructs | 16 slots |
| Cosmic objects | 24 slots |
| Short-lived visual effects | 32 slots |
| Leviathan / Serpentine segments | At most 24 |
| Game snapshot | 52,792 bytes in the current native layout; core state contains no owning heap pointers |
| Decoded image cache | 2 MiB, bounded; compressed images decode on load/cache miss |

These are intentional limits for the calculator. Source attacks are scheduled
and retained where their budgets allow; component and projectile pools are not
unbounded. Deferred wave spawns avoid silently losing a scheduled enemy when
all actor slots are occupied. A bounded simulation can differ from a PC scene
that has more simultaneous objects.

Asteroid rendering uses the appropriate Small, Medium, Large and Mega original
sprite groups. Nearly transparent glow margins no longer determine the apparent
rock size. The visible solid body is scaled around its collision diameter and
dark rock pixels receive a restrained contrast boost. This makes small rocks
legible without replacing their original silhouette with a new symbol.

## Image preservation and performance evidence

The standard conversion retains all **2,590 animation frames**, their IDs,
origins and frame rates, all eight source alpha bits and the same selected
source dimensions. RGB565 and source resampling for the target display are the
image adaptations. The thirteen large/thin boss sprite groups retain additional
source resolution. Animation-frame reduction is an explicit converter option
and is not enabled in the delivered package.

The current lossless pack revision reduces the preceding candidate's data from
9,250,199 to **7,579,988 bytes**. Every decoded frame was compared between the two
revisions; the **20,669,277 decoded bytes** are equal. Planar compression is
chosen only when it saves both at least 32 bytes and at least five percent. The
new representation trades some cold decoding work for less flash I/O; it is not
claimed to improve frame rate merely because the files are smaller. See
[architecture](architecture.md) and the release validation record for the actual
ARM measurements.

The RGB565 blend optimization was checked against the previous per-channel
implementation, including alpha samples and a real original-menu background.
The original and optimized menu captures have identical bytes. This check caught
and removed a temporary red-to-blue bit contamination before release.

## Platform differences and verification limits

- PC audio playback, Steam account/achievement integration and remote score
  submission are absent. The calculator audio page explains the silent target.
- Layout, controls, shaders, particle density, world scaling and finite object
  pools are adapted for the calculator. The ending uses native effects and the
  original sampled assets, rather than executing the PC shader.
- Restored source constants and control flow have dedicated regressions. Other
  functional native mechanics remain labelled adaptations where clause-by-clause
  equivalence has not been established. Neither a catalogue count nor a test
  pass certifies every combination of mods, constructs and enemies.
- Emulator measurements describe the stated emulated calculator and test
  workload. They are not measurements from a physical CX or CX II. Deliberately
  loaded stress fixtures are labelled synthetic; they are not represented as
  naturally completed playthroughs.
- Settings and supported state versions are checked and migrated where defined.
  A run snapshot with an incompatible native structure is rejected safely;
  binary snapshots from every earlier development build are not interchangeable.

## Reproducing the checks

```sh
cmake -S . -B build/verify -DCMAKE_BUILD_TYPE=Release
cmake --build build/verify --parallel
ctest --test-dir build/verify --output-on-failure
build/verify/app-flow-tests /path/to/nNova/data
build/verify/nova-localization-audit /path/to/nNova/data
build/verify/nova-converter --self-test
```

The public CI builds and tests the source without the commercial game files.
Additional release validation uses the supplied installation to verify all
converted frames, real font glyphs, interface sprite references, application
flows and the ARM launcher. The source contains the converter and recovery
helpers, not the supplied commercial executable, textures or language tables.
