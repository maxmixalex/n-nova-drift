// Eclipse Other_10 creates two independent chasses, two panels and a shield.
// Coordinates use the original .5 art scale and 1/6 source-world mapping.
if(e.type==E_ECLIPSE){
 if(!existing){
  unsigned free=0;for(unsigned j=0;j<MAX_BOSS_PARTS;++j)free+=!boss_parts[j].max_hull;
  if(free<5)return;
  const auto add=[&](unsigned kind,unsigned slot,unsigned hp,unsigned scale){
   for(unsigned j=0;j<MAX_BOSS_PARTS;++j)if(!boss_parts[j].max_hull){BossPart& p=boss_parts[j];clearBytes(&p,sizeof(p));
    p.active=1;p.owner=(uint8_t)owner;p.kind=(uint8_t)kind;p.slot=(uint8_t)slot;p.mirror=slot==1||slot==3;
    p.source_scale=(uint16_t)clamp((int64_t)scale*e.size_scale*65536/(6000*256),1,65535);
    p.hull=p.max_hull=(int32_t)clamp((int64_t)hp*e.max_hull/1200,1,2000000000);
    const BossPartArt& a=bossPartArt(kind);p.radius=(uint8_t)clamp((maxv(a.bounds_width,a.bounds_height)*p.source_scale+131071)/131072,1,100);
    p.local_x=(int16_t)((kind==BP_ECLIPSE_CHASSIS?30+5*e.elite:kind==BP_ECLIPSE_PANEL?35+14*e.elite:0)*256);
    p.x=e.x;p.y=e.y;p.angle=e.angle;return;
   }
  };
  // The shield is encountered first by swept projectile collision.
  add(BP_ECLIPSE_SHIELD,4,1900,375+90*e.elite);
  add(BP_ECLIPSE_CHASSIS,0,3200,500);add(BP_ECLIPSE_CHASSIS,1,3200,500);
  add(BP_ECLIPSE_PANEL,2,1800,500);add(BP_ECLIPSE_PANEL,3,1800,500);
 }
 for(unsigned j=0;j<MAX_BOSS_PARTS;++j){BossPart& p=boss_parts[j];if(!p.active||p.owner!=owner)continue;
  if(p.flash)--p.flash;
  if(p.kind==BP_ECLIPSE_SHIELD){p.timer=(uint16_t)maxv(0,(int)p.timer-2);p.local_y=(int16_t)maxv(0,(int)p.local_y-2);}
  int target=p.kind==BP_ECLIPSE_CHASSIS?(30+5*e.elite)*256:p.kind==BP_ECLIPSE_PANEL?(35+14*e.elite)*256:0;
  if(e.phase)target=target*(p.kind==BP_ECLIPSE_CHASSIS?3:4)/2;
  for(unsigned sub=0;sub<2;++sub)p.local_x=(int16_t)(p.local_x+(target-p.local_x)/7);
  const BossPartArt& a=bossPartArt(p.kind);int co=dirX(e.angle),si=dirY(e.angle);int sign=p.mirror?1:-1;
  int extension=(int)((int64_t)p.local_x*e.size_scale/(6*256));
  int cx=(int)((int64_t)a.center_x*p.source_scale/256),cy=(int)((int64_t)a.center_y*p.source_scale/256);if(p.mirror)cy=-cy;
  int32_t ax=e.x-si*sign*extension/FP,ay=e.y+co*sign*extension/FP;
  p.x=wrap(ax+(cx*co-cy*si)/FP,worldWidth());p.y=wrap(ay+(cx*si+cy*co)/FP,worldHeight());p.angle=e.angle;
 }
 return;
}
