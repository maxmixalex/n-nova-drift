#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
using namespace nova;
namespace {
void setup(Game &g){g.start(72819);g.configureCareer(64,true);g.configureRun(RUN_ENDLESS);g.wave=120;g.next_wave=65000;g.player.invulnerable=65000;}
int part(const Game &g,int owner,unsigned kind){for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].active&&g.boss_parts[i].owner==owner&&g.boss_parts[i].kind==kind)return int(i);return -1;}
unsigned parts(const Game &g,int owner){unsigned n=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)n+=g.boss_parts[i].active&&g.boss_parts[i].owner==owner;return n;}
void bulwark(){
 for(unsigned rarity=0;rarity<3;++rarity){Game g={};setup(g);int id=g.spawnEnemy(E_BULWARK,150,120,rarity);assert(id>=0);Enemy &e=g.enemies[id];e.angle=0;e.age=100;g.updateBossParts(id);
  assert(parts(g,id)==(rarity?13:10));int barrier=part(g,id,BP_BULWARK_BARRIER),core=part(g,id,BP_BULWARK_CORE);assert(barrier>=0&&core>=0);BossPart &p=g.boss_parts[barrier];
  assert(!g.sweptBossPart(p,e.x,e.y,e.x,e.y,0));
  assert(!g.sweptBossPart(p,e.x-60*FP,e.y,e.x-30*FP,e.y,0));
  assert(g.sweptBossPart(p,e.x+70*FP,e.y,e.x,e.y,0));
  // A rail traveling across the entire crescent cannot tunnel through it.
  assert(g.sweptBossPart(p,e.x+80*FP,e.y,e.x-80*FP,e.y,1));
  int owner_hp=e.hull,shield_hp=p.hull;g.hitBossPart(barrier,400);assert(p.hull==shield_hp-100&&e.hull==owner_hp&&p.timer==100);
  g.hitBossPart(core,100);assert(e.hull==owner_hp-100);
  e.hard_shield=200;int before=g.boss_parts[core].hull;assert(!g.hitBossPart(core,100));assert(e.hard_shield==100&&g.boss_parts[core].hull==before&&g.boss_parts[core].active);e.hard_shield=0;
  int body=part(g,id,BP_BULWARK_BODY2);g.hitBossPart(body,1000000);g.updateBossParts(id);assert(part(g,id,BP_BULWARK_TAIL)<0);assert(part(g,id,BP_BULWARK_BODY3)<0);
  // Source counter delay is bounded even under sustained shield pressure.
  e.form=1;e.phase_timer=60000;e.action_timer=0;e.target_x=750*256;e.target_y=0;p.timer=4000;g.player.x=e.x+60*FP;g.player.y=e.y;g.updateRegularEnemy(e);
  unsigned counter=0;for(unsigned b=FRIENDLY_BULLETS;b<MAX_BULLETS;++b)counter+=g.bullets[b].active&&g.bullets[b].kind==SHOT_CELESTIAL;const unsigned source_counts[]={42,55,67};assert(counter==source_counts[rarity]&&e.target_y==120&&p.timer==0);
  assert(g.validState());
 }
 // The source counter can emit 154 shots. Its own volley must fit in the
 // reserved hostile pool without replacing its first shots as it is created.
 Game full={};setup(full);int owner=full.spawnEnemy(E_BULWARK,150,120,2);assert(owner>=0);
 Enemy &e=full.enemies[owner];e.angle=0;e.form=1;e.age=100;e.phase_timer=60000;e.action_timer=0;e.target_x=750*256;e.target_y=0;
 int shield=part(full,owner,BP_BULWARK_BARRIER);assert(shield>=0);full.boss_parts[shield].timer=65535;
 full.player.x=e.x+60*FP;full.player.y=e.y;full.updateRegularEnemy(e);
 unsigned fan=0;for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;++i)fan+=full.bullets[i].active&&full.bullets[i].kind==SHOT_CELESTIAL;
 assert(fan==154&&full.validState());
}
void worm(){
 Game g={};setup(g);int id=g.spawnEnemy(E_WORM,150,120,2);assert(id>=0&&parts(g,id)==66);Enemy &e=g.enemies[id];int head=part(g,id,BP_WORM_HEAD);assert(head>=0);int cut=-1,next=-1;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(g.boss_parts[i].owner==id){if(g.boss_parts[i].slot==10)cut=int(i);if(g.boss_parts[i].slot==11)next=int(i);}
 assert(cut>=0&&next>=0);int hp=e.hull,cut_hp=g.boss_parts[cut].hull;g.hitBossPart(cut,1000000);assert(e.hull==hp-cut_hp);g.updateBossParts(id);assert(g.boss_parts[next].active&&g.boss_parts[next].kind==BP_WORM_HEAD);
 assert(part(g,id,BP_WORM_HEAD)>=0);assert(g.nearestTarget(e.x,e.y,200)>=MAX_ENEMIES);
 // Two long chains and a 29-part boss fit without partial bodies/heap growth.
 Game crowded={};setup(crowded);assert(crowded.spawnEnemy(E_WORM,100,100,2)>=0);assert(crowded.spawnEnemy(E_WORM,200,100,2)>=0);assert(crowded.spawnEnemy(E_STAR_EATER,160,50)>=0);assert(crowded.spawnEnemy(E_BULWARK,200,170,2)>=0);assert(crowded.spawnEnemy(E_BULWARK,60,170,2)>=0);assert(crowded.spawnEnemy(E_BULWARK,90,70,2)<0);
 assert(crowded.validState());
 for(unsigned t=0;t<1000;++t){g.next_wave=65000;g.player.invulnerable=65000;g.state=Playing;g.tick({});assert(g.validState());}
}
void rewards_and_ignition(){
 unsigned scores[2]={0,0},xp[2]={0,0};
 for(unsigned mode=0;mode<2;++mode){Game g={};setup(g);int id=g.spawnEnemy(E_BULWARK,150,100);int shield=part(g,id,BP_BULWARK_BARRIER),core=part(g,id,BP_BULWARK_CORE);
  if(!mode)g.hitBossPart(shield,1000000);
  g.hitBossPart(core,1000000);assert(!g.enemies[id].active&&g.kills==1);scores[mode]=g.score;
  for(unsigned i=0;i<MAX_PICKUPS;++i)if(g.pickups[i].active&&g.pickups[i].kind==PICKUP_XP)xp[mode]+=g.pickups[i].value;
 }
 assert(scores[0]==scores[1]&&xp[0]==xp[1]&&xp[0]>0);
 const unsigned types[]={E_WORM,E_BULWARK,E_STATION_OMEGA,E_STAR_EATER};
 for(unsigned type:types){Game g={};setup(g);g.player.mods[M_INCENDIARY]=1;int id=g.spawnEnemy(type,150,60);assert(id>=0);
  unsigned kind=type==E_WORM?BP_WORM_BODY:type==E_BULWARK?BP_BULWARK_CORE:type==E_STATION_OMEGA?BP_STATION_NODE:BP_STAR_EYE;
  int index=part(g,id,kind);assert(index>=0);g.hitBossPart(index,100);int after=g.boss_parts[index].hull;assert(g.boss_parts[index].burn_time);
  for(unsigned t=0;t<16;++t){g.next_wave=65000;g.player.invulnerable=65000;g.state=Playing;g.tick({});}
  assert(g.boss_parts[index].hull<after);assert(g.validState());
 }
}

void healing(){
 Game g={};setup(g);int id=g.spawnEnemy(E_BULWARK,150,100),core=part(g,id,BP_BULWARK_CORE),shield=part(g,id,BP_BULWARK_BARRIER);
 assert(id>=0&&core>=0&&shield>=0);g.hitBossPart(core,100);g.hitBossPart(shield,400);
 int owner_before=g.enemies[id].hull,part_before=g.boss_parts[core].hull,shield_before=g.boss_parts[shield].hull;
 assert(g.healEnemy(id,50)==50&&g.enemies[id].hull==owner_before+50&&g.boss_parts[core].hull==part_before+50);
 assert(g.boss_parts[shield].hull==shield_before);
 int cherub=g.spawnEnemy(E_CHERUB,155,100);assert(cherub>=0);g.updateRegularEnemy(g.enemies[cherub]);
 assert(g.enemies[id].hull>owner_before+50&&g.boss_parts[core].hull>part_before+50&&g.boss_parts[shield].hull==shield_before);
 Game worm_game={};setup(worm_game);id=worm_game.spawnEnemy(E_WORM,150,100,2);int dead=part(worm_game,id,BP_WORM_BODY),live=part(worm_game,id,BP_WORM_HEAD);
 assert(dead>=0&&live>=0);worm_game.hitBossPart(dead,1000000);worm_game.hitBossPart(live,50);owner_before=worm_game.enemies[id].hull;
 assert(worm_game.healEnemy(id,1000000)==50&&worm_game.enemies[id].hull==owner_before+50&&!worm_game.boss_parts[dead].active);
 assert(worm_game.healEnemy(id,1000000)==0&&worm_game.validState());
 const unsigned types[]={E_STATION_OMEGA,E_STAR_EATER};
 for(unsigned type:types){Game boss={};setup(boss);int owner=boss.spawnEnemy(type,150,60),at=part(boss,owner,type==E_STAR_EATER?BP_STAR_EYE:BP_STATION_NODE);assert(at>=0);
  boss.hitBossPart(at,100);int before=boss.boss_parts[at].hull,hull=boss.enemies[owner].hull;assert(boss.healEnemy(owner,50)==50&&boss.boss_parts[at].hull==before+50&&boss.enemies[owner].hull==hull+50&&boss.validState());
 }
}

void juggernaut(){
 for(unsigned rarity=0;rarity<3;++rarity){Game g={};setup(g);int id=g.spawnEnemy(E_JUGGERNAUT,120,100,rarity);assert(id>=0);Enemy &e=g.enemies[id];bool gun=false,missile=false,orb=false;
  for(unsigned t=0;t<1100;++t){g.player.invulnerable=65000;g.next_wave=65000;g.state=Playing;if(t%100==0){e.phase_timer=0;e.phase=0;e.angle=direction((g.player.x-e.x)/FP,(g.player.y-e.y)/FP);}g.tick({});
   for(unsigned b=FRIENDLY_BULLETS;b<MAX_BULLETS;++b)if(g.bullets[b].active){gun|=g.bullets[b].kind==SHOT_JUGGERNAUT;orb|=g.bullets[b].kind==SHOT_JUGGERNAUT_ORB;}
   for(unsigned n=0;n<MAX_ENEMIES;++n)if(g.enemies[n].active)missile|=g.enemies[n].type==E_JUGG_MISSILE||g.enemies[n].type==E_JUGG_MISSILE_HOMING;
   assert(g.validState());
  }assert(gun&&missile&&orb);
 }
 Game g={};setup(g);int m=g.spawnEnemy(E_JUGG_MISSILE,60,80);assert(m>=0&&g.enemies[m].hull==5);g.hitEnemy(m,6);assert(!g.enemies[m].active);m=g.spawnEnemy(E_JUGG_MISSILE_HOMING,60,80);assert(m>=0&&g.enemies[m].hull==60);g.hitEnemy(m,61);assert(!g.enemies[m].active);
}
}
int main(){bulwark();worm();rewards_and_ignition();healing();juggernaut();std::puts("ordinary multipart enemies: crescent collision, independent hull, split chains, pool retry, component healing/ignition/rewards, all rarity attacks and destructible missiles PASS");}
