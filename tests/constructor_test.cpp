#include <assert.h>
#include <stdio.h>
#include <string.h>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void clean(){g.start(181);g.configureCareer(64,true);g.next_wave=60000;g.player.x=10*FP;g.player.y=10*FP;g.player.invulnerable=60000;}
static void step(unsigned slot,unsigned ticks=1){while(ticks--){++g.frame;++g.enemies[slot].age;g.updateRegularEnemy(g.enemies[slot]);}}
static unsigned missiles(){unsigned count=0;for(unsigned i=0;i<MAX_ENEMIES;i++)if(g.enemies[i].active&&g.enemies[i].type==E_CONSTRUCTOR_MISSILE){++count;g.enemies[i].active=0;}return count;}
static void healing(){
 clean();int owner=g.spawnEnemy(E_CONSTRUCTOR,100,100),drone=g.spawnEnemy(E_CONSTRUCTOR_DRONE,100,100),near=g.spawnEnemy(E_WASP,125,100),far=g.spawnEnemy(E_WASP,180,100),other=g.spawnEnemy(E_CONSTRUCTOR_DRONE,100,100);
 assert(owner>=0&&drone>=0&&near>=0&&far>=0&&other>=0);Enemy &d=g.enemies[drone];d.leader=(int8_t)owner;d.leader_generation=g.enemies[owner].generation;d.formation=0;
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(g.enemies[i].active)g.enemies[i].hull=1;
 step(drone,22);assert(g.enemies[owner].hull==1&&g.enemies[near].hull==1);step(drone);assert(g.enemies[owner].hull==26&&g.enemies[near].hull==26&&g.enemies[far].hull==1&&g.enemies[other].hull==1);
 step(drone,22);assert(g.enemies[owner].hull==51);assert(g.enemies[near].hull<=g.enemies[near].max_hull);assert(!g.bulletCount());
 ++g.enemies[owner].generation;step(drone);assert(!g.enemies[drone].active);
}
static void cycle(){
 clean();int t=g.spawnEnemy(E_CONSTRUCTOR_TURRET,150,100);assert(t>=0);step(t,119);assert(!missiles());step(t);assert(missiles()==1);
 unsigned count=1;for(unsigned tick=0;tick<89;++tick){step(t);count+=missiles();}assert(count==30);step(t);assert(!g.enemies[t].phase&&!missiles());
 for(unsigned tick=0;tick<119;++tick){step(t);assert(!missiles());}step(t);assert(g.enemies[t].phase&&missiles()==1);
}
static void tether(){
 clean();int leader=g.spawnEnemy(E_CONSTRUCTOR,100,100),t=g.spawnEnemy(E_CONSTRUCTOR_TURRET,170,100);assert(leader>=0&&t>=0);Enemy &e=g.enemies[t];e.leader=(int8_t)leader;e.leader_generation=g.enemies[leader].generation;
 step(t,239);missiles();g.player.x=135*FP;g.player.y=100*FP;g.player.invulnerable=0;g.player.shield=0;g.player.hull=g.player.max_hull=5000;
 step(t);int start=g.player.hull;for(unsigned n=0;n<150;++n){step(t);missiles();}assert(g.player.hull<start&&g.player.hull>start-200);assert(e.target_x==150000);
 int before=g.player.hull;step(t,30);assert(before-g.player.hull>=35&&before-g.player.hull<=37);
 ++g.enemies[leader].generation;before=g.player.hull;step(t,30);assert(g.player.hull==before);
}
static void destructibleMissile(){
 clean();int m=g.spawnEnemy(E_CONSTRUCTOR_MISSILE,200,100);assert(m>=0&&g.enemies[m].max_hull==20);assert(g.enemySeen(E_CONSTRUCTOR_MISSILE));g.enemies[m].angle=0;int32_t x=g.enemies[m].x;step(m);assert(g.enemies[m].x-x==8*FP/3);
 g.hitEnemy((unsigned)m,5);assert(g.enemies[m].hull==15);g.player.x=g.enemies[m].x;g.player.y=g.enemies[m].y;g.player.invulnerable=0;g.player.shield=0;int hp=g.player.hull;g.hitEnemy((unsigned)m,15);assert(!g.enemies[m].active&&g.player.hull<hp);assert(g.validState());
 Career c;c.clear();g.state=GameOver;c.award(g);assert(c.enemySeen(E_CONSTRUCTOR_MISSILE)&&c.valid());
}
int main(){healing();cycle();tether();destructibleMissile();puts("constructor-flow: 45-frame all-nearby healing, 180/240 firing cycle, physical growing tether, shootable 20HP missile and high discovery word PASS");}
