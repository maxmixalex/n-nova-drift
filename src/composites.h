#pragma once
#include "assets.h"
#include "draw.h"
#include "game.h"

namespace nova {
// Visual assemblies only: the simulation keeps one collider per enemy.
class Composites {
public:
    void initialize(Assets* assets);
    bool draw(Canvas& canvas, const Enemy& enemy, int x, int y,
              int displayDiameter, uint32_t tick);
private:
    enum { PartCapacity=32, MirrorCapacity=11, MirrorEdge=84 };
    struct Mirror {
        Sprite sprite;
        uint16_t colors[MirrorEdge*MirrorEdge];
        uint8_t alpha[MirrorEdge*MirrorEdge];
        bool ready;
    };
    Assets* assets_;
    int ids_[PartCapacity];
    int8_t mirror_index_[PartCapacity];
    int8_t lit_index_[PartCapacity];
    Mirror mirrors_[MirrorCapacity];
    bool paint(Canvas&, unsigned part, int x, int y, int diameter,
               int localX, int localY, int extent, uint8_t heading,
               int localAngle=0, uint16_t tint=65535, unsigned opacity=255,
               bool mirrored=false, bool additive=false);
};
}
