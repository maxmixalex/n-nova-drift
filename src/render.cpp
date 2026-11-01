#include "render.h"

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

void Renderer::initialize(Assets* assets) {
    assets_=assets;
    composites_.initialize(assets_);
    for(unsigned i=0;i<BODY_COUNT;++i)bodies_[i]=assets_->index_of(bodies[i].sprite);
    for(unsigned i=0;i<ENEMY_COUNT;++i)enemies_[i]=assets_->index_of(enemy_types[i].sprite);
    for(unsigned i=0;i<WEAPON_COUNT;++i)weapon_icons_[i]=assets_->index_of(weapons[i].sprite);
    for(unsigned i=0;i<SHIELD_COUNT;++i)shield_icons_[i]=assets_->index_of(shields[i].sprite);
    for(unsigned i=0;i<MOD_COUNT;++i) {
        Offer offer={OFFER_MOD,uint8_t(i)};
        mod_icons_[i]=assets_->index_of(offerSprite(offer));
    }
    const char* bulletNames[13]={"sDefaultWeapon","sBulletBlue","sFlakBlue","sRailgunBlue",
        "sGrenade","sSalvoBlue","sVortexCoreBlue","sTorrentBlue","sDartBlue",
        "sBulletBlue","sBulletRed","sRailShardBlue","sBlade"};
    for(unsigned i=0;i<13;++i)bullets_[i]=assets_->index_of(bulletNames[i]);
    const char* constructNames[9]={"sConstructDrone","sConstructDroneDefense","sMine",
        "sTurret","sConstructAlly","sConstructAllyInterceptorArmored","sConstructAllyGuardianArmored","sCarrierDrone","sBlade"};
    for(unsigned i=0;i<9;++i)constructs_[i]=assets_->index_of(constructNames[i]);
    const char* pickupNames[4]={"s_power_white","s_power_mega","s_power_gold","s_power_gold"};
    for(unsigned i=0;i<4;++i)pickups_[i]=assets_->index_of(pickupNames[i]);
    logo_=assets_->index_of("sNovaDriftLogo");
    planet_=assets_->index_of("sMenuPlanet");
    nebula_=assets_->index_of("sMenuNebula");
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
                      uint16_t tint,unsigned opacity,bool additive) {
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
                  dirX(angle),dirY(angle),scale,tint,opacity,additive);
    return true;
}

void Renderer::background(uint32_t ticks,bool animated) {
    for(int i=0;i<ScreenW*ScreenH;++i)canvas.pixels[i]=background_[i];
    uint32_t random=0x91e10da5;
    for(int i=0;i<56;++i) {
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

void Renderer::world(const Game& g,uint32_t ticks,bool effects) {
    projection=ScreenW*256/g.worldWidth();
    background(ticks,true);
    for(unsigned i=0;i<MAX_PICKUPS;++i) {
        const Pickup& p=g.pickups[i];if(!p.active)continue;
        int x=scenePixel(p.x),y=scenePixel(p.y);
        unsigned kind=p.kind<4?p.kind:0;
        if(!sprite(pickups_[kind],0,x,y,sceneRadius(kind==PICKUP_XP?6:9))) {
            uint16_t color=kind==PICKUP_HEAL?Green:Cyan;
            canvas.line(x-2,y,x,y-2,color);canvas.line(x,y-2,x+2,y,color);
            canvas.line(x+2,y,x,y+2,color);canvas.line(x,y+2,x-2,y,color);
        }
    }
    for(unsigned i=0;i<MAX_ENEMIES;++i) {
        const Enemy& e=g.enemies[i];if(!e.active||e.type>=ENEMY_COUNT)continue;
        int x=scenePixel(e.x),y=scenePixel(e.y);
        unsigned frame=(g.frame/6)%3;
        int diameter=sceneRadius(e.radius*2+5);
        uint16_t tint=e.flash?White:65535;
        if(!composites_.draw(canvas,e,x,y,diameter,ticks)&&
           !sprite(enemies_[e.type],frame,x,y,diameter,e.angle,tint))
            fallbackShip(x,y,e.angle,sceneRadius(e.radius),e.elite?Red:Orange);
        if(e.elite)canvas.circle(x,y,sceneRadius(e.radius+2),Purple);
        if(e.burn_time && effects) {
            int off=int((g.frame+i)%5)-2;
            canvas.pixel(x+off,y-sceneRadius(e.radius),Orange);
            canvas.pixel(x-off,y+sceneRadius(e.radius),Red);
        }
        if(e.hull<e.max_hull && int(i)!=g.boss_index)
            canvas.bar(x-sceneRadius(e.radius),y-sceneRadius(e.radius)-4,sceneRadius(e.radius*2),2,e.hull,e.max_hull,e.elite?Purple:Orange);
    }
    for(unsigned i=0;i<MAX_CONSTRUCTS;++i) {
        const Construct& c=g.constructs[i];if(!c.active)continue;
        unsigned kind=c.kind<9?c.kind:0;
        if(!sprite(constructs_[kind],0,scenePixel(c.x),scenePixel(c.y),sceneRadius(c.radius*2+5),c.angle))
            fallbackShip(scenePixel(c.x),scenePixel(c.y),c.angle,sceneRadius(c.radius),Cyan);
    }
    if(g.player.body==B_LEVIATHAN) for(unsigned remaining=g.segment_count;remaining>0;--remaining) {
        unsigned i=remaining-1;
        if(i>=MAX_SEGMENTS)continue;
        int r=sceneRadius(6-int(i)/7);if(r<2)r=2;
        canvas.disk(scenePixel(g.segment_x[i]),scenePixel(g.segment_y[i]),r,rgb(38,58,75));
        canvas.circle(scenePixel(g.segment_x[i]),scenePixel(g.segment_y[i]),r,White);
    }
    const Player& p=g.player;
    int px=scenePixel(p.x),py=scenePixel(p.y);
    if(p.shield>0) {
        uint16_t shieldColor=p.shield_type==S_HALO?Orange:p.shield_type==S_TEMPORAL?Purple:Cyan;
        int radius=sceneRadius(p.radius+6);
        if(p.shield_type==S_BASTION) {
            uint8_t prev=uint8_t(p.angle-48);
            int lastx=px+dirX(prev)*radius/256,lasty=py+dirY(prev)*radius/256;
            for(unsigned n=1;n<=12;++n) {
                uint8_t angle=uint8_t(p.angle-48+n*8);
                int x=px+dirX(angle)*radius/256,y=py+dirY(angle)*radius/256;
                canvas.line(lastx,lasty,x,y,shieldColor);lastx=x;lasty=y;
            }
        } else canvas.circle(px,py,radius,Canvas::blend(rgb(0,0,0),shieldColor,150));
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
        int x=px-dirX(p.angle)*(sceneRadius(p.radius)+length)/256;
        int y=py-dirY(p.angle)*(sceneRadius(p.radius)+length)/256;
        canvas.line(px-dirX(p.angle)*sceneRadius(p.radius)/256,py-dirY(p.angle)*sceneRadius(p.radius)/256,x,y,
                    p.body==B_FIREFLY?Orange:Cyan);
        canvas.pixel(x,y,White);
    }
    if(!p.invulnerable || (g.frame&2)==0) {
        unsigned opacity=p.cloak>30?100:255;
        if(p.body>=BODY_COUNT || !sprite(bodies_[p.body],(g.frame/8)%3,px,py,sceneRadius(p.radius*2+10),p.angle,65535,opacity))
            fallbackShip(px,py,p.angle,sceneRadius(p.radius+4),White);
    }
    for(unsigned i=0;i<MAX_BULLETS;++i) {
        const Bullet& b=g.bullets[i];if(!b.active)continue;
        int x=scenePixel(b.x),y=scenePixel(b.y);
        unsigned kind=b.kind<13?b.kind:0;
        int size=sceneRadius(b.radius*2+4);
        if(kind==SHOT_RAIL)size=sceneRadius(21);
        else if(kind==SHOT_VORTEX)size=sceneRadius(b.radius*2+8);
        else if(size<8)size=8;
        if(b.owner)kind=SHOT_ENEMY;
        if(!sprite(bullets_[kind],b.age/3,x,y,size,b.angle)) {
            uint16_t color=b.owner?Red:Cyan;
            canvas.line(x,y,x-scenePixel(b.vx),y-scenePixel(b.vy),color);
            if(b.radius>2)canvas.circle(x,y,sceneRadius(b.radius),color);
            else canvas.pixel(x,y,White);
        }
    }
    for(unsigned i=0;i<MAX_EFFECTS;++i) {
        const Effect& e=g.effects[i];if(!e.active||!e.max_life)continue;
        int x=scenePixel(e.x),y=scenePixel(e.y);
        uint16_t color=e.color==1?Orange:e.color==2?Purple:e.color==3?Red:Cyan;
        unsigned opacity=unsigned(e.life)*255/e.max_life;
        color=Canvas::blend(rgb(3,9,18),color,opacity);
        int radius=sceneRadius(e.radius)*(e.max_life-e.life+1)/e.max_life;
        if(e.kind==FX_SWORD) {
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
    // One compact HUD row leaves most of 320x240 for steering and aiming.
    Canvas::setFont(0);
    canvas.shade(0,0,320,20,rgb(1,5,12),200);
    char number[24]={0};decimal(number,unsigned(p.hull>0?p.hull:0));
    canvas.text(5,3,number,Green);
    decimal(number,unsigned(p.shield>0?p.shield:0));canvas.text(83,3,number,Cyan);
    canvas.bar(5,13,70,3,p.hull,p.max_hull,Green);
    canvas.bar(83,13,70,3,p.shield,p.max_shield,Cyan);
    char label[36]="WAVE ";appendNumber(label,sizeof(label),g.wave);
    canvas.text(166,3,label,White);
    decimal(number,g.score);canvas.right(315,3,number,White);
    canvas.bar(0,237,320,3,int(g.xp),int(g.xp_next),Purple);
    label[0]=0;append(label,sizeof(label),"LV ");appendNumber(label,sizeof(label),g.level);
    canvas.text(5,225,label,Purple);
    if(g.banked_upgrades) {
        label[0]=0;append(label,sizeof(label),"U: UPGRADE ");appendNumber(label,sizeof(label),g.banked_upgrades);
        canvas.fill(49,222,122,13,rgb(15,34,54));canvas.text(54,225,label,White);
    }
    if(p.weapon==W_SALVO) {
        label[0]=0;append(label,sizeof(label),"SALVO ");appendNumber(label,sizeof(label),p.missiles);
        canvas.right(315,225,label,Cyan);
    } else if(p.charge>0)canvas.bar(255,227,58,3,p.charge,p.weapon==W_VORTEX?150:p.weapon==W_THERMAL_LANCE?90:180,Cyan);
    if(g.boss_index>=0 && g.boss_index<MAX_ENEMIES) {
        const Enemy& boss=g.enemies[g.boss_index];
        if(boss.active) {
            canvas.bar(80,24,160,3,boss.hull,boss.max_hull,Red);
            if(boss.type<ENEMY_COUNT)canvas.centered(31,enemy_types[boss.type].name,Red);
        }
    }
    if(g.wave_banner) {
        canvas.centered(57,g.waveName(),White);
        label[0]=0;append(label,sizeof(label),"WAVE ");appendNumber(label,sizeof(label),g.wave);
        canvas.centered(45,label,Cyan);
    }
    if(g.damage_flash)canvas.box(0,20,320,217,Red);
}

void Renderer::titleArt(uint32_t ticks) {
    background(ticks,true);
    sprite(planet_,0,280,155,150,0,65535,180);
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
            int scale=logo->canvas_width?270*256/logo->canvas_width:256;
            if(scale>1024)scale=1024;
            int w=logo->canvas_width*scale/256,h=logo->canvas_height*scale/256;
            // The source SPRT pivot is (132,50) on an 800x200 canvas.
            canvas.sprite((320-w)/2+w*132/800,32+(60-h)/2+h/4,logo->pixels,logo->alpha,
                          logo->width,logo->height,logo->origin_x,logo->origin_y,256,0,scale,65535,255,true);
        }
    } else {
        Canvas::setFont(0);canvas.centered(34,"NOVA DRIFT",White,3);
    }
}

void Renderer::bodyIcon(unsigned body,int x,int y,int size) {
    if(body<BODY_COUNT && !sprite(bodies_[body],0,x,y,size))fallbackShip(x,y,0,size/3,White);
}
void Renderer::icon(const Offer& offer,int x,int y,int size) {
    int id=-1;
    if(offer.kind==OFFER_WEAPON && offer.id<WEAPON_COUNT)id=weapon_icons_[offer.id];
    else if(offer.kind==OFFER_SHIELD && offer.id<SHIELD_COUNT)id=shield_icons_[offer.id];
    else if(offer.kind==OFFER_BODY && offer.id<BODY_COUNT)id=bodies_[offer.id];
    else if(offer.kind==OFFER_MOD && offer.id<MOD_COUNT)id=mod_icons_[offer.id];
    if(!sprite(id,0,x,y,size)) {
        uint16_t c=offer.kind==OFFER_MOD?Purple:Cyan;
        int r=size/3;
        for(unsigned n=0;n<6;++n) {
            uint8_t a=uint8_t(n*43),b=uint8_t((n+1)*43);
            canvas.line(x+dirX(a)*r/256,y+dirY(a)*r/256,x+dirX(b)*r/256,y+dirY(b)*r/256,c);
        }
    }
}
void Renderer::pointer(int x,int y,bool pressed) {
    uint16_t c=pressed?White:Cyan;
    canvas.line(x-3,y,x+3,y,c);canvas.line(x,y-3,x,y+3,c);
    canvas.pixel(x,y,White);
}
}
