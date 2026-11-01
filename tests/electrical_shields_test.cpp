#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
using namespace nova;
namespace {
void setup(Game &g){g.start(72819);g.configureCareer(64,true);g.configureRun(RUN_ENDLESS);g.wave=120;g.next_wave=65000;g.player.invulnerable=65000;}
unsigned nodes(const Game &g){unsigned n=0;for(unsigned i=0;i<FRIENDLY_BULLETS;++i)n+=g.bullets[i].active&&g.bullets[i].kind==SHOT_DISCHARGE;return n;}
void discharge(){
 Game g={};setup(g);g.player.mods[M_DISCHARGE]=1;g.player.mods[M_X_INFUSE]=1;g.player.shield=100;g.triggerDischarge();assert(nodes(g)==1&&g.discharge_clock_==6);
 const Bullet &b=g.bullets[0];assert(b.damage==6118);assert(b.size_percent==1114);assert(b.bounces>=8&&b.bounces<=10);assert(g.discharge_self_fraction_==500&&g.player.shield==99);
 assert(b.vx||b.vy);g.triggerDischarge();assert(nodes(g)==1&&g.discharge_self_fraction_==500);
 g.discharge_clock_=0;g.triggerDischarge();assert(nodes(g)==2&&g.player.shield==97&&g.discharge_self_fraction_==0);
 Game slow={};setup(slow);slow.equipWeapon(W_RAILGUN);slow.player.mods[M_DISCHARGE]=1;slow.player.mods[M_X_INFUSE]=1;slow.player.shield=100;slow.triggerDischarge();assert(nodes(slow)==1);assert(slow.bullets[0].damage>b.damage);assert(slow.discharge_self_fraction_+1000u*(100-slow.player.shield)>1500);
 Game gemini={};setup(gemini);gemini.player.mods[M_DISCHARGE]=1;gemini.player.mods[M_X_INFUSE]=1;gemini.player.mods[M_X_GEMINI]=1;gemini.player.shield=100;gemini.player.hull=100;
 gemini.triggerDischarge();assert(gemini.player.hull==100&&gemini.discharge_gemini_fraction_==750);gemini.discharge_clock_=0;gemini.triggerDischarge();assert(gemini.player.hull==99&&gemini.player.shield==97&&gemini.discharge_gemini_fraction_==500);
 Game down={};setup(down);down.player.mods[M_DISCHARGE]=1;down.player.shield=0;down.triggerDischarge();assert(nodes(down)==0);
 bool one=false,two=false;
 for(unsigned seed=1;seed<=40;++seed){Game chance={};chance.start(seed);chance.player.mods[M_DISCHARGE]=1;chance.player.shield=100;chance.triggerDischarge();unsigned count=nodes(chance);assert(count==1||count==2);one|=count==1;two|=count==2;
  for(unsigned i=0;i<FRIENDLY_BULLETS;++i)if(chance.bullets[i].active){const Bullet &node=chance.bullets[i];assert(node.damage==6118);assert(count==1?(node.bounces>=8&&node.bounces<=10):(node.bounces>=5&&node.bounces<=6));}
 }assert(one&&two);
}
void persistent_burn(){
 Game g={};setup(g);int id=g.spawnEnemy(E_ASTEROID,90,100),outside=g.spawnEnemy(E_ASTEROID,90,125);assert(id>=0&&outside>=0);
 g.enemies[id].hull=g.enemies[id].max_hull=g.enemies[outside].hull=g.enemies[outside].max_hull=1000;g.enemies[id].radius=g.enemies[outside].radius=1;
 int node=g.emitElectricalNode(80*FP,100*FP,0,20*FP,10*256,0,256);assert(node>=0);
 for(unsigned t=0;t<7;++t){++g.frame;g.updateBullets();assert(g.enemies[id].hull==1000-int(t+1)*5);assert(g.enemies[outside].hull==1000);}
 assert(!g.bullets[node].active&&g.validState());
 // Low fractional burn accumulates; it is not rounded up once per tick.
 int before=g.enemies[id].hull;g.emitElectricalNode(80*FP,100*FP,0,20*FP,256,0,256);
 for(unsigned t=0;t<7;++t){++g.frame;g.updateBullets();}assert(before-g.enemies[id].hull==3);
}
void propagation(){
 Game g={};setup(g);int root=g.emitElectricalNode(160*FP,120*FP,0,16*FP,256,8,768);assert(root>=0&&nodes(g)==1);
 ++g.frame;g.updateBullets();assert(nodes(g)>=2);
 ++g.frame;g.updateBullets();assert(nodes(g)>=3);
 for(unsigned t=0;t<16;++t){++g.frame;g.updateBullets();assert(g.validState());}
 assert(nodes(g)==0);
}
void tempest(){
 bool two=false,three=false;
 for(unsigned seed=1;seed<=30;++seed){Game g={};setup(g);g.start(seed);g.triggerTempest(160*FP,120*FP,100);unsigned n=nodes(g);assert(n==2||n==3);two|=n==2;three|=n==3;int angle=-1;
  for(unsigned i=0;i<FRIENDLY_BULLETS;++i){const Bullet &b=g.bullets[i];if(!b.active)continue;assert(b.damage==95334&&b.size_percent==1719&&b.bounces>=12&&b.bounces<=24);if(angle<0)angle=b.angle;else assert(angle==b.angle);}
  assert(g.validState());
 }assert(two&&three);
}
}
int main(){discharge();persistent_burn();propagation();tempest();std::puts("electrical shields: source damage/ROF cost, internal cooldown, persistent fractional burn, timed chain growth and randomized Tempest roots PASS");}
