// Original wave recipes are recovered in source_waves.cpp. This consumer maps
// their spatial formations to the fixed 320x240 field and preserves delayed
// spawns when the 64-slot enemy pool fills; it does not silently truncate them.
namespace {
int sourceEnemyId(SourceEnemy type) {
 static const int8_t ids[SOURCE_ENEMY_COUNT]={
 E_ASTEROID,E_ASTEROID,E_SKIRMISHER,E_WARDEN,E_GYROGUN,E_MINE,
 E_SWARMLING,E_SWARMLING_GREEN,E_HOST,E_BEAMCASTER,E_MYRMIDON,E_PULSAR,
 E_SERPENT,E_BOLT_THROWER,E_FRIGATE,E_SNAPPER,E_WORM,E_WASP,E_HIVE,
 E_CRAB_NEST,E_TRACER,E_SHREDDER_MINE,E_MINELAYER,E_ECLIPSE,E_CONSTRUCTOR,
 E_CHERUB,E_BULWARK,E_JUGGERNAUT,E_CELESTIAL,E_SPITFIRE,E_CERATOTITAN,
 E_STATION_OMEGA,E_DREADNOUGHT,E_SCION,E_DWELLER,E_WARBRINGER,E_CRUCIBLE,
 E_SERAPH,E_GLAUCUS,E_DWELLER_DRONE,E_DWELLER_GUARD,E_DWELLER_WARRIOR,-1};
 return (unsigned)type<SOURCE_ENEMY_COUNT?ids[type]:-1;
}
int sourceDefault(int value,int fallback) {return value==SOURCE_UNDEFINED?fallback:value;}
unsigned sourceTicks(int value,unsigned fallback=0) {return value==SOURCE_UNDEFINED?fallback:(unsigned)clamp((value+1)/2,0,65535);}
// scaleWaves: .0045 decrement clamped to [.25,1], then difficulty /
// Assist / Practice multipliers. Padding is separate from the enemy budget.
unsigned sourceRecipeDelayTicks(const SourceSpawnSpec *specs,unsigned count,unsigned wave,unsigned flags,bool assist) {
 uint64_t mult=(uint64_t)maxv(2500,10000-(int)wave*45)*65536/10000;
 unsigned danger=(flags&RUN_DANGER)?(flags&RUN_ANNIHILATION?553125:flags&RUN_MAYHEM?593750:675000):1000000;
 mult=mult*danger/1000000;if(assist)mult=mult*13/10;if(flags&RUN_PRACTICE)mult=mult*13/10;
 uint64_t padding=wave>=180?20*65536:80*65536-wave*65536/3;
 if(wave<30)padding+=(uint64_t)(30-wave)*16*65536*(1000000+danger)/2000000;
 uint64_t total=0;
 for(unsigned i=0;i<count;i++){
  const SourceSpawnSpec &s=specs[i];unsigned n=(unsigned)maxv(1,sourceDefault(s.count,1)),repeat=(unsigned)maxv(0,sourceDefault(s.repeating,0));
  unsigned flanks=s.spawn==2?2:1,first=flanks==2?maxv(1,(int)n/2):n;
  uint64_t roots=first+(uint64_t)repeat*first*flanks;
  uint64_t base=(uint64_t)sourceEnemyDelayFrames((SourceEnemy)s.enemy)*roots*mult;
  unsigned frequency=(unsigned)maxv(0,sourceDefault(s.repeat_frequency_ticks,s.entry==1?150:0));
  base+=((uint64_t)frequency*roots+(unsigned)maxv(0,sourceDefault(s.added_delay_ticks,0))+(s.entry==2?75u:0u))*65536;
  if(s.challenger!=SOURCE_UNDEFINED&&s.challenger)base+=base*2*(unsigned)maxv(1,sourceDefault(s.size_q8,256))*(unsigned)maxv(1,sourceDefault(s.damage_q8,256))/65536;
  uint64_t followers=(uint64_t)maxv(1,sourceDefault(s.follower_count,1))*maxv(1,sourceDefault(s.repeat_formation,1))*(repeat+1);
  base+=(uint64_t)sourceEnemyDelayFrames((SourceEnemy)s.follower)*followers*mult;
  total+=base+padding;
 }
 total=(total+131071)/131072;return total>65535?65535:(unsigned)total;
}
SourceSpawnSpec emptySourceSpec(SourceEnemy enemy) {
 SourceSpawnSpec s;int32_t *p=&s.enemy;for(unsigned i=0;i<31;i++)p[i]=SOURCE_UNDEFINED;s.enemy=enemy;return s;
}
}
unsigned Game::sourceEnemyRarity(SourceEnemy type) {
 const SourceEnemyInfo &info=sourceEnemyInfo(type,run_flags);
 // getRarity 0x140321c45: one inclusive irandom(99)+1 roll for both
 // rarity gates. scaleWaves: rareChance=10+.45*wave. Champion uses
 // that SAME roll and round(rareChance)/3, not a second random trial.
 unsigned rarity=0,roll=1+range(100),chance=(1000+45*wave+50)/100;
 if(wave>=info.elite_wave&&roll<chance)rarity=1;
 if(wave>=info.champion_wave&&roll*3<chance)rarity=2;
 if(type==SE_ASTEROID||type==SE_ASTEROID_MEGA||type==SE_SWARMLING_GREEN)rarity=0;
 return rarity;
}
int Game::spawnSourceEnemy(SourceEnemy type,int x,int y,int rarity) {
 int id=sourceEnemyId(type);if(id<0)return -1;
 if(rarity==SOURCE_UNDEFINED||rarity<0)rarity=(int)sourceEnemyRarity(type);
 int slot=spawnEnemy((unsigned)id,x,y,(unsigned)clamp(rarity,0,2));
 if(slot>=0&&type==SE_ASTEROID_MEGA){Enemy &e=enemies[slot];e.phase=3;e.radius=19;e.hull=e.max_hull=720;}
 return slot;
}
void Game::loadWaveSpawns(const SourceSpawnSpec *specs,unsigned count) {
 clearBytes(wave_queue_,sizeof(wave_queue_));wave_queue_count_=(uint8_t)minv(SOURCE_MAX_SPECS,count);
 for(unsigned i=0;i<wave_queue_count_;i++){
  WaveSpawnQueue &q=wave_queue_[i];q.spec=specs[i];q.leader=-1;
  q.delay=(uint16_t)sourceTicks(q.spec.added_delay_ticks);
  q.edge=(uint8_t)(q.spec.fixed_direction==SOURCE_UNDEFINED?range(4):(unsigned)q.spec.fixed_direction%4);
  q.origin_x=(int16_t)range(worldWidth());q.origin_y=(int16_t)range(worldHeight());
  if(sourceEnemyId((SourceEnemy)q.spec.enemy)<0)q.done=1;
 }
}
unsigned Game::pendingWaveSpawns() const {
 unsigned total=0;
 for(unsigned i=0;i<wave_queue_count_;i++){
  const WaveSpawnQueue &q=wave_queue_[i];if(q.done)continue;
  unsigned count=(unsigned)maxv(1,sourceDefault(q.spec.count,1));
  unsigned repeats=(unsigned)maxv(0,sourceDefault(q.spec.repeating,0));
  unsigned followers=q.spec.follower==SOURCE_UNDEFINED?0:(unsigned)maxv(1,sourceDefault(q.spec.follower_count,1));
  followers*=(unsigned)maxv(1,sourceDefault(q.spec.repeat_formation,1));
  unsigned supports=q.spec.support==SOURCE_UNDEFINED?0:(unsigned)maxv(1,sourceDefault(q.spec.support_count,1));
  unsigned roots=(repeats+1-q.repeat_index)*count-q.root_index;
  total+=roots*(1+followers+supports);
  if(q.leader>=0)total-=1+minv(followers+supports,q.follower_index);
 }
 return total;
}
void Game::updateWaveSpawns() {
 unsigned budget=8;
 for(unsigned i=0;i<wave_queue_count_;i++){
  WaveSpawnQueue &q=wave_queue_[i];const SourceSpawnSpec &s=q.spec;
  if(q.done)continue;
  if(q.delay){--q.delay;continue;}
  unsigned count=(unsigned)maxv(1,sourceDefault(s.count,1));
  unsigned followers=s.follower==SOURCE_UNDEFINED?0:(unsigned)maxv(1,sourceDefault(s.follower_count,1));
  unsigned rings=(unsigned)maxv(1,sourceDefault(s.repeat_formation,1));
  unsigned supports=s.support==SOURCE_UNDEFINED?0:(unsigned)maxv(1,sourceDefault(s.support_count,1));
  unsigned child_count=followers*rings+supports;
  while(budget&&!q.done){
   if(q.leader>=0&&q.follower_index>=child_count){
    q.leader=-1;q.follower_index=0;++q.root_index;
    if(q.root_index>=count){
     q.root_index=0;
     if(q.repeat_index>=(unsigned)maxv(0,sourceDefault(s.repeating,0))){q.done=1;break;}
     ++q.repeat_index;q.delay=(uint16_t)maxv(1,sourceTicks(s.repeat_frequency_ticks,30));break;
    }
   }
   if(enemyCount()>=MAX_ENEMIES)break;
   if(q.leader<0){
    int w=worldWidth(),h=worldHeight(),x=q.origin_x,y=q.origin_y;
    unsigned position=(q.root_index+1)*65536/(count+1),edge=q.edge;
    int formation=sourceDefault(s.spawn,0);
    if(formation==2){edge=(q.root_index&1)?2:0;position=((q.root_index/2+1)*65536)/((count+1)/2+1);}
    if(formation==7){edge=(q.root_index+q.edge)%4;position=((q.root_index/4+1)*65536)/((count+3)/4+1);}
    if(formation==6){edge=(q.root_index+q.repeat_index+q.edge)%4;position=(q.root_index*25031u+32768u)&65535u;}
    if(formation==8&&s.spawn_parameter_q8!=SOURCE_UNDEFINED)position=(unsigned)clamp(s.spawn_parameter_q8*256,0,65535);
    if(formation==3||formation==4){int spacing=formation==4?maxv(1,absn(sourceDefault(s.spawn_parameter_q8,84*256))/1536):10;int offset=((int)q.root_index-(int)count/2)*spacing;x=clamp(q.origin_x+offset,8,w-8);y=clamp(q.origin_y+absn(offset)/2,8,h-8);}
    else {if(edge==0||edge==2){x=edge==0?5:w-5;y=8+(int)(position*(unsigned)(h-16)/65536);}else{x=8+(int)(position*(unsigned)(w-16)/65536);y=edge==1?5:h-5;}}
    if(formation==5){unsigned arc=(unsigned)maxv(15,absn(sourceDefault(s.spawn_parameter_q8,90*256))/256);uint8_t a=(uint8_t)(edge*64+128+(int)(q.root_index*arc*256/maxv(1,count)/360)-(int)arc*128/360);x=w/2+dirX(a)*(w/2-8)/256;y=h/2+dirY(a)*(h/2-8)/256;}
    if(s.entry==1||s.entry==2){uint8_t a=(uint8_t)(q.edge*64+q.root_index*29);x=pixel(wrap(player.x+dirX(a)*85,w));y=pixel(wrap(player.y+dirY(a)*70,h));}
    int slot=spawnSourceEnemy((SourceEnemy)s.enemy,x,y,s.rarity);if(slot<0)break; // A full part/enemy pool defers this recipe.
    Enemy &e=enemies[slot];q.leader=(int16_t)slot;q.leader_generation=e.generation;
    e.angle=direction(delta(player.x,e.x,w),delta(player.y,e.y,h));
    e.formation_delay=(uint16_t)sourceTicks(s.action_delay_ticks);
    if(s.entry==1||s.entry==2)e.formation_delay=(uint16_t)maxv(15,e.formation_delay);
    if(s.size_q8!=SOURCE_UNDEFINED)e.radius=(uint8_t)clamp(e.radius*s.size_q8/256,2,64);
    e.damage_scale=(uint16_t)clamp(sourceDefault(s.damage_q8,256),1,4096);e.treasure_scale=(uint16_t)clamp(sourceDefault(s.treasure_q8,256),1,4096);
    int add=sourceEnemyId((SourceEnemy)s.add_enemy);if(add>=0){e.summon_type=(uint8_t)add;e.summon_interval=(uint16_t)maxv(1,sourceTicks(s.add_frequency_ticks,330));e.summon_timer=e.summon_interval;}
    --budget;
   } else {
    bool support=q.follower_index>=followers*rings;
    SourceEnemy type=(SourceEnemy)(support?s.support:s.follower);
    unsigned ring=support?0:q.follower_index/maxv(1,followers),at=support?q.follower_index-followers*rings:q.follower_index%maxv(1,followers);
    Enemy *leader=&enemies[q.leader];bool linked=leader->active&&leader->generation==q.leader_generation;
    int32_t lx=linked?leader->x:q.origin_x*FP,ly=linked?leader->y:q.origin_y*FP;
    unsigned n=support?supports:followers;uint8_t angle=(uint8_t)(at*256/maxv(1,n));
    int spacing=maxv(9,absn(sourceDefault(s.behavior_parameter2_q8,108*256))/1536)+ring*9;
    int rarity=sourceEnemyRarity(type);rarity=maxv(sourceDefault(s.follower_min_rarity,0),minv(sourceDefault(s.follower_max_rarity,2),rarity));
    int slot=spawnSourceEnemy(type,pixel(lx)+dirX(angle)*spacing/256,pixel(ly)+dirY(angle)*spacing/256,rarity);
    if(slot<0){if(sourceEnemyId(type)>=0)break;++q.follower_index;continue;}
    Enemy &e=enemies[slot];e.leader=linked?(int8_t)q.leader:-1;e.leader_generation=q.leader_generation;
    e.formation=(uint8_t)(support?5:sourceDefault(s.behavior,4));e.formation_slot=(uint8_t)at;e.formation_count=(uint8_t)minv(255,n);
    e.formation_spacing=(uint16_t)spacing;e.formation_angle=(int16_t)(sourceDefault(s.behavior_parameter1_q8,0)*256/360);
    e.formation_delay=(uint16_t)minv(65535,sourceTicks(s.action_delay_ticks)*(q.follower_index+1));e.formation_break=(uint16_t)sourceTicks(s.break_delay_ticks);
    unsigned scale=256;for(unsigned r=0;r<ring;r++)scale=scale*(unsigned)clamp(sourceDefault(s.repeat_size_q8,256),64,512)/256;
    e.radius=(uint8_t)clamp(e.radius*(int)scale/256,2,64);e.damage_scale=linked?leader->damage_scale:256;e.treasure_scale=linked?leader->treasure_scale:256;
    ++q.follower_index;--budget;
   }
  }
 }
 bool pending=false;for(unsigned i=0;i<wave_queue_count_;i++)if(!wave_queue_[i].done)pending=true;
 if(!pending)wave_queue_count_=0;
}
void Game::dynamicWaveSpawns() {
 // Source selection gates, triangular quantity distribution and wave scaling.
 // Keep fractional quantities until formation-specific rounding.
 unsigned total=0;
 for(unsigned i=0;i<SOURCE_ENEMY_COUNT;i++){const SourceEnemyInfo &info=sourceEnemyInfo((SourceEnemy)i,run_flags);if(sourceEnemyId((SourceEnemy)i)>=0&&wave>=info.min_wave)total+=info.weight_q8;}
 unsigned roll=range(total),selected=SE_INTERCEPTOR;
 for(unsigned i=0;i<SOURCE_ENEMY_COUNT;i++){const SourceEnemyInfo &info=sourceEnemyInfo((SourceEnemy)i,run_flags);if(sourceEnemyId((SourceEnemy)i)<0||wave<info.min_wave||!info.weight_q8)continue;if(roll<info.weight_q8){selected=i;break;}roll-=info.weight_q8;}
 const SourceEnemyInfo &info=sourceEnemyInfo((SourceEnemy)selected,run_flags);
 SourceSpawnSpec s=emptySourceSpec((SourceEnemy)selected);
 unsigned first=range(65536),second=range(65536),variance=range(65536);
 uint32_t amount=sourceDynamicAmountQ16((SourceEnemy)selected,wave,run_flags,first,second,variance);
 s.count=clamp((int)((amount+32768)/65536),info.quantity_min,info.quantity_max);
 s.entry=0;s.spawn=0;next_wave=(uint16_t)sourceRecipeDelayTicks(&s,1,wave,run_flags,options.assist_mode);loadWaveSpawns(&s,1);wave_name_kind_=254;
}
const char *Game::waveName() const {
 if(boss_index>=0&&enemies[boss_index].active)return enemy_types[enemies[boss_index].type].name;
 if(wave<=1)return "ASTEROID BELT";
 if(wave<=5)return "INTERCEPTORS";
 if(wave_name_kind_<22)return sourceCuratedName(wave_name_kind_);
 return "ENEMY FORMATION";
}
void Game::updateWaves() {
 if(updateDailyWaves())return;
 unsigned hazard_interval=(run_flags&RUN_HOSTILE)?((run_flags&RUN_MAYHEM)?350:400):600;
 if(++hazard_clock_>=hazard_interval){hazard_clock_=0;if(enemyCount()<40)spawnEnemy(E_ASTEROID,(int)range(worldWidth()),4);}
 updateWaveSpawns();if(next_wave)--next_wave;
 if((daily(DAILY_TURBO)||daily(DAILY_TURBO_ENDLESS))&&frame%9==0&&next_wave)--next_wave;
 unsigned alive=enemyCount();if(wave&&!alive&&!wave_queue_count_&&next_wave>45)next_wave=45;
 if(next_wave||wave_queue_count_)return;
 if(boss_index>=0&&enemies[boss_index].active){next_wave=30;return;}
 if(alive>38){next_wave=30;return;}
 // oWaves Alarm1 0x140f01801..0x140f01891 adds2 for Turbo when
 // the current wave is even, and1 otherwise. The first wave remains1.
 unsigned wave_step=(daily(DAILY_TURBO)||daily(DAILY_TURBO_ENDLESS))&&wave&&!(wave&1)?2:1;
 wave=(uint16_t)(wave+wave_step);wave_banner=90;wave_name_kind_=255;
 bool final=(wave==120||wave%240==0),boss_wave=final||(wave<=100&&wave%20==0);
 if(boss_wave){static const uint8_t pairs[5][2]={{E_CERATOTITAN,E_SPITFIRE},{E_STATION_OMEGA,E_DREADNOUGHT},{E_SCION,E_DWELLER},{E_WARBRINGER,E_CRUCIBLE},{E_SERAPH,E_GLAUCUS}};
  unsigned tier=wave<=100?wave/20-1:(wave/20-1)%5;unsigned boss=final?(unsigned)E_STAR_EATER:(unsigned)pairs[tier][(wave_pattern_>>tier)&1];if(wave>120&&!final)boss=pairs[tier][range(2)];if(spawnEnemy(boss,worldWidth()/2,25)<0){wave=(uint16_t)(wave-wave_step);next_wave=30;return;}next_wave=900;return;}
 if(wave<=5){
  // Native spawnCuratedWaves 0x14061baa4..0x14061ccc1, replayed for
  // each branch. Added delays retain the original 60 Hz frame unit.
  SourceSpawnSpec opening[3];unsigned count=0;
  auto add=[&](SourceEnemy type,int amount)->SourceSpawnSpec&{opening[count]=emptySourceSpec(type);opening[count].count=amount;return opening[count++];};
  if(wave==1){add(SE_ASTEROID,3+(int)range(2));if(range(20)==0)add(SE_ASTEROID_MEGA,1);}
  else if(wave==2)add(SE_INTERCEPTOR,2);
  else if(wave==3){add(SE_INTERCEPTOR,1);SourceSpawnSpec &second=add(SE_INTERCEPTOR,1);second.fixed_direction=range(2)?6:7;second.added_delay_ticks=30;add(SE_INTERCEPTOR,1);}
  else if(wave==4){add(SE_INTERCEPTOR,2).spawn=2;add(SE_WARDEN,1).added_delay_ticks=150;}
  else {SourceSpawnSpec &v=add(SE_INTERCEPTOR,3);v.spawn=4;v.spawn_parameter_q8=120*256;add(SE_WARDEN,1);SourceSpawnSpec &reinforcements=add(SE_INTERCEPTOR,2);reinforcements.spawn=3;reinforcements.added_delay_ticks=210;}
  next_wave=(uint16_t)sourceRecipeDelayTicks(opening,count,wave,run_flags,options.assist_mode);loadWaveSpawns(opening,count);updateWaveSpawns();return;
 }
 SourceSpawnSpec specs[SOURCE_MAX_SPECS];uint32_t before=random_,choice=before;choice^=choice<<13;choice^=choice>>17;choice^=choice<<5;
 unsigned count=sourceCuratedWave(wave,run_flags,random_,specs,SOURCE_MAX_SPECS,dailyMod(DAILY_COSMIC_BALLET),daily(DAILY_TURBO)||daily(DAILY_TURBO_ENDLESS));
 if(count){unsigned limit=(run_flags&RUN_ELITE)&&wave>=((run_flags&RUN_ANNIHILATION)?80u:120u)?22:15;wave_name_kind_=(uint8_t)(choice%limit);if((wave>=300&&wave%50==0)||(wave>100&&wave%100==0))wave_name_kind_=21;next_wave=(uint16_t)sourceRecipeDelayTicks(specs,count,wave,run_flags,options.assist_mode);loadWaveSpawns(specs,count);}
 else dynamicWaveSpawns();
 updateWaveSpawns();
}
