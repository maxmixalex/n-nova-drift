#include "source_waves.h"
#include "game.h"
namespace nova {
namespace {
const char* const enemy_names[]={"asteroid","asteroidMega","interceptor","warden","gyrogun","scatterDrone","swarmling","swarmlingGreen","host","beamcaster","myrmidon","pulsar","saucer","boltThrower","hammerhead","snapper","serpent","vanguard","hive","crabNest","tracer","shredderMine","minelayer","eclipse","constructor","cherub","bulwark","juggernaut","celestial","spitfire","ceratotitan","stationOmega","dreadnought","scion","dweller","warbringer","crucible","seraph","glaucus","dwellerDrone","dwellerGuard","dwellerWarrior","null"};
// Native constructors 59..101, evaluated after configureDifficulty.
const SourceEnemyInfo enemy_info[4][SOURCE_ENEMY_COUNT]={
  { // normal
    {0,0,0,384,256,1536,3072,1,65535}, // asteroid
    {0,0,0,128,256,256,256,1,3}, // asteroidMega
    {0,0,0,1024,960,1344,1792,2,65535}, // interceptor
    {0,0,0,1024,640,768,1216,1,65535}, // warden
    {20,20,20,640,512,640,960,1,65535}, // gyrogun
    {0,0,0,512,3072,4096,6144,3,65535}, // scatterDrone
    {25,25,25,205,5120,7680,10240,6,65535}, // swarmling
    {25,25,25,0,3840,5632,7680,6,65535}, // swarmlingGreen
    {0,10,20,1024,512,768,1024,1,65535}, // host
    {10,10,15,960,640,768,1088,1,65535}, // beamcaster
    {45,50,60,576,640,896,1280,1,65535}, // myrmidon
    {15,25,40,768,384,576,704,1,65535}, // pulsar
    {10,10,15,768,256,320,448,1,65535}, // saucer
    {20,20,20,640,1792,2048,2560,1,65535}, // boltThrower
    {45,50,60,704,768,1280,1664,1,65535}, // hammerhead
    {20,20,35,512,320,512,768,1,7}, // snapper
    {20,25,35,512,256,320,704,1,6}, // serpent
    {20,20,45,768,320,448,640,1,65535}, // vanguard
    {15,25,41,768,256,358,358,1,6}, // hive
    {15,25,41,0,256,256,256,1,1}, // crabNest
    {65,50,60,512,256,550,576,1,65535}, // tracer
    {10,10,10,768,1280,1792,2304,1,65535}, // shredderMine
    {15,15,15,512,256,512,768,1,65535}, // minelayer
    {25,25,45,461,256,256,256,1,65535}, // eclipse
    {25,25,45,512,256,256,256,1,6}, // constructor
    {65,50,65,256,256,320,512,1,65535}, // cherub
    {65,70,90,448,256,256,256,1,2}, // bulwark
    {40,45,60,512,256,256,256,1,3}, // juggernaut
    {40,45,60,576,256,256,256,1,5}, // celestial
    {50,50,50,384,256,256,256,1,3}, // spitfire
    {75,75,75,192,256,256,256,1,2}, // ceratotitan
    {70,70,70,128,256,256,256,1,1}, // stationOmega
    {105,105,105,128,256,256,256,1,1}, // dreadnought
    {90,90,90,192,256,256,256,1,1}, // scion
    {135,135,135,96,256,256,256,1,1}, // dweller
    {110,110,110,128,256,256,256,1,1}, // warbringer
    {165,165,165,64,256,256,256,1,1}, // crucible
    {130,130,130,128,256,256,256,1,1}, // seraph
    {195,195,195,64,256,256,256,1,1}, // glaucus
    {135,135,135,0,256,256,256,1,65535}, // dwellerDrone
    {135,135,135,0,256,256,256,1,65535}, // dwellerGuard
    {135,135,135,0,256,256,256,1,65535}, // dwellerWarrior
    {0,0,0,0,256,256,256,1,1}, // null
  },
  { // elite
    {0,0,0,384,256,1536,3072,1,65535}, // asteroid
    {0,0,0,128,256,256,256,1,3}, // asteroidMega
    {0,0,0,1024,960,1344,1792,2,65535}, // interceptor
    {0,0,0,1024,640,768,1216,1,65535}, // warden
    {20,20,20,640,512,640,960,1,65535}, // gyrogun
    {0,0,0,512,3072,4096,6144,3,65535}, // scatterDrone
    {0,0,0,205,5120,7680,10240,6,65535}, // swarmling
    {0,0,0,0,3840,5632,7680,6,65535}, // swarmlingGreen
    {0,10,20,1024,512,768,1024,1,65535}, // host
    {10,10,15,960,640,768,1088,1,65535}, // beamcaster
    {20,25,35,576,640,896,1280,1,65535}, // myrmidon
    {15,25,40,768,384,576,704,1,65535}, // pulsar
    {10,10,15,768,256,320,448,1,65535}, // saucer
    {20,20,20,640,1792,2048,2560,1,65535}, // boltThrower
    {20,25,35,704,768,1280,1664,1,65535}, // hammerhead
    {20,20,35,512,320,512,768,1,7}, // snapper
    {20,25,35,512,256,320,704,1,6}, // serpent
    {20,20,45,768,320,448,640,1,65535}, // vanguard
    {15,25,41,768,256,358,358,1,6}, // hive
    {15,25,41,0,256,256,256,1,1}, // crabNest
    {40,25,35,512,256,550,576,1,65535}, // tracer
    {10,10,10,768,1280,1792,2304,1,65535}, // shredderMine
    {15,15,15,512,256,512,768,1,65535}, // minelayer
    {25,25,45,461,256,256,256,1,65535}, // eclipse
    {25,25,45,512,256,256,256,1,6}, // constructor
    {40,25,40,256,256,320,512,1,65535}, // cherub
    {40,45,65,448,256,256,256,1,2}, // bulwark
    {40,45,60,512,256,256,256,1,3}, // juggernaut
    {40,45,60,576,256,256,256,1,5}, // celestial
    {50,50,50,384,256,256,256,1,3}, // spitfire
    {75,75,75,192,256,256,256,1,2}, // ceratotitan
    {70,70,70,128,256,256,256,1,1}, // stationOmega
    {105,105,105,128,256,256,256,1,1}, // dreadnought
    {90,90,90,192,256,256,256,1,1}, // scion
    {135,135,135,96,256,256,256,1,1}, // dweller
    {110,110,110,128,256,256,256,1,1}, // warbringer
    {165,165,165,64,256,256,256,1,1}, // crucible
    {130,130,130,128,256,256,256,1,1}, // seraph
    {195,195,195,64,256,256,256,1,1}, // glaucus
    {135,135,135,0,256,256,256,1,65535}, // dwellerDrone
    {135,135,135,0,256,256,256,1,65535}, // dwellerGuard
    {135,135,135,0,256,256,256,1,65535}, // dwellerWarrior
    {0,0,0,0,256,256,256,1,1}, // null
  },
  { // anni
    {0,0,0,384,256,1536,3072,1,65535}, // asteroid
    {0,0,0,128,256,256,256,1,3}, // asteroidMega
    {0,0,0,1024,960,1344,1792,2,65535}, // interceptor
    {0,0,0,1024,640,768,1216,1,65535}, // warden
    {15,10,10,640,512,640,960,1,65535}, // gyrogun
    {0,0,0,512,3072,4096,6144,3,65535}, // scatterDrone
    {20,15,15,205,5120,7680,10240,6,65535}, // swarmling
    {20,15,15,0,3840,5632,7680,6,65535}, // swarmlingGreen
    {0,0,10,1024,512,768,1024,1,65535}, // host
    {5,0,5,960,640,768,1088,1,65535}, // beamcaster
    {40,40,50,576,640,896,1280,1,65535}, // myrmidon
    {10,15,30,768,384,576,704,1,65535}, // pulsar
    {5,0,5,768,256,320,448,1,65535}, // saucer
    {15,10,10,640,1792,2048,2560,1,65535}, // boltThrower
    {40,40,50,704,768,1280,1664,1,65535}, // hammerhead
    {15,10,25,512,320,512,768,1,7}, // snapper
    {15,15,25,512,256,320,704,1,6}, // serpent
    {15,10,35,768,320,448,640,1,65535}, // vanguard
    {10,15,31,768,256,358,358,1,6}, // hive
    {10,15,31,0,256,256,256,1,1}, // crabNest
    {60,40,50,512,256,550,576,1,65535}, // tracer
    {5,0,0,768,1280,1792,2304,1,65535}, // shredderMine
    {10,5,5,512,256,512,768,1,65535}, // minelayer
    {20,15,35,461,256,256,256,1,65535}, // eclipse
    {20,15,35,512,256,256,256,1,6}, // constructor
    {60,40,55,256,256,320,512,1,65535}, // cherub
    {60,60,80,448,256,256,256,1,2}, // bulwark
    {35,35,50,512,256,256,256,1,3}, // juggernaut
    {35,35,50,576,256,256,256,1,5}, // celestial
    {50,50,50,384,256,256,256,1,3}, // spitfire
    {75,75,75,192,256,256,256,1,2}, // ceratotitan
    {70,70,70,128,256,256,256,1,1}, // stationOmega
    {105,105,105,128,256,256,256,1,1}, // dreadnought
    {90,90,90,192,256,256,256,1,1}, // scion
    {135,135,135,96,256,256,256,1,1}, // dweller
    {110,110,110,128,256,256,256,1,1}, // warbringer
    {165,165,165,64,256,256,256,1,1}, // crucible
    {130,130,130,128,256,256,256,1,1}, // seraph
    {195,195,195,64,256,256,256,1,1}, // glaucus
    {135,135,135,0,256,256,256,1,65535}, // dwellerDrone
    {135,135,135,0,256,256,256,1,65535}, // dwellerGuard
    {135,135,135,0,256,256,256,1,65535}, // dwellerWarrior
    {0,0,0,0,256,256,256,1,1}, // null
  },
  { // elite_anni
    {0,0,0,384,256,1536,3072,1,65535}, // asteroid
    {0,0,0,128,256,256,256,1,3}, // asteroidMega
    {0,0,0,1024,960,1344,1792,2,65535}, // interceptor
    {0,0,0,1024,640,768,1216,1,65535}, // warden
    {15,10,10,640,512,640,960,1,65535}, // gyrogun
    {0,0,0,512,3072,4096,6144,3,65535}, // scatterDrone
    {0,0,0,205,5120,7680,10240,6,65535}, // swarmling
    {0,0,0,0,3840,5632,7680,6,65535}, // swarmlingGreen
    {0,0,10,1024,512,768,1024,1,65535}, // host
    {5,0,5,960,640,768,1088,1,65535}, // beamcaster
    {15,15,25,576,640,896,1280,1,65535}, // myrmidon
    {10,15,30,768,384,576,704,1,65535}, // pulsar
    {5,0,5,768,256,320,448,1,65535}, // saucer
    {15,10,10,640,1792,2048,2560,1,65535}, // boltThrower
    {15,15,25,704,768,1280,1664,1,65535}, // hammerhead
    {15,10,25,512,320,512,768,1,7}, // snapper
    {15,15,25,512,256,320,704,1,6}, // serpent
    {15,10,35,768,320,448,640,1,65535}, // vanguard
    {10,15,31,768,256,358,358,1,6}, // hive
    {10,15,31,0,256,256,256,1,1}, // crabNest
    {35,15,25,512,256,550,576,1,65535}, // tracer
    {5,0,0,768,1280,1792,2304,1,65535}, // shredderMine
    {10,5,5,512,256,512,768,1,65535}, // minelayer
    {20,15,35,461,256,256,256,1,65535}, // eclipse
    {20,15,35,512,256,256,256,1,6}, // constructor
    {35,15,30,256,256,320,512,1,65535}, // cherub
    {35,35,55,448,256,256,256,1,2}, // bulwark
    {35,35,50,512,256,256,256,1,3}, // juggernaut
    {35,35,50,576,256,256,256,1,5}, // celestial
    {50,50,50,384,256,256,256,1,3}, // spitfire
    {75,75,75,192,256,256,256,1,2}, // ceratotitan
    {70,70,70,128,256,256,256,1,1}, // stationOmega
    {105,105,105,128,256,256,256,1,1}, // dreadnought
    {90,90,90,192,256,256,256,1,1}, // scion
    {135,135,135,96,256,256,256,1,1}, // dweller
    {110,110,110,128,256,256,256,1,1}, // warbringer
    {165,165,165,64,256,256,256,1,1}, // crucible
    {130,130,130,128,256,256,256,1,1}, // seraph
    {195,195,195,64,256,256,256,1,1}, // glaucus
    {135,135,135,0,256,256,256,1,65535}, // dwellerDrone
    {135,135,135,0,256,256,256,1,65535}, // dwellerGuard
    {135,135,135,0,256,256,256,1,65535}, // dwellerWarrior
    {0,0,0,0,256,256,256,1,1}, // null
  },
};
const char* const pattern_names[]={
 "Explosive Delivery","Swarmling Unleash","Fractal Formation","Twin Hammer",
 "Trains","Asteroid Belt","Big Pincer","Don't Talk To Me Or My Son Ever Again",
 "Early Champion","Here Comes The Night Train","Beeeeeamcaster","Saucerpede",
 "Short, Controlled Bursts!","Vic","ASMD","The Whole Shebang","Green Machine",
 "Stellar Destroyers","MINIBOSS","Crab Nest","Crab Vic","Cursed Wave"};
unsigned random32(uint32_t& s){if(!s)s=0x6d2b79f5u;s^=s<<13;s^=s>>17;s^=s<<5;return s;}
unsigned choose(unsigned n,uint32_t& s){return n?random32(s)%n:0;}
int roundRatio(int n,int d){return (n+d/2)/d;}
int maxv(int a,int b){return a>b?a:b;}
int minv(int a,int b){return a<b?a:b;}
SourceSpawnSpec blank(SourceEnemy e){SourceSpawnSpec s={SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED,SOURCE_UNDEFINED};s.enemy=e;return s;}
struct Recipes {
 SourceSpawnSpec* out;unsigned capacity,count;
 SourceSpawnSpec& add(SourceEnemy enemy){static SourceSpawnSpec discarded;SourceSpawnSpec s=blank(enemy);if(count>=capacity){discarded=s;return discarded;}out[count]=s;return out[count++];}
};
}
const char* sourceEnemyName(SourceEnemy enemy){return (unsigned)enemy<SOURCE_ENEMY_COUNT?enemy_names[enemy]:"undefined";}
const char* sourceCuratedName(unsigned pattern){return pattern<22?pattern_names[pattern]:"Opening";}
const SourceEnemyInfo& sourceEnemyInfo(SourceEnemy enemy,uint16_t flags){unsigned mode=((flags&RUN_ELITE)?1u:0u)|((flags&RUN_ANNIHILATION)?2u:0u);return enemy_info[mode][(unsigned)enemy<SOURCE_ENEMY_COUNT?enemy:SE_NULL];}

namespace {
bool available(SourceEnemy e,unsigned wave,uint16_t flags){const SourceEnemyInfo& i=sourceEnemyInfo(e,flags);return i.weight_q8&&wave>=i.min_wave;}
struct Choices {
 SourceEnemy values[20];unsigned count;
 void add(SourceEnemy e){if(count<20)values[count++]=e;}
 void availableAdd(SourceEnemy e,unsigned wave,uint16_t flags){if(available(e,wave,flags))add(e);}
 SourceEnemy pick(uint32_t& rng){return count?values[choose(count,rng)]:SE_HOST;}
 SourceEnemy take(uint32_t& rng){if(!count)return SE_HOST;unsigned at=choose(count,rng);SourceEnemy e=values[at];for(unsigned i=at+1;i<count;i++)values[i-1]=values[i];--count;return e;}
};
int hostileQ8(uint16_t flags){if(!(flags&RUN_HOSTILE))return 256;return 256+((flags&RUN_MAYHEM)?320:256)/3;}
bool challenging(SourceEnemy e,unsigned wave,int lower,int upper,uint16_t flags){
 int c=sourceEnemyInfo(e,flags).champion_wave;
 int floor=60+((flags&RUN_ANNIHILATION)?-5:0);
 return ((int)wave>=c+lower&&(int)wave<=c+upper)||(c>=floor&&(int)wave>=floor+lower);
}
void flank(SourceSpawnSpec& s,SourceEnemy follower,int count,int angle,int spread){s.behavior=3;s.follower=follower;s.follower_count=count;s.behavior_parameter1_q8=angle*256;s.behavior_parameter2_q8=spread*256;}
void orbit(SourceSpawnSpec& s,SourceEnemy follower,int count){s.behavior=4;s.follower=follower;s.follower_count=count;}
void trail(SourceSpawnSpec& s,SourceEnemy follower,int count){s.behavior=6;s.follower=follower;s.follower_count=count;}
}
unsigned sourceCuratedPattern(unsigned pattern,unsigned wave,uint16_t flags,uint32_t& rng,SourceSpawnSpec* out,unsigned capacity){
 if(!out||!capacity||pattern>=22)return 0;
 Recipes r={out,capacity,0};int w=minv((int)wave,240),offset=(flags&RUN_ANNIHILATION)?-5:0;
 switch(pattern){
 case 0: case 1:{
  Choices choices={{SE_MINELAYER},1};const SourceEnemy heavy[]={SE_ECLIPSE,SE_CONSTRUCTOR,SE_BULWARK,SE_JUGGERNAUT};
  for(unsigned i=0;i<4;i++)choices.availableAdd(heavy[i],wave,flags);
  if(pattern==1){choices.availableAdd(SE_SCION,wave,flags);choices.availableAdd(SE_SERAPH,wave,flags);choices.availableAdd(SE_CERATOTITAN,wave,flags);}
  SourceEnemy e=choices.pick(rng);SourceSpawnSpec& s=r.add(e);s.count=1;if(w>=120)s.rarity=2;
  if(pattern==0){flank(s,SE_HAMMERHEAD,8,75,24);s.break_delay_ticks=90;}
  else {orbit(s,e==SE_CERATOTITAN?SE_SWARMLING_GREEN:SE_SWARMLING,e==SE_CERATOTITAN?12:16);s.action_delay_ticks=9;s.break_delay_ticks=18;s.repeat_formation=3+roundRatio(w,120);}
  break;
 }
 case 2:{SourceSpawnSpec& s=r.add(SE_HOST);s.count=1;s.rarity=2;orbit(s,SE_HOST,3+roundRatio(w,120));s.action_delay_ticks=45;s.repeat_formation=1+roundRatio(w,60);s.repeat_size_q8=224;break;}
 case 3:{bool heavy=available(SE_JUGGERNAUT,wave,flags);SourceSpawnSpec& s=r.add(heavy?SE_JUGGERNAUT:SE_SHREDDER_MINE);s.count=1;if(!heavy)s.rarity=2;orbit(s,SE_SHREDDER_MINE,2);s.repeat_formation=3+roundRatio(w,80);break;}
 case 4:{
  if(available(SE_CELESTIAL,wave,flags)){SourceSpawnSpec& s=r.add(SE_CELESTIAL);s.count=1;s.rarity=roundRatio(w*256,25600+(int)choose(10241,rng));trail(s,SE_INTERCEPTOR,7);s.action_delay_ticks=20;}
  SourceSpawnSpec& s=r.add(SE_INTERCEPTOR);s.count=2;s.rarity=2;s.spawn=choose(2,rng)?3:2;trail(s,SE_INTERCEPTOR,5);s.action_delay_ticks=20;break;
 }
 case 5:{r.add(SE_ASTEROID).count=1+roundRatio(w*hostileQ8(flags),20*256);r.add(SE_ASTEROID_MEGA).count=1+roundRatio(w*hostileQ8(flags),80*256);break;}
 case 6:{Choices c={{SE_VANGUARD},1};c.availableAdd(SE_SPITFIRE,wave,flags);if(w>=40)c.availableAdd(SE_PULSAR,wave,flags);if(w>=80)c.availableAdd(SE_ECLIPSE,wave,flags);
  if(w>=240){c.availableAdd(SE_CELESTIAL,wave,flags);c.availableAdd(SE_BULWARK,wave,flags);c.availableAdd(SE_JUGGERNAUT,wave,flags);}SourceSpawnSpec& s=r.add(c.pick(rng));s.count=2;s.spawn=2;s.rarity=w<45?-1:2;break;}
 case 7: case 8:{
  Choices c={{SE_HOST},pattern==7?1u:0u};const SourceEnemy candidates[]={SE_HOST,SE_MYRMIDON,SE_PULSAR,SE_SNAPPER,SE_SERPENT,SE_VANGUARD,SE_TRACER,SE_HIVE,SE_BULWARK,SE_JUGGERNAUT,SE_CELESTIAL,SE_SPITFIRE};
  for(unsigned i=pattern==7?1u:0u;i<((flags&RUN_NEMESIS)?12u:11u);i++)if(challenging(candidates[i],wave,pattern==7?10:-10,pattern==7?40:20,flags))c.add(candidates[i]);
  SourceEnemy e=c.pick(rng);SourceSpawnSpec& s=r.add(e);s.count=1;s.rarity=2;
  if(pattern==7){s.size_q8=302;s.damage_q8=287;s.treasure_q8=338;SourceSpawnSpec& child=r.add(e);child.count=1;child.rarity=0;child.size_q8=218;child.damage_q8=child.treasure_q8=256;}
  else {s.behavior=5;s.follower=SE_CHERUB;}break;
 }
 case 9:{bool v=choose(2,rng)&&w>=sourceEnemyInfo(SE_VANGUARD,flags).champion_wave;SourceSpawnSpec& s=r.add(v?SE_VANGUARD:SE_HAMMERHEAD);s.rarity=v?2:w>=sourceEnemyInfo(SE_HAMMERHEAD,flags).champion_wave?2:1;trail(s,SE_HAMMERHEAD,4+roundRatio(w,30));s.break_delay_ticks=105+(int)choose(91,rng);break;}
 case 10:{SourceSpawnSpec& s=r.add(SE_BEAMCASTER);s.count=1;s.rarity=2;trail(s,SE_BEAMCASTER,3+roundRatio(w,40));s.behavior_parameter1_q8=60*256;s.action_delay_ticks=20;s.support=SE_CHERUB;s.support_count=1;s.follower_min_rarity=1;break;}
 case 11:{
  int total=11+roundRatio(w,20);unsigned angle=choose(4,rng),variant=choose(3,rng);SourceSpawnSpec& s=r.add(SE_SAUCER);s.entry=0;
  if(variant==2){s.count=2;s.spawn=3;s.repeating=roundRatio(total*320,512);}
  else if(variant==1){s.count=1;s.spawn=2;s.repeating=2*roundRatio(total,4);}
  else {s.count=1;s.spawn=8;s.spawn_parameter_q8=128;s.repeating=total;s.fixed_direction=(int)angle;}
  s.repeat_frequency_ticks=90+120/maxv(1,s.repeating);break;
 }
 case 12:{int followers=3+roundRatio(w,80);for(unsigned i=0;i<3;i++){SourceSpawnSpec& s=r.add(SE_BEAMCASTER);s.count=1;s.rarity=minv(2,roundRatio(w,60));orbit(s,SE_SHREDDER_MINE,followers);s.behavior_parameter1_q8=(i&1?3:-3)*256;s.behavior_parameter2_q8=followers*8*256;s.added_delay_ticks=i*80;s.follower_max_rarity=1;}break;}
 case 13:{unsigned variant=choose(2+(w>=60+offset)+(w>=100+offset),rng);SourceEnemy e=variant==0?SE_WARDEN:variant==1?SE_INTERCEPTOR:variant==2?SE_MYRMIDON:SE_VANGUARD;SourceSpawnSpec& s=r.add(e);flank(s,e,2,130,45);s.support_count=1;s.repeat_size_q8=224;s.follower_min_rarity=0;s.follower_max_rarity=1;
  if(variant>=2){s.count=1;s.rarity=2;s.support=SE_CHERUB;s.repeat_formation=(variant==3?1:2)+roundRatio(w,variant==3?120:180);}
  else {s.count=2;s.spawn=2;s.repeat_formation=(variant==0?2:3)+roundRatio(w,variant==0?80:60);}break;
 }
 case 14:{int n=1+roundRatio(w,150),followers=2*roundRatio(2+roundRatio(w,90),2),angle=110-followers*5,spread=15*256+70*256/followers;
  if(w>50+offset){SourceSpawnSpec& s=r.add(SE_TRACER);s.count=n;s.spawn=5;s.spawn_parameter_q8=40*256;flank(s,SE_INTERCEPTOR,4,angle,0);s.behavior_parameter2_q8=spread;s.action_delay_ticks=8;s.added_delay_ticks=480;}
  if(w>120+offset){SourceSpawnSpec& s=r.add(SE_INTERCEPTOR);s.count=2;s.added_delay_ticks=240;}
  SourceSpawnSpec& s=r.add(SE_TRACER);s.count=1;s.rarity=w>=sourceEnemyInfo(SE_TRACER,flags).champion_wave?2:1;flank(s,SE_HAMMERHEAD,followers,angle,0);s.behavior_parameter2_q8=spread;s.break_delay_ticks=2;break;
 }
 case 15:{SourceSpawnSpec& s=r.add(SE_BULWARK);flank(s,SE_MYRMIDON,2*roundRatio(4+roundRatio(w,60),2),75,24);s.action_delay_ticks=20;s.add_enemy=SE_HAMMERHEAD;s.add_frequency_ticks=660;s.support=SE_CHERUB;s.support_count=1;break;}
 case 16:{SourceSpawnSpec& a=r.add(SE_ECLIPSE);a.rarity=2;a.support=SE_PULSAR;a.support_count=1;a.size_q8=256;SourceSpawnSpec& b=r.add(SE_SPITFIRE);b.entry=1;b.support=SE_PULSAR;b.support_count=1;b.size_q8=256;r.add(SE_PULSAR).rarity=2;break;}
 case 17:{SourceSpawnSpec& s=r.add(SE_CELESTIAL);s.count=1;s.entry=1;s.rarity=2;s.behavior=3;s.follower=SE_CELESTIAL;s.follower_count=2;s.support=SE_CHERUB;break;}
 case 18:{SourceSpawnSpec& s=r.add(SE_SERPENT);s.count=1;s.rarity=2;s.add_enemy=SE_INTERCEPTOR;s.add_frequency_ticks=900;s.support=SE_CHERUB;s.size_q8=282;SourceSpawnSpec& child=r.add(SE_SERPENT);child.count=1;child.rarity=0;break;}
 case 19:{SourceSpawnSpec& s=r.add(SE_CRAB_NEST);s.count=1;s.rarity=2;break;}
 case 20:{SourceSpawnSpec& s=r.add(SE_DWELLER_WARRIOR);s.count=1;flank(s,SE_DWELLER_GUARD,2,130,45);s.add_enemy=SE_DWELLER_DRONE;s.add_frequency_ticks=90;s.support=SE_DWELLER_DRONE;s.support_count=12;s.repeat_formation=maxv(3,1+roundRatio(w,80));s.repeat_size_q8=218;s.follower_min_rarity=0;s.follower_max_rarity=1;break;}
 case 21:{
  int extra=w<200?50:0;Choices c={{SE_JUGGERNAUT,SE_BULWARK},2};
  if(w>=sourceEnemyInfo(SE_SCION,flags).min_wave+extra)c.add(SE_SCION);
  if(w>=sourceEnemyInfo(SE_WARBRINGER,flags).min_wave+extra)c.add(SE_WARBRINGER);
  SourceSpawnSpec& a=r.add(c.take(rng));a.count=1;a.rarity=2;
  const SourceEnemy later[]={SE_DWELLER,SE_GLAUCUS,SE_SERAPH};for(unsigned i=0;i<3;i++)if(w>=sourceEnemyInfo(later[i],flags).min_wave+extra)c.add(later[i]);
  if(wave>=300&&w>=sourceEnemyInfo(SE_DREADNOUGHT,flags).min_wave+extra)c.add(SE_DREADNOUGHT);
  SourceSpawnSpec& b=r.add(c.pick(rng));b.count=1;b.rarity=2;if(b.enemy==SE_CRUCIBLE)b.entry=1;break;
 }
 }
 return r.count;
}
unsigned sourceCuratedWave(unsigned wave,uint16_t flags,uint32_t& rng,SourceSpawnSpec* out,unsigned capacity,bool frequent_curated,bool turbo){
 // spawnCuratedWaves 0x14061cd41..0x14061cd93: Cosmic Ballet changes
 // the interval to five; it does not change boss scheduling or wave numbers.
 // Turbo advances the even wave counter by two; the native extra Ballet
 // gate is (wave+6)%10==0, retaining the in-between curated recipe.
 if(!wave||(wave%(frequent_curated?5:10)&&!(frequent_curated&&turbo&&wave%10==4)))return 0;
 unsigned limit=(flags&RUN_ELITE)&&wave>=((flags&RUN_ANNIHILATION)?80u:120u)?22:15;
 unsigned pattern=choose(limit,rng);
 if((wave>=300&&wave%50==0)||(wave>100&&wave%100==0))pattern=21;
 return sourceCuratedPattern(pattern,wave,flags,rng,out,capacity);
}

uint32_t sourceDynamicAmountQ16(SourceEnemy enemy,unsigned wave,uint16_t flags,
                               unsigned first,unsigned second,unsigned variance){
 // spawnDynamicWaves 0x140630c30..0x140631f07, checked by executing the
 // original x64 arithmetic and lerp callbacks. The native code draws r1-r2,
 // then interpolates each side of the mean separately. A fixed mean loses
 // both its distribution and the wave-dependent lower/upper bounds.
 const SourceEnemyInfo& info=sourceEnemyInfo(enemy,flags);
 const int w=minv((int)wave,240), threshold=(flags&RUN_ANNIHILATION)?80:120;
 const int skew=(flags&RUN_ELITE)?26214:39322; // .4 / .6 in Q16.
 int64_t low=(int64_t)info.quantity_low_q8*256;
 const int64_t mean=(int64_t)info.quantity_mean_q8*256;
 int64_t high=(int64_t)info.quantity_high_q8*256;
 int high_bias=skew-minv(skew,skew*w/threshold);
 high+=(mean-high)*high_bias/65536;
 if(w>threshold)low+=(mean-low)*minv(skew,skew*(w-threshold)/threshold)/65536;
 const int draw=(int)(first&65535)-(int)(second&65535);
 int64_t amount=mean+(draw<0?(low-mean)*-(int64_t)draw:(high-mean)*(int64_t)draw)/65536;
 // Native scale = 1.25 + .00416667*(min(wave,240)-120), with an
 // additional 1/480 after wave 120. Fixed point avoids ARM soft doubles.
 int scale=81920+(w-120)*65536/240+(w>120?137:0);
 amount=amount*scale/65536;
 if(flags&RUN_ANNIHILATION)amount=amount*115/100;
 // random_range(.75, 1.1 + min(.15, .00125*wave)).
 const int upper=72090+minv(9830,w*65536/800);
 const int jitter=49152+(int)((uint64_t)(variance&65535)*(unsigned)(upper-49152)/65536);
 amount=amount*jitter/65536;
 return (uint32_t)(amount>0?amount:0);
}

unsigned sourceEnemyDelayFrames(SourceEnemy enemy) {
 // Original setWaveDelay 0x14058b780, replayed for all 864 object IDs.
 static const uint16_t frames[SOURCE_ENEMY_COUNT]={0,90,60,150,240,16,0,0,240,180,420,640,180,48,480,640,900,700,1800,15000,670,90,400,3500,5200,60,18000,5200,5200,7500,15000,36000,36000,24000,32000,32000,32000,36000,36000,60,800,6000,6400};
 return (unsigned)enemy<SOURCE_ENEMY_COUNT?frames[(unsigned)enemy]:0;
}

} // namespace nova
