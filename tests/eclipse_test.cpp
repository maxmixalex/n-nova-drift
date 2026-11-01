#include <cassert>
#include <cstdio>
#include <cstring>
#define private public
#include "game.h"
#undef private
using namespace nova;
static void setup(Game& g){g.start(84511);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=250*FP;g.player.y=200*FP;}
static int part(const Game& g,unsigned owner,unsigned kind,unsigned slot=255){for(unsigned i=0;i<MAX_BOSS_PARTS;++i){const BossPart& p=g.boss_parts[i];if(p.active&&p.owner==owner&&p.kind==kind&&(slot==255||p.slot==slot))return int(i);}return -1;}
static unsigned parts(const Game& g,unsigned owner){unsigned n=0;for(const BossPart& p:g.boss_parts)n+=p.active&&p.owner==owner;return n;}
static unsigned bullets(const Game& g,unsigned kind){unsigned n=0;for(const Bullet& b:g.bullets)n+=b.active&&b.kind==kind;return n;}
static void ai(Game& g,unsigned owner,unsigned ticks){for(unsigned i=0;i<ticks&&g.enemies[owner].active;++i){++g.frame;++g.enemies[owner].age;assert(g.updateRegularEnemy(g.enemies[owner]));}}
static void structure(){
 for(unsigned rarity=0;rarity<3;++rarity){Game g={};setup(g);int owner=g.spawnEnemy(E_ECLIPSE,140,100,rarity);assert(owner>=0&&parts(g,owner)==5);Enemy& e=g.enemies[owner];int shield=part(g,owner,BP_ECLIPSE_SHIELD),chassis=part(g,owner,BP_ECLIPSE_CHASSIS,0),other=part(g,owner,BP_ECLIPSE_CHASSIS,1),panel=part(g,owner,BP_ECLIPSE_PANEL,2);
  assert(g.boss_parts[shield].max_hull==(int64_t)e.max_hull*1900/1200);assert(g.boss_parts[chassis].max_hull==(int64_t)e.max_hull*3200/1200);assert(g.boss_parts[panel].max_hull==(int64_t)e.max_hull*1800/1200);assert(g.nearestTarget(e.x,e.y)==MAX_ENEMIES+shield);
  assert(!g.sweptBossPart(g.boss_parts[shield],e.x+g.boss_parts[shield].radius*FP,e.y+g.boss_parts[shield].radius*FP,e.x+g.boss_parts[shield].radius*FP,e.y+g.boss_parts[shield].radius*FP,0));
  int hp=e.hull;int c_hp=g.boss_parts[chassis].hull;int b=g.emitBullet(e.x-60*FP,e.y,0,50,100,SHOT_BLASTER,20);assert(b>=0);g.bullets[b].vx=g.bullets[b].speed=50*FP;g.buildGrid();g.updateBullets();assert(g.boss_parts[shield].hull==g.boss_parts[shield].max_hull-100&&e.hull==hp&&g.boss_parts[chassis].hull==c_hp);
  // hardShielded is targeting/obstruction state, not general damage immunity.
  g.hitBossPart(panel,1,false,true);assert(g.boss_parts[panel].hull==g.boss_parts[panel].max_hull-1&&e.hull==hp);
  g.hitBossPart(shield,10000000);assert(g.eclipseShield(owner)<0&&parts(g,owner)==4&&e.hull==hp);
  assert(bullets(g,SHOT_ECLIPSE_BURST)==(rarity?1u:0u));
  g.hitBossPart(chassis,1);assert(e.hull==hp);g.hitBossPart(chassis,10000000);assert(e.active&&parts(g,owner)==3);
  g.hitBossPart(other,10000000);assert(!e.active&&parts(g,owner)==0&&g.validState());
 }
 Game g={};setup(g);for(unsigned i=0;i<MAX_BOSS_PARTS-4;++i)g.boss_parts[i].max_hull=1;assert(g.spawnEnemy(E_ECLIPSE,130,90)<0);g.boss_parts[0].max_hull=0;assert(g.spawnEnemy(E_ECLIPSE,130,90)>=0);
}
static void schedules(){
 for(unsigned rarity=0;rarity<3;++rarity){Game g={};setup(g);int owner=g.spawnEnemy(E_ECLIPSE,140,100,rarity);Enemy& e=g.enemies[owner];ai(g,owner,174);assert(!e.phase&&bullets(g,SHOT_ECLIPSE_BOLT)==0);ai(g,owner,1);assert(e.phase&&e.phase_timer==180);
  int closed=(30+5*(int)rarity)*256;int ch=part(g,owner,BP_ECLIPSE_CHASSIS,0);ai(g,owner,30);assert(g.boss_parts[ch].local_x>closed);assert(bullets(g,SHOT_ECLIPSE_TORRENT)==0);
  ai(g,owner,60);assert(bullets(g,SHOT_ECLIPSE_BOLT)==4+rarity&&!e.phase);
  ai(g,owner,88);assert(bullets(g,SHOT_ECLIPSE_ORB)==2);for(const Bullet& b:g.bullets)if(b.active&&b.kind==SHOT_ECLIPSE_ORB)assert(b.damage==40&&b.speed==5*FP/3);
  // A destroyed panel suppresses its own orb and both torrent emitters.
  int panel=part(g,owner,BP_ECLIPSE_PANEL,2);g.hitBossPart(panel,10000000);std::memset(g.bullets,0,sizeof(g.bullets));e.target_y=1;e.action_timer=60000;e.fire_delay=60000;ai(g,owner,1);assert(bullets(g,SHOT_ECLIPSE_ORB)==1);
  int shield=part(g,owner,BP_ECLIPSE_SHIELD);g.hitBossPart(shield,10000000);std::memset(g.bullets,0,sizeof(g.bullets));e.attack_angle=1;e.phase_age=100;e.action_count=0;e.fire_delay=1;ai(g,owner,1);assert(bullets(g,SHOT_ECLIPSE_TORRENT)==0);
  e.action_count=1;e.fire_delay=1;ai(g,owner,1);assert(bullets(g,SHOT_ECLIPSE_TORRENT)==2);assert(g.validState());
 }
}
static void projectiles(){
 Game g={};setup(g);g.player.invulnerable=0;g.player.hull=g.player.max_hull=1000;g.player.shield=0;int at=g.emitBullet(100*FP,100*FP,0,1,40,SHOT_ECLIPSE_ORB,30,1,8);g.bullets[at].tracking_weapon=2;g.bullets[at].speed=0;g.bullets[at].vx=g.bullets[at].vy=0;g.destroyHostileBullet(at);assert(bullets(g,SHOT_ECLIPSE_VORTEX)==1);int field=-1;for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;++i)if(g.bullets[i].active)field=(int)i;assert(field>=0);
  Bullet& b=g.bullets[field];g.player.x=b.x;g.player.y=b.y;for(unsigned i=0;i<30;++i){++g.frame;g.updateBullets();}assert(g.player.hull==940&&b.field_scale==204000&&b.active);g.player.invulnerable=20;g.updateBullets();assert(g.player.hull==940);for(unsigned i=0;i<310;++i)g.updateBullets();assert(!b.active);
 setup(g);int owner=g.spawnEnemy(E_ECLIPSE,140,100,2);int shield=part(g,owner,BP_ECLIPSE_SHIELD);g.hitBossPart(shield,1);assert(bullets(g,SHOT_ECLIPSE_REFLECT)==1&&bullets(g,SHOT_ECLIPSE_SHOT)==1);g.hitBossPart(shield,1);assert(bullets(g,SHOT_ECLIPSE_REFLECT)==1);g.updateBossParts(owner);g.updateBossParts(owner);g.hitBossPart(shield,1);assert(bullets(g,SHOT_ECLIPSE_REFLECT)==2);
 std::memset(g.bullets,0,sizeof(g.bullets));g.triggerEclipseShield(shield,true);int blast=-1;for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;++i)if(g.bullets[i].active)blast=(int)i;assert(blast>=0);g.player.x=g.bullets[blast].x;g.player.y=g.bullets[blast].y;g.player.hull=g.player.max_hull=1000;g.player.shield=0;g.player.invulnerable=0;
 for(unsigned i=0;i<10;++i){++g.frame;g.updateBullets();}assert(g.player.hull==960);assert(g.validState());
}
static void contactBurst(){
 for(unsigned rarity=0;rarity<3;++rarity){Game g={};setup(g);int owner=g.spawnEnemy(E_ECLIPSE,140,100,rarity);Enemy& e=g.enemies[owner];int shield=part(g,owner,BP_ECLIPSE_SHIELD);BossPart& p=g.boss_parts[shield];
  e.angle=0;e.formation_angle=0;g.player.x=p.x+(p.radius+g.player.radius-2)*FP;g.player.y=p.y;g.player.invulnerable=0;g.player.shield=0;g.player.hull=g.player.max_hull=1000;int shield_hp=p.hull;
  g.updateEnemies();assert(bullets(g,SHOT_ECLIPSE_BURST)==1&&p.hull==shield_hp&&p.local_y==4);assert(bullets(g,SHOT_ECLIPSE_REFLECT)==0);
  // Isolate the emitted burst from separate chassis/panel crash contacts.
  for(const Bullet& b:g.bullets)if(b.active&&b.kind==SHOT_ECLIPSE_BURST){assert(b.damage==20*(int)(rarity+1));g.player.x=b.x;g.player.y=b.y;}
  g.player.hull=1000;g.player.invulnerable=0;g.buildGrid();g.updateBullets();assert(g.player.hull==1000-20*(int)(rarity+1));
  g.updateEnemies();assert(bullets(g,SHOT_ECLIPSE_BURST)==1&&p.local_y==2);g.updateEnemies();assert(bullets(g,SHOT_ECLIPSE_BURST)==2&&p.local_y==4);assert(p.hull==shield_hp&&g.validState());
 }
}
int main(){structure();schedules();projectiles();contactBurst();std::printf("Eclipse native5-part hull, shield collision, dual chassis death, panel weapon gates,350/525+180 schedules, shield counter,20/40/60 contact burst and orb field PASS (Game%zu)\n",sizeof(Game));}
