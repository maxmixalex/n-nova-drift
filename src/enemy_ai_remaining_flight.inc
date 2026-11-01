// Original event boundaries: docs/evidence/remaining_flight.json.
// Host timers below retain source frames; each update advances two of them.
case E_HOST:{
 if(!e.form){e.form=1;e.target_y=(int)(228+range(92));if(range(2))e.target_y=-e.target_y;}
 while(e.action_count<8){
  unsigned type=rarity==2?unsigned(E_HOST_DRONE_3):rarity==1?unsigned(E_HOST_DRONE_2):unsigned(E_HOST_DRONE);
  int child=summon(type,e.action_count,8,(60+15*rarity)/6);
  if(child<0)break;
  Enemy &q=enemies[child];q.formation_angle=(int16_t)(e.action_count*8192u);q.target_y=e.target_y;q.size_scale=e.size_scale;
  ++e.action_count;
 }
 bool live=false;
 for(unsigned j=0;j<MAX_ENEMIES;++j)if(enemies[j].active&&enemies[j].leader==(int)owner&&enemies[j].leader_generation==e.generation){live=true;break;}
 if(e.action_count==8&&!live){
  if(!e.phase){e.phase=1;e.attack_angle=(uint8_t)(aim+128);}
  e.angle=toward(e.angle,e.attack_angle,2);e.vx=dirX(e.angle)*6;e.vy=dirY(e.angle)*6;e.x+=e.vx;e.y+=e.vy;
  if(e.x<0||e.x>=worldWidth()*FP||e.y<0||e.y>=worldHeight()*FP)e.active=0;
 }else move((uint8_t)(aim+(distance<55?96:distance<85?64:0)),256-32*rarity,1);
 return true;
}
case E_HOST_DRONE_2:{
 if(e.leader<0||e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation){destroyEnemy(owner);return true;}
 Enemy &mother=enemies[(unsigned)e.leader];
 const unsigned speed_factor=1600u+7u*(unsigned)maxv(0,wave-120);
 const auto period=[&](unsigned source_frames)->uint16_t{return (uint16_t)maxv(1,(int)((source_frames*1600u+speed_factor/2)/speed_factor));};
 if(!e.form){
  e.form=1;e.phase=1;e.phase_timer=1;e.fire_delay=period(200);e.action_timer=0;e.attack_angle=aim;e.target_x=e.angle*256;
  if(!mother.target_y){mother.target_y=(int)(228+range(92));if(range(2))mother.target_y=-mother.target_y;}
  e.target_y=mother.target_y;
 }
 const bool cloaked=player.body==B_SPECTRE&&player.cloak>130;
 for(unsigned sub=0;sub<2;++sub){
  e.formation_angle=(int16_t)((uint16_t)e.formation_angle-e.target_y);
  uint8_t orbit=(uint8_t)((uint16_t)e.formation_angle/256);
  // Original source orbitRadius=75*enemySizeModifier; preserve the half pixel.
  int orbit_q8=(int)((75*FP/6)*(unsigned)e.size_scale/256u);
  e.x=wrap(mother.x+dirX(orbit)*orbit_q8/FP,worldWidth());e.y=wrap(mother.y+dirY(orbit)*orbit_q8/FP,worldHeight());
  if(!cloaked){e.attack_angle=direction(delta(player.x,e.x,worldWidth()),delta(player.y,e.y,worldHeight()));e.action_timer=0;}
  else if(e.action_timer<301)++e.action_timer;
  uint8_t target=e.attack_angle;
  if(e.action_timer>300)target=orbit;
  else if(e.action_timer>180){int turn=(int8_t)(orbit-target);target=(uint8_t)(target+turn*(e.action_timer-180)/120);}
  int turn=(int16_t)((unsigned)target*256-(unsigned)e.target_x);e.target_x=(uint16_t)(e.target_x+clamp(turn,-546,546));e.angle=(uint8_t)(e.target_x/256);
  // GameMaker Alarm0 runs before Alarm1/2. A closed firing gate still
  // reschedules Alarm0, so opening a burst does not reset its cadence.
  if(e.fire_delay)--e.fire_delay;
  if(!e.fire_delay){if(e.phase){int b=regularShot(e,e.angle,12*FP,8,SHOT_HOST_DRONE,180,2);if(b>=0)bullets[b].size_percent=(uint16_t)(1352u*e.size_scale/256u);}e.fire_delay=period(12);}
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer){e.phase^=1;e.phase_timer=period(e.phase?72:360);}
 }
 e.vx=e.vy=0;return true;
}
case E_SERAPH_ZONE_DRONE:{
 if(e.leader<0||e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation){destroyEnemy(owner);return true;}
 if(!e.form){e.form=1;if(!e.target_x&&!e.target_y){e.target_x=e.x;e.target_y=e.y;}e.action_timer=60;e.formation_angle=(int16_t)range(65536);}
 e.formation_angle=(int16_t)((uint16_t)e.formation_angle-146); // +.4 source degrees/frame; native screen angles are clockwise.
 uint8_t orbit=(uint8_t)((uint16_t)e.formation_angle/256);
 e.x=wrap(e.target_x+dirX(orbit)*5,worldWidth());e.y=wrap(e.target_y+dirY(orbit)*5,worldHeight());e.vx=e.vy=0;
 if(e.action_timer)--e.action_timer;
 if(!e.action_timer){
  const int margin=225*FP/6;
  bool in_zone=e.formation==1?player.y<margin:e.formation==2?player.y>worldHeight()*FP-margin:e.formation==3?player.x<margin:e.formation==4?player.x>worldWidth()*FP-margin:false;
  if(in_zone&&!(player.body==B_SPECTRE&&player.cloak>130)&&player.hull>0){
   int b=regularShot(e,e.angle,60*FP,15,SHOT_SERAPH_ZONE,180,2);
   if(b>=0){Bullet &shot=bullets[b];shot.field_scale=1620000u*e.size_scale/256u;shot.size_percent=(uint16_t)minv(65535,(int)((uint64_t)shot.field_scale*4096/1000000));shot.last_hit=(int16_t)minv(30000,1229u*e.size_scale/256u);}
  }
  e.action_timer=15;
 }
 return true;
}
