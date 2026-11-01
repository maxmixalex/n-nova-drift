// Source-derived acceptance scenarios; synthetic fixtures, not a natural run.
#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace nova;
static Game g;
static void reset(){g.start(31);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=220*FP;g.player.y=120*FP;}
static int spawn(unsigned type,unsigned rarity=0){int n=g.spawnEnemy(type,100,120,rarity);assert(n>=0);return n;}
static void ai(int n,unsigned ticks){while(ticks--){++g.enemies[n].age;assert(g.updateRegularEnemy(g.enemies[n]));}}
static unsigned shots(unsigned kind){unsigned n=0;for(const Bullet &b:g.bullets)if(b.active&&b.kind==kind)++n;return n;}
static unsigned enemies(unsigned kind){unsigned n=0;for(const Enemy &e:g.enemies)if(e.active&&e.type==kind)++n;return n;}
int main(){
 // The former generic shot routine had neither the source volley count nor a
 // terminating escape phase. Normal/elite/champion Warden fire once, 1/5/1.
 for(unsigned rarity=0;rarity<3;rarity++){
  reset();int n=spawn(E_WARDEN,rarity);unsigned charge=rarity==2?105:75;ai(n,40+charge-1);assert(!shots(SHOT_WARDEN)&&!shots(SHOT_WARDEN_VORTEX));ai(n,1);
  assert(shots(rarity==2?SHOT_WARDEN_VORTEX:SHOT_WARDEN)==(rarity==1?5u:1u));
  for(const Bullet &b:g.bullets)if(b.active)assert(b.damage==(rarity==2?80:rarity?30:45));
  ai(n,180);assert(!g.enemies[n].active);
 }
 // Sine preserves the entry heading and wavy flight; elite bursts are three
 // shots at ten ticks, not the previous uniformly spaced generic fan.
 reset();int n=spawn(E_SERPENT);uint8_t base=g.enemies[n].angle;ai(n,25);assert(g.enemies[n].angle!=base);ai(n,74);assert(!shots(SHOT_SINE));ai(n,1);assert(shots(SHOT_SINE)==1);
 reset();n=spawn(E_SERPENT,1);ai(n,150);assert(shots(SHOT_SINE)==1);ai(n,10);assert(shots(SHOT_SINE)==2);ai(n,10);assert(shots(SHOT_SINE)==3);ai(n,149);assert(shots(SHOT_SINE)==3);ai(n,1);assert(shots(SHOT_SINE)==4);
 reset();n=spawn(E_SERPENT,2);ai(n,150);assert(shots(SHOT_SINE_MEGA)==1&&shots(SHOT_SINE)==0);
 // Pulsebug has a visible charge and 7/10/15-projectile volley, with a real
 // homing ring only for champion rarity. It is not a stationary mine entity.
 for(unsigned rarity=0;rarity<3;rarity++){
  reset();n=spawn(E_MINE,rarity);g.player.x=120*FP;unsigned charge=20+10*rarity;
  ai(n,charge);assert(!shots(SHOT_PULSEBUG)&&!shots(SHOT_PULSEBUG_HOMING));ai(n,1);
  assert(shots(rarity==2?SHOT_PULSEBUG_HOMING:SHOT_PULSEBUG)==(rarity==2?15u:rarity?10u:7u));
  for(const Bullet &b:g.bullets)if(b.active){assert(b.damage==7);assert(bool(b.homing)==(rarity==2));}
 }
 reset();n=spawn(E_SKIRMISHER);ai(n,43);assert(!shots(SHOT_ENEMY));ai(n,1);assert(shots(SHOT_ENEMY)==4);
 reset();n=spawn(E_SKIRMISHER,1);ai(n,44);assert(shots(SHOT_INTERCEPTOR_MISSILE)==4);
 reset();n=spawn(E_SKIRMISHER,2);ai(n,92);assert(enemies(E_INTERCEPTOR_MINE)>=2);
 // A Station Drone is tied to the real station generation, and its initial
 // 3-shot magazine is followed by 4-shot magazines after an eight-second wait.
 reset();n=spawn(E_DRONE);ai(n,1);assert(!g.enemies[n].active);
 reset();int station=spawn(E_STATION_OMEGA);n=spawn(E_DRONE);ai(n,119);assert(!shots(SHOT_STATION_DRONE));ai(n,1);assert(shots(SHOT_STATION_DRONE)==1);ai(n,30);assert(shots(SHOT_STATION_DRONE)==3);ai(n,239);assert(shots(SHOT_STATION_DRONE)==3);ai(n,1);assert(shots(SHOT_STATION_DRONE)==4);g.enemies[station].generation++;ai(n,1);assert(!g.enemies[n].active);
 // Fractional aura healing preserves 2% of a low-HP enemy per second, while
 // its separate projectile is a shield carrier with a generation-safe target.
 reset();n=spawn(E_CHERUB);int ally=spawn(E_SKIRMISHER);g.enemies[ally].x=108*FP;g.enemies[ally].hull=25;g.enemies[ally].max_hull=50;
 ai(n,30);assert(g.enemies[ally].hull==26);ai(n,22);assert(shots(SHOT_ENEMY_SHIELD)==1);
 for(const Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_ENEMY_SHIELD){assert(b.damage==0&&b.last_hit==ally&&b.size_percent==g.enemies[ally].generation);}
 // Original Standard ancestry includes boss actors. Specific nonstandard
 // explosives cannot steal a cast, and Host Drone explicitly disables shields.
 reset();n=spawn(E_CHERUB);ally=spawn(E_SCION);g.enemies[ally].x=108*FP;ai(n,52);
 assert(shots(SHOT_ENEMY_SHIELD)==1);for(const Bullet &b:g.bullets)if(b.active&&b.kind==SHOT_ENEMY_SHIELD)assert(b.last_hit==ally);
 reset();n=spawn(E_CHERUB);ally=spawn(E_HOST_DRONE);g.enemies[ally].x=108*FP;ai(n,90);assert(!shots(SHOT_ENEMY_SHIELD));
 reset();n=spawn(E_CHERUB);ally=spawn(E_JUGG_MISSILE);g.enemies[ally].x=108*FP;g.enemies[ally].hull=1;int original=g.enemies[ally].hull;ai(n,90);assert(!shots(SHOT_ENEMY_SHIELD)&&g.enemies[ally].hull==original);
 // Native opening recipes have delayed reinforcements rather than simultaneous
 // 2+1 generic enemies. Inspecting queue behavior also catches dropped spawns.
 reset();g.wave=3;g.next_wave=0;g.updateWaves();assert(g.wave==4&&enemies(E_SKIRMISHER)==2&&enemies(E_WARDEN)==0&&g.pendingWaveSpawns()==1);for(unsigned i=0;i<76;i++)g.updateWaveSpawns();assert(enemies(E_WARDEN)==1);
 reset();g.wave=4;g.next_wave=0;g.updateWaves();assert(g.wave==5&&enemies(E_SKIRMISHER)==3&&enemies(E_WARDEN)==1&&g.pendingWaveSpawns()==2);for(unsigned i=0;i<106;i++)g.updateWaveSpawns();assert(enemies(E_SKIRMISHER)==5&&g.pendingWaveSpawns()==0);
 std::puts("PASS flight AI: native volleys, finite Warden, Sine bursts, Pulse charges, Interceptor mines, Station link, Cherub aura/shield, opening delays");
}
