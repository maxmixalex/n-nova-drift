// White-box acceptance checks for recovered rules and finite-pool adaptations.
// Fixtures are synthetic; these are not recorded natural playthroughs.
#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>
using namespace nova;
static Game g;
static void empty(unsigned seed=29){g.start(seed);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;}
static void ticks(unsigned n,const GameInput& input={}){while(n--){g.tick(input);assert(g.validState());}}
static int target(int x=180,int hp=10000){int n=g.spawnEnemy(E_ASTEROID,x,120);assert(n>=0);Enemy &e=g.enemies[n];e.phase=0;e.radius=5;e.vx=e.vy=0;e.hull=e.max_hull=hp;return n;}
static SourceSpawnSpec spec(SourceEnemy type){SourceSpawnSpec s;int32_t *p=&s.enemy;for(unsigned i=0;i<31;i++)p[i]=SOURCE_UNDEFINED;s.enemy=type;return s;}
static unsigned typeCount(unsigned type){unsigned n=0;for(const Enemy &e:g.enemies)n+=e.active&&e.type==type;return n;}

static unsigned friendlyCount(){unsigned n=0;for(const Bullet &b:g.bullets)n+=b.active&&!b.owner;return n;}
static void chargedAndMinefield(){
 // Native player C=1 multipliers are 2 damage, 1.2 velocity / blast, and
 // 1.275 physical size; constructs separately use 2.15/1.25/1.325. Only Split, Flak and Torrent gain extra projectiles.
 empty();g.player.mods[M_CHARGED_SHOT]=1;g.charged_scale_=1000000;g.fireWeapon();assert(friendlyCount()==2);
 for(const Bullet &b:g.bullets)if(b.active){assert(b.damage==50&&b.speed==1843&&b.radius==2&&b.size_percent==127);}
 const unsigned weapon[]={W_SPLIT_SHOT,W_FLAK,W_TORRENT,W_RAILGUN};const unsigned expected[]={6,14,14,1};
 for(unsigned k=0;k<4;k++){empty();g.equipWeapon(weapon[k]);g.player.mods[M_CHARGED_SHOT]=1;g.charged_scale_=1000000;g.fireWeapon();assert(friendlyCount()==expected[k]);}
 empty();g.equipWeapon(W_FLAK);g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_SINGULAR_STRIKE_NEW]=1;g.charged_scale_=2000000;g.fireWeapon();assert(friendlyCount()==1);
 // Charged Mines transfers Charged Shot away from ship fire, while a mine's
 // explicit construct-origin call receives the same recovered shot formulas.
 empty();g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_CHARGED_MINES]=1;g.charged_scale_=1000000;g.fireWeapon();for(const Bullet &b:g.bullets)if(b.active)assert(b.damage==25);
 empty();g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_CHARGED_MINES]=1;g.charged_scale_=1000000;g.fireWeapon(1,true);for(const Bullet &b:g.bullets)if(b.active)assert(b.damage==53);
 // Innately charging weapons accumulate the 20% bonus without rounding it
 // away on each 30 Hz tick. Mine transfer removes that ship-side bonus.
 GameInput fire={};fire.fire=true;empty();g.equipWeapon(W_VORTEX);g.player.mods[M_CHARGED_SHOT]=1;ticks(25,fire);assert(g.player.charge==25&&g.vortex_scale_>500000);
 empty();g.equipWeapon(W_THERMAL_LANCE);g.player.mods[M_CHARGED_SHOT]=1;ticks(25,fire);assert(g.player.charge==60);
 empty();g.equipWeapon(W_VORTEX);g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_CHARGED_MINES]=1;ticks(25,fire);assert(g.player.charge==25);
 // Extreme charge/speed remains valid even after inherited ship velocity.
 empty();g.equipWeapon(W_RAILGUN);g.player.mods[M_CHARGED_SHOT]=1;g.stats.velocity=1000;g.charged_scale_=20000000;g.player.vx=3*FP;g.fireWeapon();assert(g.validState());
 // A connected three-mine component includes the initiating mine. Every
 // blast gets 1+.04*3 once, independent of which array slot starts the chain.
 for(unsigned first=0;first<3;first++){
  empty();g.options.auto_deploy_mask=0;g.player.mods[M_MINES]=g.player.mods[M_MINEFIELD]=1;g.recalculate();
  for(unsigned i=0;i<4;i++){int n=g.spawnConstruct(C_MINE);assert(n==(int)i);g.constructs[n].x=(i<3?100+15*i:230)*FP;g.constructs[n].y=80*FP;g.constructs[n].vx=g.constructs[n].vy=0;}
  int victim=g.spawnEnemy(E_ASTEROID,115,80);g.enemies[victim].phase=0;g.enemies[victim].hull=g.enemies[victim].max_hull=10000;g.enemies[victim].vx=g.enemies[victim].vy=0;
  g.constructs[first].hull=0;g.updateConstructs();g.updateConstructs();assert(g.constructs[3].active&&g.constructCount()==1);
  for(unsigned i=0;i<3;i++)assert(g.constructs[i].chain_percent==112);
  assert(g.enemies[victim].hull==10000-672); // 3 * 250 * .8 Minefield * 1.12.
 }
 // Shield/Hull self costs use source ordering and retain sub-HP mitigation.
 empty();g.player.body=B_SENTINEL;g.player.mods[M_CORE_SHIELDING]=1;g.player.max_shield=g.player.shield=1000;
 for(unsigned i=0;i<100;i++)g.damageShieldSelf(1500);
 assert(g.player.shield==892&&g.discharge_self_fraction_==0&&g.run_stats.damage_taken==108);
 empty();g.player.mods[M_CORE_SHIELDING]=g.player.mods[M_FORCE_ARMOR]=1;
 for(unsigned i=0;i<10;i++)g.damageHullSelf(1000);
 assert(g.player.hull==93&&g.discharge_gemini_fraction_==200);
 empty();g.player.mods[M_BARRIER]=1;g.player.max_shield=g.player.shield=1000;g.damageShieldSelf(500000);assert(g.player.shield==975);
 empty();g.player.mods[M_X_MEDI_CHARGE]=1;int guard0=g.spawnConstruct(C_GUARDIAN),guard1=g.spawnConstruct(C_GUARDIAN);assert(guard0>=0&&guard1>=0);
 g.damageShieldSelf(10000);assert(g.player.shield==94&&g.constructs[guard0].hull==178&&g.constructs[guard1].hull==178);
 g.damageHullSelf(10000);assert(g.player.hull==94&&g.constructs[guard0].hull==176&&g.constructs[guard1].hull==176);
 empty();g.player.mods[M_X_MEDI_CHARGE]=1;guard0=g.spawnConstruct(C_GUARDIAN);
 for(unsigned i=0;i<10;i++)g.damageShieldSelf(250);
 assert(g.player.shield==99&&g.discharge_self_fraction_==500&&g.constructs[guard0].hull==179&&g.constructs[guard0].redirected_fraction==0);
 // A CRC-valid but corrupt line vector is rejected without abs(INT_MIN) UB.
 empty();int line=g.emitBullet(10*FP,10*FP,0,1,10,SHOT_DISCHARGE,5);g.bullets[line].vx=INT32_MIN;assert(!g.validState());
 // A new source class must explicitly opt into an AI handler; an obsolete
 // generic gun may not quietly stand in for a missing family implementation.
 for(unsigned type=0;type<ENEMY_COUNT;type++)if(type!=E_ASTEROID&&!enemyIsBoss(type)){empty();int n=g.spawnEnemy(type,60,60);assert(n>=0);assert(g.updateRegularEnemy(g.enemies[n]));}
}

static void rewardsAndSwarm(){
 empty();g.wave=17;int n=target(120,50);unsigned base=enemy_types[E_ASTEROID].xp*10+17;
 g.hitEnemy(n,100);assert(g.score==base&&g.kills==1);unsigned xp=0;for(const Pickup &p:g.pickups)if(p.active&&p.kind==PICKUP_XP)xp+=p.value;assert(xp==enemy_types[E_ASTEROID].xp);
 empty();g.wave=23;n=g.spawnEnemy(E_BULWARK,100,100);assert(n>=0);base=enemy_types[E_BULWARK].xp*10+23;
 g.awardEnemyPart(n,5,6,false);g.awardEnemyPart(n,1,6,true);assert(g.score==base&&g.kills==1);xp=0;for(const Pickup &p:g.pickups)if(p.active&&p.kind==PICKUP_XP)xp+=p.value;assert(xp==enemy_types[E_BULWARK].xp);
 // Green's native HP2 and .05 single/area modifier need fractional hits.
 empty();n=g.spawnEnemy(E_SWARMLING_GREEN,100,120);for(unsigned j=0;j<19;j++)g.hitEnemy(n,1);assert(g.enemies[n].hull==2);g.hitEnemy(n,1);assert(g.enemies[n].hull==1);for(unsigned j=0;j<20;j++)g.hitEnemy(n,1,false,true);assert(!g.enemies[n].active);
 for(unsigned green=0;green<2;green++)for(unsigned rare=0;rare<3;rare++){
  empty();n=g.spawnEnemy(green?E_SWARMLING_GREEN:E_SWARMLING,100,120,rare);Enemy &e=g.enemies[n];e.angle=0;
  g.player.x=110*FP;g.player.y=120*FP;g.player.hull=g.player.max_hull=1000;g.player.shield=0;g.player.invulnerable=0;
  for(unsigned t=0;t<(green?4u:3u);t++){e.x=100*FP;e.y=120*FP;e.vx=e.vy=0;g.updateRegularEnemy(e);}assert(e.action_count==1&&e.phase==2);
  // Isolate one exact15-source-frame beam for damage and recovery checks.
  e.action_timer=0;e.phase_timer=(uint16_t)(15+(green?10:0));e.fire_delay=(uint16_t)(green?25:20);e.heal_fraction=0;g.player.hull=1000;
  for(unsigned t=0;t<8;t++){e.x=100*FP;e.y=120*FP;e.vx=e.vy=0;g.updateRegularEnemy(e);}
  unsigned milli=(green?750:600)+125*rare;assert(g.player.hull==1000-(int)(milli*15/1000));assert(e.heal_fraction==milli*15%1000);
  if(green)assert(e.phase==2&&e.action_timer==16);else assert(e.phase==0);
 }
 // Explicit invulnerability blocks the source continuous beam.
 empty();n=g.spawnEnemy(E_SWARMLING,100,120);Enemy &e=g.enemies[n];e.form=1;e.phase=2;e.phase_timer=15;e.fire_delay=20;e.angle=0;g.player.x=110*FP;g.player.y=120*FP;unsigned shield=g.player.shield;for(unsigned t=0;t<8;t++){e.x=100*FP;e.vx=e.vy=0;g.updateRegularEnemy(e);}assert((unsigned)g.player.shield==shield);
}
static void pulsarSource(){
 for(unsigned rare=0;rare<3;rare++){
  empty();int n=g.spawnEnemy(E_PULSAR,100,120,rare);Enemy &e=g.enemies[n];e.angle=0;
  for(unsigned t=0;t<75;t++){g.updateRegularEnemy(e);}assert(e.phase==2);
  unsigned shots=0;for(unsigned t=0;t<200&&e.phase!=3;t++){g.updateRegularEnemy(e);for(Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_PULSAR_BARRAGE){++shots;assert(b.damage==0&&b.blast==0&&b.size_percent==1024);b.active=0;}}assert(shots==14+9*rare);
  unsigned pulses=0;for(unsigned t=0;t<200;t++){g.updateRegularEnemy(e);for(Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_PULSAR_PULSE){++pulses;assert(b.damage==(rare==2?24:16));b.active=0;}}
  assert(pulses==(rare?3u:0u));assert(typeCount(E_PULSAR_MINE)==1);
 }
 empty();int b=g.emitBullet(100*FP,100*FP,0,1,0,SHOT_PULSAR_BARRAGE,150,1,2);Bullet &shot=g.bullets[b];shot.damage=0;shot.speed=110;shot.vx=110;shot.vy=0;shot.size_percent=1024;shot.last_hit=737;shot.traveled=10;shot.homing=45;shot.tracking_seek=128;shot.field_scale=5000;
 g.player.x=160*FP;g.player.y=200*FP;g.updateBullets();g.updateBullets();assert(!shot.damage);g.updateBullets();assert(shot.damage==10&&shot.angle==0&&shot.speed>110);
 for(unsigned i=3;i<25;i++)g.updateBullets();
 assert(shot.tracking_scale_x==91&&shot.angle==0); // first half-degree source turn retains sub-angle precision.
 g.updateBullets();assert(shot.angle>0);
 // Pulsar pulses are forward lances: an equally close lateral player is safe.
 empty();b=g.emitBullet(100*FP,100*FP,0,0,16,SHOT_PULSAR_PULSE,15,1,3);g.bullets[b].size_percent=1843;g.bullets[b].vx=g.bullets[b].vy=0;g.player.x=100*FP;g.player.y=120*FP;g.player.invulnerable=0;g.player.shield=0;g.updateBullets();assert(g.player.hull==100);g.player.x=125*FP;g.player.y=100*FP;g.updateBullets();assert(g.player.hull==84);g.updateBullets();assert(g.player.hull==84);
}

static void vortexAndRancor(){
 // Original growth is a 60 Hz two-stage recurrence, not a charge timer.
 for(unsigned charged=0;charged<2;charged++){
  empty();g.equipWeapon(W_VORTEX);g.player.max_hull=g.player.hull=10000;g.player.firing=true;g.player.mods[M_CHARGED_SHOT]=(uint8_t)charged;
  double scale=.1,rate=.012375,cost=0;
  for(unsigned tick=0;tick<300;tick++){
   for(unsigned sub=0;sub<2;sub++){scale+=rate*(charged?1.1:1.0);rate+=(.00075-rate)/50;cost+=std::max(0.0,.0085*std::pow(1+scale,4.5)-.05);}
   g.updateVortexCharge();
  }
  assert(std::abs((double)g.vortex_scale_/1e6-scale)<.0001);
  double paid=10000-g.player.hull+(double)g.discharge_gemini_fraction_/1000;
  assert(std::abs(paid-cost)<.02);
  assert(g.validState());
 }
 // Cost is evaluated before Rancor's free scale: absorbed damage adds charge.
 empty();g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_RANCOR]=1;g.player.firing=true;
 g.charged_scale_=1000000;g.rancor_scale_=300000;g.recordRancorDamage(10000);assert(g.rancor_scale_==445833);
 g.charged_scale_=2000000;g.rancor_scale_=300000;g.recordRancorDamage(10000);assert(g.rancor_scale_==387500);
 g.player.firing=false;g.recordRancorDamage(100000);assert(g.rancor_scale_==387500);
 g.player.firing=true;g.rancor_scale_=500000;g.charged_scale_=500000;g.fireWeapon();
 for(const Bullet &b:g.bullets)if(b.active)assert(b.damage==50);
 // Explicit stat setup above has no recalculate, so base remains 25.
 empty();g.equipWeapon(W_VORTEX);g.player.mods[M_X_RANCOR]=1;g.player.firing=true;g.vortex_scale_=2000000;g.rancor_scale_=300000;g.recordRancorDamage(10000);assert(g.rancor_scale_==327500);
 g.updateRancor();assert(g.rancor_scale_>=325510&&g.rancor_scale_<=325513);
 // Thermal Rancor uses its actual native X scale, not a charge timer proxy.
 empty();g.equipWeapon(W_THERMAL_LANCE);GameInput thermal={};thermal.fire=true;g.tick(thermal);assert(g.thermal_x_scale_==132670&&g.thermal_y_scale_==119008);
 g.player.mods[M_X_RANCOR]=1;g.thermal_x_scale_=2000000;g.rancor_scale_=300000;g.recordRancorDamage(10000);assert(g.rancor_scale_==387500);
 // Native attraction uses direct distance, affected-solid resistance and the
 // released-field count; no wrap shortcut and no arbitrary 100px radius cap.
 empty();int e=target(100);g.enemies[e].hull=g.enemies[e].max_hull=500;g.enemies[e].type=E_SKIRMISHER;
 int32_t old=g.enemies[e].x;g.applyVortexGravity(old+100*FP/6,g.enemies[e].y,500000,1);assert(g.enemies[e].x-old>=89&&g.enemies[e].x-old<=90);
 old=g.enemies[e].x;g.applyVortexGravity(old+100*FP/6,g.enemies[e].y,1000000,2);assert(g.enemies[e].x-old>=179&&g.enemies[e].x-old<=180);
 g.player.mods[M_X_POLAR_INVERSION]=1;old=g.enemies[e].x;g.applyVortexGravity(old+100*FP/6,g.enemies[e].y,500000,1);assert(g.enemies[e].x<old);
 // Each released field retains its own native size/decay and burn accumulator.
 empty();e=target(160);g.enemies[e].y=120*FP;g.player.weapon=W_VORTEX;g.vortex_scale_=1000000;g.player.charge=1;g.releaseVortex();int b=-1;for(unsigned i=0;i<FRIENDLY_BULLETS;i++)if(g.bullets[i].active)b=(int)i;assert(b>=0);
 Bullet &v=g.bullets[b];v.x=g.enemies[e].x;v.y=g.enemies[e].y;v.vx=v.vy=0;
 int before=g.enemies[e].hull;g.updateVortexBullet(v,1);assert(v.field_scale==996004);assert(before-g.enemies[e].hull==29);assert(!g.enemies[e].burn_time&&g.enemies[e].wake_time);
 unsigned life=0;while(v.active&&life++<2000)g.updateVortexBullet(v,1);assert(!v.active&&life<400);
 // Sustained hull damage and mitigation feed the same Rancor accumulator,
 // including fractional self damage. Releasing consumes it exactly once.
 empty();g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_RANCOR]=1;g.player.firing=true;g.player.shield=0;g.player.invulnerable=0;g.charged_scale_=1000000;g.damagePlayer(12);assert(g.rancor_scale_==175000);
 g.damageHullSelf(250);assert(g.rancor_scale_==178645);g.fireWeapon();assert(!g.rancor_scale_);
}

int main(){
 // Source getRarity uses one random trial and a shared elite/champion threshold.
 empty();g.wave=100;for(unsigned i=0;i<1000;i++){uint32_t next=g.random_;next^=next<<13;next^=next>>17;next^=next<<5;unsigned roll=1+next%100;unsigned expected=roll*3<55?2:roll<55?1:0;assert(g.sourceEnemyRarity(SE_WARDEN)==expected);assert(g.random_==next);}
 // All recovered source patterns and all difficulty-dependent gates are usable.
 for(unsigned flags=0;flags<4;flags++)for(unsigned pattern=0;pattern<22;pattern++){
  empty(37+pattern);g.wave=240;g.run_flags=(uint16_t)((flags&1?RUN_ELITE:0)|(flags&2?RUN_ANNIHILATION|RUN_MAYHEM:0));
  SourceSpawnSpec out[SOURCE_MAX_SPECS];uint32_t rng=19+pattern;unsigned count=sourceCuratedPattern(pattern,g.wave,g.run_flags,rng,out,SOURCE_MAX_SPECS);assert(count&&count<=SOURCE_MAX_SPECS);g.loadWaveSpawns(out,count);ticks(180);assert(g.enemyCount()<=MAX_ENEMIES);
 }
 // A full pool must defer a pending formation, not discard its members.
 empty();SourceSpawnSpec s=spec(SE_INTERCEPTOR);s.count=20;s.rarity=0;g.loadWaveSpawns(&s,1);for(unsigned i=0;i<MAX_ENEMIES;i++)assert(g.spawnEnemy(E_ASTEROID,10+(int)(i%20)*14,12+(int)(i/20)*20)>=0);
 unsigned dropped=g.dropped_enemies;assert(g.pendingWaveSpawns()==20);ticks(5);assert(g.pendingWaveSpawns()==20&&g.dropped_enemies==dropped);
 for(unsigned i=0;i<MAX_ENEMIES;i++){g.enemies[i].active=0;}ticks(3);assert(!g.pendingWaveSpawns()&&typeCount(E_SKIRMISHER)==20);
 // Original follower repetitions are linear, not an exponential recursive tree.
 empty();s=spec(SE_INTERCEPTOR);s.count=2;s.rarity=0;s.follower=SE_WARDEN;s.follower_count=3;s.repeat_formation=2;s.behavior=4;s.support=SE_CHERUB;s.support_count=1;s.repeating=2;s.repeat_frequency_ticks=4;
 g.loadWaveSpawns(&s,1);assert(g.pendingWaveSpawns()==48);ticks(40);assert(!g.pendingWaveSpawns());assert(typeCount(E_SKIRMISHER)==6&&typeCount(E_WARDEN)==36&&typeCount(E_CHERUB)==6);
 // Hosts create the actual native drone type and cannot bind to a recycled slot.
 empty();int host=g.spawnEnemy(E_HOST,40,40);ticks(10);assert(typeCount(E_HOST_DRONE)==8);uint16_t generation=g.enemies[host].generation;g.hitEnemy(host,100000);assert(!g.enemies[host].active);int reused=g.spawnEnemy(E_ASTEROID,40,40);assert(reused==host&&g.enemies[host].generation!=generation);ticks(2);for(const Enemy &e:g.enemies)if(e.active&&e.type==E_HOST_DRONE)assert(e.leader<0);
 empty();g.spawnEnemy(E_HIVE,40,40,1);ticks(80);assert(typeCount(E_HIVE_DRONE_HOT)>=3);
 empty();g.spawnEnemy(E_CONSTRUCTOR,40,40);ticks(200);assert(typeCount(E_CONSTRUCTOR_DRONE)==5&&typeCount(E_CONSTRUCTOR_TURRET)>=1);
 // Hammerhead's attack is a charge, without the former generic gun volley.
 empty();int hammer=g.spawnEnemy(E_FRIGATE,40,120);ticks(100);assert(g.enemies[hammer].phase==2);for(const Bullet &b:g.bullets)assert(!b.active||!b.owner);
 empty();int gyro=g.spawnEnemy(E_GYROGUN,40,40);ticks(170);assert(g.enemies[gyro].phase==1);ticks(50);assert(g.enemies[gyro].phase==0);
 // Crucible closes screen wrapping after its visible entry grace period.
 empty();int crucible=g.spawnEnemy(E_CRUCIBLE,160,50);g.enemies[crucible].age=89;assert(g.screenWrappingEnabled());g.enemies[crucible].age=90;assert(!g.screenWrappingEnabled());g.player.x=(WIDTH-g.player.radius-1)*FP;g.player.y=200*FP;g.player.vx=3*FP;ticks(1);assert(g.player.vx<0&&g.player.x<(WIDTH-g.player.radius)*FP);
 g.player.mods[M_BLINK]=1;g.player.ability_delay=0;g.player.angle=0;g.player.vx=0;g.player.x=300*FP;GameInput blink={};blink.ability=true;ticks(1,blink);assert(g.player.x>290*FP&&g.player.x<WIDTH*FP);g.enemies[crucible].active=0;for(BossPart &p:g.boss_parts)p.active=0;assert(g.screenWrappingEnabled());
 // Assist subtracts 15 percentage points from final incoming modifiers.
 empty();g.player.mods[M_AEGIS]=1;g.aegis_clock_=75;unsigned before=g.incomingDamagePercent();g.options.assist_mode=1;assert(g.incomingDamagePercent()+15==before);g.player.shield=0;g.player.invulnerable=0;int hp=g.player.hull;g.damagePlayer(50);assert(hp-g.player.hull==35);
 // Reserve capacity must not change max-hull-derived calculations.
 empty();g.player.mods[M_X_HYPERMETABOLISM]=1;g.recalculate();assert(g.player.max_hull==100&&g.hullRecoveryCap()==85);g.player.mods[M_X_ATARAXIA]=1;g.banked_upgrades=5;g.recalculate();assert(g.player.max_hull==105&&g.hullRecoveryCap()==89&&g.bodyPowerPercent()==130);
 // High ROF is accumulated below one simulation tick, without a 30Hz cap.
 empty();g.stats.fire_rate=600;GameInput fire={};fire.fire=true;ticks(30,fire);assert(g.run_stats.shots_fired>=70&&g.run_stats.shots_fired<=74);empty();g.stats.fire_rate=6000;g.player.angle=0;g.player.x=20*FP;ticks(30,fire);assert(g.run_stats.shots_fired>=710&&g.run_stats.shots_fired<=730);assert(g.run_stats.coalesced_shots>0&&g.dropped_bullets==0);unsigned packet_damage=0;for(const Bullet &b:g.bullets)if(b.active&&!b.owner)packet_damage+=(unsigned)b.damage;assert(packet_damage==g.run_stats.shots_fired*25);
 empty();g.stats.fire_rate=600;g.player.mods[M_X_SINGULAR_STRIKE_NEW]=1;assert(g.weaponRateMilli()>6500&&g.weaponRateMilli()<6600);
 empty();g.equipWeapon(W_SALVO);assert(!g.player.missiles);ticks(15);assert(g.player.missiles==4);ticks(45);assert(g.player.missiles==16);ticks(30,fire);assert(g.run_stats.shots_fired==16&&g.player.missiles==0);
 // Dart is not ignite: the native two-second lodged burn scales with size and
 // gains 1% per simultaneously lodged dart. Initial hit adds 50% area damage.
 empty();int victim=target();int dart=g.emitBullet(170*FP,120*FP,0,10,45,SHOT_DART,30);ticks(1);assert(g.bullets[dart].active&&g.bullets[dart].pierce==254&&g.bullets[dart].life==60);assert(!g.enemies[victim].burn_time);ticks(60);assert(!g.bullets[dart].active&&g.enemies[victim].hull==10000-157);
 empty();victim=target();g.emitBullet(170*FP,120*FP,0,10,45,SHOT_DART,30);g.emitBullet(170*FP,120*FP,0,10,45,SHOT_DART,30);ticks(61);assert(g.enemies[victim].hull==10000-316);
 empty();victim=target();g.stats.size=200;dart=g.emitBullet(170*FP,120*FP,0,10,45,SHOT_DART,30);ticks(1);assert(g.bullets[dart].life==120);ticks(120);assert(g.enemies[victim].hull==10000-430);
 empty();int killed=target(180,30);victim=target(200);dart=g.emitBullet(170*FP,120*FP,0,10,45,SHOT_DART,40);ticks(1);assert(!g.enemies[killed].active&&g.bullets[dart].active&&g.bullets[dart].pierce==255);ticks(2);assert(g.bullets[dart].last_hit==victim&&g.bullets[dart].pierce==254);g.hitEnemy(victim,100000);assert(!g.bullets[dart].active);
 // Asteroids explicitly do not receive Blitz's fresh-target bonus.
 empty();victim=target();g.player.mods[M_BLITZ]=1;g.hitEnemy(victim,100);assert(g.enemies[victim].hull==9900);
 chargedAndMinefield();vortexAndRancor();rewardsAndSwarm();pulsarSource();
 std::printf("PASS native rules: 88 source recipes, queued formations, native summons, arena/Assist, high ROF, reserves, Dart, charged formulas and connected Minefield; Vortex/Rancor, Swarm/Pulsar, rewards; Game=%zu bytes\n",sizeof(Game));
}
