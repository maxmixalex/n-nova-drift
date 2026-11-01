#include <assert.h>
#include <stdio.h>
#include <string.h>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void setup(){g.start(713);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.options.auto_deploy_mask=0;}
static unsigned tracked(){unsigned n=0;for(unsigned i=0;i<FRIENDLY_BULLETS;i++)n+=g.bullets[i].active&&g.bullets[i].tracking;return n;}
static int live(){for(unsigned i=0;i<MAX_ENEMIES;i++)if(g.enemies[i].active&&g.enemies[i].challenger)return (int)i;return -1;}
static unsigned fields(){unsigned n=0;for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++)n+=g.persistent_fields[i].active;return n;}
static void homingSourceGoldens(){
 setup();g.player.mods[M_HOMING_STRIKE]=1;g.fireHomingStrike(0,10,10,100,80,2);assert(tracked()==6);
 for(unsigned i=0;i<6;i++){const Bullet &b=g.bullets[i];assert(b.angle==(i&1?85:171));assert(b.damage==30&&b.speed==4*FP);assert(b.tracking_scale_x==2867&&b.tracking_scale_y==2867);assert(b.homing==5&&b.tracking_seek==3072);}
 Bullet &b=g.bullets[0];b.age=40;g.updateHomingStrike(b);assert(b.tracking_scale_x==2867&&b.tracking_scale_y==2867);b.age=41;g.updateHomingStrike(b);assert(b.tracking_scale_x==2777&&b.tracking_scale_y==2759);
 setup();g.player.mods[M_HOMING_STRIKE]=1;g.player.weapon=W_SPLIT_SHOT;g.fireHomingStrike(0,1,10,100,80,weapons[W_SPLIT_SHOT].radius);Bullet &split=g.bullets[0];unsigned before=split.tracking_scale_x;split.age=43;g.updateHomingStrike(split);assert(split.tracking_scale_x==before);split.age=44;g.updateHomingStrike(split);assert(split.tracking_scale_x==before-1);
 setup();g.player.mods[M_HOMING_STRIKE]=1;g.player.weapon=W_DART;g.fireHomingStrike(0,1,10,100,80,weapons[W_DART].radius);Bullet &dart=g.bullets[0];before=dart.tracking_scale_x;dart.age=41;g.updateHomingStrike(dart);assert(dart.tracking_scale_x==before-204);dart.pierce=254;before=dart.tracking_scale_x;dart.age=80;g.updateHomingStrike(dart);assert(dart.tracking_scale_x==before&&dart.active);
 setup();g.player.mods[M_HOMING_STRIKE]=1;g.player.weapon=W_VORTEX;g.vortex_scale_=2000000;g.rancor_scale_=1000000;g.fireHomingStrike(0,1,10,100,80,4);assert(g.bullets[0].field_scale==2100000&&g.bullets[0].damage==(int)g.weaponDamage()*1000);
 setup();g.player.mods[M_HOMING_STRIKE]=1;g.player.x=20*FP;g.player.y=20*FP;int target=g.spawnEnemy(E_DRONE,250,20);assert(target>=0);g.fireHomingStrike(0,1,10,100,80,2);g.bullets[0].age=1;g.updateHomingStrike(g.bullets[0]);assert(g.bullets[0].tracking_target==target);assert(g.bullets[0].tracking_seek>3072);
 setup();g.player.mods[M_HOMING_STRIKE]=1;g.player.x=20*FP;g.player.y=20*FP;
 // The source Eclipse now has a real shield: prove it obstructs the body
 // before isolating the original distance/weakness weighting regression.
 int weak=g.spawnEnemy(E_ECLIPSE,120,20);assert(weak>=0);
 int shield_part=-1;for(unsigned i=0;i<MAX_BOSS_PARTS;i++)if(g.boss_parts[i].active&&g.boss_parts[i].owner==(unsigned)weak&&g.boss_parts[i].kind==BP_ECLIPSE_SHIELD)shield_part=(int)i;
 assert(shield_part>=0&&g.targetObstructedByEclipse(weak));
 g.fireHomingStrike(0,1,10,100,80,2);Bullet &weighted=g.bullets[0];weighted.age=1;g.updateHomingStrike(weighted);assert(weighted.tracking_target==MAX_ENEMIES+shield_part);
 g.hitBossPart((unsigned)shield_part,g.boss_parts[shield_part].hull+1,false);
 assert(!g.boss_parts[shield_part].active&&!g.targetObstructedByEclipse(weak));
 int ignored=g.spawnEnemy(E_HOST_DRONE,30,20),near=g.spawnEnemy(E_DRONE,80,20);assert(ignored>=0&&near>=0);
 weighted.age=3;g.updateHomingStrike(weighted);assert(weighted.tracking_target==weak);
 g.enemies[weak].hard_shield=100;weighted.age=3;g.updateHomingStrike(weighted);assert(weighted.tracking_target==near);
 g.enemies[near].hard_shield=100;weighted.age=6;g.updateHomingStrike(weighted);assert(weighted.tracking_target==weak);
}
static void lastStandSourceGoldens(){
 setup();g.player.mods[M_X_LAST_STAND]=1;int hull=g.player.max_hull,shield=g.player.max_shield;g.player.hull=0;g.player.shield=0;g.preventDeath();assert(g.last_stand_used_&&g.state==Playing);assert(g.player.hull==hull*40/100&&g.player.shield==shield*40/100);assert(g.player.invulnerable==90&&fields()==1);
 PersistentField &f=g.persistent_fields[0];assert(f.reserved[0]==1&&f.damage==18000&&!f.burn&&f.base_radius==333&&f.radius==166);
 int enemy=g.spawnEnemy(E_WASP,10,10);assert(enemy>=0);g.enemies[enemy].hull=g.enemies[enemy].max_hull=100000;g.updatePersistentEffects();assert(f.source_age==2&&f.radius==333&&f.opacity==255);assert(g.enemies[enemy].hull==82000);
 for(unsigned i=1;i<6;i++){g.updatePersistentEffects();}assert(f.source_age==12&&f.radius==1165&&f.opacity==248);assert(g.enemies[enemy].hull==82000);
 for(unsigned i=6;i<25;i++){g.updatePersistentEffects();}assert(f.source_age==50&&f.radius==4096&&f.opacity==6&&f.active);g.updatePersistentEffects();assert(!f.active);
 g.player.hull=0;g.preventDeath();assert(g.state==GameOver);
 setup();g.player.mods[M_X_LAST_STAND]=1;g.last_stand_used_=1;unsigned xp=g.xp;g.giveExperience(1);assert(g.xp==xp+2);
 setup();g.player.mods[M_X_LAST_STAND]=1;g.emitPickup(g.player.x+50*FP,g.player.y,PICKUP_XP,1);g.pickups[0].vx=g.pickups[0].vy=0;g.updatePickups();assert(g.pickups[0].vx<0);
}
static void bastionSourceCurve(){
 setup();g.player.x=150*FP;g.player.y=120*FP;g.player.angle=0;Construct origin={};origin.x=g.player.x;origin.y=g.player.y;g.triggerBurnout(origin,100,1);PersistentField &f=g.persistent_fields[0];f.reserved[0]=2;f.reserved[1]=255;f.base_radius=1000;f.radius=26;f.burn=0;
 int front=g.spawnEnemy(E_DRONE,170,120),back=g.spawnEnemy(E_DRONE,130,120);assert(front>=0&&back>=0);g.enemies[front].hull=g.enemies[front].max_hull=g.enemies[back].hull=g.enemies[back].max_hull=1000;
 g.updatePersistentEffects();assert(f.source_age==2&&f.radius==28&&f.opacity==255);assert(g.enemies[front].hull==900&&g.enemies[back].hull==1000);
 g.player.angle=128;g.updatePersistentEffects();assert(g.enemies[back].hull==900);g.player.mods[M_X_SANCTUARY]=1;g.sanctuary_x=80*FP;g.sanctuary_y=90*FP;g.updatePersistentEffects();assert(f.x==80*FP&&f.y==90*FP);
 while(f.source_age<16){g.updatePersistentEffects();}assert(f.opacity==243&&f.radius==42);while(f.active){g.updatePersistentEffects();}assert(f.source_age==35&&g.validState());
}
static void challengerSourceGoldens(){
 setup();g.wave=30;g.challenger_source_=SE_SNAPPER+1;g.queueChallenger();g.updateChallenger();int id=live();assert(id>=0);const Enemy first=g.enemies[id];assert(first.type==E_SNAPPER&&first.elite==2);assert(first.size_scale==303&&first.damage_scale==297&&first.treasure_scale==350);assert(g.challenger_pending_==0&&g.challenger_spawned_==1);assert(first.hull>enemy_types[E_SNAPPER].hull*22/10);
 setup();g.wave=30;g.challenger_spawned_=4;g.challenger_source_=SE_SNAPPER+1;g.queueChallenger();g.updateChallenger();id=live();assert(id>=0);assert(g.enemies[id].size_scale==397&&g.enemies[id].damage_scale==378&&g.enemies[id].treasure_scale==585);
 setup();g.wave=100;for(unsigned i=0;i<MAX_ENEMIES;i++)assert(g.spawnEnemy(E_ASTEROID,10,10)>=0);g.challenger_source_=SE_HIVE+1;g.queueChallenger();g.updateChallenger();assert(g.challenger_pending_==1&&g.challenger_source_==SE_HIVE+1&&!g.challenger_spawned_);g.enemies[0].active=0;g.updateChallenger();assert(live()==0&&g.enemies[0].type==E_HIVE);
 setup();g.wave=100;g.challenger_source_=SE_BULWARK+1;g.queueChallenger();g.updateChallenger();id=live();assert(id>=0);assert(g.enemies[id].size_scale>256);unsigned parts=0;for(unsigned i=0;i<MAX_BOSS_PARTS;i++)if(g.boss_parts[i].active&&g.boss_parts[i].owner==(unsigned)id){parts++;assert(g.boss_parts[i].source_scale>0);}assert(parts==13&&g.validState());
 setup();g.wave=100;g.challenger_source_=SE_HIVE+1;g.queueChallenger();g.updateChallenger();id=live();unsigned level=g.level,bank=g.banked_upgrades;g.destroyEnemy((unsigned)id);assert(g.level==level+1&&g.banked_upgrades==bank+1);unsigned rewards=0;for(unsigned i=0;i<MAX_PICKUPS;i++)if(g.pickups[i].active&&(g.pickups[i].kind==PICKUP_POWER||g.pickups[i].kind==PICKUP_REROLL))rewards++;assert(rewards==1);g.destroyEnemy((unsigned)id);assert(g.level==level+1);
 setup();for(unsigned i=0;i<MAX_PICKUPS;i++){g.pickups[i].active=1;g.pickups[i].kind=PICKUP_HEAL;g.pickups[i].value=1;}
 unsigned old_rerolls=g.rerolls;Enemy reward={};reward.challenger=1;g.rewardChallenger(reward);
 assert(g.hyperboost_ticks_==450||g.source_armor_ticks_==150||g.source_medi_ticks_==30||g.rerolls==old_rerolls+1);
 setup();g.wave=30;for(unsigned i=0;i<20;i++){g.queueChallenger();g.challenger_delay_=0;g.updateChallenger();int n=live();assert(n>=0);assert(g.enemies[n].type!=E_STAR_EATER&&g.enemies[n].type!=E_CRUCIBLE&&g.enemies[n].type!=E_STATION_OMEGA);g.enemies[n].active=0;memset(g.boss_parts,0,sizeof(g.boss_parts));}assert(g.challenger_spawned_==20);
}
static void sourcePowerups(){
 setup();g.emitPickup(g.player.x,g.player.y,PICKUP_POWER,1);g.updatePickups();assert(g.hyperboost_ticks_==450&&g.hasHyperboost());
 setup();g.emitPickup(g.player.x,g.player.y,PICKUP_POWER,2);g.updatePickups();assert(g.source_armor_ticks_==150&&g.incomingDamagePercent()==86&&g.incomingDamagePercent(true)==86);GameInput none={};for(unsigned i=0;i<150;i++)g.updatePlayer(none);assert(!g.source_armor_ticks_&&g.incomingDamagePercent()==100);
 setup();g.player.hull=10;g.player.shield=10;g.player.shield_delay=500;Game baseline=g;for(unsigned i=0;i<30;i++)baseline.updatePlayer(none);g.emitPickup(g.player.x,g.player.y,PICKUP_POWER,3);g.updatePickups();assert(g.source_medi_ticks_==30&&g.player.hull==10);for(unsigned i=0;i<30;i++)g.updatePlayer(none);assert(g.player.hull==baseline.player.hull+20&&g.player.shield==30&&!g.source_medi_ticks_);
 setup();g.player.hull=10;g.player.shield=0;g.player.shield_delay=500;baseline=g;for(unsigned i=0;i<30;i++)baseline.updatePlayer(none);g.emitPickup(g.player.x,g.player.y,PICKUP_POWER,3);g.updatePickups();for(unsigned i=0;i<30;i++)g.updatePlayer(none);assert(g.player.hull==baseline.player.hull+20&&g.player.shield==0);
}
// Direct source paths: mRancor_Alarm_0, damageReduction, triggerShield,
// and Hidden Power's explicitly permitted shield-gear constructs.
static void defensiveSourceCorrections(){
 setup();g.player.mods[M_BLINK]=1;g.player.x=50*FP;g.player.y=100*FP;g.player.angle=0;
 int behind=g.spawnEnemy(E_DRONE,35,100),ahead=g.spawnEnemy(E_DRONE,130,100);assert(behind>=0&&ahead>=0);
 g.enemies[behind].hull=g.enemies[behind].max_hull=g.enemies[ahead].hull=g.enemies[ahead].max_hull=1000;
 g.enemies[behind].vx=g.enemies[behind].vy=g.enemies[ahead].vx=g.enemies[ahead].vy=0;
 GameInput blink={};blink.ability=true;g.updatePlayer(blink);
 assert(g.enemies[behind].hull==900&&g.enemies[ahead].hull==900);
 assert(g.enemies[behind].vx<0&&g.enemies[ahead].vx>0);
 setup();int thrust=g.stats.thrust;g.player.mods[M_X_RANCOR]=1;g.recalculate();assert(g.stats.thrust==thrust*90/100);
 setup();g.player.mods[M_CHARGED_SHOT]=g.player.mods[M_X_RANCOR]=g.player.mods[M_X_CHARGED_SHIELDS]=1;
 g.player.charge=1;g.player.angle=0;g.player.vx=g.player.vy=0;g.stats.thrust=1400;
 GameInput charging={};charging.up=charging.fire=true;g.updatePlayer(charging);assert(g.player.vx==126);
 setup();g.player.mods[M_AEGIS]=1;GameInput none={},up={};up.up=true;
 for(unsigned i=0;i<75;i++)g.updatePlayer(none);
 assert(g.aegis_clock_==75&&g.incomingDamagePercent()==85);
 g.updatePlayer(up);assert(g.aegis_clock_==60&&g.incomingDamagePercent()==88);
 g.updatePlayer(up);assert(g.aegis_clock_==45&&g.incomingDamagePercent()==91);
 for(unsigned i=0;i<3;i++)g.updatePlayer(up);
 assert(!g.aegis_clock_&&g.incomingDamagePercent()==100);
 setup();g.player.mods[M_ADAPTIVE_ARMOR]=1;
 static const unsigned stacks[]={0,1,5,10,20,30,40,50};
 static const unsigned incoming[]={100,99,95,90,82,74,67,61};
 for(unsigned i=0;i<8;i++){g.armor_stacks_=stacks[i];assert(g.incomingDamagePercent()==incoming[i]);}
 g.player.mods[M_AEGIS]=1;g.aegis_clock_=75;assert(g.incomingDamagePercent()==46);
 g.player.mods[M_OMNI_SHIELD]=1;assert(g.incomingDamagePercent(true)==31);
 g.player.body=B_SENTINEL;assert(g.incomingDamagePercent(true)==25);
 // Initial and repeated hits use the full incoming damage, not absorbed HP.
 setup();g.player.invulnerable=0;g.stats.shield_cooldown=100;
 g.damagePlayer(1);assert(g.player.shield_delay==100);
 g.player.invulnerable=0;g.player.shield_delay=20;g.damagePlayer(10);assert(g.player.shield_delay==50);
 g.player.invulnerable=0;g.player.shield=0;g.player.shield_delay=20;g.damagePlayer(10);assert(g.player.shield_delay==50);
 g.player.invulnerable=0;g.player.shield_delay=20;g.player.mods[M_OMNI_SHIELD]=1;g.damagePlayer(10);assert(g.player.shield_delay==38);
 g.player.invulnerable=0;g.player.shield_delay=95;g.damagePlayer(10);assert(g.player.shield_delay==100);
 g.player.invulnerable=0;g.player.shield_delay=20;g.player.mods[M_X_TEMPEST_BREAK]=1;g.damagePlayer(1);assert(g.player.shield_delay==20);
 g.player.invulnerable=0;g.player.shield_delay=0;g.damagePlayer(1);assert(g.player.shield_delay==100);
 setup();g.player.invulnerable=0;g.player.shield_type=S_BASTION;g.recalculate();g.damagePlayer(1);assert(g.player.shield_delay==0);
 setup();g.wild_mode=true;g.player.shield_type=S_BASTION;g.player.mods[M_DRONES]=1;
 g.player.mods[M_INCENDIARY]=g.player.mods[M_BLAST_RADIUS]=g.player.mods[M_VOLATILE_SHIELDS]=g.player.mods[M_DEADLY_WAKE]=1;g.recalculate();
 int bastion=g.spawnConstruct(C_BASTION),drone=g.spawnConstruct(C_DRONE);assert(bastion>=0&&drone>=0);
 assert(g.takeMod(M_X_HIDDEN_POWER));assert(g.constructs[bastion].active&&!g.constructs[drone].active);
 assert(g.constructLimit(C_BASTION)==1&&!g.constructLimit(C_DRONE));assert(g.spawnConstruct(C_DRONE)<0);
 g.constructs[bastion].active=0;assert(g.spawnConstruct(C_BASTION)>=0);
 assert(g.validState());
 puts("Source Rancor/charge thrust, Hidden Power Bastion, Aegis decay, Adaptive Armor/Omni composition and cooldown, Blink impulse PASS");
}
int main(){defensiveSourceCorrections();homingSourceGoldens();lastStandSourceGoldens();bastionSourceCurve();challengerSourceGoldens();sourcePowerups();puts("Homing Strike source axes/seek/degeneration, Last Stand revival/expansion, Bastion front/follow/fade, Challenger families/scales/retry/reward PASS");}
