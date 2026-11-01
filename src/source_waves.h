#ifndef NOVA_SOURCE_WAVES_H
#define NOVA_SOURCE_WAVES_H
#include <stdint.h>
namespace nova {
// Original spawnInfo keys, deliberately independent of historical EnemyId names.
enum SourceEnemy {
    SE_NONE=-1, SE_ASTEROID, SE_ASTEROID_MEGA, SE_INTERCEPTOR, SE_WARDEN,
    SE_GYROGUN, SE_SCATTER_DRONE, SE_SWARMLING, SE_SWARMLING_GREEN, SE_HOST,
    SE_BEAMCASTER, SE_MYRMIDON, SE_PULSAR, SE_SAUCER, SE_BOLT_THROWER,
    SE_HAMMERHEAD, SE_SNAPPER, SE_SERPENT, SE_VANGUARD, SE_HIVE, SE_CRAB_NEST,
    SE_TRACER, SE_SHREDDER_MINE, SE_MINELAYER, SE_ECLIPSE, SE_CONSTRUCTOR,
    SE_CHERUB, SE_BULWARK, SE_JUGGERNAUT, SE_CELESTIAL, SE_SPITFIRE,
    SE_CERATOTITAN, SE_STATION_OMEGA, SE_DREADNOUGHT, SE_SCION, SE_DWELLER,
    SE_WARBRINGER, SE_CRUCIBLE, SE_SERAPH, SE_GLAUCUS, SE_DWELLER_DRONE,
    SE_DWELLER_GUARD, SE_DWELLER_WARRIOR, SE_NULL,
    SOURCE_ENEMY_COUNT
};
enum { SOURCE_UNDEFINED = (-2147483647-1), SOURCE_MAX_SPECS=8 };
// One native spawnEnemies call. Counts/timers/enums are integers; fields marked
// q8 retain the original numeric value times 256. Timers retain ORIGINAL 60 Hz,
// distances ORIGINAL 1920x1080 units. Consumer converts to 30 Hz / 320x240.
// SOURCE_UNDEFINED preserves omitted arguments; never confuse it with zero.
// Native entry: 0/default=automatic, 1=warp gate, 2=emerge from space.
// Native spawn: 0/default=standard, 2=pincer, 3=fleet, 4=V formation,
// 5=arc, 6=spiral, 7=box, 8=fixed position. These are NOT screen-edge IDs.
// Native behavior: 3=flank, 4=orbit, 5=support, 6=trail.
// repeating is an additional spawn count, separated by repeat_frequency_ticks.
// repeat_formation repeats a follower formation; repeat_size_q8 scales each
// successive formation. added_delay_ticks delays this call's spawn, not the wave.
// fixed_direction is the original direction selector (including opening-wave
// values 6/7), not an angle in degrees.
// spawn_parameter_q8 is formation-specific: arc angle/distance, or normalized
// fixed-position offset; never globally interpret it as an angle.
struct SourceSpawnSpec {
    int32_t enemy,count,entry,spawn,spawn_parameter_q8,rarity,repeating;
    int32_t repeat_frequency_ticks,behavior,follower,follower_count;
    int32_t behavior_parameter1_q8,behavior_parameter2_q8,fixed_direction;
    int32_t action_delay_ticks,break_delay_ticks,add_enemy,add_frequency_ticks;
    int32_t support,support_count,added_delay_ticks,iterate_spawn_parameter;
    int32_t repeat_formation,repeat_size_q8,size_q8;
    int32_t follower_min_rarity,follower_max_rarity,damage_q8,treasure_q8;
    int32_t challenger,is_boss;
};
struct SourceEnemyInfo {
    uint16_t min_wave,elite_wave,champion_wave;
    uint16_t weight_q8,quantity_low_q8,quantity_mean_q8,quantity_high_q8;
    uint16_t quantity_min,quantity_max; // 65535 means native infinity.
};
const char* sourceEnemyName(SourceEnemy enemy);
const char* sourceCuratedName(unsigned pattern);
// setWaveDelay result at waveMult=1, in original 60 Hz frames.
unsigned sourceEnemyDelayFrames(SourceEnemy enemy);
const SourceEnemyInfo& sourceEnemyInfo(SourceEnemy enemy,uint16_t flags=0);
// Boss scheduling must run first. 0 means this is not a curated wave.
unsigned sourceCuratedWave(unsigned wave,uint16_t flags,uint32_t& rng,
                           SourceSpawnSpec* out,unsigned capacity,bool frequent_curated=false,bool turbo=false);
unsigned sourceCuratedPattern(unsigned pattern,unsigned wave,uint16_t flags,
                              uint32_t& rng,SourceSpawnSpec* out,unsigned capacity);
// Native dynamic quantity before formation-specific rounding. Three independent
// uniform draws use [0,65535]; the first two form the original triangular draw.
// The returned quantity is Q16, preserving fractions until a recipe is formed.
uint32_t sourceDynamicAmountQ16(SourceEnemy enemy,unsigned wave,uint16_t flags,
                               unsigned first,unsigned second,unsigned variance);
// Native spawnType constructor order. A zero weight (box) is retained for
// explicit recipes, but is never selected by the weighted dynamic scheduler.
enum SourceDynamicFormation {
    SD_STANDARD,SD_ARC,SD_CENTERED,SD_FLEET,SD_PINCER,SD_SPIRAL,
    SD_V_FORMATION,SD_V_PINCER,SD_DOUBLE_SPIRAL,SD_TRIPLE_SPIRAL,SD_BOX,
    SD_CASCADE_PINCER,SD_CASCADE_OPPOSITE,SD_CASCADE_FLEET,
    SD_CASCADE_ADJACENT,SD_CASCADE_EDGES,SD_ORBIT,SD_FLANK,SD_RANK,
    SD_TRAIL,SD_PROTECT,SD_SUPPORT,SOURCE_DYNAMIC_FORMATIONS
};
unsigned sourceDynamicWeight(SourceEnemy enemy,unsigned formation,bool paired=false);
unsigned sourceDynamicPattern(unsigned formation,SourceEnemy enemy,unsigned wave,
                              uint16_t flags,uint32_t amount_q16,uint32_t& rng,
                              SourceSpawnSpec* out,unsigned capacity);
unsigned sourceDynamicRecipe(SourceEnemy enemy,unsigned wave,uint16_t flags,
                             uint32_t amount_q16,uint32_t& rng,
                             SourceSpawnSpec* out,unsigned capacity);
}
#endif
