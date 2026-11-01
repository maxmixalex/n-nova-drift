// Source: docs/evidence/flight_ai.csv. Original durations are 60 Hz frames;
// this loop advances two source frames per tick. Spatial steering uses the
// common fixed-point 1/6-world mapping and bounded integer turn easing.
case E_SKIRMISHER:{ // oInterceptor: four wing shots; champion alternates dual mines.
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=rarity==2?20:200;e.action_timer=rarity==2?60:0;e.fire_delay=44;e.action_count=(uint8_t)range(2);e.summon_timer=12;}
 int flank=e.action_count?64:-64;
 move((uint8_t)(aim+(distance<85?flank:flank/3)),3*FP,2);
 if(e.action_timer){--e.action_timer;return true;}
 if(rarity==2){
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer){e.phase^=1;e.phase_timer=e.phase?70:200;if(e.phase)e.summon_timer=12;}
 }
 if(e.phase){
  if(e.summon_timer)--e.summon_timer;
  if(!e.summon_timer){
   for(unsigned side=0;side<2;side++){
    uint8_t a=(uint8_t)(e.angle+(side?46:-46));
    int n=spawnEnemy(E_INTERCEPTOR_MINE,pixel(e.x)+dirX(a)*4/FP,pixel(e.y)+dirY(a)*4/FP);
    if(n>=0){Enemy &m=enemies[n];m.angle=(uint8_t)(e.angle+(side?114:-114));m.vx=dirX(m.angle)/2;m.vy=dirY(m.angle)/2;m.damage_scale=e.damage_scale;}
   }
   e.summon_timer=10;
  }
 }else{
  if(e.fire_delay)--e.fire_delay;
  if(!e.fire_delay){
   static const int8_t angles[4]={-57,-32,32,57};
   for(unsigned j=0;j<4;j++){int b=regularShot(e,(uint8_t)(e.angle+angles[j]),640,10,rarity?SHOT_INTERCEPTOR_MISSILE:SHOT_ENEMY,150,2);if(b>=0&&rarity)bullets[b].homing=1;}
   e.fire_delay=175;
  }
 }
 return true;
}
case E_DRONE:{ // oStationDrone remains linked to its station; no independent horde AI.
 if(!e.form){
  e.form=1;e.elite=0;e.action_count=3;e.fire_delay=120;e.phase=(uint8_t)range(2);
  e.target_x=103+(int)range(11); // (617.5 + random(.1)*650) / 6.
  if(e.leader<0)for(unsigned j=0;j<MAX_ENEMIES;j++)if(enemies[j].active&&enemies[j].type==E_STATION_OMEGA){e.leader=(int8_t)j;e.leader_generation=enemies[j].generation;break;}
 }
 if(e.leader<0||e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation){e.active=0;return true;}
 const Enemy &mother=enemies[(unsigned)e.leader];int mx=delta(mother.x,e.x,worldWidth()),my=delta(mother.y,e.y,worldHeight());
 uint8_t center=direction(mx,my);int side=e.phase?1:-1;
 move((uint8_t)(center+side*(approx(mx,my)>e.target_x?32:64)),4*FP,2);
 if(e.fire_delay)--e.fire_delay;
 if(!e.fire_delay){
  int b=regularShot(e,(uint8_t)(e.angle+side*85),5*FP,15,SHOT_STATION_DRONE,200,3);if(b>=0)bullets[b].homing=1;
  if(--e.action_count)e.fire_delay=15;else {e.action_count=4;e.fire_delay=240;}
 }
 return true;
}
case E_SERPENT:{ // oSaucer, displayed as Sine. Fixed entry heading with +/-45-degree sine.
 if(!e.form){e.form=1;e.attack_angle=e.angle;e.formation_angle=0;e.phase=(uint8_t)range(2);e.action_count=rarity==1?3:1;e.fire_delay=rarity?150:100;}
 e.formation_angle=(int16_t)((uint16_t)e.formation_angle+(e.phase?364:-364));
 int sway=dirY((uint8_t)((uint16_t)e.formation_angle/256))/8;
 move((uint8_t)(e.attack_angle+sway),3*FP,255);
 if(e.fire_delay)--e.fire_delay;
 if(!e.fire_delay){
  regularShot(e,e.angle,5*FP,25,rarity==2?SHOT_SINE_MEGA:SHOT_SINE,240,rarity==2?6:3);
  if(--e.action_count)e.fire_delay=10;else {e.action_count=rarity==1?3:1;e.fire_delay=rarity?150:100;}
 }
 return true;
}
case E_MINE:{ // oScatterDrone / Pulsebug: ranged shotgun, never a stationary mine.
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=0;e.target_y=230+(int)range(52);}
 int near=rarity==2?35:rarity?27:20,far=rarity==2?42:rarity?33:25;
 int speed=448*(rarity==2?12:rarity?11:10)/10*e.target_y/256;
 move((uint8_t)(aim+(distance<near?128:distance<=far?64:0)),e.phase?speed/3:speed,3);
 if(e.phase){
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer){
   unsigned count=rarity==2?15:rarity?10:7;int spread=rarity==1?30:60,base=rarity==2?10:rarity?20:15,variance=rarity==2?6:rarity?12:8;
   for(unsigned j=0;j<count;j++){
    int offset=rarity==2?(int)j*256/15:((int)range((unsigned)spread*256+1)-spread*128)/360;
    int v=base*FP+(int)range((unsigned)variance*FP+1)-variance*FP/2;
    int b=regularShot(e,(uint8_t)(e.attack_angle+offset),v,7,rarity==2?SHOT_PULSEBUG_HOMING:SHOT_PULSEBUG,rarity==2?52:24,2);
    if(b>=0&&rarity==2)bullets[b].homing=1;
   }
   e.phase=0;e.phase_timer=120;
  }
 }else{
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer&&distance<=far){e.phase=1;e.phase_timer=rarity==2?40:rarity?30:20;e.attack_angle=aim;emitEffect(e.x,e.y,FX_RING,8,(unsigned)e.phase_timer,1);}
 }
 return true;
}
case E_CHERUB:{ // Support: fractional healing aura plus a separate hard-shield carrier.
 if(!e.form){e.form=1;e.action_count=(uint8_t)range(2);e.phase_timer=32;e.action_timer=13;e.phase=0;}
 move((uint8_t)(aim+(e.action_count?64:-64)),320,1);
 unsigned radius=(550u*(85+25*rarity)+600)/1200;
 if(e.action_timer)--e.action_timer;
 if(!e.action_timer){emitEffect(e.x,e.y,FX_RING,radius,12,2);e.action_timer=39;}
 int ally=-1,best=0x7fffffff;
 for(unsigned j=0;j<MAX_ENEMIES;j++){
  Enemy &a=enemies[j];if(!a.active||j==owner)continue;
  // Aura and shield searches enumerate oEnemyStandard. Station's native
  // node/wall actors are represented by one owner in this fixed-pool port.
  bool standard=a.type!=E_ASTEROID&&a.type!=E_PULSAR_MINE&&a.type!=E_STEALTH_MINE&&
    a.type!=E_SINGULARITY_MINE&&a.type!=E_INTERCEPTOR_MINE&&a.type!=E_DWELLER_CHITIN&&
    a.type!=E_JUGG_MISSILE&&a.type!=E_JUGG_MISSILE_HOMING;
  if(!standard)continue;
  int d=distance2(e.x,e.y,a.x,a.y);
  if(d<(int)(radius*radius)&&a.hull<a.max_hull){
   uint32_t f=(uint32_t)a.aura_fraction+(uint32_t)a.max_hull*(4+2*rarity);int gain=(int)(f/6000);a.aura_fraction=(uint16_t)(f%6000);if(gain)healEnemy(j,gain);
  }
  // Standard PreCreate sets true; these specific actors override false.
  // Boss ownership is not a blanket exclusion in the source search.
  bool shieldable=a.type!=E_CHERUB&&a.type!=E_CONSTRUCTOR_DRONE&&a.type!=E_HOST_DRONE&&a.type!=E_HOST_DRONE_2&&a.type!=E_HOST_DRONE_3;
  if(shieldable&&!a.hard_shield&&d<best&&d<(int)((180+40*rarity)*(180+40*rarity))){best=d;ally=(int)j;}
 }
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase&&!e.phase_timer){
  if(ally<0)e.phase_timer=53; // Native no-ally retry: .25 * 420 source frames.
  else {e.phase=1;e.phase_timer=20;e.target_x=ally;e.target_y=enemies[ally].generation;emitEffect(e.x,e.y,FX_RING,8,20,2);}
 }else if(e.phase&&!e.phase_timer){
  if(e.target_x>=0&&e.target_x<MAX_ENEMIES){Enemy &a=enemies[e.target_x];if(a.active&&a.generation==e.target_y&&!a.hard_shield){
   uint8_t heading=direction(delta(a.x,e.x,worldWidth()),delta(a.y,e.y,worldHeight()));int b=regularShot(e,heading,1152,0,SHOT_ENEMY_SHIELD,150,3);
   if(b>=0){bullets[b].damage=0;bullets[b].last_hit=(int16_t)e.target_x;bullets[b].size_percent=a.generation;bullets[b].homing=1;}
  }}
  e.phase=0;e.phase_timer=210;
 }
 return true;
}
case E_WARDEN:{ // One arrival/charge/volley/escape sequence; never fires indefinitely.
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=40;e.action_count=(uint8_t)range(2);}
 if(e.phase_timer)--e.phase_timer;
 if(e.phase<3){
  move(aim,e.phase==0?3200*(int)e.phase_timer/40:0,2);
  if(!e.phase_timer){
   if(e.phase==0){e.phase=1;e.phase_timer=rarity==2?105:75;emitEffect(e.x,e.y,FX_RING,12,30,1);}
   else if(e.phase==1){
    e.phase=2;e.phase_timer=30;e.attack_angle=aim;
    unsigned count=rarity==1?5:1;
    for(unsigned j=0;j<count;j++){int offset=rarity==1?((int)j*20-40)*256/360:0;regularShot(e,(uint8_t)(aim+offset),rarity==2?2176:rarity?2432:3200,rarity==2?80:rarity?30:45,rarity==2?SHOT_WARDEN_VORTEX:SHOT_WARDEN,170,rarity==2?7:4);}
   }else {e.phase=3;e.phase_timer=150;e.action_timer=0;}
  }
 }else{
  ++e.action_timer;e.angle=(uint8_t)(e.angle+(e.action_count?1:-1));int speed=25*FP*minv(70,e.action_timer)/210;
  e.vx=dirX(e.angle)*speed/FP;e.vy=dirY(e.angle)*speed/FP;e.x+=e.vx;e.y+=e.vy;
  if(!e.phase_timer||e.x<0||e.x>=worldWidth()*FP||e.y<0||e.y>=worldHeight()*FP)e.active=0;
 }
 return true;
}
