#ifndef NOVA_ADVANCED_CONSTRUCTS_H
#define NOVA_ADVANCED_CONSTRUCTS_H
#include <stdint.h>
namespace nova {
// Fixed replacement for the source SwordObject controller and per-attack hitList.
// Bastion shares the same construct pool; its shield HP is mirrored in the HUD.
struct InheritedWeaponState {
 uint32_t thermal_x,thermal_y;
 uint16_t fire_fraction,stock_clock,magazine,charge_ticks,calibration,innate_fraction;
 uint16_t charge_clock,release_delay;
 uint8_t burst_left,burst_delay,weapon,charging;
 uint16_t damage_fraction,reserved;
};
static_assert(sizeof(InheritedWeaponState)==32,"Inherited weapon state reuses the Sword target-set storage");
struct AdvancedConstructState {
 union {uint64_t hit_mask[4];InheritedWeaponState weapon;};
 int32_t anchor_x,anchor_y,damage;
 uint16_t duration,clock;
 uint8_t state,side,heading,temporary;
 uint8_t wielder; //0=player; construct slot+1 for a registered Interceptor owner.
};
}
#endif
