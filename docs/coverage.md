# Gameplay coverage and original-game evidence

This is a native, fixed-budget TI-Nspire adaptation of Nova Drift using assets and mechanical descriptions from the supplied PC package. **It is not the PC executable running on the calculator, a recovered GameMaker VM, or a claim of complete original-game parity.** The scope below describes the implemented native gameplay, including its concrete limits.

## Evidence from the supplied package

The supplied `Nova Drift.zip` has 51 entries and 852,255,215 bytes of uncompressed content. The main `data.win` is 435,498,132 bytes. `options.ini` identifies an `[LLVM-WINDOWS]` build. The GameMaker archive contains sprite, object, script-name, string, texture and audio metadata, but **no `CODE`, `VARI` or `FUNC` bytecode chunks**. This is consistent with a YYC native Windows build; there is no GML instruction stream in the archive for this runtime to execute.

`localization.csv` exposes 37 gear titles (13 weapons, 14 bodies, 10 shields) and exactly 180 mod titles. `prefabPlayerBuilds.csv` and its snippets expose original gear/mod identifiers and build combinations. Most numerical descriptions contain unresolved engine-variable placeholders. Consequently the calculator's damage, cooldown, health, movement, radius, scaling, dependency and repetition limits are explicitly authored port values. They have not been recovered from the PC executable.

The sprite exporter identifies 1,293 valid original sprite records. Content names and sprite references in the native tables are checked against those records. Composite bosses use selected original component sprites; their assembly, full animation rigs and original phase scripts are not recreated.

## Implemented native loop

The native game has thrust/inertia/braking, keyboard or direct cursor aiming, screen wrapping, firing and active mobility. Enemies and bosses attack, projectiles use swept collision checks, shields and hull take damage, kills produce collectible experience and repair/reroll drops, levels open gear/mod choices, and death supports restarting. Upgrades can be banked and reopened, and banked points affect Ataraxia. Gear is introduced through weapon, shield and body choices at levels 2, 3 and 4; later choices are generated from eligible mods, with periodic replacement gear.

All 37 gear choices have distinct implemented behavior. Examples include Salvo missile stockpiles, charged/self-damaging Vortex, a sustained Thermal Lance, returning Blade constructs, projectile-blocking Swords, Spectre cloaking, Firefly exhaust damage, Carrier swarms, growing Leviathan segments, directional Bastion protection, reflected projectiles, Halo burning, Warp breaks, Temporal slowing, Orbitals, Siphon recovery and Shockwave recharges. These are adaptations of their main identities, not assertions that every PC sub-rule, interaction, statistic or visual is reproduced.

The mod catalog contains every original title: **177 adapted mechanics, one intentionally empty THE VOID offer, and two unavailable source placeholders**. Every active nonempty entry has a concrete implementation branch or changes values used by gameplay. `Havoc` and `Onslaught` are excluded from selection because the supplied full descriptions contain only `[$=*]...`. The 177 count measures functional native adaptations; it does not mean 177 exact reproductions of all original clauses. The per-entry table below gives this port's actual behavior.

The optional Wild mode permits recursive Wild Mods with explicit finite rank limits (usually 5, and 3 for Far Sight, Grandeur and Challenger). Revelation, Obsession, Explosive Growth, banishment and Transmogrification affect subsequent selections. Super Mods use authored prerequisite combinations. Hidden Power disables and removes ordinary constructs and suppresses weapon fire. The baseline mode excludes Wild entries.

There are **26 native enemy profiles**, including **10 bosses**: Scion, Station Omega, Dreadnought, Ceratotitan, Dweller, Glaucus, Warbringer, Seraph, Crucible and Star Eater. Other profiles include asteroids, Swarmlings, Station Drones, Vanguard, Spitfire, Interceptors, Bulwark, Hammerhead, Sine, Pulsebug, Warden, Beamcaster, Cherub, Juggernaut, Minelayer and Tracer. These profiles provide different attacks, movement or support behavior. They are not an exhaustive port of every original enemy object/rarity/formation. Boss patterns, phase transitions, wave composition and cadence are independently authored approximations. Star Eater is present as a boss profile; the original ending sequence is not recreated.

## Runtime budgets

| Item | Native implementation |
| --- | --- |
| Simulation | Fixed 30 Hz, integer Q8 coordinates, 256 discrete heading units |
| Catch-up | At most 6 simulation steps per advance call; elapsed input clamped to 200 ms |
| Ordinary arena | 320 × 240 logical units |
| Far Sight arena | 384 × 288, 448 × 336, or 512 × 384; renderer projects to the physical display |
| Enemies | 64 simultaneous fixed slots |
| Projectiles | 320 fixed slots: 192 friendly and 128 hostile |
| Hostile saturation | Replaces the oldest hostile projectile so new attacks still appear |
| Friendly saturation | New friendly projectiles are omitted and counted diagnostically |
| Pickups | 72 slots; experience merges when saturated |
| Constructs | 16 simultaneous slots |
| Effects | 32 short-lived visual slots |
| Leviathan | At most 24 visual/collision segments |
| Simulation memory | 18,488-byte Game snapshot in the current build, no core heap allocations |

Friendly and hostile projectile budgets are separate: a volley-heavy player cannot consume the hostile reserve. Capacity ceilings can still change density compared with the PC game. Spatial bins narrow collision candidates; damage over time and aura work are spread over ticks. There is no floating point, STL, dynamic allocation, or required global constructor in the simulation.

## Explicit remaining differences and omissions

- There is no original YYC runtime reconstruction or execution of the Windows executable. Native formulas, upgrade prerequisites, weapon timing, enemy AI, boss phases, formation generation, difficulty and progression are calculator-specific approximations.
- The original full modifier algebra, all edge-case synergies, exact targeting/construct assembly rules, every enemy part/shield/rarity, and authored campaign waves are not reproduced. Some effects are intentionally represented by a simpler equivalent, such as repair pickups instead of separate original Nanotech/Armor Fragment power-up classes, and Research's defense bonus on spent rerolls in place of its original dedicated banishment UI.
- All original gear is available to the native selection system. The original account unlock progression, achievement tracking, challenge/daily modes, full bestiary/glossary, Steam integration, cloud saves and online leaderboards are not recreated.
- Visuals reuse downscaled original assets, with a compact native HUD/menu and native particles. Composite bosses, shaders, postprocessing, original animation timing, cinematic sequences and full PC GUI layouts are not reproduced.
- The core does not reproduce PC music/audio streaming, mixer behavior or sound-effect playback.
- Save files hold a version/size/checksummed native snapshot. Binary compatibility across future changes to the Game structure is not promised.
- Desktop sanitizer and deterministic simulation results do not certify original-game equivalence or calculator performance. Real ARM/emulator performance measurements are recorded separately by the platform validation.

## Verification

`tests/game_test.cpp` checks deterministic fixed-step replay, bounded catch-up, pause/resume, gear offer progression, upgrade eligibility, all active mod prerequisite paths, weapon-specific damage for all 13 weapons, body/shield setup, reflection, Warp, Halo, Salvo stockpiles, Vortex charge/self-damage/release, construct limits, Architect exclusivity, Carrier swarms, Hidden Power removal, rail penetration, capacity behavior, and death/restart. It also checks upgrade banking/Ataraxia, Far Sight bounds, Last Stand, recursive Wild stacking, banishment, Power Spike, and representative late builds. Cursor aiming across opposite screen quarters is explicitly tested.

The current test includes two matching 24,000-tick evolving runs; the final fixture recorded 44 upgrades, 6 deaths and 429 kills with matching deterministic checksums. It also tests a completely full friendly projectile pool while a boss fires, and a saturated hostile pool being replaced by a new volley. AddressSanitizer and UndefinedBehaviorSanitizer pass with leak detection disabled for the tool environment. `-Wall -Wextra -Wpedantic -Werror` passes.

`tests/content_audit.py --source /path/to/localization.csv` checks exact coverage of all 180 original mod keys and validates the two source placeholders. Without the optional original CSV it still checks table count/uniqueness and emits the coverage classification. `tests/late_build_fixture.cpp` generates an explicitly labeled synthetic QA save for renderer/performance testing; it is not a natural playthrough or gameplay-completion claim.

## Per-mod native behavior

All “adapted” rows mean a functional native implementation with the behavior shown here, not PC-exact parity.

| Original source key | Native title | Class | Coverage | Actual native behavior |
| --- | --- | --- | --- | --- |
| `ModTitleAddedProjectiles` | VOLLEY | normal | adapted | +2 shots; -15% shot damage and speed. |
| `ModTitleFocusFire` | FOCUS FIRE | normal | adapted | Halves projectile spread. |
| `ModTitleFiringArray` | FIRING ARRAY | normal | adapted | Central projectiles +50% damage; wider firing array. |
| `ModTitleFusillade` | FUSILLADE | normal | adapted | Double base projectiles; -25% damage and speed. |
| `ModTitleBlastRadius` | BLAST RADIUS | normal | adapted | +25% blast radius. Ordinary shots gain a small blast. |
| `ModTitleHighExplosive` | HIGH EXPLOSIVE | normal | adapted | +45% blast radius. |
| `ModTitleConcentratedBlast` | CONCENTRATED BLAST | normal | adapted | +45% blast damage; -25% blast radius. |
| `ModTitleRupture` | RUPTURE | normal | adapted | Destroyed enemies explode for 15% of maximum hull. |
| `ModTitleVelocity` | VELOCITY | normal | adapted | +20% projectile speed; +5% weapon damage. |
| `ModTitleSnipe` | SNIPE | normal | adapted | Damage grows with projectile age; +15% shot speed. |
| `ModTitleCalibrate` | CALIBRATE | normal | adapted | Cease fire to improve spread, targeting and shot speed. |
| `ModTitleBreach` | INCENDIARY STRIKE | normal | adapted | Weapon hits ignite enemies for lingering damage. |
| `ModTitleRapidFire` | RAPID FIRE | normal | adapted | +20% rate of fire. |
| `ModTitleBurstFire` | BURST FIRE | normal | adapted | Fire three rapid volleys, followed by recovery. |
| `ModTitleWarpath` | WARPATH | normal | adapted | Kills temporarily increase weapon rate and thrust. |
| `ModTitleSiegeWeaponry` | SIEGE WEAPONRY | normal | adapted | Continuous fire boosts rate, but begins to harm hull. |
| `ModTitleMagnitude` | MAGNITUDE | normal | adapted | +15% weapon damage and projectile size. |
| `ModTitlePayload` | PAYLOAD | normal | adapted | +30% weapon damage and blast; -15% rate of fire. |
| `ModTitleChargedShot` | CHARGED SHOT | normal | adapted | Pausing fire charges the next volley for extra damage. |
| `ModTitleSplinter` | SPLINTER | normal | adapted | Destroyed targets release three secondary projectiles. |
| `ModTitleTargeting` | TARGETING | normal | adapted | Projectiles turn gently toward nearby targets. |
| `ModTitleGuidance` | GUIDANCE | normal | adapted | Stronger homing; +15% projectile speed. |
| `ModTitleHomingStrike` | HOMING STRIKE | normal | adapted | Strong homing, with lower shot speed. |
| `ModTitleConvergence` | CONVERGENCE | normal | adapted | Narrower volleys; improved homing and damage. |
| `ModTitleHullStrength` | HULL STRENGTH | normal | adapted | +20% maximum hull. |
| `ModTitleAbsorption` | ABSORPTION | normal | adapted | +3 plating: reduces each ordinary hull hit. |
| `ModTitleForceArmor` | FORCE ARMOR | normal | adapted | Large hull hits are reduced by 25%. |
| `ModTitleJuggernaut` | JUGGERNAUT | normal | adapted | +35% maximum hull and crash damage; -15% thrust. |
| `ModTitleHullRegeneration` | REGENERATION | normal | adapted | Slowly regenerate damaged hull. |
| `ModTitleRapidReconstruction` | RAPID RECONSTRUCTION | normal | adapted | Remove shield; +50% hull and much faster regeneration. |
| `ModTitleAdrenalModule` | ADRENAL MODULE | normal | adapted | Weapon rate increases as hull falls. |
| `ModTitleEssenceSap` | ESSENCE SAP | normal | adapted | Every third kill restores a small amount of hull. |
| `ModTitleShieldDurability` | SHIELD DURABILITY | normal | adapted | +20% maximum shields. |
| `ModTitleShieldCooldown` | SHIELD COOLDOWN | normal | adapted | Shield recovery starts 20% sooner. |
| `ModTitleBarrier` | BARRIER | normal | adapted | A single hit cannot remove more than 30 shield. |
| `ModTitleOmniShield` | OMNI SHIELD | normal | adapted | +60% shields, -40% hull. |
| `ModTitleShieldEffectRadius` | SHIELD RADIUS | normal | adapted | +30% shield effect radius. |
| `ModTitleFocusedShields` | FOCUSED SHIELDS | normal | adapted | +35% shield effect power; -15% effect radius. |
| `ModTitleRadiantShields` | RADIANT SHIELDS | normal | adapted | +30% shield radius and +15% capacity. |
| `ModTitleWeaponizedShields` | WEAPONIZED SHIELDS | normal | adapted | +60% effect power. Active shields burn close targets. |
| `ModTitleVolatileShields` | VOLATILE SHIELDS | normal | adapted | Shield breaks release a powerful damaging blast. |
| `ModTitleDischarge` | DISCHARGE | normal | adapted | Firing a weapon emits a close electric burst; costs shield. |
| `ModTitleRegenerativeShields` | REGENERATIVE SHIELDS | normal | adapted | Regenerate shields while active. |
| `ModTitleEmergencySystems` | EMERGENCY SYSTEMS | normal | adapted | Brief invulnerability after shield breaks. |
| `ModTitleImprovedThrusters` | IMPROVED THRUSTERS | normal | adapted | +25% thrust and maximum speed. |
| `ModTitleStreamline` | STREAMLINE | normal | adapted | +15% thrust; less drag while coasting. |
| `ModTitleDeadlyWake` | DEADLY WAKE | normal | adapted | Thrusters leave a damaging trail. |
| `ModTitleKineticBoost` | KINETIC BOOST | normal | adapted | Weapon damage increases with ship speed. |
| `ModTitleStabilization` | STABILIZATION | normal | adapted | Stronger braking and faster rotation. |
| `ModTitleAgility` | AGILITY | normal | adapted | +50% rotation speed; +10% thrust. |
| `ModTitlePowerReserves` | POWER RESERVES | normal | adapted | Dash recharges faster; +10% thrust. |
| `ModTitleBlink` | BLINK | normal | adapted | Ability key teleports and leaves damaging endpoint blasts. |
| `ModTitleDrones` | DRONES | normal | adapted | Assemble two orbiting armed drones. |
| `ModTitleReinforcedDrones` | GRACE PROTOCOL | normal | adapted | +40% construct hull. |
| `ModTitleAdvancedEngineering` | ADVANCED ENGINEERING | normal | adapted | +30% construct power and faster assembly. |
| `ModTitleAssaultDrones` | ASSAULT DRONES | normal | adapted | Drones fire faster, stronger homing rounds. |
| `ModTitleDefensiveDrones` | DEFENSE DRONES | normal | adapted | Drones intercept enemy projectiles and repair hull. |
| `ModTitleMines` | MINES | normal | adapted | Automatically lay proximity mines with explosive bomblets. |
| `ModTitleAutoMines` | MINEFIELD | normal | adapted | More mines and faster mine assembly. |
| `ModTitleLoadedMines` | LOADED MINES | normal | adapted | Detonating mines also fire a radial volley. |
| `ModTitleTurret` | TURRET | normal | adapted | Assemble a stationary automatic turret. |
| `ModTitlePointDefense` | POINT DEFENSE | normal | adapted | Turrets intercept nearby enemy projectiles. |
| `ModTitleWarMachine` | WAR MACHINE | normal | adapted | Turrets fire powerful homing rockets. |
| `ModTitleCounterArtillery` | COUNTER ARTILLERY | normal | adapted | Turrets fire faster; intercepted shots trigger a blast. |
| `ModTitleAlly` | ALLY | normal | adapted | Assemble an autonomous escort with explosive volleys. |
| `ModTitleIntegratedWeaponry` | INTERCEPTOR | normal | adapted | Ally becomes a fast assault fighter. |
| `ModTitleGuardian` | GUARDIAN | normal | adapted | Ally intercepts shots and repairs your hull. |
| `ModTitleWingman` | WINGMAN | normal | adapted | Deploy an additional ally. |
| `ModTitleElegantConstruction` | ELEGANT CONSTRUCTION | normal | adapted | +25% construct power and durability. |
| `ModTitleSelfDestruction` | SELF DESTRUCTION | normal | adapted | Destroyed and expired constructs explode. |
| `ModTitleOverseer` | OVERSEER | normal | adapted | More constructs and faster assembly. Disables your weapon. |
| `ModTitleOverclock` | OVERCLOCK | normal | adapted | +35% construct fire rate and +20% construct damage. |
| `ModTitleCandescence` | CANDESCENCE | normal | adapted | +35% burn damage. |
| `ModTitleCorrosion` | CORROSION | normal | adapted | Burn damage makes targets increasingly vulnerable. |
| `ModTitlePurge` | PURGE | normal | adapted | Burning enemies explode on destruction. |
| `ModTitlePurification` | PURIFICATION | normal | adapted | Burn damage improves as hull falls; +15% hull. |
| `ModTitleSkirmish` | SKIRMISH | normal | adapted | +35% crash damage; crashing grants brief protection. |
| `ModTitleTerminate` | TERMINATE | normal | adapted | Weapon hits execute targets below 8% hull. |
| `ModTitleBlitz` | BLITZ | normal | adapted | More damage against nearly undamaged targets. |
| `ModTitleEfficiency` | EFFICIENCY | normal | adapted | While not firing, faster recovery and regeneration. |
| `ModTitleCoreShielding` | CORE SHIELDING | normal | adapted | Reduces burn and self-damage; +10% hull. |
| `ModTitleAdaptiveArmor` | ADAPTIVE ARMOR | normal | adapted | Taking damage temporarily increases plating. |
| `ModTitleChanneling` | CHANNELING | normal | adapted | Damage splits between remaining hull and shield. |
| `ModTitleAegis` | AEGIS | normal | adapted | While moving slowly, regenerate hull and strengthen shields. |
| `ModTitleRetribution` | RETRIBUTION | normal | adapted | Heavy hull hits automatically deploy a mine. |
| `ModTitleLance` | CELESTIAL LANCE | normal | adapted | High speed creates a burning aura that also harms your hull. |
| `ModTitleGemini` | GEMINI PROTOCOL | normal | adapted | Incoming damage is shared between hull and shields. |
| `ModTitleRocketDrones` | SUBSUMPTION | normal | adapted | Deploy drones faster, paying 2 hull per assembly. |
| `ModTitleNanotech` | NANOTECH RECONSTRUCTION | normal | adapted | Allies regenerate 2 hull each second. |
| `ModTitleEvolution` | EVOLUTION | normal | adapted | Ally damage and durability grow with your level. |
| `ModTitleEchoStrike` | ECHO STRIKE | normal | adapted | Ally rounds gain a larger secondary blast. |
| `ModTitleBattalionDrones` | FORMATION: BATTALION | normal | adapted | Assault drones line up at your flanks and fire ahead. |
| `ModTitlePredatorDrones` | TERMINAL DIRECTIVE | normal | adapted | Destroyed drones launch a seeking explosive strike. |
| `ModTitleFleet` | FLEET COMMANDER | normal | adapted | First two drones gain doubled hull and damage. |
| `ModTitleFormationRampart` | ARC BARRIER | normal | adapted | Defense drones reduce incoming blast and hull damage. |
| `ModTitleCounterpulse` | COUNTERPULSE | normal | adapted | Hit defense drones pulse damage into nearby enemies. |
| `ModTitleFlotilla` | RECONSTITUTION | normal | adapted | Destroyed defense drones drop a repair pickup. |
| `ModTitleNanotechDissemination` | NANOTECH DISSEMINATION | normal | adapted | Guardian absorbs construct damage and drops repairs on death. |
| `ModTitleDecoySignal` | DECOY SIGNAL | normal | adapted | Nearby enemies aim and steer toward your guardian. |
| `ModTitleMediCharge` | MEDI-CHARGE | normal | adapted | Guardian absorbs self-damage and periodically drops repairs. |
| `ModTitleEvasion` | EVASION | normal | adapted | Allies have a chance to dodge enemy projectiles. |
| `ModTitleOutmaneuver` | OUTMANEUVER | normal | adapted | Interceptors dodge and counter hostile shots. |
| `ModTitleOverpower` | OVERPOWER | normal | adapted | Interceptors fire a copy of your current projectile weapon. |
| `ModTitleShieldedDrones` | SHIELDED CONSTRUCTS | normal | adapted | Shield energy protects constructs; your hull takes less damage. |
| `ModTitleRebuke` | REBUKE | normal | adapted | Damage stored by Adaptive Armor powers your next volley. |
| `ModTitleTheVoid` | THE VOID | wild | intentional empty original effect | An intentionally empty offer introduced by Evolutionary Niche. |
| `ModTitleFlashShielding` | FLASH SHIELDING | normal | adapted | -30% shield cooldown; -20% maximum shields. |
| `ModTitleReflexiveShields` | REFLEXIVE SHIELDS | normal | adapted | Taking shield damage releases a nearby energy pulse. |
| `ModTitleStrafe` | STRAFE | normal | adapted | Ability darts sideways when Left or Right is held. |
| `ModTitlePriorityZero` | PRIORITY ZERO | normal | adapted | Kills advance construct assembly and repair active constructs. |
| `ModTitleTacticalLink` | TACTICAL LINK | normal | adapted | Nearby advanced constructs and you take reduced damage. |
| `ModTitleSupportSpecialist` | SUPPORT SPECIALIST | normal | adapted | +40% construct power; -15% maximum hull and shields. |
| `ModTitleRegression` | REGRESSION | normal | adapted | Banish offered base mods, shrink and gain one level. |
| `ModTitleConversion` | CONVERSION | normal | adapted | Convert half of the smaller defense pool into the larger one. |
| `ModTitleHiddenPower` | HIDDEN POWER | super | adapted | +80% global damage and +30% defenses; disables weapon and constructs. |
| `ModTitleLastStand` | LAST STAND | super | adapted | Once per run, revive at 50% hull and gain double experience. |
| `ModTitleMastery` | MASTERY | super | adapted | +5% weapon damage, speed, size, radius and fire rate. |
| `ModTitleApotheosis` | APOTHEOSIS | super | adapted | Pass through enemies. Hull slowly degenerates. |
| `ModTitleTempestBreak` | TEMPEST BREAK | super | adapted | Shield break releases a burning storm. Cooldown cannot be reset. |
| `ModTitleSanctuary` | SANCTUARY | super | adapted | Shield effects originate from a large field that follows slowly. |
| `ModTitleSaturationFire` | SATURATION FIRE | super | adapted | Sustained firing calibrates your weapon and boosts damage. |
| `ModTitleApexMachinery` | APEX MACHINERY | super | adapted | Double construct hull and power, but halve assembly speed. |
| `ModTitleLeafOnTheWind` | LEAF ON THE WIND | super | adapted | Damage grows while avoiding hits, to +80%. |
| `ModTitleCelestialSurge` | CELESTIAL SURGE | super | adapted | Braking at speed launches a powerful burning orb. |
| `ModTitleVitalBond` | VITAL BOND | super | adapted | Constructs share your hull regeneration. |
| `ModTitleAnnihilation` | ANTIMATTER ROUNDS | super | adapted | Shots destroy enemy shots; +20 damage, but firing harms hull. |
| `ModTitleAtaraxia` | ATARAXIA | super | adapted | Each unspent upgrade gives +8% damage and +4% defenses/thrust. |
| `ModTitleDyingStar` | DYING STAR | super | adapted | A vast burning aura consumes 3 hull each second. |
| `ModTitleWarpStrike` | WARP STRIKE | super | adapted | Shots wrap once and gain damage. Ship wrapping costs defenses. |
| `ModTitleRancor` | RANCOR | super | adapted | Damage taken while charging greatly increases weapon charge. |
| `ModTitleBarrage` | BARRAGE | super | adapted | Convert volleys into a rapid single-projectile stream. |
| `ModTitleSingularStrikeNew` | SINGULAR STRIKE | super | adapted | One greatly enlarged projectile, gaining +25% damage per lost shot. |
| `ModTitleBurnoutReactors` | BURNOUT REACTORS | super | adapted | Constructs assemble twice as fast, decay, and fire faster when hurt. |
| `ModTitleDisplacement` | VOID SLICE | super | adapted | Strafing Blinks cut through enemies along the path. |
| `ModTitleChargedMines` | CHARGED MINES | super | adapted | Mines charge up and fire your projectile pattern when detonated. |
| `ModTitleMortar` | MORTAR | super | adapted | Firing launches assembled mines forward at high speed. |
| `ModTitleDeflagration` | DEFLAGRATION | super | adapted | Burning enemies release an igniting nova on destruction. |
| `ModTitleChargedShields` | CHARGED SHIELDS | super | adapted | Weapon charge increases shield radius and effect power. |
| `ModTitleStrafingStrikes` | STRAFING STRIKES | super | adapted | Dashing releases rapid volleys from both flanks. |
| `ModTitleInfuse` | INFUSE | super | adapted | Discharge invests shield power into your projectiles. |
| `ModTitleTacticalBeacon` | DEMOLITION SPHERE | super | adapted | One large mine with 3 payload charges, triggered by damage. |
| `ModTitleSlipstream` | SLIPSTREAM | wild | adapted | Recursive: +12% thrust, faster turning and less drag. |
| `ModTitleDoubleTap` | DOUBLE TAP | wild | adapted | Recursive: extra burst shot and +8% fire rate. |
| `ModTitleHypermetabolism` | HYPERMETABOLISM | wild | adapted | Recursive: +2 hull regeneration, -8% maximum defenses. |
| `ModTitleTwinStrike` | TWIN STRIKE | wild | adapted | Recursive: +1 projectile and narrower spread. |
| `ModTitleDefiance` | DEFIANCE | wild | adapted | Recursive: +2 plating and 10% self-damage resistance. |
| `ModTitleHeavyCaliber` | HEAVY CALIBER | wild | adapted | Recursive: +20% damage and size; -10% shot speed, recoil. |
| `ModTitleWinnow` | WINNOW | wild | adapted | Banish other offered mods, gain 3 rerolls and shrink. |
| `ModTitleOutrageModule` | OUTRAGE MODULE | wild | adapted | Heavy hull hits trigger an immediate empowered volley. |
| `ModTitlePolarInversion` | POLAR INVERSION | wild | adapted | Reverse pulls and recoil; +25% blast radius. |
| `ModTitleSolarHeart` | SOLAR HEART | wild | adapted | Recursive: +25% burn and flat shot damage; ongoing self-damage. |
| `ModTitleGalvanicOutburst` | GALVANIC OUTBURST | wild | adapted | Recursive: shield hits release seeking burning sparks. |
| `ModTitleScorchingWake` | SCORCHING WAKE | wild | adapted | Recursive: thrusters eject burning fireballs. |
| `ModTitleDeathBlossom` | DEATH BLOSSOM | wild | adapted | Shield breaks release a radial volley of your weapon. |
| `ModTitleRicochet` | RICOCHET | wild | adapted | Recursive: projectiles rebound toward another target. |
| `ModTitlePhantomStrike` | PHANTOM STRIKE | wild | adapted | Nonfatal rams teleport you behind the target and fire. |
| `ModTitleRevelation` | REVELATION | wild | adapted | Gain the next recursive Wild Mod two additional times. |
| `ModTitleEvolutionaryNiche` | EVOLUTIONARY NICHE | wild | adapted | Recursive: stronger body stats/effects; adds THE VOID to pool. |
| `ModTitleChaoticAmbition` | CHAOTIC AMBITION | wild | adapted | Recursive: improves the chance of Wild Mods in each offer. |
| `ModTitlePropulsiveMunitions` | PROPULSIVE MUNITIONS | wild | adapted | +20% weapon damage, reverse firing direction and strong recoil. |
| `ModTitleObsession` | OBSESSION | wild | adapted | Recursive mods gain an extra rank; each extra rank lowers defenses. |
| `ModTitleBravado` | BRAVADO | wild | adapted | +20% body stats and shield effects; all enemies become elite. |
| `ModTitleSpecialistMine` | MINE SPECIALIST | wild | adapted | Mines gain 60% damage/hull; slower other construct assembly. |
| `ModTitleSpecialistTurret` | TURRET SPECIALIST | wild | adapted | Turrets gain 60% damage/hull; slower other construct assembly. |
| `ModTitleSpecialistDrone` | DRONE SPECIALIST | wild | adapted | Drones gain 60% damage/hull; slower other construct assembly. |
| `ModTitleSpecialistAlly` | ALLY SPECIALIST | wild | adapted | Allies gain 60% damage/hull; slower other construct assembly. |
| `ModTitleRampage` | RAMPAGE | wild | adapted | Recursive: +15% damage and defenses, but permanent thrust. |
| `ModTitleDiscord` | DISCORD | wild | adapted | Erratic orbiting orbs strike nearby targets. |
| `ModTitleHavoc` | HAVOC | wild | source placeholder unavailable | Unavailable: supplied game's description is an ellipsis. |
| `ModTitleOnslaught` | ONSLAUGHT | wild | source placeholder unavailable | Unavailable: supplied game's description is an ellipsis. |
| `ModTitleTransmogrification` | TRANSMOGRIFICATION | wild | adapted | Next 3 upgrades offer only recursive Wild Mods; no rerolls. |
| `ModTitleEnergizedShields` | ENERGIZED SHIELDS | wild | adapted | Recursive: speed empowers shields; faster cooldown, slower weapon/assembly. |
| `ModTitleMasochism` | MASOCHISM | wild | adapted | Recursive: 15% less self-damage, 10% more enemy damage. |
| `ModTitleGrandeur` | GRANDEUR | wild | adapted | Recursive: grow in size and gain a level. |
| `ModTitlePowerSpike` | QUANTUM DECOHERENCE | wild | adapted | Gain 5 levels. Lose all rerolls and cannot gain more. |
| `ModTitleMaelstrom` | MAELSTROM | wild | adapted | Active shield builds a damaging storm as you take hits. |
| `ModTitleSpontaneousGeneration` | SPONTANEOUS GENERATION | wild | adapted | Recursive: kills may assemble temporary swarm constructs. |
| `ModTitleFarsight` | FAR SIGHT | wild | adapted | Recursive: +20% battlefield dimensions; +15% enemy damage. |
| `ModTitleExplosiveGrowth` | EXPLOSIVE GROWTH | wild | adapted | Next recursive Wild Mod gains 3 extra ranks and is banished. |
| `ModTitleChallenger` | CHALLENGER | wild | adapted | Recursive: summon a powerful champion bearing extra rewards. |
