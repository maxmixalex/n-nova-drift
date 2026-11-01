// Included inside updateBossParts with e, owner and existing. Private native
// evidence: Other10, Alarm0, Tentacle Alarm0/Step and both Jaw Step events.
if(e.type==E_SNAPPER){
 const unsigned chains=e.elite?5u:3u,segments=e.elite>=2?12u:e.elite?10u:8u;
 const unsigned total=2+chains*(segments+1),variant=e.elite>=2?950u:e.elite?800u:700u;
 if(!existing){
  unsigned free=0;for(unsigned n=0;n<MAX_BOSS_PARTS;++n)free+=!boss_parts[n].max_hull;
  if(free<total)return;
  auto add=[&](unsigned kind,unsigned slot,unsigned hull,unsigned scale){
   for(unsigned n=0;n<MAX_BOSS_PARTS;++n)if(!boss_parts[n].max_hull){
    BossPart& p=boss_parts[n];clearBytes(&p,sizeof(p));p.owner=(uint8_t)owner;p.kind=(uint8_t)kind;p.slot=(uint8_t)slot;
    p.source_scale=(uint16_t)clamp((int64_t)scale*e.size_scale*65536/(6000*256),1,65535);
    const BossPartArt& art=bossPartArt(kind);p.radius=(uint8_t)clamp((maxv(art.bounds_width,art.bounds_height)*p.source_scale+131071)/131072,1,100);
    p.hull=p.max_hull=(int32_t)clamp((int64_t)hull*e.max_hull/500,1,2000000000);
    p.x=e.x;p.y=e.y;p.angle=e.angle;
    if(kind==BP_SNAPPER_JAW_LEFT||kind==BP_SNAPPER_JAW_RIGHT)p.local_x=45*256;
    else if(kind==BP_SNAPPER_BASE){int base=(e.elite>=2?-80:-70)+(int)(slot/16)*(e.elite>=2?40:e.elite?35:70);p.local_x=(int16_t)(base*128);p.local_y=(int16_t)(base*256);}
    return;
   }
  };
  add(BP_SNAPPER_JAW_LEFT,0,1500,550*variant/1000);add(BP_SNAPPER_JAW_RIGHT,1,1500,550*variant/1000);
  for(unsigned chain=0;chain<chains;++chain){add(BP_SNAPPER_BASE,chain*16,500,500*variant/1000);
   for(unsigned depth=1;depth<=segments;++depth)add(BP_SNAPPER_SEGMENT,chain*16+depth,120,720*(1000-575*depth/segments)/1000);
  }
 }
 int16_t links[5][13];for(unsigned c=0;c<5;++c)for(unsigned s=0;s<13;++s)links[c][s]=-1;
 for(unsigned n=0;n<MAX_BOSS_PARTS;++n){BossPart& p=boss_parts[n];if(!p.max_hull||p.owner!=owner)continue;
  if(p.kind==BP_SNAPPER_BASE||p.kind==BP_SNAPPER_SEGMENT){unsigned chain=p.slot/16,depth=p.slot%16;if(chain<5&&depth<13)links[chain][depth]=(int16_t)n;}
 }
 const auto center=[&](BossPart& p,int32_t ax,int32_t ay){const BossPartArt& a=bossPartArt(p.kind);int cx=(int)((int64_t)a.center_x*p.source_scale/256),cy=(int)((int64_t)a.center_y*p.source_scale/256);int co=dirX(p.angle),si=dirY(p.angle);p.x=wrap(ax+(cx*co-cy*si)/FP,worldWidth());p.y=wrap(ay+(cx*si+cy*co)/FP,worldHeight());};
 const auto anchor=[&](const BossPart& p,int32_t& x,int32_t& y){const BossPartArt& a=bossPartArt(p.kind);int cx=(int)((int64_t)a.center_x*p.source_scale/256),cy=(int)((int64_t)a.center_y*p.source_scale/256);int co=dirX(p.angle),si=dirY(p.angle);x=p.x-(cx*co-cy*si)/FP;y=p.y-(cx*si+cy*co)/FP;};
 unsigned head_scale=660*variant/1000;int head_offset=(int)((int64_t)-22*head_scale*e.size_scale*FP/(6000*256));
 int32_t head_x=e.x-dirX(e.angle)*head_offset/FP,head_y=e.y-dirY(e.angle)*head_offset/FP;
 for(unsigned n=0;n<MAX_BOSS_PARTS;++n){BossPart& p=boss_parts[n];if(!p.max_hull||p.owner!=owner||p.hull<=0)continue;
  if(p.kind!=BP_SNAPPER_JAW_LEFT&&p.kind!=BP_SNAPPER_JAW_RIGHT)continue;
  if(!e.age)continue;
  bool born=!p.active;p.active=1;if(p.flash)--p.flash;
  unsigned target=e.action_count==100?100u:0u;
  for(unsigned sub=0;sub<2;++sub){int divisor=target>unsigned(p.local_x/256)?80:4;p.local_x=(int16_t)(p.local_x+((int)target*256-p.local_x)/divisor);int extension=target?40*FP*e.size_scale/(6*256):0;p.local_y=(int16_t)(p.local_y+(extension-p.local_y)/divisor);}
  int side=p.kind==BP_SNAPPER_JAW_LEFT?-1:1;
  p.angle=(uint8_t)(e.angle+side*(int)p.local_x/360);
  int32_t ax=head_x-dirY(e.angle)*side*p.local_y/FP,ay=head_y+dirX(e.angle)*side*p.local_y/FP;
  center(p,ax,ay);if(born)p.timer=1;
 }
 for(unsigned chain=0;chain<chains;++chain){
  for(unsigned depth=0;depth<=segments;++depth){int index=links[chain][depth];if(index<0)continue;BossPart& p=boss_parts[index];if(p.hull<=0)continue;
   if(e.age*2<depth+1)continue;
   BossPart* previous=depth?links[chain][depth-1]>=0?&boss_parts[links[chain][depth-1]]:0:0;
   if(depth&&(!previous||previous->hull<=0)){p.hull=0;p.active=0;continue;}
   bool born=!p.active;p.active=1;if(p.flash)--p.flash;
   int32_t ax=head_x,ay=head_y;
   if(!depth){
    for(unsigned sub=0;sub<2;++sub){int target=e.phase?0:p.local_y,step=e.phase?640:64;if(p.local_x<target)p.local_x=(int16_t)minv(target,p.local_x+step);else if(p.local_x>target)p.local_x=(int16_t)maxv(target,p.local_x-step);}
    p.angle=(uint8_t)(e.angle+128+(int)p.local_x/360);
    int distance=(int)((int64_t)6993*FP*e.size_scale/(600*256)); // 37*1.2*2.1*.75 source pixels.
    ax+=dirX(p.angle)*distance/FP;ay+=dirY(p.angle)*distance/FP;
    center(p,ax,ay);p.timer=(uint16_t)p.angle*256;
   }else {
    anchor(*previous,ax,ay);unsigned spacing=44400*(1000-650*depth/segments)/1000;
    int distance=(int)((int64_t)spacing*FP*e.size_scale/(6000*256));
    ax+=dirX(previous->angle)*distance/FP;ay+=dirY(previous->angle)*distance/FP;
    if(born){p.angle=previous->angle;p.timer=(uint16_t)p.angle*256;center(p,ax,ay);}
    else for(unsigned sub=0;sub<2;++sub){int32_t oldx,oldy;anchor(p,oldx,oldy);int angular=(int16_t)((uint16_t)previous->angle*256-p.timer);p.timer=(uint16_t)(p.timer+angular/(e.phase?1:25));p.angle=(uint8_t)(p.timer/256);int32_t nx=oldx+delta(ax,oldx,worldWidth())*FP*(e.phase?1:2)/(e.phase?3:3);int32_t ny=oldy+delta(ay,oldy,worldHeight())*FP*(e.phase?1:2)/(e.phase?3:3);center(p,nx,ny);}
   }
  }
 }
 return;
}
