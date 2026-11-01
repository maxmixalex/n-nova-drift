#include "render.h"
#include "font.h"
#include "localization.h"
#include "visual_feedback.h"

namespace nova {
static int projection = 256;
static int scenePixel(int32_t q8) { return q8 * projection / 65536; }
static int sceneRadius(int radius) { int r=radius*projection/256; return r<1?1:r; }
static const uint16_t White=rgb(222,242,255);
static const uint16_t Cyan=rgb(78,206,245);
static const uint16_t Purple=rgb(167,109,245);
static const uint16_t Green=rgb(102,231,166);
static const uint16_t Orange=rgb(255,174,92);
static const uint16_t Red=rgb(255,104,130);

void Renderer::initialize(Assets* assets,Game* scratch) {
    assets_=assets;menu_valid_[0]=menu_valid_[1]=false;skins_=nullptr;
    options_={3,true,true,true,false,false,false,false,false};
    composites_.initialize(assets_,scratch);
    for(unsigned i=0;i<BODY_COUNT;++i)bodies_[i]=assets_->index_of(bodies[i].sprite);
    const char* bodyIcons[BODY_COUNT]={"sHexDefaultBody","sHexAssault","sHexHullbreaker","sHexStealth","sHexFirefly","sHexSentinel","sHexEngineer","sHexResearch","sHexBattery","sHexArchitect","sHexViper","sHexCarrier","sHexCourser","sHexLeviathan"};
    for(unsigned i=0;i<BODY_COUNT;++i)body_icons_[i]=assets_->index_of(bodyIcons[i]);
    for(unsigned i=0;i<ENEMY_COUNT;++i)enemies_[i]=assets_->index_of(enemy_types[i].sprite);
    const char* asteroidNames[4][4]={
        {"sAsteroidSmall1","sAsteroidSmall2","sAsteroidSmall3","sAsteroidSmall4"},
        {"sAsteroidMedium1","sAsteroidMedium2","sAsteroidMedium3","sAsteroidMedium4"},
        {"sAsteroidLarge","sAsteroidLarge2","sAsteroidLarge3","sAsteroidLarge4"},
        {"sAsteroidMega","sAsteroidMega","sAsteroidMega","sAsteroidMega"}};
    for(unsigned tier=0;tier<4;++tier)for(unsigned i=0;i<4;++i){
        asteroids_[tier][i]=assets_->index_of(asteroidNames[tier][i]);asteroid_visible_[tier][i]=0;
        const Sprite *rock=asteroids_[tier][i]>=0?assets_->find_id(unsigned(asteroids_[tier][i]),0):nullptr;
        if(rock){int left=rock->width,top=rock->height,right=0,bottom=0;
            // Ignore the source's almost transparent glow when fitting the
            // solid rock. Cache this once, never scan alpha during combat.
            for(unsigned y=0;y<rock->height;++y)for(unsigned x=0;x<rock->width;++x)
                if(!rock->alpha||rock->alpha[y*rock->width+x]>=100){if((int)x<left)left=x;if((int)x>right)right=x;if((int)y<top)top=y;if((int)y>bottom)bottom=y;}
            int width=right-left+1,height=bottom-top+1;
            asteroid_visible_[tier][i]=(uint8_t)(width>0&&height>0?(width>height?width:height):(rock->width>rock->height?rock->width:rock->height));
        }
    }
    for(unsigned i=0;i<WEAPON_COUNT;++i)weapon_icons_[i]=assets_->index_of(weapons[i].sprite);
    for(unsigned i=0;i<SHIELD_COUNT;++i)shield_icons_[i]=assets_->index_of(shields[i].sprite);
    for(unsigned i=0;i<MOD_COUNT;++i) {
        Offer offer={OFFER_MOD,uint8_t(i)};
        mod_icons_[i]=assets_->index_of(offerSprite(offer));
    }
    const char* bulletNames[]={"sDefaultWeapon","sBulletBlue","sFlakBlue","sRailgunBlue",
        "sGrenade","sSalvoBlue","sVortexCoreBlue","sTorrentBlue","sDartBlue",
        "sBulletBlue","sBulletRed","sRailShardBlue","sBlade","sBulletBlue","sOrbSeraph","sSeraphShot",
        "sCastHardShieldInner","sPlasmaShot","sPulseShotgun","sSineShot","sSineShotLarge",
        "sRazerShot","sGaussProjectile","sRazerShot","sTracerShot","sTracerBall","sDeadlyWakeEnemyRed",
        "sOrbJuggernaut","sJuggMissile","sDwellerGuardShot","sDwellerChitinMissile","sDwellerWarriorArc",
        "sInterceptorMissile","sPulseShotgun","sStationDroneShot","sWardenVortex",
        "sJuggernautShot","sJuggMissileHoming","sPulseShotgunHot","sMineDroneShot","sSnakeSpine","sChainBody","sPulsarMine","sEclipseShot","sSpike","sSpike","sTripleSpike","sPulseShotgunHot","sEclipseShot","sEclipseLanceReflect","sCelestialStrike","sCelestialStrike2","sCelestialBurstWarning",
        "sLanceRed","sEclipseShot","sEclipseShot","sEclipseCharge","sEclipseVortex",
        "sEnemyDroneShot","sSeraphShot","sMyrmidonBomb","sWarningCirclePink","sWarningCirclePink",
        "sMyrmidonAutoGun","sEclipseLanceReflect","sEclipseWave","sEclipseShot"};
    static_assert(sizeof(bulletNames)/sizeof(bulletNames[0])==SHOT_KIND_COUNT,"Every projectile needs its original sprite");
    for(unsigned i=0;i<SHOT_KIND_COUNT;++i)bullets_[i]=assets_->index_of(bulletNames[i]);
    const char* wakeNames[7]={"sCelestialSurgeBlue","sCelestialSurgeGreen","sCelestialSurgePink","sCelestialSurgePurple","sCelestialSurgeRed","sCelestialSurgeWhite","sCelestialSurgeYellow"};
    for(unsigned i=0;i<7;++i)wake_sprites_[i]=assets_->index_of(wakeNames[i]);
    enemy_hard_shield_=assets_->index_of("sCherubShield");
    maelstrom_[0]=assets_->index_of("sMaelstromLarge");maelstrom_[1]=assets_->index_of("sMaelstromSmall");
    // Source shieldEnumToUpgrade and refreshEffects tables, mapped to our IDs.
    const char *waves[SHIELD_COUNT]={"sWaveBlue","sWaveYellow","sWaveGreen","sWaveYellow","sWavePurple","sWaveRed","sWavePink","sWaveBlue","sWaveGreen","sWaveRed"};
    for(unsigned i=0;i<SHIELD_COUNT;i++)fire_bursts_[i]=assets_->index_of(waves[i]);
    bastion_wave_=assets_->index_of("sBastionWave");
    swarm_beams_[0]=assets_->index_of("sThermalLancePurple");swarm_beams_[1]=assets_->index_of("sThermalLanceGreen");
    swarm_charges_[0]=assets_->index_of("sThermalLanceChargePurple");swarm_charges_[1]=assets_->index_of("sThermalLanceChargeGreen");overseer_beam_=assets_->index_of("sBeamOverseer");
    myrmidon_blast_=assets_->index_of("sExplosionYellow");
    pulsar_burst_=assets_->index_of("sEclipseBurstReflect");pulsar_charge_=assets_->index_of("sEclipseCharge");
    const char* explosions[SHIELD_COUNT]={"sExplosionBlue","sExplosionYellow","sExplosionGreen","sExplosionYellow","sExplosionPurple","sExplosionRed","sExplosionPink","sExplosionBlue","sExplosionGreen","sExplosionRed"};
    for(unsigned i=0;i<SHIELD_COUNT;i++)last_stand_sprites_[i]=assets_->index_of(explosions[i]);
    const char* tempests[SHIELD_COUNT]={"sTempestShield","sTempestHalo","sTempestReflect","sTempestHalo","sTempestTemporal","sTempestAmp","sTempestWarp","sTempestShield","sTempestReflect","sTempestAmp"};
    for(unsigned i=0;i<SHIELD_COUNT;i++)last_stand_tempests_[i]=assets_->index_of(tempests[i]);
    const char* constructNames[C_KIND_COUNT]={"sConstructDrone","sConstructDroneDefense","sMine",
        "sTurret","sConstructAlly","sConstructAllyInterceptorArmored","sConstructAllyGuardianArmored","sCarrierDrone","sBlade","sSwords","sBastion2"};
    for(unsigned i=0;i<C_KIND_COUNT;++i)constructs_[i]=assets_->index_of(constructNames[i]);
    const char* pickupNames[4]={"s_power_white","s_power_mega","s_power_gold","s_power_gold"};
    for(unsigned i=0;i<4;++i)pickups_[i]=assets_->index_of(pickupNames[i]);
    source_powerups_[0]=assets_->index_of("s_power_mega");source_powerups_[1]=assets_->index_of("sArmorFragment");source_powerups_[2]=assets_->index_of("sMediCharge");
    logo_=assets_->index_of("sNovaDriftLogo");
    planet_=assets_->index_of("sMenuPlanet");
    nebula_=assets_->index_of("sMenuNebula");
    rings_=assets_->index_of("sMenuPlanetRings");moon_=assets_->index_of("sMenuMoon");
    menu_cursor_=assets_->index_of("sMenuCursor");cursor_=assets_->index_of("sCursorSmall");
    const char *cosmicNames[COSMIC_KIND_COUNT]={"sComet","sTrainEngine","sTrainCar","sBlackHole"};
    for(unsigned i=0;i<COSMIC_KIND_COUNT;i++)cosmic_sprites_[i]=assets_->index_of(cosmicNames[i]);
    cosmic_warning_=assets_->index_of("sWarningComet");cosmic_egg_=assets_->index_of("sprCosmicEgg1");
    canvas.bind(background_);
    static const uint8_t dither[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
    for(int y=0;y<ScreenH;++y) for(int x=0;x<ScreenW;++x) {
        int dx=x-220,dy=y-70;
        int cloud=50-(dx*dx+dy*dy)/450;
        if(cloud<0)cloud=0;
        unsigned noise=dither[(y&3)*4+(x&3)];
        background_[y*ScreenW+x]=rgb(2+cloud/16+noise/2,5+cloud/9+noise/4,10+cloud/7+noise/2);
    }
    // Background decoration is baked once, so combat never decodes a nebula.
    sprite(nebula_,0,242,95,270,0,rgb(70,120,180),75,true);
}

bool Renderer::sprite(int id,unsigned frame,int x,int y,int size,uint8_t angle,
                      uint16_t tint,unsigned opacity,bool additive,unsigned lighting) {
    if(id<0||!assets_->ready())return false;
    const Sprite* s=assets_->find_id(unsigned(id),frame);
    if(!s)return false;
    int scale=256;
    if(size>0) {
        unsigned extent=s->canvas_width>s->canvas_height?s->canvas_width:s->canvas_height;
        if(!extent)return false;
        scale=size*256/int(extent);
        if(scale<1)scale=1;
        if(scale>1024)scale=1024;
    }
    canvas.sprite(x,y,s->pixels,s->alpha,s->width,s->height,s->origin_x,s->origin_y,
                  dirX(angle),dirY(angle),scale,tint,opacity,additive,lighting);
    return true;
}

void Renderer::background(uint32_t ticks,bool animated) {
    if(options_.background_detail==0){canvas.clear(0);return;}
    if(options_.background_detail==1)canvas.clear(rgb(1,3,8));
    else for(int i=0;i<ScreenW*ScreenH;++i)canvas.pixels[i]=background_[i];
    uint32_t random=0x91e10da5;
    for(int i=0;i<(options_.background_detail==1?18:options_.background_detail==2?32:56);++i) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        unsigned x=random%320;
        unsigned y=(random>>12)%240;
        if(animated)y=(y+(ticks/(i%3==0?45:90)))%240;
        uint16_t c=i%9==0?rgb(106,150,180):rgb(37,66,88);
        canvas.pixel(int(x),int(y),c);
        if(i%17==0)canvas.pixel(int(x)+1,int(y),rgb(42,64,85));
    }
}

void Renderer::fallbackShip(int x,int y,uint8_t angle,int size,uint16_t c) {
    int fx=dirX(angle)*size/256,fy=dirY(angle)*size/256;
    int sx=-dirY(angle)*size/384,sy=dirX(angle)*size/384;
    canvas.line(x+fx,y+fy,x-fx/2+sx,y-fy/2+sy,c);
    canvas.line(x+fx,y+fy,x-fx/2-sx,y-fy/2-sy,c);
    canvas.line(x-fx/2+sx,y-fy/2+sy,x-fx/4,y-fy/4,c);
    canvas.line(x-fx/2-sx,y-fy/2-sy,x-fx/4,y-fy/4,c);
}

void Renderer::menuBackground(uint32_t ticks,bool celestial) {
    (void)ticks;unsigned kind=celestial?1u:0u;
    if(!menu_valid_[kind]) {
        uint16_t* target=canvas.pixels;canvas.bind(menu_background_[kind]);
        uint8_t detail=options_.background_detail;options_.background_detail=3;
        background(0,false);options_.background_detail=detail;
        ui("sMenuNebula",174,112,355,rgb(145,174,255),170);
        if(celestial) {
            ui("sMenuPlanet",124,122,142,65535,210);
            ui("sMenuPlanetRings",124,122,220,65535,210);
            ui("sMenuMoon",167,71,12,65535,220);
            ui("sMenuStar",244,184,52,65535,205);
        }
        menu_valid_[kind]=true;canvas.bind(target);
    }
    for(int i=0;i<ScreenW*ScreenH;++i)canvas.pixels[i]=menu_background_[kind][i];
}
bool Renderer::skin(unsigned kind,int x,int y,int size,uint8_t angle,unsigned opacity) {
    const Sprite* s=skins_?skins_->image(kind):nullptr;if(!s)return false;
    int extent=s->canvas_width>s->canvas_height?s->canvas_width:s->canvas_height;
    int scale=extent?size*256/extent:256;if(scale<1)scale=1;
    canvas.sprite(x,y,s->pixels,s->alpha,s->width,s->height,s->origin_x,s->origin_y,
        dirX(angle),dirY(angle),scale,65535,opacity,false);return true;
}
void Renderer::shake(int dx,int dy) {
    if(!dx&&!dy)return;
    for(int yi=0;yi<ScreenH;++yi){int y=dy>0?ScreenH-1-yi:yi;
        for(int xi=0;xi<ScreenW;++xi){int x=dx>0?ScreenW-1-xi:xi;int sx=x-dx,sy=y-dy;
            canvas.pixels[y*ScreenW+x]=sx>=0&&sx<ScreenW&&sy>=0&&sy<ScreenH?canvas.pixels[sy*ScreenW+sx]:rgb(1,3,8);}}
}
bool Renderer::ui(const char* name,int x,int y,int size,uint16_t tint,unsigned opacity) {
    const Sprite* s=assets_->find(name);if(!s)return false;
    unsigned extent=s->canvas_width>s->canvas_height?s->canvas_width:s->canvas_height;
    if(!extent)return false;
    int scale=size*256/int(extent);if(scale<1)scale=1;
            if(scale>1024)scale=1024;
    canvas.sprite(x,y,s->pixels,s->alpha,s->width,s->height,s->width/2,s->height/2,
        256,0,scale,tint,opacity,false);
    return true;
}
bool Renderer::uiStrip(const char* name,int x,int y,int width,int height,unsigned opacity) {
    const Sprite* s=assets_->find(name);if(!s||width<1||height<1)return false;
    for(int dy=0;dy<height;++dy)for(int dx=0;dx<width;++dx) {
        unsigned sx=unsigned(dx)*s->width/unsigned(width),sy=unsigned(dy)*s->height/unsigned(height),at=sy*s->width+sx;
        unsigned a=unsigned(s->alpha[at])*opacity/255;if(a)canvas.alphaPixel(x+dx,y+dy,s->pixels[at],a);
    }
    return true;
}
void Renderer::selectIcon(int x,int y,int size,bool selected) {
    ui(selected?"sSelectaWhite":"sSelecta",x,y,size,65535,selected?255:160);
}
void Renderer::world(const Game& g,uint32_t ticks,bool effects,bool asteroids,bool hullbars,const char* upgrade_key,bool upgrade_prompt) {
    projection=ScreenW*256/g.worldWidth();
    background(ticks,true);
    for(unsigned i=0;i<MAX_COSMIC_OBJECTS;i++){
        const CosmicObject &c=g.cosmic_objects[i];if(!c.active||c.kind>=COSMIC_KIND_COUNT)continue;
        int x=scenePixel(c.x),y=scenePixel(c.y);
        if(c.warning){
            if(x<9)x=9;
            if(x>310)x=310;
            if(y<30)y=30;
            if(y>228)y=228;
            if((c.warning/5)&1)sprite(cosmic_warning_,0,x,y,17,c.angle,65535,220);
            if(c.kind==COSMIC_BLACK_HOLE)canvas.circle(x,y,sceneRadius(c.radius),Purple);
        }else {
            unsigned size=c.kind==COSMIC_COMET?32:c.kind==COSMIC_BLACK_HOLE?70:22;
            sprite(cosmic_sprites_[c.kind],c.age/5,x,y,sceneRadius(size),c.angle,65535,255,c.kind==COSMIC_COMET);
            if(hullbars&&(c.kind==COSMIC_TRAIN_ENGINE||c.kind==COSMIC_TRAIN_CAR)&&c.hull<(c.kind==COSMIC_TRAIN_ENGINE?900:180))canvas.bar(x-8,y-10,16,2,c.hull,c.kind==COSMIC_TRAIN_ENGINE?900:180,Green);
        }
    }
    if(g.daily(DAILY_COSMIC_EGG)&&g.daily_egg_hull>0){
        sprite(cosmic_egg_,ticks/8,scenePixel(g.daily_egg_x),scenePixel(g.daily_egg_y),sceneRadius(38),0,65535,g.daily_egg_grace?150:255);
        canvas.bar(90,32,140,3,g.daily_egg_hull,15000,Green);
    }
    if(g.maelstrom_state.active){
        int32_t cx=g.player.mods[M_X_SANCTUARY]?g.sanctuary_x:g.player.x;
        int32_t cy=g.player.mods[M_X_SANCTUARY]?g.sanctuary_y:g.player.y;
        const MaelstromState &m=g.maelstrom_state;
        for(unsigned layer=0;layer<2;layer++){
            const Sprite *art=maelstrom_[layer]>=0?assets_->find_id((unsigned)maelstrom_[layer],0):nullptr;
            uint32_t sourceScale=layer?m.small_scale_q12:m.large_scale_q12;
            if(!art||!sourceScale||!art->canvas_width)continue;
            uint64_t scaled=(uint64_t)art->original_width*sourceScale*projection/(4096ull*6*art->canvas_width);
            if(!scaled)continue;
            int scale=(int)(scaled>0x3fffffffu?0x3fffffffu:scaled);
            uint8_t angle=(uint8_t)((layer?m.small_angle_q8:m.angle_q8)>>8);
            canvas.sprite(scenePixel(cx),scenePixel(cy),art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(angle),dirY(angle),scale,65535,layer?64:128,true);
        }
    }
    for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++){
        const PersistentField &field=g.persistent_fields[i];if(!field.active||!field.radius)continue;
        unsigned shield=g.player.shield_type<SHIELD_COUNT?g.player.shield_type:0;
        int id=field.reserved[0]==1?last_stand_sprites_[shield]:field.reserved[0]==2?bastion_wave_:fire_bursts_[shield];
        if(field.reserved[0]==1&&field.source_age<45){
            // checkDestroyPlayer creates ten original oTempest swirls. Their
            // source RNG ranges are reproduced with a stable visual-only seed.
            const Sprite *swirl=last_stand_tempests_[shield]>=0?assets_->find_id((unsigned)last_stand_tempests_[shield],0):nullptr;
            if(swirl&&swirl->canvas_width){
                uint32_t seed=0x9e3779b9u+field.angle;unsigned age=field.source_age;
                for(unsigned j=0;j<10;j++){
                    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;unsigned q12=164+(seed&65535)*410/65536;
                    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;unsigned rotation=1280+(seed&65535)*3840/65536;
                    for(unsigned f=0;f<age;f++)q12=q12*1048/1000;
                    int scale=(int)((uint64_t)swirl->original_width*q12*projection/(4096ull*6*swirl->canvas_width));
                    unsigned opacity=age<25?255:(45-age)*255/20;uint8_t angle=(uint8_t)(0u-age*rotation/360);
                    if(scale>0)canvas.sprite(scenePixel(field.x),scenePixel(field.y),swirl->pixels,swirl->alpha,swirl->width,swirl->height,swirl->origin_x,swirl->origin_y,dirX(angle),dirY(angle),scale,65535,opacity,true);
                }
            }
        }
        const Sprite *art=id>=0?assets_->find_id((unsigned)id,0):nullptr;
        if(art&&art->canvas_height){
            unsigned diameter=field.reserved[0]==1?(unsigned)field.radius*643/200:(unsigned)field.radius*2;
            uint64_t scaled=(uint64_t)diameter*projection/art->canvas_height;
            int scale=(int)(scaled>0x3fffffffu?0x3fffffffu:scaled);if(scale<1)scale=1;
            // sBastionWave preserves its left-edge source pivot; its growing
            // front follows the same player/Sanctuary anchor as collision.
            canvas.sprite(scenePixel(field.x),scenePixel(field.y),art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(field.angle),dirY(field.angle),scale,65535,field.opacity,true);
        }
    }
    for(unsigned i=0;i<MAX_PICKUPS;++i) {
        const Pickup& p=g.pickups[i];if(!p.active)continue;
        int x=scenePixel(p.x),y=scenePixel(p.y);
        unsigned kind=p.kind<4?p.kind:0;
        int pickupArt=kind==PICKUP_POWER?source_powerups_[p.value>=1&&p.value<=3?p.value-1:0]:pickups_[kind];
        if(!sprite(pickupArt,0,x,y,sceneRadius(kind==PICKUP_XP?6:9))) {
            uint16_t color=kind==PICKUP_HEAL?Green:Cyan;
            canvas.line(x-2,y,x,y-2,color);canvas.line(x,y-2,x+2,y,color);
            canvas.line(x+2,y,x,y+2,color);canvas.line(x,y+2,x-2,y,color);
        }
    }
    for(unsigned i=0;i<MAX_BOSS_PARTS;++i){const BossPart& p=g.boss_parts[i];if(p.active&&p.owner<MAX_ENEMIES&&!Composites::foreground(p.kind)&&g.enemies[p.owner].active&&!g.enemies[p.owner].boss_hidden)composites_.part(canvas,p,g.enemies[p.owner],scenePixel(p.x),scenePixel(p.y),projection,ticks);}
    for(unsigned i=0;i<MAX_ENEMIES;++i) {
        const Enemy& e=g.enemies[i];if(!e.active||e.boss_hidden||e.type>=ENEMY_COUNT)continue;
        int x=scenePixel(e.x),y=scenePixel(e.y);
        // applyRarity writes image_speed=0 and image_index=rarity.
        unsigned frame=e.elite;
        int diameter=sceneRadius(e.radius*2+5);
        int enemySprite=enemies_[e.type];
        if(e.type==E_ASTEROID){
            unsigned tier=e.phase<4?e.phase:3,variant=e.generation%4;
            if(asteroids_[tier][variant]>=0)enemySprite=asteroids_[tier][variant];
            // The source rocks have transparent canvas padding. Fit the actual
            // cropped silhouette to its collision diameter, keeping its source
            // pivot and texture, so small shards remain visible on the LCD.
            int minimum=tier?20:11;if(diameter<minimum)diameter=minimum;
            const Sprite *rock=enemySprite>=0?assets_->find_id(unsigned(enemySprite),0):nullptr;
            if(rock){int visible=asteroid_visible_[tier][variant];
                int extent=rock->canvas_width>rock->canvas_height?rock->canvas_width:rock->canvas_height;
                if(visible)diameter=(diameter*extent+visible-1)/visible;}
        }
        uint16_t tint=e.flash?White:65535;
        if(!composites_.core(canvas,e,x,y,projection,ticks)&&!composites_.draw(canvas,e,x,y,diameter,ticks)&&
           !sprite(enemySprite,frame,x,y,diameter,e.angle,tint,255,false,e.type==E_ASTEROID&&asteroids?1:0))
            fallbackShip(x,y,e.angle,sceneRadius(e.radius),e.elite?Red:Orange);
        if(e.elite)canvas.circle(x,y,sceneRadius(e.radius+2),Purple);
        if(e.hard_shield>0&&!sprite(enemy_hard_shield_,e.elite,x,y,sceneRadius(e.radius*2+12),e.angle,65535,205))
            canvas.circle(x,y,sceneRadius(e.radius+5),Cyan);
        if((e.burn_time||e.wake_time) && effects) {
            int off=int((g.frame+i)%5)-2;
            canvas.pixel(x+off,y-sceneRadius(e.radius),Orange);
            canvas.pixel(x-off,y+sceneRadius(e.radius),Red);
        }
        if(hullbars&&e.hull<e.max_hull && int(i)!=g.boss_index)
            canvas.bar(x-sceneRadius(e.radius),y-sceneRadius(e.radius)-4,sceneRadius(e.radius*2),2,e.hull,e.max_hull,e.elite?Purple:Orange);
    }
    for(unsigned i=0;i<MAX_CONSTRUCTS;++i) {
        const Construct& c=g.constructs[i];if(!c.active||c.hull<=0)continue;
        unsigned kind=c.kind<C_KIND_COUNT?c.kind:0;
        if(kind==C_SWORD||kind==C_BASTION){
            const AdvancedConstructState& state=g.advanced_constructs[i];
            const Sprite* image=kind==C_SWORD&&g.player.mods[M_X_ANNIHILATION]?assets_->find("sSwordsAntimatter",0):assets_->find_id(unsigned(constructs_[kind]),0);
            if(!image)continue;
            int size=kind==C_SWORD?g.stats.size:100;
            int source_scale=kind==C_SWORD?4096*size/100:3823*int(c.radius)/10;
            if(kind==C_SWORD&&state.state)source_scale=source_scale*4/3;
            int center=kind==C_SWORD?167:25;
            int offset=int(int64_t(center)*source_scale*projection/16777216);
            int x=scenePixel(c.x)-dirX(c.angle)*offset/256,y=scenePixel(c.y)-dirY(c.angle)*offset/256;
            int scale=int(int64_t(source_scale)*projection*image->original_width/(int64_t(image->canvas_width)*65536));
            if(scale<1)scale=1;
            if(scale>1024)scale=1024;
            canvas.sprite(x,y,image->pixels,image->alpha,image->width,image->height,image->origin_x,image->origin_y,
                          dirX(c.angle),dirY(c.angle),scale,65535,255,false,0,kind==C_SWORD&&state.side);
            if(kind==C_BASTION&&g.player.shield_energy)sprite(assets_->index_of("sBastionGlow"),0,scenePixel(c.x),scenePixel(c.y),sceneRadius(2*c.radius),c.angle,65535,100,true);
            continue;
        }
        if(!((kind==C_DRONE||kind==C_DEFENSE_DRONE||kind==C_SWARM)&&skin(2,scenePixel(c.x),scenePixel(c.y),sceneRadius(c.radius*2+5),c.angle))&&!sprite(constructs_[kind],0,scenePixel(c.x),scenePixel(c.y),sceneRadius(c.radius*2+5),c.angle))
            fallbackShip(scenePixel(c.x),scenePixel(c.y),c.angle,sceneRadius(c.radius),Cyan);
    }
    if(g.player.body==B_LEVIATHAN) for(unsigned remaining=g.segment_count;remaining>0;--remaining) {
        unsigned i=remaining-1;
        if(i>=MAX_SEGMENTS)continue;
        int r=sceneRadius(6-int(i)/7);if(r<2)r=2;
        if(!skin(0,scenePixel(g.segment_x[i]),scenePixel(g.segment_y[i]),r*2+5,g.player.angle)) {
            canvas.disk(scenePixel(g.segment_x[i]),scenePixel(g.segment_y[i]),r,rgb(38,58,75));
            canvas.circle(scenePixel(g.segment_x[i]),scenePixel(g.segment_y[i]),r,White);
        }
    }
    for(unsigned i=0;i<MAX_BOSS_PARTS;++i){const BossPart& p=g.boss_parts[i];if(p.active&&p.owner<MAX_ENEMIES&&Composites::foreground(p.kind)&&g.enemies[p.owner].active&&!g.enemies[p.owner].boss_hidden)composites_.part(canvas,p,g.enemies[p.owner],scenePixel(p.x),scenePixel(p.y),projection,ticks);}
    const Player& p=g.player;
    int px=scenePixel(p.x),py=scenePixel(p.y);
    if(p.shield>0) {
        uint16_t shieldColor=p.shield_type==S_HALO?Orange:p.shield_type==S_TEMPORAL?Purple:Cyan;
        int radius=sceneRadius(p.radius+6);
        if(p.shield_type!=S_BASTION)canvas.circle(px,py,radius,Canvas::blend(rgb(0,0,0),shieldColor,150));
        int32_t shieldX=p.mods[M_X_SANCTUARY]?g.sanctuary_x:p.x;
        int32_t shieldY=p.mods[M_X_SANCTUARY]?g.sanctuary_y:p.y;
        int effectRadius=g.stats.shield_radius;
        if(p.mods[M_X_SANCTUARY])effectRadius=effectRadius*160/100;
        if(p.mods[M_X_CHARGED_SHIELDS])effectRadius+=p.charge/10;
        if(p.shield_type==S_ORBITAL)for(unsigned i=0;i<3;++i) {
            uint8_t a=uint8_t(g.frame*2+i*256/3);
            int32_t ox=(shieldX+dirX(a)*effectRadius+g.worldWidth()*FP)%(g.worldWidth()*FP);
            int32_t oy=(shieldY+dirY(a)*effectRadius+g.worldHeight()*FP)%(g.worldHeight()*FP);
            int x=scenePixel(ox),y=scenePixel(oy);
            if(effects)canvas.glow(x,y,sceneRadius(5),Cyan);
            canvas.disk(x,y,sceneRadius(2),White);
        }
        int sx=scenePixel(shieldX),sy=scenePixel(shieldY),field=sceneRadius(effectRadius);
        if(p.shield_type==S_HALO)canvas.circle(sx,sy,field,Canvas::blend(0,Orange,85));
        if(p.mods[M_X_SANCTUARY]||p.mods[M_X_CHARGED_SHIELDS]) {
            canvas.circle(sx,sy,field,Canvas::blend(0,Cyan,135));
            if(effects)canvas.circle(sx,sy,field+1,Canvas::blend(0,Purple,85));
        }
    }
    if(p.thrusting && effects) {
        int length=sceneRadius(7+int(g.frame%5));
        const Sprite* custom=skins_?skins_->image(0):nullptr;
        if(custom&&custom->original_width){int distance=skins_->jetDistance()*sceneRadius(p.radius*2+10)/custom->original_width;
            for(int side=-1;side<=1;side+=2){uint8_t a=uint8_t(p.angle+side*skins_->jetAngle()*256/360);
                int x=px+dirX(a)*distance/256,y=py+dirY(a)*distance/256;
                canvas.line(x,y,x-dirX(p.angle)*length/256,y-dirY(p.angle)*length/256,Cyan);}
        } else {
            int x=px-dirX(p.angle)*(sceneRadius(p.radius)+length)/256;
            int y=py-dirY(p.angle)*(sceneRadius(p.radius)+length)/256;
            canvas.line(px-dirX(p.angle)*sceneRadius(p.radius)/256,py-dirY(p.angle)*sceneRadius(p.radius)/256,x,y,p.body==B_FIREFLY?Orange:Cyan);canvas.pixel(x,y,White);
        }
    }
    if(p.mods[M_FORCE_ARMOR])skin(1,px,py,sceneRadius(p.radius*2+10),p.angle);
    if(!p.invulnerable || (g.frame&2)==0) {
        unsigned opacity=p.cloak>30?100:255;
        if(!skin(0,px,py,sceneRadius(p.radius*2+10),p.angle,opacity)&&(p.body>=BODY_COUNT || !sprite(bodies_[p.body],(g.frame/8)%3,px,py,sceneRadius(p.radius*2+10),p.angle,65535,opacity)))
            fallbackShip(px,py,p.angle,sceneRadius(p.radius+4),White);
    }
    for(unsigned i=0;i<MAX_BULLETS;++i) {
        const Bullet& b=g.bullets[i];if(!b.active)continue;
        int x=scenePixel(b.x),y=scenePixel(b.y);
        unsigned kind=b.kind<SHOT_KIND_COUNT?b.kind:unsigned(SHOT_ENEMY);
        if(kind==SHOT_TRIANGLE_SPAWNER)continue;
        unsigned animation=b.owner?0:b.age/3;uint8_t angle=b.angle;
        if(b.tracking&&kind!=SHOT_VORTEX){
            const Sprite *tracked=assets_->find_id((unsigned)bullets_[kind],animation);
            if(tracked&&tracked->canvas_width&&tracked->canvas_height){
                int sx=(int)((uint64_t)tracked->original_width*b.tracking_scale_x*projection/(4096ull*6*tracked->canvas_width));
                int sy=(int)((uint64_t)tracked->original_height*b.tracking_scale_y*projection/(4096ull*6*tracked->canvas_height));
                if(sx>0&&sy>0)canvas.sprite(x,y,tracked->pixels,tracked->alpha,tracked->width,tracked->height,tracked->origin_x,tracked->origin_y,dirX(angle),dirY(angle),sx,65535,255,false,0,false,sy);
            }
            continue;
        }
        if(kind==SHOT_PULSAR_PULSE){
            int ids[2]={bullets_[kind],pulsar_burst_};
            for(unsigned layer=0;layer<2;layer++){
                const Sprite *art=ids[layer]>=0?assets_->find_id((unsigned)ids[layer],0):nullptr;
                if(!art||!art->canvas_width||!art->canvas_height)continue;
                int sx=(int)((uint64_t)art->original_width*b.size_percent*projection/(4096ull*6*art->canvas_width));
                int sy=(int)((uint64_t)art->original_height*b.size_percent*projection/(4096ull*6*art->canvas_height));
                if(sx>0&&sy>0)canvas.sprite(x,y,art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(angle),dirY(angle),sx,65535,255,true,0,false,sy);
            }
            continue;
        }
        if(kind==SHOT_WAKE){
            const Sprite *wake=assets_->find_id((unsigned)wake_sprites_[b.pierce<7?b.pierce:6],0);
            if(wake&&wake->canvas_width){
                int scale=(int)((uint64_t)wake->original_width*b.size_percent*projection/(uint64_t(4096)*6* wake->canvas_width));
                if(scale>0)canvas.sprite(x,y,wake->pixels,wake->alpha,wake->width,wake->height,wake->origin_x,wake->origin_y,dirX(angle),dirY(angle),scale,65535,255,true);
            }
            continue;
        }
        if(kind==SHOT_DISCHARGE){
            // A native oChain node is a stationary segment. Its original art
            // spans 310 source pixels from the pivot, inside a 380px canvas.
            int dx=scenePixel(b.vx),dy=scenePixel(b.vy),length=0;
            unsigned square=(unsigned)(dx*dx+dy*dy);while((unsigned)(length+1)*(length+1)<=square&&length<640)++length;
            const Sprite *chain=assets_->find_id((unsigned)bullets_[kind],b.pierce);
            if(chain&&length&&chain->canvas_width&&chain->canvas_height){
                int sx=length*256*380/(310*chain->canvas_width);
                int sy=(int)((uint64_t)b.size_percent*2u*160u*projection/(256u*62u*chain->canvas_height));
                if(sx<1)sx=1;
                if(sy<1)sy=1;
                unsigned opacity=b.age<=5?255:b.age==6?128:0;
                canvas.sprite(x,y,chain->pixels,chain->alpha,chain->width,chain->height,chain->origin_x,chain->origin_y,dirX(angle),dirY(angle),sx,65535,opacity,true,0,false,sy);
            }else canvas.line(x,y,x+dx,y+dy,Cyan);
            continue;
        }
        #include "render_snapper.inc"
        if(kind==SHOT_MYRMIDON_SPAWNER)continue;
        const bool eclipse_art=kind==SHOT_ECLIPSE_TORRENT||kind==SHOT_ECLIPSE_BOLT||kind==SHOT_ECLIPSE_ORB||kind==SHOT_ECLIPSE_VORTEX||kind==SHOT_ECLIPSE_REFLECT||kind==SHOT_ECLIPSE_BURST||kind==SHOT_ECLIPSE_SHOT;
        if(kind==SHOT_HOST_DRONE||kind==SHOT_SERAPH_ZONE||eclipse_art||kind==SHOT_MYRMIDON_BOMB||kind==SHOT_MYRMIDON_BLAST||kind==SHOT_MYRMIDON_TURRET){
            int art_id=kind==SHOT_MYRMIDON_BLAST&&b.pierce?myrmidon_blast_:bullets_[kind];
            const Sprite *art=art_id>=0?assets_->find_id((unsigned)art_id,0):nullptr;
            unsigned scale_x=b.size_percent,scale_y=scale_x,opacity=255;
            if(kind==SHOT_SERAPH_ZONE)scale_y=(unsigned)b.last_hit;
            else if(eclipse_art){scale_x=b.tracking_scale_x;scale_y=b.tracking_scale_y;}
            if(kind==SHOT_ECLIPSE_VORTEX||kind==SHOT_ECLIPSE_REFLECT||kind==SHOT_ECLIPSE_BURST)opacity=b.homing;
            if(kind==SHOT_MYRMIDON_BLAST&&b.pierce&&b.tracking_scale_x>7)opacity=b.tracking_scale_x>=12?0u:255u-51u*(b.tracking_scale_x-7u);
            if(kind==SHOT_MYRMIDON_BOMB)angle=(uint8_t)(b.bounces-b.tracking_scale_y*256u/24u);
            if(art&&art->canvas_width&&art->canvas_height&&opacity){
                int sx=(int)((uint64_t)art->original_width*scale_x*projection/(4096ull*6*art->canvas_width));
                int sy=(int)((uint64_t)art->original_height*scale_y*projection/(4096ull*6*art->canvas_height));
                if(sx>0&&sy>0)canvas.sprite(x,y,art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(angle),dirY(angle),sx,65535,opacity,false,0,false,sy);
            }
            continue;
        }
        int size=sceneRadius(b.radius*2+4);
        if(kind==SHOT_RAIL)size=sceneRadius(21);
        else if(kind==SHOT_VORTEX)size=sceneRadius(b.radius*2+8);
        else if(size<8)size=8;
        // These source projectiles have explicit scales, independent of their
        // collision radius. Original dimensions survive the texture packing.
        unsigned sourceScale=0;
        if(kind==SHOT_PULSAR_MINE)sourceScale=64;
        else if(kind==SHOT_SPIKE||kind==SHOT_SPIKE_MINI||kind==SHOT_SPIKE_TRIPLE)sourceScale=b.size_percent*256/4096;
        else if(kind==SHOT_CELESTIAL_STRIKE||kind==SHOT_CELESTIAL_STRIKE_MINI||kind==SHOT_CELESTIAL_WARNING)sourceScale=b.size_percent*256/100;
        else if(kind==SHOT_ENEMY_SHIELD)sourceScale=102;
        else if(kind==SHOT_WARDEN)sourceScale=136+b.age*1408/1000;
        else if(kind==SHOT_WARDEN_VORTEX)sourceScale=115;
        else if(kind==SHOT_SINE||kind==SHOT_SINE_MEGA){sourceScale=b.age<297?128-b.age*43/100:1;angle=(uint8_t)(angle+b.age*128/3);}
        else if(kind==SHOT_STATION_DRONE)sourceScale=115;
        else if(kind==SHOT_INTERCEPTOR_MISSILE)sourceScale=77;
        else if(kind==SHOT_PULSEBUG||kind==SHOT_PULSEBUG_HOMING){
            static const uint16_t normal[53]={4096,3504,2998,2565,2194,1877,1606,1374,1175,1005,859,734,628,537,459,392,335,286,244,208,177,151,129,110,94,80,68,58,49,41,35,29,24,20,17,14,11,9,7,5,4,3,2,1,0,0,0,0,0,0,0,0,0};
            static const uint16_t homing[53]={4096,3814,3551,3306,3078,2866,2668,2484,2313,2153,2004,1866,1737,1617,1505,1401,1304,1214,1130,1052,979,911,848,789,734,683,636,592,551,513,477,444,413,384,357,332,309,287,267,248,230,214,199,185,172,160,148,137,127,118,109,101,94};
            unsigned age=b.age<53?b.age:52;sourceScale=(kind==SHOT_PULSEBUG?normal[age]:homing[age])/16;if(!sourceScale)sourceScale=1;
        }
        else if(kind==SHOT_JUGGERNAUT){sourceScale=83u*b.size_percent/100;if(!sourceScale)sourceScale=1;}
        else if(kind==SHOT_JUGGERNAUT_ORB||kind==SHOT_CELESTIAL){size=size*(int)b.size_percent/100;if(size<1)size=1;}
        else if(kind==SHOT_DWELLER_ARC){sourceScale=b.size_percent*256/100;if(!sourceScale)sourceScale=1;}
        if(sourceScale&&bullets_[kind]>=0){const Sprite *original=assets_->find_id((unsigned)bullets_[kind],animation);if(original){unsigned extent=original->original_width>original->original_height?original->original_width:original->original_height;size=sceneRadius((int)(extent*sourceScale/1536));if(size<1)size=1;}}
        bool drawn=false;
        if(kind==SHOT_DWELLER_GUARD&&bullets_[kind]>=0){
            const Sprite *guard=assets_->find_id((unsigned)bullets_[kind],0);
            if(guard&&guard->canvas_width&&guard->canvas_height){
                int sx=(int)((unsigned)guard->original_width*166/guard->canvas_width/6)*projection/256;
                int sy=(int)((unsigned)guard->original_height*205*b.size_percent/guard->canvas_height/600)*projection/256;
                if(sx<1)sx=1;
                if(sy<1)sy=1;
                canvas.sprite(x,y,guard->pixels,guard->alpha,guard->width,guard->height,guard->origin_x,guard->origin_y,dirX(angle),dirY(angle),sx,65535,255,false,0,false,sy);drawn=true;
            }
        }
        if((kind==SHOT_CELESTIAL_DRONE||kind==SHOT_PULSAR_BARRAGE)&&bullets_[kind]>=0){
            const Sprite *original=assets_->find_id((unsigned)bullets_[kind],0);
            if(original&&original->canvas_width&&original->canvas_height){
                int sx=(int)((uint64_t)original->original_width*b.size_percent*projection/(uint64_t(4096)*6*original->canvas_width));
                int sy=(int)((uint64_t)original->original_height*(unsigned)b.last_hit*projection/(uint64_t(4096)*6*original->canvas_height));
                if(sx>0&&sy>0)canvas.sprite(x,y,original->pixels,original->alpha,original->width,original->height,original->origin_x,original->origin_y,dirX(angle),dirY(angle),sx,65535,255,true,0,false,sy);
                drawn=true;
            }
        }
        if(kind==SHOT_CELESTIAL_WARNING&&b.pierce){
            const Sprite *blast=assets_->find("sExplosionYellow");
            unsigned age=b.age*2,alpha=age<7?255:age<12?(12-age)*51:0;
            if(blast&&blast->canvas_width){int scale=(int)((uint64_t)blast->original_width*b.size_percent*projection/(uint64_t(600)*blast->canvas_width));
                canvas.sprite(x,y,blast->pixels,blast->alpha,blast->width,blast->height,blast->origin_x,blast->origin_y,dirX(angle),dirY(angle),scale,65535,alpha,true);drawn=true;
            }
        }
        if(!drawn&&!sprite(bullets_[kind],animation,x,y,size,angle)) {
            uint16_t color=b.owner?Red:Cyan;
            canvas.line(x,y,x-scenePixel(b.vx),y-scenePixel(b.vy),color);
            if(b.radius>2)canvas.circle(x,y,sceneRadius(b.radius),color);
            else canvas.pixel(x,y,White);
        }
    }
    for(unsigned i=0;i<MAX_EFFECTS;++i) {
        const Effect& e=g.effects[i];if(!e.active||!e.max_life)continue;
        if(!options_.shockwaves&&(e.kind==FX_RING||e.kind==FX_SHIELD_BREAK||e.kind==FX_WARP))continue;
        int x=scenePixel(e.x),y=scenePixel(e.y);
        uint16_t color=e.color==8?Green:e.color==1?Orange:e.color==2?Purple:e.color==3?Red:Cyan;
        unsigned opacity=unsigned(e.life)*255/e.max_life;
        color=Canvas::blend(rgb(3,9,18),color,opacity);
        int radius=sceneRadius(e.radius)*(e.max_life-e.life+1)/e.max_life;
        if(e.kind==FX_PULSAR_CHARGE){
            const Sprite *art=pulsar_charge_>=0?assets_->find_id((unsigned)pulsar_charge_,0):nullptr;
            if(art&&art->canvas_width&&e.x2>0){
                int scale=(int)((uint64_t)art->original_width*(unsigned)e.x2*projection/(4096ull*6*art->canvas_width));
                uint8_t angle=(uint8_t)e.y2;
                if(scale>0)canvas.sprite(x,y,art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(angle),dirY(angle),scale,65535,255,true);
            }
        }else if(e.kind==FX_SWARM_CHARGE&&e.color==9){
            const Sprite* art=assets_->find("sBeamGlowRed");
            uint8_t angle=direction(e.x2-e.x,e.y2-e.y);
            unsigned age=(e.max_life-e.life)*2,scale=1000+75*age;
            if(art&&art->canvas_width){int factor=(int)((uint64_t)art->original_width*scale*projection/(60000ull*art->canvas_width));
                if(factor>0)canvas.sprite(x+dirX(angle)*sceneRadius(8)/256,y+dirY(angle)*sceneRadius(8)/256,art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(angle),dirY(angle),factor,65535,255,true);
            }
        }else if(e.kind==FX_SWARM_BEAM||e.kind==FX_SWARM_CHARGE||(e.kind==FX_BEAM&&e.color==7)){
            int tx=scenePixel(e.x2),ty=scenePixel(e.y2),dx=tx-x,dy=ty-y;
            unsigned length=0,square=(unsigned)(dx*dx+dy*dy);while((length+1)*(length+1)<=square&&length<1024)++length;
            int id=e.kind==FX_SWARM_CHARGE?swarm_charges_[e.color&1]:e.kind==FX_SWARM_BEAM?swarm_beams_[e.color&1]:overseer_beam_;
            const Sprite *art=id>=0?assets_->find_id((unsigned)id,0):nullptr;
            if(art&&art->canvas_width&&art->canvas_height){
                uint8_t angle=direction(dx,dy);
                int sx=e.kind==FX_SWARM_CHARGE?sceneRadius(e.radius*2)*256/art->canvas_width:(int)length*256/art->canvas_width;
                int sy=sceneRadius(e.radius*2)*256/art->canvas_height;if(sx<1)sx=1;if(sy<1)sy=1;
                canvas.sprite(x,y,art->pixels,art->alpha,art->width,art->height,art->origin_x,art->origin_y,dirX(angle),dirY(angle),sx,65535,255,true,0,false,sy);
            }else canvas.line(x,y,tx,ty,e.color?Green:Purple);
        }else if(e.kind==FX_SERAPH_LANCE) {
            int tx=scenePixel(e.x2),ty=scenePixel(e.y2),dx=tx-x,dy=ty-y;
            unsigned square=unsigned(dx*dx+dy*dy),length=0;while((length+1)*(length+1)<=square&&length<320)++length;
            const Sprite* lance=assets_->find("sLanceRed");
            if(lance&&length){uint8_t angle=direction(dx,dy);int scaleX=int(length)*256/lance->canvas_width,scaleY=sceneRadius(e.radius)*256/lance->canvas_height;
                if(scaleX<1)scaleX=1;
                if(scaleY<1)scaleY=1;
                canvas.sprite((x+tx)/2,(y+ty)/2,lance->pixels,lance->alpha,lance->width,lance->height,lance->width/2,lance->height/2,dirX(angle),dirY(angle),scaleX,65535,255,false,0,false,scaleY);
            }else canvas.line(x,y,tx,ty,Red);
        } else if(e.kind==FX_SWORD) {
            // Sword x2 stores the facing angle, not an endpoint coordinate.
            int sweep=-62+124*(e.max_life-e.life)/e.max_life;
            for(int side=-1;side<=1;side+=2) {
                uint8_t angle=uint8_t(int(e.x2)+sweep*side);
                int reach=sceneRadius(e.radius);
                int tx=x+dirX(angle)*reach/256,ty=y+dirY(angle)*reach/256;
                int bx=x+dirX(angle)*reach/768,by=y+dirY(angle)*reach/768;
                canvas.line(bx,by,tx,ty,White);
                if(effects)canvas.line(bx+1,by,tx+1,ty,color);
            }
        } else if(e.kind==FX_BEAM) {
            int tx=scenePixel(e.x2),ty=scenePixel(e.y2);
            int thickness=effects?sceneRadius(e.radius):1;if(thickness>5)thickness=5;
            int dx=tx-x,dy=ty-y;if(dx<0)dx=-dx;if(dy<0)dy=-dy;
            for(int n=0;n<thickness;++n) {
                int offset=n-thickness/2;
                int ox=dx>=dy?0:offset,oy=dx>=dy?offset:0;
                canvas.line(x+ox,y+oy,tx+ox,ty+oy,n==thickness/2?White:color);
                if(tx<0)canvas.line(x+ox+ScreenW,y+oy,tx+ox+ScreenW,ty+oy,color);
                if(tx>=ScreenW)canvas.line(x+ox-ScreenW,y+oy,tx+ox-ScreenW,ty+oy,color);
                if(ty<0)canvas.line(x+ox,y+oy+ScreenH,tx+ox,ty+oy+ScreenH,color);
                if(ty>=ScreenH)canvas.line(x+ox,y+oy-ScreenH,tx+ox,ty+oy-ScreenH,color);
                int wrapx=tx<0?ScreenW:tx>=ScreenW?-ScreenW:0;
                int wrapy=ty<0?ScreenH:ty>=ScreenH?-ScreenH:0;
                if(wrapx&&wrapy)canvas.line(x+ox+wrapx,y+oy+wrapy,tx+ox+wrapx,ty+oy+wrapy,color);
            }
        } else if(e.kind==FX_MUZZLE||e.kind==FX_HIT) {
            if(effects){canvas.line(x-2,y,x+2,y,White);canvas.line(x,y-2,x,y+2,color);}
        } else {
            if(radius<1)radius=1;
            canvas.circle(x,y,radius,color);
            if(effects && e.kind==FX_BURST) for(unsigned n=0;n<6;++n) {
                uint8_t a=uint8_t(n*43+i*11);
                canvas.pixel(x+dirX(a)*(radius+2)/256,y+dirY(a)*(radius+2)/256,color);
            }
        }
    }
    if(options_.visibility) {
        canvas.circle(px,py,sceneRadius(p.radius+5),rgb(1,3,8));
        canvas.circle(px,py,sceneRadius(p.radius+6),Cyan);
        if(!skin(0,px,py,sceneRadius(p.radius*2+10),p.angle)&&p.body<BODY_COUNT)
            sprite(bodies_[p.body],(g.frame/8)%3,px,py,sceneRadius(p.radius*2+10),p.angle);
    }
    Canvas::setFont(font_);
    for(unsigned i=0;i<24;++i){const FeedbackEvent& e=feedback_events[i];
        if(!e.active||g.frame-e.frame>30)continue;
        if((e.kind==FeedbackDamage&&!options_.damage_numbers)||(e.kind==FeedbackHealing&&!options_.healing_numbers)||(e.kind==FeedbackSelfDamage&&!options_.self_damage_numbers))continue;
        char value[16]={0};if(e.kind==FeedbackHealing)append(value,sizeof(value),"+");appendNumber(value,sizeof(value),e.value);
        canvas.text(scenePixel(e.x)-Canvas::textWidth(value)/2,scenePixel(e.y)-12-int((g.frame-e.frame)/3),value,e.kind==FeedbackHealing?Green:e.kind==FeedbackSelfDamage?Orange:White);
    }
    if(options_.shake&&g.damage_flash)shake(int((g.frame*17)%5)-2,int((g.frame*11)%5)-2);
    // One compact HUD row leaves most of 320x240 for steering and aiming.
    Canvas::setFont(font_);
    canvas.shade(0,0,320,22,rgb(1,5,12),190);
    char number[24]={0};decimal(number,unsigned(p.hull>0?p.hull:0));
    canvas.text(5,0,number,Green);
    decimal(number,unsigned(p.shield>0?p.shield:0));canvas.text(74,0,number,Cyan);
    canvas.bar(5,17,62,3,p.hull,p.max_hull,Green);
    canvas.bar(74,17,62,3,p.shield,p.max_shield,Cyan);
    char label[96]={0};append(label,sizeof(label),locale_?locale_->text("GUIWave","Wave"):"Wave");
    append(label,sizeof(label)," ");appendNumber(label,sizeof(label),g.wave);
    canvas.clipped(145,0,94,label,White);
    decimal(number,g.score);canvas.right(315,0,number,White);
    ui("sExpBar",160,236,216,65535,240);
    canvas.bar(53,235,214,3,int(g.xp),int(g.xp_next),Purple);
    label[0]=0;append(label,sizeof(label),locale_?locale_->text("GUILevel","Level "):"Level ");appendNumber(label,sizeof(label),g.level);
    canvas.clipped(5,216,83,label,Purple);
    if(g.banked_upgrades&&upgrade_prompt) {
        label[0]=0;append(label,sizeof(label),upgrade_key);append(label,sizeof(label),": ");append(label,sizeof(label),locale_?locale_->text("DefineKeyboardControlsUpgrade","Upgrade"):"Upgrade");
        append(label,sizeof(label)," ");appendNumber(label,sizeof(label),g.banked_upgrades);
        canvas.shade(88,215,142,18,rgb(8,13,30),225);canvas.clipped(92,216,134,label,White);
    }
    if(p.weapon==W_SALVO) {
        decimal(label,p.missiles);canvas.right(315,216,label,Cyan);
    } else if(p.charge>0)canvas.bar(255,225,58,3,p.charge,p.weapon==W_VORTEX?150:p.weapon==W_THERMAL_LANCE?90:180,Cyan);
    if(g.boss_index>=0 && g.boss_index<MAX_ENEMIES) {
        const Enemy& boss=g.enemies[g.boss_index];
        if(boss.active) {
            canvas.bar(80,24,160,3,boss.hull,boss.max_hull,Red);
            if(boss.type<ENEMY_COUNT) {
                const char* keys[ENEMY_COUNT]={"EnemyNameAsteroid","EnemyNameSwarmling","EnemyNameDrone","EnemyNameWasp","EnemyNameSpitfire","EnemyNameSkirmisher","EnemyNameBulwark","EnemyNameFrigate","EnemyNameSerpent","EnemyNameMine","EnemyNameWarden","EnemyNameBeamcaster","EnemyNameCherub","EnemyNameJuggernaut","EnemyNameMinelayer","EnemyNameTracer","EnemyNameScion","EnemyNameDreadnought","EnemyNameStationOmega","EnemyNameWarbringer","EnemyNameSeraph","EnemyNameStarEater","EnemyNameDweller","EnemyNameCeratotitan","EnemyNameGlaucus","EnemyNameCrucible"};
                canvas.centered(28,locale_?locale_->text(keys[boss.type],enemy_types[boss.type].name):enemy_types[boss.type].name,Red);
            }
        }
    }
    if(g.wave_banner) {
        label[0]=0;append(label,sizeof(label),locale_?locale_->text("GUIWave","Wave"):"Wave");
        append(label,sizeof(label)," ");appendNumber(label,sizeof(label),g.wave);
        canvas.centered(48,label,Cyan);
    }
    if(!g.screenWrappingEnabled()){
        bool entering=false;for(unsigned i=0;i<MAX_ENEMIES;++i)if(g.enemies[i].active&&g.enemies[i].type==E_CRUCIBLE&&g.enemies[i].age<180)entering=true;
        if(entering){canvas.box(0,21,320,217,Red);canvas.shade(20,47,280,19,rgb(1,3,8),220);canvas.centered(49,locale_?locale_->text("GUIWarpDisabled","Screen wrapping disabled"):"Screen wrapping disabled",Red);}
    }
    if(options_.speedrun){char timer[32]={0};unsigned seconds=g.frame/30;
        appendNumber(timer,sizeof(timer),seconds/60);append(timer,sizeof(timer),":");if(seconds%60<10)append(timer,sizeof(timer),"0");appendNumber(timer,sizeof(timer),seconds%60);
        append(timer,sizeof(timer),".");unsigned ms=(g.frame%30)*1000/30;if(ms<100)append(timer,sizeof(timer),"0");if(ms<10)append(timer,sizeof(timer),"0");appendNumber(timer,sizeof(timer),ms);
        canvas.shade(4,196,Canvas::textWidth(timer)+5,18,0,180);canvas.text(6,196,timer,Cyan);}
    if(g.damage_flash&&options_.flash)canvas.box(0,20,320,217,Red);
}

void Renderer::titleArt(uint32_t ticks) {
    menuBackground(ticks,true);
    if(logo_>=0) {
        unsigned frames=assets_->frames(unsigned(logo_));
        unsigned fps=assets_->playback_fps_q8(unsigned(logo_));
        if(!fps)fps=60*256;
        unsigned frame=ticks>10000?(frames?frames-1:0):ticks*fps/256000;
        if(frames && frame>=frames)frame=frames-1;
        const Sprite* logo=assets_->find_id(unsigned(logo_),frame);
        if(logo) {
            // Anchor to the unchanged pre-crop canvas, never the varying bounds
            // of the animated reveal. This prevents early frames zooming in.
            int scale=logo->canvas_width?190*256/logo->canvas_width:256;
            if(scale>1024)scale=1024;
            int w=logo->canvas_width*scale/256,h=logo->canvas_height*scale/256;
            // The source SPRT pivot is (132,50) on an 800x200 canvas.
            canvas.sprite(8+w*132/800,3+h/4,logo->pixels,logo->alpha,
                          logo->width,logo->height,logo->origin_x,logo->origin_y,256,0,scale,65535,255,true);
        }
    } else {
        Canvas::setFont(0);canvas.centered(34,"NOVA DRIFT",White,3);
    }
}

void Renderer::bodyIcon(unsigned body,int x,int y,int size) {
    if(body<BODY_COUNT && !sprite(bodies_[body],0,x,y,size))fallbackShip(x,y,0,size/3,White);
}
void Renderer::enemyIcon(unsigned enemy,int x,int y,int size) {
    if(enemy>=ENEMY_COUNT)return;
    if(composites_.preview(canvas,enemy,x,y,size))return;
    Enemy preview={};preview.type=uint8_t(enemy);preview.angle=192;
    if(composites_.draw(canvas,preview,x,y,size,0))return;
    if(!sprite(enemies_[enemy],0,x,y,size,192,65535,255,false,enemy==E_ASTEROID?1:0))fallbackShip(x,y,192,size/3,White);
}
void Renderer::icon(const Offer& offer,int x,int y,int size) {
    int id=-1;
    if(offer.kind==OFFER_WEAPON && offer.id<WEAPON_COUNT)id=weapon_icons_[offer.id];
    else if(offer.kind==OFFER_SHIELD && offer.id<SHIELD_COUNT)id=shield_icons_[offer.id];
    else if(offer.kind==OFFER_BODY && offer.id<BODY_COUNT)id=body_icons_[offer.id];
    else if(offer.kind==OFFER_MOD && offer.id<MOD_COUNT)id=mod_icons_[offer.id];
    if(id>=0&&ui(assets_->name(unsigned(id)),x,y,size))return;
    {
        uint16_t c=offer.kind==OFFER_MOD?Purple:Cyan;
        int r=size/3;
        for(unsigned n=0;n<6;++n) {
            uint8_t a=uint8_t(n*43),b=uint8_t((n+1)*43);
            canvas.line(x+dirX(a)*r/256,y+dirY(a)*r/256,x+dirX(b)*r/256,y+dirY(b)*r/256,c);
        }
    }
}
void Renderer::pointer(int x,int y,bool pressed) {
    if(ui("sCursorSmall",x+3,y+4,pressed?13:12))return;
    uint16_t c=pressed?White:Cyan;
    canvas.line(x-3,y,x+3,y,c);canvas.line(x,y-3,x,y+3,c);
    canvas.pixel(x,y,White);
}
}
