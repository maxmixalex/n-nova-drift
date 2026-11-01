# Targeted source audit follow-up (1.2.38.4)

This audit revisits the earlier 40 `partial_source_behavior` and 14
`source_coefficient_alignment` labels. Those labels describe an older tree;
they are not a current missing-feature count, and this review does not extend
its conclusion to the other 123 historically unaudited mod entries. Source routine
names and addresses below identify the supplied original Windows executable.
No original executable or recovered assembly is redistributed here.

## Defects corrected in this follow-up

| Area | Recovered source behavior | Current implementation and verification |
| --- | --- | --- |
| Rancor | `mRancor_Alarm_0` at `0x1407df542` subtracts `rancorMultThrust=.1` | Base thrust now loses 10%, instead of gaining 10%. Charging penalties from Charged Shot/Rancor/Charged Shields add before applying once (`refreshWeapon` at `0x1404d6a43..0x1404d6a85`), instead of multiplying three reductions; tests verify the stat and actual acceleration. |
| Hidden Power | The original description permits shield gear constructs | Bastion survives taking Hidden Power and remains deployable. Weapons and the other construct kinds remain disabled. Test exercises existing Bastion, existing drone, and rebuilding after removal. |
| Aegis | `damageReduction` at `0x14015ea65` adds `aegisIncrement`; at `0x14015eb47` subtracts .1 stillness per source frame while thrusting | Builds to 15% reduction over 2.5 s; decays over five 30 Hz ticks instead of immediately disappearing. Tests check 85%, 88%, 91%, then 100% incoming damage. |
| Adaptive Armor | `damageReduction` at `0x140160778..0x140160899` subtracts `1-.99^stacks` from damage reduction factors | Replaces the incorrect rational curve. A 51-byte lookup rounds to the existing whole-percent damage API, with no runtime floating point. Golden values include 10/20/30/40/50 stacks and stacking with Aegis. Independent stack expiry and the 50-stack limit remain. The original `maxDR=.25` cap is applied to the combined defensive result before encounter modifiers. |
| Omni Shield | `triggerShield` at `0x140746e36..0x1407470a2` adds `originalDamage*shieldCool*.03`, or `.018` with Omni; first hit starts full timer, additions cap at full | The source flat 15-point shield reduction now composes additively with Aegis and Armor. Ordinary shields are delayed by hits while empty as well as while absorbing damage. Tests cover initial hit, repeat hit, empty shield, original incoming damage, cap, Tempest exemption, and Bastion exemption. Timer increments round to the 30 Hz tick. |
| Blink | `playerBlink` at `0x14042f30b` creates a damaging burst; `spawnBurstGrowDamaging` at `0x14060f820` assigns damage-derived knockback | Both teleport endpoints now apply the existing integer burst impulse as well as damage. Tests hit and push two real enemies in opposite directions. The port retains its converted endpoint geometry, rather than claiming source-frame visual identity. |
| Daily completion score | `setup` at `0x14057975d` defines a 10^12 time numerator. `oStarEaterController_Step_0` at `0x140dd2174..0x140dd22e4` divides by elapsed active milliseconds and halves Turbo/Boss Rush. Egg scoring at `0x140dd1bc8..0x140dd1c51` is proportional to surviving hull. | Non-endless challenges now award the time bonus at Star Eater death; Egg awards up to one million in proportion to surviving HP. Golden checks include normal, Turbo, Boss Rush, endless exclusion, half-full and 1 HP eggs. Active simulation time is used in the offline port. |

`tests/mod_source_rules_test.cpp` exercises the first six corrected behaviors.
`tests/daily_test.cpp` exercises the completion rewards and the ten challenge
variants. These tests enter the real gameplay methods; they do not replace the
implementation with diagnostic placeholders.

## Historical gaps already implemented in the current tree

| Earlier unresolved area | Existing implementation evidence |
| --- | --- |
| Charged Shot curves, extra shots, native innate charging; Singular Strike fusion and flat firing period | `game.cpp::updatePlayer`, `fireWeapon`, `weaponPeriodQ16`; `vortex.inc`; `tests/native_rules_test.cpp` and `tests/inherited_weapons_test.cpp` |
| Homing Strike target selection, axes, seek and degeneration | `mod_source_rules.inc`; `tests/mod_source_rules_test.cpp` includes Shielded Eclipse target obstruction and original weighted targeting |
| Rapid Reconstruction refund/pool exclusion; Essence Sap; Hypermetabolism reservation | `takeMod`, `canTakeMod`, gear offers, `creditDamage`, regeneration and recovery-cap methods in `game.cpp`; gameplay tests |
| Discharge and Tempest bolts, fractional shield cost and chains | `electrical_shields.inc`; `tests/electrical_shields_test.cpp` |
| Grace Protocol non-projectile death handling; Fleet Commander promotion/replacement; Loaded Mines native weapon inheritance | `damageConstruct`, `updateConstructs`, `inherited_weapons.inc`; `tests/constructs_flow.cpp` and `tests/inherited_weapons_test.cpp` |
| Self Destruction and Burnout hull-based bursts; Last Stand death field; Bastion release field | `persistent_effects.inc`, `mod_source_rules.inc`; `tests/persistent_effects_test.cpp`, `tests/constructs_flow.cpp`, and `tests/mod_source_rules_test.cpp` |
| Ataraxia five-point body power and twenty-point Hyperboost; Dying Star missing-defense aura; Rancor charge accumulation/decay | `bodyPowerPercent`, `hasHyperboost`, `updatePlayer`, `vortex.inc`; native-rules/gameplay tests |
| Charged Shields ratios, cap and linger; Outrage threshold/direction/empower; Phantom Strike crash bonuses | `updatePlayer`, `damagePlayer`, `updateEnemies`; source constants and gameplay/native-rules coverage |
| Scorching Wake simultaneous overlap; Maelstrom damage-to-force curve; Spontaneous Generation HP-dependent spawn and decay; Challenger families/scales/rewards | `scorching_wake.inc`, `persistent_effects.inc`, `constructs.inc`, `mod_source_rules.inc`; matching focused test targets |
| Kinetic Boost Euclidean speed scaling; Shielded Constructs centers; Overseer and Support Specialist limits; Channeling; Purification; Efficiency | Concrete simulation paths in `game.cpp`, `constructs.inc` and `advanced_constructs.inc`; old report text no longer describes absent mechanics. This row does not claim exhaustive equivalence for every combined build. |
| Leaf on the Wind hit-reset ramp; Warp Strike penalty and wrap; Void Slice charges/path; Chaotic Ambition weighted offers | `globalDamagePermille`, `updatePlayer`, projectile update, and `generateOffers`; bounded native simulation rather than a script VM |

The twelve coefficients other than Omni Shield and Firing Array were checked
against their source constants in this pass: High Explosive, Concentrated
Blast, Velocity, Rapid Fire, Magnitude, Payload, Hull Strength, Shield
Durability, Barrier, Candescence, Core Shielding, and Farsight. Their already
implemented values matched. Firing Array has a separate targeted restoration
and test described below.

## Firing Array and original weapon spread

`shootSpecialCases3` at `0x1405d6c7f..0x1405d7a03` initializes the ordinary
projectile's angular weight to **.5**. For a distributed fan with a nonzero arc,
it derives the weight from the shot's angle relative to the center of that arc.
Damage and blast use `1 + lerp(0, .2, weight)`; physical size and the separate
`sizeMod` use `1 + lerp(-.3, .3, weight)`. The outermost damage multiplier is
therefore **100%**, rather than zero. The ordinary single-shot fallback is
**110% damage and 100% size**, rather than the former 120% and 130%.

The restored `firing_array.inc` applies the same interpolated size to collision
radius and `Bullet::size_percent`. The latter feeds Dart's attached lifetime and
burn damage. Vortex applies the physical size to its persistent field scale;
its native Step routine recomputes burn from the base damage and field size, so
the base damage is not multiplied again. Sword handling uses a different source
routine: `specialCasesSwords` is called by `spawnSwordsSparks` at `0x14068598d`.
Its no-fan weight begins at zero. Consequently the modifier affects the emitted
parry sparks and their spread, without inventing a bonus for the physical blade.

That check also found obsolete zero-spread entries for several weapons. They
made additional Dart, Vortex, Railgun and Grenade projectiles overlap and
could bypass the original Firing Array arc calculation. `weaponSpreadStep` now
uses the following values from `assignBaseStats`, preserving hundredths of a
degree through the existing modifier arithmetic before converting to the
engine's 256-angle representation:

| Weapon | Original base spread | Source assignment address | Unmodified native angle step |
| --- | ---: | --- | ---: |
| Blaster | 10 degrees | `0x14006cbe9` | 7 |
| Split Shot | 12 degrees | `0x14006cda8` | 9 |
| Flak | 1.45 degrees | `0x14006d50a` | 1 |
| Railgun | 12 degrees | `0x14006d1a6` | 9 |
| Grenade | 8 degrees | `0x14006d342` | 6 |
| Salvo | 12 degrees | `0x14006dbfd` | 9 |
| Vortex | 20 degrees | `0x14006d846` | 14 |
| Pulse sparks | 240 / (12 - 1) degrees | `0x14006de5f..0x14006de81` | 16 |
| Torrent | 24 degrees | `0x14006d6ef` | 17 |
| Blade | 12 degrees | `0x14006e06c` | 9 |
| Dart | 7 degrees | `0x14006cf67` | 5 |
| Thermal Lance sparks | 5 degrees | `0x14006da19` | 4 |
| Swords | 1 degree | `0x14006ca4d` | 1 |

`tests/firing_array_test.cpp` checks all thirteen converted base steps, ordinary
single-shot behavior, a real three-Dart fan with 70%/130%/70% sizes, and actual
attachment lifetimes of 42/78 ticks with matching burn scaling. It also checks
released Vortex field scales of 1.4/2.6/1.4 from a scale-2 three-shot fan, the
single-Vortex fallback, and Sword spark endpoints/center while preserving blade
damage. The focused `firing-array`, `advanced-constructs`, `inherited-weapons`
and `gameplay` CTest targets all pass after this change. `Game` remains 52,792
bytes; this change adds no serialized state.

The source values are preserved; emitted geometry is still resolved at the
port's integer angular and spatial precision. This check does not claim that
every original cannon or Barrage spawn arrangement is reproduced frame for frame.

## Limits of the claim

The engine runs a bounded fixed-step simulation at 30 Hz, with integer screen
coordinates and a whole-percent incoming-damage API. Source curves are
translated to those representations. Blink/Voidslice path geometry, persistent
sprite presentation, object capacity, and colliding multi-effect builds are
not claimed to be byte-for-byte equivalents of the desktop runtime.

The offline daily calendar is deterministic within this port; it does not
reproduce the publisher's online seed service or submit official rankings.
Cosmic encounters are real actors with collision/reward behavior, while their
port scheduling intervals, lifetime/geometry and reward distribution remain
adapted rather than completely decompiled source scheduling.

The complete item name/eligibility audit is separate from this mechanic audit.
An implemented item count must not be presented as evidence that every clause
and every interaction of all 180 source rows has been independently verified.
