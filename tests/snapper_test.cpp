#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
using namespace nova;
static void setup(Game& g){g.start(7107);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=230*FP;g.player.y=190*FP;}
static unsigned parts(const Game& g,unsigned owner,bool active=true){unsigned n=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i){const BossPart& p=g.boss_parts[i];n+=p.owner==owner&&(active?p.active!=0:p.max_hull!=0);}return n;}
static int part(const Game& g,unsigned owner,unsigned kind,unsigned slot=255){for(unsigned i=0;i<MAX_BOSS_PARTS;++i){const BossPart& p=g.boss_parts[i];if(p.owner==owner&&p.kind==kind&&p.max_hull&&(slot==255||p.slot==slot))return int(i);}return -1;}
int main(){
 for(unsigned rarity=0;rarity<3;++rarity){
  Game g={};setup(g);int owner=g.spawnEnemy(E_SNAPPER,145,100,rarity);assert(owner>=0);Enemy& e=g.enemies[owner];e.angle=0;
  unsigned segments=rarity>=2?12:rarity?10:8,total=2+(rarity?5:3)*(segments+1);
  assert(parts(g,owner,false)==total);assert(parts(g,owner)==0);
  for(unsigned t=0;t<10;++t){g.tick({});}
  assert(parts(g,owner)==total);assert(g.validState());
  int jaw=part(g,owner,BP_SNAPPER_JAW_LEFT),base=part(g,owner,BP_SNAPPER_BASE,0),middle=part(g,owner,BP_SNAPPER_SEGMENT,3);
  assert(jaw>=0&&base>=0&&middle>=0);assert(g.boss_parts[jaw].max_hull==e.max_hull*3);assert(g.boss_parts[base].max_hull==e.max_hull);assert(g.boss_parts[middle].max_hull==e.max_hull*120/500);
  int head=e.hull;g.hitBossPart(middle,1);assert(e.hull==head);assert(g.boss_parts[middle].hull==g.boss_parts[middle].max_hull-1);
  unsigned score=g.score,kills=g.kills;g.hitBossPart(middle,1000000);g.tick({});assert(e.active&&e.hull==head&&g.score==score&&g.kills==kills);
  for(unsigned depth=3;depth<=segments;++depth){int at=part(g,owner,BP_SNAPPER_SEGMENT,depth);assert(at>=0&&!g.boss_parts[at].active);}
  assert(parts(g,owner)==total-(segments-2));
  e.phase=1;int charge=e.target_y;g.hitBossPart(jaw,1000000);assert(e.active&&e.hull==head&&e.target_y==charge/2&&e.phase==0&&e.formation_spacing==4000&&e.formation_count==1);
  charge=e.target_y;e.phase=1;g.hitBossPart(base,1000000);assert(e.active&&e.target_y==charge*4/5&&e.phase==0&&e.action_count==0);
  for(unsigned t=0;t<20;++t){g.tick({});}
  assert(e.formation_spacing==1000);assert(g.validState());
  g.hitEnemy(owner,100000000);assert(!e.active&&parts(g,owner,false)==0);
  std::printf("Snapper rarity%u full%u independentHP, middle break, jaw/base cripple, head death PASS\n",rarity,total);
 }
 // Full ownership is reserved up front: a new champion cannot silently lose
 // its final chain because another entity consumed its delayed growth slots.
 {Game g={};setup(g);assert(g.spawnEnemy(E_SNAPPER,100,100,2)>=0);assert(g.spawnEnemy(E_SNAPPER,200,100,2)>=0);assert(g.spawnEnemy(E_SNAPPER,150,100,2)<0);assert(g.spawnEnemy(E_SNAPPER,150,100,0)>=0);}
 for(unsigned rarity=0;rarity<3;++rarity){Game g={};setup(g);int owner=g.spawnEnemy(E_SNAPPER,145,100,rarity);assert(owner>=0);unsigned spines=0,beams=0;
  for(unsigned tick=0;tick<440;++tick){g.tick({});for(unsigned b=FRIENDLY_BULLETS;b<MAX_BULLETS;++b)if(g.bullets[b].active&&g.bullets[b].age==1){if(g.bullets[b].kind==SHOT_WORM_SPINE){++spines;assert(g.bullets[b].damage==24);assert(g.bullets[b].speed==40*FP/3);}if(g.bullets[b].kind==SHOT_SNAPPER_BEAM)++beams;}assert(g.validState());}
  assert(rarity==2?(spines>=7&&spines%7==0&&beams>0):(!spines&&!beams));std::printf("Snapper rarity%u champion spine%u beam%u PASS\n",rarity,spines,beams);
 }
 {Game g={};setup(g);int owner=g.spawnEnemy(E_SNAPPER,100,100,2);assert(owner>=0);g.tick({});Enemy& e=g.enemies[owner];e.angle=0;e.formation_angle=0;e.phase=1;e.target_x=e.target_y=0;e.summon_timer=1;e.action_timer=60000;g.updateRegularEnemy(e);
  int at=-1;for(unsigned b=FRIENDLY_BULLETS;b<MAX_BULLETS;++b)if(g.bullets[b].active&&g.bullets[b].kind==SHOT_SNAPPER_BEAM)at=(int)b;assert(at>=0);
  Bullet& beam=g.bullets[at];beam.field_scale=900000;beam.tracking_scale_y=3000;beam.age=30;
  g.player.x=e.x+20*FP;g.player.y=e.y;g.player.hull=g.player.max_hull=1000;g.player.shield=0;g.player.invulnerable=0;
  unsigned scale=beam.field_scale;for(unsigned i=0;i<4;++i)g.updateBullets();assert(g.player.hull==988&&beam.active&&beam.field_scale>scale);
  e.x+=3*FP;g.player.x=e.x+20*FP;g.updateBullets();assert(beam.x==e.x&&g.player.hull==985);
  beam.age=91;scale=beam.field_scale;unsigned height=beam.tracking_scale_y;g.updateBullets();assert(beam.field_scale==scale&&beam.tracking_scale_y<height);
  e.active=0;g.updateBullets();assert(!beam.active);
 }
 std::printf("Snapper original parts, source schedules, continuous champion beam and atomic capacity PASS (Game%zu)\n",sizeof(Game));
}
