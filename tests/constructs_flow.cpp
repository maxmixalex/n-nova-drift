#include <assert.h>
#include <stdio.h>
#include <string.h>
// These deterministic checks use the real core entry points and bounded pools.
// Private stepping avoids unrelated wave/ship regeneration while measuring
// source timing and construct health; no implementation is duplicated here.
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void clean(unsigned seed=42){g.start(seed);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;}
static void step(unsigned ticks=1){while(ticks--){++g.frame;g.updateConstructs();}}
static unsigned count(unsigned kind){unsigned n=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)n+=g.constructs[i].active&&g.constructs[i].kind==kind;return n;}
static unsigned commanders(){unsigned n=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)n+=g.constructs[i].active&&g.constructs[i].promoted;return n;}
static void limitsAndCosts(){
 clean();g.player.mods[M_DRONES]=g.player.mods[M_MINES]=g.player.mods[M_TURRET]=g.player.mods[M_ALLY]=1;
 assert(g.combinedConstructLimits()==5);assert(g.constructLimit(C_MINE)==3);
 g.player.mods[M_OVERSEER]=1;assert(g.constructLimit(C_DRONE)==6);assert(g.constructLimit(C_MINE)==6);
 assert(g.constructLimit(C_TURRET)==2&&g.constructLimit(C_ALLY)==2);assert(g.combinedConstructLimits()==10);
 g.player.body=B_ENGINEER;assert(g.constructLimit(C_DRONE)==10&&g.constructLimit(C_MINE)==10);assert(g.combinedConstructLimits()==16);
 g.player.mods[M_X_SPECIALIST_DRONE]=1;assert(g.constructLimit(C_DRONE)==14);assert(g.combinedConstructLimits()==20);
 clean();g.player.mods[M_DRONES]=g.player.mods[M_MINES]=g.player.mods[M_TURRET]=g.player.mods[M_ALLY]=1;g.player.mods[M_HULL_STRENGTH]=1;g.recalculate();int hull=g.player.max_hull,shield=g.player.max_shield;
 g.player.mods[M_X_SUPPORT_SPECIALIST]=1;g.recalculate();assert(g.player.max_hull==hull*90/100);assert(g.player.max_shield==shield*90/100);
 g.player.mods[M_MINES]=0;g.recalculate();assert(g.player.max_hull==hull*90/100); // Basic mine limits do not tax defences.
}
static void deployment(){
 clean();g.player.mods[M_DRONES]=g.player.mods[M_MINES]=g.player.mods[M_TURRET]=g.player.mods[M_ALLY]=1;g.recalculate();g.options.auto_deploy_mask=0;
 step(2700);assert(g.constructCount()==0);g.construct_deploy_requested_=true;step();
 assert(count(C_DRONE)==3&&count(C_MINE)==3&&count(C_TURRET)==1&&count(C_ALLY)==1);
 assert(!g.construct_deploy_requested_);unsigned before=g.constructCount();step(10);assert(g.constructCount()==before);
 clean();g.player.mods[M_DRONES]=g.player.mods[M_X_ROCKET_DRONES]=1;g.recalculate();g.options.auto_deploy_mask=0;
 g.construct_deploy_requested_=true;step();assert(count(C_DRONE)==1);assert(g.player.hull==85&&g.player.shield==85);
 clean();g.player.mods[M_DRONES]=g.player.mods[M_X_ROCKET_DRONES]=1;g.recalculate();step(150);assert(count(C_DRONE)==1);assert(g.player.hull==100&&g.player.shield==100);
 clean();g.player.mods[M_MINES]=g.player.mods[M_MINEFIELD]=1;g.recalculate();step(239);assert(count(C_MINE)==0);step();assert(count(C_MINE)==6);
}
static void graceAndFleet(){
 clean();g.player.mods[M_GRACE_PROTOCOL]=1;int d=g.spawnConstruct(C_DRONE);assert(d>=0);g.damageConstruct(d,1000);assert(g.constructs[d].hull==1&&g.constructs[d].grace_invulnerable==30&&g.constructs[d].grace_cooldown==450);
 g.damageConstruct(d,1000);assert(g.constructs[d].hull==1);step(29);g.damageConstruct(d,1000);assert(g.constructs[d].hull==1);step();g.damageConstruct(d,1000);step();assert(!g.constructs[d].active);
 clean();g.player.mods[M_GRACE_PROTOCOL]=1;d=g.spawnConstruct(C_DRONE);g.constructs[d].hull=0;step();assert(g.constructs[d].active&&g.constructs[d].hull==1);
 clean();g.player.mods[M_DRONES]=g.player.mods[M_X_FLEET]=1;g.options.auto_deploy_mask=0;for(unsigned i=0;i<4;i++)assert(g.spawnConstruct(C_DRONE)>=0);step();assert(commanders()==2);assert(g.constructs[0].max_hull==75&&g.constructs[2].max_hull==25);
 g.damageConstruct(0,1000);step();assert(commanders()==1);step(59);assert(commanders()==1);step();assert(commanders()==2);assert(g.constructs[2].max_hull==75);
}
static void regenerationAndDecay(){
 clean();g.player.mods[M_X_VITAL_BOND]=1;int t=g.spawnConstruct(C_TURRET);assert(t>=0);g.constructs[t].hull=100;
 step(300);assert(g.constructs[t].active&&g.constructs[t].hull==71); // +21 recovery, -50 degeneration.
 clean();g.player.mods[M_X_VITAL_BOND]=1;int d=g.spawnConstruct(C_DRONE);g.constructs[d].hull=5;step(300);assert(g.constructs[d].hull==8);step(300);assert(g.constructs[d].hull==12);
 clean();int s=g.spawnConstruct(C_SWARM);g.constructs[s].charges=1;step(300);assert(g.constructs[s].hull==1);step(21);assert(!g.constructs[s].active); // Fractional .75 hp/s, no 10s timeout.
 clean();t=g.spawnConstruct(C_TURRET);g.player.mods[M_X_VITAL_BOND]=g.player.mods[M_RAPID_RECONSTRUCTION]=1;g.recalculate();step(950);assert(g.constructs[t].active); // Regeneration extends a turret beyond its old hard timeout.
}
static void destruction(){
 clean();g.player.mods[M_SELF_DESTRUCTION]=1;int d=g.spawnConstruct(C_DRONE);g.constructs[d].x=100*FP;g.constructs[d].y=100*FP;
 int e=g.spawnEnemy(E_WASP,100,100);assert(e>=0);g.enemies[e].hull=g.enemies[e].max_hull=1000;g.constructs[d].hull=0;step();assert(g.enemies[e].hull==1000);for(unsigned tick=0;tick<9;tick++)g.updatePersistentEffects();assert(g.enemies[e].hull==805);
 clean();g.player.mods[M_SELF_DESTRUCTION]=1;int t=g.spawnConstruct(C_TURRET);g.constructs[t].x=100*FP;g.constructs[t].y=100*FP;e=g.spawnEnemy(E_WASP,140,100);g.enemies[e].hull=g.enemies[e].max_hull=1000;g.constructs[t].hull=0;step();assert(g.enemies[e].hull==1000);for(unsigned tick=0;tick<9;tick++)g.updatePersistentEffects();assert(g.enemies[e].hull==430);
}
static void spontaneous(){
 Enemy wreck={};wreck.type=E_WASP;wreck.x=40*FP;wreck.y=60*FP;
 unsigned tiny=0,large=0;
 for(unsigned seed=1;seed<=2000;seed++){
  clean(seed);g.player.mods[M_X_SPONTANEOUS_GENERATION]=1;wreck.max_hull=10;g.onEnemyDestroyedConstructEffects(wreck);tiny+=g.constructCount();
  clean(seed);g.player.mods[M_X_SPONTANEOUS_GENERATION]=1;wreck.max_hull=10000;g.onEnemyDestroyedConstructEffects(wreck);large+=g.constructCount();
  for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(g.constructs[c].active)assert(g.constructs[c].x==wreck.x&&g.constructs[c].y==wreck.y&&g.constructs[c].charges==1);
 }
 assert(tiny<30);assert(large>8500&&large<12000); // Native expectation is about 5.24 per 10,000-hull wreck.
 clean();g.player.mods[M_X_SPONTANEOUS_GENERATION]=mods[M_X_SPONTANEOUS_GENERATION].max_rank;wreck.max_hull=2000000000;g.onEnemyDestroyedConstructEffects(wreck);assert(g.constructCount()==MAX_CONSTRUCTS);assert(g.validState());
 clean();g.player.mods[M_X_SPONTANEOUS_GENERATION]=mods[M_X_SPONTANEOUS_GENERATION].max_rank;wreck.type=E_ASTEROID;g.onEnemyDestroyedConstructEffects(wreck);assert(!g.constructCount());
}
static void loadedWeapons(){
 static const int kinds[WEAPON_COUNT]={SHOT_BLASTER,SHOT_SPLIT,SHOT_FLAK,SHOT_RAIL,SHOT_GRENADE,SHOT_MISSILE,SHOT_VORTEX,SHOT_SPARK,SHOT_TORRENT,-1,SHOT_DART,-2,-3};
 for(unsigned weapon=0;weapon<WEAPON_COUNT;weapon++){
  clean(80+weapon);g.player.weapon=(uint8_t)weapon;g.player.mods[M_MINES]=g.player.mods[M_LOADED_MINES]=1;g.recalculate();g.options.auto_deploy_mask=0;
  g.player.missiles=17;g.player.charge=23;g.player.fire_delay=7;g.player.angle=39;g.player.vx=47;g.player.vy=-17;
  g.fire_fraction_=57;g.charged_scale_=387123;g.burst_left_=2;g.burst_delay_=3;g.stored_damage_=71;
  Player before=g.player;int mine=g.spawnConstruct(C_MINE);assert(mine>=0);g.constructs[mine].x=80*FP;g.constructs[mine].y=120*FP;g.constructs[mine].charge_ticks=360;g.constructs[mine].hull=0;
  step();assert(memcmp(&before,&g.player,sizeof(Player))==0);assert(g.fire_fraction_==57&&g.charged_scale_==387123&&g.burst_left_==2&&g.burst_delay_==3&&g.stored_damage_==71);
  unsigned matching=0;
  if(kinds[weapon]>=0){for(unsigned b=0;b<FRIENDLY_BULLETS;b++)if(g.bullets[b].active&&g.bullets[b].kind==kinds[weapon]){++matching;assert(g.bullets[b].construct_source==mine+1);}}
  else if(kinds[weapon]==-1)matching=count(C_BLADE);
  else if(kinds[weapon]==-3){matching=count(C_SWORD);for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(g.constructs[c].active&&g.constructs[c].kind==C_SWORD)assert(g.advanced_constructs[c].temporary&&g.advanced_constructs[c].anchor_x==80*FP);}
  else for(unsigned e=0;e<MAX_EFFECTS;e++)matching+=g.effects[e].active&&g.effects[e].kind==(kinds[weapon]==-2?FX_BEAM:FX_SWORD);
  if(!matching)fprintf(stderr,"Loaded weapon %u emitted no inherited attack\n",weapon);
  assert(matching);if(weapon==W_BLASTER)assert(matching==16);
  assert(g.validState());
 }
 clean();g.player.weapon=W_VORTEX;g.player.mods[M_MINES]=g.player.mods[M_LOADED_MINES]=1;g.recalculate();g.options.auto_deploy_mask=0;int mine=g.spawnConstruct(C_MINE);step(60);assert(g.constructs[mine].vortex_scale>=564800&&g.constructs[mine].vortex_scale<=564900&&g.constructs[mine].hull==17);assert(g.constructs[mine].redirected_fraction>=675&&g.constructs[mine].redirected_fraction<=677);assert(g.player.hull==100);
 clean();g.player.weapon=W_THERMAL_LANCE;g.player.mods[M_MINES]=g.player.mods[M_LOADED_MINES]=1;g.recalculate();g.options.auto_deploy_mask=0;mine=g.spawnConstruct(C_MINE);step(60);assert(g.constructs[mine].charge_ticks==360&&g.constructs[mine].hull==17);assert(g.player.hull==100);
 clean();g.player.mods[M_MINES]=g.player.mods[M_LOADED_MINES]=g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_CHARGED_MINES]=1;g.recalculate();g.options.auto_deploy_mask=0;assert(g.constructLimit(C_MINE)==2);mine=g.spawnConstruct(C_MINE);step(60);assert(g.constructs[mine].weapon_charge>0&&g.constructs[mine].hull==21);
 GameInput fire={};fire.fire=true;g.player.invulnerable=60000;g.next_wave=60000;g.tick(fire);assert(g.bulletCount()>0); // Charged Shot no longer delays the ship's weapon.
}
int main(){limitsAndCosts();deployment();graceAndFleet();regenerationAndDecay();destruction();spontaneous();loadedWeapons();puts("constructs-flow: source limits, deployment, Grace/Fleet, fractional health, blasts, wreck spawning and 13 loaded weapon families PASS");}
