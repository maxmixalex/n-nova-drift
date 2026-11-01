# nNova 0.2.0 release validation

Validation date: 2026-11-01. The supplied reference installation is PC Nova
Drift 1.2.38.4. The delivered launcher is a native ARM926 Ndless program.

## Deliverable and image checks

| Item | Verified result |
| --- | --- |
| Production ARM launcher | 375,376 bytes |
| Production launcher SHA-256 | `2538a2d626008c866aa4d2e57c9ba32f0468ad603ff2132e609ae054b4905dca` |
| Converted data | 22 files, 7,579,988 bytes |
| Launcher plus calculator data | 7,955,364 bytes (7.59 MiB), excluding later user saves |
| Sprite records / animation frames | 1,292 / 2,590 |
| Unique stored frames | 2,447 |
| Fonts / languages | Five font groups / eleven languages |
| Whole preceding candidate's data | 23 files, 9,250,199 bytes |
| Installed data reduction | 1,670,211 bytes, 18.06% |
| Image comparison | All 2,590 frames, 20,669,277 decoded pixel bytes equal between image formats |
| Animation / alpha preservation | No removed animation frames; unchanged dimensions, IDs, origins and frame rates; all eight alpha bits preserved |

The total reduction includes the current text, font and skin packs. The graphics
and index alone fall from 7,532,875 to 5,868,281 bytes, a 1,664,594-byte reduction.
The two performance runs use the **same current eighteen non-graphics packs**;
they do not compare different language or skin versions.

The final standalone Windows converter converted the complete supplied game
under Wine with `--verify`. Its twenty-two output files are byte-identical to
the Linux converter's files; native Windows CI separately checks the executable,
codecs and Unicode/default-directory handling. See the per-file
[Windows conversion evidence](evidence/windows-full-conversion.json).

The normal launcher is compiled with Clang/LLD 18 and the pinned build-time
Zopfli 0.4.3 encoder. Python is not required to run the launcher or either
standalone converter. The executable's Zehn allocation is 6,669,664 bytes;
this header allocation is not presented as a measurement of total OS-plus-game
peak RAM.

## Tests and resource coverage

- The final Release build completes with zero compiler warnings. **32/32 CTest
  checks pass**, including the converter, Unicode paths, packaging, gameplay,
  input, source waves, Daily and Firing Array checks.
- The actual v3 data passes the complete application-flow test.
- The language audit checks **14,586 lookups** with no missing keys, missing
  glyphs or unresolved numerical references.
- The registered catalogue checks all 282 enemy/gear/mod images. The expanded
  source-reference audit checks **522 unique runtime image references**,
  including Boss attachments, effects and cosmic objects, with no missing
  entries or invalid frame ranges.
- Eleven-language UI fixtures comprise 638 standard views plus separate ending
  captures. Key pages were visually inspected in all eleven languages.
- AddressSanitizer and UndefinedBehaviorSanitizer pass the latest gameplay,
  source-mod, Daily and Firing Array tests. LeakSanitizer's process-enumeration
  check is disabled in the restricted local container; address and undefined
  behavior checks remain enabled. CI runs the complete sanitizer suite.
- The native RGB565 blending change passes the old per-channel oracle, including
  approximately 34 million samples. A real menu capture is byte-identical to
  the old blend and the complete earlier drawing implementation; its SHA-256 is
  `af35b0f1dada132b8aac9818d150d8b6cacc3f8cb39997ae91ba083f3ee79ec0`.

The [host acceptance record](evidence/host-acceptance.json) also confirms that
source files did not change during the complete build and test run. The targeted
[source audit](mod-audit-followup.md) distinguishes recovered rules from earlier
historical audit labels.

## ARM performance comparison

These are **Firebird emulator measurements**, not measurements from a physical
calculator. The emulated ARM926 configuration reports product 448 and a
396,000,000 Hz CPU clock. Presentation is capped at approximately 30 FPS; the
33 ms frame interval produces a nominal 30.30 FPS ceiling.

Both runs use the same profiling executable, the same fixed starting Game
snapshots and the same non-graphics packs. Each scene is copied into its own
NAND image; `tools/prepare_firebird.py` rewrites the snapshot's embedded NAND
path, which otherwise overrides a command-line path. Every recorded startup
was checked for the intended isolated flash path. A fresh emulator/USB process
collects files after each native run.

Sampling starts after 3.64–3.71 seconds of active play, after resume transitions,
and holds calculator keys **8 + 5**. The ordinary checkpoint is an early game
with four asteroids. The other four are deliberately constructed stress scenes,
including a five-controller scene with 187 Boss components. They are not claimed
to be naturally completed playthroughs. Ordinary sampling lasts five seconds;
the other samples last fifteen seconds. Cold image accesses that occur during
those intervals remain included.

| Scene | Same runtime, v2 graphics FPS | Final v3 graphics FPS | v3 drawing median / 95th percentile / maximum |
| --- | ---: | ---: | ---: |
| Early ordinary play | 30.11 | 29.90 | 4 / 4 / 66 ms |
| Late build | 29.94 | 30.30 | 13 / 15 / 17 ms |
| 187-component stress scene | 30.18 | 30.30 | 31 / 32 / 33 ms |
| Persistent effects / constructs | 29.95 | 30.16 | 12 / 20 / 77 ms |
| Seraph | 30.03 | 30.30 | 5 / 9 / 10 ms |

The smaller graphics format causes no substantial frame-rate regression in
these samples. The steady cached drawing costs remain similar. This is not a
claim of a higher frame-rate ceiling or of guaranteed physical-hardware FPS.
The short ordinary sample is 0.20 FPS lower, while the other four are slightly
higher. The sampling does not establish that every small difference is caused
by compression; scene timing and first-file accesses also vary.

The 66 ms and 77 ms maxima show that occasional cold access pauses remain.
They are not removed from the averages or disguised as steady-state costs.
The source keeps a bounded 2 MiB decoded-image cache and does not decompress
whole packs every frame. The complete metrics and workload counts are retained
in [arm-timings.json](evidence/arm-timings.json), with the
[controlled input description](evidence/arm-ab-controls.json) and matching
[v2](evidence/arm-v2-sampling.json) / [v3](evidence/arm-v3-sampling.json)
launcher and starting-state hashes.

Earlier diagnostic runs that resumed a shared-NAND snapshot or included a
new-run confirmation/loading interval were discarded from this comparison.
No production-source change was made to conceal those measurement errors.

## Production package checks

The uninstrumented production executable and all twenty-two data files are
extracted from a candidate ZIP, checked against its SHA-256 manifest and used
for the calculator interaction test. The final archive is regenerated from the
clean published source commit; its playable payload must match that tested
candidate exactly. The shipped `state/` directory is empty.

All seven production scenarios passed. The compact
[production acceptance record](evidence/production-acceptance.json) contains
the twenty-three installed payload hashes, save checks and thirteen behavioral
assertions. Its launcher hash is the production hash listed above.

| Scenario | Verified behavior |
| --- | --- |
| New run and mixed controls | Holding confirm does not skip the mode page; a fresh numeric 8 input moves the ship while touch aim and fire remain active. Holding touch while pausing does not click a different pause item. |
| Natural save and continue | A normal gameplay save passes CRC and state validation; resuming advances simulation frame 191 to 251. |
| Upgrade navigation | Numeric 8/2 move selection without purchasing, rerolling or banishing; banked upgrades remain 2, rerolls 12, acquired mods 0 and banished mods 0. |
| Daily date and launch | The actual 2026-11-01 RTC date is displayed; selecting 2026-01-01 shows the corresponding rules and starts the Cosmic Egg challenge with 15,000 egg HP. |
| Daily continue | A separately restored save advances frame 75 to 133 while preserving the selected date, variant and egg HP. |
| Victory and ending | The complete approximately 38-second ending reaches the 2,670-point results screen automatically. A confirm key held across entry does not skip it; the victory save and career record pass CRC checks. |
| Crucible | The arena boundary is present in gameplay and the resulting save passes state validation. |

The review includes 67 actual emulator screenshots and sixteen complete native
execution/USB collection logs. Every process exits successfully and every final
USB queue is empty. Gameplay state is 52,792 bytes and its snapshot is 52,816
bytes. These tests use the uninstrumented executable; the performance table
above uses a separate, explicitly instrumented build.

## Boundaries

The implementation targets the calculator's controls, 320 × 240 RGB565 screen,
fixed update rate and finite object pools. Its daily dates and records are
local. It does not include PC audio output, Steam integration or the original
online leaderboard. Integer/percentage quantization and the documented native
simulation policies remain. Catalogue coverage and successful tests are not
presented as a proof of every original script clause or every possible mod
combination. See [gameplay coverage](coverage.md) and
[interface coverage](ui-parity.md) for those exact boundaries.
