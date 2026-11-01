#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
using namespace nova;
namespace {
void setup(Game &g){g.start(42719);g.configureCareer(64,true);g.configureRun(RUN_ENDLESS);g.wave=120;g.next_wave=65000;g.player.invulnerable=65000;}
unsigned wakes(const Game &g){unsigned n=0;for(unsigned i=0;i<FRIENDLY_BULLETS;++i)n+=g.bullets[i].active&&g.bullets[i].kind==SHOT_WAKE;return n;}
void recursive_emission(){
 for(unsigned rank=1;rank<=5;++rank){Game g={};setup(g);g.player.mods[M_X_SCORCHING_WAKE]=(uint8_t)rank;g.player.thrusting=true;
  g.frame=1;g.triggerScorchingWake();assert(wakes(g)==0);
  g.frame=3;g.triggerScorchingWake();assert(wakes(g)==rank);
  for(unsigned i=0;i<FRIENDLY_BULLETS;++i)if(g.bullets[i].active){const Bullet &b=g.bullets[i];assert(b.damage==180&&b.last_hit>=512&&b.last_hit<2048);assert(b.traveled==b.last_hit&&b.size_percent==b.traveled/10);assert(b.speed>=7168&&b.speed<19456);}
  g.frame=6;g.triggerScorchingWake();assert(wakes(g)==rank*2&&g.validState());
  g.player.thrusting=false;g.frame=9;g.triggerScorchingWake();assert(wakes(g)==rank*2);
 }
}
void stationary(Bullet &b){b.x=160*FP;b.y=100*FP;b.angle=0;b.speed=0;b.vx=b.vy=0;b.last_hit=2048;b.traveled=2048;b.size_percent=204;b.homing=0;}
void contact_and_overlap(){
 Game g={};setup(g);g.player.mods[M_X_SCORCHING_WAKE]=2;g.player.thrusting=true;g.frame=3;g.triggerScorchingWake();assert(wakes(g)==2);
 int target=g.spawnEnemy(E_ASTEROID,150,100);assert(target>=0);Enemy &e=g.enemies[target];e.hull=e.max_hull=10000;e.radius=16;
 for(unsigned i=0;i<FRIENDLY_BULLETS;++i)if(g.bullets[i].active)stationary(g.bullets[i]);
 for(unsigned t=0;t<30;++t){++g.frame;g.updateBullets();assert(e.hull==10000-int(t+1)*12&&g.validState());}
 assert(e.wake_time&&!e.burn_time); // contact burn marker, no invented ignite.
 int after=e.hull;for(Bullet &b:g.bullets)if(b.kind==SHOT_WAKE)b.active=0;
 for(unsigned t=0;t<20;++t){++g.frame;g.updateEnemies();}assert(e.hull==after&&e.wake_time);
}
void part_contact_and_ignition(){
 Game g={};setup(g);g.player.mods[M_X_SCORCHING_WAKE]=1;g.player.thrusting=true;g.frame=3;g.triggerScorchingWake();
 for(Bullet &b:g.bullets)if(b.active)stationary(b);
 int id=g.spawnEnemy(E_WORM,150,100,2);assert(id>=0);int at=-1;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].active&&g.boss_parts[i].owner==id){if(at<0)at=(int)i;else g.boss_parts[i].active=0;}
 assert(at>=0);BossPart &p=g.boss_parts[at];p.x=150*FP;p.y=100*FP;p.burn_time=17;p.burn_damage=4;int before=p.hull;
 ++g.frame;g.updateBullets();assert(p.hull==before-6&&p.wake_time&&p.burn_time==17&&p.burn_damage==4&&g.validState());
}
void size_lifecycle(){
 Game g={};setup(g);g.player.mods[M_X_SCORCHING_WAKE]=1;g.player.thrusting=true;g.frame=3;g.triggerScorchingWake();stationary(g.bullets[0]);unsigned initial=g.bullets[0].size_percent;
 ++g.frame;g.updateBullets();assert(g.bullets[0].size_percent>initial);
 for(unsigned t=0;t<149;++t){++g.frame;g.updateBullets();assert(g.validState());}
 assert(wakes(g)==0);
}
}
int main(){recursive_emission();contact_and_overlap();part_contact_and_ignition();size_lifecycle();std::puts("scorching wake: source six-frame recursive emission, 180DPS per overlapping field, live-part burn, independent ignition state and growth/decay lifecycle PASS");}
