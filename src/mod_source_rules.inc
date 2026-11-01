// Source rules are summarized as scalar facts in docs/evidence/mod_source_rules.json.
// Included inside namespace nova; no heap and no host floating-point routines.
namespace { uint32_t modCap64(uint64_t n,uint32_t cap){return n>cap?cap:(uint32_t)n;} }
void Game::fireHomingStrike(uint8_t fire_angle,int projectiles,int speed,int damage,int life,int radius) {
 if(!player.mods[M_HOMING_STRIKE]||player.weapon==W_SWORDS)return;
 static const uint8_t kinds[WEAPON_COUNT]={SHOT_BLASTER,SHOT_SPLIT,SHOT_FLAK,SHOT_RAIL,SHOT_GRENADE,SHOT_MISSILE,SHOT_VORTEX,SHOT_SPARK,SHOT_TORRENT,SHOT_BLADE,SHOT_DART,SHOT_SPARK,SHOT_SPARK};
 unsigned weapon=player.weapon,kind=kinds[weapon],pairs=1+(unsigned)maxv(0,projectiles)/5;
 unsigned scale=(unsigned)clamp((int64_t)radius*4096*7/(10*maxv(1,weapons[weapon].radius)),103,65535);
 for(unsigned j=0;j<pairs*2;j++){
  // Native firingDirectionModifier +/-120 degrees, with no artificial spread.
  uint8_t angle=(uint8_t)(fire_angle+((j&1)?85:-85));
  int slot=emitBullet(player.x,player.y,angle,maxv(1,speed*40/100),maxv(1,damage*30/100),kind,maxv(life,60000),0,maxv(1,radius*7/10));
  if(slot<0)continue;
  Bullet &b=bullets[slot];b.tracking=1;b.tracking_weapon=(uint8_t)weapon;
  b.tracking_scale_x=b.tracking_scale_y=(uint16_t)scale;
  b.tracking_target=-1;
  b.homing=(uint8_t)clamp(2*stats.homing+5,1,31); // source seeking += 2.5
  b.tracking_seek=(uint16_t)(b.homing*2048*3/10); // source seek=.3*seeking
  b.blast=(uint8_t)(b.blast*40/100);b.size_percent=(uint16_t)maxv(1,b.size_percent*40/100);
  b.speed=(uint16_t)clamp((int64_t)speed*FP*40/100,1,64*FP);b.vx=dirX(angle)*b.speed/FP;b.vy=dirY(angle)*b.speed/FP;
  if(kind==SHOT_VORTEX){
   // oVortex Step computes its own instantaneous damage from the base weapon.
   // HS changes the field axes, not that source burn coefficient.
   b.field_scale=(uint32_t)minv(64000000,(int)(((uint64_t)vortex_scale_+rancor_scale_)*7/10));
   b.damage=(int)minv(2000000,(int64_t)weaponDamage()*1000);b.traveled=0;b.bounces=0;
  }
  if(kind==SHOT_GRENADE)b.life=(uint16_t)life; // the original grenade fuse still applies
 }
}
void Game::updateHomingStrike(Bullet &b) {
 if(!b.active||b.owner||!b.tracking)return;
 if(b.kind==SHOT_DART&&b.pierce==254)return; // attached Dart uses its own source burn lifetime
 unsigned weapon=b.tracking_weapon<WEAPON_COUNT?unsigned(b.tracking_weapon):unsigned(W_BLASTER);
 // assignBaseStats: per-weapon source smoothing denominators.
 static const uint8_t seek_rate[WEAPON_COUNT]={60,50,25,18,50,12,100,100,25,50,30,120,10};
 if(b.age==1||b.age%3==0||b.tracking_target<0||!targetActive(b.tracking_target)){
  // chooseBestTarget: priority-zero objects are not tracking targets.
  // At zero angle weighting, minimize distance/(weakness*vulnerability).
  // Comparing squared ratios avoids a square root or a float per candidate.
  int best=-1,clear=-1;uint64_t best_distance=0,clear_distance=0;unsigned best_weight=1,clear_weight=1;
  for(unsigned t=0;t<MAX_ENEMIES+MAX_BOSS_PARTS;t++)if(targetActive((int)t)&&!targetObstructedByEclipse(t)){
   unsigned owner=targetOwner((int)t);const Enemy &e=enemies[owner];unsigned weakness=1;
   if(t<MAX_ENEMIES){
    if(e.type==E_HOST_DRONE||e.type==E_HOST_DRONE_2||e.type==E_HOST_DRONE_3||e.type==E_CELESTIAL_DRONE||e.type==E_CONSTRUCTOR_DRONE)continue;
    if(e.type==E_WARBRINGER||e.type==E_ECLIPSE)weakness=2;
   }else{
    switch(boss_parts[t-MAX_ENEMIES].kind){
     case BP_BULWARK_BARRIER:continue;
     case BP_DREAD_GUN_LARGE:case BP_STATION_NODE:weakness=2;break;
     case BP_DREAD_GUN_SMALL:weakness=5;break;
     case BP_GLAUCUS_EYE:weakness=4;break;
     default:break;
    }
   }
   int64_t dx=(targetX((int)t)-b.x)/FP,dy=(targetY((int)t)-b.y)/FP;
   uint64_t distance=(uint64_t)(dx*dx+dy*dy);unsigned weight=weakness*(100+e.vulnerability);
   if(best<0||distance*best_weight*best_weight<best_distance*weight*weight){best=(int)t;best_distance=distance;best_weight=weight;}
   if(e.hard_shield>0)continue;
   if(clear>=0&&distance*clear_weight*clear_weight>=clear_distance*weight*weight)continue;
   bool blocked=false;
   for(unsigned j=0;j<MAX_ENEMIES;j++)if(enemies[j].active&&enemies[j].hard_shield>0&&
     lineHit(b.x,b.y,targetX((int)t),targetY((int)t),enemies[j].x,enemies[j].y,enemies[j].radius+4)){blocked=true;break;}
   if(!blocked){clear=(int)t;clear_distance=distance;clear_weight=weight;}
  }
  b.tracking_target=(int16_t)(clear>=0?clear:best);
 }
 for(unsigned sub=0;sub<2;sub++){
  int goal=b.homing*2048,seek=b.tracking_seek;
  seek+=(goal-seek+(int)seek_rate[weapon]/2)/(int)seek_rate[weapon];b.tracking_seek=(uint16_t)clamp(seek,0,65535);
  if(b.tracking_target>=0&&targetActive(b.tracking_target)){
   uint8_t desired=direction(targetX(b.tracking_target)-b.x,targetY(b.tracking_target)-b.y);
   unsigned step=(unsigned)maxv(1,(int)((uint64_t)b.tracking_seek*2*256/(4096*360)));
   b.angle=toward(b.angle,desired,step);
  }
  if(b.kind==SHOT_FLAK)b.speed=(uint16_t)(b.speed*98/100);
  if(b.kind==SHOT_VORTEX||b.kind==SHOT_MISSILE||b.kind==SHOT_GRENADE||b.kind==SHOT_BLADE)continue;
  // HS Alarm6 is the ordinary degeneration delay plus 60 source frames.
  // Split Shot and Flak have weapon-specific delay overrides in assignBaseStats.
  unsigned source_age=(unsigned)b.age*2-1+sub;
  unsigned delay=weapon==W_SPLIT_SHOT?85:weapon==W_FLAK?68:80;
  if(source_age<=delay)continue;
  unsigned age=source_age-delay,dx=0,dy=0;
  if(b.kind==SHOT_FLAK){b.tracking_scale_x=(uint16_t)(b.tracking_scale_x*95/100);b.tracking_scale_y=b.tracking_scale_x;continue;}
  unsigned nx=6000,ny=6000;
  if(b.kind==SHOT_BLASTER){nx=11000;ny=13200;}
  else if(b.kind==SHOT_TORRENT){nx=5500;ny=7500;}
  else if(b.kind==SHOT_DART){nx=ny=25000;}
  else if(b.kind==SHOT_SPLIT){nx=ny=125;}
  dx=(unsigned)((uint64_t)age*nx*4096/1000000-(uint64_t)(age-1)*nx*4096/1000000);
  dy=(unsigned)((uint64_t)age*ny*4096/1000000-(uint64_t)(age-1)*ny*4096/1000000);
  b.tracking_scale_x=(uint16_t)(b.tracking_scale_x>dx?b.tracking_scale_x-dx:0);
  b.tracking_scale_y=(uint16_t)(b.tracking_scale_y>dy?b.tracking_scale_y-dy:0);
 }
 if(b.kind!=SHOT_VORTEX&&(b.tracking_scale_x<=102||b.tracking_scale_y<=102)){b.active=0;return;}
 if(b.kind!=SHOT_VORTEX)b.radius=(uint8_t)clamp((int)((unsigned)weapons[weapon].radius*minv(b.tracking_scale_x,b.tracking_scale_y)/4096),1,32);
 b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
}
void Game::triggerLastStand() {
 last_stand_used_=1;recalculate();
 player.hull=(int16_t)hullRecoveryCap();player.shield=(int16_t)shieldRecoveryCap();
 player.shield_delay=0;player.invulnerable=90; // source iFrames=180 at 60Hz
 // Share the fixed burst allocator; mode 1 selects Last Stand's own source
 // .5 initial scale, .25 growth and 11-frame/.025 fade profile below.
 unsigned slot=MAX_PERSISTENT_FIELDS;
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++)if(!persistent_fields[i].active){slot=i;break;}
 if(slot==MAX_PERSISTENT_FIELDS){slot=0;for(unsigned i=1;i<MAX_PERSISTENT_FIELDS;i++)if(persistent_fields[i].source_age>persistent_fields[slot].source_age)slot=i;}
 Construct source={};source.x=player.x;source.y=player.y;
 triggerBurnout(source,18000*(player.mods[M_CONCENTRATED_BLAST]?135:100)/100,(int)minv(4096,1000*globalBlastRadiusPercent()/300));
 PersistentField &f=persistent_fields[slot];f.reserved[0]=1;f.burn=0;f.radius=(uint16_t)(f.base_radius/2);
 // Original transition removes enemy beam/blast objects, not every normal shot.
 for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;i++)if(bullets[i].active&&(bullets[i].kind==SHOT_BEAMCASTER||bullets[i].kind==SHOT_TRACER||bullets[i].kind==SHOT_TRACER_WAKE))bullets[i].active=0;
 emitEffect(player.x,player.y,FX_RING,255,18,0);
}

void Game::queueChallenger() {
 challenger_pending_=(uint16_t)minv(65535,(unsigned)challenger_pending_+1);
}
void Game::updateChallenger() {
 if(!challenger_pending_)return;
 if(challenger_delay_){--challenger_delay_;return;}
 static const SourceEnemy family[]={SE_BEAMCASTER,SE_MYRMIDON,SE_PULSAR,SE_SNAPPER,SE_SERPENT,SE_VANGUARD,SE_TRACER,SE_HIVE,SE_BULWARK,SE_JUGGERNAUT,SE_CELESTIAL,SE_SPITFIRE,SE_CERATOTITAN,SE_SCION,SE_DREADNOUGHT,SE_DWELLER,SE_SERAPH,SE_WARBRINGER,SE_GLAUCUS};
 unsigned rank=challenger_spawned_;
 // mChallenger Alarm1 adds rank^3/100 to the selection wave after rank7.
 uint64_t effective=(uint64_t)wave+(rank>7?(uint64_t)rank*rank*rank/100:0);
 if(!challenger_source_){
  SourceEnemy candidates[19];unsigned count=0;
  int floor=100+((run_flags&RUN_ANNIHILATION)?-5:0);
  for(unsigned j=0;j<19;j++){
   int low=j<11?-10:10,high=j<11?80:100;
   int champion=sourceEnemyInfo(family[j],run_flags).champion_wave;
   bool eligible=(effective>=(unsigned)maxv(0,champion+low)&&effective<=(unsigned)(champion+high))||
       (champion>=floor&&effective>=(unsigned)(floor+low));
   if(eligible)candidates[count++]=family[j];
  }
  // Preserve a queued request until its source eligibility window opens.
  if(!count)return;
  challenger_source_=(int16_t)(candidates[range(count)]+1);
 }
 SourceEnemy type=(SourceEnemy)(challenger_source_-1);
 unsigned edge=range(4);int x=edge==0?5:edge==2?worldWidth()-5:(int)range(worldWidth());
 int y=edge==1?5:edge==3?worldHeight()-5:(int)range(worldHeight());
 int at=spawnSourceEnemy(type,x,y,2);
 if(at<0)return; // Keep this exact selected recipe until both pools can fit it.
 Enemy &e=enemies[at];e.challenger=1;
 const SourceEnemyInfo &info=sourceEnemyInfo(type,run_flags);
 int quantity_extra=(int)info.quantity_mean_q8-256;
 uint32_t size_base=(uint32_t)maxv(0,9830+quantity_extra*256/30);
 uint32_t damage_base=(uint32_t)maxv(0,8192+quantity_extra*256/30);
 uint32_t size_q16=modCap64(65536+(uint64_t)size_base*(rank+2)/2,1048576);
 uint32_t damage_q16=modCap64(65536+(uint64_t)damage_base*(rank+2)/2,1048576);
 e.size_scale=(uint16_t)((size_q16+128)/256);
 e.damage_scale=(uint16_t)((damage_q16+128)/256);
 e.treasure_scale=(uint16_t)modCap64((uint64_t)size_q16*damage_q16/16777216,4096);
 uint32_t hull_factor=(uint32_t)((uint64_t)size_q16*size_q16/65536);
 auto scale_hull=[&](int32_t hull)->int32_t{return (int32_t)modCap64((uint64_t)maxv(1,hull)*hull_factor/65536,2000000000);};
 e.hull=e.max_hull=scale_hull(e.max_hull);e.radius=(uint8_t)clamp((e.radius*e.size_scale+128)/256,2,64);
 e.formation_delay=15;
 for(unsigned part=0;part<MAX_BOSS_PARTS;part++){
  BossPart &p=boss_parts[part];if(!p.active||p.owner!=(unsigned)at)continue;
  p.hull=p.max_hull=scale_hull(p.max_hull);
  p.source_scale=(uint16_t)minv(65535,(unsigned)p.source_scale*e.size_scale/256);
  p.radius=(uint8_t)clamp((p.radius*e.size_scale+128)/256,2,100);
  if(e.type==E_WORM){
   int32_t dx=delta(p.x,e.x,worldWidth())*FP,dy=delta(p.y,e.y,worldHeight())*FP;
   p.x=wrap(e.x+(int64_t)dx*e.size_scale/256,worldWidth());p.y=wrap(e.y+(int64_t)dy*e.size_scale/256,worldHeight());
  }else{
   p.local_x=(int16_t)clamp((int)p.local_x*e.size_scale/256,-32767,32767);
   p.local_y=(int16_t)clamp((int)p.local_y*e.size_scale/256,-32767,32767);
  }
 }
 SourceSpawnSpec delay=emptySourceSpec(type);delay.count=1;delay.challenger=1;delay.size_q8=e.size_scale;delay.damage_q8=e.damage_scale;
 next_wave=(uint16_t)minv(65535,(unsigned)next_wave+sourceRecipeDelayTicks(&delay,1,wave,run_flags,options.assist_mode));
 --challenger_pending_;++challenger_spawned_;challenger_source_=0;challenger_delay_=challenger_pending_?60:0;
}
void Game::rewardChallenger(const Enemy &enemy) {
 if(!enemy.challenger)return;
 // spawnTreasure grants a level directly and one of four source power-ups.
 bonusLevels(1);
 unsigned power=range(player.mods[M_X_POWER_SPIKE]?3:4);
 bool free=false;for(unsigned i=0;i<MAX_PICKUPS;i++)if(!pickups[i].active){free=true;break;}
 if(free){
  if(power==3)emitPickup(enemy.x,enemy.y,PICKUP_REROLL,1);
  else emitPickup(enemy.x,enemy.y,PICKUP_POWER,power+1);
 }else{
  // Fixed-pool saturation delivers the selected reward immediately instead
  // of silently deleting it; ordinary drops retain their visible pickup.
  if(power==3)rerolls=(uint16_t)minv(99,rerolls+1);
  else if(power==1)source_armor_ticks_=150;
  else if(power==2)source_medi_ticks_=(uint16_t)minv(65535,(unsigned)source_medi_ticks_+30);
  else hyperboost_ticks_=450;
 }
}
