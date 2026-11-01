#include <assert.h>
#include <stdio.h>
#include <string.h>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void setup(){g.start(419);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.options.auto_deploy_mask=0;}
static unsigned fields(){unsigned n=0;for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++)n+=g.persistent_fields[i].active;return n;}
static int durable(int x,int y){int id=g.spawnEnemy(E_WASP,x,y);assert(id>=0);g.enemies[id].hull=g.enemies[id].max_hull=10000;g.enemies[id].vx=g.enemies[id].vy=0;return id;}
static void burnout(){
 setup();g.player.mods[M_SELF_DESTRUCTION]=g.player.mods[M_X_BURNOUT_REACTORS]=1;
 int d=g.spawnConstruct(C_DRONE);assert(d>=0);g.constructs[d].x=100*FP;g.constructs[d].y=100*FP;
 int near=durable(102,100),far=durable(126,100);g.constructs[d].hull=0;g.updateConstructs();
 assert(!g.constructs[d].active&&fields()==1);assert(g.enemies[near].hull==10000&&g.enemies[far].hull==10000);
 int expected=120+3*25;unsigned changes=0;int last=10000;
 for(unsigned t=0;t<9;t++){g.updatePersistentEffects();assert(g.persistent_fields[0].x==100*FP&&g.persistent_fields[0].y==100*FP);
  if(g.enemies[far].hull!=last){changes++;last=g.enemies[far].hull;}if(t==0)assert(g.enemies[far].hull==10000);
 }
 assert(fields()==0&&changes==1&&last==10000-expected);assert(g.enemies[near].hull==10000-expected);
 setup();g.player.mods[M_X_BURNOUT_REACTORS]=1;d=g.spawnConstruct(C_DRONE);g.constructs[d].hull=0;g.updateConstructs();assert(fields()==0);
 setup();g.player.mods[M_SELF_DESTRUCTION]=1;d=g.spawnConstruct(C_DRONE);g.constructs[d].x=100*FP;g.constructs[d].y=100*FP;far=durable(126,100);g.constructs[d].hull=0;g.updateConstructs();assert(fields()==1&&g.enemies[far].hull==10000&&!g.persistent_fields[0].burn);for(unsigned tick=0;tick<9;tick++)g.updatePersistentEffects();assert(g.enemies[far].hull==10000-expected);
}
static void generationAndParts(){
 setup();g.player.mods[M_X_BURNOUT_REACTORS]=1;Construct dead={};dead.x=dead.y=100*FP;dead.max_hull=25;int id=durable(102,100);g.triggerBurnout(dead,195,30);g.updatePersistentEffects();assert(g.enemies[id].hull==9805);
 uint16_t generation=g.enemies[id].generation;g.enemies[id].active=0;int reused=durable(102,100);assert(reused==id&&g.enemies[id].generation!=generation);g.updatePersistentEffects();assert(g.enemies[id].hull==9805);g.updatePersistentEffects();assert(g.enemies[id].hull==9805);
 setup();id=g.spawnEnemy(E_STATION_OMEGA,160,120);assert(id>=0);unsigned part=MAX_BOSS_PARTS;for(unsigned i=0;i<MAX_BOSS_PARTS;i++)if(g.boss_parts[i].active){part=i;break;}assert(part<MAX_BOSS_PARTS);
 dead.x=g.boss_parts[part].x;dead.y=g.boss_parts[part].y;int before=g.boss_parts[part].hull;g.triggerBurnout(dead,20,30);g.updatePersistentEffects();assert(g.boss_parts[part].hull<before);int hit=g.boss_parts[part].hull;g.updatePersistentEffects();assert(g.boss_parts[part].hull==hit);
}
static void chargeAndForce(){
 setup();g.player.mods[M_X_MAELSTROM]=1;g.player.invulnerable=0;g.damagePlayer(10);assert(g.player.shield==90&&g.maelstrom_state.charge_q16>=5570&&g.maelstrom_state.charge_q16<=5571);
 uint32_t charged=g.maelstrom_state.charge_q16;g.updatePersistentEffects();assert(g.maelstrom_state.charge_q16<charged&&g.maelstrom_state.charge_q16>charged*98/100-111);
 setup();g.player.mods[M_X_MAELSTROM]=1;g.player.x=g.player.y=100*FP;g.recordMaelstromDamage(100);int id=durable(120,100);int32_t ox=g.enemies[id].x,oy=g.enemies[id].y;
 int bullet=g.emitBullet(120*FP,100*FP,0,0,10,SHOT_ENEMY,100,1);assert(bullet>=FRIENDLY_BULLETS);int32_t bullet_vx=g.bullets[bullet].vx,bullet_vy=g.bullets[bullet].vy;g.updatePersistentEffects();
 assert(g.enemies[id].x>ox&&g.enemies[id].y<oy&&g.enemies[id].hull==10000);assert(g.bullets[bullet].x>ox&&g.bullets[bullet].y<oy);assert(g.bullets[bullet].vx==bullet_vx&&g.bullets[bullet].vy==bullet_vy);assert(g.run_stats.damage_dealt==0);
 setup();g.player.mods[M_X_MAELSTROM]=g.player.mods[M_X_POLAR_INVERSION]=1;g.player.x=g.player.y=100*FP;g.recordMaelstromDamage(100);id=durable(120,100);g.updatePersistentEffects();assert(g.enemies[id].x<120*FP&&g.enemies[id].y>100*FP&&g.enemies[id].hull==10000);
 setup();g.player.mods[M_X_MAELSTROM]=1;g.player.x=g.player.y=100*FP;g.recordMaelstromDamage(10);id=durable(200,100);g.updatePersistentEffects();assert(g.enemies[id].x==200*FP&&g.enemies[id].y==100*FP);
 uint32_t radius=g.maelstrom_state.radius_q8;g.recordMaelstromDamage(100);g.stats.shield_radius*=2;g.updatePersistentEffects();assert(g.maelstrom_state.radius_q8>radius*2);
 g.player.shield=0;g.updatePersistentEffects();assert(!g.maelstrom_state.active&&!g.maelstrom_state.charge_q16);
 g.player.shield=100;g.player.mods[M_X_SHIELDED_DRONES]=1;g.recordMaelstromDamage(100);g.updatePersistentEffects();assert(!g.maelstrom_state.active&&!g.maelstrom_state.charge_q16);
}
static void anchoredAndHighPower(){
 setup();g.player.mods[M_X_MAELSTROM]=g.player.mods[M_X_SANCTUARY]=1;g.player.x=250*FP;g.player.y=200*FP;g.sanctuary_x=100*FP;g.sanctuary_y=100*FP;g.recordMaelstromDamage(100);int id=durable(120,100);g.updatePersistentEffects();assert(g.enemies[id].y<100*FP);
 setup();g.player.mods[M_X_MAELSTROM]=1;g.stats.shield_effect=32767;g.stats.shield_radius=32767;g.recordMaelstromDamage(UINT32_MAX);assert(g.maelstrom_state.charge_q16==UINT32_MAX);g.player.x=g.player.y=100*FP;id=durable(101,100);
 for(unsigned i=0;i<20;i++){g.updatePersistentEffects();assert(g.validState());assert(g.enemies[id].hull==10000);}
 setup();g.player.mods[M_X_MAELSTROM]=1;g.stats.shield_effect=1000;g.recordMaelstromDamage(10000);id=g.spawnEnemy(E_WORM,319,120,2);assert(id>=0);g.player.x=318*FP;g.player.y=120*FP;g.updatePersistentEffects();assert(g.validState());
}
static void boundedPool(){
 setup();Construct dead={};dead.x=100*FP;dead.y=100*FP;dead.max_hull=25;
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS*3;i++){g.triggerBurnout(dead,10,30);if(i&1)g.updatePersistentEffects();assert(fields()<=MAX_PERSISTENT_FIELDS);assert(g.validState());}
 for(unsigned i=0;i<10;i++)g.updatePersistentEffects();
 assert(fields()==0);
}
int main(){burnout();generationAndParts();chargeAndForce();anchoredAndHighPower();boundedPool();puts("persistent source burst expansion, hit identity, shield charge/decay, force inversion, part targets and high-power safety PASS");}
