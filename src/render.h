#pragma once
#include "assets.h"
#include "draw.h"
#include "game.h"
#include "composites.h"

namespace nova {
class Renderer {
public:
    void initialize(Assets* assets);
    void bind(uint16_t* framebuffer) { canvas.bind(framebuffer); }
    void background(uint32_t ticks, bool animated = true);
    void world(const Game& game, uint32_t ticks, bool effects);
    void titleArt(uint32_t ticks);
    void icon(const Offer& offer, int x, int y, int size=28);
    void bodyIcon(unsigned body, int x, int y, int size=28);
    void pointer(int x,int y,bool pressed);
    Canvas canvas;
private:
    Assets* assets_;
    Composites composites_;
    int bodies_[BODY_COUNT],enemies_[ENEMY_COUNT];
    int bullets_[13],constructs_[9],pickups_[4];
    int weapon_icons_[WEAPON_COUNT],shield_icons_[SHIELD_COUNT],mod_icons_[MOD_COUNT];
    int logo_,planet_,nebula_;
    alignas(4) uint16_t background_[ScreenW*ScreenH];
    bool sprite(int id,unsigned frame,int x,int y,int size=0,uint8_t angle=0,
                uint16_t tint=65535,unsigned opacity=255,bool additive=false);
    void fallbackShip(int x,int y,uint8_t angle,int size,uint16_t color);
};
}
