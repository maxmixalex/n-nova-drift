#include <assert.h>
#include <stdio.h>
#include <string.h>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void clean(){g.start(819);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.player.angle=0;g.player.x=140*FP;g.player.y=120*FP;}
static void step(unsigned ticks=1){while(ticks--){++g.frame;g.updateConstructs();}}
static unsigned count(unsigned kind){unsigned n=0;for(unsigned i=0;i<MAX_CONSTRUCTS;++i)n+=g.constructs[i].active&&g.constructs[i].hull>0&&g.constructs[i].kind==kind;return n;}
static int first(unsigned kind){for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(g.constructs[i].active&&g.constructs[i].hull>0&&g.constructs[i].kind==kind)return int(i);return -1;}
static void swordObjects(){
 clean();assert(g.equipWeapon(W_SWORDS));assert(g.constructLimit(C_SWORD)==2);assert(g.combinedConstructLimits()==1);
 step(119);assert(!count(C_SWORD));step();assert(count(C_SWORD)==2);
 int s=first(C_SWORD);assert(g.constructs[s].max_hull==100);g.damageConstruct(s,102);step();assert(count(C_SWORD)==1);step(119);assert(count(C_SWORD)==1);step();assert(count(C_SWORD)==2);
 clean();assert(g.equipWeapon(W_SWORDS));g.player.body=B_ENGINEER;g.player.mods[M_OVERSEER]=1;g.recalculate();assert(g.constructLimit(C_SWORD)==6);assert(g.combinedConstructLimits()==3);
 g.player.mods[M_X_APEX_MACHINERY]=1;g.recalculate();step(600);assert(count(C_SWORD)==6);s=first(C_SWORD);assert(g.constructs[s].max_hull==130);
}
static void swordAttack(){
 clean();g.equipWeapon(W_SWORDS);step(120);int s=first(C_SWORD);for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(int(i)!=s)g.constructs[i].active=0;
 int e=g.spawnEnemy(E_WASP,165,110);assert(e>=0);g.enemies[e].hull=g.enemies[e].max_hull=10000;g.enemies[e].radius=10;
 int32_t old_x=g.constructs[s].x,old_y=g.constructs[s].y;g.attackSwords(150,25,0,false);bool moved=false,hit=false;int after=10000;
 for(unsigned n=0;n<45;n++){step();moved|=g.constructs[s].x!=old_x||g.constructs[s].y!=old_y;if(g.enemies[e].hull<10000){if(!hit){hit=true;after=g.enemies[e].hull;}else assert(g.enemies[e].hull==after);}}
 assert(moved&&hit&&after==9850);assert(g.advanced_constructs[s].state==0);
 g.attackSwords(150,25,0,false);step(45);assert(g.enemies[e].hull==9700);
 // A mine receives independent temporary swords. The player pair is untouched.
 AdvancedConstructState saved=g.advanced_constructs[s];int32_t saved_x=g.constructs[s].x;g.player.x=50*FP;g.player.y=40*FP;g.attackSwords(150,25,64,true);
 assert(memcmp(&saved,&g.advanced_constructs[s],sizeof(saved))==0&&g.constructs[s].x==saved_x);unsigned temporary=0;
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(g.constructs[i].active&&g.advanced_constructs[i].temporary){++temporary;assert(g.advanced_constructs[i].anchor_x==50*FP);}
 assert(temporary==2);step(45);for(unsigned i=0;i<MAX_CONSTRUCTS;++i)assert(!g.constructs[i].active||!g.advanced_constructs[i].temporary);
}
static void registeredWielder(){
 clean();g.equipWeapon(W_SWORDS);g.player.mods[M_ALLY]=1;g.player.mods[M_INTERCEPTOR]=1;g.player.mods[M_X_OVERPOWER]=1;g.player.mods[M_OVERSEER]=1;g.recalculate();step(240);
 int owner=g.spawnConstruct(C_INTERCEPTOR);assert(owner>=0);g.constructs[owner].x=70*FP;g.constructs[owner].y=70*FP;g.constructs[owner].vx=g.constructs[owner].vy=0;
 // Registration cannot move or activate the player's idle controller.
 unsigned player_swords=0;for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(g.constructs[i].active&&g.constructs[i].kind==C_SWORD&&!g.advanced_constructs[i].wielder)++player_swords;
 assert(player_swords==4);for(unsigned n=0;n<120;++n)g.updateSwordWielder((unsigned)owner,-1);
 unsigned owned=0;int sword=-1;for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(g.constructs[i].active&&g.advanced_constructs[i].wielder==owner+1){++owned;sword=(int)i;assert(!g.advanced_constructs[i].temporary);}
 assert(owned==2&&sword>=0&&count(C_SWORD)==6&&g.constructLimit(C_SWORD)==4);
 int enemy=g.spawnEnemy(E_WASP,95,60);assert(enemy>=0);g.enemies[enemy].hull=g.enemies[enemy].max_hull=10000;g.enemies[enemy].radius=10;g.constructs[owner].angle=0;g.constructs[owner].fire_delay=0;
 g.updateSwordWielder((unsigned)owner,enemy);
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(g.constructs[i].active&&g.constructs[i].kind==C_SWORD&&!g.advanced_constructs[i].wielder)assert(g.advanced_constructs[i].state==0);
 for(unsigned tick=0;tick<45;++tick){for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(g.constructs[i].active&&g.advanced_constructs[i].wielder==owner+1){if(g.constructs[i].fire_delay)--g.constructs[i].fire_delay;g.updateAdvancedConstruct(i);}}
 assert(g.enemies[enemy].hull<10000);assert(!g.bulletCount());assert(g.player.fire_delay==0);
 g.damageConstruct((unsigned)sword,102);step();assert(!g.constructs[sword].active);assert(g.constructs[owner].active);
 // Removing the wielder cleans both blades; slot reuse cannot inherit them.
 g.constructs[owner].hull=0;step();for(unsigned i=0;i<MAX_CONSTRUCTS;++i)assert(!g.constructs[i].active||g.advanced_constructs[i].wielder!=owner+1);
 assert(g.validState());
}
static unsigned parry(unsigned damage){
 clean();g.equipWeapon(W_SWORDS);step(120);g.attackSwords(150,25,0,false);step();int s=first(C_SWORD);Construct &c=g.constructs[s];int b=g.emitBullet(c.x,c.y,c.angle,0,damage,SHOT_ENEMY,50,1,2);assert(b>=0);
 assert(g.interceptAdvancedProjectile(unsigned(b),c.x,c.y));assert(!g.bullets[b].active);unsigned sparks=0;for(unsigned i=0;i<FRIENDLY_BULLETS;++i)if(g.bullets[i].active&&g.bullets[i].kind==SHOT_SPARK){++sparks;assert(g.bullets[i].damage==75);}assert(sparks);return sparks;
}
static void swordParry(){unsigned small=parry(10),large=parry(300);assert(large>small);clean();g.equipWeapon(W_SWORDS);step(120);int s=first(C_SWORD);Construct &c=g.constructs[s];int b=g.emitBullet(c.x,c.y,0,0,10,SHOT_ENEMY,20,1,2);assert(!g.interceptAdvancedProjectile(unsigned(b),c.x,c.y));}
static void bastion(){
 clean();assert(g.equipShield(S_BASTION));step();int s=first(C_BASTION);assert(s>=0&&count(C_BASTION)==1);assert(g.constructs[s].max_hull==120&&g.player.max_shield==120);
 g.damageConstruct(s,40);assert(g.player.shield==80&&g.constructs[s].hull==80&&g.player.shield_energy==30);assert(g.bastionResistancePercent()>0);
 GameInput input={};g.tick(input);assert(g.player.shield==80);step(299);assert(g.player.shield>=99&&g.player.shield<=100);
 // Direct hull damage bypasses the physical barrier and releases stored energy.
 g.player.invulnerable=0;int shield=g.player.shield,hull=g.player.hull;g.damagePlayer(5);assert(g.player.shield==shield&&g.player.hull<hull);assert(g.advanced_constructs[s].state==1);
 step();assert(g.player.shield_energy==20);step(2);assert(!g.player.shield_energy&&!g.advanced_constructs[s].state);
 g.damageConstruct(s,1000);assert(g.player.shield==0&&g.player.shield_energy==0);step();assert(!count(C_BASTION));
 step(449);assert(!count(C_BASTION));step();assert(count(C_BASTION)==1&&g.player.shield==120);
 g.player.mods[M_ELEGANT_CONSTRUCTION]=1;g.recalculate();assert(g.player.max_shield==150);step();assert(g.constructs[first(C_BASTION)].max_hull==150);
}
static void bastionGeometry(){
 clean();g.equipShield(S_BASTION);step();int s=first(C_BASTION);Construct &c=g.constructs[s];int b=g.emitBullet(c.x,c.y,128,0,10,SHOT_ENEMY,50,1,2);assert(b>=0);assert(g.interceptAdvancedProjectile(unsigned(b),c.x+5*FP,c.y));assert(g.player.shield==110);
 b=g.emitBullet(g.player.x-15*FP,g.player.y,0,0,10,SHOT_ENEMY,50,1,2);assert(!g.interceptAdvancedProjectile(unsigned(b),g.player.x-20*FP,g.player.y));
 assert(g.validState());
}
int main(){swordObjects();swordAttack();swordParry();registeredWielder();bastion();bastionGeometry();printf("advanced-constructs: real sword assembly, per-attack hits, parry, mine independence, physical Bastion and rebuild PASS (Game %zu, controller %zu)\n",sizeof(Game),sizeof(AdvancedConstructState));}
