// Native spawnDynamicWaves recipes. All arithmetic is fixed point; no floating
// point runtime or heap allocation is needed on the calculator.
namespace {
#include "dynamic_choices.inc"
// Constructor 36..57 weights, multiplied by four to preserve .25 weights.
const uint8_t dynamic_weights[SOURCE_DYNAMIC_FORMATIONS]={16,28,24,12,12,10,8,4,5,4,0,12,12,12,12,12,32,32,20,20,16,16};
int dynamicRound(int64_t n,unsigned multiple=1){
 const int64_t unit=(int64_t)multiple*65536;
 // roundToMultiple is floor(value/multiple+.5)*multiple, including the
 // amount-1 expression used by odd V formations.
 int64_t shifted=n+unit/2;
 return (int)(shifted>=0?shifted/unit:-((-shifted+unit-1)/unit))*(int)multiple;
}
int dynamicClamp(int n,SourceEnemy enemy,uint16_t flags){const SourceEnemyInfo& i=sourceEnemyInfo(enemy,flags);return maxv(i.quantity_min,minv(n,i.quantity_max));}
unsigned dynamicPickFormation(SourceEnemy enemy,bool paired,uint32_t& rng){
 unsigned total=0;for(unsigned f=0;f<SOURCE_DYNAMIC_FORMATIONS;f++)total+=sourceDynamicWeight(enemy,f,paired);
 unsigned roll=choose(total,rng);
 for(unsigned f=0;f<SOURCE_DYNAMIC_FORMATIONS;f++){unsigned weight=sourceDynamicWeight(enemy,f,paired);if(roll<weight)return f;roll-=weight;}
 return SD_STANDARD;
}
SourceEnemy dynamicPickEnemy(const uint8_t* list,unsigned wave,uint16_t flags,uint32_t& rng){
 unsigned total=0;for(unsigned i=0;i<8&&list[i]!=255;i++)if(wave>=sourceEnemyInfo((SourceEnemy)list[i],flags).min_wave)total+=sourceEnemyInfo((SourceEnemy)list[i],flags).weight_q8;
 if(!total)return SE_NONE;
 unsigned roll=choose(total,rng);
 for(unsigned i=0;i<8&&list[i]!=255;i++){
  SourceEnemy e=(SourceEnemy)list[i];const SourceEnemyInfo& info=sourceEnemyInfo(e,flags);if(wave<info.min_wave)continue;
  if(roll<info.weight_q8)return e;
  roll-=info.weight_q8;
 }
 return SE_NONE;
}
int dynamicEntry(SourceEnemy enemy,uint32_t& rng){
 unsigned mask=dynamic_choices[enemy].entries,n=0;for(unsigned i=0;i<3;i++)if(mask&(1u<<i))++n;
 unsigned roll=choose(n,rng);for(unsigned i=0;i<3;i++)if(mask&(1u<<i)){if(!roll--)return (int)i;}return 0;
}
int dynamicFrequency(SourceEnemy enemy,int count,bool cascade){
 if(!cascade)return enemy==SE_SAUCER?180:160+20*minv(count,10);
 // Original anonymous function @9632: 25+220/max(count,1), with
 // hammerhead/mine and small-enemy adjustments.
 int value=(25*256+220*256/maxv(1,count));
 if(enemy==SE_HAMMERHEAD||enemy==SE_SHREDDER_MINE)value=value*3/4;
 if(enemy==SE_SCATTER_DRONE||enemy==SE_SWARMLING||enemy==SE_SWARMLING_GREEN)value=value*3/5;
 return maxv(1,(value+128)/256);
}
uint32_t dynamicFollowerAmount(SourceEnemy enemy,unsigned wave,uint16_t flags,uint32_t& rng){
 const SourceEnemyInfo& info=sourceEnemyInfo(enemy,flags);unsigned w=minv(wave,240u);
 int64_t low=(int64_t)info.quantity_low_q8*256,mean=(int64_t)info.quantity_mean_q8*256,high=(int64_t)info.quantity_high_q8*256;
 // The follower upper bound grows separately from the leader quantity.
 high=mean+(high-mean)*(49152+minv(16384u,w*65536/480))/65536;
 int draw=(int)choose(65536,rng)-(int)choose(65536,rng);
 int64_t amount=mean+(draw<0?(low-mean)*-(int64_t)draw:(high-mean)*draw)/65536;
 amount=amount*(65536+((int)w-120)*65536/240)/65536;
 unsigned upper=65536+minv(9830u,w*65536/800);
 amount=amount*(49152+(uint64_t)choose(65536,rng)*(upper-49152)/65536)/65536;
 return (uint32_t)(amount>0?amount:0);
}
int dynamicRankWidth(unsigned n){
 // ceil((width+.5)^(1/.45)) thresholds reproduce round(power(n,.45))
 // for the bounded follower quantities without a soft-float pow call.
 static const uint16_t threshold[]={3,8,17,29,45,65,89,117,149,186,228,274,325,381,442,508,579,655,736,823,915,1012,1114,1222,1336,1455,1580,1710,1847,1989,2136,2290,2449,2615,2786,2964};
 unsigned width=1;while(width<=sizeof(threshold)/sizeof(threshold[0])&&n>=threshold[width-1])++width;return (int)width;
}
unsigned dynamicPattern(unsigned formation,SourceEnemy enemy,unsigned wave,uint16_t flags,uint32_t amount,uint32_t& rng,SourceSpawnSpec* out,unsigned capacity,unsigned follower_percent){
 if(!out||!capacity||(unsigned)enemy>=SOURCE_ENEMY_COUNT||formation>=SOURCE_DYNAMIC_FORMATIONS)return 0;
 Recipes r={out,capacity,0};const DynamicChoices& choices=dynamic_choices[enemy];
 int entry=dynamicEntry(enemy,rng),w=minv((int)wave,240),repeat=0;
 // Saucer's alwaysRepeat is independent of the root count; normal waves
 // may add repetitions from the original per-family mayRepeat parameter.
 int scale=81920+(w-120)*65536/240+(w>120?137:0);
 // @6473d0: a one-in-four chance, then round(random(mayRepeat) *
 // lerp(waveScale,1,.5)). Saucer alwaysRepeat also draws [.75,1.25).
 if(choose(4,rng)==0&&choices.may_repeat_q8)
  repeat=dynamicRound((int64_t)choose(65536,rng)*choices.may_repeat_q8*((scale+65536)/2)/(256*65536));
 if(choices.always_repeat_q8){int jitter=49152+(int)choose(32768,rng);repeat+=dynamicRound((int64_t)choices.always_repeat_q8*256*scale/65536*jitter/65536);}
 unsigned edge=choose(4,rng);int count=dynamicClamp(dynamicRound(amount),enemy,flags);
 int64_t scaled=amount;
 switch(formation){
 case SD_FLEET:case SD_PINCER:case SD_V_FORMATION:case SD_CASCADE_FLEET:scaled=scaled*5/4;break;
 case SD_V_PINCER:scaled=scaled*5/8;break;
 case SD_SPIRAL:scaled=scaled*3/2;break;
 case SD_DOUBLE_SPIRAL:scaled=scaled*3/4;break;
 case SD_TRIPLE_SPIRAL:scaled=scaled/4;break;
 case SD_BOX:scaled=scaled*11/8;break;
 case SD_CASCADE_PINCER:case SD_CASCADE_OPPOSITE:case SD_CASCADE_ADJACENT:scaled=scaled*5/8;break;
 case SD_CASCADE_EDGES:scaled=scaled*11/32;break;
 default:break;
 }
 if(formation==SD_V_FORMATION||formation==SD_V_PINCER)count=dynamicClamp(dynamicRound(scaled-65536,2)+1,enemy,flags);
 else if(formation==SD_PINCER||formation==SD_CASCADE_PINCER||formation==SD_CASCADE_OPPOSITE||formation==SD_CASCADE_ADJACENT)count=dynamicClamp(dynamicRound(scaled,2),enemy,flags);
 else if(formation==SD_BOX)count=dynamicClamp(maxv(8,dynamicRound(scaled,4)),enemy,flags);
 else if(formation==SD_CASCADE_EDGES)count=dynamicClamp(dynamicRound(scaled,4),enemy,flags);
 else count=dynamicClamp(dynamicRound(scaled),enemy,flags);
 if(formation>=SD_ORBIT){
  SourceEnemy follower=dynamicPickEnemy(choices.followers,wave,flags,rng);
  if(follower==SE_NONE){SourceSpawnSpec& s=r.add(enemy);s.count=count;s.entry=entry;return r.count;}
  uint32_t followers=(uint32_t)((uint64_t)dynamicFollowerAmount(follower,wave,flags,rng)*follower_percent/100);
  int n=dynamicRound(followers);SourceSpawnSpec& s=r.add(enemy);s.count=sourceEnemyInfo(enemy,flags).quantity_min;s.entry=entry;s.follower=follower;
  if(formation==SD_ORBIT){s.behavior=4;n=maxv(2,dynamicRound((int64_t)followers*9/8));}
  else if(formation==SD_FLANK){s.behavior=3;n=maxv(2,dynamicRound((int64_t)dynamicRound(followers,2)*65536*9/8));s.behavior_parameter1_q8=(110-n*5)*256;s.behavior_parameter2_q8=15*256+70*256/n;}
  else if(formation==SD_RANK){s.behavior=1;n=maxv(1,dynamicRound((int64_t)followers*7/5));s.behavior_parameter1_q8=dynamicRankWidth(n)*256;}
  else if(formation==SD_TRAIL){s.behavior=6;n=maxv(2,dynamicRound((int64_t)followers*11/8));s.behavior_parameter1_q8=60*256;}
  else s.behavior=formation==SD_PROTECT?2:5;
  s.follower_count=maxv(1,n);s.action_delay_ticks=(int)choose(46,rng);s.break_delay_ticks=0;return r.count;
 }
 auto add=[&]()->SourceSpawnSpec&{SourceSpawnSpec& s=r.add(enemy);s.count=count;s.entry=entry;s.repeating=repeat;s.repeat_frequency_ticks=dynamicFrequency(enemy,count,false);return s;};
 switch(formation){
 case SD_STANDARD:add();break;
 case SD_ARC:{SourceSpawnSpec& s=add();s.spawn=5;s.spawn_parameter_q8=(choose(3,rng)?60+15*count:360)*256;if(enemy==SE_SHREDDER_MINE)s.spawn_parameter_q8=s.spawn_parameter_q8*9/10;break;}
 case SD_CENTERED:{SourceSpawnSpec& s=add();s.count=1;s.spawn=8;s.spawn_parameter_q8=128;s.fixed_direction=(int)edge;s.repeating=maxv(count,repeat)-1;break;}
 case SD_FLEET:add().spawn=3;break;
 case SD_PINCER:add().spawn=2;break;
 case SD_V_FORMATION:{SourceSpawnSpec& s=add();s.spawn=4;s.spawn_parameter_q8=90*256;break;}
 case SD_V_PINCER:{unsigned pair=choose(2,rng)*2;for(unsigned i=0;i<2;i++){SourceSpawnSpec& s=add();s.spawn=4;s.spawn_parameter_q8=90*256;s.fixed_direction=(int)(pair+i);}break;}
 case SD_BOX:add().spawn=7;break;
 case SD_SPIRAL:case SD_DOUBLE_SPIRAL:case SD_TRIPLE_SPIRAL:{unsigned arms=formation==SD_SPIRAL?1:formation==SD_DOUBLE_SPIRAL?2:3;int angle=(int)choose(360,rng);
  for(unsigned i=0;i<arms;i++){SourceSpawnSpec& s=add();s.count=1;s.spawn=6;s.spawn_parameter_q8=360*256;s.repeating=maxv(count,repeat)-(arms==1?1:0);s.repeat_frequency_ticks=dynamicFrequency(enemy,count,true);if(arms>1){s.fixed_direction=angle;s.added_delay_ticks=i*120;}}
  break;}
 case SD_CASCADE_FLEET:{SourceSpawnSpec& s=add();s.count=1;s.spawn=8;s.fixed_direction=(int)edge;s.repeating=maxv(count,repeat);s.repeat_frequency_ticks=dynamicFrequency(enemy,count,true);bool backwards=choose(2,rng)!=0;s.spawn_parameter_q8=backwards?230:26;s.iterate_spawn_parameter=(backwards?-1:1)*roundRatio(205,maxv(1,count));break;}
 case SD_CASCADE_PINCER:case SD_CASCADE_OPPOSITE:case SD_CASCADE_ADJACENT:case SD_CASCADE_EDGES:{
  unsigned arms=formation==SD_CASCADE_EDGES?4:2,pair=choose(2,rng)*2;int step=formation==SD_CASCADE_PINCER?roundRatio(179,maxv(1,count)):formation==SD_CASCADE_OPPOSITE?roundRatio(154,maxv(1,count)):roundRatio(128,maxv(1,count));
  for(unsigned i=0;i<arms;i++){SourceSpawnSpec& s=add();s.count=1;s.spawn=8;s.repeating=maxv(count,repeat);s.repeat_frequency_ticks=dynamicFrequency(enemy,count,true);
   bool reverse=formation==SD_CASCADE_EDGES?!(i&1):formation==SD_CASCADE_OPPOSITE||formation==SD_CASCADE_ADJACENT?i!=0:false;
   int start=formation==SD_CASCADE_PINCER?38:formation==SD_CASCADE_OPPOSITE?13:64;
   s.spawn_parameter_q8=reverse?256-start:start;s.iterate_spawn_parameter=reverse?-step:step;
   s.fixed_direction=formation==SD_CASCADE_EDGES?(int)(i<2?i:5-i):formation==SD_CASCADE_ADJACENT?(int)((edge+i)%4):(int)(pair+(i^1));
  }break;}
 default:add();break;
 }
 return r.count;
}
}
unsigned sourceDynamicWeight(SourceEnemy enemy,unsigned formation,bool paired){
 if((unsigned)enemy>=SOURCE_ENEMY_COUNT||formation>=SOURCE_DYNAMIC_FORMATIONS)return 0;
 const uint32_t mask=dynamic_choices[enemy].formations;
 // Before wave 120 (80 in Annihilation), native secondary waves
 // suppress the six follower formations. The caller supplies this gate.
 return (mask&(1u<<formation))&&!(paired&&formation>=SD_ORBIT)?dynamic_weights[formation]:0;
}
unsigned sourceDynamicPattern(unsigned formation,SourceEnemy enemy,unsigned wave,uint16_t flags,uint32_t amount,uint32_t& rng,SourceSpawnSpec* out,unsigned capacity){
 return dynamicPattern(formation,enemy,wave,flags,amount,rng,out,capacity,100);
}
unsigned sourceDynamicRecipe(SourceEnemy enemy,unsigned wave,uint16_t flags,uint32_t amount,uint32_t& rng,SourceSpawnSpec* out,unsigned capacity){
 if(!out||!capacity||(unsigned)enemy>=SOURCE_ENEMY_COUNT)return 0;
 unsigned formation=dynamicPickFormation(enemy,false,rng);
 // Native pair probability: (.3 elite / .4 normal) + .2*w/240;
 // Annihilation multiplies the result by 1.2.
 unsigned chance=(flags&RUN_ELITE?19661:26214)+minv(wave,240u)*13107/240;if(flags&RUN_ANNIHILATION)chance=chance*6/5;
 SourceEnemy partner=SE_NONE;
 if(choose(65536,rng)<chance)partner=dynamicPickEnemy(dynamic_choices[enemy].pairs,wave,flags,rng);
 unsigned count=dynamicPattern(formation,enemy,wave,flags,partner==SE_NONE?amount:(uint32_t)((uint64_t)amount*3/5),rng,out,capacity,partner==SE_NONE?100u:60u);
 if(partner!=SE_NONE&&count<capacity){
  unsigned a=choose(65536,rng),b=choose(65536,rng),v=choose(65536,rng);
  uint32_t partner_amount=(uint32_t)((uint64_t)sourceDynamicAmountQ16(partner,wave,flags,a,b,v)*2/5);
  unsigned partner_form=dynamicPickFormation(partner,wave<((flags&RUN_ANNIHILATION)?80u:120u),rng);
  unsigned extra=dynamicPattern(partner_form,partner,wave,flags,partner_amount,rng,out+count,capacity-count,40);
  for(unsigned i=0;i<extra;i++){SourceSpawnSpec& s=out[count+i];s.added_delay_ticks=80+(s.added_delay_ticks==SOURCE_UNDEFINED?0:s.added_delay_ticks);}
  count+=extra;
 }
 return count;
}
