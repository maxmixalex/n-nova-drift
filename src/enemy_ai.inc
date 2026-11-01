// Native ordinary-enemy setup and attack evidence: docs/evidence/regular_ai.csv.
// Source velocities are pixels/60 Hz frame. The port uses a uniform 1/6 spatial
// mapping and two source frames/tick, hence velocity /3. Target ranges and
// formations use the same mapping; turn easing and collision radii are adapted.
int Game::regularShot(Enemy &e,uint8_t angle,int source_speed_q8,int damage,unsigned kind,unsigned life,unsigned radius) {
 damage=(int)((int64_t)damage*e.damage_scale/256*(10000+maxv(0,wave-120)*33)/10000);
 int slot=emitBullet(e.x,e.y,angle,maxv(1,(source_speed_q8+767)/768),damage,kind,life,1,radius);
 if(slot>=0){Bullet &b=bullets[slot];b.speed=(uint16_t)clamp(source_speed_q8/3,1,64*FP);b.vx=dirX(angle)*b.speed/FP;b.vy=dirY(angle)*b.speed/FP;}
 return slot;
}
void Game::followEnemyLeader(Enemy &e) {
 if(e.leader<0||e.leader>=MAX_ENEMIES)return;
 Enemy &leader=enemies[(unsigned)e.leader];
 if(!leader.active||leader.generation!=e.leader_generation){
  if(e.formation_break){--e.formation_break;return;}
  e.leader=-1;e.formation=0;return;
 }
 if(!e.formation)return;
 unsigned count=maxv(1,e.formation_count),slot=e.formation_slot;
 int spacing=maxv(8,e.formation_spacing),offset=0;uint8_t angle=leader.angle;
 if(e.formation==4){e.formation_angle=(int16_t)(e.formation_angle+190);angle=(uint8_t)((uint16_t)e.formation_angle/256+slot*256/count);}
 else if(e.formation==6){angle=(uint8_t)(leader.angle+128);spacing*=1+slot;}
 else if(e.formation==3){offset=(int)(slot/2+1)*spacing;angle=(uint8_t)(leader.angle+((slot&1)?64:-64));spacing=offset;}
 else {angle=(uint8_t)(leader.angle+128+((int)slot*2-(int)count+1)*12);}
 int32_t tx=wrap(leader.x+dirX(angle)*spacing,worldWidth()),ty=wrap(leader.y+dirY(angle)*spacing,worldHeight());
 int dx=delta(tx,e.x,worldWidth()),dy=delta(ty,e.y,worldHeight());
 e.vx=(e.vx*3+leader.vx+clamp(dx*FP/6,-4*FP,4*FP))/4;e.vy=(e.vy*3+leader.vy+clamp(dy*FP/6,-4*FP,4*FP))/4;
 e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());
}
bool Game::updateRegularEnemy(Enemy &e) {
 const unsigned owner=(unsigned)(&e-enemies);const unsigned rarity=e.elite;
 int32_t target_x=daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0?daily_egg_x:player.x;
 int32_t target_y=daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0?daily_egg_y:player.y;
 int dx=delta(target_x,e.x,worldWidth()),dy=delta(target_y,e.y,worldHeight()),distance=approx(dx,dy);
 uint8_t aim=direction(dx,dy);
 if(player.mods[M_X_DECOY_SIGNAL])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_GUARDIAN&&distance2(e.x,e.y,constructs[c].x,constructs[c].y)<100*100){aim=direction(delta(constructs[c].x,e.x,worldWidth()),delta(constructs[c].y,e.y,worldHeight()));break;}
 if(e.formation_delay){--e.formation_delay;return true;}
 auto move=[&](uint8_t heading,int source_speed_q8,unsigned turn=3){
  int speed=(int)minv(64*FP,(int64_t)source_speed_q8*(1600+7*maxv(0,wave-120))/4800);
  if(player.body==B_SPECTRE&&player.cloak>130){heading=(uint8_t)(e.age/4);speed/=2;}
  if(player.shield>0&&player.shield_type==S_TEMPORAL&&shieldDistance2(e.x,e.y)<(int)(activeShieldRadius()*activeShieldRadius()))speed=speed*35/100;
  if(e.stun)speed/=3;
  e.angle=toward(e.angle,heading,turn);e.vx=(e.vx*7+dirX(e.angle)*speed/FP)/8;e.vy=(e.vy*7+dirY(e.angle)*speed/FP)/8;
  if(e.formation&&e.leader>=0)followEnemyLeader(e);else {e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());}
 };
 auto summon=[&](unsigned type,unsigned slot,unsigned count,unsigned orbit,unsigned formation=4)->int{
  if(enemyCount()>=MAX_ENEMIES)return -1;
  uint8_t a=(uint8_t)(slot*256/maxv(1,count));
  int n=spawnEnemy(type,pixel(e.x)+dirX(a)*(int)orbit/FP,pixel(e.y)+dirY(a)*(int)orbit/FP,0);
  if(n>=0){Enemy &child=enemies[n];child.leader=(int8_t)owner;child.leader_generation=e.generation;child.formation=(uint8_t)formation;child.formation_slot=(uint8_t)slot;child.formation_count=(uint8_t)count;child.formation_spacing=(uint16_t)orbit;child.angle=aim;child.damage_scale=e.damage_scale;child.treasure_scale=e.treasure_scale;}
  return n;
 };
 if(e.summon_type<ENEMY_COUNT&&e.summon_interval){if(e.summon_timer)--e.summon_timer;else if(summon(e.summon_type,e.action_count++,3,20,0)>=0)e.summon_timer=e.summon_interval;}
 if(e.type==E_CRUCIBLE_DRONE){
  bool linked=e.leader>=0&&e.leader<MAX_ENEMIES&&enemies[(unsigned)e.leader].active&&enemies[(unsigned)e.leader].generation==e.leader_generation;
  if(!linked){e.active=0;return true;}
  Enemy &leader=enemies[(unsigned)e.leader];int base=maxv(1,e.formation_spacing)*FP;
  if(!e.form){e.form=1;e.target_x=base;e.target_y=256;}
  if(leader.phase==1){e.target_x=minv(200*FP,e.target_x+83);e.target_y=maxv(64,256-(int)leader.phase_age*256/300);}
  else {e.target_x=maxv(base,e.target_x-83);e.target_y=minv(256,e.target_y+1);}
  e.formation_angle=(int16_t)(e.formation_angle+182); // .5 degrees/source frame.
  uint8_t angle=(uint8_t)((uint16_t)e.formation_angle/256+e.formation_slot*256/maxv(1,e.formation_count));
  e.x=wrap(leader.x+dirX(angle)*e.target_x/FP,worldWidth());e.y=wrap(leader.y+dirY(angle)*e.target_x/FP*e.target_y/256,worldHeight());e.angle=angle;
  return true; // The mother's orbParty callback owns every firing event.
 }
 if(e.type==E_SERAPH_DRONE){
  bool linked=e.leader>=0&&e.leader<MAX_ENEMIES&&enemies[(unsigned)e.leader].active&&enemies[(unsigned)e.leader].generation==e.leader_generation;
  if(!linked){e.active=0;return true;}
  Enemy &leader=enemies[(unsigned)e.leader];

   unsigned orbit=e.formation_spacing?e.formation_spacing:33;
   e.formation_angle=(int16_t)(e.formation_angle+728); // 2 degrees/source frame.
   uint8_t a=(uint8_t)((uint16_t)e.formation_angle/256+e.formation_slot*256/maxv(1,e.formation_count));
   e.x=wrap(leader.x+dirX(a)*(int)orbit,worldWidth());e.y=wrap(leader.y+dirY(a)*(int)orbit,worldHeight());
   e.angle=direction(delta(player.x,e.x,worldWidth()),delta(player.y,e.y,worldHeight()));
   if(e.fire_delay){--e.fire_delay;}if(!e.fire_delay) {unsigned shots=(e.action_count++&1)?4:3;for(unsigned j=0;j<shots;j++)regularShot(e,(uint8_t)(e.angle+((int)j*2-(int)shots+1)*18*256/(720*(int)(shots-1))),9*FP,20,SHOT_SERAPH_SNIPE,110,2);e.fire_delay=8;}
  
  return true;
 }
 switch(e.type){
 #include "enemy_ai_hazards.inc"
 #include "enemy_ai_flight.inc"
 #include "enemy_ai_remaining_flight.inc"
 #include "enemy_ai_dweller.inc"
 #include "enemy_ai_parts.inc"
 #include "enemy_ai_summons.inc"
 #include "enemy_ai_constructor.inc"
 #include "enemy_ai_swarm.inc"
 #include "enemy_ai_pulsar.inc"


 case E_HOST_DRONE:{
  if(e.leader>=0){followEnemyLeader(e);e.angle=aim;}else move(aim,3*FP);
  if(e.fire_delay){--e.fire_delay;}if(!e.fire_delay) {
   regularShot(e,aim,(e.type==E_HOST_DRONE_2?12:9)*FP,8);e.fire_delay=60;
  }return true;
 }
 case E_HIVE:case E_CRAB_NEST:{
  if(!e.form){e.form=1;e.fire_delay=e.type==E_HIVE?14:3;e.action_timer=385;e.phase_timer=578;}
  move(e.angle,FP,1);if(e.age<20)return true;
  if(e.fire_delay){--e.fire_delay;}if(!e.fire_delay) {
   unsigned type=e.type==E_CRAB_NEST?E_DWELLER_DRONE:rarity==2?E_WARDEN:rarity?E_HIVE_DRONE_HOT:E_HIVE_DRONE;
   if(e.type!=E_CRAB_NEST||e.action_count<66){int n=summon(type,e.action_count,8,8,0);if(n>=0){enemies[n].angle=(uint8_t)(e.angle+128);enemies[n].vx=dirX(enemies[n].angle)*4;enemies[n].vy=dirY(enemies[n].angle)*4;++e.action_count;e.fire_delay=e.type==E_CRAB_NEST?(uint16_t)(12+(e.action_count&1)):20;}}
   else {e.action_count=0;e.fire_delay=660;}
  }
  if(e.type==E_CRAB_NEST){if(e.action_timer)--e.action_timer;if(!e.action_timer&&summon(E_DWELLER_GUARD,0,1,8,0)>=0)e.action_timer=385;if(e.phase_timer)--e.phase_timer;else if(summon(E_DWELLER_WARRIOR,0,1,8,0)>=0)e.phase_timer=578;}
  return true;
 }
 case E_CONSTRUCTOR:{
  if(!e.form){e.form=1;e.fire_delay=(uint16_t)((500-100*rarity)/3);}
  unsigned count=5+rarity+(rarity==2?1:0);if(e.action_count<count&&summon(E_CONSTRUCTOR_DRONE,e.action_count,count,(65+5*rarity)/6)>=0)++e.action_count;
  move((uint8_t)(aim+(distance<90?96:0)),FP,1);
  if(e.fire_delay){--e.fire_delay;}if(!e.fire_delay) {unsigned turrets=0;for(unsigned j=0;j<MAX_ENEMIES;j++)if(enemies[j].active&&enemies[j].type==E_CONSTRUCTOR_TURRET&&enemies[j].leader==(int)owner&&enemies[j].leader_generation==e.generation)++turrets;if(turrets<3&&summon(E_CONSTRUCTOR_TURRET,turrets,3,10,0)>=0)e.fire_delay=(uint16_t)((500-100*rarity)/2);}
  return true;
 }
 case E_GYROGUN:{
  if(!e.form){e.form=1;e.phase_timer=100;e.action_timer=65;e.fire_delay=3;e.target_x=0;}
  if(e.action_timer){--e.action_timer;move(e.angle,14*FP*(int)e.action_timer/65,1);return true;}
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer){e.phase^=1;e.phase_timer=e.phase?50:100;e.attack_angle=aim;e.target_x=0;}
  if(e.phase){move(e.attack_angle,12*FP,1);return true;}
  int change=rarity==2?131:rarity==1?262:426;e.target_x=clamp(e.target_x+2*change,3277,change*300); // Q16 degrees/source frame.
  e.formation_angle=(int16_t)(e.formation_angle+e.target_x/180);e.angle=(uint8_t)((uint16_t)e.formation_angle/256);
  e.vx=e.vx*15/16;e.vy=e.vy*15/16;e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());
  if(e.fire_delay){--e.fire_delay;}if(!e.fire_delay) {regularShot(e,e.angle,512+51*rarity,6);regularShot(e,(uint8_t)(e.angle+128),512+51*rarity,6);e.fire_delay=3;}
  return true;
 }
 case E_FRIGATE:{ // Hammerhead has no gun: its attack is a telegraphed charge.
  if(!e.form){e.form=1;e.phase_timer=78;e.phase=0;}
  if(e.phase_timer)--e.phase_timer;
  if(e.phase==0){move(aim,3*FP,2);if(!e.phase_timer){e.phase=1;e.phase_timer=15;e.attack_angle=aim;emitEffect(e.x,e.y,FX_BEAM,1,15,1,e.x+dirX(aim)*120,e.y+dirY(aim)*120);}}
  else if(e.phase==1){move(e.attack_angle,FP,1);if(!e.phase_timer){e.phase=2;e.phase_timer=25;}}
  else {move(e.attack_angle,22*FP+640*rarity,1);if(!e.phase_timer){e.phase=0;e.phase_timer=(uint16_t)((405-45*rarity)/2);}}
  return true;
 }
#include "enemy_ai_myrmidon.inc"
 #include "enemy_ai_snapper.inc"
 #include "enemy_ai_celestial.inc"
 #include "enemy_ai_eclipse.inc"
 case E_WASP:{
  if(!e.form){e.form=1;e.phase_timer=100;e.action_count=0;e.fire_delay=5;}
  if(e.phase_timer){--e.phase_timer;move(e.angle,13*FP*(int)e.phase_timer/100,1);return true;}
  if(e.phase){move(e.attack_angle,18*FP,1);if(++e.phase_age>=90){e.phase=0;e.action_count=0;e.phase_age=0;}return true;}
  move((uint8_t)(aim+(distance<90?64:0)),832,2);
  if(e.fire_delay){--e.fire_delay;}if(!e.fire_delay) {int swing=dirY((uint8_t)(e.action_count*2))*40*256/(256*360);regularShot(e,(uint8_t)(aim+swing-3),9*FP,12);regularShot(e,(uint8_t)(aim+swing+3),9*FP,12);e.fire_delay=rarity?3:5;if(++e.action_count>=(rarity?120:51)){e.phase=1;e.attack_angle=aim;e.phase_age=0;}}
  return true;
 }
 default:return false;
 }
}
