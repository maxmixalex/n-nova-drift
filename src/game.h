#ifndef NOVA_GAME_H
#define NOVA_GAME_H
#include <stdint.h>
#include "content.h"
#include "boss_parts.h"
#include "source_waves.h"
#include "runtime_options.h"
#include "persistent_effects.h"
#include "advanced_constructs.h"

namespace nova {
enum { WIDTH=320, HEIGHT=240, TICK_RATE=30, FP=256,
    MAX_ENEMIES=64, FRIENDLY_BULLETS=192, HOSTILE_BULLETS=256,
    MAX_BULLETS=FRIENDLY_BULLETS+HOSTILE_BULLETS, MAX_PICKUPS=72,
    MAX_EFFECTS=32, MAX_CONSTRUCTS=16, MAX_SEGMENTS=24, MAX_COSMIC_OBJECTS=24, OFFER_COUNT=7 };
// Coordinates and velocities are signed Q8; angles are 0..255 clockwise,
// with 0 pointing right. No floating point or heap allocation in the core.
inline int pixel(int32_t q8) { return q8 / FP; }
int dirX(uint8_t angle); // Q8 cosine
int dirY(uint8_t angle); // Q8 sine
uint8_t direction(int32_t dx, int32_t dy);
enum State { Title, Playing, Upgrade, Pause, GameOver };
enum RunFlag { RUN_WILD=1, RUN_DRAFT=2, RUN_MAYHEM=4, RUN_DANGER=8,
    RUN_ELITE=16, RUN_NEMESIS=32, RUN_HOSTILE=64, RUN_ENEMY_TERRITORY=128,
    RUN_ANNIHILATION=256, RUN_ENDLESS=512, RUN_PRACTICE=1024, RUN_DAILY=2048 };
enum DailyVariant { DAILY_SERPENTINE,DAILY_BOSS_RUSH,DAILY_DRAFT20,DAILY_DRAFT45,
    DAILY_COSMIC_EGG,DAILY_PURITY,DAILY_TURBO,DAILY_TURBO_ENDLESS,DAILY_SWORD,DAILY_NOVA_BROTHERS,DAILY_VARIANT_COUNT };
enum DailyModifier { DAILY_NONE,DAILY_GRACE,DAILY_UNBREAKABLE,DAILY_UNSTOPPABLE,
    DAILY_IMMOVABLE,DAILY_HEIST,DAILY_HYPERACTIVE,DAILY_COMPACT,DAILY_HURT,
    DAILY_VIOLENCE,DAILY_IM_THE_BOSS,DAILY_CHOSEN,DAILY_WILDER,DAILY_PANDEMONIUM,
    DAILY_CAGE_MATCH,DAILY_TOUGHER,DAILY_ROGUE,DAILY_VENGEANCE,DAILY_SLIPPERY,
    DAILY_FRENZY,DAILY_LONG_SHOT,DAILY_CELESTIAL_SHOWER,DAILY_COSMIC_BALLET,DAILY_WILDERNESS,DAILY_MODIFIER_COUNT };
enum CosmicKind { COSMIC_COMET,COSMIC_TRAIN_ENGINE,COSMIC_TRAIN_CAR,COSMIC_BLACK_HOLE,COSMIC_KIND_COUNT };
struct CosmicObject {
    int32_t x,y,vx,vy;
    int16_t hull;
    uint16_t age,life,warning;
    uint8_t active,kind,angle,radius,train;
};
struct RunStats {
    uint32_t damage_dealt,damage_taken,shots_fired,experience_collected;
    uint32_t distance_q8,highest_hit;
    uint64_t seen_enemies,seen_enemies_hi;
    uint32_t coalesced_shots;
    uint16_t enemy_kills[ENEMY_COUNT],boss_kills,rerolls_used,mods_taken;
    uint16_t hull_damage,shield_breaks;
};
enum BulletKind { SHOT_BLASTER, SHOT_SPLIT, SHOT_FLAK, SHOT_RAIL,
    SHOT_GRENADE, SHOT_MISSILE, SHOT_VORTEX, SHOT_TORRENT,
    SHOT_DART, SHOT_SPARK, SHOT_ENEMY, SHOT_SHRAPNEL, SHOT_BLADE, SHOT_WAKE,
    SHOT_SERAPH_ORB, SHOT_SERAPH_SNIPE,
    SHOT_ENEMY_SHIELD, SHOT_WARDEN, SHOT_PULSEBUG, SHOT_SINE, SHOT_SINE_MEGA,
    SHOT_BEAMCASTER, SHOT_BOLT, SHOT_BOLT_CHAMPION, SHOT_TRACER,
    SHOT_TRACER_BALL, SHOT_TRACER_WAKE, SHOT_JUGGERNAUT_ORB,
    SHOT_JUGGERNAUT_BOMB, SHOT_DWELLER_GUARD, SHOT_DWELLER_CHITIN,
    SHOT_DWELLER_ARC, SHOT_INTERCEPTOR_MISSILE, SHOT_PULSEBUG_HOMING,
    SHOT_STATION_DRONE, SHOT_WARDEN_VORTEX, SHOT_JUGGERNAUT,
    SHOT_JUGGERNAUT_HOMING, SHOT_CELESTIAL, SHOT_MINE_DRONE, SHOT_WORM_SPINE, SHOT_DISCHARGE, SHOT_TRIANGLE_SPAWNER, SHOT_PULSAR_MINE,
    SHOT_SPIKE, SHOT_SPIKE_MINI, SHOT_SPIKE_TRIPLE, SHOT_CELESTIAL_DRONE,
    SHOT_PULSAR_BARRAGE, SHOT_PULSAR_PULSE, SHOT_CELESTIAL_STRIKE, SHOT_CELESTIAL_STRIKE_MINI, SHOT_CELESTIAL_WARNING,
    SHOT_SNAPPER_BEAM, SHOT_ECLIPSE_TORRENT, SHOT_ECLIPSE_BOLT, SHOT_ECLIPSE_ORB, SHOT_ECLIPSE_VORTEX,
    SHOT_HOST_DRONE, SHOT_SERAPH_ZONE, SHOT_MYRMIDON_BOMB, SHOT_MYRMIDON_SPAWNER, SHOT_MYRMIDON_BLAST,
    SHOT_MYRMIDON_TURRET, SHOT_ECLIPSE_REFLECT, SHOT_ECLIPSE_BURST, SHOT_ECLIPSE_SHOT, SHOT_KIND_COUNT };
enum ConstructKind { C_DRONE, C_DEFENSE_DRONE, C_MINE, C_TURRET,
    C_ALLY, C_INTERCEPTOR, C_GUARDIAN, C_SWARM, C_BLADE, C_SWORD, C_BASTION, C_KIND_COUNT };
enum EffectKind { FX_BURST, FX_RING, FX_BEAM, FX_SHIELD_BREAK, FX_WARP,
    FX_MUZZLE, FX_HIT, FX_SWORD, FX_SPAWN, FX_SERAPH_LANCE, FX_SWARM_BEAM, FX_SWARM_CHARGE, FX_PULSAR_CHARGE };
enum PickupKind { PICKUP_XP, PICKUP_HEAL, PICKUP_REROLL, PICKUP_POWER };
struct GameInput {
    bool up, down, left, right, fire, confirm, back, pause, ability;
    bool pointer_valid, pointer_down;
    int16_t pointer_x, pointer_y;

};
struct Player {
    int32_t x,y,vx,vy;
    int16_t hull,max_hull,shield,max_shield;
    uint16_t shield_delay,fire_delay,ability_delay,invulnerable;
    uint16_t charge,cloak,missiles,missile_clock,shield_energy;
    uint8_t angle,radius,weapon,body,shield_type;
    uint8_t mods[MOD_COUNT];
    bool thrusting,firing;
};
struct Enemy {
    int32_t x,y,vx,vy;
    int32_t hull,max_hull;
    int32_t hard_shield;
    uint16_t hard_shield_timer,heal_fraction,aura_fraction;
    int16_t burn_damage;
    uint16_t age,fire_delay,burn_time,stun,wake_time;
    uint16_t phase_age,phase_timer,action_timer;
    uint8_t form,action_count,attack_angle,boss_hidden;
    int32_t target_x,target_y;
    uint8_t type,angle,radius,active,elite,phase,flash,vulnerability,burn_ignite;
    uint16_t generation,leader_generation,formation_delay,formation_break;
    int8_t leader;
    uint8_t formation,formation_slot,formation_count;
    int16_t formation_angle;
    uint16_t formation_spacing,summon_timer,summon_interval;
    uint16_t damage_scale,treasure_scale,size_scale;
    uint8_t summon_type,challenger,damage_fraction;
};
struct Bullet {
    int32_t x,y,vx,vy;
    int32_t damage;
    uint32_t field_scale; // Vortex scale (millionths); dedicated hostile effects use documented fixed-point state.
    uint16_t tracking_scale_x,tracking_scale_y,tracking_seek;
    int16_t tracking_target;
    uint8_t tracking,tracking_weapon;
    uint16_t life,age,speed,traveled,size_percent;
    uint8_t active,owner,kind,radius,angle,homing,pierce,blast,bounces,wraps,construct_source;
    int16_t last_hit;
};
struct Pickup {
    int32_t x,y,vx,vy;
    uint16_t value,age;
    uint8_t active,kind;
};
struct Effect {
    int32_t x,y,x2,y2;
    uint8_t active,kind,life,max_life,radius,color;
};
struct Construct {
    int32_t x,y,vx,vy;
    int16_t hull,max_hull;
    uint16_t age,fire_delay,life,grace_cooldown,grace_invulnerable;
    uint32_t regen_fraction,weapon_charge;
    uint32_t vortex_scale,vortex_rate,vortex_fraction;
    uint16_t decay_fraction,charge_ticks,promotion_ticks,chain_percent,redirected_fraction,vortex_cost_fraction;
    uint8_t active,kind,angle,radius,charges,promoted;
};
struct WaveSpawnQueue {
    SourceSpawnSpec spec;
    uint16_t delay,root_index,follower_index,repeat_index,leader_generation;
    int16_t leader,origin_x,origin_y;
    uint8_t edge,done;
};
struct Stats {
    int16_t damage,fire_rate,velocity,blast,size,thrust,turn,shield_effect;
    int16_t construct_damage,assembly,regen,plating,homing,projectiles;
    int16_t shield_radius,shield_cooldown,crash_damage;
};
class Game {
public:
    State state;
    Player player;
    Stats stats;
    Enemy enemies[MAX_ENEMIES];
    BossPart boss_parts[MAX_BOSS_PARTS];
    Bullet bullets[MAX_BULLETS];
    Pickup pickups[MAX_PICKUPS];
    Effect effects[MAX_EFFECTS];
    Construct constructs[MAX_CONSTRUCTS];
    AdvancedConstructState advanced_constructs[MAX_CONSTRUCTS];
    PersistentField persistent_fields[MAX_PERSISTENT_FIELDS];
    MaelstromState maelstrom_state;
    int32_t segment_x[MAX_SEGMENTS],segment_y[MAX_SEGMENTS];
    uint8_t segment_count;
    Offer offers[OFFER_COUNT];
    uint8_t selected_offer,offer_count;
    uint16_t level,wave,rerolls,banked_upgrades;
    uint32_t score,kills,xp,xp_next,frame,seed;
    uint16_t next_wave,wave_banner,damage_flash;
    uint32_t dropped_bullets,dropped_enemies; // Capacity diagnostics only.
    int16_t boss_index;
    bool auto_fire,wild_mode;
    uint16_t run_flags,starting_wave;
    uint8_t account_rank;
    bool final_boss_unlocked;
    RunStats run_stats;
    RuntimeOptions options;
    bool victory;
    int32_t sanctuary_x,sanctuary_y;
    uint32_t daily_day;
    uint8_t daily_variant,daily_modifier;
    int32_t daily_egg_x,daily_egg_y;
    int16_t daily_egg_hull;
    uint16_t daily_egg_grace;
    CosmicObject cosmic_objects[MAX_COSMIC_OBJECTS];

    Game() = default;
    void resetInput();
    bool validState() const;
    void start(uint32_t run_seed);
    void configureCareer(unsigned rank,bool final_boss_defeated);
    void configureRun(uint16_t flags); // Call after start, before the first tick.
    static uint32_t dailySeedFor(uint32_t day);
    static unsigned dailyVariantFor(uint32_t day);
    static unsigned dailyModifierFor(uint32_t day);
    void configureDaily(uint32_t day);
    bool daily(unsigned variant) const {return (run_flags&RUN_DAILY)&&daily_variant==variant;}
    bool dailyMod(unsigned modifier) const {return (run_flags&RUN_DAILY)&&daily_modifier==modifier;}
    unsigned dailyEnemySpeedPercent() const;
    void tick(const GameInput &input); // Exactly one 1/30s simulation step.
    void advance(uint32_t elapsed_ms,const GameInput &input); // bounded catch-up
    bool chooseOffer(unsigned index);
    bool rerollOffers();
    bool gearSelectionComplete() const {return gear_choices_>=3;}
    bool prepareGearOffers(unsigned kind); // Uses the current upgrade point.
    bool banishOffer(unsigned index); // Research body: two rerolls per mod.
    unsigned scoreBonus() const;
    unsigned experienceBonus() const;
    unsigned weaponProjectiles() const;
    unsigned weaponSpreadArcDegrees() const;
    unsigned weaponCannons() const;
    unsigned crashDamageTakenPercent() const;
    unsigned knockbackResistancePercent() const {return 0;}
    unsigned constructHullPercent(unsigned kind) const;
    unsigned constructFireRatePercent(unsigned kind) const;
    unsigned weaponDamage() const;
    unsigned weaponCooldownTicks() const;
    unsigned globalDamagePermille() const;
    unsigned burnDamagePercent() const;
    unsigned igniteDamagePercent() const;
    unsigned igniteDurationTicks() const {return 90;}
    unsigned globalBlastRadiusPercent() const {return 100;}
    unsigned incomingDamagePercent(bool shield=false,bool self=false) const;
    unsigned hullRecoveryCap() const;
    unsigned shieldRecoveryCap() const;
    unsigned hullRegenerationMilli() const;
    unsigned shieldRegenerationMilli() const;
    unsigned activeShieldCooldownTicks() const;
    unsigned activeShieldRadius() const;
    unsigned activeShieldEffect() const;
    unsigned chargeValueMilli() const {return storedChargeMicro()/1000;}
    unsigned vortexScaleMilli() const {return (vortex_scale_+rancor_scale_)/1000;}
    unsigned rancorChargeMilli() const {return rancor_scale_/1000;}
    int32_t vortexChargeX() const;
    int32_t vortexChargeY() const;
    unsigned armorStacks() const {return armor_stacks_;}
    unsigned abilityCharges() const {return player.mods[M_X_DISPLACEMENT]?void_charges_:!player.ability_delay;}
    bool hasHyperboost() const {return hyperboost_ticks_||(player.mods[M_X_ATARAXIA]&&banked_upgrades>=20);}
    unsigned bodyPowerPercent() const;
    unsigned weaponRateMilli() const; // Exact nominal shots/s in thousandths.
    bool bankUpgrade();
    bool reopenUpgrade();
    int worldWidth() const;
    int worldHeight() const;
    bool screenWrappingEnabled() const;
    bool canTakeMod(unsigned id) const;
    bool takeMod(unsigned id);
    bool equipWeapon(unsigned id);
    bool equipBody(unsigned id);
    bool equipShield(unsigned id);
    void recalculate();
    unsigned enemyCount() const;
    bool enemySeen(unsigned id) const {return id<ENEMY_COUNT&&(id<64?((run_stats.seen_enemies>>(id&63))&1):((run_stats.seen_enemies_hi>>(id&63))&1));}
    unsigned pendingWaveSpawns() const;
    unsigned bulletCount() const;
    unsigned constructCount() const;
    unsigned constructLimit(unsigned kind) const;
    unsigned combinedConstructLimits() const;
    bool advancedConstructActive(unsigned kind) const;
    unsigned bastionResistancePercent() const;
    void damageConstruct(unsigned index,int damage);
    uint32_t checksum() const; // Deterministic replay diagnostic.
    const char *waveName() const;
    // Internal spawns are public for platform-independent acceptance tests.
    int spawnEnemy(unsigned type,int x,int y,unsigned elite=0);
    void giveExperience(uint32_t amount);
    void damagePlayer(int amount,bool self_damage=false,int attacker_angle=-1,bool continuous=false);

private:
    uint16_t cosmic_clock_,train_clock_,black_hole_clock_;
    uint8_t daily_boss_step_,daily_damage_redirect_;
    void updateCosmicObjects();
    bool updateDailyWaves();
    void updateDaily();
    void damageDailyEgg(unsigned damage);
    int spawnCosmic(unsigned kind,int x,int y,uint8_t angle,unsigned delay=0);
    GameInput previous_;
    uint32_t random_,accumulator_;
    uint16_t enemy_serial_;
    WaveSpawnQueue wave_queue_[SOURCE_MAX_SPECS];
    uint8_t wave_queue_count_,wave_name_kind_;
    uint16_t construct_clock_,kill_buff_,calibration_,research_bonus_;
    uint16_t assembly_clock_[C_KIND_COUNT],fire_fraction_,viper_clock_;
    uint8_t normal_hit_grace_;
    uint32_t regen_fraction_;
    uint8_t burst_left_,burst_delay_,rupture_depth_,dash_ticks_;
    uint8_t banished_[MOD_COUNT],revelation_,transmog_,extra_ranks_,last_stand_used_;
    uint16_t untouched_,stored_damage_;
    uint16_t challenger_pending_,challenger_spawned_,challenger_delay_;
    uint16_t source_armor_ticks_,source_medi_ticks_,source_medi_fraction_;
    int16_t challenger_source_;
    uint8_t offer_kind_,gear_choices_,gear_owned_,wave_pattern_;
    uint16_t hazard_clock_,hyperboost_ticks_,shield_charge_,shield_linger_;
    uint32_t charged_scale_,siphon_fraction_;
    uint32_t vortex_scale_,vortex_rate_,vortex_growth_fraction_,rancor_scale_;
    uint32_t thermal_x_scale_,thermal_y_scale_;
    uint16_t vortex_damage_fraction_,vortex_cost_fraction_;
    uint16_t siphon_clock_,halo_fraction_;
    uint8_t outrage_clock_;
    bool forced_empowered_;
    bool construct_deploy_requested_;
    uint16_t fleet_promotion_clock_;
    uint16_t discharge_clock_,discharge_self_fraction_,discharge_gemini_fraction_;
    uint16_t innate_charge_fraction_,innate_cost_fraction_;
    bool firing_latch_,auto_fire_previous_;
    uint8_t void_charges_,aegis_clock_,armor_stacks_,emergency_clock_;
    uint16_t armor_timers_[50],armor_fraction_;
    uint32_t essence_,essence_fraction_,shield_regen_fraction_,purification_fraction_;
    int16_t grid_[256],grid_next_[MAX_ENEMIES];
    int distance2(int32_t ax,int32_t ay,int32_t bx,int32_t by) const;
    bool sweptHit(int32_t ox,int32_t oy,int32_t nx,int32_t ny,
        int32_t tx,int32_t ty,int radius) const;
    bool lineHit(int32_t ox,int32_t oy,int32_t nx,int32_t ny,int32_t tx,int32_t ty,unsigned radius) const;
    int32_t playerDestination(int32_t coordinate,bool horizontal) const;
    void bonusLevels(unsigned count);
    void preventDeath();
    void triggerLastStand();
    void queueChallenger();
    void updateChallenger();
    void rewardChallenger(const Enemy &enemy);
    uint32_t random();
    unsigned range(unsigned max);
    void beginUpgrade();
    void generateOffers();
    void updatePlayer(const GameInput &input);
    void updateEnemies();
    bool updateRegularEnemy(Enemy &enemy);
    int regularShot(Enemy &enemy,uint8_t angle,int source_speed_q8,int damage,unsigned kind=SHOT_ENEMY,unsigned life=120,unsigned radius=2);
    void followEnemyLeader(Enemy &enemy);
    void updateBullets();
    void destroyHostileBullet(unsigned index);
    int eclipseShield(unsigned owner) const;
    bool targetObstructedByEclipse(int target) const;
    void triggerEclipseShield(unsigned part,bool broken,bool contact=false);
    void applyMyrmidonField(Bullet &field);
    int spawnMyrmidonField(int32_t x,int32_t y,unsigned scale_q12,int damage,unsigned warning_source_frames,bool hostile);
    void detonateMyrmidon(const Bullet &projectile);
    void updatePickups();
    void updateConstructs();
    void attackSwords(int damage,unsigned reach,uint8_t angle,bool construct_origin);
    void updateAdvancedConstruct(unsigned index);
    void updateSwordWielder(unsigned index,int target);
    void updateInterceptorWeapon(unsigned index,int target);
    bool interceptAdvancedProjectile(unsigned bullet,int32_t ox,int32_t oy);
    void releaseBastionEnergy(unsigned actualHullDamage);
    void updatePersistentEffects();
    void triggerBurnout(const Construct &construct,int damage,int radius);
    void recordMaelstromDamage(unsigned amount);
    void updateShields();
    void updateWaves();
    void loadWaveSpawns(const SourceSpawnSpec *specs,unsigned count);
    void updateWaveSpawns();
    void dynamicWaveSpawns();
    unsigned sourceEnemyRarity(SourceEnemy type);
    int spawnSourceEnemy(SourceEnemy type,int x,int y,int rarity=SOURCE_UNDEFINED);
    void fireWeapon(unsigned projectile_override=0,bool construct_origin=false);
    void updateBoss(Enemy &enemy);
    void fireBoss(Enemy &enemy);
    int emitBullet(int32_t x,int32_t y,uint8_t angle,int speed,int damage,
        unsigned kind,unsigned life,unsigned owner=0,unsigned radius=2);
    void emitEffect(int32_t x,int32_t y,unsigned kind,unsigned radius,
        unsigned life=12,unsigned color=0,int32_t x2=0,int32_t y2=0);
    void emitPickup(int32_t x,int32_t y,unsigned kind,unsigned value);
    void blast(int32_t x,int32_t y,int radius,int damage,int excluded=-1);
    void hitEnemy(unsigned index,int damage,bool burn=false,bool area=false);
    int healEnemy(unsigned index,int amount);
    void triggerScorchingWake();
    void updateScorchingWake(Bullet &bullet);
    unsigned bossDamagePercent(const Enemy &enemy,bool area) const;
    bool hitBossPart(unsigned index,int damage,bool burn=false,bool area=false);
    void updateBossParts(unsigned owner);
    bool sweptBossPart(const BossPart &part,int32_t ox,int32_t oy,int32_t nx,int32_t ny,unsigned padding) const;
    void creditDamage(unsigned actual);
    void awardEnemyPart(unsigned owner,unsigned numerator,unsigned denominator,bool count_kill,int32_t x=-1,int32_t y=-1);
    unsigned regenerationPercent() const;
    unsigned weaponPeriodQ8(int rate) const;
    unsigned weaponPeriodQ16(int rate) const;
    int weaponSpreadStep() const;
    unsigned crashContactPercent() const;
    unsigned shieldConstructPermille() const;
    void destroyEnemy(unsigned index);
    void onEnemyDestroyedConstructEffects(const Enemy &enemy);
    void shieldBroken();
    int emitElectricalNode(int32_t x,int32_t y,uint8_t angle,unsigned length_q8,unsigned damage_q8,unsigned links,unsigned thickness);
    void triggerDischarge();
    unsigned reduceSelfDamageMilli(unsigned amount,bool shield);
    void damageShieldSelf(unsigned amount_milli);
    void damageHullSelf(unsigned amount_milli);
    void triggerTempest(int32_t x,int32_t y,unsigned power);
    void releaseVortex(unsigned projectile_override=0,bool construct_origin=false);
    unsigned storedChargeMicro() const;
    void recordRancorDamage(unsigned milli);
    void updateRancor();
    void updateVortexCharge();
    void updateVortexBullet(Bullet &bullet,unsigned released_count=1);
    void updateVortexMine(unsigned index,unsigned rate_permille,unsigned cost_multiplier=2);
    unsigned vortexChargeModifier(bool construct_origin=false) const;
    void vortexField(int32_t x,int32_t y,unsigned scale,unsigned damage_milli,uint16_t &fraction,unsigned released_count);
    void applyVortexGravity(int32_t x,int32_t y,unsigned scale,unsigned released_count);
    void fireHomingStrike(uint8_t angle,int projectiles,int speed,int damage,int life,int radius);
    void updateHomingStrike(Bullet &bullet);
    void hostileBlast(int32_t x,int32_t y,unsigned radius,int damage);
    int nearestTarget(int32_t x,int32_t y,int radius=10000) const;
    bool targetActive(int target) const;
    int32_t targetX(int target) const;
    int32_t targetY(int target) const;
    unsigned targetRadius(int target) const;
    int targetHull(int target) const;
    bool targetInRadius(int target,int32_t x,int32_t y,unsigned radius) const;
    bool targetSwept(int target,int32_t ox,int32_t oy,int32_t nx,int32_t ny,unsigned padding) const;
    unsigned targetOwner(int target) const;
    void damageTarget(int target,int damage,bool burn=false,bool area=false);
    int nearestEnemy(int32_t x,int32_t y,int radius=400) const;
    int shieldDistance2(int32_t x,int32_t y) const;
    int spawnConstruct(unsigned kind);
    int constructPower(unsigned kind) const;
    void buildGrid();
    void checkLevel();
};
// Account progression is separate from the resumable run. The application owns
// atomic, versioned persistence; award() ignores duplicate completed snapshots.
struct Career {
    uint32_t runs,total_score,total_kills,total_ticks,best_score,highest_hit;
    uint64_t total_distance_q8;
    uint64_t seen_enemies,seen_enemies_hi;
    uint32_t enemy_kills[ENEMY_COUNT],unlock_points_milli,last_seed,last_frame;
    uint16_t best_wave,rank,final_boss_kills,flags;
    void clear();
    bool valid() const;
    unsigned award(const Game &game);
    unsigned nextRankPoints() const;
    bool enemySeen(unsigned id) const {return id<ENEMY_COUNT&&(id<64?((seen_enemies>>(id&63))&1):((seen_enemies_hi>>(id&63))&1));}
    bool modeUnlocked(unsigned flag) const;
    bool offerUnlocked(unsigned kind,unsigned id) const;
    static const char *unlockKey(unsigned rank);
};
unsigned offerUnlockRank(unsigned kind,unsigned id);
unsigned modeUnlockRank(unsigned flag); // 255 means a final boss victory is required.
} // namespace nova
#endif
