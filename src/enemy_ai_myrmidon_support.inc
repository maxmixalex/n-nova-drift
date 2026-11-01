// Native Other10 aura and optional Alarm0 enemyAutoTurret callback.
// Only champion flank/orbit followers receive this callback.
if(!(e.action_count&128u)){
 e.action_count=(uint8_t)(128u|((rarity==2&&(e.formation==3||e.formation==4))?64u:0u)|(range(2)?1u:0u));
 e.action_timer=180u*256u;e.target_x=(int32_t)e.angle*256;e.target_y=0;
}
if(rarity){
 unsigned radius=(550u*(60u+25u*rarity)*e.size_scale+153600u)/307200u;
 if((e.age%39)==1)emitEffect(e.x,e.y,FX_RING,radius,12,2);
 for(unsigned j=0;j<MAX_ENEMIES;++j){
  Enemy &a=enemies[j];if(!a.active||j==owner||a.hull<=0||a.hull>=a.max_hull)continue;
  bool standard=a.type!=E_ASTEROID&&a.type!=E_PULSAR_MINE&&a.type!=E_STEALTH_MINE&&a.type!=E_SINGULARITY_MINE&&a.type!=E_INTERCEPTOR_MINE&&a.type!=E_DWELLER_CHITIN&&a.type!=E_JUGG_MISSILE&&a.type!=E_JUGG_MISSILE_HOMING&&a.type!=E_CONSTRUCTOR_MISSILE;
  if(!standard||distance2(e.x,e.y,a.x,a.y)>(int)(radius*radius))continue;
  uint32_t fraction=(uint32_t)a.aura_fraction+(uint32_t)a.max_hull*(rarity==1?3u:4u);
  unsigned healed=fraction/6000u;a.aura_fraction=(uint16_t)(fraction%6000u);if(healed)healEnemy(j,(int)healed);
 }
}
if((e.action_count&64u)&&e.leader>=0&&e.leader<MAX_ENEMIES&&enemies[(unsigned)e.leader].active&&enemies[(unsigned)e.leader].generation==e.leader_generation){
 const Enemy &leader=enemies[(unsigned)e.leader];
 unsigned source_speed=1600u+7u*(unsigned)maxv(0,wave-120),speed_q8=(source_speed*256u+800u)/1600u;
 for(unsigned sub=0;sub<2;++sub){
  if(e.action_timer){e.action_timer=(uint16_t)(e.action_timer>speed_q8?e.action_timer-speed_q8:0);continue;}
  if(player.hull<=0||(player.body==B_SPECTRE&&player.cloak>130))continue;
  int turn=(int16_t)((unsigned)aim*256-(unsigned)e.target_x);e.target_x=(uint16_t)(e.target_x+turn/8);
  uint8_t leader_target=direction(delta(player.x,leader.x,worldWidth()),delta(player.y,leader.y,worldHeight()));
  if(absn((int8_t)(leader_target-leader.angle))<64)continue;
  bool blocked=lineHit(e.x,e.y,player.x,player.y,leader.x,leader.y,leader.radius);
  for(unsigned j=0;j<MAX_ENEMIES&&!blocked;++j){const Enemy &a=enemies[j];if(!a.active||j==owner)continue;if(a.type==E_MINE_SHIELD||a.hard_shield>0)blocked=lineHit(e.x,e.y,player.x,player.y,a.x,a.y,a.radius+(a.hard_shield>0?3:0));}
  for(unsigned j=0;j<MAX_BOSS_PARTS&&!blocked;++j){const BossPart &p=boss_parts[j];if(p.active&&p.kind==BP_BULWARK_BARRIER)blocked=sweptBossPart(p,e.x,e.y,player.x,player.y,0);}
  if(blocked)continue;
  uint8_t firing=(uint8_t)(e.target_x/256),side=(uint8_t)(firing+((e.action_count&1u)?64:-64));
  int shot=regularShot(e,firing,19*FP,12,SHOT_MYRMIDON_TURRET,180,2);
  if(shot>=0){Bullet &b=bullets[shot];b.x=wrap(e.x+dirX(side)*32*(int)e.size_scale/1536,worldWidth());b.y=wrap(e.y+dirY(side)*32*(int)e.size_scale/1536,worldHeight());b.size_percent=(uint16_t)minv(65535,3072u*e.size_scale/256u);}
  uint32_t delay=(uint32_t)(6ull*1600u*65536u/source_speed)+(unsigned)e.target_y;
  unsigned whole=delay/65536u;e.target_y=(int32_t)(delay%65536u);e.action_timer=(uint16_t)minv(65535,whole*256u);e.action_count^=1u;
  // Successful shots postpone the main cooldown, including a charge in progress.
  e.phase_timer=(uint16_t)minv(65535,(unsigned)e.phase_timer+1u+(delay+65535u)/65536u);
 }
}
