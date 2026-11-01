// Synthetic platform fixtures, not naturally achieved runs. Build against the
// exact runtime revision before transferring: Game is a versioned POD snapshot.
#define private public
#include "game.h"
#undef private
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace nova;
static Game game;
static uint32_t crc32(const void* ptr,unsigned size){const uint8_t* p=(const uint8_t*)ptr;uint32_t c=~0u;for(unsigned i=0;i<size;i++){c^=p[i];for(unsigned b=0;b<8;b++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static unsigned parts(){unsigned n=0;for(const BossPart& p:game.boss_parts)n+=p.active;return n;}
static void setup(){game.start(0x51413535u);game.configureCareer(64,true);game.configureRun(RUN_ENDLESS);game.wave=120;game.next_wave=60000;game.xp_next=1000000;game.player.invulnerable=60000;game.options.auto_deploy_mask=15;}
static void heavy(){
 setup();assert(game.spawnEnemy(E_WORM,40,65,2)>=0);assert(game.spawnEnemy(E_WORM,270,175,2)>=0);
 int star=game.spawnEnemy(E_STAR_EATER,160,70);assert(star>=0);game.enemies[star].age=100;game.updateBossParts((unsigned)star);
 assert(game.spawnEnemy(E_BULWARK,70,170,2)>=0);assert(game.spawnEnemy(E_BULWARK,255,60,2)>=0);
 assert(parts()==187);unsigned enemies=game.enemyCount();assert(game.spawnEnemy(E_BULWARK,140,160,2)<0&&game.enemyCount()==enemies&&parts()==187);
 game.player.x=160*FP;game.player.y=190*FP;game.auto_fire=true;
}
static void effects(){
 setup();game.wave=100;game.equipBody(B_ENGINEER);game.equipWeapon(W_SWORDS);game.equipShield(S_BASTION);
 const unsigned mods[]={M_DISCHARGE,M_X_TEMPEST_BREAK,M_X_SCORCHING_WAKE,M_X_MAELSTROM,M_DRONES,M_ASSAULT_DRONES,M_ALLY,M_INTERCEPTOR,M_X_OVERPOWER,M_MINES,M_LOADED_MINES};
 for(unsigned m:mods){game.player.mods[m]=1;}
 game.player.mods[M_X_SCORCHING_WAKE]=3;game.recalculate();
 for(unsigned tick=0;tick<180;tick++){++game.frame;game.updateConstructs();}
 // A large released Vortex remains independent after switching to Swords.
 // This intentionally mixes systems to measure their fixed-pool cost.
 game.player.weapon=W_VORTEX;game.player.charge=120;game.vortex_scale_=20000000;game.releaseVortex(1,true);game.player.weapon=W_SWORDS;game.recalculate();
 game.player.thrusting=true;game.triggerScorchingWake();game.triggerTempest(game.player.x,game.player.y,100);game.triggerDischarge();
 game.player.shield_energy=80;game.releaseBastionEnergy(1);game.attackSwords(150,25,game.player.angle,false);
 for(unsigned i=0;i<20;i++){int enemy=game.spawnEnemy(i<3?E_PULSAR_MINE:E_ASTEROID,20+(i%5)*58,20+(i/5)*55,i<3?2:0);assert(enemy>=0);
  // Preserve targets during the mixed field benchmark; artificial HP is
  // explicitly confined to this synthetic pressure fixture.
  game.enemies[enemy].hull=game.enemies[enemy].max_hull=1000000;
  if(game.enemies[enemy].type==E_PULSAR_MINE){game.enemies[enemy].age=1;game.enemies[enemy].form=1;game.enemies[enemy].phase_timer=1;}
 }
 game.auto_fire=true;
}
static void victory(){
 setup();game.configureRun(0);int owner=game.spawnEnemy(E_STAR_EATER,180,70);assert(owner>=0);Enemy& boss=game.enemies[owner];boss.age=1000;boss.form=2;boss.phase=0;boss.phase_timer=1000;
 game.tick({});boss.hull=1;boss.boss_hidden=0;int chosen=-1;
 for(unsigned i=0;i<MAX_BOSS_PARTS;i++)if(game.boss_parts[i].active){if(chosen<0)chosen=(int)i;else game.boss_parts[i].active=0;}
 assert(chosen>=0);BossPart& part=game.boss_parts[chosen];part.hull=part.max_hull=1;for(Bullet& b:game.bullets)b.active=0;
 Bullet& b=game.bullets[0];b={};b.active=1;b.kind=SHOT_RAIL;b.x=part.x;b.y=part.y;b.radius=50;b.damage=10000;b.life=100;b.last_hit=-1;
 Game probe=game;probe.tick({});assert(probe.victory&&probe.state==GameOver);
}
static void upgrade(){
 game.start(0x55495033u);game.configureCareer(64,true);game.configureRun(0);
 game.next_wave=60000;game.player.invulnerable=60000;game.options.auto_deploy_mask=15;
 // Finish the real initial gear sequence, so the test reaches the seven-offer
 // mod screen, gear replacement and Research banish rather than a fake offer.
 for(unsigned i=0;i<3;++i){game.giveExperience(game.xp_next);assert(game.state==Upgrade&&game.chooseOffer(0));}
 assert(game.gearSelectionComplete());assert(game.equipBody(B_RESEARCH));
 game.giveExperience(game.xp_next);assert(game.state==Upgrade&&game.offer_count==OFFER_COUNT);
 game.banked_upgrades=2;game.rerolls=12;game.score=12345;game.wave=12;
 assert(game.bankUpgrade()&&game.state==Playing&&game.banked_upgrades==2);
 Game probe=game;assert(probe.reopenUpgrade()&&probe.state==Upgrade&&probe.banked_upgrades==2);
 assert(probe.chooseOffer(0)&&probe.banked_upgrades==1); // first real offer remains purchasable
}
int main(int argc,char** argv){
 if(argc!=4){std::fprintf(stderr,"Usage: platform_qa_fixture MODE SAVE CONFIG\nMODE: heavy, effects, seraph, crucible, victory, upgrade, daily-menu\n");return 2;}
 const bool menu_only=!std::strcmp(argv[1],"daily-menu");
 if(!std::strcmp(argv[1],"heavy"))heavy();else if(!std::strcmp(argv[1],"effects"))effects();else if(!std::strcmp(argv[1],"victory"))victory();
 else if(!std::strcmp(argv[1],"upgrade"))upgrade();
 else if(menu_only){setup();game.configureRun(0);}
 else if(!std::strcmp(argv[1],"seraph")){setup();int n=game.spawnEnemy(E_SERAPH,160,60);assert(n>=0);game.enemies[n].age=201;game.auto_fire=true;}
 else if(!std::strcmp(argv[1],"crucible")){setup();game.wave=80;game.player.x=302*FP;game.player.y=120*FP;game.player.angle=0;assert(game.spawnEnemy(E_CRUCIBLE,160,80)>=0);}
 else return 2;
 game.state=Playing;game.resetInput();assert(game.validState());
 struct Snapshot{char magic[8];uint32_t version,bytes,valid,crc;Game value;};static Snapshot save={};std::memcpy(save.magic,"NNOVAS1",7);save.version=2;save.bytes=sizeof(Game);save.valid=menu_only?0:1;save.value=game;save.crc=crc32(&save.value,sizeof(Game));
 FILE* f=std::fopen(argv[2],"wb");if(!f)return 3;bool ok=std::fwrite(&save,1,sizeof(save),f)==sizeof(save);std::fclose(f);if(!ok)return 4;
 struct Config{uint32_t magic,version,flags,score,wave,runs,language,run_flags,deploy,reminders,autosave,background,skin,crc;};Config cfg={0x4e4f5641u,3,2|4|32|128,0,0,0,9,game.run_flags,15,1,1,3,0,0};cfg.crc=crc32(&cfg,sizeof(cfg)-4);
 f=std::fopen(argv[3],"wb");if(!f)return 5;ok=std::fwrite(&cfg,1,sizeof(cfg),f)==sizeof(cfg);std::fclose(f);if(!ok)return 6;
 std::printf("Synthetic %s: Game=%zu actors=%u parts=%u bullets=%u constructs=%u snapshot=%zu bytes\n",argv[1],sizeof(Game),game.enemyCount(),parts(),game.bulletCount(),game.constructCount(),sizeof(save));
}
