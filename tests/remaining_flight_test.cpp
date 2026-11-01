// Synthetic source-event fixtures; no proprietary resource bytes are embedded.
#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace nova;
static Game g;
static unsigned checks;
#define CHECK(x) do{++checks;assert(x);}while(0)
static void reset(){g.start(73);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=160*FP;g.player.y=180*FP;}
static int spawn(unsigned type,unsigned rarity=0){int n=g.spawnEnemy(type,100,100,rarity);CHECK(n>=0);return n;}
static void ai(int n,unsigned count){while(count--){++g.enemies[n].age;CHECK(g.updateRegularEnemy(g.enemies[n]));CHECK(g.validState());}}
static unsigned shots(unsigned kind){unsigned n=0;for(const Bullet &b:g.bullets)n+=b.active&&b.kind==kind;return n;}
static int linked(unsigned type,int mother){int n=spawn(type);g.enemies[n].leader=(int8_t)mother;g.enemies[n].leader_generation=g.enemies[mother].generation;return n;}
static void host_bursts(){
 reset();int m=spawn(E_HOST,1),n=linked(E_HOST_DRONE_2,m);ai(n,183);CHECK(!shots(SHOT_HOST_DRONE));ai(n,1);CHECK(shots(SHOT_HOST_DRONE)==1);
 ai(n,30);CHECK(shots(SHOT_HOST_DRONE)==6);ai(n,185);CHECK(shots(SHOT_HOST_DRONE)==6);ai(n,1);CHECK(shots(SHOT_HOST_DRONE)==7);
 for(const Bullet &b:g.bullets)if(b.active){CHECK(b.damage==8);CHECK(b.speed==4*FP);CHECK(b.size_percent==1352);}
 int dx=g.enemies[n].x-g.enemies[m].x,dy=g.enemies[n].y-g.enemies[m].y;
 CHECK(dx*dx+dy*dy>11*11*FP*FP&&dx*dx+dy*dy<14*14*FP*FP);
 g.enemies[m].generation++;ai(n,1);CHECK(!g.enemies[n].active);
 reset();n=spawn(E_HOST_DRONE_2);ai(n,1);CHECK(!g.enemies[n].active);
 // The eight elite escorts share their mother's orbit speed and appear in one event.
 reset();m=spawn(E_HOST,1);ai(m,1);unsigned count=0;int speed=0;
 for(const Enemy &e:g.enemies)if(e.active&&e.type==E_HOST_DRONE_2){CHECK(e.leader==m&&e.leader_generation==g.enemies[m].generation);if(count)CHECK(e.target_y==speed);speed=e.target_y;++count;}
 CHECK(count==8);CHECK((speed>=228&&speed<=319)||(speed<=-228&&speed>=-319));
 for(unsigned j=0;j<MAX_ENEMIES;++j)if(g.enemies[j].active&&g.enemies[j].leader==m)g.destroyEnemy(j);
 for(unsigned tick=0;tick<100&&g.enemies[m].active;++tick)ai(m,1);
 CHECK(!g.enemies[m].active);
}
static void zone_gates(){
 const int margin=225*FP/6;
 for(unsigned orientation=1;orientation<=4;++orientation)for(int side=-1;side<=1;++side)for(unsigned cloak=0;cloak<2;++cloak){
  reset();int m=spawn(E_SERAPH),n=linked(E_SERAPH_ZONE_DRONE,m);Enemy &e=g.enemies[n];e.formation=(uint8_t)orientation;e.angle=32;
  bool lower=orientation==1||orientation==3;int edge=lower?margin:(orientation==2?g.worldHeight():g.worldWidth())*FP-margin;
  if(orientation<=2)g.player.y=edge+side;else g.player.x=edge+side;
  if(cloak){g.player.body=B_SPECTRE;g.player.cloak=200;}
  bool expected=!cloak&&(lower?side<0:side>0);
  ai(n,59);CHECK(!shots(SHOT_SERAPH_ZONE));ai(n,1);CHECK(shots(SHOT_SERAPH_ZONE)==unsigned(expected));
  ai(n,14);CHECK(shots(SHOT_SERAPH_ZONE)==unsigned(expected));ai(n,1);CHECK(shots(SHOT_SERAPH_ZONE)==2u*unsigned(expected));
  for(const Bullet &b:g.bullets)if(b.active){CHECK(b.angle==32);CHECK(b.damage==15);CHECK(b.speed==20*FP);CHECK(b.field_scale==1620000&&b.last_hit==1229);}
  g.enemies[m].generation++;ai(n,1);CHECK(!e.active);
 }
 // The original child depends on the exact mother's lifetime, not a reused slot.
 reset();int n=spawn(E_SERAPH_ZONE_DRONE);ai(n,1);CHECK(!g.enemies[n].active);
}
static void source_projectile_curve(){
 reset();int n=g.emitBullet(40*FP,40*FP,0,0,15,SHOT_SERAPH_ZONE,180,1,2);CHECK(n>=0);Bullet &b=g.bullets[n];b.field_scale=1620000;b.size_percent=6635;b.last_hit=1229;b.vx=b.vy=0;
 for(unsigned tick=1;tick<=100;++tick){g.updateBullets();unsigned source_age=tick*2;unsigned expected=1620000+7500*source_age-75*source_age*(source_age-1)/2;CHECK(b.active);CHECK(b.field_scale==expected);CHECK(b.size_percent==expected*uint64_t(4096)/1000000);CHECK(b.last_hit==1229);}
}
static void source_spawn_pairs(){
 reset();int m=spawn(E_SERAPH);Enemy &e=g.enemies[m];e.age=1000;e.form=1;e.phase=0;e.phase_age=45;e.phase_timer=1000;e.action_count=45;
 g.updateBoss(e);unsigned count=0;static const uint8_t orientation[8]={1,1,3,3,4,4,2,2},angle[8]={0,128,64,192,64,192,0,128};
 for(const Enemy &q:g.enemies)if(q.active&&q.type==E_SERAPH_ZONE_DRONE){unsigned j=q.formation_slot;CHECK(j<8);CHECK(q.formation==orientation[j]);CHECK(q.angle==angle[j]);CHECK(q.leader==m);++count;}
 CHECK(count==8);CHECK(g.validState());
}
int main(){host_bursts();zone_gates();source_projectile_curve();source_spawn_pairs();std::printf("PASS remaining flight: %u checks, source Host bursts/orbits/lifecycle and Seraph edge gates/fixed crossfire/curve\n",checks);}
