// Source evidence: docs/evidence/hazard_ai.csv. Every duration below is in
// port ticks (two source frames). See docs/enemy-hazards-parity.md for the
// explicit movement/shape/finite-pool adaptations; these are separate attacks,
// not the former generic fireEnemy fan.
case E_MINELAYER:{
 if(!e.form){e.form=1;e.fire_delay=rarity==1?207:75;e.target_x=13*FP;e.action_count=0;}
 // Spawn the finite escort/shield group once. A full pool retries instead of
 // silently losing a required attachment or growing replacements after death.
 unsigned attachments=rarity==2?3:1;
 if(e.action_count<attachments){unsigned kind=e.action_count?E_MINE_DRONE:E_MINE_SHIELD;
  int n=summon(kind,e.action_count?e.action_count-1:0,e.action_count?2:1,e.action_count?13:0,0);
  if(n>=0){enemies[n].elite=e.elite;enemies[n].radius=(uint8_t)(kind==E_MINE_SHIELD?(rarity==2?31:rarity?26:22):7);++e.action_count;}
 }
 e.target_x=maxv(rarity==2?122:256,e.target_x*9506/10000);
 move((uint8_t)(aim+(distance<100?90:35)),e.target_x,1);
 if(rarity<2){if(e.fire_delay)--e.fire_delay;if(!e.fire_delay){
  uint8_t a=(uint8_t)(e.angle+128);int n=spawnEnemy(rarity?E_SINGULARITY_MINE:E_STEALTH_MINE,pixel(e.x)+dirX(a)*6/FP,pixel(e.y)+dirY(a)*6/FP);
  if(n>=0){Enemy &mine=enemies[n];mine.angle=a;int speed=rarity?192:128;mine.vx=dirX(a)*speed/FP;mine.vy=dirY(a)*speed/FP;mine.damage_scale=e.damage_scale;e.fire_delay=rarity?275:100;}
 }}return true;
}
case E_MINE_SHIELD:{
 if(e.leader<0||e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation){e.active=0;return true;}
 const Enemy &mother=enemies[(unsigned)e.leader];e.x=mother.x;e.y=mother.y;e.angle=mother.angle;return true;
}
case E_MINE_DRONE:{
 if(e.leader<0||e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation){e.active=0;return true;}
 const Enemy &mother=enemies[(unsigned)e.leader];uint8_t mount=(uint8_t)(mother.angle+(e.formation_slot?100:-100));
 e.x=wrap(mother.x+dirX(mount)*13,worldWidth());e.y=wrap(mother.y+dirY(mount)*13,worldHeight());e.angle=toward(e.angle,aim,3);
 if(!e.form){e.form=1;e.fire_delay=40;e.phase_timer=125;e.phase=0;}
 if(e.fire_delay)--e.fire_delay;
 if(!e.fire_delay){int n=spawnEnemy(E_INTERCEPTOR_MINE,pixel(e.x),pixel(e.y));if(n>=0){enemies[n].angle=e.angle;enemies[n].vx=dirX(e.angle)/2;enemies[n].vy=dirY(e.angle)/2;enemies[n].damage_scale=e.damage_scale;e.fire_delay=90;}}
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase_timer){if(!e.phase){e.phase=1;e.phase_timer=10;emitEffect(e.x,e.y,FX_MUZZLE,4,10,1);}else{regularShot(e,e.angle,7*FP,10,SHOT_MINE_DRONE,120,2);e.phase=0;e.phase_timer=30;}}
 return true;
}
case E_STEALTH_MINE:case E_INTERCEPTOR_MINE:case E_SINGULARITY_MINE:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=10;e.action_timer=e.type==E_SINGULARITY_MINE?750:400;e.target_x=256;}
 e.vx=(int32_t)((int64_t)e.vx*950625/1000000);e.vy=(int32_t)((int64_t)e.vy*950625/1000000);e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());e.angle=(uint8_t)(e.angle+(e.type==E_SINGULARITY_MINE?-1:1));
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase_timer)e.phase=1;
 if(e.action_timer)--e.action_timer;
 if(e.type==E_SINGULARITY_MINE&&e.phase){
  // Other10: pull grows .1/source frame up to55; radius and force are
  // converted to the compact 1/6 arena. The source inverse-distance easing
  // is bounded here to avoid one-tick teleportation in Q8.
  e.target_x=minv(55*FP,e.target_x+51);int r=55;
  if(distance<r){int force=minv(32,e.target_x/256);player.vx-=clamp(dx,-r,r)*force/16;player.vy-=clamp(dy,-r,r)*force/16;}
  for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&distance2(e.x,e.y,constructs[c].x,constructs[c].y)<r*r){constructs[c].vx+=clamp(delta(e.x,constructs[c].x,worldWidth()),-r,r)*e.target_x/(256*32);constructs[c].vy+=clamp(delta(e.y,constructs[c].y,worldHeight()),-r,r)*e.target_x/(256*32);}
  if(e.age%30==0)emitEffect(e.x,e.y,FX_RING,r,20,3);
 }
 bool touched=e.phase&&distance2(e.x,e.y,player.x,player.y)<(e.radius+player.radius)*(e.radius+player.radius);
 if(e.phase&&!touched)for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&distance2(e.x,e.y,constructs[c].x,constructs[c].y)<(e.radius+constructs[c].radius)*(e.radius+constructs[c].radius)){touched=true;break;}
 if(touched||!e.action_timer){e.hull=0;destroyEnemy(owner);}return true;
}
case E_BOLT_THROWER:{
 // Source one-pass entry, stationary charging and accelerating retreat.
 if(!e.form){e.form=1;e.target_x=25*FP;e.attack_angle=e.angle;e.phase=0;e.action_count=0;}
 unsigned retreat=rarity?160:125;
 if(e.age<retreat)e.target_x=(int32_t)((int64_t)e.target_x*855625/1000000);
 else {e.phase=2;e.target_x=minv(64*FP*3,e.target_x+(rarity==2?282:rarity?173:115));}
 int speed=e.target_x/3;e.vx=dirX(e.attack_angle)*speed/FP;e.vy=dirY(e.attack_angle)*speed/FP;
 e.x+=e.vx;e.y+=e.vy;
 if(e.x<0||e.x>=worldWidth()*FP||e.y<0||e.y>=worldHeight()*FP){
  // Natural retreat is not a kill. Remove attached jaws without rewards or
  // destruction effects, and release their reserved physical-part slots.
  e.active=0;for(unsigned part=0;part<MAX_BOSS_PARTS;++part)if(boss_parts[part].max_hull&&boss_parts[part].owner==owner)clearBytes(&boss_parts[part],sizeof(BossPart));
  return true;
 }
 if(e.age<(rarity==2?100:75)){e.phase=0;if(e.age%15==0)emitEffect(e.x,e.y,FX_MUZZLE,6,15,1);}
 if(!rarity&&e.age==75){regularShot(e,e.attack_angle,30*FP,26,SHOT_BOLT,75,2);e.phase=1;++e.action_count;}
 if(rarity==2&&e.age==100){for(unsigned j=0;j<5;j++)regularShot(e,(uint8_t)(e.attack_angle+((int)j*2-4)*90*256/(8*360)),30*FP,32,SHOT_BOLT_CHAMPION,65,3);e.phase=1;++e.action_count;}
 if(rarity==1&&(e.age==80||e.age==103||e.age==125)){
  for(unsigned side=0;side<2;side++){int b=regularShot(e,e.attack_angle,25*FP,26,SHOT_BOLT,75,2);if(b>=0){uint8_t a=(uint8_t)(e.attack_angle+(side?13:-13));bullets[b].x=wrap(e.x+dirX(a)*15,worldWidth());bullets[b].y=wrap(e.y+dirY(a)*15,worldHeight());}}
  e.phase=1;++e.action_count;
 }return true;
}
case E_BEAMCASTER:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=75;e.target_x=0;e.action_count=(uint8_t)range(2);}
 int speed=rarity==1?576:rarity==2?384:256;
 uint8_t heading=(uint8_t)(aim+(e.action_count?1:-1)*(distance<100?55:18));
 move(heading,e.phase==2?speed/3:speed,1);
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase_timer){
  if(e.phase==0){e.phase=1;e.phase_timer=rarity==2?75:60;e.attack_angle=aim;}
  else if(e.phase==1){e.phase=2;e.phase_timer=75;e.target_x=0;}
  else if(e.phase==2){e.phase=3;e.phase_timer=30;}
  else {e.phase=0;e.phase_timer=90;e.action_count^=1;}
 }
 if(e.phase==1){e.attack_angle=toward(e.attack_angle,aim,2);emitEffect(e.x,e.y,FX_BEAM,1,2,1,e.x+dirX(e.attack_angle)*180,e.y+dirY(e.attack_angle)*180);}
 if(e.phase==2){e.attack_angle=toward(e.attack_angle,aim,1);
  if(rarity==2){
   // enemyFireBulletStream 0x140275985/59a3: interval = 2 *
   // enemySpeedInverse source frames, one 16-damage shot per port tick.
   regularShot(e,(uint8_t)(e.attack_angle+((int)range(5121)-2560)/360),15*FP,16,SHOT_BEAMCASTER,65,2);
  }else{
   int length=400;int32_t ex=e.x+dirX(e.attack_angle)*length,ey=e.y+dirY(e.attack_angle)*length;
   emitEffect(e.x,e.y,FX_BEAM,3,2,1,ex,ey);
   e.target_x+=(int)((int64_t)900*e.damage_scale/256*(10000+maxv(0,wave-120)*33)/10000);unsigned damage=(unsigned)(e.target_x/1000);e.target_x%=1000;
   if(damage){if(lineHit(e.x,e.y,ex,ey,player.x,player.y,player.radius+3))damagePlayer((int)damage,false,(uint8_t)(e.attack_angle+128),true);
    for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&lineHit(e.x,e.y,ex,ey,constructs[c].x,constructs[c].y,constructs[c].radius+3))damageConstruct(c,(int)damage);}
  }
 }return true;
}
case E_TRACER:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=60;e.action_count=0;e.target_x=-1;e.target_y=0;}
 move((uint8_t)(aim+(distance<100?70:distance>150?0:40)),3*FP,2);
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase_timer){
  if(e.phase==0){
   // Alternate native secondary orb and primary ray. A live orb becomes the
   // next ray's target so the original self-combination can actually occur.
   bool orb=(e.action_count&1)==0;e.target_x=-1;
   if(!orb)for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;j++)if(bullets[j].active&&bullets[j].owner&&bullets[j].kind==SHOT_TRACER_BALL&&!bullets[j].pierce){e.target_x=(int32_t)j;break;}
   e.phase=1;e.phase_timer=orb?60:20;e.attack_angle=aim;
  }else if(e.phase==1){
   bool orb=(e.action_count&1)==0;
   if(!orb&&e.target_x>=FRIENDLY_BULLETS&&e.target_x<MAX_BULLETS&&bullets[e.target_x].active&&bullets[e.target_x].kind==SHOT_TRACER_BALL)e.attack_angle=direction(delta(bullets[e.target_x].x,e.x,worldWidth()),delta(bullets[e.target_x].y,e.y,worldHeight()));
   int b=regularShot(e,e.attack_angle,(orb?8:30)*FP,orb?1:12,orb?SHOT_TRACER_BALL:SHOT_TRACER,orb?105:80,orb?6:2);
   if(b>=0){bullets[b].construct_source=(uint8_t)(owner+1);bullets[b].homing=(uint8_t)rarity;bullets[b].blast=(uint8_t)(orb?30+10*rarity:0);if(!orb)bullets[b].homing=0;}
   ++e.action_count;e.phase=0;e.phase_timer=orb?15:90;
  }
 }
 if(e.phase==1){if(e.target_x>=FRIENDLY_BULLETS&&e.target_x<MAX_BULLETS&&bullets[e.target_x].active)e.attack_angle=direction(delta(bullets[e.target_x].x,e.x,worldWidth()),delta(bullets[e.target_x].y,e.y,worldHeight()));else e.attack_angle=toward(e.attack_angle,aim,2);if(e.age%5==0)emitEffect(e.x,e.y,FX_MUZZLE,(e.action_count&1)?5:9,6,1);}
 return true;
}
