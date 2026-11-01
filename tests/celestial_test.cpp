#include <assert.h>
#include <stdio.h>
#include <string.h>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void clean(){g.start(6401);g.configureCareer(64,true);g.next_wave=60000;g.player.x=10*FP;g.player.y=10*FP;g.player.invulnerable=60000;}
static void phases(){
 for(unsigned rarity=0;rarity<3;++rarity){clean();int n=g.spawnEnemy(E_CELESTIAL,160,120,rarity);assert(n>=0);g.enemies[n].elite=(uint8_t)rarity;unsigned central=0,mini=0,warnings=0,phases=0;int first=-1,last=-1;
  for(unsigned tick=0;tick<800;++tick){Enemy &e=g.enemies[n];++e.age;++g.frame;g.updateRegularEnemy(e);phases|=1u<<e.phase;
   for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;++j){const Bullet &b=g.bullets[j];if(!b.active)continue;
    if(b.kind==SHOT_CELESTIAL_STRIKE){++central;if(first<0)first=(int)tick;last=(int)tick;assert(b.damage==(int)(50*e.damage_scale/256)&&b.speed==14*FP/3);}
    if(b.kind==SHOT_CELESTIAL_STRIKE_MINI){++mini;assert(b.damage==(int)(20*e.damage_scale/256)&&b.speed==7*FP/3);}
    if(b.kind==SHOT_CELESTIAL_WARNING)++warnings;
    g.bullets[j].active=0;
   }
   if(tick>400&&e.phase==0&&central)break;
  }
  printf("rarity%u phases%u center%u mini%u warnings%u first%d last%d\n",rarity,phases,central,mini,warnings,first,last);fflush(stdout);assert(phases==7&&central==4+rarity&&mini==2*central&&warnings>3);assert(first>=250&&last>first);assert(g.validState());
 }
}
static void orbit(){
 clean();int a=g.emitBullet(100*FP,100*FP,0,1,20,SHOT_CELESTIAL_STRIKE_MINI,100,1,2),b=g.emitBullet(100*FP,100*FP,0,1,20,SHOT_CELESTIAL_STRIKE_MINI,100,1,2);assert(a>=0&&b>=0);
 for(unsigned k=0;k<2;++k){Bullet &p=g.bullets[k?a:b];p.speed=7*FP/3;p.field_scale=5*65536;p.tracking_scale_x=350;p.size_percent=5;p.homing=k?2:1;}
 int32_t base=100*FP;for(unsigned tick=0;tick<10;++tick){++g.frame;g.updateBullets();assert(g.bullets[a].active&&g.bullets[b].active);}
 assert(g.bullets[a].y<base&&g.bullets[b].y>base);assert(g.bullets[a].x>base&&g.bullets[b].x>base);assert(g.bullets[a].size_percent==35);assert(g.bullets[a].field_scale<5*65536);assert(g.validState());
}
static void warning(){
 clean();int n=g.emitBullet(100*FP,100*FP,0,0,80,SHOT_CELESTIAL_WARNING,90,1,1);Bullet &b=g.bullets[n];b.vx=b.vy=0;b.field_scale=65536;b.tracking_scale_x=40;b.speed=0;
 g.player.x=b.x;g.player.y=b.y;g.player.invulnerable=0;g.player.shield=0;g.player.hull=g.player.max_hull=1000;
 for(unsigned tick=0;tick<74;++tick){g.updateBullets();assert(g.player.hull==1000&&b.active&&!b.pierce);}
 g.updateBullets();assert(b.pierce&&g.player.hull==920);for(unsigned tick=0;tick<5;++tick)g.updateBullets();assert(g.player.hull==920);g.updateBullets();assert(!b.active);
}
static void impact(){
 clean();g.player.x=150*FP;g.player.y=100*FP;g.player.invulnerable=0;g.player.shield=0;g.player.hull=g.player.max_hull=1000;
 int n=g.emitBullet(g.player.x-6*FP,g.player.y,0,1,50,SHOT_CELESTIAL_STRIKE,100,1,5);Bullet &b=g.bullets[n];b.speed=14*FP/3;b.last_hit=30;b.size_percent=58;b.tracking_scale_x=575;
 g.updateBullets();assert(g.player.hull==950);g.updateBullets();assert(g.player.hull==920);
}
int main(){phases();orbit();warning();impact();puts("Celestial: three phases, rarity4/5/6 charged triples, counter-orbit minis, 150-frame safe warnings and separate direct/blast damage PASS");}
