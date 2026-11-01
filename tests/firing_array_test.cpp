#include <cassert>
#include <cstdio>
#include <cstring>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void setup(unsigned weapon,bool array=true){
 g.start(713);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;
 g.player.weapon=(uint8_t)weapon;g.player.angle=0;g.player.x=100*FP;g.player.y=100*FP;
 g.player.mods[M_FIRING_ARRAY]=array?1:0;g.recalculate();g.options.auto_deploy_mask=0;
}
static void spreadSourceGoldens(){
 // Original degrees rounded only after the 256-angle conversion. This guards
 // against restoring the old zero-spread Dart/Vortex/Railgun table.
 static const unsigned steps[]={7,9,1,9,6,9,14,16,17,9,5,4,1};
 for(unsigned w=0;w<WEAPON_COUNT;++w){setup(w,false);assert(g.weaponSpreadStep()==(int)steps[w]);}
 setup(W_FLAK);assert(g.weaponSpreadStep()==1);
}
static void ordinarySourceLerp(){
 setup(W_RAILGUN,false);g.fireWeapon(1);const Bullet normal=g.bullets[0];
 setup(W_RAILGUN);g.fireWeapon(1);const Bullet single=g.bullets[0];
 // Native ordinary no-arc t=.5: +10% damage/blast, unchanged size.
 assert(single.damage==normal.damage*110/100&&single.size_percent==normal.size_percent&&single.radius==normal.radius);
 assert(single.blast==normal.blast*110/100);
 setup(W_DART);g.fireWeapon(3);
 const int base=(int)g.weaponDamage();
 assert(g.bullets[0].damage==base&&g.bullets[1].damage==base*120/100&&g.bullets[2].damage==base);
 assert(g.bullets[0].size_percent==70&&g.bullets[1].size_percent==130&&g.bullets[2].size_percent==70);
 assert(g.bullets[0].radius<g.bullets[1].radius&&g.bullets[0].angle==uint8_t(0-g.bullets[2].angle));
 // The separate sizeMod survives attachment, affecting actual Dart burn and
 // lifetime, rather than only the visible/collision radius.
 for(unsigned which=0;which<2;++which){
  setup(W_DART);g.fireWeapon(3);Bullet dart=g.bullets[which];
  std::memset(g.bullets,0,sizeof(g.bullets));g.bullets[0]=dart;
  int enemy=g.spawnEnemy(E_DRONE,(dart.x+dart.vx)/FP,(dart.y+dart.vy)/FP);assert(enemy>=0);
  g.enemies[enemy].hull=g.enemies[enemy].max_hull=10000;g.enemies[enemy].radius=10;
  g.updateBullets();assert(g.bullets[0].active&&g.bullets[0].pierce==254);
  assert(g.bullets[0].life==(which?78:42));
  assert(g.bullets[0].damage==dart.damage*dart.size_percent/100);
 }
}
static void vortexSourceAxes(){
 setup(W_VORTEX);g.vortex_scale_=2000000;g.player.charge=30;g.releaseVortex(1);
 assert(g.bullets[0].field_scale==2000000&&g.bullets[0].damage==(int)g.weaponDamage()*1000);
 setup(W_VORTEX);g.vortex_scale_=2000000;g.player.charge=30;g.releaseVortex(3);
 assert(g.bullets[0].field_scale==1400000&&g.bullets[1].field_scale==2600000&&g.bullets[2].field_scale==1400000);
 assert(g.bullets[0].size_percent==140&&g.bullets[1].size_percent==260);
 // Vortex recomputes burn from baseDamage and the altered scale every step;
 // no second damage multiplier is applied to the stored base coefficient.
 for(unsigned i=0;i<3;++i)assert(g.bullets[i].damage==(int)g.weaponDamage()*1000);
 g.updateVortexBullet(g.bullets[0],3);g.updateVortexBullet(g.bullets[1],3);
 assert(g.bullets[0].field_scale<1400000&&g.bullets[1].field_scale>2500000);
 assert(g.validState());
}
static unsigned parry(unsigned hit){
 setup(W_SWORDS);int sword=g.spawnConstruct(C_SWORD);assert(sword>=0);
 Construct &c=g.constructs[sword];AdvancedConstructState &a=g.advanced_constructs[sword];
 c.x=g.player.x;c.y=g.player.y;c.angle=0;c.fire_delay=0;
 a.state=1;a.damage=150;a.heading=0;a.duration=30;
 int incoming=g.emitBullet(c.x,c.y,0,0,(int)hit,SHOT_ENEMY,30,1,2);assert(incoming>=0);
 assert(g.interceptAdvancedProjectile((unsigned)incoming,c.x,c.y));assert(!g.bullets[incoming].active);
 unsigned count=0;for(unsigned i=0;i<FRIENDLY_BULLETS;++i)if(g.bullets[i].active&&g.bullets[i].kind==SHOT_SPARK)++count;
 assert(a.damage==150); // Firing Array affects the sparks, not the blade hit.
 return count;
}
static void swordSparkSourceLerp(){
 assert(parry(1)==1);assert(g.bullets[0].damage==75&&g.bullets[0].size_percent==70);
 unsigned count=parry(100);assert(count>2);
 assert(g.bullets[0].damage==75&&g.bullets[count-1].damage==75);
 assert(g.bullets[0].size_percent==70&&g.bullets[count-1].size_percent==70);
 assert(g.bullets[count/2].damage>75&&g.bullets[count/2].size_percent>100);
 assert(g.validState());
}
int main(){spreadSourceGoldens();ordinarySourceLerp();vortexSourceAxes();swordSparkSourceLerp();std::puts("Firing Array source lerp, 13 original spreads, single-shot fallback, Dart burn/lifetime, Vortex scale and Sword sparks PASS");}
