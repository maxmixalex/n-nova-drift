#include "game.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <type_traits>
using namespace nova;
static Game a,b;
static void empty(Game &g,unsigned seed=17) {g.start(seed);g.next_wave=60000;g.player.invulnerable=60000;}
static int target(Game &g,int x=182,int y=120) {
 int i=g.spawnEnemy(E_ASTEROID,x,y);assert(i>=0);g.enemies[i].hull=g.enemies[i].max_hull=10000;
 g.enemies[i].radius=5;g.enemies[i].vx=g.enemies[i].vy=0;return i;
}
static void ticks(Game &g,unsigned count,GameInput in={}) {while(count--){g.tick(in);assert(g.validState());}}
static void grant(Game &g,unsigned id) {
 if(g.player.mods[id])return;
 if(id==M_X_THE_VOID)grant(g,M_X_EVOLUTIONARY_NICHE);
 if(mods[id].prerequisite_a>=0)grant(g,(unsigned)mods[id].prerequisite_a);
 if(mods[id].prerequisite_b>=0)grant(g,(unsigned)mods[id].prerequisite_b);
 assert(g.takeMod(id));
}
static void hostile(Game &g,int x,int y,int vx,int damage=15) {
 Bullet &p=g.bullets[MAX_BULLETS-1];p={};p.active=1;p.owner=1;p.kind=SHOT_ENEMY;p.x=x*FP;p.y=y*FP;p.vx=vx*FP;p.damage=damage;p.radius=2;p.life=120;
}
int main() {
 static_assert(std::is_trivially_default_constructible<Game>::value,"Game must not need global constructors");
 static_assert(std::is_trivially_copyable<Game>::value,"Snapshot persistence needs a POD core");
 assert(dirX(0)==256&&dirY(64)==256&&dirX(128)==-256&&dirY(192)==-256);
 assert(direction(10,0)==0&&direction(0,10)==64&&direction(-10,0)==128&&direction(0,-10)==192);
 empty(a);a.player.x=20*FP;a.player.y=20*FP;a.player.angle=0;GameInput cursor={};cursor.pointer_valid=true;cursor.pointer_x=280;cursor.pointer_y=20;ticks(a,30,cursor);assert(a.player.angle==0);
 cursor.pointer_x=20;cursor.pointer_y=220;ticks(a,30,cursor);assert(a.player.angle==64);
 empty(a);empty(b);GameInput move={};move.up=true;move.right=true;move.fire=true;
 for(unsigned i=0;i<300;i++)a.tick(move);
 for(unsigned i=0;i<200;i++)b.advance(50,move);
 assert(a.frame==300&&b.frame==300&&a.checksum()==b.checksum());
 unsigned f=b.frame;b.advance(10000,move);assert(b.frame==f+6);
 GameInput pause={};pause.pause=true;b.tick(pause);assert(b.state==Pause);f=b.frame;ticks(b,30,pause);assert(b.frame==f);
 b.tick({});b.tick(pause);assert(b.state==Playing);
 empty(a);a.giveExperience(8);assert(a.state==Upgrade&&a.level==2);
 for(unsigned i=0;i<3;i++)assert(a.offers[i].kind==OFFER_WEAPON);
 assert(a.chooseOffer(0));a.giveExperience(a.xp_next);assert(a.offers[0].kind==OFFER_SHIELD);
 assert(a.rerollOffers());assert(a.rerolls==3);assert(a.chooseOffer(1));a.giveExperience(a.xp_next);assert(a.offers[0].kind==OFFER_BODY);
 assert(!a.chooseOffer(4));assert(a.chooseOffer(2));
 for(unsigned m=0;m<MOD_COUNT;m++){empty(a);a.wild_mode=true;
  if(!mods[m].implemented){assert(!a.canTakeMod(m));continue;}
  grant(a,m);assert(a.player.mods[m]);if(mods[m].max_rank==1)assert(!a.takeMod(m));assert(a.validState());}
 empty(a);assert(!a.takeMod(M_FUSILLADE));grant(a,M_ASSAULT_DRONES);assert(!a.canTakeMod(M_DEFENSE_DRONES));
 empty(a);grant(a,M_RAPID_RECONSTRUCTION);assert(a.player.max_shield==0&&!a.equipShield(S_HALO));
 for(unsigned w=0;w<WEAPON_COUNT;w++){
  empty(a);a.player.angle=0;assert(a.equipWeapon(w));int enemy=target(a);
  GameInput fire={};fire.fire=true;ticks(a,90,fire);ticks(a,100);
  assert(a.enemies[enemy].hull<10000);std::printf("weapon %-16s target damage %d\n",weapons[w].name,10000-a.enemies[enemy].hull);
 }
 for(unsigned body=0;body<BODY_COUNT;body++){empty(a);assert(a.equipBody(body));ticks(a,120,move);assert(a.validState());}
 for(unsigned shield=0;shield<SHIELD_COUNT;shield++){empty(a);assert(a.equipShield(shield));a.player.invulnerable=0;a.damagePlayer(300);assert(a.player.shield==0||a.state==GameOver);}
 empty(a);a.equipShield(S_REFLECT);a.player.invulnerable=0;hostile(a,180,120,-2);ticks(a,5);assert(!a.bullets[MAX_BULLETS-1].owner);
 empty(a);a.equipShield(S_WARP);a.player.invulnerable=0;int32_t oldx=a.player.x,oldy=a.player.y;a.damagePlayer(70);assert(a.player.x!=oldx||a.player.y!=oldy);assert(a.player.invulnerable>=35);
 empty(a);a.equipShield(S_HALO);int h=target(a,178);ticks(a,12);assert(a.enemies[h].hull<10000&&a.player.shield<a.player.max_shield);
 empty(a);a.equipWeapon(W_SALVO);ticks(a,120);assert(a.player.missiles>4);
 empty(a);a.equipWeapon(W_VORTEX);GameInput fire={};fire.fire=true;ticks(a,75,fire);assert(a.player.hull<a.player.max_hull&&a.player.charge==75);a.tick({});assert(a.player.charge==0&&a.bulletCount()>0);
 empty(a);grant(a,M_DRONES);grant(a,M_TURRET);grant(a,M_MINES);grant(a,M_ALLY);ticks(a,400);assert(a.constructCount()>=5);
 empty(a);a.equipBody(B_ARCHITECT);grant(a,M_DRONES);grant(a,M_TURRET);ticks(a,400);assert(a.constructCount()==1);
 empty(a);a.equipBody(B_CARRIER);ticks(a,200);assert(a.constructCount()>=3);
 empty(a);grant(a,M_OVERSEER);ticks(a,90,fire);assert(a.constructCount()>0&&a.bulletCount()==0);
 empty(a);a.equipWeapon(W_RAILGUN);a.player.angle=0;int e1=a.spawnEnemy(E_DRONE,190,120),e2=a.spawnEnemy(E_DRONE,205,120);a.enemies[e1].hull=1;a.enemies[e2].hull=1;ticks(a,10,fire);assert(a.kills>=2);
 empty(a);for(unsigned i=0;i<MAX_ENEMIES;i++)assert(a.spawnEnemy(E_ASTEROID,i%320,5)>=0);assert(a.spawnEnemy(E_DRONE,5,5)==-1&&a.enemyCount()==MAX_ENEMIES);
 empty(a);a.player.invulnerable=0;a.damagePlayer(500);assert(a.state==GameOver);GameInput confirm={};confirm.confirm=true;a.tick(confirm);assert(a.state==Playing&&a.player.hull==a.player.max_hull);
 empty(a);grant(a,M_X_ATARAXIA);a.giveExperience(a.xp_next);int damage=a.stats.damage;assert(a.banked_upgrades==1);assert(a.bankUpgrade()&&a.state==Playing);assert(a.reopenUpgrade());assert(a.chooseOffer(0));assert(a.banked_upgrades==0&&a.stats.damage<damage);
 empty(a);a.wild_mode=true;for(unsigned rank=1;rank<=3;rank++){assert(a.takeMod(M_X_FARSIGHT));assert(a.worldWidth()==320+(int)rank*64&&a.worldHeight()==240+(int)rank*48);ticks(a,300,move);}assert(!a.takeMod(M_X_FARSIGHT));
 empty(a);grant(a,M_X_LAST_STAND);a.player.invulnerable=0;a.damagePlayer(1000);assert(a.state==Playing&&a.player.hull>0);a.player.invulnerable=0;a.damagePlayer(1000);assert(a.state==GameOver);
 empty(a);a.wild_mode=true;grant(a,M_X_REVELATION);assert(a.takeMod(M_X_TWIN_STRIKE));assert(a.player.mods[M_X_TWIN_STRIKE]==3&&a.stats.projectiles==3);
 empty(a);a.wild_mode=true;grant(a,M_X_EXPLOSIVE_GROWTH);assert(a.takeMod(M_X_HEAVY_CALIBER));assert(a.player.mods[M_X_HEAVY_CALIBER]==4&&!a.canTakeMod(M_X_HEAVY_CALIBER));
 empty(a);a.wild_mode=true;grant(a,M_X_POWER_SPIKE);assert(a.level==6&&a.banked_upgrades==5&&a.rerolls==0);
 empty(a);grant(a,M_DRONES);ticks(a,100);assert(a.constructCount()>0);grant(a,M_X_HIDDEN_POWER);ticks(a,120,fire);assert(a.bulletCount()==0&&a.constructCount()==0&&!a.canTakeMod(M_TURRET));
 empty(a);a.wild_mode=true;grant(a,M_X_BRAVADO);h=a.spawnEnemy(E_DRONE,20,20);assert(a.enemies[h].elite>=1);
 empty(a);grant(a,M_X_DISPLACEMENT);a.player.x=318*FP;a.player.angle=0;GameInput dash={};dash.ability=true;a.tick(dash);bool unwrapped_beam=false;
 for(unsigned j=0;j<MAX_EFFECTS;j++)if(a.effects[j].active&&a.effects[j].kind==FX_BEAM&&a.effects[j].x2>320*FP)unwrapped_beam=true;
 assert(unwrapped_beam&&a.player.x<100*FP);
 empty(a);a.wild_mode=true;grant(a,M_X_DISCORD);a.player.x=280*FP;target(a,5,120);ticks(a,8);unwrapped_beam=false;
 for(unsigned j=0;j<MAX_EFFECTS;j++)if(a.effects[j].active&&a.effects[j].kind==FX_BEAM&&a.effects[j].x2>320*FP)unwrapped_beam=true;
 assert(unwrapped_beam);
 empty(a);for(unsigned j=0;j<FRIENDLY_BULLETS;j++){Bullet &p=a.bullets[j];p={};p.active=1;p.life=300;p.x=5*FP;p.y=5*FP;p.radius=1;}
 h=a.spawnEnemy(E_SCION,200,40);a.enemies[h].age=31;a.enemies[h].fire_delay=1;a.tick({});unsigned hostile_count=0;
 for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;j++)hostile_count+=a.bullets[j].active&&a.bullets[j].owner;
 assert(hostile_count>=5); // Full friendly pool never suppresses boss attack.
 for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;j++){Bullet &p=a.bullets[j];p={};p.active=1;p.owner=1;p.kind=SHOT_ENEMY;p.life=300;p.age=100;p.x=5*FP;p.y=5*FP;p.radius=1;}
 a.enemies[h].fire_delay=1;a.tick({});unsigned fresh=0;for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;j++)fresh+=a.bullets[j].active&&a.bullets[j].age<=1;assert(fresh>=5);
 // Late build interactions, including reconstructed Super and recursive Wild
 // mechanics, run through the same public simulation API as the launcher.
 const unsigned builds[][8]={
  {M_FUSILLADE,M_X_BARRAGE,M_X_ANNIHILATION,M_X_DOUBLE_TAP,M_X_RICOCHET,M_X_MASTERY,M_X_WARP_STRIKE,M_X_HEAVY_CALIBER},
  {M_OVERSEER,M_X_APEX_MACHINERY,M_X_BURNOUT_REACTORS,M_X_VITAL_BOND,M_X_SPECIALIST_DRONE,M_X_FLEET,M_X_PREDATOR_DRONES,M_X_BATTALION_DRONES},
  {M_MINES,M_X_TACTICAL_BEACON,M_X_CHARGED_MINES,M_X_MORTAR,M_X_SPECIALIST_MINE,M_X_RETRIBUTION,M_X_DEFLAGRATION,M_RUPTURE},
  {M_X_HIDDEN_POWER,M_X_DYING_STAR,M_X_SANCTUARY,M_X_MAELSTROM,M_X_TEMPEST_BREAK,M_X_CHARGED_SHIELDS,M_X_ENERGIZED_SHIELDS,M_REGENERATION},
  {M_X_LANCE,M_X_CELESTIAL_SURGE,M_X_DISPLACEMENT,M_X_STRAFING_STRIKES,M_X_PHANTOM_STRIKE,M_X_RAMPAGE,M_X_SLIPSTREAM,M_X_FARSIGHT},
  {M_GUARDIAN,M_X_NANOTECH_DISSEMINATION,M_X_MEDI_CHARGE,M_X_DECOY_SIGNAL,M_X_SHIELDED_DRONES,M_X_TACTICAL_LINK,M_X_NANOTECH,M_X_EVOLUTION}
 };
 for(unsigned build=0;build<sizeof(builds)/sizeof(builds[0]);build++){
  a.start(500+build);a.wild_mode=true;bool hidden=false;for(unsigned j=0;j<8;j++){if(builds[build][j]==M_X_HIDDEN_POWER)hidden=true;else grant(a,builds[build][j]);}if(hidden)grant(a,M_X_HIDDEN_POWER);a.equipBody(build==1?B_ENGINEER:build==2?B_ARCHITECT:B_SENTINEL);a.equipShield(S_HALO);a.equipWeapon(build==0?W_SALVO:W_RAILGUN);
  for(unsigned t=0;t<3600&&a.state!=GameOver;t++){GameInput in={};in.fire=t%120<90;in.up=t%80<50;in.down=t%80>=70;in.right=t%100<60;in.ability=t%100==0;if(a.state==Upgrade)a.chooseOffer(0);a.tick(in);assert(a.validState());}
 }
 // Deterministic evolving runs exercise deaths, collection, upgrades and
 // fixed pools without invulnerability or direct experience injection.
 a.start(737);b.start(737);unsigned deaths=0,upgrades=0,total_kills=0;
 for(unsigned t=0;t<24000;t++){
  GameInput in={};in.fire=true;in.up=t%90<65;in.right=t%160<90;in.ability=t%150==0;
  if(a.state==Upgrade){assert(b.state==Upgrade);a.chooseOffer(0);b.chooseOffer(0);++upgrades;}
  if(a.state==GameOver){++deaths;total_kills+=a.kills;a.start(737+deaths);b.start(737+deaths);}
  a.tick(in);b.tick(in);assert(a.checksum()==b.checksum());assert(a.validState());
 }
 assert(upgrades>5&&deaths>0&&total_kills>10);
 std::printf("PASS Game=%zu bytes, deterministic 24000-tick runs: %u upgrades, %u deaths, %u kills\n",sizeof(Game),upgrades,deaths,total_kills);
}
