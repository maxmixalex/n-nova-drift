#ifndef NOVA_PERSISTENT_EFFECTS_H
#define NOVA_PERSISTENT_EFFECTS_H
#include <stdint.h>
namespace nova {
enum { MAX_PERSISTENT_FIELDS=16 };
// Fixed-pool source fire bursts. Age is in original 60 Hz frames; each core
// tick advances two. The target hit set belongs to this burst, not its source.
struct PersistentField {
 int32_t x,y,damage;
 uint64_t hit_mask[4],pushed_mask;
 uint16_t owner_generation[64];
 uint16_t base_radius,radius;
 uint8_t source_age,opacity,active,angle,burn,reserved[3];
};
struct MaelstromState {
 uint32_t charge_q16,radius_q8;
 uint32_t large_scale_q12,small_scale_q12;
 uint16_t angle_q8,small_angle_q8,rot_q8,small_rot_q8;
 uint16_t charge_remainder;
 uint8_t active,reserved;
};
}
#endif
