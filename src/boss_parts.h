#ifndef NOVA_BOSS_PARTS_H
#define NOVA_BOSS_PARTS_H
#include <stdint.h>
namespace nova {
enum { MAX_BOSS_PARTS=192 };
enum BossPartKind {
 BP_DREAD_WING, BP_DREAD_GUN_LARGE, BP_DREAD_GUN_SMALL,
 BP_WAR_CHASSIS, BP_WAR_WING, BP_WAR_THRUSTER, BP_WAR_BROADSIDE, BP_WAR_LASER,
 BP_CERA_THORAX, BP_CERA_PINCER, BP_CERA_ELYTRON, BP_CERA_WING,
 BP_GLAUCUS_SEGMENT, BP_GLAUCUS_EYE, BP_STAR_SEGMENT, BP_STAR_EYE, BP_STAR_ABDOMEN,
 BP_STATION_NODE, BP_STATION_WALL,
 BP_BULWARK_BARRIER, BP_BULWARK_CORE, BP_BULWARK_BODY2, BP_BULWARK_BODY3,
 BP_BULWARK_ARM, BP_BULWARK_TAIL, BP_BULWARK_THRUSTER, BP_BULWARK_THRUSTER_LARGE,
 BP_WORM_HEAD, BP_WORM_BODY, BP_BOLT_JAW_LEFT, BP_BOLT_JAW_RIGHT,
 BP_BOLT_JAW_LEFT_ELITE, BP_BOLT_JAW_RIGHT_ELITE,
 BP_SNAPPER_JAW_LEFT, BP_SNAPPER_JAW_RIGHT, BP_SNAPPER_BASE, BP_SNAPPER_SEGMENT,
 BP_ECLIPSE_CHASSIS, BP_ECLIPSE_PANEL, BP_ECLIPSE_SHIELD, BP_CRUCIBLE_CORNER, BP_CRUCIBLE_EDGE, BP_COUNT
};
// A separate fixed pool keeps detached components out of the ordinary-enemy
// budget. Coordinates and attachment offsets are Q8. There are no pointers,
// so parts remain portable in the versioned resumable-run snapshot.
struct BossPart {
 int32_t x,y,hull,max_hull;
 int16_t local_x,local_y;
 uint16_t timer,source_scale; // Q16 source pixels to world; occupies former padding.
 uint8_t owner,kind,radius,angle,active,slot,mirror,flash;
 uint16_t burn_time,wake_time;int16_t burn_damage; // Per-part ignition, including invisible controllers.
};
struct BossPartArt {
 const char* sprite;
 uint16_t source_width,source_height,bounds_width,bounds_height;
 int16_t center_x,center_y; // source collision-box center minus source pivot
};
const BossPartArt& bossPartArt(unsigned kind);
const char* bossPartSprite(unsigned kind);
}
#endif
