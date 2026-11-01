#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace nova;
static Game g;
static void clean(unsigned seed=8501){
 g.start(seed);g.configureCareer(64,true);g.next_wave=60000;
 g.player.x=20*FP;g.player.y=20*FP;g.player.shield=0;g.player.invulnerable=60000;
}
static unsigned count(unsigned kind){unsigned n=0;for(unsigned i=0;i<MAX_BULLETS;++i)n+=g.bullets[i].active&&g.bullets[i].kind==kind;return n;}
static int bomb(unsigned rarity=0,int owner=-1){
 int at=g.emitBullet(140*FP,100*FP,0,1,20+5*(int)rarity,SHOT_MYRMIDON_BOMB,300,1,4);assert(at>=0);
 Bullet &b=g.bullets[at];b.field_scale=(uint32_t)b.damage;b.size_percent=2048;b.speed=FP;
 b.tracking_scale_x=(uint16_t)(1229+410*rarity);b.tracking_scale_y=0;b.tracking_seek=256;
 b.tracking_target=(int16_t)owner;b.last_hit=owner>=0?(int16_t)g.enemies[owner].generation:-1;
 b.traveled=0;b.construct_source=(uint8_t)rarity;b.pierce=b.blast=b.homing=0;
 return at;
}
static void flight(){
 clean();int owner=g.spawnEnemy(E_MYRMIDON,50,50);assert(owner>=0);int at=bomb(2,owner);Bullet &b=g.bullets[at];
 g.player.x=b.x;g.player.y=200*FP;unsigned initialSpeed=b.speed;
 for(unsigned tick=0;tick<14;++tick){g.updateBullets();assert(b.active&&b.traveled==0);b.x=140*FP;b.y=100*FP;}
 assert(b.tracking_scale_y==28&&b.speed>initialSpeed);
 g.updateBullets();assert(b.tracking_scale_y==30&&b.traveled>0&&b.tracking_scale_x>=2048&&b.tracking_scale_x<2400);
 // The mother is live at53 source frames: no degeneration is allowed merely
 // because the projectile has aged. Deleting/reusing its slot must trigger it.
 b.x=140*FP;b.y=100*FP;b.tracking_scale_y=51;b.tracking_scale_x=3277;
 g.updateBullets();assert(b.tracking_scale_y==53&&!b.pierce);
 ++g.enemies[owner].generation;b.x=140*FP;b.y=100*FP;g.updateBullets();assert(b.pierce);
 unsigned turn=b.tracking_scale_x;b.x=140*FP;b.y=100*FP;g.updateBullets();assert(b.tracking_scale_x<turn&&b.active);assert(g.validState());
 clean();at=bomb();Bullet &deadMother=g.bullets[at];deadMother.tracking_scale_y=50;
 g.updateBullets();assert(!deadMother.pierce&&deadMother.tracking_scale_y==52);
 g.updateBullets();assert(deadMother.pierce&&deadMother.tracking_scale_y==54);
 std::puts("Myrmidon source30 turning, multiplicative acceleration and source53 generation-aware degeneration PASS");
}
static void warnings(){
 clean();g.player.x=160*FP;g.player.y=120*FP;g.player.invulnerable=0;g.player.hull=g.player.max_hull=1000;
 int c=g.spawnConstruct(C_TURRET);assert(c>=0);g.constructs[c].x=g.player.x+7*FP;g.constructs[c].hull=g.constructs[c].max_hull=1000;
 int field=g.spawnMyrmidonField(g.player.x,g.player.y,2867,49,30,true);assert(field>=0);Bullet &b=g.bullets[field];
 unsigned firstSize=b.size_percent;for(unsigned tick=0;tick<14;++tick){g.updateBullets();assert(!b.pierce&&g.player.hull==1000&&g.constructs[c].hull==1000);}
 assert(b.size_percent>firstSize&&b.size_percent<b.field_scale);
 g.updateBullets();assert(b.pierce&&g.player.hull==951&&g.constructs[c].hull==951);
 unsigned radius=b.radius;g.updateBullets();assert(b.radius<radius&&g.player.hull==951&&g.constructs[c].hull==951);
 for(unsigned tick=0;tick<6;++tick)g.updateBullets();
 assert(!b.active&&g.player.hull==951&&g.constructs[c].hull==951);
 // Hit lists track each target independently: entering after the first burst
 // step is still a hit, while remaining in the field cannot repeat damage.
 clean();g.player.x=20*FP;g.player.y=20*FP;g.player.invulnerable=0;g.player.hull=g.player.max_hull=1000;
 field=g.spawnMyrmidonField(160*FP,120*FP,4096,70,0,true);assert(field>=0);g.updateBullets();assert(g.player.hull==1000);
 g.player.x=160*FP;g.player.y=120*FP;g.updateBullets();assert(g.player.hull==930);
 for(unsigned tick=0;tick<7;++tick){g.updateBullets();}
 assert(g.player.hull==930);
 std::puts("Myrmidon safe easing warning, shrinking12-frame burst and per-target one-hit behavior PASS");
}
static void chainedFields(){
 for(unsigned rarity=0;rarity<3;++rarity){
  uint64_t radiusSquaredSum=0;unsigned samples=0;
  for(unsigned seed=1;seed<=96;++seed){
   clean(seed*17+rarity);int at=bomb(rarity);Bullet dead=g.bullets[at];g.bullets[at].active=0;
   g.detonateMyrmidon(dead);assert(count(SHOT_MYRMIDON_BLAST)==1&&count(SHOT_MYRMIDON_SPAWNER)==1);
   int spawner=-1;for(unsigned i=0;i<MAX_BULLETS;++i)if(g.bullets[i].active){if(g.bullets[i].kind==SHOT_MYRMIDON_BLAST){assert(g.bullets[i].damage==14&&g.bullets[i].field_scale==2457);g.bullets[i].active=0;}else if(g.bullets[i].kind==SHOT_MYRMIDON_SPAWNER)spawner=(int)i;}
   assert(spawner>=0);unsigned charges=g.bullets[spawner].homing;
   assert(rarity==0?charges==5:rarity==1?(charges>=9&&charges<=10):(charges>=12&&charges<=15));
   unsigned produced=0;int32_t radius=g.bullets[spawner].traveled;
   for(unsigned tick=0;tick<40&&g.bullets[spawner].active;++tick){
    g.updateBullets();
    for(unsigned i=0;i<MAX_BULLETS;++i){Bullet &f=g.bullets[i];if(!f.active||f.kind!=SHOT_MYRMIDON_BLAST||!f.tracking_scale_y)continue;
     int64_t dx=(int64_t)f.x-dead.x,dy=(int64_t)f.y-dead.y,sq=dx*dx+dy*dy;
     assert(sq<=(int64_t)(radius+FP)*(radius+FP));
     assert(f.tracking_scale_y>=25&&f.tracking_scale_y<=42&&f.field_scale>=819&&f.field_scale<=1840);
     assert(f.damage==(int)(70*f.field_scale/4096)&&!f.pierce);
     radiusSquaredSum+=(uint64_t)(sq*100000/((int64_t)radius*radius));++samples;++produced;f.active=0;
    }
   }
   assert(!g.bullets[spawner].active&&produced==charges);assert(g.validState());
  }
  // A uniform area distribution has E[r^2/R^2]=1/2. A uniform radius
  // accidentally substituted for sqrt(random) has1/3 and fails this bound.
  unsigned mean=(unsigned)(radiusSquaredSum/samples);assert(mean>42000&&mean<57000);
  std::printf("Myrmidon rarity%u original charge range, radius/delay/damage and uniform-area chain (%u samples, moment%u/100000) PASS\n",rarity,samples,mean);
 }
 // Partial projectile health scales the spawned area and chain damage rather
 // than silently retaining the original full-health explosion.
 clean();int at=bomb(2);Bullet dead=g.bullets[at];g.bullets[at].active=0;dead.damage=15;g.detonateMyrmidon(dead);
 bool initial=false,chain=false;for(unsigned i=0;i<MAX_BULLETS;++i)if(g.bullets[i].active){const Bullet &b=g.bullets[i];if(b.kind==SHOT_MYRMIDON_BLAST){assert(b.field_scale==1228&&b.damage==14);initial=true;}if(b.kind==SHOT_MYRMIDON_SPAWNER){assert(b.damage==35&&b.traveled==145*FP/12);chain=true;}}
 assert(initial&&chain);
}
static void boundariesAndReflection(){
 clean();int at=bomb();g.bullets[at].x=g.worldWidth()*FP-1;g.updateBullets();assert(!g.bullets[at].active&&count(SHOT_MYRMIDON_BLAST)==0&&count(SHOT_MYRMIDON_SPAWNER)==0);
 clean();at=bomb();Bullet dead=g.bullets[at];g.bullets[at].active=0;dead.x=-1;g.detonateMyrmidon(dead);assert(!g.bulletCount());
 clean();g.player.x=150*FP;g.player.y=100*FP;g.player.shield_type=S_REFLECT;g.player.shield=g.player.max_shield=100;g.recalculate();
 int target=g.spawnEnemy(E_WASP,185,100);assert(target>=0);g.enemies[target].hull=g.enemies[target].max_hull=10000;
 at=bomb();g.updateBullets();assert(g.bullets[at].active&&!g.bullets[at].owner&&g.bullets[at].homing);
 int hull=g.enemies[target].hull;
 for(unsigned tick=0;tick<40&&g.enemies[target].hull==hull;++tick)g.updateBullets();
 assert(g.enemies[target].hull<hull&&g.player.shield==100);assert(g.validState());
 std::puts("Myrmidon edge suppression and real Reflect ownership/target damage PASS");
}
static void impactOrder(){
 clean();g.player.x=150*FP;g.player.y=100*FP;g.player.invulnerable=0;g.player.hull=g.player.max_hull=1000;
 int c=g.spawnConstruct(C_TURRET);assert(c>=0);g.constructs[c].x=g.player.x;g.constructs[c].y=g.player.y+15*FP;g.constructs[c].hull=g.constructs[c].max_hull=1000;
 int at=g.emitBullet(g.player.x-6*FP,g.player.y,0,1,50,SHOT_CELESTIAL_STRIKE,100,1,5);assert(at>=0);
 Bullet &b=g.bullets[at];b.speed=14*FP/3;b.last_hit=30;b.size_percent=58;b.tracking_scale_x=575;
 g.updateBullets();assert(g.player.hull==950&&g.constructs[c].hull==1000);
 g.updateBullets();assert(g.player.hull==920&&g.constructs[c].hull==970);
 for(unsigned tick=0;tick<6;++tick){g.updateBullets();}
 assert(g.player.hull==920&&g.constructs[c].hull==970);
 std::puts("Shared collision: Celestial direct50 precedes the independent blast30/player and nearby construct PASS");
 clean();g.player.x=150*FP;g.player.y=100*FP;g.player.invulnerable=0;g.player.hull=g.player.max_hull=1000;
 c=g.spawnConstruct(C_TURRET);assert(c>=0);g.constructs[c].x=g.player.x;g.constructs[c].y=g.player.y+15*FP;g.constructs[c].hull=g.constructs[c].max_hull=1000;
 bomb();g.updateBullets();assert(g.player.hull==980&&g.constructs[c].hull==1000);
 g.updateBullets();assert(g.player.hull==966&&g.constructs[c].hull==986);
 std::puts("Myrmidon direct20 and independent initial burst14/nearby construct PASS");
}
static void support(){
 for(unsigned rarity=0;rarity<3;++rarity){
  clean(990+rarity);int owner=g.spawnEnemy(E_MYRMIDON,160,120,rarity),near=g.spawnEnemy(E_WASP,180,120),far=g.spawnEnemy(E_WASP,220,120),nonstandard=g.spawnEnemy(E_ASTEROID,180,120);
  assert(owner>=0&&near>=0&&far>=0&&nonstandard>=0);Enemy &e=g.enemies[owner];
  e.hull=1;for(int n:{near,far,nonstandard}){g.enemies[n].hull=1;g.enemies[n].max_hull=1000;}
  for(unsigned tick=0;tick<30;++tick){e.x=160*FP;e.y=120*FP;++e.age;++g.frame;g.updateRegularEnemy(e);}
  assert(e.hull==1&&g.enemies[far].hull==1&&g.enemies[nonstandard].hull==1);
  assert(g.enemies[near].hull==(rarity==0?1:rarity==1?16:21));assert(g.enemies[near].aura_fraction==0);
  assert(!count(SHOT_MYRMIDON_TURRET));
 }
 for(unsigned formation=3;formation<=4;++formation){
  clean();g.player.x=50*FP;g.player.y=100*FP;int leader=g.spawnEnemy(E_WASP,100,100),owner=g.spawnEnemy(E_MYRMIDON,130,150,2);assert(leader>=0&&owner>=0);
  Enemy &e=g.enemies[owner];e.leader=(int8_t)leader;e.leader_generation=g.enemies[leader].generation;e.formation=(uint8_t)formation;g.enemies[leader].angle=0;
  for(unsigned tick=0;tick<90;++tick){e.x=130*FP;e.y=150*FP;++e.age;++g.frame;g.updateRegularEnemy(e);assert(!count(SHOT_MYRMIDON_TURRET));}
  assert((e.action_count&64u)&&e.action_timer==0);
  int64_t previousSide=0;
  for(unsigned shot=0;shot<2;++shot){e.x=130*FP;e.y=150*FP;e.action_timer=0;e.phase_timer=1000;++e.age;++g.frame;g.updateRegularEnemy(e);
   int at=-1;for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;++i)if(g.bullets[i].active&&g.bullets[i].kind==SHOT_MYRMIDON_TURRET)at=(int)i;
   assert(at>=0&&e.phase_timer==1005);const Bullet &b=g.bullets[at];assert(b.damage==12&&b.speed==19*FP/3&&b.size_percent==3072);
   int64_t side=(int64_t)(b.x-e.x)*dirY(b.angle)-(int64_t)(b.y-e.y)*dirX(b.angle);assert(side!=0);if(previousSide)assert((side>0)!=(previousSide>0));previousSide=side;
   std::memset(g.bullets,0,sizeof(g.bullets));
  }
  // The parent-facing rear-half gate is distinct from the follower's own aim.
  g.enemies[leader].angle=128;e.action_timer=0;g.updateRegularEnemy(e);assert(!count(SHOT_MYRMIDON_TURRET));
  g.enemies[leader].angle=0;++g.enemies[leader].generation;e.action_timer=0;g.updateRegularEnemy(e);assert(!count(SHOT_MYRMIDON_TURRET));
 }
 std::puts("Myrmidon1.5/2percent aura, self/excluded/outside suppression, valid3/4 formation rear turret and alternate muzzles PASS");
}
static void motherCycle(){
 for(unsigned rarity=0;rarity<3;++rarity){clean(901+rarity);int owner=g.spawnEnemy(E_MYRMIDON,160,120,rarity);assert(owner>=0);Enemy &e=g.enemies[owner];unsigned bombs=0;int first=-1,previous=-1;
  for(unsigned tick=0;tick<700;++tick){++g.frame;++e.age;g.updateRegularEnemy(e);
   for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;++i){const Bullet &b=g.bullets[i];if(!b.active)continue;
    if(b.kind==SHOT_MYRMIDON_BOMB){assert(b.damage==(int)((20+5*rarity)*e.damage_scale/256)&&b.size_percent==2048);
     assert(b.life>=(190+30*rarity)/2&&b.life<=(250+30*rarity)/2&&b.tracking_target==owner&&b.last_hit==(int16_t)e.generation);
     if(first<0)first=(int)tick;else assert((int)tick-previous>=252&&(int)tick-previous<=253);previous=(int)tick;++bombs;
    }g.bullets[i].active=0;
   }
  }
  assert(first==122&&bombs==3);assert(g.validState());std::printf("Myrmidon rarity%u original205+40-source first windup and465+40 repeat (%u bombs) PASS\n",rarity,bombs);
 }
}
int main(){flight();warnings();chainedFields();boundariesAndReflection();impactOrder();motherCycle();support();std::printf("Myrmidon independent mechanics and shared collision-order regression PASS (Game%zu)\n",sizeof(Game));}
