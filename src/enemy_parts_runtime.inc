// Included inside Game::updateBossParts. Source ordinary-enemy space is 1/6.
// Geometry is from SPRT, and independent hull from enemyInit/buildBulwarkPart.
const int32_t unit_hull=e.max_hull;
const auto nativePut=[&](unsigned kind,unsigned slot,int sx100,int sy100,unsigned hull,
                         unsigned scale=500,bool mirrored=false)->int {
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(!boss_parts[i].max_hull){
  BossPart &p=boss_parts[i];clearBytes(&p,sizeof(p));
  p.active=1;p.owner=(uint8_t)owner;p.kind=(uint8_t)kind;p.slot=(uint8_t)slot;p.mirror=mirrored;
  if(e.type==E_BULWARK){unsigned body_scale=e.elite>=2?500u:e.elite?425u:350u;scale=scale*body_scale/500;sx100=sx100*int(body_scale)/500;sy100=sy100*int(body_scale)/500;}
  p.source_scale=(uint16_t)(scale*65536u/6000u);
  p.local_x=(int16_t)clamp(sx100*FP/600,-32767,32767);p.local_y=(int16_t)clamp(sy100*FP/600,-32767,32767);
  const BossPartArt &a=bossPartArt(kind);
  p.radius=(uint8_t)clamp((maxv(a.bounds_width,a.bounds_height)*p.source_scale+65535)/131072,2,90);
  p.hull=p.max_hull=(int32_t)clamp((int64_t)hull*unit_hull/maxv(1,enemy_types[e.type].hull),(int64_t)1,(int64_t)2000000000);
  p.x=e.x;p.y=e.y;p.angle=e.angle;if(e.type==E_WORM&&slot&&e.elite==1)p.timer=(uint16_t)(200+(slot+1)/2);return (int)i;
 }
 return -1;
};
if(e.type==E_BOLT_THROWER){
 // Other_10 schedules Alarm 2 at source frame 1 for elite/champion only.
 // A destroyed jaw is never rebuilt; its Destroy event kills the leader.
 if(!e.elite||!e.age)return;
 if(!existing){
  unsigned free=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)free+=!boss_parts[i].max_hull;
  if(free<2)return;
  unsigned first=e.elite>=2?BP_BOLT_JAW_LEFT:BP_BOLT_JAW_LEFT_ELITE;
  nativePut(first,0,0,0,100);nativePut(first+1,1,0,0,100);
 }
 const unsigned source_age=e.age*2;
 unsigned mouth=0;
 if(e.elite>=2){if(source_age>=30&&source_age<220)mouth=45;}
 else if(source_age>=150&&source_age<280)mouth=source_age>=240?60:source_age>=195?40:20;
 const int behind=(e.elite>=2?20:16)*FP/6;
 const int center_offset=21*FP/12;
 const int32_t ax=e.x-dirX(e.angle)*(behind+center_offset)/FP,ay=e.y-dirY(e.angle)*(behind+center_offset)/FP;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i){BossPart &p=boss_parts[i];
  if(!p.active||p.owner!=owner||p.kind<BP_BOLT_JAW_LEFT||p.kind>BP_BOLT_JAW_RIGHT_ELITE)continue;
  if(p.flash)--p.flash;
  // Keep fractional degrees for the original .1 easing at 60 Hz.
  for(unsigned step=0;step<2;++step)p.timer=(uint16_t)((int)p.timer+((int)mouth*256-p.timer)/10);
  int angular=int(p.timer)/360;p.angle=(uint8_t)(e.angle+(p.slot?angular:-angular));
  const BossPartArt &art=bossPartArt(p.kind);int c=dirX(p.angle),sn=dirY(p.angle);
  int cx=(int)((int64_t)art.center_x*p.source_scale/256),cy=(int)((int64_t)art.center_y*p.source_scale/256);
  p.x=wrap(ax+(cx*c-cy*sn)/FP,worldWidth());p.y=wrap(ay+(cx*sn+cy*c)/FP,worldHeight());
 }
 return;
}
if(!existing){
 if(e.type==E_BULWARK){
  // Barrier first: a projectile meeting the shield cannot hit the hull behind it.
  nativePut(BP_BULWARK_BARRIER,0,0,0,20000);
  nativePut(BP_BULWARK_CORE,1,0,0,5000);
  nativePut(BP_BULWARK_BODY2,2,2900,0,3000);
  if(e.elite)nativePut(BP_BULWARK_BODY3,3,-2900,0,3000);
  nativePut(BP_BULWARK_ARM,4,0,10300,2500);
  nativePut(BP_BULWARK_ARM,5,0,-10300,2500,500,true);
  nativePut(BP_BULWARK_TAIL,6,e.elite?-8300:-2500,0,2500);
  const int lengths[]={4500,8600,16300},angles[]={102,144,162};
  for(unsigned pair=0;pair<(e.elite?3u:2u);++pair)for(unsigned side=0;side<2;++side){
   uint8_t a=(uint8_t)((side?-1:1)*angles[pair]*256/360);
   int root=pair==2?-2900:2900;
   nativePut(pair?BP_BULWARK_THRUSTER:BP_BULWARK_THRUSTER_LARGE,7+pair*2+side,
             root+dirX(a)*lengths[pair]/FP,dirY(a)*lengths[pair]/FP,1750,375,side!=0);
  }
 }else{
  // Other_11 decrements `segments` and instance_copy()s that many bodies;
  // the original head remains, so the elite count is 65 + 1, not 65 total.
  unsigned count=e.elite>=2?66u:e.elite?41u+range(21):21u+range(11);
  unsigned scale=e.elite>=2?375u:e.elite?362u:350u;
  unsigned spacing=e.elite>=2?34u:e.elite?33u:25u;
  unsigned made=0;
  for(unsigned j=0;j<count;++j){int at=nativePut(j?BP_WORM_BODY:BP_WORM_HEAD,j,0,0,220,scale);
   if(at<0)break;
   BossPart &p=boss_parts[at];p.local_x=p.local_y=0;
   p.x=wrap(e.x-(int64_t)dirX(e.angle)*int(j*spacing)/6,worldWidth());
   p.y=wrap(e.y-(int64_t)dirY(e.angle)*int(j*spacing)/6,worldHeight());++made;
  }
  e.action_count=(uint8_t)made;e.formation_slot=0;
  e.hull=e.max_hull=(int32_t)minv((int64_t)2000000000,(int64_t)unit_hull*made);
 }
}
if(e.type==E_WORM){
 int previous=-1;unsigned previous_slot=999;
 unsigned first_slot=999;int32_t first_x=e.x,first_y=e.y;
 const int spacing=(e.elite>=2?34:e.elite?33:25)*FP*e.size_scale/(6*256);
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i){BossPart &p=boss_parts[i];
  if(!p.active||p.owner!=owner||p.kind<BP_WORM_HEAD||p.kind>BP_WORM_BODY)continue;
  if(p.flash)--p.flash;
  bool head=previous<0||p.slot!=previous_slot+1;
  if(head&&p.kind==BP_WORM_BODY)p.timer=0;
  p.kind=(uint8_t)(head?BP_WORM_HEAD:BP_WORM_BODY);
  const BossPartArt &a=bossPartArt(p.kind);
  p.radius=(uint8_t)clamp((maxv(a.bounds_width,a.bounds_height)*p.source_scale+65535)/131072,2,90);
  if(head){
   if(first_slot==999){
    first_slot=p.slot;
    if(p.slot!=e.formation_slot){e.x=p.x;e.y=p.y;e.angle=p.angle;e.formation_slot=p.slot;}
    p.x=e.x;p.y=e.y;p.angle=e.angle;p.local_x=(int16_t)e.vx;p.local_y=(int16_t)e.vy;
    first_x=p.x;first_y=p.y;
   }else{
    uint8_t target=direction(delta(player.x,p.x,worldWidth()),delta(player.y,p.y,worldHeight()));
    p.angle=toward(p.angle,target,1);
    int speed=(e.elite>=2?832:e.elite?1024:768)/3;
    p.local_x=(int16_t)((p.local_x*7+dirX(p.angle)*speed/FP)/8);
    p.local_y=(int16_t)((p.local_y*7+dirY(p.angle)*speed/FP)/8);
    p.x=wrap(p.x+p.local_x,worldWidth());p.y=wrap(p.y+p.local_y,worldHeight());
   }
  }else{
   const BossPart &leader=boss_parts[previous];
   int dx=delta(leader.x,p.x,worldWidth()),dy=delta(leader.y,p.y,worldHeight());
   uint8_t heading=direction(dx,dy);
   int32_t ex=dx*FP-dirX(heading)*spacing/FP,ey=dy*FP-dirY(heading)*spacing/FP;
   // Two 60 Hz spring steps are combined; .89 damping squared is .7921.
   p.local_x=(int16_t)clamp(p.local_x*79/100+ex/10,-6*FP,6*FP);
   p.local_y=(int16_t)clamp(p.local_y*79/100+ey/10,-6*FP,6*FP);
   p.x=wrap(p.x+p.local_x,worldWidth());p.y=wrap(p.y+p.local_y,worldHeight());p.angle=heading;
  }
  previous=(int)i;previous_slot=p.slot;
 }
 e.x=first_x;e.y=first_y;
}else{
 bool body2=false,body3=false;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(boss_parts[i].owner==owner&&boss_parts[i].active){body2|=boss_parts[i].kind==BP_BULWARK_BODY2;body3|=boss_parts[i].kind==BP_BULWARK_BODY3;}
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i){BossPart &p=boss_parts[i];
  if(!p.active||p.owner!=owner)continue;
  if((p.kind==BP_BULWARK_BODY3&&!body2)||(p.kind==BP_BULWARK_TAIL&&(!body2||(e.elite&&!body3)))){p.active=0;p.hull=0;emitEffect(p.x,p.y,FX_BURST,p.radius,12,1);continue;}
  if(p.flash)--p.flash;
  const BossPartArt &a=bossPartArt(p.kind);
  p.angle=e.angle;
  int c=dirX(e.angle),s=dirY(e.angle);
  int32_t bx=e.x+(p.local_x*c-p.local_y*s)/FP,by=e.y+(p.local_x*s+p.local_y*c)/FP;
  int cx=(int)((int64_t)a.center_x*p.source_scale/256);
  int cy=(int)((int64_t)a.center_y*p.source_scale/256);if(p.mirror)cy=-cy;
  p.x=wrap(bx+(cx*c-cy*s)/FP,worldWidth());p.y=wrap(by+(cx*s+cy*c)/FP,worldHeight());
 }
}
