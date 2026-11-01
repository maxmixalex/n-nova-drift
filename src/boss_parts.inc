// Original sprite collision boxes and pivots, extracted from SPRT.
// A fixed pool provides independent hull, collision, detachment and weapons.
namespace {
const BossPartArt novaBossArt[BP_COUNT]={
 {"sDreadnoughtWing",686,632,642,566,-117,-354},
 {"sDreadnoughtGunLarge",226,226,136,136,-1,-1},
 {"sDreadnoughtGunSmall",182,182,92,94,-1,-1},
 {"sWarbringerChasis",261,403,176,321,89,0},
 {"sWarbringerWing",179,183,95,100,-109,-115},
 {"sWarbringerThruster",238,271,155,183,11,-96},
 {"sWarbringerBroadside",239,207,157,124,-56,-65},
 {"sWarbringerLaser",301,251,218,165,129,0},
 {"sCeratotitanThorax",363,262,297,190,-130,-1},
 {"sCeratotitanPincer",437,328,368,255,149,-95},
 {"sCeratotitanElytra",302,149,230,81,-109,21},
 {"sCeratotitanWing",367,400,97,210,-15,-107},
 {"sGlaucusSegment",210,278,91,143,0,-1},
 {"sGlaucusEye",59,38,123,65,-23,-1},
 {"sStarEaterSegment",451,591,366,472,92,0},
 {"sStarEaterEye",124,82,124,82,-1,-1},
 {"sStarEaterAbdomen",1185,1074,1185,1074,442,-1},
 {"sStationNode",167,203,59,109,-72,0},
 {"sStation",286,646,204,566,-6,-1},
 {"sBulwarkBarrierDim",811,1133,637,947,300,0},
 {"sBulwarkBody",529,680,418,555,202,1},
 {"sBulwarkBody2",354,283,246,221,-116,0},
 {"sBulwarkBody3",351,333,244,221,-117,0},
 {"sBulwarkArm",227,384,116,315,14,111},
 {"sBulwarkTail",610,268,508,164,-253,-1},
 {"sBulwarkThruster",294,256,182,140,-87,-61},
 {"sBulwarkThrusterLarge",350,363,234,245,-115,-124},
 {"sWormHead",255,402,184,338,23,-1},
 {"sWormBody",255,402,151,130,21,-1},
 {"sGaussJawLeft",302,151,237,81,89,-48},
 {"sGaussJawRight",302,151,237,81,89,48},
 {"sGaussJawLeft2",251,136,188,65,73,-41},
 {"sGaussJawRight2",251,136,188,65,73,39},
 {"sSquidJawLeft",400,250,244,126,122,-56},
 {"sSquidJawRight",400,250,242,125,121,60},
 {"sSquidTent",174,111,115,51,34,0},
 {"sSquidTent",174,111,115,51,34,0},
 {"sEclipseChasis",513,268,451,215,-40,-27},
 {"sEclipsePanel",504,305,443,243,43,-51},
 {"sEclipseShield",657,657,611,611,0,0},
 {"sCrucibleCornerCool",464,464,356,356,160,160},
 {"sCrucibleEdgeCool",141,1035,31,929,0,0},
};
}
const BossPartArt& bossPartArt(unsigned kind){return novaBossArt[kind<BP_COUNT?kind:0];}
const char* bossPartSprite(unsigned kind){return kind<BP_COUNT?novaBossArt[kind].sprite:0;}
namespace {
bool novaChainBoss(unsigned type){return type==E_STAR_EATER||type==E_GLAUCUS;}
unsigned novaPartOwnerType(unsigned kind){
 if(kind<=BP_DREAD_GUN_SMALL)return E_DREADNOUGHT;
 if(kind<=BP_WAR_LASER)return E_WARBRINGER;
 if(kind<=BP_CERA_WING)return E_CERATOTITAN;
 if(kind<=BP_GLAUCUS_EYE)return E_GLAUCUS;
 if(kind<=BP_STAR_ABDOMEN)return E_STAR_EATER;
 if(kind<=BP_STATION_WALL)return E_STATION_OMEGA;
  if(kind<=BP_BULWARK_THRUSTER_LARGE)return E_BULWARK;
  if(kind<=BP_WORM_BODY)return E_WORM;
  if(kind<=BP_BOLT_JAW_RIGHT_ELITE)return E_BOLT_THROWER;
  if(kind<=BP_SNAPPER_SEGMENT)return E_SNAPPER;
  if(kind<=BP_ECLIPSE_SHIELD)return E_ECLIPSE;
 return E_CRUCIBLE;
}
bool novaPartWeaponAlive(const Game &g,const Enemy &e,unsigned kind,unsigned slot=255){
 unsigned owner=unsigned(&e-g.enemies);
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i){const BossPart &p=g.boss_parts[i];
  if(p.active&&p.owner==owner&&p.kind==kind&&(slot==255||slot==p.slot))return true;}
 return false;
}
}

bool Game::sweptBossPart(const BossPart &p,int32_t ox,int32_t oy,int32_t nx,int32_t ny,unsigned padding) const {
 if(!p.active||p.owner>=MAX_ENEMIES||!enemies[p.owner].active||enemies[p.owner].boss_hidden)return false;
 const bool chain=novaChainBoss(enemies[p.owner].type)||enemies[p.owner].type==E_CRUCIBLE;
 int dx=delta(nx,ox,worldWidth()),dy=delta(ny,oy,worldHeight());
 int tx=chain?int((p.x-ox)/FP):delta(p.x,ox,worldWidth());
 int ty=chain?int((p.y-oy)/FP):delta(p.y,oy,worldHeight());
 int extent=int(p.radius+padding);
 if(absn(tx)>extent+absn(dx)||absn(ty)>extent+absn(dy))return false;
 const BossPartArt &art=bossPartArt(p.kind);int major=maxv(art.bounds_width,art.bounds_height);
 int rx=maxv(1,p.source_scale?int((int64_t)art.bounds_width*p.source_scale/131072):int(p.radius)*art.bounds_width/major)+int(padding);
 int ry=maxv(1,p.source_scale?int((int64_t)art.bounds_height*p.source_scale/131072):int(p.radius)*art.bounds_height/major)+int(padding);
 int c=dirX(p.angle),s=dirY(p.angle);
 int x=-(tx*c+ty*s)/FP,y=(tx*s-ty*c)/FP;
 int vx=(dx*c+dy*s)/FP,vy=(-dx*s+dy*c)/FP;
 // Swept oriented source bounding box: thin station walls stay thin, and
 // high-speed rail shots cannot tunnel through a gun. All math is integer.
 int enter=0,leave=1024;
 const auto slab=[&](int pos,int speed,int r){
  if(!speed)return absn(pos)<=r;
  int lo=(-r-pos)*1024/speed,hi=(r-pos)*1024/speed;
  if(lo>hi){int swap=lo;lo=hi;hi=swap;}
  enter=maxv(enter,lo);leave=minv(leave,hi);return enter<=leave;
 };
 if(p.kind==BP_ECLIPSE_SHIELD)return sweptHit(ox,oy,nx,ny,p.x,p.y,p.radius+padding);
 if(p.kind==BP_BULWARK_BARRIER){
  // The source shield is an open crescent. Its transparent interior and rear
  // are not a filled bounding rectangle. Sweep against an expanded ellipse.
  int center=int((int64_t)art.center_x*p.source_scale/65536);
  int ex=maxv(1,rx-int(padding)+center),ey=maxv(1,ry-int(padding));
  int front=x+center,back=front+vx;
  if(front<-(int)padding&&back<-(int)padding)return false;
  int t0=0,t1=1024;
  if(front<-(int)padding&&vx>0)t0=(-int(padding)-front)*1024/vx;
  if(back<-(int)padding&&vx<0)t1=(-int(padding)-front)*1024/vx;
  int64_t ax=(front+vx*t0/1024)*256/ex,ay=(y+vy*t0/1024)*256/ey;
  int64_t bx=(front+vx*t1/1024)*256/ex,by=(y+vy*t1/1024)*256/ey;
  int64_t sx=bx-ax,sy=by-ay,len=sx*sx+sy*sy;
  int t=len?(int)clamp((-ax*sx-ay*sy)*1024/len,(int64_t)0,(int64_t)1024):0;
  int64_t nx=ax+sx*t/1024,ny=ay+sy*t/1024;
  int band=(2+int(padding))*256/minv(ex,ey),outer=256+band,inner=maxv(0,256-band);
  return nx*nx+ny*ny<=(int64_t)outer*outer&&maxv(ax*ax+ay*ay,bx*bx+by*by)>=(int64_t)inner*inner;
 }
 if(p.kind==BP_CRUCIBLE_CORNER){
  int thin=2+int(padding),shiftx=maxv(0,rx-thin),shifty=maxv(0,ry-thin);
  bool first=slab(x+shiftx,vx,thin)&&slab(y,vy,ry)&&leave>=0&&enter<=1024;
  enter=0;leave=1024;
  return first||(slab(x,vx,rx)&&slab(y+shifty,vy,thin)&&leave>=0&&enter<=1024);
 }
 return slab(x,vx,rx)&&slab(y,vy,ry)&&leave>=0&&enter<=1024;
}

int Game::healEnemy(unsigned index,int amount){
 if(index>=MAX_ENEMIES||amount<=0)return 0;
 Enemy &e=enemies[index];if(!e.active||e.hull<=0)return 0;
 amount=minv(amount,e.max_hull-e.hull);if(amount<=0)return 0;
 bool aggregate=e.type==E_BULWARK||e.type==E_WORM||e.type==E_STATION_OMEGA||e.type==E_STAR_EATER;
 if(!aggregate){e.hull+=amount;return amount;}
 // enemyHealingAura visits oEnemyStandard, not independent oEnemySegment
 // appendages. The native pure controllers must restore actual target hull
 // along with their aggregate, without reviving a destroyed component.
 const auto eligible=[&](const BossPart &p){
  if(!p.active||p.owner!=index||p.hull>=p.max_hull)return false;
  if(e.type==E_BULWARK)return p.kind==BP_BULWARK_CORE;
  if(e.type==E_WORM)return p.kind==BP_WORM_HEAD||p.kind==BP_WORM_BODY;
  if(e.type==E_STATION_OMEGA)return p.kind==BP_STATION_NODE||p.kind==BP_STATION_WALL;
  return p.kind==BP_STAR_SEGMENT||p.kind==BP_STAR_EYE||p.kind==BP_STAR_ABDOMEN;
 };
 int64_t missing=0;for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(eligible(boss_parts[i]))missing+=boss_parts[i].max_hull-boss_parts[i].hull;
 int total=(int)minv((int64_t)amount,missing),left=total;
 for(unsigned i=0;i<MAX_BOSS_PARTS&&left;++i){BossPart &p=boss_parts[i];if(!eligible(p))continue;
  int need=p.max_hull-p.hull,gain=(int)((int64_t)left*need/missing);p.hull+=gain;left-=gain;missing-=need;
 }
 e.hull+=total-left;return total-left;
}

bool Game::hitBossPart(unsigned index,int raw,bool burn,bool area){
 if(index>=MAX_BOSS_PARTS||raw<=0)return false;
 BossPart &p=boss_parts[index];if(!p.active||p.owner>=MAX_ENEMIES)return false;
 const unsigned owner=p.owner,kind=p.kind;if(kind>=BP_CRUCIBLE_CORNER)return false;Enemy &e=enemies[owner];
 if(!e.active||e.boss_hidden)return false;
 int coefficient=1000;bool share=true;
 switch(kind){
 case BP_DREAD_WING:coefficient=400;break;
 case BP_DREAD_GUN_LARGE:coefficient=2000;break;
 case BP_DREAD_GUN_SMALL:coefficient=5000;break;
 case BP_WAR_CHASSIS:case BP_WAR_WING:case BP_WAR_THRUSTER:case BP_WAR_BROADSIDE:case BP_WAR_LASER:
  coefficient=area?500:1000;share=false;break;
 case BP_CERA_THORAX:coefficient=area?500:1000;break;
 case BP_CERA_PINCER:case BP_CERA_ELYTRON:case BP_CERA_WING:coefficient=area?100:300;break;
 case BP_GLAUCUS_SEGMENT:coefficient=area?264:880;break;
 // Eye End Step calls segmentLeaderDamage(dependency) at 0x140b1badf
 // even though its generic buildEnemyPart damageLeader flag is false.
 case BP_GLAUCUS_EYE:coefficient=4000;break;
 case BP_STAR_SEGMENT:case BP_STAR_ABDOMEN:coefficient=area?125:500;break;
 case BP_STATION_NODE:coefficient=2000;break;
  case BP_BULWARK_BARRIER:coefficient=250;share=false;break;
  case BP_BULWARK_BODY2:case BP_BULWARK_BODY3:case BP_BULWARK_ARM:case BP_BULWARK_TAIL:
  case BP_BULWARK_THRUSTER:case BP_BULWARK_THRUSTER_LARGE:share=false;break;
  case BP_BOLT_JAW_LEFT:case BP_BOLT_JAW_RIGHT:
  case BP_BOLT_JAW_LEFT_ELITE:case BP_BOLT_JAW_RIGHT_ELITE:
  case BP_SNAPPER_JAW_LEFT:case BP_SNAPPER_JAW_RIGHT:case BP_SNAPPER_BASE:case BP_SNAPPER_SEGMENT:
  case BP_ECLIPSE_CHASSIS:case BP_ECLIPSE_PANEL:case BP_ECLIPSE_SHIELD:share=false;break;
 default:break;
 }
 if(kind==BP_ECLIPSE_SHIELD&&!burn)triggerEclipseShield(index,false);
 int64_t value=(int64_t)raw*globalDamagePermille()/1000;
 if(burn)value=value*burnDamagePercent()/100;
 else if(player.mods[M_BLITZ]&&p.hull>=p.max_hull*95/100)value=value*120/100;
 value=value*coefficient/1000;
 value=value*(100+e.vulnerability)/100;
 int damage=(int)(value>2000000000?2000000000:value);if(damage<1)damage=1;
 if(e.hard_shield>0){int absorbed=minv(e.hard_shield,damage);e.hard_shield-=absorbed;damage-=absorbed;creditDamage((unsigned)absorbed);emitFeedback(FeedbackDamage,frame,p.x,p.y,absorbed);p.flash=3;if(damage<=0)return false;}
 int actual=minv(p.hull,damage);p.hull-=actual;p.flash=3;
 if(kind==BP_BULWARK_BARRIER)p.timer=(uint16_t)minv(65535,(int)p.timer+actual);
 if(player.mods[M_TERMINATE]&&p.hull<p.max_hull*12/100){actual+=p.hull;p.hull=0;}
 creditDamage((unsigned)actual);emitFeedback(FeedbackDamage,frame,p.x,p.y,actual);
 // segmentLeaderDamage (0x14054c310) forwards the actual hull delta once.
 if(share){e.hull=maxv(0,e.hull-actual);e.flash=3;}
 if(burn&&player.mods[M_CORROSION])e.vulnerability=(uint8_t)minv(60,e.vulnerability+2);
 if(!burn&&player.mods[M_INCENDIARY]&&(!p.burn_time||damage/4>=absn(p.burn_damage))){p.burn_time=(uint16_t)igniteDurationTicks();p.burn_damage=(int16_t)clamp(damage/4,1,30000);}
 bool destroyed=p.hull<=0;
 if(destroyed){
#include "enemy_part_damage_snapper.inc"
#include "enemy_part_damage_eclipse.inc"
  // Original left/right Destroy events set the leader hull to zero. Damage
  // before destruction stays on the jaw; this is not damageLeader sharing.
  if(kind>=BP_BOLT_JAW_LEFT&&kind<=BP_BOLT_JAW_RIGHT_ELITE)e.hull=0;
  p.active=0;emitEffect(p.x,p.y,FX_BURST,p.radius+4,16,1);
  if(kind==BP_WORM_HEAD||kind==BP_WORM_BODY)awardEnemyPart(owner,1,1,true,p.x,p.y);
  else if(kind==BP_BULWARK_BARRIER)awardEnemyPart(owner,5,6,false,p.x,p.y);
  else if(kind==BP_BULWARK_CORE){
   awardEnemyPart(owner,1,6,true,p.x,p.y);
   for(unsigned i=0;i<MAX_BOSS_PARTS;++i){BossPart &shield=boss_parts[i];if(shield.active&&shield.owner==owner&&shield.kind==BP_BULWARK_BARRIER){awardEnemyPart(owner,5,6,false,shield.x,shield.y);shield.active=0;shield.hull=0;}}
  }
 }
 if(e.hull<=0){destroyEnemy(owner);return true;}
 return destroyed;
}

void Game::updateBossParts(unsigned owner){
 if(owner>=MAX_ENEMIES||!enemies[owner].active)return;
 Enemy &e=enemies[owner];if(!isBoss(e.type)&&e.type!=E_BULWARK&&e.type!=E_WORM&&e.type!=E_BOLT_THROWER&&e.type!=E_SNAPPER&&e.type!=E_ECLIPSE)return;
 unsigned existing=0,marker=1;
 // The source has 28 arrival/body segments, a compact entry state and 20 in
 // its second combat form. No animation frames are dropped to fit this pool.
 if(e.type==E_STAR_EATER)marker=e.form==2?3u:e.age>60?2u:1u;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i){BossPart &p=boss_parts[i];if(!p.max_hull||p.owner!=owner)continue;
  if(novaPartOwnerType(p.kind)!=e.type||(e.type==E_STAR_EATER&&p.timer!=marker)){clearBytes(&p,sizeof(p));continue;}++existing;}
 const auto put=[&](unsigned kind,unsigned slot,int sx100,int sy100,unsigned hull,bool mirror=false,int scale=1000){
  for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(!boss_parts[i].max_hull){
   BossPart &p=boss_parts[i];clearBytes(&p,sizeof(p));p.active=1;p.owner=(uint8_t)owner;p.kind=(uint8_t)kind;p.slot=(uint8_t)slot;p.mirror=(uint8_t)mirror;p.timer=(uint16_t)marker;
   // .5 source-art scale followed by the existing 4.6 world reduction.
   p.local_x=(int16_t)clamp(sx100*FP/920,-32767,32767);p.local_y=(int16_t)clamp(sy100*FP/920,-32767,32767);
   const BossPartArt &a=bossPartArt(kind);p.radius=(uint8_t)clamp(maxv(a.bounds_width,a.bounds_height)*scale/18400,2,90);
   p.max_hull=(int32_t)((int64_t)hull*e.max_hull/maxv(1,enemy_types[e.type].hull));p.hull=maxv(1,p.max_hull);p.max_hull=p.hull;
   p.x=e.x;p.y=e.y;p.angle=e.angle;return int(i);
  }
  return -1;
 };
 const auto polar=[&](unsigned kind,unsigned slot,int length100,int degrees,unsigned hull,bool mirror=false,int scale=1000){
  uint8_t a=(uint8_t)(-degrees*256/360);return put(kind,slot,dirX(a)*length100/FP,dirY(a)*length100/FP,hull,mirror,scale);
 };
#include "enemy_parts_snapper.inc"
#include "enemy_parts_eclipse.inc"
 if(e.type==E_BULWARK||e.type==E_WORM||e.type==E_BOLT_THROWER){
#include "enemy_parts_runtime.inc"
  return;
 }
 if(!existing){
  switch(e.type){
  case E_DREADNOUGHT:
   put(BP_DREAD_WING,0,0,0,65000);put(BP_DREAD_WING,1,0,0,65000,true);
   polar(BP_DREAD_GUN_LARGE,2,24525,120,5000);polar(BP_DREAD_GUN_LARGE,3,24525,-120,5000,true);
   polar(BP_DREAD_GUN_SMALL,4,27180,82,5000);polar(BP_DREAD_GUN_SMALL,5,27180,-82,5000,true);break;
  case E_WARBRINGER:
   put(BP_WAR_CHASSIS,0,0,0,18500);
   for(unsigned side=0;side<2;++side){int sign=side?1:-1;
    put(BP_WAR_WING,1+side,0,0,5750,side!=0);
    put(BP_WAR_THRUSTER,3+side,-12000,sign*9000,5750,side!=0);
    put(BP_WAR_BROADSIDE,5+side,0,sign*10000,11500,side!=0);}
   for(unsigned j=0;j<5;++j){polar(BP_WAR_LASER,7+j,8000,int(j)*30-60,12000);}
   break;
  case E_CERATOTITAN:
   put(BP_CERA_THORAX,0,0,0,25000);
   polar(BP_CERA_PINCER,1,3500,26,15000);polar(BP_CERA_PINCER,2,3500,-26,15000,true);
   polar(BP_CERA_ELYTRON,3,1500,90,8000);polar(BP_CERA_ELYTRON,4,1500,-90,8000,true);break;
  case E_GLAUCUS:
   for(unsigned j=0;j<28;++j)put(BP_GLAUCUS_SEGMENT,j,-int(j+1)*7000,0,20000,false,maxv(400,1100-int(j)*22));
   put(BP_GLAUCUS_EYE,31,3000,0,10000);break;
  case E_STAR_EATER:{
   unsigned count=marker==1?1u:marker==2?28u:20u;
   put(BP_STAR_ABDOMEN,0,-12000,0,20000,false,600);
   for(unsigned j=1;j<count;++j)put(BP_STAR_SEGMENT,j,-int(j)*11000,0,20000,false,maxv(350,800-int(j)*12));
   put(BP_STAR_EYE,31,0,0,20000);break;
  }
  case E_STATION_OMEGA:
   for(unsigned j=0;j<5;++j){polar(BP_STATION_NODE,j,32200,int(j)*72,3600);polar(BP_STATION_WALL,5+j,30500,int(j)*72+36,20400);}break;
  case E_CRUCIBLE:
   // These are temporary arena burns, not boss health components. Their
   // source baseDamage=1.2/frame and they do not absorb player projectiles.
   for(unsigned j=0;j<2;++j){int n=put(BP_CRUCIBLE_CORNER,j,0,0,1);if(n>=0){boss_parts[n].active=0;boss_parts[n].timer=0;}}
   {int n=put(BP_CRUCIBLE_EDGE,2,0,0,1);if(n>=0){boss_parts[n].active=0;boss_parts[n].timer=0;}}
   break;
  default:break;
  }
 }
 int32_t previous_x=e.x,previous_y=e.y;unsigned previous_slot=999;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i){BossPart &p=boss_parts[i];if(!p.max_hull||p.owner!=owner)continue;
  if(p.flash)--p.flash;
  if(p.kind>=BP_CRUCIBLE_CORNER){
   bool edge=p.kind==BP_CRUCIBLE_EDGE;
   if(!e.phase_age&&((edge&&(e.phase==3||e.phase==6||e.phase==8))||(!edge&&(e.phase==4||e.phase==8)))){p.active=1;p.timer=300;}
   if(!p.active)continue;
   if(!p.timer){p.active=0;continue;}
   unsigned elapsed=300-p.timer;--p.timer;
   int len100=edge?160000-int(elapsed)*170-int(elapsed*elapsed)*17/60:25000+int(elapsed)*100+int(elapsed*elapsed)/3;
   int step=edge?70:79,turn=int(elapsed)*step*256/36000;
   uint8_t a=(uint8_t)((edge?0:96+int(p.slot)*128)+((e.phase&1)?turn:-turn));
   int distance=maxv(0,len100)*FP/920;
   const BossPartArt &art=bossPartArt(p.kind);int major=maxv(art.bounds_width,art.bounds_height);
   p.angle=(uint8_t)(a+(edge?0:32));
   int cx=art.center_x*int(p.radius)*2*FP/major,cy=art.center_y*int(p.radius)*2*FP/major;
   p.x=e.x+dirX(a)*distance/FP+(cx*dirX(p.angle)-cy*dirY(p.angle))/FP;
   p.y=e.y+dirY(a)*distance/FP+(cx*dirY(p.angle)+cy*dirX(p.angle))/FP;
   if(elapsed>=15&&!player.invulnerable&&sweptBossPart(p,player.x,player.y,player.x,player.y,player.radius)){
    damagePlayer(2+(frame%5<2));if(player.invulnerable==7)player.invulnerable=0;
   }
   continue;
  }
  if(!p.active)continue;
  const BossPartArt &a=bossPartArt(p.kind);int major=maxv(a.bounds_width,a.bounds_height);
  int offset=0;
  if(p.kind==BP_CERA_PINCER){unsigned phase=e.phase;int degrees=(phase==5?50:phase==6?35:phase==2||phase==7||phase==9||phase==11?85:phase==3||phase==8||phase==10||phase==12?95:20);offset=(p.mirror?1:-1)*degrees*256/360;}
  if(p.kind==BP_CERA_ELYTRON&&(e.phase==5||e.phase==6))offset=(p.mirror?1:-1)*32;
  if(p.kind==BP_WAR_LASER)offset=(int(p.slot)-9)*21;
  if(p.kind==BP_STATION_NODE)offset=-int(p.slot)*256/5;
  if(p.kind==BP_STATION_WALL)offset=-int(p.slot-5)*256/5-26;
  if(p.kind==BP_CRUCIBLE_CORNER)offset=(p.slot&1?64:0)+(p.slot&2?128:0);
  if(p.kind==BP_CRUCIBLE_EDGE)offset=int(p.slot-4)*64;
  p.angle=(uint8_t)(e.angle+offset);
  bool chain=(p.kind==BP_GLAUCUS_SEGMENT||p.kind==BP_STAR_SEGMENT||p.kind==BP_STAR_ABDOMEN);
  if(chain){
   int spacing=p.kind==BP_STAR_ABDOMEN?22:p.kind==BP_STAR_SEGMENT?(p.slot<5?18:12):6;
   uint8_t follow=direction(previous_x-p.x,previous_y-p.y);
   if(!existing||previous_slot==999){p.x=previous_x-dirX(e.angle)*spacing;p.y=previous_y-dirY(e.angle)*spacing;follow=e.angle;}
   else{int dx=int((previous_x-p.x)/FP),dy=int((previous_y-p.y)/FP),distance=approx(dx,dy);
    if(distance>spacing){int32_t targetx=previous_x-dirX(follow)*spacing,targety=previous_y-dirY(follow)*spacing;p.x+=(targetx-p.x)/2;p.y+=(targety-p.y)/2;}}
   p.angle=follow;p.x=clamp(p.x,-4000*FP,4000*FP);p.y=clamp(p.y,-4000*FP,4000*FP);
   previous_x=p.x;previous_y=p.y;previous_slot=p.slot;continue;
  }
  int c=dirX(e.angle),s=dirY(e.angle);
  int32_t base_x=e.x+(p.local_x*c-p.local_y*s)/FP,base_y=e.y+(p.local_x*s+p.local_y*c)/FP;
  int center_x=a.center_x*int(p.radius)*2*FP/major;
  int center_y=a.center_y*int(p.radius)*2*FP/major;if(p.mirror)center_y=-center_y;
  int pc=dirX(p.angle),ps=dirY(p.angle);
  p.x=base_x+(center_x*pc-center_y*ps)/FP;p.y=base_y+(center_x*ps+center_y*pc)/FP;
  if(!novaChainBoss(e.type)){p.x=wrap(p.x,worldWidth());p.y=wrap(p.y,worldHeight());}
 }
}
