// Source constants from setup 0x140579028..0x140579750, plus behavior checks
// through the same controllers used by normal gameplay and saved sessions.
#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace nova;
static Game g;
static uint32_t dayFor(unsigned variant,unsigned modifier=DAILY_MODIFIER_COUNT){
 for(uint32_t d=20000;d<30000;d++)if(Game::dailyVariantFor(d)==variant&&(modifier==DAILY_MODIFIER_COUNT||Game::dailyModifierFor(d)==modifier))return d;
 assert(false);return 0;
}
static void setup(unsigned variant,unsigned modifier=DAILY_MODIFIER_COUNT){
 uint32_t d=dayFor(variant,modifier);g.start(Game::dailySeedFor(d));g.configureCareer(64,true);g.configureRun(RUN_DAILY);g.configureDaily(d);assert(g.validState());
}
static void upgrades(){
 for(unsigned i=0;g.state==Upgrade&&i<70;i++){bool chosen=false;for(unsigned j=0;j<g.offer_count&&!chosen;j++)chosen=g.chooseOffer(j);assert(chosen);}
}
int main(){
 for(unsigned variant=0;variant<DAILY_VARIANT_COUNT;variant++){
  setup(variant);assert(g.daily(variant)&&g.daily_day==dayFor(variant));
  uint32_t expected=g.checksum();setup(variant);assert(g.checksum()==expected);
  if(variant==DAILY_DRAFT20)assert(g.wave==40&&g.banked_upgrades==20);
  if(variant==DAILY_DRAFT45)assert(g.wave==200&&g.banked_upgrades==45);
  if(variant==DAILY_COSMIC_EGG)assert(g.daily_egg_hull==15000);
  if(variant==DAILY_SERPENTINE)assert(g.player.body==B_LEVIATHAN&&(g.run_flags&RUN_ENDLESS));
  upgrades();
  for(unsigned tick=0;tick<900;tick++){g.player.invulnerable=65000;if(g.daily_egg_hull)g.daily_egg_hull=15000;GameInput in={};in.up=true;in.left=tick%100<30;in.fire=true;g.tick(in);upgrades();assert(g.validState());}
 }
 setup(DAILY_TURBO,DAILY_CHOSEN);uint32_t fixed=g.checksum(),chosen_day=g.daily_day;
 g.start(888);g.configureCareer(64,true);g.configureRun(RUN_DRAFT);g.configureDaily(chosen_day);assert(g.checksum()==fixed);
 setup(DAILY_PURITY);g.frame=30000;assert(g.dailyCompletionBonus()==1000000);
 setup(DAILY_TURBO);g.frame=30000;assert(g.dailyCompletionBonus()==500000);
 setup(DAILY_BOSS_RUSH);g.frame=30000;assert(g.dailyCompletionBonus()==500000);
 setup(DAILY_TURBO_ENDLESS);g.frame=30000;assert(!g.dailyCompletionBonus());
 setup(DAILY_COSMIC_EGG);g.daily_egg_hull=7500;assert(g.dailyCompletionBonus()==500000);
 g.daily_egg_hull=1;assert(g.dailyCompletionBonus()==67);
 setup(DAILY_COSMIC_EGG);g.state=Playing;g.damageDailyEgg(15000);assert(g.state==GameOver&&!g.victory&&!g.daily_egg_hull);
 setup(DAILY_BOSS_RUSH);g.next_wave=0;
 static const unsigned bosses[]={E_CERATOTITAN,E_SPITFIRE,E_STATION_OMEGA,E_DREADNOUGHT,E_SCION,E_DWELLER,E_WARBRINGER,E_CRUCIBLE,E_SERAPH,E_GLAUCUS,E_STAR_EATER};
 for(unsigned type:bosses){g.state=Playing;g.next_wave=0;assert(g.updateDailyWaves());assert(g.boss_index>=0);unsigned at=(unsigned)g.boss_index;assert(g.enemies[at].type==type);g.destroyEnemy(at);}
 assert(g.victory&&g.state==GameOver&&g.run_stats.boss_kills==11);
 setup(DAILY_PURITY);g.gear_choices_=3;g.banked_upgrades=50;
 for(unsigned n=0;n<7;n++){bool took=false;for(unsigned m=0;m<MOD_COUNT&&!took;m++)took=g.takeMod(m);assert(took);}
 for(unsigned m=0;m<MOD_COUNT;m++)assert(!g.canTakeMod(m));
 g.beginUpgrade();assert(g.state==Playing);
 setup(DAILY_SWORD);int sword=g.spawnConstruct(C_SWORD);assert(sword>=0);int hp=g.constructs[sword].hull;g.damageConstruct(sword,10000);assert(g.constructs[sword].hull==hp);assert(!g.equipWeapon(W_BLASTER));
 setup(DAILY_NOVA_BROTHERS);int ally=-1;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(g.constructs[i].active&&g.constructs[i].kind==C_ALLY)ally=(int)i;assert(ally>=0);g.player.invulnerable=0;int hpAlly=g.constructs[ally].hull;g.damagePlayer(10);assert(g.constructs[ally].hull<hpAlly);assert(!g.canTakeMod(M_WINGMAN));
 assert(g.constructFireRatePercent(C_ALLY)==166);
 g.player.hull=100;g.player.shield=100;g.constructs[ally].hull=0;g.updateConstructs();assert(g.player.hull==40&&g.player.shield==40);
 // Every modifier must produce a valid, playable controller state, including
 // the larger Long Shot spatial grid and no-recovery Boss defenses.
 for(unsigned modifier=1;modifier<DAILY_MODIFIER_COUNT;modifier++){
  setup(DAILY_TURBO,modifier);g.next_wave=65000;g.player.invulnerable=65000;
  for(unsigned tick=0;tick<100;tick++){GameInput in={};in.up=true;in.fire=true;g.tick(in);upgrades();assert(g.validState());}
 }
 setup(DAILY_TURBO,DAILY_CAGE_MATCH);assert(!g.screenWrappingEnabled());
 setup(DAILY_TURBO,DAILY_ROGUE);assert(g.rerolls==0);
 setup(DAILY_TURBO,DAILY_IM_THE_BOSS);g.frame=1;g.player.hull=17;g.player.shield=13;assert(g.hullRecoveryCap()==17&&g.shieldRecoveryCap()==13&&g.regenerationPercent()==0);
 setup(DAILY_TURBO,DAILY_ROGUE);g.next_wave=0;g.updateWaves();assert(g.wave==1);
 for(unsigned before: {1u,2u,18u,118u}){
  for(Enemy &e:g.enemies)e.active=0;
  for(BossPart &p:g.boss_parts)p={};
  g.boss_index=-1;g.wave_queue_count_=0;g.next_wave=0;g.wave=(uint16_t)before;
  g.updateWaves();assert(g.wave==before+(before&1?1:2));
 }
 // A real comet crosses both player and an enemy, rather than only drawing.
 g.start(42);g.configureCareer(64,true);g.next_wave=60000;g.wave=10;g.player.invulnerable=0;g.player.x=160*FP;g.player.y=120*FP;
 int rock=g.spawnEnemy(E_ASTEROID,160,120);g.enemies[rock].hull=g.enemies[rock].max_hull=1000;
 int comet=g.spawnCosmic(COSMIC_COMET,156,120,0);assert(comet>=0);g.updateCosmicObjects();assert(g.player.shield<100&&g.enemies[rock].hull<1000);
 // Train cargo receives weapon hits and actually drops XP plus a power-up.
 for(unsigned i=0;i<MAX_COSMIC_OBJECTS;i++)g.cosmic_objects[i].active=0;
 int car=g.spawnCosmic(COSMIC_TRAIN_CAR,80,80,0);assert(car>=0);g.cosmic_objects[car].vx=0;
 Bullet &b=g.bullets[0];b={};b.active=1;b.kind=SHOT_BLASTER;b.x=80*FP;b.y=80*FP;b.damage=200;b.radius=2;
 g.updateCosmicObjects();assert(!g.cosmic_objects[car].active);unsigned power=0,xp=0;for(const Pickup &p:g.pickups)if(p.active){power+=p.kind==PICKUP_POWER;xp+=p.kind==PICKUP_XP;}assert(power&&xp);
 // A black hole pulls nearby actors and consumes projectiles and pickups.
 g.start(43);g.player.x=130*FP;g.player.y=120*FP;g.player.vx=g.player.vy=0;
 int hole=g.spawnCosmic(COSMIC_BLACK_HOLE,160,120,0);assert(hole>=0);
 g.bullets[0]={};g.bullets[0].active=1;g.bullets[0].x=160*FP;g.bullets[0].y=120*FP;
 g.emitPickup(160*FP,120*FP,PICKUP_XP,5);g.updateCosmicObjects();assert(g.player.vx>0&&!g.bullets[0].active);for(const Pickup &p:g.pickups)assert(!p.active);
 puts("Daily: ten playable variants, source draft/egg/reward constants, seven-mod Purity cap, 11-boss gauntlet, sword/ally behavior, real comet and cargo collisions PASS");
}
