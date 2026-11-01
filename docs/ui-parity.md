# Calculator interface and localization coverage

The native interface uses the original game's converted art, language tables,
item descriptions and numeric text references. Its layout is arranged for the
TI-Nspire's 320 × 240 screen: long PC panels become tabs or scrollable pages.
This is an implemented interface inventory, not a claim that a 1280 × 720 PC
screenshot is reproduced pixel for pixel.

## Screens and reference evidence

The reference capture names below identify the supplied original-game captures
used during review. Generated capture names are produced by
`nova-ui-snapshots`, once for each of the eleven languages.

| Screen | Implemented behavior | Original reference | Generated capture examples |
| --- | --- | --- | --- |
| Title and resume | Resume preview, new run, unlocks, learn, glossary, changelog, settings, scores, credits, Discord address and exit; existing-save confirmation | `original-return-menu.png`, `original-savequery.png`, `original-resume-state.png` | `01-title.ppm`, `31-resume-preview.ppm` |
| Challenge modes | Mode icons, lock state, dependent challenge flags, start and complete descriptions; offline daily date, rules and personal results | `original-loadout.png`; original `Unlocks*` and `Daily*` language entries | `02-loadout.ppm`, `32-daily-calendar.ppm`, `33-daily-rules.ppm`, `37-daily-best.ppm` |
| Upgrade | Gear and mod selection, original icons, short/full descriptions, tags, prerequisite tree, reroll, Research banish and banking; direction keys never purchase an offer | Original upgrade/gear artwork and language entries; `original-decline-gear.png` for the banked-upgrade return to play | `07-upgrade.ppm`, `08-description.ppm`, `08b-upgrade-tree.ppm`, `30-first-gear.ppm` |
| Pause and build | Resume, settings, super mods, save-and-quit, current equipment/mod list; eight statistics tabs with additional pages for long tables | `original-pause.png`, `original-current.png` | `09-pause.ppm`, `10-build.ppm`, `11-statistics.ppm`, `11a-*` through `11g-*`, `27-supermods.ppm` |
| Settings and controls | Six categories; live gameplay/display options, auto-deployment by construct type, autosave behavior, optional converted skins, language selection, key capture/conflict handling, reset scores/progress | `original-settings.png`, `original-controls-settings.png` | `03-settings.ppm`, `04-languages.ppm`, `05-controls.ppm`, `20-*` through `25b-*`, `35-controls-menu.ppm` |
| Learn and glossary | Twenty learn pages, scrollable glossary entries, full descriptions and an enemy reference view | `original-learn.png`; original learn/glossary language entries | `14-learn.ppm`, `15-glossary.ppm`, `15b-glossary-description.ppm`, `17-bestiary.ppm`, `17b-bestiary-description.ppm`, `28-learn-gear.ppm` |
| Unlocks and progress | Equipment/mod/mode catalogue, prerequisite-aware lock state, rank progress and reward descriptions | `original-unlocks.png` | `16-unlocks.ppm`, `26-progress.ppm` |
| Results and scores | Retry, statistics, main menu, exit; ten ordinary local scores and career aggregates; separate best results for sixteen recent daily dates | `original-scores.png`; original results/statistics language entries | `12-results.ppm`, `13-scores.ppm`, `13b-record.ppm`, `13c-career-records.ppm`, `36-daily-results.ppm`, `37-daily-best.ppm` |
| Credits and changelog | Source credit order, scrollable complete text and source changelog | `original-credits.png`, supplied `changelog.txt` | `18-credits.ppm`, `19-changelog.ppm`, `29-credits-page2.ppm` |
| Ending | Recovered sequence timing, original ending/speed-line/mask assets, hold-to-skip and transition to results; held fire cannot skip or retry through page boundaries | Supplied `SEQN` resource and ending-controller code references | `eng-ending-*.ppm`, `zhs-ending-*.ppm` from the `ending` fixture |

The snapshot states are synthetic review fixtures. Their equipment, scores,
unlocks and victory states are not evidence of a naturally completed playthrough.
The ending application test also checks the 37,984 ms automatic completion and
750 ms fresh-confirm skip boundary.

## Input behavior

The default bindings use the calculator numeric area: **8** thrust, **2** brake,
**4 / 6** turn, **5** fire/confirm, **7** ability or manual deployment, **9**
upgrades, and **0** an explicit keypad/touch-aim switch. **Esc**, **Menu** and
**Doc** pause combat. The minus and Delete keys perform reroll and banish.
Complete instructions and rebinding behavior are in [CONTROLS.md](../CONTROLS.md).

The touchpad report and physical key matrix are decoded separately. In keypad
mode, a pressed touchpad sector is latched until release. Numeric directions
take precedence on the axis they control; the other pad axis and numeric fire
remain usable together. In menus, a suppressed pad direction/center requires a
fresh press, preventing a delayed opposite move or confirmation when a numeric
key is released. Touch aiming never silently changes the selected input mode.
Page changes and mode changes consume held controls until their release.

The title shows the current input mode. Control/help hints follow saved custom
bindings, including the details, progress, statistics and mod-tree actions.

## Localization verification

The release data was checked in English, German, Spanish, French, Japanese,
Korean, Brazilian Portuguese, Russian, Polish, Simplified Chinese and Traditional
Chinese. The audit covered **14,586 interface/content lookups**, including all
daily variants and modifiers, with **zero missing keys or glyphs and zero
unresolved numeric references**. It also resolved **282 registered runtime
sprite references**: 65 enemies, 37 gear items and 180 mod offers, with zero
missing or unreadable entries. These are the registered catalogue entries;
the separate [full sprite audit](evidence/sprite-reference-audit.json) covers
522 unique runtime names, also including effects, components and the ending.
The two counts describe overlapping coverage and must not be added together.

The native snapshot tool produces 58 views per language, 638 standard review
images in total, plus the separate ending series. New daily date/rule/record,
control and audio pages were reviewed across all eleven languages. Font coverage
is checked against the actual converted font packs, not merely against the CSV
text. The source changelog remains English in every language, as supplied by
the original game; every selected font can render it.

The 180-title catalogue includes the original intentionally empty The Void and
the unavailable source placeholders Havoc and Onslaught. Neither placeholder
appears in the original playable wild-mod pools; their source descriptions and
object routines define no effect. They remain unavailable rather than acquiring
invented abilities. See [the source evidence](content-audit-sources.md).

## Platform differences

- The PC layout, mouse hints, display-resolution options and large scrolling
  panels are adapted to a fixed calculator screen and numeric navigation.
- TI-Nspire has no built-in speaker. This version has no audio playback backend;
  its audio page states that it runs without sound. It does not expose volume
  controls that pretend to affect playback.
- Daily challenges use a repeatable **offline** date rotation and local records.
  They do not reproduce the PC service's random schedule or upload to its online
  leaderboard. A plausible configured RTC supplies the initial date; after a
  reset, the user can select a date or reuse the last saved date. Elapsed runtime
  seconds are never presented as today's calendar date.
- Account, score and unlock state are local. Online account services, Steam
  achievements and remote leaderboard submission are not implemented. The
  Discord item displays its address for use on another device.
- If the required data/font files are absent, the recovery screen uses the
  built-in ASCII font so the exit/install instructions remain readable.
- The ending uses original sampled motion and sprite masks with native color
  effects; it does not execute the original PC shader or soundtrack.

## Reproducing the checks

Use a dedicated build directory when other contributors are building:

```sh
cmake -S . -B build/ui-proof -DNOVA_ENABLE_SDL=OFF -DBUILD_TESTING=ON
cmake --build build/ui-proof --target nova-input-tests app-flow-tests nova-localization-audit nova-ui-snapshots -j4
build/ui-proof/nova-input-tests
build/ui-proof/app-flow-tests /path/to/converted/data
build/ui-proof/nova-localization-audit /path/to/converted/data
build/ui-proof/nova-ui-snapshots /path/to/converted/data /tmp/nnova-ui-review
build/ui-proof/nova-ui-snapshots /path/to/converted/data /tmp/nnova-ending-review ending
```

The input test covers physical matrix positions, simultaneous numeric/pad
directions, menu confirmation ownership, mode/page release barriers, rebinding,
checksum recovery and all Gregorian dates from 2020 through 2099. Application
tests exercise the daily entry, rule view, deterministic retry, independent
records and backup recovery as well as ordinary save/resume and upgrade flows.

For emulator fixtures, compile against the same source revision as the launcher:

```sh
c++ -std=c++17 -O2 -UNDEBUG -I src tests/platform_qa_fixture.cpp src/game.cpp src/content.cpp src/source_waves.cpp -o /tmp/nnova-platform-qa
/tmp/nnova-platform-qa victory /tmp/nNova.sav.tns /tmp/nNova.cfg.tns
```

Other fixture modes are `heavy`, `effects`, `seraph`, `crucible`, `upgrade` and
`daily-menu`. The two output
files go into the emulator's test installation `state/` folder. These fixtures
have deliberate artificial state and must not be included as a player's saved
game in the release ZIP. The `victory` fixture sets up the final boss's next-hit
death path; loading it and advancing one simulation tick enters the ending.
The `upgrade` fixture resumes in combat with two banked points, the real initial
gear sequence completed, Research equipped and twelve rerolls. Press **9** to
open its generated seven-offer mod screen. **8 / 2** only change selection;
**5** spends a point. The `daily-menu` fixture writes an intentionally invalidated
run so the launcher opens the title with Start selected; press **5**, press
**2** twelve times, then **5** to enter the date/rules page.
