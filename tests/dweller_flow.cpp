#include <cassert>
#include <cstdio>
#include <cstring>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void clean(){g.start(583);g.configureCareer(64,true);g.next_wave=60000;g.wave=80;g.player.invulnerable=60000;g.player.x=240*FP;g.player.y=190*FP;}
static unsigned shots(unsigned kind){unsigned n=0;for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;++i)n+=g.bullets[i].active&&g.bullets[i].kind==kind;return n;}
static void clearShots(){for(unsigned i=0;i<MAX_BULLETS;++i)g.bullets[i].active=0;}
static void ai(unsigned index,unsigned ticks=1){while(ticks--){++g.frame;++g.enemies[index].age;assert(g.updateRegularEnemy(g.enemies[index]));assert(g.validState());}}
static void chargeTiming(){
 const unsigned types[]={E_DWELLER_GUARD,E_DWELLER_SOLDIER};
 for(unsigned type:types){
  clean();int n=g.spawnEnemy(type,80,80);assert(n>=0);unsigned first=type==E_DWELLER_GUARD?93:208;
  ai(n,first-1);assert(shots(SHOT_DWELLER_GUARD)==0);assert(g.enemies[n].phase==1);
  ai(n);assert(shots(SHOT_DWELLER_GUARD)==1);assert(g.enemies[n].phase==0);clearShots();
  unsigned cycle=type==E_DWELLER_GUARD?212:332;ai(n,cycle-1);assert(!shots(SHOT_DWELLER_GUARD));ai(n);assert(shots(SHOT_DWELLER_GUARD)==1);
 }
}
static void warriorPattern(){
 clean();int n=g.spawnEnemy(E_DWELLER_WARRIOR,80,80);assert(n>=0);
 ai(n,149);assert(!shots(SHOT_DWELLER_ARC));ai(n);assert(shots(SHOT_DWELLER_ARC)==1);
 ai(n,75);assert(shots(SHOT_DWELLER_ARC)==2);ai(n,75);assert(shots(SHOT_DWELLER_ARC)==3);
 unsigned phaseMask=1u<<2,missiles=0;
 for(unsigned t=0;t<700;++t){ai(n);phaseMask|=1u<<g.enemies[n].phase;for(unsigned i=0;i<MAX_ENEMIES;++i)if(g.enemies[i].active&&g.enemies[i].type==E_DWELLER_CHITIN){++missiles;assert(g.enemies[i].hull==400);g.enemies[i].active=0;}}
 assert(phaseMask==15&&missiles>=2);assert(g.validState());
 // At full pool a firing phase remains bounded and advances normally.
 for(unsigned i=0;i<MAX_ENEMIES;++i)if(!g.enemies[i].active){int q=g.spawnEnemy(E_WASP,20,20);assert(q>=0);}
 Enemy &e=g.enemies[n];e.phase=0;e.fire_delay=0;e.action_timer=1;e.action_count=0;e.phase_timer=300;ai(n);assert(e.action_count==2&&g.enemyCount()==MAX_ENEMIES);
}
static void projectiles(){
 clean();g.player.invulnerable=0;g.player.shield=0;g.player.hull=g.player.max_hull=1000;g.player.x=100*FP;g.player.y=100*FP;
 int n=g.emitBullet(g.player.x,g.player.y,0,1,65,SHOT_DWELLER_ARC,300,1,2);assert(n>=0);
 for(unsigned t=0;t<30;++t){Bullet &b=g.bullets[n];b.x=g.player.x;b.y=g.player.y;b.vx=b.vy=0;++g.frame;g.updateBullets();assert(b.active&&g.validState());}
 assert(g.player.hull==935); // True 65 DPS, not a one-hit bullet or 7-tick i-frame loss.
 g.player.invulnerable=100;int before=g.player.hull;for(unsigned t=0;t<10;++t){g.bullets[n].x=g.player.x;g.bullets[n].y=g.player.y;g.updateBullets();}assert(g.player.hull==before);
 unsigned size=g.bullets[n].size_percent;for(unsigned t=0;t<35;++t){g.bullets[n].x=100*FP;g.bullets[n].y=100*FP;g.updateBullets();}assert(g.bullets[n].size_percent>=size);
 for(unsigned t=0;t<300&&g.bullets[n].active;++t){g.bullets[n].x=100*FP;g.bullets[n].y=100*FP;g.updateBullets();}assert(!g.bullets[n].active);
 clean();n=g.emitBullet(80*FP,80*FP,0,5,14,SHOT_DWELLER_GUARD,150,1,3);g.bullets[n].speed=16*FP/3;
 unsigned speed=g.bullets[n].speed;for(unsigned t=0;t<20;++t){g.bullets[n].x=80*FP;g.bullets[n].y=80*FP;g.updateBullets();}assert(g.bullets[n].active&&g.bullets[n].speed<speed&&g.bullets[n].size_percent<100);
 for(unsigned t=0;t<150&&g.bullets[n].active;++t){g.bullets[n].x=80*FP;g.bullets[n].y=80*FP;g.updateBullets();}assert(!g.bullets[n].active);
}
static void droneAndMissile(){
 clean();g.player.x=100*FP;g.player.y=100*FP;g.player.invulnerable=0;int n=g.spawnEnemy(E_DWELLER_DRONE,100,100);assert(n>=0);g.hitEnemy(n,2000000);assert(!g.enemies[n].active&&g.player.shield==100);
 clean();g.player.x=100*FP;g.player.y=100*FP;g.player.invulnerable=0;n=g.spawnEnemy(E_DWELLER_DRONE,100,100);ai(n,21);assert(g.enemies[n].active&&g.player.shield==100);ai(n);assert(!g.enemies[n].active&&g.player.shield==82);
 clean();n=g.spawnEnemy(E_DWELLER_DRONE,80,80);int hp=g.enemies[n].hull;g.hitEnemy(n,100);assert(g.enemies[n].hull==hp-5);
 clean();n=g.spawnEnemy(E_DWELLER_CHITIN,80,80);assert(g.enemies[n].hull==400);g.player.x=80*FP;g.player.y=80*FP;g.player.invulnerable=0;
 int b=g.emitBullet(80*FP,80*FP,0,1,500,SHOT_RAIL,10,0,3);assert(b>=0);g.buildGrid();g.updateBullets();assert(!g.enemies[n].active&&g.player.shield==92);
}
static void parentDeath(){
 clean();int boss=g.spawnEnemy(E_DWELLER,240,100);assert(boss>=0);Enemy &p=g.enemies[boss];p.phase_timer=2000;
 const unsigned types[]={E_DWELLER_DRONE,E_DWELLER_SOLDIER,E_DWELLER_GUARD,E_DWELLER_WARRIOR};
 unsigned thirds=0;int before=p.hull;
 for(unsigned type:types){int child=g.spawnEnemy(type,40,40);assert(child>=0);g.enemies[child].leader=(int8_t)boss;g.enemies[child].leader_generation=p.generation;g.enemies[child].hull=0;g.destroyEnemy(child);thirds+=type==E_DWELLER_DRONE?2100:type==E_DWELLER_SOLDIER?875:type==E_DWELLER_GUARD?8750:70000;assert(p.hull==before-int(thirds/3));}
 assert(p.phase_timer<2000&&p.phase_timer>=30);
 int child=g.spawnEnemy(E_DWELLER_WARRIOR,40,40);g.enemies[child].leader=(int8_t)boss;g.enemies[child].leader_generation=p.generation+1;before=p.hull;g.enemies[child].hull=0;g.destroyEnemy(child);assert(p.hull==before);
 child=g.spawnEnemy(E_DWELLER_WARRIOR,40,40);g.enemies[child].leader=(int8_t)boss;g.enemies[child].leader_generation=p.generation;p.hull=10;g.enemies[child].hull=0;g.destroyEnemy(child);assert(!p.active&&g.validState());
}
int main(){chargeTiming();warriorPattern();projectiles();droneAndMissile();parentDeath();puts("Dweller source charge, phase, continuous AoE, damageable missile, explosion and parent-death tests passed");}
