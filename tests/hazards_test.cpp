#include <cassert>
#include <cstdio>
#include <cstring>
#define private public
#include "game.h"
#undef private
using namespace nova;
static void empty(Game &g){g.start(0x485A5244);std::memset(g.enemies,0,sizeof(g.enemies));std::memset(g.boss_parts,0,sizeof(g.boss_parts));std::memset(g.bullets,0,sizeof(g.bullets));std::memset(g.constructs,0,sizeof(g.constructs));g.next_wave=60000;g.wave=30;g.player.hull=g.player.max_hull=10000;g.player.shield=0;g.player.invulnerable=0;g.player.x=160*FP;g.player.y=120*FP;g.options.auto_deploy_mask=0;}
static unsigned countEnemy(const Game &g,unsigned kind){unsigned count=0;for(const Enemy &e:g.enemies)count+=e.active&&e.type==kind;return count;}
static unsigned countBullet(const Game &g,unsigned kind){unsigned count=0;for(const Bullet &b:g.bullets)count+=b.active&&b.kind==kind;return count;}
static void ai(Game &g,unsigned index,unsigned ticks){for(unsigned i=0;i<ticks&&g.enemies[index].active;i++){++g.frame;++g.enemies[index].age;assert(g.updateRegularEnemy(g.enemies[index]));}}
static void minelayer(){
 for(unsigned rarity=0;rarity<3;rarity++){Game g;empty(g);int n=g.spawnEnemy(E_MINELAYER,70,70,rarity);ai(g,n,rarity==1?207:75);assert(countEnemy(g,E_MINE_SHIELD)==1);assert(countEnemy(g,E_MINE)==0);
  if(rarity==2)assert(countEnemy(g,E_MINE_DRONE)==2);else assert(countEnemy(g,rarity?E_SINGULARITY_MINE:E_STEALTH_MINE)==1);
  int shield=-1;for(unsigned j=0;j<MAX_ENEMIES;j++)if(g.enemies[j].active&&g.enemies[j].type==E_MINE_SHIELD)shield=(int)j;
  int before=g.enemies[n].hull;g.hitEnemy(n,100);assert(g.enemies[n].hull==before&&g.enemies[shield].hull==1900);
  g.hitEnemy((unsigned)shield,100000);assert(!g.enemies[shield].active);unsigned mines=countEnemy(g,E_STEALTH_MINE);g.hitEnemy(n,100000);assert(!g.enemies[n].active||g.enemies[n].type==E_STEALTH_MINE);assert(countEnemy(g,E_STEALTH_MINE)==mines+3+3*rarity);assert(countEnemy(g,E_MINE_DRONE)==0);
 }
 Game g;empty(g);int n=g.spawnEnemy(E_INTERCEPTOR_MINE,160,120);ai(g,n,9);assert(g.player.hull==10000&&g.enemies[n].active);ai(g,n,1);assert(g.player.hull==9970&&!g.enemies[n].active);
 empty(g);n=g.spawnEnemy(E_STEALTH_MINE,160,120);g.hitEnemy(n,100);assert(g.player.hull==10000); // Unarmed destruction never blasts.
 empty(g);n=g.spawnEnemy(E_SINGULARITY_MINE,140,120);ai(g,n,10);assert(g.enemies[n].phase==1);g.player.vx=0;ai(g,n,1);assert(g.player.vx<0);g.enemies[n].x=g.player.x;g.enemies[n].y=g.player.y;ai(g,n,1);assert(g.player.hull==9950);
}
static void bolt(){
 for(unsigned rarity=0;rarity<3;rarity++){Game g;empty(g);int n=g.spawnEnemy(E_BOLT_THROWER,80,80,rarity);Enemy &e=g.enemies[n];e.form=1;e.target_x=0;e.attack_angle=0;ai(g,n,130);assert(countBullet(g,rarity==2?SHOT_BOLT_CHAMPION:SHOT_BOLT)==(rarity==1?6u:rarity==2?5u:1u));unsigned shots=g.bulletCount();ai(g,n,180);assert(!e.active);assert(g.bulletCount()==shots);}
}
static void boltDeparture(){
 for(unsigned rarity=1;rarity<3;++rarity){
  Game g;empty(g);int n=g.spawnEnemy(E_BOLT_THROWER,80,80,rarity);Enemy &e=g.enemies[n];
  e.age=1;g.updateBossParts((unsigned)n);unsigned parts=0;for(const BossPart &p:g.boss_parts)parts+=p.active&&p.owner==n;assert(parts==2);
  e.form=1;e.age=200;e.attack_angle=0;e.target_x=64*FP*3;e.x=(g.worldWidth()-1)*FP;
  const unsigned score=g.score,kills=g.kills;Effect effects[MAX_EFFECTS];std::memcpy(effects,g.effects,sizeof(effects));assert(g.updateRegularEnemy(e));assert(!e.active);
  for(const BossPart &p:g.boss_parts)assert(!p.max_hull||p.owner!=n);
  for(const Pickup &p:g.pickups)assert(!p.active);
  assert(std::memcmp(effects,g.effects,sizeof(effects))==0);
  assert(g.score==score&&g.kills==kills&&g.validState());
 }
}
static void beam(){
 Game g;empty(g);g.player.x=160*FP;int n=g.spawnEnemy(E_BEAMCASTER,20,120);Enemy &e=g.enemies[n];e.form=1;e.phase=2;e.phase_timer=31;e.angle=e.attack_angle=0;e.vx=e.vy=0;ai(g,n,30);assert(g.player.hull==9973);assert(g.lineHit(20*FP,120*FP,420*FP,120*FP,160*FP,120*FP,2));
 empty(g);n=g.spawnEnemy(E_BEAMCASTER,20,120,2);Enemy &c=g.enemies[n];c.form=1;c.phase=2;c.phase_timer=31;c.angle=c.attack_angle=0;ai(g,n,30);assert(countBullet(g,SHOT_BEAMCASTER)==30);for(const Bullet &b:g.bullets)if(b.active)assert(b.damage==16);
}
static void shieldCarrier(){
 Game g;empty(g);int target=g.spawnEnemy(E_SWARMLING,180,120);Enemy &a=g.enemies[target];a.elite=2;a.hull=a.max_hull=100;int b=g.emitBullet(a.x,a.y,0,1,0,SHOT_ENEMY_SHIELD,150,1,3);g.bullets[b].damage=0;g.bullets[b].last_hit=(int16_t)target;g.bullets[b].size_percent=a.generation;g.updateBullets();assert(a.hard_shield==100&&a.hard_shield_timer==180&&g.player.hull==10000);g.hitEnemy(target,60);assert(a.hard_shield==40&&a.hull==100);g.hitEnemy(target,60);assert(a.hard_shield==0&&a.hull==80);
 empty(g);target=g.spawnEnemy(E_SWARMLING,180,120);b=g.emitBullet(g.player.x,g.player.y,0,1,0,SHOT_ENEMY_SHIELD,150,1,3);g.bullets[b].last_hit=(int16_t)target;g.bullets[b].size_percent=g.enemies[target].generation+1;g.updateBullets();assert(!g.bullets[b].active&&g.player.hull==10000&&!g.enemies[target].hard_shield);
}
static void projectiles(){
 Game g;empty(g);int b=g.emitBullet(40*FP,40*FP,0,10,7,SHOT_PULSEBUG,24,1,2);unsigned speed=g.bullets[b].speed;g.updateBullets();assert(g.bullets[b].speed==(uint64_t)speed*950625/1000000);for(unsigned i=0;i<24;i++)g.updateBullets();assert(!g.bullets[b].active);
 empty(g);b=g.emitBullet(40*FP,40*FP,0,2,10,SHOT_INTERCEPTOR_MISSILE,120,1,2);speed=g.bullets[b].speed;g.updateBullets();assert(g.bullets[b].speed==speed+26);
 empty(g);g.player.x=220*FP;g.player.y=200*FP;int ball=g.emitBullet(100*FP,100*FP,0,1,1,SHOT_TRACER_BALL,100,1,6);g.bullets[ball].homing=2;g.bullets[ball].blast=50;g.bullets[ball].vx=g.bullets[ball].vy=0;int ray=g.emitBullet(90*FP,100*FP,0,10,12,SHOT_TRACER,80,1,2);(void)ray;g.player.invulnerable=100;g.updateBullets();assert(countBullet(g,SHOT_TRACER_BALL)==8&&countBullet(g,SHOT_TRACER)==4);for(const Bullet &p:g.bullets)if(p.active&&p.kind==SHOT_TRACER)assert(p.damage==6&&p.wraps==1);assert(g.validState());
 empty(g);b=g.emitBullet(g.player.x,g.player.y,0,1,100,SHOT_TRACER_WAKE,45,1,3);g.bullets[b].vx=g.bullets[b].vy=0;for(unsigned i=0;i<30;i++){++g.frame;g.updateBullets();}assert(g.player.hull==9900);
}
static void targets(){
 Game g;empty(g);int boss=g.spawnEnemy(E_STATION_OMEGA,160,120);int target=g.nearestTarget(160*FP,120*FP);assert(target>=MAX_ENEMIES);int before=g.enemies[boss].hull;g.hitEnemy(boss,500);assert(g.enemies[boss].hull==before);int hp=g.targetHull(target);g.damageTarget(target,100);assert(g.targetHull(target)<hp&&g.enemies[boss].hull<before);
 assert(ENEMY_COUNT==65);empty(g);int last=g.spawnEnemy(E_JUGG_MISSILE_HOMING,30,30);assert(last>=0&&(g.run_stats.seen_enemies&(uint64_t(1)<<63)));assert(g.enemies[last].hull==60&&g.enemies[last].max_hull==90);int extra=g.spawnEnemy(E_CONSTRUCTOR_MISSILE,60,60);assert(extra>=0&&g.enemySeen(64)&&g.run_stats.seen_enemies_hi==1&&!g.enemySeen(65));assert(g.validState());
}
int main(){minelayer();bolt();boltDeparture();beam();shieldCarrier();projectiles();targets();std::printf("PASS native hazards: 3 Minelayers/Bolts, continuous beam/stream, armed mines, hard shield, decay/homing, Tracer self-combo/burn, physical target routing; Game=%zu\n",sizeof(Game));}
