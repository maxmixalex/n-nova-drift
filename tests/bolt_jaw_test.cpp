#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
using namespace nova;
static void setup(Game& g){g.start(811);g.configureCareer(64,true);g.configureRun(RUN_ENDLESS);g.next_wave=65000;g.player.invulnerable=65000;}
static unsigned parts(const Game& g,int owner){unsigned count=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)count+=g.boss_parts[i].active&&g.boss_parts[i].owner==owner;return count;}
static int jaw(const Game& g,int owner,unsigned slot){for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].active&&g.boss_parts[i].owner==owner&&g.boss_parts[i].slot==slot)return int(i);return -1;}
int main(){
 for(unsigned rarity=0;rarity<3;++rarity){
  Game g={};setup(g);int owner=g.spawnEnemy(E_BOLT_THROWER,140,110,rarity);assert(owner>=0);Enemy &e=g.enemies[owner];e.angle=0;
  assert(parts(g,owner)==0);g.tick({});assert(parts(g,owner)==(rarity?2u:0u));assert(g.validState());
  if(!rarity)continue;
  int left=jaw(g,owner,0),right=jaw(g,owner,1);assert(left>=0&&right>=0);assert(g.boss_parts[left].max_hull==e.max_hull);
  int before=e.hull;g.hitBossPart(left,1);assert(e.hull==before&&g.boss_parts[left].hull==g.boss_parts[left].max_hull-1);
  assert(g.sweptBossPart(g.boss_parts[left],g.boss_parts[left].x-30*FP,g.boss_parts[left].y,g.boss_parts[left].x+30*FP,g.boss_parts[left].y,0));
  e.age=rarity==2?40:130;for(unsigned j=0;j<15;++j)g.updateBossParts(owner);
  assert(g.boss_parts[left].angle!=e.angle&&g.boss_parts[right].angle!=e.angle);
  e.age=150;for(unsigned j=0;j<60;++j)g.updateBossParts(owner);assert(g.boss_parts[left].timer<20);
  g.hitBossPart(right,100000);assert(!e.active&&parts(g,owner)==0);assert(g.validState());
 }
 // A shortage never creates only half a pair; retry after the space is free.
 Game g={};setup(g);int owner=g.spawnEnemy(E_BOLT_THROWER,140,110,1);assert(owner>=0);g.enemies[owner].age=1;
 for(unsigned i=0;i<MAX_BOSS_PARTS-1;++i){g.boss_parts[i].max_hull=1;g.boss_parts[i].owner=63;}
 g.updateBossParts(owner);assert(parts(g,owner)==0);g.boss_parts[0].max_hull=0;g.updateBossParts(owner);assert(parts(g,owner)==2);
 std::puts("Bolt jaws: variants, delayed assembly, independent hull, parent death, mouth motion and pool retry PASS");
}
