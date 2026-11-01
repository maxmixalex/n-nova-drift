#include "composites.h"

namespace nova {
namespace {
enum Part {
    DCenter, DWing, DGunLarge, DGunSmall, DThrust,
    WChassis, WChassisGlow, WCore, WCoreGlow, WWing, WWingGlow,
    WThruster, WThrusterGlow,
    BBody, BBodyGlow, BArm, BArmGlow, BTail, BTailGlow,
    SAbdomen, SSegment, SLeg, SLegLarge, SEye, SAura, PartCount
};
const char* const names[PartCount] = {
    "sDreadnoughtCenter", "sDreadnoughtWing", "sDreadnoughtGunLarge",
    "sDreadnoughtGunSmall", "sDreadnoughtThrust",
    "sWarbringerChasis", "sWarbringerChasisGlow", "sWarbringerCore",
    "sWarbringerCoreGlow", "sWarbringerWing", "sWarbringerWingGlow",
    "sWarbringerThruster", "sWarbringerThrusterGlow",
    "sBulwarkBody", "sBulwarkBodyGlow", "sBulwarkArm", "sBulwarkArmGlow",
    "sBulwarkTail", "sBulwarkTailGlow", "sStarEaterAbdomen",
    "sStarEaterSegment", "sStarEaterLeg", "sStarEaterLegLarge",
    "sStarEaterEye", "sStarEaterAura"
};
const unsigned cached_parts[11] = {
    DWing, WWing, WWingGlow, BArm, BArmGlow, SLeg, SLegLarge,
    SAbdomen, SSegment, SLeg, SLegLarge
};
}
void Composites::initialize(Assets* assets) {
    assets_=assets;
    for(unsigned i=0;i<PartCapacity;++i){ids_[i]=-1;mirror_index_[i]=-1;lit_index_[i]=-1;}
    for(unsigned i=0;i<MirrorCapacity;++i)mirrors_[i].ready=false;
    if(!assets_||!assets_->ready())return;
    for(unsigned i=0;i<PartCount;++i)ids_[i]=assets_->index_of(names[i]);
    // Mirrored wings/limbs are cached once; combat does not flip or decompress
    // them every frame. Mirroring also reflects the cropped attachment pivot.
    for(unsigned slot=0;slot<MirrorCapacity;++slot) {
        unsigned part=cached_parts[slot];if(ids_[part]<0)continue;
        const Sprite* original=assets_->find_id(unsigned(ids_[part]),0);
        if(!original||original->width>MirrorEdge||original->height>MirrorEdge)continue;
        Mirror& mirror=mirrors_[slot];mirror.sprite=*original;
        unsigned w=original->width,h=original->height;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
            unsigned dst=y*w+x,src=(slot<7?h-1-y:y)*w+x;
            uint16_t color=original->pixels[src];
            if(part>=SAbdomen&&part<=SLegLarge) {
                // The original black chitin is normally lit by the PC shaders.
                // Small, fixed ambient light keeps its original edge detail
                // readable on the dark calculator backdrop. Geometry/alpha stay exact.
                unsigned r=(color>>11)*2+4,g=((color>>5)&63)*2+6,b=(color&31)*2+7;
                if(r>31)r=31;
                if(g>63)g=63;
                if(b>31)b=31;
                color=uint16_t((r<<11)|(g<<5)|b);
            }
            mirror.colors[dst]=color;mirror.alpha[dst]=original->alpha[src];
        }
        mirror.sprite.pixels=mirror.colors;mirror.sprite.alpha=mirror.alpha;
        if(slot<7)mirror.sprite.origin_y=int16_t(int(h)-1-original->origin_y);
        mirror.ready=true;
        if(slot<7)mirror_index_[part]=int8_t(slot);else lit_index_[part]=int8_t(slot);
    }
}
bool Composites::paint(Canvas& canvas,unsigned part,int x,int y,int diameter,
                       int localX,int localY,int extent,uint8_t heading,
                       int localAngle,uint16_t tint,unsigned opacity,
                       bool mirrored,bool additive) {
    if(!assets_||!assets_->ready()||part>=PartCount||ids_[part]<0)return false;
    const Sprite* image=0;
    if(mirrored) {
        int slot=mirror_index_[part];
        if(slot<0||!mirrors_[slot].ready)return false;
        image=&mirrors_[slot].sprite;
    }else if(lit_index_[part]>=0)image=&mirrors_[unsigned(lit_index_[part])].sprite;
    else image=assets_->find_id(unsigned(ids_[part]),0);
    if(!image)return false;
    unsigned full=image->canvas_width>image->canvas_height?image->canvas_width:image->canvas_height;
    if(!full)return false;
    int width=diameter*extent/1000;if(width<1)width=1;
    int scale=width*256/int(full);if(scale<1)scale=1;if(scale>1024)scale=1024;
    int dx=diameter*localX/1000,dy=diameter*localY/1000;
    int c=dirX(heading),s=dirY(heading);
    int px=x+(dx*c-dy*s)/256,py=y+(dx*s+dy*c)/256;
    uint8_t angle=uint8_t(int(heading)+localAngle);
    canvas.sprite(px,py,image->pixels,image->alpha,image->width,image->height,
                  image->origin_x,image->origin_y,dirX(angle),dirY(angle),scale,
                  tint,opacity,additive);
    // Two small additive glow passes approximate the original bloom and keep
    // the gold outline legible after downsampling to the calculator display.
    if(additive && part>=WChassisGlow && part<=BTailGlow)
        canvas.sprite(px,py,image->pixels,image->alpha,image->width,image->height,
                      image->origin_x,image->origin_y,dirX(angle),dirY(angle),scale,
                      tint,opacity,true);
    return true;
}
bool Composites::draw(Canvas& canvas,const Enemy& enemy,int x,int y,
                      int displayDiameter,uint32_t tick) {
    if(!assets_||!assets_->ready()||displayDiameter<1)return false;
    // Each assembly has its own normalization; layouts leave a margin for
    // antialiased edges and rotated limbs inside the supplied diameter.
    int d=displayDiameter;if(d<1)d=1;
    uint8_t angle=enemy.angle;
    uint16_t hull=enemy.flash?rgb(230,208,180):rgb(52,39,32);
    unsigned thrust=180+((tick/90)&1)*50;
    bool drawn=false;
    switch(enemy.type) {
    case E_DREADNOUGHT:
        paint(canvas,DThrust,x,y,d,-155,0,245,angle,128,65535,thrust,false,true);
        paint(canvas,DWing,x,y,d,65,0,460,angle);
        paint(canvas,DWing,x,y,d,65,0,460,angle,0,65535,255,true);
        drawn=paint(canvas,DCenter,x,y,d,65,0,440,angle);
        paint(canvas,DGunLarge,x,y,d,-50,-140,140,angle);
        paint(canvas,DGunLarge,x,y,d,-50,140,140,angle);
        paint(canvas,DGunSmall,x,y,d,175,-65,85,angle);
        paint(canvas,DGunSmall,x,y,d,175,65,85,angle);
        return drawn;
    case E_WARBRINGER:
        d=displayDiameter*9/8;
        for(int side=-1;side<=1;side+=2) {
            paint(canvas,WThruster,x,y,d,-155,side*160,230,angle,-64,hull);
            paint(canvas,WThrusterGlow,x,y,d,-155,side*160,230,angle,-64,65535,thrust,false,true);
            paint(canvas,WWing,x,y,d,25,side*80,250,angle,0,hull,255,side>0);
            paint(canvas,WWingGlow,x,y,d,25,side*80,250,angle,0,65535,255,side>0,true);
        }
        drawn=paint(canvas,WChassis,x,y,d,-150,0,610,angle,0,hull);
        paint(canvas,WChassisGlow,x,y,d,-150,0,610,angle,0,65535,255,false,true);
        paint(canvas,WCore,x,y,d,70,0,340,angle,0,hull);
        paint(canvas,WCoreGlow,x,y,d,70,0,340,angle,0,65535,255,false,true);
        return drawn;
    case E_BULWARK:
        d=displayDiameter*3/2;
        paint(canvas,BTail,x,y,d,-85,0,360,angle,0,hull);
        paint(canvas,BTailGlow,x,y,d,-85,0,360,angle,0,65535,255,false,true);
        for(int side=-1;side<=1;side+=2) {
            paint(canvas,BArm,x,y,d,-15,side*90,380,angle,side*12,hull,255,side<0);
            paint(canvas,BArmGlow,x,y,d,-15,side*90,380,angle,side*12,65535,255,side<0,true);
        }
        drawn=paint(canvas,BBody,x,y,d,-90,0,620,angle,0,hull);
        paint(canvas,BBodyGlow,x,y,d,-90,0,620,angle,0,65535,255,false,true);
        return drawn;
    case E_STAR_EATER: {
        paint(canvas,SAura,x,y,d,0,0,780,angle,0,65535,150,false,true);
        int sway=dirY(uint8_t(tick/35))*5/256;
        for(int side=-1;side<=1;side+=2) {
            paint(canvas,SLegLarge,x,y,d,-205,side*90,320,angle,side*sway,65535,255,side<0);
            paint(canvas,SLeg,x,y,d,-55,side*120,285,angle,-side*sway,65535,255,side<0);
            paint(canvas,SLeg,x,y,d,105,side*100,275,angle,side*sway,65535,255,side<0);
            paint(canvas,SLeg,x,y,d,250,side*75,235,angle,-side*sway,65535,255,side<0);
        }
        drawn=paint(canvas,SAbdomen,x,y,d,-315,0,430,angle);
        paint(canvas,SSegment,x,y,d,-160,0,405,angle);
        paint(canvas,SSegment,x,y,d,5,0,370,angle);
        paint(canvas,SSegment,x,y,d,165,0,315,angle);
        paint(canvas,SEye,x,y,d,295,0,170,angle,0,65535,255,false,true);
        return drawn;
    }
    default:return false;
    }
}
}
