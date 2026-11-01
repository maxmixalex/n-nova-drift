#ifndef NOVA_GAME_H
#define NOVA_GAME_H
#include <stdint.h>
#include "content.h"

namespace nova {
enum { WIDTH=320, HEIGHT=240, TICK_RATE=30, FP=256,
    MAX_ENEMIES=64, FRIENDLY_BULLETS=192, HOSTILE_BULLETS=128,
    MAX_BULLETS=FRIENDLY_BULLETS+HOSTILE_BULLETS, MAX_PICKUPS=72,
    MAX_EFFECTS=32, MAX_CONSTRUCTS=16, MAX_SEGMENTS=24, OFFER_COUNT=3 };
// Coordinates and velocities are signed Q8; angles are 0..255 clockwise,
// with 0 pointing right. No floating point or heap allocation in the core.
inline int pixel(int32_t q8) { return q8 / FP; }
int dirX(uint8_t angle); // Q8 cosine
int dirY(uint8_t angle); // Q8 sine
uint8_t direction(int32_t dx, int32_t dy);
enum State { Title, Playing, Upgrade, Pause, GameOver };
enum BulletKind { SHOT_BLASTER, SHOT_SPLIT, SHOT_FLAK, SHOT_RAIL,
    SHOT_GRENADE, SHOT_MISSILE, SHOT_VORTEX, SHOT_TORRENT,
    SHOT_DART, SHOT_SPARK, SHOT_ENEMY, SHOT_SHRAPNEL, SHOT_BLADE };
enum ConstructKind { C_DRONE, C_DEFENSE_DRONE, C_MINE, C_TURRET,
    C_ALLY, C_INTERCEPTOR, C_GUARDIAN, C_SWARM, C_BLADE };
enum EffectKind { FX_BURST, FX_RING, FX_BEAM, FX_SHIELD_BREAK, FX_WARP,
    FX_MUZZLE, FX_HIT, FX_SWORD, FX_SPAWN };
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
    int16_t hull,max_hull,burn_damage;
    uint16_t age,fire_delay,burn_time,stun;
    uint8_t type,angle,radius,active,elite,phase,flash,vulnerability;
};
struct Bullet {
    int32_t x,y,vx,vy;
    int16_t damage;
    uint16_t life,age,speed;
    uint8_t active,owner,kind,radius,angle,homing,pierce,blast,bounces,wraps;
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
    uint16_t age,fire_delay,life;
    uint8_t active,kind,angle,radius,charges,promoted;
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
    Bullet bullets[MAX_BULLETS];
    Pickup pickups[MAX_PICKUPS];
    Effect effects[MAX_EFFECTS];
    Construct constructs[MAX_CONSTRUCTS];
    int32_t segment_x[MAX_SEGMENTS],segment_y[MAX_SEGMENTS];
    uint8_t segment_count;
    Offer offers[OFFER_COUNT];
    uint8_t selected_offer;
    uint16_t level,wave,rerolls,banked_upgrades;
    uint32_t score,kills,xp,xp_next,frame,seed;
    uint16_t next_wave,wave_banner,damage_flash;
    uint32_t dropped_bullets,dropped_enemies; // Capacity diagnostics only.
    int16_t boss_index;
    bool auto_fire,wild_mode;
    int32_t sanctuary_x,sanctuary_y;

    Game() = default;
    void resetInput();
    bool validState() const;
    void start(uint32_t run_seed);
    void tick(const GameInput &input); // Exactly one 1/30s simulation step.
    void advance(uint32_t elapsed_ms,const GameInput &input); // bounded catch-up
    bool chooseOffer(unsigned index);
    bool rerollOffers();
    bool bankUpgrade();
    bool reopenUpgrade();
    int worldWidth() const;
    int worldHeight() const;
    bool canTakeMod(unsigned id) const;
    bool takeMod(unsigned id);
    bool equipWeapon(unsigned id);
    bool equipBody(unsigned id);
    bool equipShield(unsigned id);
    void recalculate();
    unsigned enemyCount() const;
    unsigned bulletCount() const;
    unsigned constructCount() const;
    uint32_t checksum() const; // Deterministic replay diagnostic.
    const char *waveName() const;
    // Internal spawns are public for platform-independent acceptance tests.
    int spawnEnemy(unsigned type,int x,int y,unsigned elite=0);
    void giveExperience(uint32_t amount);
    void damagePlayer(int amount,bool self_damage=false);

private:
    GameInput previous_;
    uint32_t random_,accumulator_;
    uint16_t construct_clock_,kill_buff_,calibration_,regen_clock_,research_bonus_;
    uint8_t burst_left_,burst_delay_,rupture_depth_,dash_ticks_;
    uint8_t banished_[MOD_COUNT],revelation_,transmog_,extra_ranks_,last_stand_used_;
    uint16_t untouched_,stored_damage_,maelstrom_;
    int16_t grid_[256],grid_next_[MAX_ENEMIES];
    int distance2(int32_t ax,int32_t ay,int32_t bx,int32_t by) const;
    bool sweptHit(int32_t ox,int32_t oy,int32_t nx,int32_t ny,
        int32_t tx,int32_t ty,int radius) const;
    void bonusLevels(unsigned count);
    void preventDeath();
    uint32_t random();
    unsigned range(unsigned max);
    void beginUpgrade();
    void generateOffers();
    void updatePlayer(const GameInput &input);
    void updateEnemies();
    void updateBullets();
    void updatePickups();
    void updateConstructs();
    void updateShields();
    void updateWaves();
    void fireWeapon();
    void fireEnemy(Enemy &enemy);
    int emitBullet(int32_t x,int32_t y,uint8_t angle,int speed,int damage,
        unsigned kind,unsigned life,unsigned owner=0,unsigned radius=2);
    void emitEffect(int32_t x,int32_t y,unsigned kind,unsigned radius,
        unsigned life=12,unsigned color=0,int32_t x2=0,int32_t y2=0);
    void emitPickup(int32_t x,int32_t y,unsigned kind,unsigned value);
    void blast(int32_t x,int32_t y,int radius,int damage,int excluded=-1);
    void hitEnemy(unsigned index,int damage,bool burn=false);
    void destroyEnemy(unsigned index);
    void shieldBroken();
    void releaseVortex();
    int nearestEnemy(int32_t x,int32_t y,int radius=400) const;
    int spawnConstruct(unsigned kind);
    int constructPower(unsigned kind) const;
    void buildGrid();
    void checkLevel();
};
} // namespace nova
#endif
