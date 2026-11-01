#pragma once
#include <stddef.h>
#include <stdint.h>
#include "nnova/pack_format.h"

namespace nova {
struct Sprite {
    const uint16_t* pixels;         // straight RGB565, row-major
    const uint8_t* alpha;           // straight alpha, 0..255
    uint16_t width, height;
    int16_t origin_x, origin_y;
    uint16_t original_width, original_height;
    uint16_t canvas_width, canvas_height; // stable dimensions across animation
};

// Keep Assets in static/global storage (roughly 2.5 MiB); no heap or STL.
// A returned Sprite is valid until close(), clear_cache(), or a later find()
// fills the bounded cache. Drawing immediately after find() is always safe.
class Assets {
public:
    bool open(const char* base_path = 0);
    void close();
    const Sprite* find(const char* name, unsigned frame = 0);
    const Sprite* find_id(unsigned sprite, unsigned frame = 0);
    void clear_cache();
    unsigned count() const { return header_.sprite_count; }
    unsigned frames(unsigned sprite) const;
    unsigned playback_fps_q8(unsigned sprite) const;
    int index_of(const char* name) const;
    const char* name(unsigned sprite) const;
    const char* error() const { return error_; }
    unsigned cache_used() const { return used_; }
    bool ready() const { return ready_; }
private:
    pack::CommonHeader header_;
    pack::SpriteRecord sprites_[pack::MaxSprites];
    pack::FrameRecord frames_[pack::MaxFrames];
    Sprite cached_[pack::MaxFrames];
    alignas(4) uint8_t arena_[pack::CacheBytes];
    uint32_t used_;
    const char* error_;
    bool ready_;
};
}
