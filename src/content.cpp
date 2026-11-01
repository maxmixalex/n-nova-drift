#include "content.h"
namespace nova {
const WeaponDef weapons[WEAPON_COUNT] = {
 {"BLASTER","Twin bullets. Balanced range and fire rate.","sHexDefaultWeapon",9,2,6,45,4,2,13},
 {"SPLIT SHOT","Fan of shots. Impacts splash nearby targets.","sHexSplit",15,5,5,37,11,2,11},
 {"FLAK","Short range shrapnel. Volley adds twice as many pellets.","sHexFlak",19,9,5,19,8,2,12},
 {"RAILGUN","Fast piercing shot; destroyed targets erupt in shrapnel.","sHexRailgun",29,1,12,42,0,2,72},
 {"GRENADE","Timed cluster explosive. Blast and size improve clusters.","sHexGrenade",25,1,3,40,0,4,54},
 {"SALVO","Cease fire to stockpile seeking missiles, then release them.","sHexSalvo",4,1,4,70,12,2,20},
 {"VORTEX","Hold fire to charge using hull; release a pulling vortex.","sHexVortex",15,1,2,110,0,10,12},
 {"PULSE","Two close radial blasts and a wide fan of sparks.","sHexPulse",25,7,4,15,25,2,29},
 {"TORRENT","Rapid short range waves. Reduces thrust while firing.","sHexTorrent",3,4,5,18,10,1,6},
 {"BLADE","Launch returning autonomous blades that fire submunitions.","sHexBladeDrone",35,1,4,120,0,4,22},
 {"DART","Piercing caustic darts stack lingering burn damage.","sHexDart",16,1,7,50,0,2,24},
 {"THERMAL LANCE","Charge a sustained beam. Burns and weakens targets.","sHexThermalLance",3,1,8,15,0,2,8},
 {"SWORDS","Sweeping paired blades. Speed adds damage; deflect shots.","sHexSwords",15,2,0,1,0,5,36}
};
const BodyDef bodies[BODY_COUNT] = {
 {"STANDARD","Balanced hull and thrust.","sPlayer",100,100,100,5},
 {"ASSAULT","Faster, tighter fire. Periodic empowered shots; fragile shield.","sPlayerAssault",90,110,75,5},
 {"HULLBREAKER","Heavy hull and speed-scaled ramming. Slower weapon.","sPlayerBreaker",160,105,70,7},
 {"SPECTRE","Coast without firing to cloak. Opening shots gain power.","sPlayerStealth",85,115,100,5},
 {"FIREFLY","Fast thrust that burns enemies behind the ship.","sPlayerFirefly",95,145,85,5},
 {"SENTINEL","Stronger shield effects and protection. Slower weapon.","sPlayerSentinel",100,85,125,6},
 {"ENGINEER","Extra constructs and faster assembly; fragile hull.","sPlayerEngineer",80,95,75,5},
 {"RESEARCH","Wide pickup attraction. Rerolls improve maximum defenses.","sPlayerResearch",90,105,80,5},
 {"BATTERY","Twin broadside cannons. More powerful, slower volleys.","sPlayerBattery",130,75,100,7},
 {"ARCHITECT","Only one construct; greatly improved power and durability.","sPlayerArchitect",105,95,100,6},
 {"VIPER","Ramming or hull hits release poisonous venom strikes.","sPlayerViper",105,110,90,5},
 {"CARRIER","Deploys a swarm that attacks or recovers damaged shields.","sPlayerCarrier",120,80,125,7},
 {"COURSER","Fast dual cannons, more projectiles with reduced damage.","sPlayerCourser",85,130,85,5},
 {"LEVIATHAN","Growing segmented hull. Segments deal crash damage.","sPlayerLeviathan",140,80,70,7}
};
const ShieldDef shields[SHIELD_COUNT] = {
 {"STANDARD","Balanced protection; fully recharges after cooldown.","sHexDefaultShield",100,195,15},
 {"HALO","Burns nearby enemies, slowly consuming shield energy.","sHexHalo",80,180,24},
 {"REFLECT","Reflects hostile projectiles toward enemies.","sHexReflect",70,195,18},
 {"BASTION","Regenerating forward barrier. Stores energy for retaliation.","sHexBastion",125,210,21},
 {"TEMPORAL","Slows nearby enemies and hostile projectiles.","sHexTemporal",75,195,34},
 {"AMP","Boosts weapon power and thrust while the shield is active.","sHexAmp",55,150,23},
 {"WARP","On breaking, teleports forward and blasts both endpoints.","sHexWarp",65,165,33},
 {"ORBITAL","Three orbiting spheres deal contact damage and block shots.","sHexHelix",110,225,25},
 {"SIPHON","Drains nearby enemies to slowly recover shields.","sHexSiphon",80,210,29},
 {"SHOCKWAVE","Pushes threats away. Recharge blasts and weakens enemies.","sHexShockwave",100,180,29}
};
// -1 means no prerequisite. Two prerequisites are AND; every listed mod has
// an implemented effect. Text documents this port's actual rules and values.
#define M(n,d,k,a,b) {n,d,k,a,b,1,0,1,"sHex" k}
#define X(n,d,k,a,b,r,c,i) {n,d,k,a,b,r,c,i,"sHex" k}
const ModDef mods[MOD_COUNT] = {
 M("VOLLEY","+2 shots; -15% shot damage and speed.","AddedProjectiles",-1,-1),
 M("FOCUS FIRE","Halves projectile spread.","FocusFire",M_VOLLEY,-1),
 M("FIRING ARRAY","Central projectiles +50% damage; wider firing array.","FiringArray",M_VOLLEY,-1),
 M("FUSILLADE","Double base projectiles; -25% damage and speed.","Fusillade",M_FOCUS_FIRE,M_FIRING_ARRAY),
 M("BLAST RADIUS","+25% blast radius. Ordinary shots gain a small blast.","BlastRadius",-1,-1),
 M("HIGH EXPLOSIVE","+45% blast radius.","HighExplosive",M_BLAST_RADIUS,-1),
 M("CONCENTRATED BLAST","+45% blast damage; -25% blast radius.","ConcentratedBlast",M_BLAST_RADIUS,-1),
 M("RUPTURE","Destroyed enemies explode for 15% of maximum hull.","Rupture",M_HIGH_EXPLOSIVE,M_CONCENTRATED_BLAST),
 M("VELOCITY","+20% projectile speed; +5% weapon damage.","Velocity",-1,-1),
 M("SNIPE","Damage grows with projectile age; +15% shot speed.","Snipe",M_VELOCITY,-1),
 M("CALIBRATE","Cease fire to improve spread, targeting and shot speed.","Calibrate",M_VELOCITY,-1),
 M("INCENDIARY STRIKE","Weapon hits ignite enemies for lingering damage.","Breach",M_SNIPE,M_CALIBRATE),
 M("RAPID FIRE","+20% rate of fire.","RapidFire",-1,-1),
 M("BURST FIRE","Fire three rapid volleys, followed by recovery.","BurstFire",M_RAPID_FIRE,-1),
 M("WARPATH","Kills temporarily increase weapon rate and thrust.","Warpath",M_RAPID_FIRE,-1),
 M("SIEGE WEAPONRY","Continuous fire boosts rate, but begins to harm hull.","SiegeWeaponry",M_BURST_FIRE,M_WARPATH),
 M("MAGNITUDE","+15% weapon damage and projectile size.","Magnitude",-1,-1),
 M("PAYLOAD","+30% weapon damage and blast; -15% rate of fire.","Payload",M_MAGNITUDE,-1),
 M("CHARGED SHOT","Pausing fire charges the next volley for extra damage.","ChargedShot",M_MAGNITUDE,-1),
 M("SPLINTER","Destroyed targets release three secondary projectiles.","Splinter",M_PAYLOAD,M_CHARGED_SHOT),
 M("TARGETING","Projectiles turn gently toward nearby targets.","Targeting",-1,-1),
 M("GUIDANCE","Stronger homing; +15% projectile speed.","Guidance",M_TARGETING,-1),
 M("HOMING STRIKE","Strong homing, with lower shot speed.","HomingStrike",M_TARGETING,-1),
 M("CONVERGENCE","Narrower volleys; improved homing and damage.","Convergence",M_GUIDANCE,M_HOMING_STRIKE),
 M("HULL STRENGTH","+20% maximum hull.","HullStrength",-1,-1),
 M("ABSORPTION","+3 plating: reduces each ordinary hull hit.","Absorption",M_HULL_STRENGTH,-1),
 M("FORCE ARMOR","Large hull hits are reduced by 25%.","ForceArmor",M_HULL_STRENGTH,-1),
 M("JUGGERNAUT","+35% maximum hull and crash damage; -15% thrust.","Juggernaut",M_ABSORPTION,M_FORCE_ARMOR),
 M("REGENERATION","Slowly regenerate damaged hull.","HullRegeneration",-1,-1),
 M("RAPID RECONSTRUCTION","Remove shield; +50% hull and much faster regeneration.","RapidReconstruction",M_REGENERATION,M_HULL_STRENGTH),
 M("ADRENAL MODULE","Weapon rate increases as hull falls.","AdrenalModule",M_REGENERATION,-1),
 M("ESSENCE SAP","Every third kill restores a small amount of hull.","EssenceSap",M_ADRENAL,-1),
 M("SHIELD DURABILITY","+20% maximum shields.","ShieldDurability",-1,-1),
 M("SHIELD COOLDOWN","Shield recovery starts 20% sooner.","ShieldCooldown",-1,-1),
 M("BARRIER","A single hit cannot remove more than 30 shield.","Barrier",M_SHIELD_DURABILITY,-1),
 M("OMNI SHIELD","+60% shields, -40% hull.","OmniShield",M_SHIELD_DURABILITY,M_SHIELD_COOLDOWN),
 M("SHIELD RADIUS","+30% shield effect radius.","ShieldEffectRadius",-1,-1),
 M("FOCUSED SHIELDS","+35% shield effect power; -15% effect radius.","FocusedShields",M_SHIELD_RADIUS,-1),
 M("RADIANT SHIELDS","+30% shield radius and +15% capacity.","RadiantShields",M_SHIELD_RADIUS,-1),
 M("WEAPONIZED SHIELDS","+60% effect power. Active shields burn close targets.","WeaponizedShields",M_FOCUSED_SHIELDS,M_RADIANT_SHIELDS),
 M("VOLATILE SHIELDS","Shield breaks release a powerful damaging blast.","VolatileShields",M_SHIELD_COOLDOWN,-1),
 M("DISCHARGE","Firing a weapon emits a close electric burst; costs shield.","Discharge",M_VOLATILE_SHIELDS,-1),
 M("REGENERATIVE SHIELDS","Regenerate shields while active.","RegenerativeShields",M_SHIELD_DURABILITY,-1),
 M("EMERGENCY SYSTEMS","Brief invulnerability after shield breaks.","EmergencySystems",M_SHIELD_COOLDOWN,-1),
 M("IMPROVED THRUSTERS","+25% thrust and maximum speed.","ImprovedThrusters",-1,-1),
 M("STREAMLINE","+15% thrust; less drag while coasting.","Streamline",M_THRUSTERS,-1),
 M("DEADLY WAKE","Thrusters leave a damaging trail.","DeadlyWake",M_THRUSTERS,-1),
 M("KINETIC BOOST","Weapon damage increases with ship speed.","KineticBoost",M_STREAMLINE,M_DEADLY_WAKE),
 M("STABILIZATION","Stronger braking and faster rotation.","Stabilization",-1,-1),
 M("AGILITY","+50% rotation speed; +10% thrust.","Agility",M_STABILIZATION,-1),
 M("POWER RESERVES","Dash recharges faster; +10% thrust.","PowerReserves",M_STABILIZATION,-1),
 M("BLINK","Ability key teleports and leaves damaging endpoint blasts.","Blink",M_POWER_RESERVES,M_AGILITY),
 M("DRONES","Assemble two orbiting armed drones.","Drones",-1,-1),
 M("GRACE PROTOCOL","+40% construct hull.","ReinforcedDrones",M_DRONES,-1),
 M("ADVANCED ENGINEERING","+30% construct power and faster assembly.","AdvancedEngineering",M_GRACE_PROTOCOL,-1),
 M("ASSAULT DRONES","Drones fire faster, stronger homing rounds.","AssaultDrones",M_DRONES,-1),
 M("DEFENSE DRONES","Drones intercept enemy projectiles and repair hull.","DefensiveDrones",M_DRONES,-1),
 M("MINES","Automatically lay proximity mines with explosive bomblets.","Mines",-1,-1),
 M("MINEFIELD","More mines and faster mine assembly.","AutoMines",M_MINES,-1),
 M("LOADED MINES","Detonating mines also fire a radial volley.","LoadedMines",M_MINES,-1),
 M("TURRET","Assemble a stationary automatic turret.","Turret",-1,-1),
 M("POINT DEFENSE","Turrets intercept nearby enemy projectiles.","PointDefense",M_TURRET,-1),
 M("WAR MACHINE","Turrets fire powerful homing rockets.","WarMachine",M_TURRET,-1),
 M("COUNTER ARTILLERY","Turrets fire faster; intercepted shots trigger a blast.","CounterArtillery",M_POINT_DEFENSE,M_WAR_MACHINE),
 M("ALLY","Assemble an autonomous escort with explosive volleys.","Ally",-1,-1),
 M("INTERCEPTOR","Ally becomes a fast assault fighter.","IntegratedWeaponry",M_ALLY,-1),
 M("GUARDIAN","Ally intercepts shots and repairs your hull.","Guardian",M_ALLY,-1),
 M("WINGMAN","Deploy an additional ally.","Wingman",M_INTERCEPTOR,-1),
 M("ELEGANT CONSTRUCTION","+25% construct power and durability.","ElegantConstruction",-1,-1),
 M("SELF DESTRUCTION","Destroyed and expired constructs explode.","SelfDestruction",M_ELEGANT_CONSTRUCTION,-1),
 M("OVERSEER","More constructs and faster assembly. Disables your weapon.","Overseer",M_ELEGANT_CONSTRUCTION,M_DRONES),
 M("OVERCLOCK","+35% construct fire rate and +20% construct damage.","Overclock",M_ELEGANT_CONSTRUCTION,-1),
 M("CANDESCENCE","+35% burn damage.","Candescence",-1,-1),
 M("CORROSION","Burn damage makes targets increasingly vulnerable.","Corrosion",M_CANDESCENCE,-1),
 M("PURGE","Burning enemies explode on destruction.","Purge",M_CANDESCENCE,-1),
 M("PURIFICATION","Burn damage improves as hull falls; +15% hull.","Purification",M_CORROSION,M_PURGE),
 M("SKIRMISH","+35% crash damage; crashing grants brief protection.","Skirmish",-1,-1),
 M("TERMINATE","Weapon hits execute targets below 8% hull.","Terminate",M_SKIRMISH,-1),
 M("BLITZ","More damage against nearly undamaged targets.","Blitz",M_SKIRMISH,-1),
 M("EFFICIENCY","While not firing, faster recovery and regeneration.","Efficiency",-1,-1),
 M("CORE SHIELDING","Reduces burn and self-damage; +10% hull.","CoreShielding",M_EFFICIENCY,-1),
 M("ADAPTIVE ARMOR","Taking damage temporarily increases plating.","AdaptiveArmor",M_EFFICIENCY,-1),
 M("CHANNELING","Damage splits between remaining hull and shield.","Channeling",M_CORE_SHIELDING,M_ADAPTIVE_ARMOR),
 M("AEGIS","While moving slowly, regenerate hull and strengthen shields.","Aegis",M_EFFICIENCY,-1),
 X("RETRIBUTION","Heavy hull hits automatically deploy a mine.","Retribution",M_MINES,-1,1,0,1),
 X("CELESTIAL LANCE","High speed creates a burning aura that also harms your hull.","Lance",M_THRUSTERS,M_DEADLY_WAKE,1,0,1),
 X("GEMINI PROTOCOL","Incoming damage is shared between hull and shields.","Gemini",M_EFFICIENCY,-1,1,0,1),
 X("SUBSUMPTION","Deploy drones faster, paying 2 hull per assembly.","RocketDrones",M_DRONES,-1,1,0,1),
 X("NANOTECH RECONSTRUCTION","Allies regenerate 2 hull each second.","Nanotech",M_ALLY,-1,1,0,1),
 X("EVOLUTION","Ally damage and durability grow with your level.","Evolution",M_ALLY,-1,1,0,1),
 X("ECHO STRIKE","Ally rounds gain a larger secondary blast.","EchoStrike",M_ALLY,-1,1,0,1),
 X("FORMATION: BATTALION","Assault drones line up at your flanks and fire ahead.","BattalionDrones",M_ASSAULT_DRONES,-1,1,0,1),
 X("TERMINAL DIRECTIVE","Destroyed drones launch a seeking explosive strike.","PredatorDrones",M_ASSAULT_DRONES,-1,1,0,1),
 X("FLEET COMMANDER","First two drones gain doubled hull and damage.","Fleet",M_ASSAULT_DRONES,-1,1,0,1),
 X("ARC BARRIER","Defense drones reduce incoming blast and hull damage.","FormationRampart",M_DEFENSE_DRONES,-1,1,0,1),
 X("COUNTERPULSE","Hit defense drones pulse damage into nearby enemies.","Counterpulse",M_DEFENSE_DRONES,-1,1,0,1),
 X("RECONSTITUTION","Destroyed defense drones drop a repair pickup.","Flotilla",M_DEFENSE_DRONES,-1,1,0,1),
 X("NANOTECH DISSEMINATION","Guardian absorbs construct damage and drops repairs on death.","NanotechDissemination",M_GUARDIAN,-1,1,0,1),
 X("DECOY SIGNAL","Nearby enemies aim and steer toward your guardian.","DecoySignal",M_GUARDIAN,-1,1,0,1),
 X("MEDI-CHARGE","Guardian absorbs self-damage and periodically drops repairs.","MediCharge",M_GUARDIAN,-1,1,0,1),
 X("EVASION","Allies have a chance to dodge enemy projectiles.","Evasion",M_ALLY,-1,1,0,1),
 X("OUTMANEUVER","Interceptors dodge and counter hostile shots.","Outmaneuver",M_INTERCEPTOR,-1,1,0,1),
 X("OVERPOWER","Interceptors fire a copy of your current projectile weapon.","Overpower",M_INTERCEPTOR,-1,1,0,1),
 X("SHIELDED CONSTRUCTS","Shield energy protects constructs; your hull takes less damage.","ShieldedDrones",M_ELEGANT_CONSTRUCTION,-1,1,0,1),
 X("REBUKE","Damage stored by Adaptive Armor powers your next volley.","Rebuke",M_ADAPTIVE_ARMOR,-1,1,0,1),
 X("THE VOID","An intentionally empty offer introduced by Evolutionary Niche.","TheVoid",-1,-1,5,2,1),
 X("FLASH SHIELDING","-30% shield cooldown; -20% maximum shields.","FlashShielding",M_SHIELD_COOLDOWN,-1,1,0,1),
 X("REFLEXIVE SHIELDS","Taking shield damage releases a nearby energy pulse.","ReflexiveShields",M_SHIELD_DURABILITY,-1,1,0,1),
 X("STRAFE","Ability darts sideways when Left or Right is held.","Strafe",M_STABILIZATION,-1,1,0,1),
 X("PRIORITY ZERO","Kills advance construct assembly and repair active constructs.","PriorityZero",M_ELEGANT_CONSTRUCTION,-1,1,0,1),
 X("TACTICAL LINK","Nearby advanced constructs and you take reduced damage.","TacticalLink",M_ELEGANT_CONSTRUCTION,-1,1,0,1),
 X("SUPPORT SPECIALIST","+40% construct power; -15% maximum hull and shields.","SupportSpecialist",M_ELEGANT_CONSTRUCTION,-1,1,0,1),
 X("REGRESSION","Banish offered base mods, shrink and gain one level.","Regression",-1,-1,1,0,1),
 X("CONVERSION","Convert half of the smaller defense pool into the larger one.","Conversion",M_EFFICIENCY,-1,1,0,1),
 X("HIDDEN POWER","+80% global damage and +30% defenses; disables weapon and constructs.","HiddenPower",M_SHIELD_RADIUS,M_REGENERATION,1,1,1),
 X("LAST STAND","Once per run, revive at 50% hull and gain double experience.","LastStand",M_HULL_STRENGTH,M_REGENERATION,1,1,1),
 X("MASTERY","+5% weapon damage, speed, size, radius and fire rate.","Mastery",M_RAPID_FIRE,M_MAGNITUDE,1,1,1),
 X("APOTHEOSIS","Pass through enemies. Hull slowly degenerates.","Apotheosis",M_SKIRMISH,M_BLINK,1,1,1),
 X("TEMPEST BREAK","Shield break releases a burning storm. Cooldown cannot be reset.","TempestBreak",M_VOLATILE_SHIELDS,M_SHIELD_COOLDOWN,1,1,1),
 X("SANCTUARY","Shield effects originate from a large field that follows slowly.","Sanctuary",M_SHIELD_RADIUS,M_BARRIER,1,1,1),
 X("SATURATION FIRE","Sustained firing calibrates your weapon and boosts damage.","SaturationFire",M_SIEGE,M_CALIBRATE,1,1,1),
 X("APEX MACHINERY","Double construct hull and power, but halve assembly speed.","ApexMachinery",M_ADVANCED_ENGINEERING,M_ELEGANT_CONSTRUCTION,1,1,1),
 X("LEAF ON THE WIND","Damage grows while avoiding hits, to +80%.","LeafOnTheWind",M_AGILITY,M_X_EVASION,1,1,1),
 X("CELESTIAL SURGE","Braking at speed launches a powerful burning orb.","CelestialSurge",M_X_LANCE,M_STABILIZATION,1,1,1),
 X("VITAL BOND","Constructs share your hull regeneration.","VitalBond",M_REGENERATION,M_ELEGANT_CONSTRUCTION,1,1,1),
 X("ANTIMATTER ROUNDS","Shots destroy enemy shots; +20 damage, but firing harms hull.","Annihilation",M_MAGNITUDE,M_PAYLOAD,1,1,1),
 X("ATARAXIA","Each unspent upgrade gives +8% damage and +4% defenses/thrust.","Ataraxia",M_EFFICIENCY,M_HULL_STRENGTH,1,1,1),
 X("DYING STAR","A vast burning aura consumes 3 hull each second.","DyingStar",M_CANDESCENCE,M_REGENERATION,1,1,1),
 X("WARP STRIKE","Shots wrap once and gain damage. Ship wrapping costs defenses.","WarpStrike",M_VELOCITY,M_BLINK,1,1,1),
 X("RANCOR","Damage taken while charging greatly increases weapon charge.","Rancor",M_CHARGED_SHOT,M_REGENERATION,1,1,1),
 X("BARRAGE","Convert volleys into a rapid single-projectile stream.","Barrage",M_BURST_FIRE,M_VOLLEY,1,1,1),
 X("SINGULAR STRIKE","One greatly enlarged projectile, gaining +25% damage per lost shot.","SingularStrikeNew",M_MAGNITUDE,M_FOCUS_FIRE,1,1,1),
 X("BURNOUT REACTORS","Constructs assemble twice as fast, decay, and fire faster when hurt.","BurnoutReactors",M_SELF_DESTRUCTION,M_OVERCLOCK,1,1,1),
 X("VOID SLICE","Strafing Blinks cut through enemies along the path.","Displacement",M_BLINK,M_X_STRAFE,1,1,1),
 X("CHARGED MINES","Mines charge up and fire your projectile pattern when detonated.","ChargedMines",M_LOADED_MINES,M_CHARGED_SHOT,1,1,1),
 X("MORTAR","Firing launches assembled mines forward at high speed.","Mortar",M_MINES,M_VELOCITY,1,1,1),
 X("DEFLAGRATION","Burning enemies release an igniting nova on destruction.","Deflagration",M_PURGE,M_BLAST_RADIUS,1,1,1),
 X("CHARGED SHIELDS","Weapon charge increases shield radius and effect power.","ChargedShields",M_CHARGED_SHOT,M_SHIELD_RADIUS,1,1,1),
 X("STRAFING STRIKES","Dashing releases rapid volleys from both flanks.","StrafingStrikes",M_X_STRAFE,M_VOLLEY,1,1,1),
 X("INFUSE","Discharge invests shield power into your projectiles.","Infuse",M_DISCHARGE,M_MAGNITUDE,1,1,1),
 X("DEMOLITION SPHERE","One large mine with 3 payload charges, triggered by damage.","TacticalBeacon",M_MINEFIELD,M_LOADED_MINES,1,1,1),
 X("SLIPSTREAM","Recursive: +12% thrust, faster turning and less drag.","Slipstream",-1,-1,5,2,1),
 X("DOUBLE TAP","Recursive: extra burst shot and +8% fire rate.","DoubleTap",-1,-1,5,2,1),
 X("HYPERMETABOLISM","Recursive: +2 hull regeneration, -8% maximum defenses.","Hypermetabolism",M_REGENERATION,-1,5,2,1),
 X("TWIN STRIKE","Recursive: +1 projectile and narrower spread.","TwinStrike",-1,-1,5,2,1),
 X("DEFIANCE","Recursive: +2 plating and 10% self-damage resistance.","Defiance",-1,-1,5,2,1),
 X("HEAVY CALIBER","Recursive: +20% damage and size; -10% shot speed, recoil.","HeavyCaliber",-1,-1,5,2,1),
 X("WINNOW","Banish other offered mods, gain 3 rerolls and shrink.","Winnow",-1,-1,1,2,1),
 X("OUTRAGE MODULE","Heavy hull hits trigger an immediate empowered volley.","OutrageModule",-1,-1,1,2,1),
 X("POLAR INVERSION","Reverse pulls and recoil; +25% blast radius.","PolarInversion",-1,-1,1,2,1),
 X("SOLAR HEART","Recursive: +25% burn and flat shot damage; ongoing self-damage.","SolarHeart",-1,-1,5,2,1),
 X("GALVANIC OUTBURST","Recursive: shield hits release seeking burning sparks.","GalvanicOutburst",-1,-1,5,2,1),
 X("SCORCHING WAKE","Recursive: thrusters eject burning fireballs.","ScorchingWake",-1,-1,5,2,1),
 X("DEATH BLOSSOM","Shield breaks release a radial volley of your weapon.","DeathBlossom",-1,-1,1,2,1),
 X("RICOCHET","Recursive: projectiles rebound toward another target.","Ricochet",-1,-1,5,2,1),
 X("PHANTOM STRIKE","Nonfatal rams teleport you behind the target and fire.","PhantomStrike",-1,-1,1,2,1),
 X("REVELATION","Gain the next recursive Wild Mod two additional times.","Revelation",-1,-1,1,2,1),
 X("EVOLUTIONARY NICHE","Recursive: stronger body stats/effects; adds THE VOID to pool.","EvolutionaryNiche",-1,-1,5,2,1),
 X("CHAOTIC AMBITION","Recursive: improves the chance of Wild Mods in each offer.","ChaoticAmbition",-1,-1,5,2,1),
 X("PROPULSIVE MUNITIONS","+20% weapon damage, reverse firing direction and strong recoil.","PropulsiveMunitions",-1,-1,1,2,1),
 X("OBSESSION","Recursive mods gain an extra rank; each extra rank lowers defenses.","Obsession",-1,-1,1,2,1),
 X("BRAVADO","+20% body stats and shield effects; all enemies become elite.","Bravado",-1,-1,1,2,1),
 X("MINE SPECIALIST","Mines gain 60% damage/hull; slower other construct assembly.","SpecialistMine",M_MINES,-1,1,2,1),
 X("TURRET SPECIALIST","Turrets gain 60% damage/hull; slower other construct assembly.","SpecialistTurret",M_TURRET,-1,1,2,1),
 X("DRONE SPECIALIST","Drones gain 60% damage/hull; slower other construct assembly.","SpecialistDrone",M_DRONES,-1,1,2,1),
 X("ALLY SPECIALIST","Allies gain 60% damage/hull; slower other construct assembly.","SpecialistAlly",M_ALLY,-1,1,2,1),
 X("RAMPAGE","Recursive: +15% damage and defenses, but permanent thrust.","Rampage",-1,-1,5,2,1),
 X("DISCORD","Erratic orbiting orbs strike nearby targets.","Discord",-1,-1,1,2,1),
 X("HAVOC","Unavailable: supplied game's description is an ellipsis.","Havoc",-1,-1,1,2,0),
 X("ONSLAUGHT","Unavailable: supplied game's description is an ellipsis.","Onslaught",-1,-1,1,2,0),
 X("TRANSMOGRIFICATION","Next 3 upgrades offer only recursive Wild Mods; no rerolls.","Transmogrification",-1,-1,1,2,1),
 X("ENERGIZED SHIELDS","Recursive: speed empowers shields; faster cooldown, slower weapon/assembly.","EnergizedShields",-1,-1,5,2,1),
 X("MASOCHISM","Recursive: 15% less self-damage, 10% more enemy damage.","Masochism",-1,-1,5,2,1),
 X("GRANDEUR","Recursive: grow in size and gain a level.","Grandeur",-1,-1,3,2,1),
 X("QUANTUM DECOHERENCE","Gain 5 levels. Lose all rerolls and cannot gain more.","PowerSpike",-1,-1,1,2,1),
 X("MAELSTROM","Active shield builds a damaging storm as you take hits.","Maelstrom",-1,-1,1,2,1),
 X("SPONTANEOUS GENERATION","Recursive: kills may assemble temporary swarm constructs.","SpontaneousGeneration",-1,-1,5,2,1),
 X("FAR SIGHT","Recursive: +20% battlefield dimensions; +15% enemy damage.","Farsight",-1,-1,3,2,1),
 X("EXPLOSIVE GROWTH","Next recursive Wild Mod gains 3 extra ranks and is banished.","ExplosiveGrowth",-1,-1,1,2,1),
 X("CHALLENGER","Recursive: summon a powerful champion bearing extra rewards.","Challenger",-1,-1,3,2,1)
};
#undef M
#undef X
const EnemyDef enemy_types[ENEMY_COUNT] = {
 {"ASTEROID","sAsteroidMedium1",35,9,1,15,3,0},
 {"SWARMLING","sSwarmling",16,4,2,9,2,0},
 {"STATION DRONE","sStationDrone",25,5,1,12,3,65},
 {"VANGUARD","sVanguard",40,6,2,16,4,60},
 {"SPITFIRE","sSpitfire",95,10,1,22,8,45},
 {"INTERCEPTOR","sInterceptor",50,6,3,18,5,50},
 {"BULWARK","sBulwarkBody",170,12,1,28,10,85},
 {"HAMMERHEAD","sHammerhead",120,10,2,28,9,70},
 {"SINE","sSine",80,8,2,20,7,48},
 {"PULSEBUG","sPulsebug",30,6,1,18,3,90},
 {"WARDEN","sWarden",150,10,1,25,10,70},
 {"BEAMCASTER","sBeamcaster",110,9,1,22,9,100},
 {"CHERUB","sCherub",65,7,1,16,8,0},
 {"JUGGERNAUT","sJuggernaut",290,14,1,35,16,55},
 {"MINELAYER","sMineLayer",110,9,1,22,10,80},
 {"TRACER","sTracer",85,7,3,20,8,40},
 {"SCION","sScion",1100,22,1,38,85,22},
 {"DREADNOUGHT","sDreadnoughtCenter",1800,27,1,45,130,18},
 {"STATION OMEGA","sStation",2500,27,1,45,160,16},
 {"WARBRINGER","sWarbringerChasis",3300,29,1,50,190,15},
 {"SERAPH","sSeraph",4000,24,2,48,220,16},
 {"STAR EATER","sStarEaterEye",6500,29,1,60,255,13},
 {"DWELLER","sDweller",2700,27,1,40,170,38},
 {"CERATOTITAN","sCeratotitan",3200,28,2,48,190,25},
 {"GLAUCUS","sGlaucusHead",3800,25,2,45,210,24},
 {"CRUCIBLE","sCrucible",4800,28,1,55,235,22}
};
const char *offerName(const Offer &o) {
 switch(o.kind) {case OFFER_WEAPON:return weapons[o.id%WEAPON_COUNT].name;
 case OFFER_BODY:return bodies[o.id%BODY_COUNT].name;
 case OFFER_SHIELD:return shields[o.id%SHIELD_COUNT].name;
 default:return mods[o.id%MOD_COUNT].name;}
}
const char *offerDescription(const Offer &o) {
 switch(o.kind) {case OFFER_WEAPON:return weapons[o.id%WEAPON_COUNT].description;
 case OFFER_BODY:return bodies[o.id%BODY_COUNT].description;
 case OFFER_SHIELD:return shields[o.id%SHIELD_COUNT].description;
 default:return mods[o.id%MOD_COUNT].description;}
}
const char *offerSprite(const Offer &o) {
 switch(o.kind) {case OFFER_WEAPON:return weapons[o.id%WEAPON_COUNT].sprite;
 case OFFER_BODY:return bodies[o.id%BODY_COUNT].sprite;
 case OFFER_SHIELD:return shields[o.id%SHIELD_COUNT].sprite;
 default:break;}
 switch(o.id){
 case M_SPLINTER:return "sHexSplinterShot";
 case M_TARGETING:return "sHexTargettingSystems";
 case M_GUIDANCE:return "sHexHeatSeeking";
 case M_SHIELD_RADIUS:return "sHexShieldRadius";
 case M_GRACE_PROTOCOL:return "sHexGraceProtocol";
 case M_DEFENSE_DRONES:return "sHexDefenseDrone";
 case M_X_ROCKET_DRONES:return "sHexSubsumption";
 case M_X_PREDATOR_DRONES:return "sHexTerminalDirective";
 case M_X_FLEET:return "sHexFleetCommander";
 case M_X_FORMATION_RAMPART:return "sHexArcBarrier";
 case M_X_FLOTILLA:return "sHexReconstitution";
 case M_X_THE_VOID:return "sHexVoid";
 case M_X_REFLEXIVE_SHIELDS:return "sHexRetaliationShields";
 case M_X_DISPLACEMENT:return "sHexVoidSlice";
 case M_X_HAVOC:case M_X_ONSLAUGHT:return "sHexLockedFrame";
 default:return mods[o.id%MOD_COUNT].sprite;}
}
const char *offerCategory(const Offer &o) {
 static const char *const categories[]={"MOD","WEAPON","BODY","SHIELD"};
 return categories[o.kind%4];
}
} // namespace nova
