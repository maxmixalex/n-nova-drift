#include "game.h"
#include <cassert>
#include <cstdio>
using namespace nova;
namespace {
void setup(Game &g,unsigned type,unsigned wave=120){
 g.start(72193+type);g.configureCareer(64,true);g.configureRun(RUN_ENDLESS);
 g.next_wave=65000;g.wave=(uint16_t)wave;g.player.mods[M_X_APOTHEOSIS]=1;g.recalculate();
 int boss=g.spawnEnemy(type,160,65);assert(boss>=0);g.boss_index=(int16_t)boss;
 g.enemies[boss].age=1000;g.player.invulnerable=65000;
}
unsigned hostiles(const Game &g){unsigned n=0;for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;++j)n+=g.bullets[j].active;return n;}
void quiet(Game &g){for(unsigned i=0;i<MAX_BULLETS;++i)g.bullets[i].active=0;for(unsigned i=0;i<MAX_ENEMIES;++i)if(int(i)!=g.boss_index)g.enemies[i].active=0;}
void cycle(unsigned type,unsigned phases){
 Game g={};setup(g,type);unsigned seen=0,hidden=0,returned=0,maxshots=0;
 bool was_hidden=false;
 for(unsigned t=0;t<7000;++t){
  g.player.invulnerable=65000;g.next_wave=65000;g.state=Playing;
  g.tick({});assert(g.validState());const Enemy &e=g.enemies[g.boss_index];assert(e.active);
  seen|=1u<<e.phase;maxshots=hostiles(g)>maxshots?hostiles(g):maxshots;
  if(e.boss_hidden)++hidden;
  if(was_hidden&&!e.boss_hidden)++returned;
  was_hidden=e.boss_hidden;
  for(unsigned i=0;i<MAX_ENEMIES;++i)if(int(i)!=g.boss_index)g.enemies[i].active=0;
 }
 assert(seen==((1u<<phases)-1));assert(maxshots>0);
 if(type==E_DREADNOUGHT||type==E_DWELLER||type==E_STAR_EATER)assert(hidden&&returned);
 std::printf("boss %u: phases=%u, hostile peak=%u, hidden=%u, returns=%u\n",type,phases,maxshots,hidden,returned);
}
void disarm(unsigned type,unsigned kind,unsigned phase){
 Game g={};setup(g,type);g.tick({});quiet(g);
 Enemy &e=g.enemies[g.boss_index];e.form=1;e.phase=(uint8_t)phase;e.phase_age=220;e.phase_timer=1000;e.fire_delay=0;e.action_count=0;e.boss_hidden=0;
 g.tick({});assert(hostiles(g)>0);quiet(g);
 unsigned removed=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].owner==g.boss_index&&g.boss_parts[i].kind==kind){g.boss_parts[i].active=0;g.boss_parts[i].hull=0;++removed;}
 if(type==E_DREADNOUGHT)for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].owner==g.boss_index&&g.boss_parts[i].kind==BP_DREAD_GUN_SMALL){g.boss_parts[i].active=0;g.boss_parts[i].hull=0;++removed;}
 assert(removed);e.fire_delay=0;e.phase_age=220;g.tick({});assert(hostiles(g)==0);assert(g.validState());
}
void part_damage(){
 Game g={};setup(g,E_DREADNOUGHT,40);g.tick({});quiet(g);
 int chosen=-1;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].kind==BP_DREAD_GUN_LARGE&&g.boss_parts[i].active){chosen=(int)i;break;}
 assert(chosen>=0);BossPart &part=g.boss_parts[chosen];Enemy &boss=g.enemies[g.boss_index];
 // Isolate a gun from the hull and its neighbouring wing, then hit it using
 // the same public bullet simulation that receives actual player shots.
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(int(i)!=chosen)g.boss_parts[i].active=0;
 part.x=40*FP;part.y=40*FP;part.local_x=-120*FP;part.local_y=-25*FP;
 boss.vx=boss.vy=0;boss.phase=0;boss.phase_timer=1000;boss.phase_age=200;boss.fire_delay=500;
 // The attachment moves with its owner, so take its position after a tick.
 g.tick({});int before=part.hull,owner_before=boss.hull;uint32_t recorded=g.run_stats.damage_dealt;
 Bullet &b=g.bullets[0];b={};b.active=1;b.owner=0;b.kind=SHOT_RAIL;b.x=part.x;b.y=part.y;b.damage=100;b.radius=2;b.life=10;b.last_hit=-1;
 g.tick({});assert(part.hull<before);assert(owner_before-boss.hull==before-part.hull);
 assert(g.run_stats.damage_dealt-recorded==unsigned(before-part.hull));
 // A detached gun stays detached across following ticks and cannot respawn.
 part.active=0;part.hull=0;for(unsigned t=0;t<30;++t)g.tick({});assert(!part.active);
}
void crucible_drones(){
 Game g={};setup(g,E_CRUCIBLE,80);g.tick({});Enemy &e=g.enemies[g.boss_index];unsigned live=0;
 for(unsigned i=0;i<MAX_ENEMIES;++i)if(g.enemies[i].active&&g.enemies[i].type==E_CRUCIBLE_DRONE){const Enemy &q=g.enemies[i];assert(q.leader==g.boss_index&&q.leader_generation==e.generation);assert(q.max_hull==e.max_hull/4);++live;}
 assert(live==3);e.phase=1;e.phase_age=35;e.phase_timer=1000;e.fire_delay=0;e.action_count=0;
 for(unsigned b=0;b<MAX_BULLETS;++b)g.bullets[b].active=0;
 g.tick({});assert(hostiles(g)>=3);
 for(unsigned i=0;i<MAX_ENEMIES;++i)if(g.enemies[i].type==E_CRUCIBLE_DRONE)g.enemies[i].active=0;
 for(unsigned b=0;b<MAX_BULLETS;++b)g.bullets[b].active=0;
 e.fire_delay=0;g.tick({});assert(hostiles(g)==0&&e.phase_timer<=75);assert(g.validState());
}
void seraph_arsenal(){
 Game g={};setup(g,E_SERAPH);unsigned orbs=0,snipes=0;bool lances=false;
 for(unsigned t=0;t<1600&&!lances;++t){
  quiet(g);g.player.invulnerable=65000;g.next_wave=65000;g.state=Playing;g.tick({});assert(g.validState());
  for(unsigned b=FRIENDLY_BULLETS;b<MAX_BULLETS;++b)if(g.bullets[b].active){orbs+=g.bullets[b].kind==SHOT_SERAPH_ORB;snipes+=g.bullets[b].kind==SHOT_SERAPH_SNIPE;}
  if(g.enemies[g.boss_index].phase==2){unsigned n=0;for(unsigned k=0;k<MAX_EFFECTS;++k)n+=g.effects[k].active&&g.effects[k].kind==FX_SERAPH_LANCE;assert(n==3);lances=true;}
 }
 assert(orbs==90&&snipes==28&&lances);
}
void crucible_hoops(){
 Game g={};setup(g,E_CRUCIBLE,80);Enemy &e=g.enemies[g.boss_index];
 // The source prepareHoops writes eight, then unconditionally replaces that
 // value with twenty. A short fixed phase used to truncate this sequence.
 e.form=1;e.phase=6;e.phase_age=0;e.phase_timer=0;e.action_count=0;e.fire_delay=0;e.boss_hidden=0;
 g.tick({});assert(e.phase==7);
 unsigned emitted=0,elapsed=0;
 for(;elapsed<1800&&e.phase==7;++elapsed){
  g.player.invulnerable=65000;g.next_wave=65000;g.state=Playing;g.tick({});assert(g.validState());
  if(e.phase==7&&e.action_count>emitted)emitted=e.action_count;
 }
 assert(emitted==20);assert(e.phase==8);assert(elapsed>500&&elapsed<1000);
}
void station_and_star(){
 Game g={};setup(g,E_STATION_OMEGA,40);unsigned nodes=0,walls=0;int64_t hull=0;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].active){const BossPart &p=g.boss_parts[i];nodes+=p.kind==BP_STATION_NODE;walls+=p.kind==BP_STATION_WALL;hull+=p.hull;}
 assert(nodes==5&&walls==5&&hull==g.enemies[g.boss_index].max_hull);
 setup(g,E_STAR_EATER);g.tick({});unsigned segments=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)segments+=g.boss_parts[i].active&&(g.boss_parts[i].kind==BP_STAR_SEGMENT||g.boss_parts[i].kind==BP_STAR_ABDOMEN);assert(segments==28);
 Enemy &e=g.enemies[g.boss_index];e.hull=e.max_hull/2-1;g.tick({});segments=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)segments+=g.boss_parts[i].active&&(g.boss_parts[i].kind==BP_STAR_SEGMENT||g.boss_parts[i].kind==BP_STAR_ABDOMEN);assert(e.form==2&&segments==20);assert(g.validState());
}
}
int main(){
 const unsigned types[]={E_CERATOTITAN,E_SPITFIRE,E_STATION_OMEGA,E_DREADNOUGHT,E_SCION,E_DWELLER,E_WARBRINGER,E_CRUCIBLE,E_SERAPH,E_GLAUCUS,E_STAR_EATER};
 const unsigned phases[]={14,4,4,4,5,4,3,9,3,4,11};
 for(unsigned i=0;i<11;++i)cycle(types[i],phases[i]);
 disarm(E_DREADNOUGHT,BP_DREAD_GUN_LARGE,0);disarm(E_WARBRINGER,BP_WAR_BROADSIDE,0);disarm(E_STATION_OMEGA,BP_STATION_NODE,0);
 part_damage();station_and_star();crucible_hoops();crucible_drones();seraph_arsenal();
 std::puts("PASS all 11 boss phase cycles, hidden returns, source hull totals, independent damage and weapon detachment");
}
