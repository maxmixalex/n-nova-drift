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
void Composites::initialize(Assets* assets,Game* scratch) {
    preview_count_=0;
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
    if(scratch) {
        const unsigned bosses[]={E_SCION,E_DREADNOUGHT,E_STATION_OMEGA,E_WARBRINGER,E_SERAPH,E_STAR_EATER,E_DWELLER,E_CERATOTITAN,E_GLAUCUS,E_CRUCIBLE,E_SPITFIRE,E_BULWARK,E_WORM,E_SNAPPER,E_ECLIPSE};
        for(unsigned type:bosses) {
            // Entries without independent live parts do not consume a slot.
            if(preview_count_>=sizeof(previews_)/sizeof(previews_[0]))break;
            scratch->start(0x50415254);scratch->player.x=240*FP;scratch->player.y=120*FP;
            for(unsigned i=0;i<MAX_ENEMIES;++i)scratch->enemies[i].active=0;
            int owner=scratch->spawnEnemy(type,260,120);if(owner<0)continue;
            scratch->next_wave=60000;scratch->player.invulnerable=60000;
            scratch->enemies[owner].angle=0;
            if(type==E_STAR_EATER){scratch->enemies[owner].form=2;scratch->enemies[owner].age=61;}
            // Advance the real part updater once before caching the complete
            // form. Arrival Star Eater intentionally has only its abdomen.
            for(unsigned step=0;step<(type==E_SNAPPER?40u:1u);++step){scratch->tick(GameInput{});}
            scratch->enemies[owner].boss_hidden=0;
            Preview& v=previews_[preview_count_];v.enemy=scratch->enemies[owner];v.count=0;
            int left=-int(v.enemy.radius),right=v.enemy.radius,top=left,bottom=right;
            for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(scratch->boss_parts[i].active&&scratch->boss_parts[i].owner==owner&&v.count<32) {
                BossPart p=scratch->boss_parts[i];p.x=(p.x-v.enemy.x);p.y=(p.y-v.enemy.y);
                if(type==E_SNAPPER){int wx=scratch->worldWidth()*FP,wy=scratch->worldHeight()*FP;if(p.x>wx/2)p.x-=wx;if(p.x<-wx/2)p.x+=wx;if(p.y>wy/2)p.y-=wy;if(p.y<-wy/2)p.y+=wy;}v.parts[v.count++]=p;
                const BossPartArt& art=bossPartArt(p.kind);
                int sx=p.source_scale?int((int64_t)art.source_width*p.source_scale/65536):(art.source_width+8)/9;
                int sy=p.source_scale?int((int64_t)art.source_height*p.source_scale/65536):(art.source_height+8)/9;
                int extent=(sx>sy?sx:sy)/2+3,px=pixel(p.x),py=pixel(p.y);
                if(p.kind==BP_GLAUCUS_SEGMENT&&(p.slot==1||p.slot==7))extent=45;
                if(px-extent<left)left=px-extent;
                if(px+extent>right)right=px+extent;
                if(py-extent<top)top=py-extent;
                if(py+extent>bottom)bottom=py+extent;
            }
            if(!v.count)continue;
            v.center_x=int16_t((left+right)/2);v.center_y=int16_t((top+bottom)/2);
            v.extent=int16_t((right-left)>(bottom-top)?right-left:bottom-top);if(v.extent<1)v.extent=1;
            ++preview_count_;
        }
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
    int scale=width*256/int(full);if(scale<1)scale=1;
    if(scale>1024)scale=1024;
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
void Composites::source(Canvas& c,const char* name,int x,int y,int sourceScale,uint8_t angle,bool mirror,uint16_t tint,unsigned opacity,bool additive,unsigned lighting,unsigned frame) {
    const Sprite* s=assets_->find(name,frame);if(!s||!s->canvas_width)return;
    int scale=int((int64_t(sourceScale)*s->original_width/s->canvas_width)/256);
    if(scale<1)scale=1;
    if(scale>1024)scale=1024;
    c.sprite(x,y,s->pixels,s->alpha,s->width,s->height,s->origin_x,s->origin_y,dirX(angle),dirY(angle),scale,tint,opacity,additive,lighting,mirror);
}
bool Composites::foreground(unsigned kind) {
    return kind==BP_DREAD_GUN_LARGE||kind==BP_DREAD_GUN_SMALL||kind==BP_WAR_BROADSIDE||kind==BP_WAR_LASER||kind==BP_CERA_PINCER||kind==BP_CERA_ELYTRON||kind==BP_GLAUCUS_EYE||kind==BP_STAR_EYE||kind==BP_STATION_NODE||kind==BP_BULWARK_BARRIER||kind==BP_WORM_HEAD||kind==BP_SNAPPER_JAW_LEFT||kind==BP_SNAPPER_JAW_RIGHT||kind==BP_ECLIPSE_SHIELD||kind==BP_CRUCIBLE_CORNER;
}
bool Composites::part(Canvas& c,const BossPart& p,const Enemy& owner,int x,int y,int projection,uint32_t tick) {
    if(!assets_||!assets_->ready()||!p.active||p.kind>=BP_COUNT||owner.boss_hidden)return false;
    const BossPartArt& art=bossPartArt(p.kind);
    if(!art.sprite||!art.bounds_width||!art.bounds_height)return false;
    // A common original-pixel scale aligns the body and its separate glow.
    // Collision-box center offsets are source metadata, not visual guesses.
    unsigned bounds=art.bounds_width>art.bounds_height?art.bounds_width:art.bounds_height;
    int scale=p.source_scale?int(int64_t(p.source_scale)*projection/256):int(int64_t(p.radius)*2*projection*256/bounds);
    int dx=int(int64_t(art.center_x)*scale/65536),dy=int(int64_t(art.center_y)*scale/65536);if(p.mirror)dy=-dy;
    int co=dirX(p.angle),si=dirY(p.angle);int ax=x-(dx*co-dy*si)/256,ay=y-(dx*si+dy*co)/256;
    bool chitin=p.kind==BP_STAR_SEGMENT||p.kind==BP_STAR_ABDOMEN;
    bool gold=p.kind>=BP_WAR_CHASSIS&&p.kind<=BP_WAR_LASER;
    if(p.kind==BP_GLAUCUS_SEGMENT&&(p.slot==1||p.slot==7)) {
        // Other_10 @0x140b1613b/162d1: round(28*.06)=2 and
        // round(28*.27)=8. The chain pool excludes the head at source index 0.
        const int factor=p.slot==1?700:450,count=p.slot==1?9:6,start=p.slot==1?-80:-40;
        int unit=projection*256*5/46,finScale=int(int64_t(unit)*factor*9285/10000000);
        for(int side=-1;side<=1;side+=2){
            int a=int(p.angle)-side*125*256/360;int length=25*unit/65536;
            int fx=x+dirX(uint8_t(a))*length/256,fy=y+dirY(uint8_t(a))*length/256;
            source(c,"sGlaucusFin",fx,fy,finScale,uint8_t(a),side<0);
            for(int finger=0;finger<count;++finger){
                int offset=finger-count/2;if(offset<0)offset=-offset;
                int fingerScale=int(int64_t(unit)*factor*(1250-150*offset)/1000000);
                int angle=int(p.angle)-side*(start+finger*20)*256/360;
                int radius=105*unit/65536*factor/1000;
                int px=fx+dirX(uint8_t(angle))*radius/256,py=fy+dirY(uint8_t(angle))*radius/256;
                source(c,"sGlaucusFinger",px,py,fingerScale,uint8_t(angle),side<0);
            }
        }
    }
    uint16_t tint=p.flash?rgb(255,242,222):gold?rgb(100,72,43):65535;
    const char* primary=art.sprite;
    unsigned frame=((p.kind>=BP_BULWARK_BARRIER&&p.kind<=BP_WORM_BODY)||p.kind==BP_SNAPPER_JAW_LEFT||p.kind==BP_SNAPPER_JAW_RIGHT||p.kind==BP_ECLIPSE_CHASSIS||p.kind==BP_ECLIPSE_PANEL)?owner.elite:0;
    if(p.kind==BP_BULWARK_BARRIER)primary=p.timer>=4000?"sBulwarkBarrierWhiteHot":p.timer>=1000?"sBulwarkBarrierHot":p.timer>=250?"sBulwarkBarrierWarm":"sBulwarkBarrierDim";
    if(p.kind==BP_CRUCIBLE_CORNER&&p.timer<=285)primary="sCrucibleCornerHot";
    if(p.kind==BP_CRUCIBLE_EDGE&&p.timer<=285)primary="sCrucibleEdgeHot";
    source(c,primary,ax,ay,scale,p.angle,p.mirror,tint,p.kind==BP_ECLIPSE_SHIELD?191:255,false,chitin?2:0,frame);
    const char* glow=0;
    switch(p.kind){
    case BP_WAR_CHASSIS:glow="sWarbringerChasisGlow";break;
    case BP_WAR_WING:glow="sWarbringerWingGlow";break;
    case BP_WAR_THRUSTER:glow="sWarbringerThrusterGlow";break;
    case BP_WAR_BROADSIDE:glow="sWarbringerBroadsideGlow";break;
    case BP_WAR_LASER:glow="sWarbringerLaserGlow";break;
    case BP_STATION_WALL:glow="sStationGlow";break;
    case BP_ECLIPSE_CHASSIS:glow="sEclipseChassisGlow";break;
    case BP_ECLIPSE_PANEL:glow="sEclipsePanelGlow";break;
    case BP_BULWARK_CORE:glow="sBulwarkBodyGlow";break;
    case BP_BULWARK_BODY2:glow="sBulwarkBody2Glow";break;
    case BP_BULWARK_BODY3:glow="sBulwarkBody3Glow";break;
    case BP_BULWARK_ARM:glow="sBulwarkArmGlow";break;
    case BP_BULWARK_TAIL:glow="sBulwarkTailGlow";break;
    case BP_BULWARK_THRUSTER:glow="sBulwarkThrusterGlow";break;
    case BP_BULWARK_THRUSTER_LARGE:glow="sBulwarkThrusterLargeGlow";break;
    default:break;
    }
    if(glow){source(c,glow,ax,ay,scale,p.angle,p.mirror,65535,255,true,0,frame);
        if(gold)source(c,glow,ax,ay,scale,p.angle,p.mirror,65535,220,true);
    }
    if(p.kind==BP_STAR_SEGMENT||p.kind==BP_STAR_ABDOMEN) {
        // Source limb sprites pivot at their root, so each pair stays attached
        // to its live segment and disappears with it. Slight articulation is
        // deterministic and never interpolates or blurs the source pixels.
        int sway=dirY(uint8_t(tick/35+p.slot*11))*4/256;
        const char* leg=p.kind==BP_STAR_ABDOMEN?"sStarEaterLegLarge":"sStarEaterLeg";
        for(int side=-1;side<=1;side+=2) {
            int off=int(int64_t(art.bounds_height)*scale/65536/3);
            int lx=x-si*side*off/256,ly=y+co*side*off/256;
            source(c,leg,lx,ly,scale,uint8_t(p.angle+side*sway),side<0,65535,255,false,2);
        }
    }
    return true;
}
bool Composites::core(Canvas& c,const Enemy& e,int x,int y,int projection,uint32_t tick) {
    if(!assets_||!assets_->ready())return false;
    const char* name=0;const char* glow=0;int cx=0,cy=0;uint16_t tint=65535;
    switch(e.type){
#include "composites_snapper.inc"
    case E_ECLIPSE:{int scale=(int)((int64_t)projection*e.size_scale*400/6000);source(c,"sEclipseCore",x,y,scale,e.angle,false,e.flash?rgb(255,242,222):65535);return true;}
    case E_JUGGERNAUT:{
        int scale=int(int64_t(projection)*e.size_scale*(e.elite>=2?575:e.elite?500:450)/6000);
        int ax=x-dirX(e.angle)*int(int64_t(32)*scale/65536)/256;
        int ay=y-dirY(e.angle)*int(int64_t(32)*scale/65536)/256;
        source(c,"sJuggernaut",ax,ay,scale,e.angle,false,e.flash?rgb(255,242,222):65535,255,false,0,e.elite);
        source(c,"sJuggernautGun",ax,ay,scale,uint8_t(uint16_t(e.formation_angle)/256),false);
        return true;
    }
    case E_BOLT_THROWER:{
        int scale=int(int64_t(projection)*e.size_scale/12);
        int cx=int(int64_t(21)*scale/65536),cy=-int(int64_t(scale)/65536);
        int ax=x-(cx*dirX(e.angle)-cy*dirY(e.angle))/256;
        int ay=y-(cx*dirY(e.angle)+cy*dirX(e.angle))/256;
        source(c,"sGauss",ax,ay,scale,e.angle,false,e.flash?rgb(255,242,222):65535,255,false,0,e.elite>=2?1:0);
        return true;
    }
    case E_DREADNOUGHT:name="sDreadnoughtCenter";cx=5;break;
    case E_WARBRINGER:name="sWarbringerCore";glow="sWarbringerCoreGlow";cx=-30;tint=e.flash?rgb(255,242,222):rgb(100,72,43);break;
    case E_CERATOTITAN:name="sCeratotitan";cx=74;break;
    case E_GLAUCUS:name="sGlaucusHead";cx=11;break;
    case E_CRUCIBLE:name="sCrucible";glow="sCrucibleSigil";cy=-9;break;
    case E_BULWARK:case E_WORM:case E_STAR_EATER:case E_STATION_OMEGA:return true; // Controllers; visible parts are separate.
    default:return false;
    }
    (void)tick;int scale=(int)((int64_t)projection*e.size_scale*5/46);int co=dirX(e.angle),si=dirY(e.angle);
    int dx=int(int64_t(cx)*scale/65536),dy=int(int64_t(cy)*scale/65536);x-=(dx*co-dy*si)/256;y-=(dx*si+dy*co)/256;
    source(c,name,x,y,scale,e.angle,false,tint);if(glow)source(c,glow,x,y,scale,e.angle,false,65535,255,true);return true;
}
bool Composites::preview(Canvas& c,unsigned type,int x,int y,int diameter,uint32_t tick) {
    for(unsigned at=0;at<preview_count_;++at){const Preview& v=previews_[at];if(v.enemy.type!=type)continue;
        int projection=diameter*256/v.extent;if(projection<1)projection=1;
        int cx=x-v.center_x*projection/256,cy=y-v.center_y*projection/256;
        for(unsigned pass=0;pass<2;++pass){
            if(pass)core(c,v.enemy,cx,cy,projection,tick);
            for(unsigned i=0;i<v.count;++i)if(foreground(v.parts[i].kind)==bool(pass))part(c,v.parts[i],v.enemy,cx+int(int64_t(v.parts[i].x)*projection/65536),cy+int(int64_t(v.parts[i].y)*projection/65536),projection,tick);
        }
        return true;
    }
    return false;
}

}
