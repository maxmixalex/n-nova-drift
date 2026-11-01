// Quantitative source-event scenarios for the formerly shared family cases.
#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace nova;
static Game g;
static void reset(){g.start(734);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=220*FP;g.player.y=120*FP;}
static int spawn(unsigned type,unsigned rarity=0){int n=g.spawnEnemy(type,100,120,rarity);assert(n>=0);return n;}
static void ai(int n,unsigned ticks){while(ticks--){assert(g.enemies[n].active);++g.enemies[n].age;assert(g.updateRegularEnemy(g.enemies[n]));}}
static unsigned shots(unsigned kind){unsigned n=0;for(const Bullet &b:g.bullets)if(b.active&&b.kind==kind)++n;return n;}
static void bullets(unsigned ticks){while(ticks--){++g.frame;g.updateBullets();}}
static void bind(int child,int parent){Enemy &e=g.enemies[child];e.leader=(int8_t)parent;e.leader_generation=g.enemies[parent].generation;e.formation=0;}
int main(){
 reset();int n=spawn(E_HIVE_DRONE);g.enemies[n].vx=0;g.enemies[n].vy=0;ai(n,1);assert(g.enemies[n].target_x==717);assert(!shots(SHOT_ENEMY));
 reset();n=spawn(E_HIVE_DRONE_HOT);g.enemies[n].vx=4*FP;g.enemies[n].vy=0;ai(n,1);assert(g.enemies[n].target_x==2980);g.destroyEnemy(n);assert(shots(SHOT_SPIKE_MINI)==3);for(const Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_SPIKE_MINI){assert(b.damage==12&&b.speed==3*FP&&b.size_percent==2867);}bullets(30);assert(!shots(SHOT_SPIKE_MINI));
 // Native Shredder arms on approach and ramps .25 ->6 by .05/source-frame.
 for(unsigned rarity=0;rarity<3;++rarity){reset();n=spawn(E_SHREDDER_MINE,rarity);g.player.x=120*FP;g.enemies[n].vx=g.enemies[n].vy=0;ai(n,57);assert(g.enemies[n].active&&g.enemies[n].phase&&g.enemies[n].phase_age==114);ai(n,1);assert(!g.enemies[n].active);assert(shots(rarity==2?SHOT_SPIKE_TRIPLE:SHOT_SPIKE)==(rarity==2?9u:6u));for(const Bullet &b:g.bullets)if(b.active)assert(b.damage==20);}
 // Champion spikes have a real25-source-frame split, including shot-down destruction.
 reset();int triple=g.emitBullet(100*FP,120*FP,0,0,20,SHOT_SPIKE_TRIPLE,75,1,2);assert(triple>=0);g.bullets[triple].size_percent=2458;g.bullets[triple].vx=g.bullets[triple].vy=0;bullets(12);assert(shots(SHOT_SPIKE_TRIPLE)==1);bullets(1);assert(!shots(SHOT_SPIKE_TRIPLE)&&shots(SHOT_SPIKE)==3);for(const Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_SPIKE)assert(b.damage==11&&b.speed==6*FP);
 reset();triple=g.emitBullet(100*FP,120*FP,0,0,20,SHOT_SPIKE_TRIPLE,75,1,2);g.destroyHostileBullet((unsigned)triple);assert(shots(SHOT_SPIKE)==3);
 // Triangle mine survives until its original120-frame fuse and then creates a
 // finite48-projectile sequencer. It is not an eight-shot proximity grenade.
 reset();n=spawn(E_PULSAR_MINE);g.enemies[n].vx=g.enemies[n].vy=0;ai(n,59);assert(g.enemies[n].active);ai(n,1);assert(!g.enemies[n].active&&shots(SHOT_TRIANGLE_SPAWNER)==1);bullets(21);assert(!shots(SHOT_TRIANGLE_SPAWNER)&&shots(SHOT_PULSAR_MINE)==48);bullets(5);for(const Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_PULSAR_MINE)assert(b.damage==10&&b.speed>=154);
 reset();n=spawn(E_PULSAR_MINE);g.destroyEnemy(n);assert(shots(SHOT_TRIANGLE_SPAWNER)==1);
 // Host3:300-frame initial wait,150-frame charge, source.15/frame beam.
 reset();int parent=spawn(E_HOST);n=spawn(E_HOST_DRONE_3);bind(n,parent);ai(n,149);assert(g.enemies[n].phase==0);ai(n,1);assert(g.enemies[n].phase==1);ai(n,74);assert(g.enemies[n].phase==1);ai(n,1);assert(g.enemies[n].phase==2);
 g.player.invulnerable=0;g.normal_hit_grace_=0;g.player.shield=g.player.max_shield=0;g.player.hull=g.player.max_hull=1000;g.enemies[n].phase_timer=55;g.enemies[n].angle=0;g.enemies[n].target_x=0;g.enemies[n].heal_fraction=0;ai(n,30);assert(g.player.hull==991);
 g.enemies[parent].active=0;ai(n,1);assert(!g.enemies[n].active);
 // Celestial drone follows its actual mother and fires18 damage at18source
 // speed in phase1 only; the shot retains independent axis degeneration.
 reset();parent=spawn(E_CELESTIAL);g.enemies[parent].phase=0;g.enemies[parent].angle=0;n=spawn(E_CELESTIAL_DRONE);bind(n,parent);g.enemies[n].formation_count=2;ai(n,37);assert(!shots(SHOT_CELESTIAL_DRONE));ai(n,1);assert(shots(SHOT_CELESTIAL_DRONE)==1);for(const Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_CELESTIAL_DRONE)assert(b.damage==18&&b.speed==6*FP&&b.angle==0&&b.size_percent==2662);
 g.enemies[parent].phase=1;ai(n,80);assert(shots(SHOT_CELESTIAL_DRONE)==1);bullets(40);assert(!shots(SHOT_CELESTIAL_DRONE));g.enemies[parent].active=0;ai(n,1);assert(!g.enemies[n].active);
 assert(g.validState());std::puts("PASS minor enemies: Hive/Hot, source Shredder warning/fans,48-shot Triangle fuse, Host continuous beam and Celestial orbit/weapon lifecycle");
}
