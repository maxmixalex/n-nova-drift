#pragma once
#include "assets.h"
#include "draw.h"
#include "game.h"
#include "composites.h"
#include "skins.h"

namespace nova {
class Locale;
class Font;
struct DisplayOptions { uint8_t background_detail;bool shake,flash,shockwaves,damage_numbers,healing_numbers,self_damage_numbers,visibility,speedrun; };
class Renderer {
public:
    void initialize(Assets* assets,Game* scratch=0);
    void bind(uint16_t* framebuffer) { canvas.bind(framebuffer); }
    void background(uint32_t ticks, bool animated = true);
    void menuBackground(uint32_t ticks,bool celestial=true);
    void world(const Game& game, uint32_t ticks, bool effects,bool asteroids=true,bool hullbars=true,const char* upgrade_key="9",bool upgrade_prompt=true);
    void setText(const Locale* locale,const Font* font) { locale_=locale;font_=font; }
    void titleArt(uint32_t ticks);
    void configure(DisplayOptions options,const Skins* skins){options_=options;skins_=skins;}
    bool ui(const char* name,int x,int y,int size,uint16_t tint=65535,unsigned opacity=255);
    bool uiStrip(const char* name,int x,int y,int width,int height,unsigned opacity=255);
    void selectIcon(int x,int y,int size,bool selected=true);
    void icon(const Offer& offer, int x, int y, int size=28);
    void bodyIcon(unsigned body, int x, int y, int size=28);
    void enemyIcon(unsigned enemy,int x,int y,int size=28);
    void pointer(int x,int y,bool pressed);
    Canvas canvas;
private:
    Assets* assets_;
    const Locale* locale_;
    const Font* font_;
    Composites composites_;
    int bodies_[BODY_COUNT],body_icons_[BODY_COUNT],enemies_[ENEMY_COUNT],asteroids_[4][4];
    uint8_t asteroid_visible_[4][4];
    int bullets_[SHOT_KIND_COUNT],constructs_[C_KIND_COUNT],pickups_[4],source_powerups_[3],enemy_hard_shield_,maelstrom_[2],fire_bursts_[SHIELD_COUNT],wake_sprites_[7],last_stand_sprites_[SHIELD_COUNT],last_stand_tempests_[SHIELD_COUNT],bastion_wave_,swarm_beams_[2],swarm_charges_[2],overseer_beam_,myrmidon_blast_,pulsar_burst_,pulsar_charge_;
    int weapon_icons_[WEAPON_COUNT],shield_icons_[SHIELD_COUNT],mod_icons_[MOD_COUNT];
    int logo_,planet_,nebula_,rings_,moon_,menu_cursor_,cursor_;
    int cosmic_sprites_[COSMIC_KIND_COUNT],cosmic_warning_,cosmic_egg_;
    alignas(4) uint16_t background_[ScreenW*ScreenH];
    alignas(4) uint16_t menu_background_[2][ScreenW*ScreenH];
    bool menu_valid_[2];DisplayOptions options_;const Skins* skins_;
    bool skin(unsigned kind,int x,int y,int size,uint8_t angle,unsigned opacity=255);
    void shake(int dx,int dy);
    bool sprite(int id,unsigned frame,int x,int y,int size=0,uint8_t angle=0,
                uint16_t tint=65535,unsigned opacity=255,bool additive=false,unsigned lighting=0);
    void fallbackShip(int x,int y,uint8_t angle,int size,uint16_t color);
};
}
