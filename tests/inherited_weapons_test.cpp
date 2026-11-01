#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>
#define private public
#include "game.h"
#undef private
using namespace nova;
static Game g;
static void clean(unsigned weapon){g.start(770+weapon);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.player.weapon=(uint8_t)weapon;g.player.mods[M_ALLY]=g.player.mods[M_INTERCEPTOR]=g.player.mods[M_X_OVERPOWER]=g.player.mods[M_OVERSEER]=1;g.recalculate();g.options.auto_deploy_mask=0;}
static void families(){
 static const int kinds[WEAPON_COUNT]={SHOT_BLASTER,SHOT_SPLIT,SHOT_FLAK,SHOT_RAIL,SHOT_GRENADE,SHOT_MISSILE,SHOT_VORTEX,SHOT_SPARK,SHOT_TORRENT,-1,SHOT_DART,-2,-3};
 for(unsigned weapon=0;weapon<WEAPON_COUNT;++weapon){
  clean(weapon);int owner=g.spawnConstruct(C_INTERCEPTOR),target=g.spawnEnemy(E_ASTEROID,75,100);assert(owner>=0&&target>=0);Construct &c=g.constructs[owner];c.x=50*FP;c.y=100*FP;c.vx=c.vy=0;c.angle=0;c.hull=c.max_hull=10000;c.fire_delay=0;g.enemies[target].hull=g.enemies[target].max_hull=100000000;
  g.player.x=250*FP;g.player.y=200*FP;g.player.charge=23;g.player.fire_delay=7;g.player.missiles=19;g.player.missile_clock=29;g.player.angle=39;g.player.vx=47;g.player.vy=-17;
  g.fire_fraction_=57;g.charged_scale_=387123;g.burst_left_=2;g.burst_delay_=3;g.stored_damage_=71;g.calibration_=43;g.vortex_scale_=123456;g.vortex_rate_=654321;g.vortex_growth_fraction_=12345;g.vortex_cost_fraction_=12;g.vortex_damage_fraction_=7;g.rancor_scale_=1234;g.thermal_x_scale_=765432;g.thermal_y_scale_=234567;g.innate_charge_fraction_=19;
  Player before=g.player;Stats stats=g.stats;unsigned matching=0;bool charged=false,damage=false;
  for(unsigned tick=0;tick<220;++tick){++g.frame;if(c.fire_delay)--c.fire_delay;g.updateInterceptorWeapon((unsigned)owner,target);
   assert(memcmp(&before,&g.player,sizeof(before))==0&&memcmp(&stats,&g.stats,sizeof(stats))==0);
   assert(g.fire_fraction_==57&&g.charged_scale_==387123&&g.burst_left_==2&&g.burst_delay_==3&&g.stored_damage_==71&&g.calibration_==43);
   assert(g.vortex_scale_==123456&&g.vortex_rate_==654321&&g.vortex_growth_fraction_==12345&&g.vortex_cost_fraction_==12&&g.vortex_damage_fraction_==7&&g.rancor_scale_==1234&&g.thermal_x_scale_==765432&&g.thermal_y_scale_==234567&&g.innate_charge_fraction_==19);
   charged|=g.advanced_constructs[owner].weapon.charge_ticks>0;damage|=g.enemies[target].hull<g.enemies[target].max_hull;
   for(unsigned b=0;b<FRIENDLY_BULLETS;++b){const Bullet &p=g.bullets[b];if(!p.active)continue;if(kinds[weapon]>=0&&p.kind==kinds[weapon]&&(weapon!=W_BLASTER||!p.blast)){++matching;assert(p.construct_source==owner+1);}g.bullets[b].active=0;}
   for(unsigned e=0;e<MAX_EFFECTS;++e){if(kinds[weapon]==-2&&g.effects[e].active&&g.effects[e].kind==FX_BEAM)++matching;g.effects[e].active=0;}
   if(kinds[weapon]==-1)for(unsigned j=0;j<MAX_CONSTRUCTS;++j)if(g.constructs[j].active&&g.constructs[j].kind==C_BLADE){++matching;g.constructs[j].active=0;}
   if(kinds[weapon]==-3)for(unsigned j=0;j<MAX_CONSTRUCTS;++j)if(g.constructs[j].active&&g.constructs[j].kind==C_SWORD&&g.advanced_constructs[j].wielder==owner+1){++matching;if(g.constructs[j].fire_delay)--g.constructs[j].fire_delay;g.updateAdvancedConstruct(j);}
  }
  if(!matching)fprintf(stderr,"weapon%u failed inherited output\n",weapon);
  assert(matching);
  if(weapon==W_THERMAL_LANCE||weapon==W_VORTEX)assert(charged&&damage);
  if(weapon==W_PULSE)assert(damage);
  assert(g.validState());
 }
}
static void independentStock(){
 clean(W_SALVO);int a=g.spawnConstruct(C_INTERCEPTOR),b=g.spawnConstruct(C_INTERCEPTOR),target=g.spawnEnemy(E_ASTEROID,100,100);assert(a>=0&&b>=0&&target>=0);g.enemies[target].hull=g.enemies[target].max_hull=1000000;
 for(unsigned tick=0;tick<90;++tick){for(int c:{a,b}){if(g.constructs[c].fire_delay)--g.constructs[c].fire_delay;g.updateInterceptorWeapon((unsigned)c,c==a?target:-1);}for(auto &p:g.bullets)p.active=0;}
 assert(g.advanced_constructs[b].weapon.magazine>g.advanced_constructs[a].weapon.magazine);assert(!g.player.missiles);
}
static void chargedIsolation(){
 clean(W_RAILGUN);g.player.mods[M_CHARGED_SHOT]=1;g.recalculate();int a=g.spawnConstruct(C_INTERCEPTOR),b=g.spawnConstruct(C_INTERCEPTOR),target=g.spawnEnemy(E_ASTEROID,100,100);assert(a>=0&&b>=0&&target>=0);
 for(unsigned tick=0;tick<30;++tick){if(g.constructs[a].fire_delay)--g.constructs[a].fire_delay;g.updateInterceptorWeapon((unsigned)a,target);g.updateInterceptorWeapon((unsigned)b,-1);for(auto &p:g.bullets)p.active=0;}
 assert(g.constructs[a].weapon_charge>0&&g.constructs[b].weapon_charge==0&&g.player.charge==0);
 g.constructs[a].fire_delay=0;g.updateInterceptorWeapon((unsigned)a,-1);bool rail=false;for(const auto &p:g.bullets)if(p.active&&p.kind==SHOT_RAIL){rail=true;assert(p.damage>(int)g.weaponDamage());}assert(rail);
}
int main(){families();independentStock();chargedIsolation();printf("Inherited weapons: all13 actual families, isolated player state, two independent Salvo magazines and charge/release PASS (Game%zu, state%zu)\n",sizeof(Game),sizeof(InheritedWeaponState));}
