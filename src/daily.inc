// Offline Daily Challenge and physical cosmic encounters. The numeric daily
// modifiers below come from setup 0x140579028..0x140579750 (1.2.38.4).
// Date rotation, hazard scheduling and low-resolution geometry are native-port
// policies; this is not the PC service's daily seed or online leaderboard.
uint32_t Game::dailySeedFor(uint32_t day) {
 uint32_t x=day^0x4e4f5641u;x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
 return x?x:0x4e4f5641u;
}
unsigned Game::dailyVariantFor(uint32_t day) {return day%DAILY_VARIANT_COUNT;}
unsigned Game::dailyModifierFor(uint32_t day) {
 unsigned modifier=1+(dailySeedFor(day)^0x9e3779b9u)%(DAILY_MODIFIER_COUNT-1);
 // The snake challenge has no gear selection, so choose a usable modifier.
 if(dailyVariantFor(day)==DAILY_SERPENTINE&&(modifier==DAILY_CHOSEN||modifier==DAILY_UNSTOPPABLE||modifier==DAILY_IMMOVABLE))modifier=DAILY_CELESTIAL_SHOWER;
 return modifier;
}
void Game::configureDaily(uint32_t day) {
 if(frame)return;
 daily_day=day;daily_variant=(uint8_t)dailyVariantFor(day);daily_modifier=(uint8_t)dailyModifierFor(day);
 run_flags=(run_flags|RUN_DAILY)&~(RUN_DRAFT|RUN_ENDLESS|RUN_PRACTICE);
 wave=starting_wave=0;level=0;xp=0;xp_next=15;banked_upgrades=0;state=Playing;next_wave=1;
 clearBytes(player.mods,sizeof(player.mods));gear_choices_=gear_owned_=0;run_stats.mods_taken=0;
 player.weapon=W_BLASTER;player.body=B_STANDARD;player.shield_type=S_STANDARD;
 if(daily(DAILY_SERPENTINE)||daily(DAILY_DRAFT20)||daily(DAILY_DRAFT45)||daily(DAILY_TURBO_ENDLESS))run_flags|=RUN_ENDLESS;
 if(daily(DAILY_SERPENTINE))run_flags|=RUN_WILD;
 if(daily(DAILY_BOSS_RUSH)||daily(DAILY_DRAFT20)||daily(DAILY_DRAFT45)||daily(DAILY_COSMIC_EGG)||daily(DAILY_TURBO_ENDLESS)||daily(DAILY_NOVA_BROTHERS))run_flags&=~RUN_WILD;
 if(dailyMod(DAILY_WILDER)||dailyMod(DAILY_PANDEMONIUM)||dailyMod(DAILY_WILDERNESS))run_flags|=RUN_WILD;
 wild_mode=(run_flags&RUN_WILD)!=0;rerolls=dailyMod(DAILY_ROGUE)?0:10;
 if(daily(DAILY_SERPENTINE)){player.body=B_LEVIATHAN;gear_choices_=3;}
 if(daily(DAILY_DRAFT20)||daily(DAILY_DRAFT45)){
  run_flags|=RUN_DRAFT;wave=starting_wave=daily(DAILY_DRAFT20)?40:200;
  bonusLevels(daily(DAILY_DRAFT20)?20:45);
 }
 if(daily(DAILY_PURITY))player.mods[M_X_ATARAXIA]=1;
 if(daily(DAILY_SWORD)){player.weapon=W_SWORDS;gear_choices_=1;gear_owned_=1u<<OFFER_WEAPON;}
 if(daily(DAILY_NOVA_BROTHERS)){player.mods[M_ALLY]=1;banished_[M_WINGMAN]=1;}
 if(dailyMod(DAILY_CHOSEN)){
  player.weapon=(uint8_t)range(WEAPON_COUNT);player.body=(uint8_t)range(BODY_COUNT);player.shield_type=(uint8_t)range(SHIELD_COUNT);
  if(daily(DAILY_SWORD))player.weapon=W_SWORDS;
  gear_choices_=3;gear_owned_=(1u<<OFFER_WEAPON)|(1u<<OFFER_BODY)|(1u<<OFFER_SHIELD);
 }
 recalculate();player.x=worldWidth()*FP/2;player.y=worldHeight()*FP*3/4;
 sanctuary_x=player.x;sanctuary_y=player.y;
 for(unsigned i=0;i<MAX_SEGMENTS;i++){segment_x[i]=player.x;segment_y[i]=player.y;}
 player.hull=(int16_t)hullRecoveryCap();player.shield=(int16_t)shieldRecoveryCap();
 if(daily(DAILY_COSMIC_EGG)){daily_egg_x=worldWidth()*FP/2;daily_egg_y=worldHeight()*FP/2;daily_egg_hull=15000;}
 if(daily(DAILY_NOVA_BROTHERS)){int slot=spawnConstruct(C_ALLY);if(slot>=0)constructs[slot].hull=constructs[slot].max_hull;}
 if(banked_upgrades)beginUpgrade();
}
unsigned Game::dailyEnemySpeedPercent() const {
 return 100+(dailyMod(DAILY_FRENZY)?12:0)+(daily(DAILY_BOSS_RUSH)||daily(DAILY_TURBO)||daily(DAILY_TURBO_ENDLESS)?5:0);
}
bool Game::updateDailyWaves() {
 if(daily(DAILY_BOSS_RUSH)){
  if(next_wave)--next_wave;
  if(boss_index>=0&&enemies[boss_index].active)return true;
  if(next_wave)return true;
  static const uint8_t gauntlet[]={E_CERATOTITAN,E_SPITFIRE,E_STATION_OMEGA,E_DREADNOUGHT,E_SCION,E_DWELLER,E_WARBRINGER,E_CRUCIBLE,E_SERAPH,E_GLAUCUS,E_STAR_EATER};
  if(daily_boss_step_>=sizeof(gauntlet)){victory=true;state=GameOver;return true;}
  unsigned stage=daily_boss_step_;wave=(uint16_t)((stage/2+1)*20);if(stage==10)wave=120;
  int boss=spawnEnemy(gauntlet[stage],worldWidth()/2,25);
  if(boss>=0){boss_index=(int16_t)boss;++daily_boss_step_;wave_banner=90;next_wave=60;}
  return true;
 }
 if(daily(DAILY_SERPENTINE)){
  if(next_wave)--next_wave;
  if(!next_wave){++wave;wave_banner=60;next_wave=150;unsigned count=2+minv(6,wave/10);
   for(unsigned i=0;i<count;i++)spawnEnemy(E_ASTEROID,(int)range(worldWidth()),(int)range(worldHeight()));
   // Collectable orbs drive the Leviathan's growth; the head must be steered
   // around its own trailing body and the physical cosmic hazards.
   for(unsigned i=0;i<3;i++)emitPickup((int)range(worldWidth())*FP,(int)range(worldHeight())*FP,PICKUP_XP,5+wave/2);
  }
  return true;
 }
 return false;
}
void Game::damageDailyEgg(unsigned damage) {
 if(!daily(DAILY_COSMIC_EGG)||daily_egg_hull<=0||daily_egg_grace||!damage)return;
 daily_egg_hull=(int16_t)maxv(0,daily_egg_hull-(int)minv(30000,damage));daily_egg_grace=3;
 emitEffect(daily_egg_x,daily_egg_y,FX_HIT,12,6,1);
 if(!daily_egg_hull){player.hull=0;victory=false;state=GameOver;emitEffect(daily_egg_x,daily_egg_y,FX_BURST,40,30,1);}
}
void Game::updateDaily() {
 if(daily_egg_grace)--daily_egg_grace;
 if(daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0){
  for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&!enemies[i].boss_hidden&&distance2(enemies[i].x,enemies[i].y,daily_egg_x,daily_egg_y)<(enemies[i].radius+12)*(enemies[i].radius+12))damageDailyEgg(enemy_types[enemies[i].type].contact);
 }
 if(daily(DAILY_SERPENTINE)&&frame>90&&!player.invulnerable){
  for(unsigned i=4;i<segment_count;i++)if(distance2(player.x,player.y,segment_x[i],segment_y[i])<49){damagePlayer(50);break;}
 }
 if(dailyMod(DAILY_ROGUE))rerolls=0;
 updateCosmicObjects();
}
int Game::spawnCosmic(unsigned kind,int x,int y,uint8_t angle,unsigned warning) {
 if(kind>=COSMIC_KIND_COUNT)return -1;
 for(unsigned i=0;i<MAX_COSMIC_OBJECTS;i++)if(!cosmic_objects[i].active){
  CosmicObject &c=cosmic_objects[i];clearBytes(&c,sizeof(c));c.active=1;c.kind=(uint8_t)kind;c.x=x*FP;c.y=y*FP;c.angle=angle;c.warning=(uint16_t)warning;
  c.radius=kind==COSMIC_COMET?5:kind==COSMIC_BLACK_HOLE?20:6;
  // setEnemyHull source line40: both cargo object classes share
  // (60+40)*enemyHullFactor; native baseEnemyHullFactor is1.
  c.hull=kind==COSMIC_TRAIN_ENGINE||kind==COSMIC_TRAIN_CAR?(int16_t)minv(32767,(800+maxv(0,wave-120)*3)/8):1;
  c.life=kind==COSMIC_BLACK_HOLE?600:kind==COSMIC_COMET?240:900;
  int speed=kind==COSMIC_COMET?4*FP:3*FP/2;c.vx=dirX(angle)*speed/FP;c.vy=dirY(angle)*speed/FP;
  return (int)i;
 }
 return -1;
}
void Game::updateCosmicObjects() {
 // Fixed pools avoid heap growth. Encounters retry when every slot is in use.
 unsigned comet_interval=dailyMod(DAILY_CELESTIAL_SHOWER)?540:(run_flags&RUN_HOSTILE)?900:1500;
 if(++cosmic_clock_>=comet_interval&&wave>=5){
  unsigned n=dailyMod(DAILY_CELESTIAL_SHOWER)?4:2;bool spawned=false;
  uint8_t direction=(uint8_t)(16+range(32));int y=(int)range(maxv(1,worldHeight()/2));
  for(unsigned i=0;i<n;i++)spawned|=spawnCosmic(COSMIC_COMET,-8,y+(int)i*22,direction,45+12*i)>=0;
  if(spawned)cosmic_clock_=0;
 }
 unsigned train_interval=dailyMod(DAILY_HEIST)?1080:2700;
 if(++train_clock_>=train_interval&&account_rank>=2&&wave>=5){
  int y=20+(int)range(maxv(1,worldHeight()-40));unsigned length=dailyMod(DAILY_GRACE)?9:6;
  int head=spawnCosmic(COSMIC_TRAIN_ENGINE,-10,y,0);
  if(head>=0){train_clock_=0;cosmic_objects[head].train=(uint8_t)(head+1);
   for(unsigned i=1;i<length;i++){int at=spawnCosmic(COSMIC_TRAIN_CAR,-10-(int)i*16,y,0);if(at>=0)cosmic_objects[at].train=(uint8_t)(head+1);}
   if(dailyMod(DAILY_HEIST)){spawnEnemy(E_WARDEN,5,y-25,1);spawnEnemy(E_INTERCEPTOR_MINE,5,y+25,1);}
  }
 }
 if(++black_hole_clock_>=3600&&wave>=25){int slot=spawnCosmic(COSMIC_BLACK_HOLE,40+(int)range(maxv(1,worldWidth()-80)),40+(int)range(maxv(1,worldHeight()-80)),0,90);if(slot>=0)black_hole_clock_=0;}
 for(unsigned i=0;i<MAX_COSMIC_OBJECTS;i++){
  CosmicObject &c=cosmic_objects[i];if(!c.active)continue;
  if(c.warning){--c.warning;continue;}
  ++c.age;if(c.life&&!--c.life){c.active=0;continue;}
  int32_t oldx=c.x,oldy=c.y;
  if(c.kind!=COSMIC_BLACK_HOLE){c.x+=c.vx;c.y+=c.vy;
   if(c.x>(worldWidth()+100)*FP||c.y>(worldHeight()+100)*FP||c.x< -300*FP||c.y< -100*FP){c.active=0;continue;}}
  else {
   c.angle=(uint8_t)(c.angle+2);unsigned reach=70;
   auto pull=[&](int32_t &x,int32_t &y,int32_t &vx,int32_t &vy){int dx=delta(c.x,x,worldWidth()),dy=delta(c.y,y,worldHeight()),d=approx(dx,dy);if(d<(int)reach&&d>2){vx+=dx*FP/(maxv(8,d)*8);vy+=dy*FP/(maxv(8,d)*8);vx=clamp(vx,-16*FP,16*FP);vy=clamp(vy,-16*FP,16*FP);}};
   pull(player.x,player.y,player.vx,player.vy);
   for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active){pull(enemies[e].x,enemies[e].y,enemies[e].vx,enemies[e].vy);if(frame%10==0&&distance2(c.x,c.y,enemies[e].x,enemies[e].y)<144)hitEnemy(e,30);}
   for(unsigned b=0;b<MAX_BULLETS;b++)if(bullets[b].active&&distance2(c.x,c.y,bullets[b].x,bullets[b].y)<144)bullets[b].active=0;
   for(unsigned p=0;p<MAX_PICKUPS;p++)if(pickups[p].active){pull(pickups[p].x,pickups[p].y,pickups[p].vx,pickups[p].vy);if(distance2(c.x,c.y,pickups[p].x,pickups[p].y)<81)pickups[p].active=0;}
  }
  bool on_screen=c.x>=0&&c.x<worldWidth()*FP&&c.y>=0&&c.y<worldHeight()*FP;
  if(!on_screen)continue;
  if(sweptHit(oldx,oldy,c.x,c.y,player.x,player.y,c.radius+player.radius))damagePlayer(c.kind==COSMIC_COMET?75:c.kind==COSMIC_BLACK_HOLE?8:20,false,(uint8_t)(c.angle+128));
  if(daily(DAILY_COSMIC_EGG)&&sweptHit(oldx,oldy,c.x,c.y,daily_egg_x,daily_egg_y,c.radius+12))damageDailyEgg(c.kind==COSMIC_COMET?75:20);
  if(c.kind==COSMIC_COMET){
   for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active&&sweptHit(oldx,oldy,c.x,c.y,enemies[e].x,enemies[e].y,c.radius+enemies[e].radius))hitEnemy(e,75);
  }
  if(c.kind==COSMIC_TRAIN_ENGINE||c.kind==COSMIC_TRAIN_CAR){
   for(unsigned b=0;b<MAX_BULLETS&&c.hull>0;b++)if(bullets[b].active&&!bullets[b].owner&&sweptHit(bullets[b].x-bullets[b].vx,bullets[b].y-bullets[b].vy,bullets[b].x,bullets[b].y,c.x,c.y,c.radius+bullets[b].radius)){
    int damage=(int)minv(c.hull,(int)((int64_t)bullets[b].damage*globalDamagePermille()/1000));c.hull-=damage;creditDamage((unsigned)damage);bullets[b].active=0;
   }
   if(c.hull<=0){c.active=0;emitEffect(c.x,c.y,FX_BURST,14,16,2);emitPickup(c.x,c.y,PICKUP_XP,c.kind==COSMIC_TRAIN_ENGINE?80:25);emitPickup(c.x,c.y,PICKUP_POWER,1+range(3));score=addSaturated(score,c.kind==COSMIC_TRAIN_ENGINE?5000:1000);}
  }
 }
}
