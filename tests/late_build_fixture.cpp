// Explicit QA fixture: this is a seeded rendering/performance stress scene,
// not a naturally completed run or evidence of full original-game parity.
#include "game.h"
#include <cassert>
#include <cstdio>
using namespace nova;
static Game game;
static void grant(unsigned id) {
 if(game.player.mods[id])return;
 if(mods[id].prerequisite_a>=0)grant((unsigned)mods[id].prerequisite_a);
 if(mods[id].prerequisite_b>=0)grant((unsigned)mods[id].prerequisite_b);
 assert(game.takeMod(id));
}
static uint32_t crc32(const void *ptr,unsigned size) {
 const uint8_t *p=(const uint8_t*)ptr;uint32_t crc=~0u;
 for(unsigned i=0;i<size;i++){crc^=p[i];for(unsigned b=0;b<8;b++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
 return ~crc;
}
int main(int argc,char **argv) {
 if(argc!=2){std::fprintf(stderr,"Usage: late_build_fixture OUTPUT.sav.tns\n");return 2;}
 game.start(0x51544658u);game.wild_mode=true;game.level=45;game.wave=0;game.next_wave=60000;game.xp_next=739;
 const unsigned mods_to_grant[]={M_FUSILLADE,M_GUIDANCE,M_BURST_FIRE,M_PAYLOAD,M_X_TWIN_STRIKE,M_X_DOUBLE_TAP,M_X_MASTERY,
  M_DRONES,M_ASSAULT_DRONES,M_X_FLEET,M_TURRET,M_WAR_MACHINE,M_ALLY,M_INTERCEPTOR,M_X_ECHO_STRIKE,M_MINES,M_LOADED_MINES,M_X_FARSIGHT};
 for(unsigned i=0;i<sizeof(mods_to_grant)/sizeof(mods_to_grant[0]);i++)grant(mods_to_grant[i]);
 game.equipWeapon(W_SALVO);game.equipBody(B_ENGINEER);game.equipShield(S_REFLECT);game.player.invulnerable=60000;
 game.auto_fire=true;game.recalculate();game.banked_upgrades=0;
 for(unsigned t=0;t<300;t++)game.tick({});
 game.wave=48;game.next_wave=60000;
 for(unsigned i=0;i<28;i++){int e=game.spawnEnemy(i%6+E_WARDEN,20+(i%7)*50,15+(i/7)*65,i%3==0?1:0);game.enemies[e].hull=game.enemies[e].max_hull=10000;}
 game.spawnEnemy(E_SERAPH,game.worldWidth()/2,35);
 for(unsigned t=0;t<45;t++){GameInput in={};in.fire=true;in.up=t%70<45;in.right=t%140<95;game.tick(in);if(game.state==Upgrade)game.bankUpgrade();}
 game.state=Playing;game.resetInput();assert(game.validState());
 struct Snapshot {char magic[8];uint32_t version,bytes,valid,crc;Game game;};
 static Snapshot save;const char magic[8]={'N','N','O','V','A','S','1',0};for(unsigned i=0;i<8;i++)save.magic[i]=magic[i];
 save.version=1;save.bytes=sizeof(Game);save.valid=1;save.game=game;save.crc=crc32(&save.game,sizeof(Game));
 FILE *f=std::fopen(argv[1],"wb");if(!f)return 3;bool ok=std::fwrite(&save,1,sizeof(save),f)==sizeof(save);std::fclose(f);if(!ok)return 4;
 std::printf("QA fixture: %zu-byte Game, %u enemies, %u bullets, %u constructs, logical arena %dx%d; not a natural playthrough.\n",sizeof(Game),game.enemyCount(),game.bulletCount(),game.constructCount(),game.worldWidth(),game.worldHeight());
}
