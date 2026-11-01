#pragma once
#include "assets.h"
#include "draw.h"
#include "game.h"

namespace nova {
// Original component art attached to the simulation's live destructible parts.
class Composites {
public:
    void initialize(Assets* assets,Game* scratch=0);
    bool core(Canvas&,const Enemy&,int x,int y,int projection,uint32_t tick);
    bool part(Canvas&,const BossPart&,const Enemy&,int x,int y,int projection,uint32_t tick);
    bool preview(Canvas&,unsigned enemy,int x,int y,int diameter,uint32_t tick=0);
    static bool foreground(unsigned kind);
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
    struct Preview {Enemy enemy;BossPart parts[32];uint8_t count;int16_t center_x,center_y,extent;};
    Preview previews_[14];
    uint8_t preview_count_=0;
    void source(Canvas&,const char* sprite,int x,int y,int sourceScale,uint8_t angle,bool mirror,uint16_t tint=65535,unsigned opacity=255,bool additive=false,unsigned lighting=0,unsigned frame=0);
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
