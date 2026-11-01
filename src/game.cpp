#include "game.h"
#include "visual_feedback.h"
namespace nova {
namespace {
const int16_t sine_quarter[65]={0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,
 98,104,109,115,121,126,132,137,142,147,152,157,162,167,172,177,
 181,185,190,194,198,202,206,209,213,216,220,223,226,229,231,234,
 237,239,241,243,245,247,248,250,251,252,253,254,255,255,256,256,256};
int absn(int x) {return x<0?-x:x;}
unsigned isqrt32(uint32_t value) {uint32_t root=0,bit=1u<<30;while(bit>value)bit>>=2;while(bit){if(value>=root+bit){value-=root+bit;root=(root>>1)+bit;}else root>>=1;bit>>=2;}return root;}
int minv(int a,int b) {return a<b?a:b;}
int maxv(int a,int b) {return a>b?a:b;}
int clamp(int a,int lo,int hi) {return a<lo?lo:a>hi?hi:a;}
int32_t wrap(int32_t n,int size) {
 const int s=size*FP;
 while(n<0)n+=s;
 while(n>=s)n-=s;
 return n;
}
int delta(int32_t a,int32_t b,int dimension) {
 int d=(a-b)/FP;
 if(d>dimension/2)d-=dimension;
 if(d<-dimension/2)d+=dimension;
 return d;
}
int approx(int dx,int dy) { dx=absn(dx);dy=absn(dy);return maxv(dx,dy)+minv(dx,dy)/2; }
int turnDelta(uint8_t from,uint8_t to) {int d=(int)to-from;if(d>127)d-=256;if(d<-128)d+=256;return d;}
uint8_t toward(uint8_t from,uint8_t to,int step) { return (uint8_t)(from+clamp(turnDelta(from,to),-step,step)); }
void clearBytes(void *ptr,unsigned n) {uint8_t *p=(uint8_t*)ptr;while(n--)*p++=0;}
// calculateMaxExp, 0x1400bc220: floor(14+13*baseLevel+6*2^(.234*baseLevel)).
// The first threshold is the controller's explicit 15. Values beyond the
// representable table saturate; no floating point enters the ARM core.
uint32_t experienceForLevel(unsigned level) {
 static const uint32_t thresholds[]={15,34,48,62,77,92,107,123,139,156,174,192,212,232,254,277,302,329,359,391,427,467,512,563,620,685,759,843,940,1053,1182,1332,1507,1709,1945,2221,2542,2918,3358,3873,4476,5183,6013,6986,8129,9470,11045,12895,15069,17624,20625,24154,28301,33176,38907,45645,53568,62883,73836,86716,101861,119671,140615,165245,194210,228273,268331,315442,370845,436003,512632,602752,708740,833388,979984,999999,999999,999999,999999,999999};
 return thresholds[minv(79,maxv(0,(int)level))];
}
bool isBoss(unsigned type) {return enemyIsBoss(type);}
bool physicalCore(unsigned type) {return type!=E_STATION_OMEGA&&type!=E_STAR_EATER&&type!=E_WORM&&type!=E_BULWARK;}
uint32_t addSaturated(uint32_t a,uint32_t b) {return 0xffffffffu-a<b?0xffffffffu:a+b;}
bool modDescendsFrom(unsigned id,unsigned first,unsigned last,unsigned depth=0) {
 if(id>=MOD_COUNT||depth>12)return false;
 if(id>=first&&id<=last)return true;
 const ModDef &m=mods[id];
 return (m.prerequisite_a>=0&&modDescendsFrom(m.prerequisite_a,first,last,depth+1))||
        (m.prerequisite_b>=0&&modDescendsFrom(m.prerequisite_b,first,last,depth+1));
}



}
int dirY(uint8_t a) {
 unsigned quadrant=a>>6,index=a&63;
 int v=(quadrant&1)?sine_quarter[64-index]:sine_quarter[index];
 return quadrant>=2?-v:v;
}
int dirX(uint8_t a) {return dirY((uint8_t)(a+64));}
uint8_t direction(int32_t dx,int32_t dy) {
 // Octant lookup by binary search: no atan or per-call 256-angle scan.
 if(dx==0&&dy==0)return 0;
 int ax=absn(dx),ay=absn(dy);
 while(ax>65535||ay>65535) {ax/=2;ay/=2;}
 unsigned lo=0,hi=64;
 while(lo<hi) {unsigned mid=(lo+hi)/2;
  if(sine_quarter[mid]*ax<sine_quarter[64-mid]*ay)lo=mid+1;else hi=mid;}
 unsigned angle=lo;
 if(dx<0)angle=128-angle;
 if(dy<0)angle=256-angle;
 return (uint8_t)angle;
}
int Game::worldWidth() const {return WIDTH*(100+7*player.mods[M_X_FARSIGHT]+(dailyMod(DAILY_LONG_SHOT)?50:0))/100;}
int Game::worldHeight() const {return HEIGHT*(100+7*player.mods[M_X_FARSIGHT]+(dailyMod(DAILY_LONG_SHOT)?50:0))/100;}
bool Game::screenWrappingEnabled() const {
 if(dailyMod(DAILY_CAGE_MATCH))return false;
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&enemies[i].type==E_CRUCIBLE&&enemies[i].age>=90)return false;
 return true;
}
int32_t Game::playerDestination(int32_t coordinate,bool horizontal) const {
 int extent=horizontal?worldWidth():worldHeight();
 return screenWrappingEnabled()?wrap(coordinate,extent):clamp(coordinate,player.radius*FP,(extent-player.radius)*FP-1);
}
int Game::distance2(int32_t ax,int32_t ay,int32_t bx,int32_t by) const {
 int dx=delta(ax,bx,worldWidth()),dy=delta(ay,by,worldHeight());
 return dx*dx+dy*dy;
}
// A swept circle in integer pixel space, including the shortest toroidal
// displacement. Avoids both tunnelling and 64-bit division on ARM926.
bool Game::sweptHit(int32_t oldx,int32_t oldy,int32_t newx,int32_t newy,
              int32_t targetx,int32_t targety,int radius) const {
 int dx=delta(newx,oldx,worldWidth()),dy=delta(newy,oldy,worldHeight());
 int tx=delta(targetx,oldx,worldWidth()),ty=delta(targety,oldy,worldHeight());
 int length=dx*dx+dy*dy;
 if(!length)return tx*tx+ty*ty<=radius*radius;
 int projection=clamp(tx*dx+ty*dy,0,length);
 int qx=tx-(dx*projection)/length,qy=ty-(dy*projection)/length;
 return qx*qx+qy*qy<=radius*radius;
}
bool Game::lineHit(int32_t ox,int32_t oy,int32_t nx,int32_t ny,int32_t tx,int32_t ty,unsigned radius) const {
 int dx=(nx-ox)/FP,dy=(ny-oy)/FP,x=(tx-ox)/FP,y=(ty-oy)/FP;
 int length=dx*dx+dy*dy;if(!length)return x*x+y*y<=(int)(radius*radius);
 int projection=clamp(x*dx+y*dy,0,length);
 int qx=x-(int)((int64_t)dx*projection/length),qy=y-(int)((int64_t)dy*projection/length);
 return qx*qx+qy*qy<=(int)(radius*radius);
}
uint32_t Game::random() {uint32_t x=random_;x^=x<<13;x^=x>>17;x^=x<<5;random_=x;return x;}
unsigned Game::range(unsigned m) {return m?random()%m:0;}
void Game::resetInput() {clearBytes(&previous_,sizeof(previous_));accumulator_=0;}
#include "progression.inc"
void Game::start(uint32_t run_seed) {
 clearBytes(this,sizeof(*this));resetFeedback();
 seed=run_seed?run_seed:0x4e4f5641u;random_=seed;state=Playing;vortex_scale_=100000;vortex_rate_=12375000;thermal_x_scale_=thermal_y_scale_=100000;
 player.x=worldWidth()*FP/2;player.y=worldHeight()*FP/2;player.angle=192;
 sanctuary_x=player.x;sanctuary_y=player.y;options.auto_deploy_mask=15;options.assist_mode=0;
 player.weapon=W_BLASTER;player.body=B_STANDARD;player.shield_type=S_STANDARD;
 level=0;xp_next=15;rerolls=3;next_wave=1;offer_kind_=OFFER_MOD;offer_count=OFFER_COUNT;boss_index=-1;auto_fire=false;
 recalculate();player.hull=(int16_t)hullRecoveryCap();player.shield=(int16_t)shieldRecoveryCap();
 player.invulnerable=60;player.missiles=0;
 for(unsigned i=0;i<MAX_SEGMENTS;i++){segment_x[i]=player.x;segment_y[i]=player.y;}
}
void Game::configureRun(uint16_t flags) {
 if(frame)return;
 Career allowed={};allowed.rank=account_rank;allowed.final_boss_kills=final_boss_unlocked?1:0;
 for(unsigned bit=1;bit<=RUN_PRACTICE;bit<<=1)if((flags&bit)&&!allowed.modeUnlocked(bit))flags=(uint16_t)(flags&~bit);
 const unsigned challenges=RUN_DANGER|RUN_ELITE|RUN_NEMESIS|RUN_HOSTILE|RUN_ENEMY_TERRITORY;
 if(flags&RUN_ANNIHILATION)flags|=RUN_MAYHEM;
 if(flags&RUN_MAYHEM)flags|=challenges;
 run_flags=flags&4095;wild_mode=(flags&RUN_WILD)!=0;
 if(flags&RUN_ENEMY_TERRITORY)wave=(flags&RUN_MAYHEM)?15:12;
 wave_pattern_=(uint8_t)range(32);
 if(flags&RUN_DRAFT){wave+=120;rerolls+=10;bonusLevels(35);beginUpgrade();}
 if(wild_mode)rerolls=(uint16_t)maxv(1,rerolls/2);
 starting_wave=wave;
 recalculate();player.hull=(int16_t)hullRecoveryCap();player.shield=(int16_t)shieldRecoveryCap();
}
unsigned Game::scoreBonus() const {
 unsigned n=(run_flags&RUN_ENDLESS)?50:0;
 for(unsigned bit=RUN_DANGER;bit<=RUN_ENEMY_TERRITORY;bit<<=1)if(run_flags&bit)n+=25;
 if(run_flags&RUN_MAYHEM)n+=25;
 if(daily(DAILY_BOSS_RUSH))n=(100+n)*12-100;
 else if(daily(DAILY_TURBO)||daily(DAILY_TURBO_ENDLESS))n=(100+n)*2-100;
 return n;
}
unsigned Game::experienceBonus() const {
 unsigned n=(run_flags&RUN_ENDLESS)?6:0;
 for(unsigned bit=RUN_DANGER;bit<=RUN_ENEMY_TERRITORY;bit<<=1)if(run_flags&bit)n+=4;
 if(run_flags&RUN_MAYHEM)n+=4;
 if(daily(DAILY_BOSS_RUSH))n=(100+n)*12-100;
 else if(daily(DAILY_TURBO)||daily(DAILY_TURBO_ENDLESS))n=(100+n)*2-100;
 return n;
}
bool Game::validState() const {
 if((run_flags&RUN_DAILY)&&(daily_variant>=DAILY_VARIANT_COUNT||daily_modifier>=DAILY_MODIFIER_COUNT||daily_egg_hull<0||daily_egg_hull>15000||daily_boss_step_>11))return false;
 for(unsigned i=0;i<MAX_COSMIC_OBJECTS;i++){const CosmicObject &c=cosmic_objects[i];if(c.active&&(c.kind>=COSMIC_KIND_COUNT||c.radius>40||c.warning>300||c.x< -4096*FP||c.x>4096*FP||c.y< -4096*FP||c.y>4096*FP||c.vx< -64*FP||c.vx>64*FP||c.vy< -64*FP||c.vy>64*FP))return false;}
 if(state<Title||state>GameOver||player.weapon>=WEAPON_COUNT||player.body>=BODY_COUNT||
    player.shield_type>=SHIELD_COUNT||selected_offer>=offer_count||offer_count<1||offer_count>OFFER_COUNT||level>999||
    wave>30000||starting_wave>wave||account_rank>64||run_flags>4095||xp_next==0||xp_next>1000000||player.max_hull<=0||
    player.max_hull>10000||player.max_shield<0||player.max_shield>10000||
    player.hull<0||player.hull>player.max_hull||player.shield<0||
    player.shield>player.max_shield||segment_count>MAX_SEGMENTS||!random_||
    accumulator_>=1000||banked_upgrades>999||boss_index<-1||boss_index>=MAX_ENEMIES||
    wave_queue_count_>SOURCE_MAX_SPECS||options.auto_deploy_mask>15||options.assist_mode>1)return false;
 if(player.x<0||player.x>=worldWidth()*FP||player.y<0||player.y>=worldHeight()*FP||
    player.vx < -32*FP||player.vx>32*FP||player.vy < -32*FP||player.vy>32*FP)return false;
 for(unsigned i=0;i<MOD_COUNT;i++)if(player.mods[i]>mods[i].max_rank)return false;
 for(unsigned i=0;i<offer_count;i++) {
  const Offer &o=offers[i];if(o.kind>OFFER_SHIELD)return false;
  unsigned limit=o.kind==OFFER_MOD?(unsigned)MOD_COUNT:o.kind==OFFER_WEAPON?(unsigned)WEAPON_COUNT:o.kind==OFFER_BODY?(unsigned)BODY_COUNT:(unsigned)SHIELD_COUNT;
  if(o.id>=limit)return false;
 }
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active){const Enemy &e=enemies[i];if(e.type>=ENEMY_COUNT||e.aura_fraction>=6000||e.x<0||e.x>=worldWidth()*FP||e.y<0||e.y>=worldHeight()*FP||e.radius>64||e.hull<0||e.hull>e.max_hull||e.max_hull<=0||e.hard_shield<0||e.vx < -64*FP||e.vx>64*FP||e.vy < -64*FP||e.vy>64*FP||e.leader<-1||e.leader>=MAX_ENEMIES||e.damage_scale>4096||e.treasure_scale>4096||e.size_scale>4096||e.damage_fraction>=20||e.challenger>1)return false;}
 for(unsigned i=0;i<MAX_BOSS_PARTS;i++)if(boss_parts[i].active){const BossPart &p=boss_parts[i];if(p.owner>=MAX_ENEMIES||!enemies[p.owner].active||p.kind>=BP_COUNT||p.radius>100||p.hull<=0||p.hull>p.max_hull||p.x < -4096*FP||p.x>4096*FP||p.y < -4096*FP||p.y>4096*FP)return false;}
 for(unsigned i=0;i<MAX_BULLETS;i++)if(bullets[i].active){const Bullet &b=bullets[i];if(b.kind>=SHOT_KIND_COUNT||b.owner>1||b.radius>100||(b.kind==SHOT_MYRMIDON_BLAST?(b.x < -100*FP||b.x>(worldWidth()+100)*FP||b.y < -100*FP||b.y>(worldHeight()+100)*FP):(b.x<0||b.x>=worldWidth()*FP||b.y<0||b.y>=worldHeight()*FP))||(b.kind==SHOT_DISCHARGE?(b.vx < -worldWidth()*FP||b.vx>worldWidth()*FP||b.vy < -worldHeight()*FP||b.vy>worldHeight()*FP):(b.vx < -64*FP||b.vx>64*FP||b.vy < -64*FP||b.vy>64*FP)))return false;}
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active){const Construct &c=constructs[i];if(c.kind>=C_KIND_COUNT||c.x<0||c.x>=worldWidth()*FP||c.y<0||c.y>=worldHeight()*FP||c.radius>64)return false;}
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&(constructs[i].kind==C_SWORD||constructs[i].kind==C_BASTION)){
  const AdvancedConstructState &a=advanced_constructs[i];
  if(a.state>(constructs[i].kind==C_BASTION?1u:2u)||a.temporary>1||a.side>1||a.wielder>MAX_CONSTRUCTS||a.duration>200||a.clock>a.duration||a.damage<0||a.damage>2000000)return false;
  if(constructs[i].kind==C_SWORD&&a.state&&!a.duration)return false;
  if(a.temporary&&(a.anchor_x<0||a.anchor_x>=worldWidth()*FP||a.anchor_y<0||a.anchor_y>=worldHeight()*FP))return false;
 }
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(constructs[i].active&&constructs[i].kind==C_INTERCEPTOR){
  const InheritedWeaponState &w=advanced_constructs[i].weapon;
  if(w.weapon>WEAPON_COUNT||w.charging>1||w.thermal_x>64000000||w.thermal_y>64000000||w.magazine>4095||w.calibration>180||w.innate_fraction>=100||w.damage_fraction>=1000)return false;
 }
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++){const PersistentField &f=persistent_fields[i];if(f.active>1)return false;if(f.active&&(f.x<0||f.x>=worldWidth()*FP||f.y<0||f.y>=worldHeight()*FP||f.damage<0||f.damage>2000000||!f.base_radius||f.reserved[0]>2||f.base_radius>(f.reserved[0]==2?65535u:f.reserved[0]?4096u:100u)||f.radius>(f.reserved[0]?4096u:100u)||f.source_age>=(f.reserved[0]==2?35u:f.reserved[0]?51u:17u)))return false;}
 if(maelstrom_state.active>1||thermal_x_scale_>64000000||thermal_y_scale_>64000000||vortex_scale_>64000000||rancor_scale_>64000000||vortex_rate_>12375000||vortex_growth_fraction_>=1000000||vortex_cost_fraction_>=1000||vortex_damage_fraction_>=1000)return false;
 if(run_stats.seen_enemies_hi>>((ENEMY_COUNT-64)&63))return false;
 for(unsigned i=0;i<MAX_BULLETS;i++){const Bullet &b=bullets[i];if(b.active&&b.kind==SHOT_DISCHARGE&&(b.life>10||b.traveled>=512||b.speed>255||b.damage<0||b.damage>2000000||b.size_percent>8192))return false;}
 for(unsigned i=0;i<MAX_BULLETS;i++){const Bullet &b=bullets[i];if(b.active&&((b.kind==SHOT_VORTEX&&b.field_scale>64000000)||(b.tracking&&(b.tracking_weapon>=WEAPON_COUNT||b.tracking_target<-1||b.tracking_target>=MAX_ENEMIES+MAX_BOSS_PARTS))))return false;}
 for(unsigned i=0;i<wave_queue_count_;i++){
  const WaveSpawnQueue &q=wave_queue_[i];
  if(q.spec.enemy<0||q.spec.enemy>=SOURCE_ENEMY_COUNT||q.leader<-1||q.leader>=MAX_ENEMIES||q.done>1||q.edge>3)return false;
 }
 return true;
}
unsigned Game::enemyCount() const {unsigned n=0;for(unsigned i=0;i<MAX_ENEMIES;i++)n+=enemies[i].active!=0;return n;}
unsigned Game::bulletCount() const {unsigned n=0;for(unsigned i=0;i<MAX_BULLETS;i++)n+=bullets[i].active!=0;return n;}
unsigned Game::constructCount() const {unsigned n=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)n+=constructs[i].active!=0;return n;}
bool Game::canTakeMod(unsigned id) const {
 if(daily(DAILY_PURITY)&&run_stats.mods_taken>=7)return false;
 if(daily(DAILY_NOVA_BROTHERS)&&id==M_WINGMAN)return false;
 if(id>=MOD_COUNT||account_rank<offerUnlockRank(OFFER_MOD,id)||!mods[id].implemented||banished_[id]||player.mods[id]>=mods[id].max_rank)return false;
 if(mods[id].category!=2&&player.mods[M_X_HIDDEN_POWER]&&(modDescendsFrom(id,M_VOLLEY,M_CONVERGENCE)||modDescendsFrom(id,M_DRONES,M_OVERCLOCK)))return false;
 if(mods[id].category==2&&!wild_mode)return false;
 if(id==M_X_THE_VOID&&!player.mods[M_X_EVOLUTIONARY_NICHE])return false;
 if(id==M_X_ATARAXIA&&banked_upgrades<4)return false;
 if(id==M_X_MASTERY){unsigned n=0;for(unsigned i=0;i<=M_CONVERGENCE;i++)n+=player.mods[i];if(n<10)return false;}
 if(id==M_X_LAST_STAND){unsigned n=0;for(unsigned i=M_HULL_STRENGTH;i<=M_BLINK;i++)n+=player.mods[i];for(unsigned i=M_SKIRMISH;i<=M_AEGIS;i++)n+=player.mods[i];if(n<10)return false;}
 if(id==M_X_HIDDEN_POWER){unsigned n=0;static const uint8_t secondary[]={M_BLAST_RADIUS,M_RUPTURE,M_INCENDIARY,M_SPLINTER,M_WEAPONIZED_SHIELDS,M_VOLATILE_SHIELDS,M_DISCHARGE,M_DEADLY_WAKE,M_SELF_DESTRUCTION,M_PURGE,M_X_LANCE,M_X_DYING_STAR,M_X_DISCORD,M_X_SCORCHING_WAKE};for(unsigned i=0;i<sizeof(secondary);i++)n+=player.mods[secondary[i]]!=0;
  n+=(player.body==B_FIREFLY||player.body==B_VIPER);n+=(player.shield_type==S_HALO||player.shield_type==S_SIPHON||player.shield_type==S_WARP);if(n<4)return false;}
 if(id==M_X_APEX_MACHINERY||id==M_X_VITAL_BOND){bool final=player.mods[M_X_FLEET]||player.mods[M_X_PREDATOR_DRONES]||player.mods[M_X_FLOTILLA]||player.mods[M_COUNTER_ARTILLERY]||player.mods[M_WINGMAN]||player.mods[M_X_NANOTECH_DISSEMINATION]||player.mods[M_LOADED_MINES];if(!final)return false;}
 const ModDef &m=mods[id];
 if(m.prerequisite_a>=0&&!player.mods[m.prerequisite_a])return false;
 if(m.prerequisite_b>=0&&!player.mods[m.prerequisite_b])return false;
 if((id==M_DEFENSE_DRONES&&player.mods[M_ASSAULT_DRONES])||
    (id==M_ASSAULT_DRONES&&player.mods[M_DEFENSE_DRONES]))return false;
 if((id==M_GUARDIAN&&player.mods[M_INTERCEPTOR])||(id==M_INTERCEPTOR&&player.mods[M_GUARDIAN]))return false;
 if(mods[id].category!=2&&player.mods[M_RAPID_RECONSTRUCTION]&&modDescendsFrom(id,M_SHIELD_DURABILITY,M_EMERGENCY_SYSTEMS))return false;
 if(id==M_RAPID_RECONSTRUCTION&&player.mods[M_OMNI_SHIELD])return false;
 return true;
}
bool Game::takeMod(unsigned id) {
 if(!canTakeMod(id))return false;
 unsigned old_rank=player.mods[id];
 ++player.mods[id];run_stats.mods_taken=(uint16_t)minv(65535,run_stats.mods_taken+1);int oldh=player.max_hull,olds=player.max_shield;
 if(mods[id].max_rank>1&&id!=M_X_THE_VOID){
  unsigned bonus=revelation_+3*(player.mods[M_X_EXPLOSIVE_GROWTH]&&!banished_[M_X_EXPLOSIVE_GROWTH])+player.mods[M_X_OBSESSION];
  if(bonus){player.mods[id]=(uint8_t)minv(mods[id].max_rank,player.mods[id]+bonus);extra_ranks_=(uint8_t)minv(40,extra_ranks_+player.mods[M_X_OBSESSION]);revelation_=0;
   if(player.mods[M_X_EXPLOSIVE_GROWTH]&&!banished_[M_X_EXPLOSIVE_GROWTH]){banished_[id]=1;banished_[M_X_EXPLOSIVE_GROWTH]=1;}}
 }
 if(id==M_X_CHAOTIC_AMBITION&&!player.mods[M_X_POWER_SPIKE])rerolls=(uint16_t)minv(999,rerolls+player.mods[id]-old_rank);
 if(id==M_RAPID_RECONSTRUCTION&&(gear_owned_&(1u<<OFFER_SHIELD))){gear_owned_=(uint8_t)(gear_owned_&~(1u<<OFFER_SHIELD));banked_upgrades=(uint16_t)minv(999,banked_upgrades+1);}
 if(id==M_X_HIDDEN_POWER&&(gear_owned_&(1u<<OFFER_WEAPON))){gear_owned_=(uint8_t)(gear_owned_&~(1u<<OFFER_WEAPON));banked_upgrades=(uint16_t)minv(999,banked_upgrades+1);}
 if(id==M_X_DISPLACEMENT)void_charges_=2;
 if(id==M_X_REVELATION)revelation_=2;
 if(id==M_X_TRANSMOGRIFICATION)transmog_=3;
 if(id==M_X_WINNOW||id==M_X_REGRESSION){
  for(unsigned i=0;i<OFFER_COUNT;i++)if(offers[i].kind==OFFER_MOD&&offers[i].id!=id&&
   (id==M_X_WINNOW||mods[offers[i].id].prerequisite_a<0))banished_[offers[i].id]=1;
  if(id==M_X_WINNOW&&!player.mods[M_X_POWER_SPIKE])rerolls=(uint16_t)minv(99,rerolls+3);
  if(id==M_X_REGRESSION)bonusLevels(1);
 }
 if(id==M_X_GRANDEUR)bonusLevels(1);
 if(id==M_X_POWER_SPIKE){rerolls=0;bonusLevels(5);}
 if(id==M_X_CHALLENGER)queueChallenger();
 if(id==M_X_HIDDEN_POWER){for(unsigned i=0;i<MAX_CONSTRUCTS;i++)constructs[i].active=0;burst_left_=burst_delay_=0;}
 recalculate();player.hull=minv(hullRecoveryCap(),player.hull+maxv(0,player.max_hull-oldh));
 player.shield=minv(shieldRecoveryCap(),player.shield+maxv(0,player.max_shield-olds));
 return true;
}
bool Game::equipWeapon(unsigned id) {if((daily(DAILY_SWORD)&&id!=W_SWORDS)||dailyMod(DAILY_CHOSEN))return false;if(id>=WEAPON_COUNT||account_rank<offerUnlockRank(OFFER_WEAPON,id))return false;player.weapon=(uint8_t)id;player.charge=0;charged_scale_=rancor_scale_=0;vortex_scale_=100000;vortex_rate_=12375000;thermal_x_scale_=thermal_y_scale_=100000;vortex_growth_fraction_=0;vortex_cost_fraction_=0;player.fire_delay=0;player.missiles=0;player.missile_clock=0;recalculate();return true;}
bool Game::equipBody(unsigned id) {
 if(daily(DAILY_SERPENTINE)||dailyMod(DAILY_CHOSEN))return false;
 if(id>=BODY_COUNT||account_rank<offerUnlockRank(OFFER_BODY,id))return false;
 int old=player.max_hull;player.body=(uint8_t)id;
 recalculate();player.hull=clamp(player.hull+player.max_hull-old,1,hullRecoveryCap());
 player.shield=minv(player.shield,shieldRecoveryCap());
 // Reassemble under the new body's construct limits.
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)constructs[i].active=0;
 for(unsigned i=0;i<MAX_SEGMENTS;i++){segment_x[i]=player.x;segment_y[i]=player.y;}
 return true;
}
bool Game::equipShield(unsigned id) {if(dailyMod(DAILY_CHOSEN))return false;if(id>=SHIELD_COUNT||account_rank<offerUnlockRank(OFFER_SHIELD,id)||player.mods[M_RAPID_RECONSTRUCTION])return false;player.shield_type=(uint8_t)id;recalculate();player.shield=(int16_t)shieldRecoveryCap();player.shield_delay=0;return true;}
void Game::recalculate() {
 const uint8_t *m=player.mods;const BodyDef &b=bodies[player.body];
 clearBytes(&stats,sizeof(stats));
 stats.damage=100+15*m[M_MAGNITUDE]+30*m[M_PAYLOAD]+5*m[M_VELOCITY]-20*m[M_PURIFICATION];
 stats.fire_rate=100+20*m[M_BURST_FIRE]+15*m[M_RAPID_FIRE]-12*m[M_PAYLOAD]-5*m[M_GUIDANCE]-15*m[M_CONVERGENCE]-15*m[M_X_SINGULAR_STRIKE_NEW]+5*m[M_TERMINATE];
 stats.velocity=100+20*m[M_VELOCITY]+20*m[M_SNIPE]-15*m[M_CONVERGENCE];
 stats.blast=100+10*m[M_BLAST_RADIUS]+20*m[M_HIGH_EXPLOSIVE]-30*m[M_CONCENTRATED_BLAST]+5*m[M_MAGNITUDE]+5*m[M_PAYLOAD];
 stats.size=(100+20*m[M_MAGNITUDE]+12*m[M_PAYLOAD])*(100-15*m[M_CONVERGENCE])/100;
 stats.thrust=b.thrust*(100+25*m[M_THRUSTERS]+15*m[M_STREAMLINE]+10*m[M_AGILITY]+10*m[M_POWER_RESERVES]-25*m[M_JUGGERNAUT])/100;
 stats.turn=3+2*m[M_AGILITY]+m[M_STABILIZATION];
 stats.plating=3*m[M_ABSORPTION]+2*m[M_RAPID_RECONSTRUCTION];
 stats.homing=m[M_TARGETING]+2*m[M_GUIDANCE]+3*m[M_CONVERGENCE];
 if(m[M_X_WARP_STRIKE])stats.homing/=2;
 stats.projectiles=2*m[M_VOLLEY]+2*m[M_CONVERGENCE];
 stats.shield_radius=shields[player.shield_type].radius*(100+25*m[M_SHIELD_RADIUS]-20*m[M_FOCUSED_SHIELDS]+50*m[M_RADIANT_SHIELDS]+5*m[M_X_HIDDEN_POWER]+10*m[M_X_TEMPEST_BREAK])/100;
 stats.shield_effect=100+10*m[M_SHIELD_RADIUS]+25*m[M_FOCUSED_SHIELDS]+15*m[M_OMNI_SHIELD]+5*m[M_X_HIDDEN_POWER]+60*m[M_WEAPONIZED_SHIELDS];
 stats.shield_cooldown=shields[player.shield_type].cooldown*(100-20*m[M_SHIELD_COOLDOWN]+10*m[M_RADIANT_SHIELDS]+5*m[M_BARRIER]-10*m[M_X_HIDDEN_POWER]+7*m[M_X_TWIN_STRIKE])/100;
 stats.regen=0; // Filled from the current maximum hull below.
 stats.construct_damage=100+2*level*m[M_ADVANCED_ENGINEERING];
 stats.assembly=(100+10*m[M_ELEGANT_CONSTRUCTION])*(200+level*m[M_ELEGANT_CONSTRUCTION])/200;
 stats.crash_damage=100+40*m[M_JUGGERNAUT]+35*m[M_SKIRMISH]+25*m[M_X_PHANTOM_STRIKE];
 int hullpercent=100+20*m[M_HULL_STRENGTH]+25*m[M_JUGGERNAUT];
 int shieldpercent=100+20*m[M_SHIELD_DURABILITY]+5*m[M_BARRIER]+10*m[M_RADIANT_SHIELDS]+5*m[M_FOCUSED_SHIELDS];
 stats.damage+=5*m[M_X_MASTERY]+7*m[M_X_HEAVY_CALIBER]+20*m[M_X_PROPULSIVE_MUNITIONS]+5*m[M_BLITZ];
 stats.velocity+=5*m[M_X_MASTERY]+10*m[M_X_ANNIHILATION];
 stats.size+=5*m[M_X_MASTERY]+7*m[M_X_HEAVY_CALIBER]+10*m[M_X_ANNIHILATION];
 stats.blast+=5*m[M_X_MASTERY]+25*m[M_X_POLAR_INVERSION];
 stats.fire_rate+=5*m[M_X_MASTERY]+7*m[M_X_DOUBLE_TAP]-2*m[M_X_HEAVY_CALIBER]-5*m[M_X_ENERGIZED_SHIELDS]-12*m[M_X_WARP_STRIKE];
 stats.projectiles+=m[M_X_TWIN_STRIKE];
 stats.plating+=2*m[M_X_DEFIANCE]+m[M_X_RANCOR];
 stats.damage+=5*m[M_X_RANCOR];
 stats.thrust=stats.thrust*(100+12*m[M_X_SLIPSTREAM]+10*m[M_X_EVOLUTIONARY_NICHE]+10*m[M_X_BRAVADO]+10*m[M_X_RANCOR]+5*m[M_X_HIDDEN_POWER])/100;
 stats.turn+=m[M_X_SLIPSTREAM];
 stats.shield_effect+=30*m[M_X_TEMPEST_BREAK]+10*m[M_X_EVOLUTIONARY_NICHE]+10*m[M_X_BRAVADO];
 stats.shield_cooldown=stats.shield_cooldown*maxv(20,100-25*m[M_X_FLASH_SHIELDING]-5*m[M_X_ENERGIZED_SHIELDS]-30*m[M_X_TEMPEST_BREAK])/100;
 stats.assembly+=20*m[M_X_SUPPORT_SPECIALIST];
 stats.assembly-=5*m[M_X_ENERGIZED_SHIELDS];
 hullpercent+=2*m[M_X_RAMPAGE]+10*m[M_X_EVOLUTIONARY_NICHE]+10*m[M_X_BRAVADO]+5*m[M_X_HIDDEN_POWER]-3*extra_ranks_;
 shieldpercent+=5*m[M_VOLATILE_SHIELDS]-25*m[M_X_TEMPEST_BREAK]+2*m[M_X_RAMPAGE]+5*m[M_X_HIDDEN_POWER]-25*m[M_X_FLASH_SHIELDING]-3*extra_ranks_;
 if(m[M_X_ATARAXIA])stats.thrust=stats.thrust*(100+minv(100,banked_upgrades))/100;
 if(m[M_X_APEX_MACHINERY]){stats.shield_effect+=20;stats.assembly=stats.assembly*4/7;}
 if(m[M_X_BURNOUT_REACTORS])stats.assembly=stats.assembly*130/100;
 hullpercent+=5*m[M_X_RANCOR]+10*m[M_X_SHIELDED_DRONES];
 hullpercent=maxv(15,hullpercent);shieldpercent=maxv(10,shieldpercent);
 if(m[M_VOLLEY]){stats.damage=stats.damage*80/100;stats.blast=stats.blast*80/100;stats.velocity=stats.velocity*90/100;stats.size=stats.size*90/100;}
 if(m[M_FUSILLADE]){stats.damage=stats.damage*85/100;stats.blast=stats.blast*85/100;stats.velocity=stats.velocity*90/100;stats.size=stats.size*90/100;}
 if(m[M_GUIDANCE])stats.velocity=stats.velocity*80/100;
 for(unsigned r=0;r<m[M_X_HEAVY_CALIBER];r++)stats.velocity=stats.velocity*93/100;
 if(player.body==B_ASSAULT){int power=15*bodyPowerPercent()/100;stats.damage+=power;stats.fire_rate+=power;stats.velocity+=power;stats.size+=power;stats.blast+=power;}
 if(player.body==B_HULLBREAKER){stats.fire_rate-=25;stats.crash_damage+=90*bodyPowerPercent()/100;}
 if(player.body==B_SENTINEL){stats.fire_rate-=10;stats.shield_effect+=10*bodyPowerPercent()/100;stats.shield_radius=stats.shield_radius*(100+25*bodyPowerPercent()/100)/100;}
 if(player.body==B_ENGINEER)stats.assembly+=level*2;
 if(player.body==B_ARCHITECT){stats.construct_damage+=50;stats.assembly=stats.assembly*2/3;}
 if(player.body==B_BATTERY){stats.projectiles+=3;stats.damage+=15;stats.size+=15;stats.fire_rate-=15;}
 if(player.body==B_COURSER){stats.projectiles+=3;stats.damage=stats.damage*90/100;stats.fire_rate+=10;}
 unsigned construct_limits=combinedConstructLimits();
 if(m[M_OVERSEER]){stats.thrust=stats.thrust*maxv(10,100-(int)construct_limits*2)/100;stats.assembly+=construct_limits*2;}
 segment_count=player.body==B_LEVIATHAN?(uint8_t)minv(MAX_SEGMENTS,10+level):0;
 player.max_hull=(b.hull+segment_count*2+50*m[M_RAPID_RECONSTRUCTION]+5*m[M_CORE_SHIELDING]+5*m[M_X_CONVERSION]+10*m[M_X_SHIELDED_DRONES])*hullpercent/100*(100+research_bonus_)/100;

 stats.regen=(int16_t)maxv(0,player.max_hull*2*(100+40*m[M_REGENERATION]+100*m[M_RAPID_RECONSTRUCTION]+60*m[M_X_HYPERMETABOLISM])/10000);
 player.max_shield=m[M_RAPID_RECONSTRUCTION]?0:(shields[player.shield_type].capacity+55*m[M_OMNI_SHIELD]+5*m[M_CORE_SHIELDING]+5*m[M_X_CONVERSION])*shieldpercent/100*b.shield_percent/100*(100+research_bonus_)/100;
 if(player.shield_type==S_BASTION)player.max_shield=(int16_t)minv(10000,(int)player.max_shield*constructHullPercent(C_BASTION)/100);
 if(m[M_X_CONVERSION]){if(player.max_hull<player.max_shield){int half=player.max_hull*30/100;player.max_hull-=half;player.max_shield+=half;}else{int half=player.max_shield*30/100;player.max_shield-=half;player.max_hull+=half;}}
 if(m[M_X_ATARAXIA]){player.max_hull=player.max_hull*(100+minv(100,banked_upgrades))/100;player.max_shield=player.max_shield*(100+minv(100,banked_upgrades))/100;if(banked_upgrades>=10)stats.shield_cooldown=stats.shield_cooldown*100/115;}
 if(m[M_X_SUPPORT_SPECIALIST]){player.max_hull=maxv(1,player.max_hull*maxv(0,100-(int)construct_limits*2)/100);player.max_shield=player.max_shield*maxv(0,100-(int)construct_limits*2)/100;}
 if(m[M_OMNI_SHIELD])player.max_hull=maxv(1,player.max_hull*15/100);
 if(m[M_ESSENCE_SAP])stats.regen=0;
 if(last_stand_used_){player.max_hull=maxv(1,player.max_hull*40/100);player.max_shield=player.max_shield*40/100;}
 player.radius=(uint8_t)clamp(b.radius+m[M_JUGGERNAUT]+m[M_X_GRANDEUR]*2-m[M_X_WINNOW]-m[M_X_REGRESSION],3,15);
 if(m[M_X_LEAF_ON_THE_WIND])player.radius=maxv(3,player.radius*90/100);
 unsigned body_bonus=dailyMod(DAILY_UNBREAKABLE)?20:dailyMod(DAILY_CHOSEN)?15:dailyMod(DAILY_CAGE_MATCH)?12:0;
 unsigned shield_bonus=dailyMod(DAILY_IMMOVABLE)?20:dailyMod(DAILY_CHOSEN)?15:dailyMod(DAILY_CAGE_MATCH)?12:0;
 unsigned weapon_bonus=dailyMod(DAILY_UNSTOPPABLE)?8:dailyMod(DAILY_CHOSEN)?6:dailyMod(DAILY_CAGE_MATCH)?5:0;
 player.max_hull=(int16_t)minv(10000,(int)player.max_hull*(100+body_bonus)/100);
 player.max_shield=(int16_t)minv(10000,(int)player.max_shield*(100+shield_bonus)/100);
 stats.thrust=stats.thrust*(100+body_bonus)/100;
 stats.shield_effect=stats.shield_effect*(100+shield_bonus)/100;
 stats.damage=stats.damage*(100+weapon_bonus)/100;stats.fire_rate=stats.fire_rate*(100+weapon_bonus)/100;
 stats.velocity=stats.velocity*(100+weapon_bonus)/100;stats.blast=stats.blast*(100+weapon_bonus)/100;stats.size=stats.size*(100+weapon_bonus)/100;
 if(dailyMod(DAILY_HYPERACTIVE))stats.shield_cooldown=stats.shield_cooldown*100/115;
 if(dailyMod(DAILY_COMPACT))player.radius=(uint8_t)maxv(2,player.radius*80/100);
 if(dailyMod(DAILY_IM_THE_BOSS)){player.max_hull=(int16_t)minv(10000,(int)player.max_hull*50);player.max_shield=(int16_t)minv(10000,(int)player.max_shield*50);player.radius=(uint8_t)minv(30,player.radius*125/100);}
 if(daily(DAILY_SERPENTINE)){player.max_hull=100+segment_count;player.max_shield=0;stats.thrust+=10;}
 if(daily(DAILY_COSMIC_EGG)){player.max_hull=(int16_t)minv(10000,player.max_hull*125/100);player.max_shield=(int16_t)minv(10000,player.max_shield*125/100);}
 if(daily(DAILY_SWORD)){stats.damage=(int16_t)minv(10000,stats.damage*7);stats.fire_rate=stats.fire_rate*125/100;stats.thrust=stats.thrust*150/100;stats.size=stats.size*120/100;}
 if(daily(DAILY_PURITY)&&m[M_X_ATARAXIA]){unsigned points=minv(100,banked_upgrades);player.max_hull=(int16_t)minv(10000,(int)player.max_hull*(100+2*points)/(100+points));player.max_shield=(int16_t)minv(10000,(int)player.max_shield*(100+2*points)/(100+points));}
 player.max_hull=(int16_t)clamp(player.max_hull,1,10000);player.max_shield=(int16_t)clamp(player.max_shield,0,10000);
 player.hull=minv(player.hull,hullRecoveryCap());player.shield=minv(player.shield,shieldRecoveryCap());
}
void Game::giveExperience(uint32_t amount) {if(last_stand_used_)amount*=2;amount=amount*(100+experienceBonus())/100;run_stats.experience_collected=addSaturated(run_stats.experience_collected,amount);xp=addSaturated(xp,amount);checkLevel();}
void Game::bonusLevels(unsigned count) {level=(uint16_t)minv(999,level+count);banked_upgrades=(uint16_t)minv(999,banked_upgrades+count);xp_next=experienceForLevel(level);}
void Game::checkLevel() {if(state==Playing&&xp>=xp_next&&level<999) {xp-=xp_next;bonusLevels(1);recalculate();beginUpgrade();}}
bool Game::bankUpgrade() {if(state!=Upgrade||!banked_upgrades)return false;state=Playing;resetInput();recalculate();return true;}
bool Game::reopenUpgrade() {if(state!=Playing||!banked_upgrades)return false;state=Upgrade;resetInput();return true;}
void Game::beginUpgrade() {
 if(daily(DAILY_SERPENTINE)||(daily(DAILY_PURITY)&&run_stats.mods_taken>=7)){state=Playing;return;}
 state=Upgrade;selected_offer=0;
 offer_kind_=gear_choices_==0?OFFER_WEAPON:gear_choices_==1?OFFER_BODY:gear_choices_==2?OFFER_SHIELD:OFFER_MOD;
 generateOffers();
}
void Game::generateOffers() {
 // Reservoir sampling visits every eligible item, avoiding arbitrary retry
 // failures, repeated cards and biased fallback equipment.
 unsigned count=0,kind=offer_kind_;
 unsigned wanted=gear_choices_<3?3:OFFER_COUNT;
 if(kind==OFFER_SHIELD&&player.mods[M_RAPID_RECONSTRUCTION])kind=OFFER_MOD;
 if(kind==OFFER_WEAPON&&player.mods[M_X_HIDDEN_POWER])kind=OFFER_MOD;
 unsigned limit=kind==OFFER_MOD?(unsigned)MOD_COUNT:kind==OFFER_WEAPON?(unsigned)WEAPON_COUNT:kind==OFFER_BODY?(unsigned)BODY_COUNT:(unsigned)SHIELD_COUNT;
 if(kind==OFFER_MOD){
  unsigned c=896+256*player.mods[M_X_CHAOTIC_AMBITION];
  unsigned wild_wanted=wild_mode?(unsigned)clamp((maxv((int)c-1792,minv(1792,range(c)))+128)/256,0,wanted):0;
  if(dailyMod(DAILY_WILDER))wild_wanted=minv(wanted,wild_wanted*3);
  if(dailyMod(DAILY_PANDEMONIUM)&&(run_stats.mods_taken&1))wild_wanted=wanted;
  if(transmog_)wild_wanted=wanted;
  unsigned wild_seen=0;
  for(unsigned id=0;id<MOD_COUNT;id++)if(mods[id].category==2&&canTakeMod(id)&&(!transmog_||mods[id].max_rank>1)){
   ++wild_seen;unsigned slot=wild_seen<=wild_wanted?wild_seen-1:range(wild_seen);
   if(slot<wild_wanted){offers[slot].kind=OFFER_MOD;offers[slot].id=(uint8_t)id;}
  }
  count=minv(wild_seen,wild_wanted);unsigned base=count,normal_seen=0,normal_wanted=transmog_?0:wanted-base;
  for(unsigned id=0;id<MOD_COUNT;id++)if(mods[id].category!=2&&canTakeMod(id)){
   ++normal_seen;unsigned slot=normal_seen<=normal_wanted?normal_seen-1:range(normal_seen);
   if(slot<normal_wanted){offers[base+slot].kind=OFFER_MOD;offers[base+slot].id=(uint8_t)id;}
  }
  count+=minv(normal_seen,normal_wanted);
 } else for(unsigned id=0;id<limit;id++){
  bool eligible=kind==OFFER_WEAPON?id!=player.weapon:kind==OFFER_BODY?id!=player.body:id!=player.shield_type;
  if(!eligible||account_rank<offerUnlockRank(kind,id))continue;
  ++count;unsigned slot=count<=wanted?count-1:range(count);
  if(slot<wanted){offers[slot].kind=(uint8_t)kind;offers[slot].id=(uint8_t)id;}
 }
 // Only a fully exhausted mod pool falls back to unlocked gear.
 if(!count){for(unsigned id=0;count<wanted&&id<BODY_COUNT;id++)if(id!=player.body&&account_rank>=offerUnlockRank(OFFER_BODY,id)){offers[count].kind=OFFER_BODY;offers[count].id=(uint8_t)id;++count;}}
 offer_count=(uint8_t)minv(wanted,count);
 for(unsigned i=offer_count;i<OFFER_COUNT;i++)offers[i]=offers[0]; // Inactive snapshot padding.
 for(unsigned i=offer_count-1;i>0;i--){unsigned j=range(i+1);Offer temp=offers[i];offers[i]=offers[j];offers[j]=temp;}
 selected_offer=0;
}
bool Game::prepareGearOffers(unsigned kind) {
 if(state!=Upgrade||kind>OFFER_SHIELD||!banked_upgrades)return false;
 if(dailyMod(DAILY_CHOSEN)||(daily(DAILY_SWORD)&&kind==OFFER_WEAPON)||(daily(DAILY_SERPENTINE)&&kind==OFFER_BODY))return false;
 if((kind==OFFER_SHIELD&&player.mods[M_RAPID_RECONSTRUCTION])||(kind==OFFER_WEAPON&&player.mods[M_X_HIDDEN_POWER]))return false;
 offer_kind_=(uint8_t)kind;generateOffers();return true;
}
bool Game::banishOffer(unsigned index) {
 if(state!=Upgrade||index>=offer_count||player.body!=B_RESEARCH||rerolls<2||offers[index].kind!=OFFER_MOD||player.mods[M_X_POWER_SPIKE])return false;
 unsigned id=offers[index].id;if(banished_[id])return false;
 banished_[id]=1;rerolls-=2;run_stats.rerolls_used=(uint16_t)minv(65535,run_stats.rerolls_used+2);
 ++research_bonus_;recalculate();generateOffers();return true;
}
bool Game::chooseOffer(unsigned i) {
 if(state!=Upgrade||i>=offer_count)return false;
 const Offer o=offers[i];bool ok=false;
 if(o.kind==OFFER_MOD)ok=takeMod(o.id);else if(o.kind==OFFER_WEAPON)ok=equipWeapon(o.id);
 else if(o.kind==OFFER_BODY)ok=equipBody(o.id);else if(o.kind==OFFER_SHIELD)ok=equipShield(o.id);
 if(!ok)return false;
 if(o.kind!=OFFER_MOD){gear_owned_=(uint8_t)(gear_owned_|(1u<<o.kind));if(gear_choices_<3)++gear_choices_;}
 if(banked_upgrades)--banked_upgrades;
 if(transmog_&&o.id!=M_X_TRANSMOGRIFICATION)--transmog_;
 state=Playing;player.invulnerable=maxv(player.invulnerable,30);resetInput();recalculate();
 if(banked_upgrades)beginUpgrade();else checkLevel();return true;
}
bool Game::rerollOffers() {
 if(state!=Upgrade||rerolls==0||transmog_||player.mods[M_X_POWER_SPIKE])return false;
 --rerolls;++run_stats.rerolls_used;generateOffers();return true;
}
void Game::advance(uint32_t ms,const GameInput &in) {
 if(ms>200)ms=200;
 accumulator_+=ms*30;
 unsigned steps=0;while(accumulator_>=1000&&steps<6){accumulator_-=1000;tick(in);++steps;}
}
void Game::tick(const GameInput &in) {
 bool confirm=in.confirm&&!previous_.confirm,back=in.back&&!previous_.back;
 bool pause=in.pause&&!previous_.pause,ability=in.ability&&!previous_.ability;
 if(state==Title) {if(confirm)start(seed?seed:0x4e4f5641u);previous_=in;return;}
 if(state==GameOver) {if(confirm)start(seed+0x9e3779b9u);previous_=in;return;}
 if(state==Pause) {if(pause||back||confirm)state=Playing;previous_=in;return;}
 if(state==Upgrade) {
  if((in.left&&!previous_.left)||(in.up&&!previous_.up))selected_offer=(selected_offer+offer_count-1)%offer_count;
  if((in.right&&!previous_.right)||(in.down&&!previous_.down))selected_offer=(selected_offer+1)%offer_count;
  if(ability)rerollOffers();
  if(confirm)chooseOffer(selected_offer);
  previous_=in;return;
 }
 if(pause||back){state=Pause;previous_=in;return;}
 ++frame;if(wave_banner)--wave_banner;if(damage_flash)--damage_flash;
 untouched_=(uint16_t)minv(2400,untouched_+1);
 if(kill_buff_)--kill_buff_;
 for(unsigned i=0;i<MAX_EFFECTS;i++)if(effects[i].active&&--effects[i].life==0)effects[i].active=0;
 updatePlayer(in);if(state==GameOver){previous_=in;return;}
 updateWaves();updateChallenger();updateEnemies();buildGrid();updateShields();updateConstructs();updatePersistentEffects();updateBullets();updatePickups();updateDaily();
 if(state==Playing)checkLevel();
 previous_=in;
}
void Game::emitEffect(int32_t x,int32_t y,unsigned kind,unsigned radius,unsigned life,unsigned color,int32_t x2,int32_t y2) {
 unsigned slot=MAX_EFFECTS;
 for(unsigned i=0;i<MAX_EFFECTS;i++)if(!effects[i].active){slot=i;break;}
 if(slot==MAX_EFFECTS)return;
 Effect &e=effects[slot];
 e.active=1;e.x=x;e.y=y;e.x2=x2;e.y2=y2;e.kind=(uint8_t)kind;
 e.radius=(uint8_t)clamp(radius,1,100);e.life=e.max_life=(uint8_t)clamp(life,1,255);e.color=(uint8_t)color;
}
int Game::emitBullet(int32_t x,int32_t y,uint8_t angle,int speed,int damage,unsigned kind,unsigned life,unsigned owner,unsigned radius) {
 // Separate budgets: a dense player build cannot suppress hostile fire.
 unsigned begin=owner?FRIENDLY_BULLETS:0,end=owner?MAX_BULLETS:FRIENDLY_BULLETS;
 unsigned slot=end;for(unsigned i=begin;i<end;i++)if(!bullets[i].active){slot=i;break;}
 if(slot==end&&!owner){
  // A sub-tick firing rate creates coincident projectiles. Fold two existing
  // identical packets into one and use the freed slot for the new shot, so
  // caller-specific velocity/empower/construct attribution is still applied.
  // No packet is moved or kept alive longer. Damage and travel time are exact
  // for these coincident rays; independent on-hit procs remain a pool mapping.
  int16_t buckets[64];for(unsigned i=0;i<64;i++)buckets[i]=-1;
  for(unsigned i=begin;i<end;i++){
   Bullet &shot=bullets[i];
   if(shot.tracking||shot.kind==SHOT_DART||shot.kind==SHOT_WAKE||shot.kind==SHOT_DISCHARGE||shot.kind==SHOT_VORTEX)continue; // Independent persistent-object state cannot be folded into damage.
   unsigned key=((unsigned)shot.angle+shot.kind*13u+(unsigned)pixel(shot.x)*7u+(unsigned)pixel(shot.y)*11u)&63u;
   int previous=buckets[key];buckets[key]=(int16_t)i;
   if(previous<0)continue;
   Bullet &old=bullets[(unsigned)previous];
   if(old.kind!=shot.kind||old.angle!=shot.angle||old.x!=shot.x||old.y!=shot.y||
      old.vx!=shot.vx||old.vy!=shot.vy||old.speed!=shot.speed||old.age!=shot.age||
      old.life!=shot.life||old.traveled!=shot.traveled||old.radius!=shot.radius||
      old.size_percent!=shot.size_percent||old.homing!=shot.homing||old.pierce!=shot.pierce||
      old.blast!=shot.blast||old.bounces!=shot.bounces||old.wraps!=shot.wraps||
      old.construct_source!=shot.construct_source||old.last_hit!=shot.last_hit||
      old.damage>2000000-shot.damage)continue;
   old.damage+=shot.damage;slot=i;
   run_stats.coalesced_shots=addSaturated(run_stats.coalesced_shots,1);break;
  }
 }
 if(slot==end){++dropped_bullets;if(!owner)return -1;
  slot=begin;for(unsigned i=begin+1;i<end;i++)if(bullets[i].age>bullets[slot].age)slot=i;
 }
 Bullet &b=bullets[slot];clearBytes(&b,sizeof(b));b.active=1;b.owner=(uint8_t)owner;
 b.x=wrap(x,worldWidth());b.y=wrap(y,worldHeight());b.angle=angle;b.speed=(uint16_t)clamp(speed*FP,1,16*FP);b.vx=dirX(angle)*b.speed/FP;b.vy=dirY(angle)*b.speed/FP;
 b.damage=clamp(damage,1,2000000);b.kind=(uint8_t)kind;b.life=(uint16_t)life;
 b.radius=(uint8_t)clamp(radius,1,32);b.last_hit=-1;b.size_percent=(uint16_t)(owner?100:maxv(1,stats.size));
 if(!owner) {b.homing=(uint8_t)stats.homing;
  b.bounces=player.mods[M_X_RICOCHET];
  b.wraps=player.mods[M_X_WARP_STRIKE];
  if(player.mods[M_BLAST_RADIUS])b.blast=8;
  run_stats.shots_fired=addSaturated(run_stats.shots_fired,1);
  if(kind==SHOT_SPLIT)b.blast=11;
  if(kind==SHOT_GRENADE)b.blast=25;
  if(kind==SHOT_MISSILE){b.blast=12;b.homing=(uint8_t)maxv(3,b.homing);}
  if(kind==SHOT_RAIL)b.pierce=5;
  if(kind==SHOT_DART){b.pierce=255;b.blast=8;}
  b.blast=(uint8_t)minv(85,b.blast*stats.blast/100);
 }
 return (int)slot;
}
void Game::emitPickup(int32_t x,int32_t y,unsigned kind,unsigned value) {
 for(unsigned i=0;i<MAX_PICKUPS;i++)if(!pickups[i].active){
  Pickup &p=pickups[i];clearBytes(&p,sizeof(p));p.active=1;p.x=x;p.y=y;p.kind=(uint8_t)kind;p.value=(uint16_t)value;
  uint8_t a=(uint8_t)range(256);p.vx=dirX(a);p.vy=dirY(a);return;
 }
 // Keep experience conserved under saturation; merging never silently loses XP.
 if(kind==PICKUP_XP){for(unsigned i=0;i<MAX_PICKUPS;i++)if(pickups[i].kind==PICKUP_XP){pickups[i].value=(uint16_t)minv(65535,pickups[i].value+value);return;}}
}
void Game::hostileBlast(int32_t x,int32_t y,unsigned radius,int damage) {
 emitEffect(x,y,FX_RING,radius,12,1);
 unsigned reach=radius+player.radius;if(distance2(x,y,player.x,player.y)<=(int)(reach*reach))damagePlayer(damage,false,direction(delta(x,player.x,worldWidth()),delta(y,player.y,worldHeight())));
 for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active){reach=radius+constructs[c].radius;if(distance2(x,y,constructs[c].x,constructs[c].y)<=(int)(reach*reach))damageConstruct(c,damage);}
}
int Game::nearestEnemy(int32_t x,int32_t y,int radius) const {
 int best=-1,dist=radius*radius;
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&!enemies[i].boss_hidden){int d=distance2(x,y,enemies[i].x,enemies[i].y);if(d<dist){dist=d;best=(int)i;}}
 return best;
}
bool Game::targetActive(int target) const {
 if(target<0||target>=MAX_ENEMIES+MAX_BOSS_PARTS)return false;
 if(target<MAX_ENEMIES){const Enemy &e=enemies[target];return e.active&&!e.boss_hidden&&physicalCore(e.type);}
 const BossPart &p=boss_parts[target-MAX_ENEMIES];
 return p.active&&p.kind<BP_CRUCIBLE_CORNER&&p.owner<MAX_ENEMIES&&enemies[p.owner].active&&!enemies[p.owner].boss_hidden&&p.x>=0&&p.x<worldWidth()*FP&&p.y>=0&&p.y<worldHeight()*FP;
}
int32_t Game::targetX(int target) const {return target<MAX_ENEMIES?enemies[target].x:boss_parts[target-MAX_ENEMIES].x;}
int32_t Game::targetY(int target) const {return target<MAX_ENEMIES?enemies[target].y:boss_parts[target-MAX_ENEMIES].y;}
unsigned Game::targetRadius(int target) const {return target<MAX_ENEMIES?enemies[target].radius:boss_parts[target-MAX_ENEMIES].radius;}
int Game::targetHull(int target) const {return target<MAX_ENEMIES?enemies[target].hull:boss_parts[target-MAX_ENEMIES].hull;}
bool Game::targetInRadius(int target,int32_t x,int32_t y,unsigned radius) const {
 if(!targetActive(target))return false;
 if(target>=MAX_ENEMIES)return sweptBossPart(boss_parts[target-MAX_ENEMIES],x,y,x,y,radius);
 unsigned reach=radius+targetRadius(target);return distance2(x,y,targetX(target),targetY(target))<=(int)(reach*reach);
}
bool Game::targetSwept(int target,int32_t ox,int32_t oy,int32_t nx,int32_t ny,unsigned padding) const {
 if(!targetActive(target))return false;
 if(target>=MAX_ENEMIES)return sweptBossPart(boss_parts[target-MAX_ENEMIES],ox,oy,nx,ny,padding);
 return sweptHit(ox,oy,nx,ny,targetX(target),targetY(target),targetRadius(target)+padding);
}
unsigned Game::targetOwner(int target) const {return target<MAX_ENEMIES?(unsigned)target:boss_parts[target-MAX_ENEMIES].owner;}
void Game::damageTarget(int target,int damage,bool burn,bool area) {
 if(!targetActive(target))return;
 if(target<MAX_ENEMIES)hitEnemy((unsigned)target,damage,burn,area);else hitBossPart((unsigned)(target-MAX_ENEMIES),damage,burn,area);
}
int Game::nearestTarget(int32_t x,int32_t y,int radius) const {
 int best=-1,dist=radius*radius;
 for(unsigned i=0;i<MAX_ENEMIES+MAX_BOSS_PARTS;i++)if(targetActive((int)i)&&!targetObstructedByEclipse((int)i)){
  int d=distance2(x,y,targetX((int)i),targetY((int)i));if(d<dist){dist=d;best=(int)i;}
 }
 return best;
}
void Game::updatePlayer(const GameInput &in) {
 if(discharge_clock_)--discharge_clock_;
 const int old_hull=player.hull,old_shield=player.shield;
 if(in.ability&&!previous_.ability)construct_deploy_requested_=true;
 Player &p=player;const uint8_t *m=p.mods;
 if(p.invulnerable)--p.invulnerable;
 if(normal_hit_grace_)--normal_hit_grace_;
 if(hyperboost_ticks_)--hyperboost_ticks_;
 if(source_armor_ticks_)--source_armor_ticks_;
 if(outrage_clock_)--outrage_clock_;
 if(viper_clock_)--viper_clock_;
 if(p.ability_delay){--p.ability_delay;if(daily(DAILY_SWORD)&&!(frame&1)&&p.ability_delay)--p.ability_delay;}
 if(m[M_X_DISPLACEMENT]&&!p.ability_delay&&void_charges_<2){++void_charges_;if(void_charges_<2)p.ability_delay=75;}
 if(emergency_clock_)--emergency_clock_;
 armor_stacks_=0;for(unsigned i=0;i<50;i++)if(armor_timers_[i]){--armor_timers_[i];if(armor_timers_[i])++armor_stacks_;}
 if(p.fire_delay)--p.fire_delay;
 int turn=stats.turn;
 // Cursor aiming follows the visible straight line; toroidal shortest-path
 // targeting is reserved for autonomous actors and collision calculations.
 if(in.pointer_valid){p.angle=toward(p.angle,direction(in.pointer_x*FP-p.x,in.pointer_y*FP-p.y),turn*2);}
 else {if(in.left)p.angle=(uint8_t)(p.angle-turn);if(in.right)p.angle=(uint8_t)(p.angle+turn);}
 p.thrusting=in.up||m[M_X_RAMPAGE];
 if(p.thrusting)aegis_clock_=0;else aegis_clock_=(uint8_t)minv(75,aegis_clock_+1);
 bool held=in.fire||in.pointer_down,previous_held=previous_.fire||previous_.pointer_down;
 if(auto_fire!=auto_fire_previous_){firing_latch_=false;auto_fire_previous_=auto_fire;}
 if(auto_fire&&held&&!previous_held)firing_latch_=!firing_latch_;
 p.firing=auto_fire?firing_latch_:held;
 if(m[M_OVERSEER]||m[M_X_HIDDEN_POWER]||daily(DAILY_SERPENTINE))p.firing=false;

 int thrust=stats.thrust;if(hasHyperboost())thrust=thrust*4/3;if(m[M_CHARGED_SHOT]&&!m[M_X_CHARGED_MINES]&&p.charge)thrust=thrust*80/100;if(m[M_X_RANCOR]&&p.charge)thrust=thrust*85/100;if(m[M_X_CHARGED_SHIELDS]&&p.charge)thrust=thrust*85/100;if(p.shield>0&&p.shield_type==S_AMP)thrust=thrust*125/100;
 if(kill_buff_&&m[M_WARPATH])thrust+=20;
 if(p.firing&&(p.weapon==W_TORRENT||p.weapon==W_THERMAL_LANCE||p.weapon==W_VORTEX))thrust=thrust*80/100;
 if(p.thrusting){p.vx+=dirX(p.angle)*thrust/1400;p.vy+=dirY(p.angle)*thrust/1400;}
 int drag=in.down?(m[M_STABILIZATION]?205:225):(m[M_STREAMLINE]||m[M_X_SLIPSTREAM]||m[M_X_REGRESSION]?255:253);
 if(dailyMod(DAILY_SLIPPERY))drag=256-(256-drag)/2;
 p.vx=p.vx*drag/256;p.vy=p.vy*drag/256;
 int speed=approx(p.vx,p.vy),maximum=(3*FP)*stats.thrust/100;
 if(dash_ticks_){--dash_ticks_;maximum+=3*FP;}
 if(speed>maximum){p.vx=p.vx*maximum/speed;p.vy=p.vy*maximum/speed;}
 run_stats.distance_q8=addSaturated(run_stats.distance_q8,isqrt32((uint32_t)(p.vx*p.vx+p.vy*p.vy)));
 int32_t oldx=p.x,oldy=p.y;
 if(!screenWrappingEnabled()){if(p.x+p.vx<player.radius*FP||p.x+p.vx>=(worldWidth()-player.radius)*FP)p.vx=-p.vx;if(p.y+p.vy<player.radius*FP||p.y+p.vy>=(worldHeight()-player.radius)*FP)p.vy=-p.vy;}
 p.x=playerDestination(p.x+p.vx,true);p.y=playerDestination(p.y+p.vy,false);
 if(m[M_X_WARP_STRIKE]&&(absn(p.x-oldx)>worldWidth()*FP/2||absn(p.y-oldy)>worldHeight()*FP/2)){if(p.shield)p.shield=maxv(1,p.shield-p.shield/10-5);p.hull=maxv(1,p.hull-p.hull/10-5);}
 sanctuary_x=wrap(sanctuary_x+delta(p.x,sanctuary_x,worldWidth())*FP/40,worldWidth());
 sanctuary_y=wrap(sanctuary_y+delta(p.y,sanctuary_y,worldHeight())*FP/40,worldHeight());
 if(in.ability&&!previous_.ability&&(!p.ability_delay||(m[M_X_DISPLACEMENT]&&void_charges_))&&(m[M_BLINK]||m[M_X_STRAFE]||p.body==B_ARCHITECT)) {
  if(p.body==B_ARCHITECT){for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active){blast(constructs[i].x,constructs[i].y,32,80*stats.construct_damage/100);constructs[i].active=0;}}
  uint8_t dash_angle=p.angle;
  if(m[M_X_STRAFE])dash_angle=(uint8_t)(p.angle+(in.down&&m[M_X_DISPLACEMENT]?128:in.left?-64:in.right?64:0));
  if(m[M_BLINK]) {int travel=m[M_X_DISPLACEMENT]?84:65;int32_t ox=p.x,oy=p.y;p.x=playerDestination(p.x+dirX(dash_angle)*travel,true);p.y=playerDestination(p.y+dirY(dash_angle)*travel,false);if(!m[M_X_DISPLACEMENT]){blast(ox,oy,22,100);blast(p.x,p.y,22,100);}emitEffect(ox,oy,FX_WARP,25,12,3);
   if(m[M_X_DISPLACEMENT]){emitEffect(ox,oy,FX_BEAM,5,12,3,ox+dirX(dash_angle)*travel,oy+dirY(dash_angle)*travel);for(unsigned e=0;e<MAX_ENEMIES+MAX_BOSS_PARTS;e++)if(targetSwept(e,ox,oy,p.x,p.y,5))damageTarget(e,1200+targetHull(e)*7/100);}}
  else {p.vx+=dirX(dash_angle)*3;p.vy+=dirY(dash_angle)*3;}
  dash_ticks_=12;
  p.invulnerable=maxv(p.invulnerable,12);if(m[M_X_DISPLACEMENT]){if(void_charges_)--void_charges_;if(!p.ability_delay)p.ability_delay=75;}else p.ability_delay=45;
 }
 if(m[M_X_CHARGED_SHIELDS]){unsigned value=storedChargeMicro()/500; // .2 of charge, in basis points.
  if(p.weapon==W_VORTEX)value=(vortex_scale_+rancor_scale_)/500;
  else if(p.weapon==W_THERMAL_LANCE)value=(thermal_x_scale_+rancor_scale_)/500;
  shield_charge_=(uint16_t)maxv(shield_charge_,minv(8000,value));
  if(p.firing&&p.charge)shield_linger_=45; // original source frames
  else if(shield_linger_)shield_linger_=shield_linger_>2?shield_linger_-2:0;
  else {shield_charge_=(uint16_t)(shield_charge_*9025/10000);if(shield_charge_<3)shield_charge_=0;}
 }
 if(!p.firing){calibration_=minv(180,calibration_+1);if(p.body==B_SPECTRE&&speed<2*FP)p.cloak=minv(180,p.cloak+2);}
 else if(p.weapon!=W_VORTEX&&p.weapon!=W_THERMAL_LANCE)p.cloak=p.cloak>4?p.cloak-4:0;
 if(speed>=2*FP)p.cloak=p.cloak>3?p.cloak-3:0;
 if(p.weapon==W_SALVO&&!p.firing){
  unsigned stock_period=384000u/maxv(20,stats.fire_rate);if(m[M_X_SINGULAR_STRIKE_NEW])stock_period+=960;if(hasHyperboost())stock_period=stock_period*3/4;
  unsigned stock=4*(weapons[p.weapon].projectiles+stats.projectiles+(m[M_FUSILLADE]?weapons[p.weapon].projectiles+1:0));if(m[M_X_SINGULAR_STRIKE_NEW])stock=1;
  unsigned clock=p.missile_clock+256;while(clock>=stock_period){clock-=stock_period;p.missiles=(uint16_t)minv(4095,p.missiles+stock);}p.missile_clock=(uint16_t)clock;
 }
 updateRancor();
 if(p.weapon==W_VORTEX) {
  updateVortexCharge(); } else if(p.weapon==W_THERMAL_LANCE) {
  if(p.firing){
   if(!p.charge)thermal_x_scale_=thermal_y_scale_=100000;
   unsigned modifier=(1000+vortexChargeModifier())/2;
   for(unsigned sub=0;sub<2;sub++){
    unsigned tx=(unsigned)minv(64000000,100000+(uint64_t)825*modifier),ty=(unsigned)minv(64000000,250000+(uint64_t)330*modifier);
    thermal_x_scale_=(uint32_t)((int64_t)thermal_x_scale_+((int64_t)tx-thermal_x_scale_)/50);
    thermal_y_scale_=(uint32_t)((int64_t)thermal_y_scale_+((int64_t)ty-thermal_y_scale_)/50);
   }
   innate_charge_fraction_=(uint16_t)(innate_charge_fraction_+(m[M_CHARGED_SHOT]&&!m[M_X_CHARGED_MINES]?240:200));p.charge=minv(90,p.charge+innate_charge_fraction_/100);innate_charge_fraction_%=100;
  }else {p.charge=0;innate_charge_fraction_=0;rancor_scale_=0;thermal_x_scale_=thermal_y_scale_=100000;}
  for(unsigned sub=0;p.firing&&!p.fire_delay&&sub<64;sub++)fireWeapon();
 } else if(m[M_CHARGED_SHOT]&&!m[M_X_CHARGED_MINES]) {
  if(p.firing){p.charge=(uint16_t)minv(65535,p.charge+1);int rate=maxv(20,stats.fire_rate);if(hasHyperboost())rate=rate*4/3;
   for(unsigned sub=0;sub<2;sub++){int64_t varying=(int64_t)14000*rate*charged_scale_/100000000;int increment=(int)(6000+varying*(2500000-(int)charged_scale_)/2500000);charged_scale_+=(unsigned)maxv(increment,1250*rate/100);charged_scale_=minv(charged_scale_,20000000);}}

  if(p.charge&&!p.fire_delay&&!p.firing)fireWeapon();
 } else {
  for(unsigned sub=0;p.firing&&!p.fire_delay&&sub<64;sub++)fireWeapon();
 }
 if(!m[M_X_HIDDEN_POWER]&&!m[M_OVERSEER]&&burst_left_&&burst_delay_&&!--burst_delay_){uint16_t save=p.fire_delay;fireWeapon();p.fire_delay=save;if(burst_left_)--burst_left_;if(burst_left_)burst_delay_=1;}
 if(m[M_SIEGE]&&p.firing&&frame%20==0&&p.charge>90)damagePlayer(2,true);
 if(m[M_SIEGE]&&p.firing)p.charge=minv(180,p.charge+1);
 if(m[M_X_SATURATION_FIRE]&&p.firing)calibration_=(uint16_t)minv(180,calibration_+3);
 if(m[M_X_STRAFING_STRIKES]&&dash_ticks_&&frame%3==0){for(unsigned side=0;side<2;side++)emitBullet(p.x,p.y,(uint8_t)(p.angle+(side?64:-64)),6,18*stats.damage/100,SHOT_BLASTER,40);}
 if(p.shield_delay) {unsigned decrement=(p.firing&&frame%5==0?0:1)+(m[M_EFFICIENCY]&&!p.firing&&frame%5==0?1:0);p.shield_delay=p.shield_delay>decrement?p.shield_delay-decrement:0;if(hasHyperboost()&&frame%3==0&&p.shield_delay)--p.shield_delay;}
 if(!dailyMod(DAILY_IM_THE_BOSS)&&p.shield_type!=S_BASTION&&!p.shield_delay&&p.shield<(int)shieldRecoveryCap()&&(!m[M_X_TEMPEST_BREAK]||p.shield==0)){bool was_down=p.shield==0;p.shield=(int16_t)shieldRecoveryCap();
  if(was_down&&p.shield_type==S_SHOCKWAVE){blast(p.x,p.y,stats.shield_radius*2,120*stats.shield_effect/100);for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&distance2(p.x,p.y,enemies[i].x,enemies[i].y)<stats.shield_radius*stats.shield_radius*4)enemies[i].vulnerability=25;}
  emitEffect(p.x,p.y,FX_RING,stats.shield_radius,14,2);}
 unsigned regen_percent=regenerationPercent();
 if(m[M_REGENERATIVE_SHIELDS]&&p.shield>0){shield_regen_fraction_+=(unsigned)p.max_shield*2*regen_percent*3/4;regen_percent/=2;}
 if(shield_regen_fraction_>=300000){unsigned heal=shield_regen_fraction_/300000;shield_regen_fraction_%=300000;p.shield=(int16_t)minv(shieldRecoveryCap(),p.shield+heal);}
 if(m[M_ESSENCE_SAP]&&essence_){unsigned consumed=minv(essence_,(unsigned)p.max_hull*100/30);essence_-=consumed;essence_fraction_+=consumed;if(essence_fraction_>=1000){p.hull=minv(hullRecoveryCap(),p.hull+essence_fraction_/1000);essence_fraction_%=1000;}}
 if(m[M_PURIFICATION]){unsigned burning=0;for(unsigned i=0;i<MAX_ENEMIES;i++)burning+=enemies[i].active&&(enemies[i].burn_time||enemies[i].wake_time);purification_fraction_+=minv(5,burning)*400;if(purification_fraction_>=30000){unsigned heal=purification_fraction_/30000;purification_fraction_%=30000;p.hull=minv(hullRecoveryCap(),p.hull+heal*(p.max_shield?1:2));p.shield=minv(shieldRecoveryCap(),p.shield+heal);}}
 regen_fraction_+=(unsigned)p.max_hull*2*regen_percent;
 if(regen_fraction_>=300000){unsigned heal=regen_fraction_/300000;regen_fraction_%=300000;p.hull=(int16_t)minv(hullRecoveryCap(),p.hull+heal);}
 // oMediCharge adds one source second of 20 HP/s to hull and a live shield.
 if(source_medi_ticks_){
  --source_medi_ticks_;unsigned rate=m[M_EFFICIENCY]&&!p.firing?24:20;
  source_medi_fraction_=(uint16_t)(source_medi_fraction_+rate);unsigned heal=source_medi_fraction_/30;source_medi_fraction_%=30;
  p.hull=(int16_t)minv(hullRecoveryCap(),p.hull+heal);if(p.shield>0)p.shield=(int16_t)minv(shieldRecoveryCap(),p.shield+heal);
 }


 if(segment_count){
  int32_t prevx=p.x,prevy=p.y;
  for(unsigned i=0;i<segment_count;i++){int dx=delta(prevx,segment_x[i],worldWidth()),dy=delta(prevy,segment_y[i],worldHeight());int d=approx(dx,dy);
   if(d>7){segment_x[i]=wrap(segment_x[i]+dx*FP*(d-7)/maxv(1,d),worldWidth());segment_y[i]=wrap(segment_y[i]+dy*FP*(d-7)/maxv(1,d),worldHeight());}
   prevx=segment_x[i];prevy=segment_y[i];}
 }
 if(p.thrusting&&(p.body==B_FIREFLY||m[M_DEADLY_WAKE])&&frame%5==0){int32_t tx=wrap(p.x-dirX(p.angle)*15,worldWidth()),ty=wrap(p.y-dirY(p.angle)*15,worldHeight());
  emitEffect(tx,ty,FX_BURST,10,8,1);
  for(unsigned i=0;i<MAX_ENEMIES+MAX_BOSS_PARTS;i++)if(targetInRadius(i,tx,ty,22))damageTarget(i,p.body==B_FIREFLY?(166+(frame%15==0))*bodyPowerPercent()/100:3,true);}
 triggerScorchingWake();
 if(m[M_X_CELESTIAL_SURGE]&&in.down&&!previous_.down&&speed>FP){int n=emitBullet(p.x,p.y,p.angle,4,25*stats.damage/100,SHOT_VORTEX,50,0,12);if(n>=0)bullets[n].homing=0;}
 if(frame%15==0&&m[M_X_DYING_STAR]){
  int life=p.hull*1000/maxv(1,p.max_hull);if(p.max_shield)life=(life+p.shield*1000/p.max_shield)/2;
  emitEffect(p.x,p.y,FX_RING,100,15,1);
  for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active){int d=approx(delta(enemies[e].x,p.x,worldWidth()),delta(enemies[e].y,p.y,worldHeight()))*300/65;
   if(d<1500){int falloff=1250*375/(375+d)-250;int damage=270*falloff/1000*(2000-life)/1000;if(damage>0)hitEnemy(e,damage,true);}}
  for(unsigned part=0;part<MAX_BOSS_PARTS;part++)if(boss_parts[part].active){int d=approx((boss_parts[part].x-p.x)/FP,(boss_parts[part].y-p.y)/FP)*300/65;if(d<1500){int damage=270*(1250*375/(375+d)-250)/1000*(2000-life)/1000;if(damage>0)hitBossPart(part,damage,true);}}
 }
 if(frame%15==0&&m[M_X_LANCE]&&speed>2*FP){emitEffect(p.x,p.y,FX_RING,25,15,1);for(unsigned e=0;e<MAX_ENEMIES+MAX_BOSS_PARTS;e++)if(targetInRadius(e,p.x,p.y,25))damageTarget(e,6,true);}
 if(frame%30==0){int self=12*m[M_X_DYING_STAR]+m[M_X_SOLAR_HEART]+m[M_X_APOTHEOSIS]+(m[M_X_LANCE]&&speed>2*FP?2:0);if(self)damagePlayer(self,true);}
 if(m[M_X_DISCORD]&&frame%8==0){uint8_t a=(uint8_t)(frame*5);int32_t ox=wrap(p.x+dirX(a)*34,worldWidth()),oy=wrap(p.y+dirY(a)*22,worldHeight());emitEffect(ox,oy,FX_BURST,4,10,3);int e=nearestTarget(ox,oy,55);if(e>=0){emitEffect(ox,oy,FX_BEAM,1,5,3,ox+delta(targetX(e),ox,worldWidth())*FP,oy+delta(targetY(e),oy,worldHeight())*FP);damageTarget(e,9);}}
 emitFeedback(FeedbackHealing,frame,p.x,p.y,(unsigned)(maxv(0,p.hull-old_hull)+maxv(0,p.shield-old_shield)));
}
unsigned Game::bodyPowerPercent() const {return (dailyMod(DAILY_UNBREAKABLE)?120:dailyMod(DAILY_CHOSEN)?115:dailyMod(DAILY_CAGE_MATCH)?112:100)+10*player.mods[M_X_EVOLUTIONARY_NICHE]+10*player.mods[M_X_BRAVADO]+(player.mods[M_X_ATARAXIA]&&banked_upgrades>=5?30:0);}
unsigned Game::globalDamagePermille() const {
 const uint8_t *m=player.mods;unsigned v=1000+70*m[M_X_RAMPAGE]+(m[M_X_ATARAXIA]?50*minv(100,banked_upgrades):0)+5*segment_count;
 // playerGlobalDamage adds .01 per ORIGINAL speed unit. Port velocity
 // maps original 60 Hz / 1920-wide coordinates to 30 Hz / 320 (factor 3).
 if(m[M_KINETIC_BOOST])v+=isqrt32((uint32_t)((int64_t)player.vx*player.vx+(int64_t)player.vy*player.vy))*30/FP;
 if(dailyMod(DAILY_HURT))v=v*115/100;
 if(dailyMod(DAILY_VIOLENCE))v=v*130/100;
 if(daily(DAILY_PURITY)&&m[M_X_ATARAXIA])v+=50*minv(100,banked_upgrades);
 if(m[M_X_HIDDEN_POWER])v=v*140/100;
 if(m[M_X_LEAF_ON_THE_WIND])v=v*(100+minv(50,untouched_*50/1800))/100;
 return v;
}
unsigned Game::burnDamagePercent() const {const uint8_t *m=player.mods;return (100+20*m[M_CANDESCENCE]+20*m[M_PURIFICATION]+20*m[M_X_DYING_STAR])*(100+2*m[M_X_SOLAR_HEART])/100;}
unsigned Game::igniteDamagePercent() const {return 100+20*player.mods[M_PURIFICATION]+30*player.mods[M_X_DYING_STAR];}
unsigned Game::regenerationPercent() const {
 const uint8_t *m=player.mods;if(m[M_ESSENCE_SAP]||dailyMod(DAILY_IM_THE_BOSS))return 0;
 unsigned v=100+40*m[M_REGENERATION]+100*m[M_RAPID_RECONSTRUCTION]+60*m[M_X_HYPERMETABOLISM];
 if(m[M_EFFICIENCY]&&!player.firing)v=v*120/100;
 if(m[M_CHANNELING])v=v*(100+armor_stacks_)/100;
 if(emergency_clock_)v=v*125/100;
 if(m[M_X_HIDDEN_POWER])v+=10;
 if(m[M_X_ATARAXIA]&&banked_upgrades>=10)v=v*115/100;
 if(player.firing)v=v*80/100;
 if(m[M_REGENERATIVE_SHIELDS])v=v*110/100;
 if(hasHyperboost())v=v*4/3;
 if(dailyMod(DAILY_HYPERACTIVE))v=v*115/100;
 if(daily(DAILY_SERPENTINE))v=v*6/5;
 return v;
}
unsigned Game::hullRecoveryCap() const {if(dailyMod(DAILY_IM_THE_BOSS)&&frame)return (unsigned)maxv(1,player.hull);return (unsigned)maxv(1,player.max_hull*maxv(0,100-15*player.mods[M_X_HYPERMETABOLISM])/100);}
unsigned Game::shieldRecoveryCap() const {if(dailyMod(DAILY_IM_THE_BOSS)&&frame)return (unsigned)maxv(0,player.shield);return (unsigned)maxv(0,player.max_shield*maxv(0,100-15*player.mods[M_X_HYPERMETABOLISM])/100);}
unsigned Game::hullRegenerationMilli() const {
 unsigned v=player.max_hull*regenerationPercent()/5;
 if(player.mods[M_REGENERATIVE_SHIELDS]&&player.shield>0)v/=2;
 return v;
}
unsigned Game::shieldRegenerationMilli() const {
 unsigned value=player.mods[M_REGENERATIVE_SHIELDS]&&player.shield>0?player.max_shield*regenerationPercent()*3/20:0;
 if(player.shield_type==S_BASTION&&player.shield>0)value+=(unsigned)((uint64_t)player.max_shield*162*activeShieldEffect()/1000);
 return value;
}
unsigned Game::activeShieldCooldownTicks() const {
 if(player.shield_type==S_BASTION){unsigned v=45000/maxv(10,stats.assembly);const uint8_t *m=player.mods;
  if(m[M_X_SPECIALIST_MINE]||m[M_X_SPECIALIST_DRONE]||m[M_X_SPECIALIST_TURRET]||m[M_X_SPECIALIST_ALLY])v=v*5/3;
  return (unsigned)clamp(v,1,60000);
 }
 unsigned v=stats.shield_cooldown;
 if(player.firing)v=v*5/4;else if(player.mods[M_EFFICIENCY])v=v*5/6;
 if(hasHyperboost())v=v*3/4;
 return v;
}
unsigned Game::incomingDamagePercent(bool shield,bool self) const {
 const uint8_t *m=player.mods;unsigned v=100;
 if(self){v=maxv(10,100-10*m[M_X_DEFIANCE]-15*m[M_X_MASOCHISM]);if(player.body==B_FIREFLY)v=v*85/100;if(m[M_CORE_SHIELDING])v=v*80/100;return v*incomingDamagePercent(shield,false)/100;}
 v=100+10*m[M_X_MASOCHISM]+2*m[M_X_FARSIGHT];if(m[M_X_SHIELDED_DRONES])v=v*90/100;
 if(m[M_AEGIS])v=v*(100-15*aegis_clock_/75)/100;
 if(m[M_ADAPTIVE_ARMOR])v=v*100/(100+armor_stacks_);
 if(m[M_X_TACTICAL_LINK]||m[M_X_FORMATION_RAMPART])for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&distance2(player.x,player.y,constructs[i].x,constructs[i].y)<55*55){v=v*80/100;break;}
 if(shield){if(player.body==B_SENTINEL)v=v*90/100;if(m[M_OMNI_SHIELD])v=v*85/100;}else if(m[M_FORCE_ARMOR])v=v*90/100;
 if(!shield&&player.shield_type==S_BASTION)v=(unsigned)maxv(0,(int)v-(int)bastionResistancePercent());
 // Source ArmorFragment base DR=.137755102; percentage damage API rounds it.
 if(source_armor_ticks_)v=(unsigned)maxv(0,(int)v-14);
 if(options.assist_mode)v=(unsigned)maxv(0,(int)v-15);
 if(dailyMod(DAILY_HURT))v=v*115/100;
 if(dailyMod(DAILY_VIOLENCE))v=v*130/100;
 if(daily(DAILY_SWORD))v=v*125/100;
 if(daily(DAILY_SERPENTINE))v=v*20/100;
 return v;
}
int Game::weaponSpreadStep() const {
 const Player &p=player;const uint8_t *m=p.mods;int spread=weapons[p.weapon].spread;
 if(m[M_FOCUS_FIRE])spread=spread*60/100;
 if(m[M_CONVERGENCE])spread=spread*175/100;
 spread=spread*(100+20*m[M_VOLLEY]+15*m[M_FUSILLADE]+(p.body==B_BATTERY?50:0)+(p.body==B_COURSER?60:0))/100;
 for(unsigned r=0;r<m[M_X_TWIN_STRIKE];r++)spread=spread*90/100;
 if(m[M_FIRING_ARRAY])spread=spread*110/100;
 if(m[M_HOMING_STRIKE])spread=spread*110/100;
 if(p.body==B_ASSAULT)spread=spread*80/100;
 if(m[M_CALIBRATE])spread=spread*(100-calibration_/3)/100;
 return spread;
}
unsigned Game::weaponSpreadArcDegrees() const {return (weaponProjectiles()-1)*weaponSpreadStep()*360/256;}
unsigned Game::weaponCannons() const {return weaponProjectiles()>1&&(player.weapon==W_BLASTER||player.body==B_BATTERY||player.body==B_COURSER)?2u:1u;}
unsigned Game::crashContactPercent() const {unsigned n=player.body==B_HULLBREAKER?35u:player.body==B_VIPER?50u:player.body==B_FIREFLY?85u:100u;if(player.mods[M_X_PHANTOM_STRIKE]&&(!player.shield||player.mods[M_X_SHIELDED_DRONES]))n=n*75/100;return n;}
unsigned Game::crashDamageTakenPercent() const {return crashContactPercent()*incomingDamagePercent(player.shield>0&&!player.mods[M_X_SHIELDED_DRONES])/100;}
unsigned Game::weaponProjectiles() const {
 const uint8_t *m=player.mods;unsigned n=weapons[player.weapon].projectiles+stats.projectiles;
 if(player.weapon==W_FLAK)n+=stats.projectiles;
 if(m[M_FUSILLADE])n+=weapons[player.weapon].projectiles+1;
 if(m[M_X_BARRAGE]||m[M_X_SINGULAR_STRIKE_NEW])n=1;
 if(player.weapon==W_BLASTER||player.weapon==W_TORRENT)n*=2;
 if(player.weapon==W_SALVO)n=1;
 return (unsigned)clamp(n,1,player.weapon==W_BLASTER||player.weapon==W_TORRENT?48:24);
}
unsigned Game::weaponDamage() const {
 int base=weapons[player.weapon].damage+2*player.mods[M_X_SOLAR_HEART]+20*player.mods[M_X_ANNIHILATION];
 if(player.weapon==W_SWORDS)base+=5*level;
 return (unsigned)maxv(1,base*stats.damage/100);
}
unsigned Game::weaponPeriodQ16(int rate) const {
 static const uint16_t rof[]={600,750,150,133,90,1600,200,100,1200,80,250,750,75};
 unsigned n=weapons[player.weapon].projectiles+stats.projectiles+(player.mods[M_FUSILLADE]?weapons[player.weapon].projectiles+1:0);
 unsigned period=(unsigned)(19660800000ull/(rof[player.weapon]*maxv(20,rate)));
 if(player.weapon==W_SALVO){
  // refreshWeapon 0x1404d4a31: 1.5*(3/5+38/(25*N-5)) source frames.
  if(player.mods[M_X_SINGULAR_STRIKE_NEW])n=1;
  unsigned denominator=25*maxv(1,n)-5;period=49152*(3*denominator+190)/(5*denominator);
 } else {
  if(player.mods[M_X_BARRAGE])period/=maxv(2,n);
  if(player.mods[M_X_SINGULAR_STRIKE_NEW])period+=245760; // .125s after ROF scaling.
 }
 if(hasHyperboost())period=period*3/4;
 return maxv(1,period);
}
unsigned Game::weaponPeriodQ8(int rate) const {return maxv(1,(weaponPeriodQ16(rate)+128)/256);}
unsigned Game::weaponRateMilli() const {return 1966080000u/weaponPeriodQ16(stats.fire_rate);}
unsigned Game::weaponCooldownTicks() const {return (weaponPeriodQ16(stats.fire_rate)+65535)/65536;}
void Game::fireWeapon(unsigned projectile_override,bool construct_origin) {
 Player &p=player;const uint8_t *m=p.mods;const WeaponDef &w=weapons[p.weapon];
 uint8_t fire_angle=(uint8_t)(p.angle+(m[M_X_PROPULSIVE_MUNITIONS]?128:0));
 int rate=stats.fire_rate;
 if(kill_buff_&&m[M_WARPATH])rate+=25;
 if(m[M_ADRENAL])rate+=60*(p.max_hull-p.hull)/maxv(1,p.max_hull);
 if(m[M_SIEGE])rate+=minv(90,p.charge);
 unsigned period=weaponPeriodQ16(rate);
 unsigned fractional=maxv(1,period)+fire_fraction_;p.fire_delay=(uint16_t)clamp(fractional/65536,0,100);fire_fraction_=(uint16_t)(fractional%65536);
 int damage=(int)weaponDamage();
 if(m[M_X_REBUKE]&&stored_damage_){damage+=stored_damage_;stored_damage_=0;}
 if(m[M_X_INFUSE]&&p.shield>0)damage+=p.shield*stats.shield_effect/400;
 if(m[M_X_SATURATION_FIRE])damage=damage*(100+calibration_/4)/100;
 bool empowered=forced_empowered_||(p.body==B_ASSAULT&&range(10000)<8*bodyPowerPercent());if(empowered)damage=damage*3;
 if(p.body==B_SPECTRE)damage=damage*(100+p.cloak*3*(int)bodyPowerPercent()/180)/100;
 if(p.shield>0&&p.shield_type==S_AMP)damage=damage*120/100;

 const unsigned charge=m[M_CHARGED_SHOT]&&(!m[M_X_CHARGED_MINES]||construct_origin)?(construct_origin?charged_scale_:storedChargeMicro()):0;
 // shootSpecialCases3: damage*(1+1.15C), velocity/blast*(1+.25C);
 // physical axes use sqrt(C), while the separate sizeMod is linear in C.
 if(charge){uint64_t boosted=(uint64_t)damage*(100000000ull+(construct_origin?115ull:100ull)*charge)/100000000ull;damage=(int)(boosted>2000000?2000000:boosted);}
 const unsigned charge_blast=1000000+charge/(construct_origin?4:5);
 const unsigned charge_size=1000000+(uint64_t)(construct_origin?325:275)*isqrt32(charge);
 const unsigned charge_size_mod=1000000+(uint64_t)(construct_origin?325:275)*charge/1000;
 int projectiles=w.projectiles+stats.projectiles;
 if(p.weapon==W_FLAK)projectiles=w.projectiles+stats.projectiles*2;
 if(m[M_FUSILLADE])projectiles+=w.projectiles+1;
 projectiles=clamp(projectiles,1,24);
 if(charge&&!m[M_X_SINGULAR_STRIKE_NEW]&&(p.weapon==W_SPLIT_SHOT||p.weapon==W_FLAK||p.weapon==W_TORRENT)){
  // shoot 0x1405afc26..5aff33: ceil(C*weaponSetProjectiles*6/fireTimeApproximated).
  projectiles+=(unsigned)(((uint64_t)charge*w.projectiles*weaponRateMilli()+9999999999ull)/10000000000ull);
 }
 projectiles=minv(FRIENDLY_BULLETS,projectiles);
 if(m[M_X_SINGULAR_STRIKE_NEW]){damage=damage*115/100*(100+50*(projectiles-1))/100;projectiles=1;}
 if(m[M_X_BARRAGE]){projectiles=1;}
 if(projectile_override)projectiles=clamp(projectile_override,1,24);
 int spread=weaponSpreadStep();
 int speed=maxv(1,w.speed*stats.velocity/100);
 if(m[M_CALIBRATE])speed+=calibration_/60;
 int radius=clamp((int)((int64_t)w.radius*stats.size*charge_size/100000000),1,32);if(empowered){speed=speed*130/100;radius=maxv(1,radius*130/100);}
 if(m[M_X_SINGULAR_STRIKE_NEW])radius=minv(16,radius*2);
 bool ordinary=true;
 if(p.weapon==W_PULSE){
  int outer=(int)minv(100,(int64_t)40*stats.blast*charge_blast/100000000),inner=(int)minv(100,(int64_t)20*stats.blast*charge_blast/100000000);
  int pulse=damage*(100+(stats.blast-100)/2)/100;blast(p.x,p.y,outer,pulse/2);blast(p.x,p.y,inner,pulse);
  emitEffect(p.x,p.y,FX_RING,outer,18,3);
 } else if(p.weapon==W_BLADE){
  unsigned count=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&constructs[i].kind==C_BLADE)++count;
  if(construct_origin||count<(unsigned)minv(6,projectiles))spawnConstruct(C_BLADE);
  ordinary=false;
 } else if(p.weapon==W_THERMAL_LANCE){
  ordinary=false;unsigned thermal_y=maxv(100000,thermal_y_scale_);damage=(int)minv(2000000,(int64_t)damage*(thermal_y+rancor_scale_*2/5)/thermal_y);
  if(p.charge<24){
   if(construct_origin||frame%6==0)emitBullet(p.x,p.y,(uint8_t)(fire_angle+(int)range(17)-8),4,maxv(1,damage/2),SHOT_SPARK,24,0,1);
  } else {
   int length=(int)minv(400,(int64_t)(55+p.charge/2)*(maxv(100000,thermal_x_scale_)+rancor_scale_)/maxv(100000,thermal_x_scale_));
   int32_t ex=wrap(p.x+dirX(fire_angle)*length,worldWidth()),ey=wrap(p.y+dirY(fire_angle)*length,worldHeight());
   emitEffect(p.x,p.y,FX_BEAM,2+radius/2,4,1,p.x+dirX(fire_angle)*length,p.y+dirY(fire_angle)*length);
   for(unsigned i=0;i<MAX_ENEMIES+MAX_BOSS_PARTS;i++)if(targetSwept(i,p.x,p.y,ex,ey,3)){
    Enemy &owner=enemies[targetOwner(i)];owner.vulnerability=(uint8_t)minv(60,owner.vulnerability+2);damageTarget(i,damage,true);
   }
  }
 } else if(p.weapon==W_SWORDS){
  ordinary=false;int reach=(int)minv(100,(int64_t)25*stats.size*charge_size/100000000);
  damage+=approx(p.vx,p.vy)/20;
  attackSwords(damage,(unsigned)reach,fire_angle,construct_origin);
 }
 if(ordinary){
  unsigned kind=SHOT_BLASTER;
  switch(p.weapon){case W_SPLIT_SHOT:kind=SHOT_SPLIT;break;case W_FLAK:kind=SHOT_FLAK;break;case W_RAILGUN:kind=SHOT_RAIL;break;
   case W_GRENADE:kind=SHOT_GRENADE;break;case W_SALVO:kind=SHOT_MISSILE;break;case W_PULSE:kind=SHOT_SPARK;damage=maxv(1,damage*45/100);break;
   case W_TORRENT:kind=SHOT_TORRENT;break;case W_DART:kind=SHOT_DART;break;default:break;}
  if(p.weapon==W_SALVO){if(!p.missiles)return;projectiles=1;--p.missiles;}
  if(p.weapon==W_RAILGUN)damage=damage*stats.velocity/100;
  if(p.weapon==W_BLASTER||p.weapon==W_TORRENT)projectiles=minv(FRIENDLY_BULLETS,projectiles*2);
  for(int i=0;i<projectiles;i++){
   int offset=(i*2-(projectiles-1))*spread/2;
   if(p.weapon==W_FLAK||p.weapon==W_SALVO)offset+=(int)range(9)-4;
   if(p.weapon==W_TORRENT)offset+=dirY((uint8_t)(frame*8))/30;
   uint8_t a=(uint8_t)(fire_angle+offset);
   int shotdamage=damage;
   int center=100-absn(i*2-(projectiles-1))*100/maxv(1,projectiles-1);
   if(m[M_FIRING_ARRAY])shotdamage=damage*(100+20*center/100)/100;
   int32_t x=p.x+dirX(fire_angle)*(p.radius+3),y=p.y+dirY(fire_angle)*(p.radius+3);
   if(p.body==B_BATTERY||p.body==B_COURSER||p.weapon==W_BLASTER){int side=(i&1)?4:-4;x+=dirX((uint8_t)(fire_angle+64))*side;y+=dirY((uint8_t)(fire_angle+64))*side;}
   int slot=emitBullet(x,y,a,speed,shotdamage,kind,w.life,0,radius);
   if(slot>=0){Bullet &shot=bullets[slot];if(charge){shot.speed=(uint16_t)clamp((int)((uint64_t)speed*FP*charge_blast/1000000),1,64*FP);shot.vx=dirX(a)*shot.speed/FP;shot.vy=dirY(a)*shot.speed/FP;shot.blast=(uint8_t)minv(100,(uint64_t)shot.blast*charge_blast/1000000);shot.size_percent=(uint16_t)minv(65535,(uint64_t)shot.size_percent*charge_size_mod/1000000);}shot.vx=clamp(shot.vx+p.vx/3,-64*FP,64*FP);shot.vy=clamp(shot.vy+p.vy/3,-64*FP,64*FP);if(empowered)shot.blast=(uint8_t)minv(100,shot.blast*130/100);if(m[M_FIRING_ARRAY]){shot.blast=(uint8_t)(shot.blast*(100+20*center/100)/100);shot.radius=(uint8_t)maxv(1,radius*(70+60*center/100)/100);}}
  }
 }
 if(m[M_HOMING_STRIKE]&&p.weapon!=W_SWORDS&&p.weapon!=W_THERMAL_LANCE)fireHomingStrike(fire_angle,projectiles,speed,damage,w.life,radius);
 if((m[M_BURST_FIRE]||m[M_X_DOUBLE_TAP])&&!m[M_X_BARRAGE]&&!burst_left_&&p.weapon!=W_THERMAL_LANCE&&p.weapon!=W_SWORDS&&p.weapon!=W_BLADE){burst_left_=(uint8_t)(2*m[M_BURST_FIRE]+m[M_X_DOUBLE_TAP]);burst_delay_=1;p.fire_delay=maxv(1,p.fire_delay)*(burst_left_+1)+1;}
 if(!construct_origin&&m[M_DISCHARGE])triggerDischarge();
 emitEffect(p.x+dirX(fire_angle)*8,p.y+dirY(fire_angle)*8,FX_MUZZLE,4,3,0);
 if(!m[M_SIEGE]&&p.weapon!=W_THERMAL_LANCE){p.charge=0;charged_scale_=0;if(!construct_origin)rancor_scale_=0;}
 calibration_=calibration_>20?calibration_-20:0;
 if(!construct_origin&&m[M_X_MORTAR])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_MINE&&constructs[c].age<60){constructs[c].vx=dirX(fire_angle)*7;constructs[c].vy=dirY(fire_angle)*7;constructs[c].life=60;}
 if(!construct_origin&&(m[M_X_HEAVY_CALIBER]||m[M_X_PROPULSIVE_MUNITIONS])){int recoil=m[M_X_HEAVY_CALIBER]*20+100*m[M_X_PROPULSIVE_MUNITIONS];if(m[M_X_POLAR_INVERSION])recoil=-recoil;p.vx-=dirX(fire_angle)*recoil/FP;p.vy-=dirY(fire_angle)*recoil/FP;}
 if(!construct_origin&&m[M_X_ANNIHILATION])damagePlayer(2+projectiles/3,true);
}
unsigned Game::reduceSelfDamageMilli(unsigned amount,bool shield) {
 // mediChargeReduceSelfDamage: redirect .4 of the original damage, split
 // equally among live Guardians, before player selfDamageMult/reduction.
 if(player.mods[M_X_MEDI_CHARGE]){
  unsigned count=0;for(const Construct &c:constructs)count+=c.active&&c.hull>0&&c.kind==C_GUARDIAN;
  if(count){unsigned redirected=(unsigned)((uint64_t)amount*40/100),remaining=redirected;amount-=redirected;
   for(unsigned i=0,seen=0;i<MAX_CONSTRUCTS;i++){Construct &c=constructs[i];if(!c.active||c.hull<=0||c.kind!=C_GUARDIAN)continue;
    unsigned share=redirected/count+(seen++<redirected%count?1u:0u);share=minv(remaining,share);remaining-=share;
    unsigned total=share+c.redirected_fraction;c.redirected_fraction=(uint16_t)(total%1000);if(total>=1000)damageConstruct(i,(int)(total/1000));
   }
  }
 }
 uint64_t reduced=(uint64_t)amount*incomingDamagePercent(shield,true)/100;
 if(shield&&player.mods[M_BARRIER]&&reduced>25000)reduced=25000;
 return (unsigned)(reduced>2000000000u?2000000000u:reduced);
}
void Game::damageShieldSelf(unsigned amount_milli) {
 Player &p=player;if(state!=Playing||p.hull<=0||p.shield<=0||!amount_milli)return;
 unsigned reduced=reduceSelfDamageMilli(amount_milli,true);recordRancorDamage(reduced);
 uint64_t total=(uint64_t)discharge_self_fraction_+reduced;
 unsigned damage=(unsigned)(total/1000);discharge_self_fraction_=(uint16_t)(total%1000);
 int actual=minv(p.shield,damage);p.shield=(int16_t)(p.shield-actual);
 if(actual){run_stats.damage_taken=addSaturated(run_stats.damage_taken,(unsigned)actual);emitFeedback(FeedbackSelfDamage,frame,p.x,p.y,(unsigned)actual);}
 if(!p.shield){p.shield_delay=stats.shield_cooldown;shieldBroken();}
 else if(!p.mods[M_X_TEMPEST_BREAK])p.shield_delay=stats.shield_cooldown;
}
void Game::damageHullSelf(unsigned amount_milli) {
 Player &p=player;if(state!=Playing||p.hull<=0||!amount_milli)return;
 unsigned reduced=reduceSelfDamageMilli(amount_milli,false);recordRancorDamage(reduced);
 uint64_t total=(uint64_t)discharge_gemini_fraction_+reduced;
 unsigned damage=(unsigned)(total/1000);discharge_gemini_fraction_=(uint16_t)(total%1000);
 int actual=minv(p.hull,damage);p.hull=(int16_t)(p.hull-actual);
 if(!actual)return;
 emitFeedback(FeedbackSelfDamage,frame,p.x,p.y,(unsigned)actual);
 run_stats.damage_taken=addSaturated(run_stats.damage_taken,(unsigned)actual);run_stats.hull_damage=(uint16_t)minv(65535,run_stats.hull_damage+actual);
 if(p.mods[M_ADAPTIVE_ARMOR]){armor_fraction_=(uint16_t)minv(65000,armor_fraction_+actual*175);unsigned added=armor_fraction_/1000;armor_fraction_%=1000;
  for(unsigned i=0;i<50&&added;i++)if(!armor_timers_[i]){armor_timers_[i]=(uint16_t)(75*(100+20*p.mods[M_CHANNELING]+20*p.mods[M_CORE_SHIELDING])/100);--added;}
 }
 preventDeath();
}
void Game::damagePlayer(int amount,bool self_damage,int attacker_angle,bool continuous) {
 Player &p=player;const uint8_t *m=p.mods;
 if(state!=Playing||amount<=0||p.hull<=0||(!self_damage&&p.invulnerable&&(!continuous||p.invulnerable>normal_hit_grace_)))return;
 int initial_hull=p.hull,initial_shield=p.shield;
 if(!self_damage&&daily(DAILY_NOVA_BROTHERS)&&!daily_damage_redirect_){
  for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&(constructs[i].kind==C_ALLY||constructs[i].kind==C_INTERCEPTOR||constructs[i].kind==C_GUARDIAN)){daily_damage_redirect_=1;damageConstruct(i,maxv(1,amount*40/100));daily_damage_redirect_=0;break;}
 }


 if(self_damage){damageHullSelf((unsigned)amount*1000);return;} else {
  const bool shielded=p.shield>0&&!m[M_X_SHIELDED_DRONES]&&p.shield_type!=S_BASTION;
  int hull_percent=incomingDamagePercent(false,false),shield_percent=incomingDamagePercent(true,false);
  amount=amount*(shielded?shield_percent:hull_percent)/100;
  untouched_=0;if(m[M_X_REBUKE])stored_damage_=(uint16_t)minv(200,stored_damage_+amount);
  if(p.shield>0&&!m[M_X_SHIELDED_DRONES]&&p.shield_type!=S_BASTION){
   if(m[M_BARRIER])amount=minv(amount,25);
   if(m[M_X_GEMINI]){int direct=maxv(1,amount/2);p.hull=maxv(0,p.hull-direct);amount-=direct;}

   int absorbed=minv(p.shield,amount);p.shield-=absorbed;amount-=absorbed;
   if(!m[M_X_TEMPEST_BREAK]||p.shield==0)p.shield_delay=stats.shield_cooldown;
   if(m[M_X_REFLEXIVE_SHIELDS])blast(p.x,p.y,stats.shield_radius,amount/2+4);
   if(m[M_X_GALVANIC_OUTBURST])for(unsigned j=0;j<(unsigned)(2+m[M_X_GALVANIC_OUTBURST]);j++){int n=emitBullet(p.x,p.y,(uint8_t)range(256),4,8*m[M_X_GALVANIC_OUTBURST],SHOT_SPARK,45);if(n>=0)bullets[n].homing=6;}
   if(p.shield==0)shieldBroken();
  }
  if(amount>0){
   amount=maxv(1,amount-stats.plating);
   if(shielded&&shield_percent)amount=amount*hull_percent/shield_percent;
   p.hull=maxv(0,p.hull-amount);
   if(amount>=12&&m[M_X_RETRIBUTION])spawnConstruct(C_MINE);
   if(amount*100>=p.max_hull*15&&m[M_X_OUTRAGE_MODULE]&&!outrage_clock_&&p.hull>0&&!m[M_X_HIDDEN_POWER]&&!m[M_OVERSEER]){
    outrage_clock_=2;uint8_t oldangle=p.angle;uint16_t delay=p.fire_delay,fraction=fire_fraction_;
    if(attacker_angle<0){int target=nearestTarget(p.x,p.y);if(target>=0)attacker_angle=direction(delta(targetX(target),p.x,worldWidth()),delta(targetY(target),p.y,worldHeight()));}
    if(attacker_angle>=0)p.angle=(uint8_t)attacker_angle;
    forced_empowered_=true;fireWeapon();forced_empowered_=false;p.angle=oldangle;p.fire_delay=delay;fire_fraction_=fraction;
   }

   if(p.body==B_VIPER){for(unsigned i=0;i<7;i++){int s=emitBullet(p.x,p.y,(uint8_t)(i*256/7),4,12+level/2,SHOT_DART,42,0,2);if(s>=0)bullets[s].homing=4;}}
  }
  if(!continuous){p.invulnerable=maxv(p.invulnerable,7);normal_hit_grace_=7;}damage_flash=18;
 }
 if(!self_damage&&p.shield_type==S_BASTION&&initial_hull>p.hull)releaseBastionEnergy((unsigned)(initial_hull-p.hull));
 if(!self_damage)recordRancorDamage((unsigned)maxv(0,initial_hull-p.hull+initial_shield-p.shield)*1000);
 if(!self_damage)recordMaelstromDamage((unsigned)maxv(0,initial_hull-p.hull+initial_shield-p.shield));
 if(self_damage)emitFeedback(FeedbackSelfDamage,frame,p.x,p.y,(unsigned)maxv(0,initial_hull-p.hull+initial_shield-p.shield));
 run_stats.damage_taken=addSaturated(run_stats.damage_taken,(unsigned)maxv(0,initial_hull-p.hull+initial_shield-p.shield));
 if(!self_damage&&m[M_ADAPTIVE_ARMOR]){unsigned lost=maxv(0,initial_hull-p.hull)+maxv(0,initial_shield-p.shield);armor_fraction_=(uint16_t)minv(65000,armor_fraction_+lost*175);
  unsigned added=armor_fraction_/1000;armor_fraction_%=1000;
  for(unsigned i=0;i<50&&added;i++)if(!armor_timers_[i]){armor_timers_[i]=(uint16_t)(75*(100+20*m[M_CHANNELING]+20*m[M_CORE_SHIELDING])/100);--added;}
 }
 run_stats.hull_damage=(uint16_t)minv(65535,run_stats.hull_damage+maxv(0,initial_hull-p.hull));
 preventDeath();
}
void Game::preventDeath() {
 if(player.hull>0)return;
 if(player.mods[M_X_LAST_STAND]&&!last_stand_used_){triggerLastStand();return;}
 player.hull=0;state=GameOver;emitEffect(player.x,player.y,FX_BURST,35,30,1);
}
void Game::shieldBroken() {
 Player &p=player;++run_stats.shield_breaks;
 bool transfer=p.mods[M_X_SHIELDED_DRONES];int rad=activeShieldRadius(),power=activeShieldEffect();
 for(unsigned center=0;center<(transfer?(unsigned)MAX_CONSTRUCTS:1u);center++){
  if(transfer&&(!constructs[center].active||constructs[center].kind>C_GUARDIAN))continue;
  int32_t sx=transfer?constructs[center].x:p.x,sy=transfer?constructs[center].y:p.y;
  emitEffect(sx,sy,FX_SHIELD_BREAK,rad,18,3);
  if(p.mods[M_VOLATILE_SHIELDS])blast(sx,sy,rad*2,1000*power/100);
  if(p.mods[M_EMERGENCY_SYSTEMS]){if(transfer)constructs[center].grace_invulnerable=maxv(constructs[center].grace_invulnerable,24);else {p.invulnerable=maxv(p.invulnerable,24);emergency_clock_=24;}}
  if(p.mods[M_X_TEMPEST_BREAK])triggerTempest(sx,sy,(unsigned)power);
  if(p.mods[M_X_DEATH_BLOSSOM])for(unsigned i=0;i<16;i++){int n=emitBullet(sx,sy,(uint8_t)(i*16),maxv(3,weapons[p.weapon].speed),weapons[p.weapon].damage*stats.damage/100,SHOT_BLASTER,45);if(n>=0)bullets[n].blast=10;}
  if(p.shield_type==S_WARP){int distance=rad*2;uint8_t angle=transfer?constructs[center].angle:p.angle;
   int32_t nx=wrap(sx+dirX(angle)*distance,worldWidth()),ny=wrap(sy+dirY(angle)*distance,worldHeight());
   if(transfer){constructs[center].x=nx;constructs[center].y=ny;constructs[center].grace_invulnerable=maxv(constructs[center].grace_invulnerable,23);}else{p.x=playerDestination(sx+dirX(angle)*distance,true);p.y=playerDestination(sy+dirY(angle)*distance,false);nx=p.x;ny=p.y;p.invulnerable=maxv(p.invulnerable,23);}
   blast(sx,sy,25,750*power/100);blast(nx,ny,25,750*power/100);emitEffect(sx,sy,FX_WARP,25,16,3);
  }
 }
}
void Game::creditDamage(unsigned actual) {
 if(actual>run_stats.highest_hit)run_stats.highest_hit=actual;
 if(player.mods[M_ESSENCE_SAP])essence_=(uint32_t)minv(player.max_hull*50,(int)essence_+(int)minv(actual,2000000)*10);
 run_stats.damage_dealt=addSaturated(run_stats.damage_dealt,actual);
}
void Game::hitEnemy(unsigned index,int damage,bool burn,bool area) {
 if(index>=MAX_ENEMIES||!enemies[index].active||enemies[index].boss_hidden||damage<=0)return;
 Enemy &e=enemies[index];if(!physicalCore(e.type))return;
 if(e.type==E_MINELAYER&&!burn)for(unsigned n=0;n<MAX_ENEMIES;n++)if(enemies[n].active&&enemies[n].type==E_MINE_SHIELD&&enemies[n].leader==(int)index&&enemies[n].leader_generation==e.generation){if(!area)hitEnemy(n,damage,false,false);return;}
 const uint8_t *m=player.mods;
 int64_t scaled=(int64_t)damage*globalDamagePermille()/1000;
 if(isBoss(e.type))scaled=scaled*bossDamagePercent(e,area)/100;
 if(scaled<=0)return;
 if(burn){scaled=scaled*burnDamagePercent()/100;
  if(m[M_CORROSION])e.vulnerability=(uint8_t)minv(60,e.vulnerability+2);
 }else if(m[M_BLITZ]&&e.type!=E_ASTEROID&&(int64_t)e.hull*100>=(int64_t)e.max_hull*95)scaled=scaled*120/100;
 scaled=scaled*(100+e.vulnerability)/100;
 damage=(int)minv((int64_t)2000000000,scaled);
 if(e.hard_shield>0){int absorbed=minv(e.hard_shield,damage);e.hard_shield-=absorbed;damage-=absorbed;creditDamage((unsigned)absorbed);emitFeedback(FeedbackDamage,frame,e.x,e.y,absorbed);e.flash=3;if(damage<=0)return;}
 if(e.type==E_DWELLER_DRONE&&!burn)damage=maxv(1,damage*5/100);
 if(e.type==E_SWARMLING_GREEN&&!burn){unsigned fractional=(unsigned)damage+e.damage_fraction;e.damage_fraction=(uint8_t)(fractional%20);damage=(int)(fractional/20);if(!damage)return;}
 unsigned actual=(unsigned)maxv(0,minv(e.hull,damage));creditDamage(actual);emitFeedback(FeedbackDamage,frame,e.x,e.y,actual);
 e.hull=maxv(0,e.hull-damage);e.flash=3;
 if(!burn&&m[M_INCENDIARY]&&(!e.burn_time||damage/4>=e.burn_damage)){e.burn_time=90;e.burn_ignite=1;e.burn_damage=(int16_t)clamp(damage/4,1,30000);}
 if(m[M_TERMINATE]&&e.hull<e.max_hull*12/100)e.hull=0;
 if(e.hull<=0)destroyEnemy(index);
}
void Game::blast(int32_t x,int32_t y,int radius,int damage,int excluded) {
 radius=clamp(radius,1,100);
 if(player.mods[M_CONCENTRATED_BLAST])damage=damage*135/100;
 emitEffect(x,y,FX_RING,radius,12,1);
 for(unsigned i=0;i<MAX_BOSS_PARTS;i++)if(boss_parts[i].active&&boss_parts[i].kind<BP_CRUCIBLE_CORNER&&(int)(MAX_ENEMIES+i)!=excluded){BossPart &part=boss_parts[i];if(!enemies[part.owner].boss_hidden&&sweptBossPart(part,x,y,x,y,radius))hitBossPart(i,damage,false,true);}
 for(unsigned i=0;i<MAX_ENEMIES;i++)if((int)i!=excluded&&enemies[i].active){
  int reach=radius+enemies[i].radius;
  if(distance2(x,y,enemies[i].x,enemies[i].y)<=reach*reach)hitEnemy(i,damage,false,true);
 }
}
void Game::awardEnemyPart(unsigned owner,unsigned numerator,unsigned denominator,bool count_kill,int32_t x,int32_t y) {
 if(owner>=MAX_ENEMIES||!denominator)return;
 const Enemy &e=enemies[owner];
 auto portion=[&](uint64_t value)->unsigned {uint64_t result=numerator==1&&denominator==6?value-value*5/6:value*numerator/denominator;return result>UINT32_MAX?UINT32_MAX:(unsigned)result;};
 unsigned treasure=portion((uint64_t)(enemy_types[e.type].xp*10*(e.elite+1)+wave)*e.treasure_scale/256);
 {uint64_t points=(uint64_t)treasure*(100+scoreBonus())/100;score=addSaturated(score,points>UINT32_MAX?UINT32_MAX:(uint32_t)points);}
 unsigned experience=portion((uint64_t)enemy_types[e.type].xp*(1+e.elite)*e.treasure_scale/256);
 if(experience)emitPickup(x<0?e.x:x,y<0?e.y:y,PICKUP_XP,minv(65535,experience));
 if(count_kill){++kills;run_stats.enemy_kills[e.type]=(uint16_t)minv(65535,run_stats.enemy_kills[e.type]+1);kill_buff_=150;}
}
void Game::destroyEnemy(unsigned index) {
 if(!enemies[index].active)return;
 Enemy e=enemies[index];
 for(unsigned b=0;b<FRIENDLY_BULLETS;b++)if(bullets[b].active&&bullets[b].kind==SHOT_DART&&bullets[b].pierce==254&&bullets[b].last_hit==(int)index)bullets[b].active=0;
 enemies[index].active=0;for(unsigned part=0;part<MAX_BOSS_PARTS;part++)if(boss_parts[part].owner==index)clearBytes(&boss_parts[part],sizeof(BossPart));
 if(e.type!=E_WORM&&e.type!=E_BULWARK){
 ++kills;
 run_stats.enemy_kills[e.type]=(uint16_t)minv(65535,run_stats.enemy_kills[e.type]+1);
 unsigned treasure=(unsigned)((uint64_t)(enemy_types[e.type].xp*10*(e.elite+1)+wave)*e.treasure_scale/256);
 {uint64_t points=(uint64_t)treasure*(100+scoreBonus())/100;score=addSaturated(score,points>UINT32_MAX?UINT32_MAX:(uint32_t)points);}
 kill_buff_=150;
 emitEffect(e.x,e.y,FX_BURST,e.radius+7,14,isBoss(e.type)?3:1);
 emitPickup(e.x,e.y,PICKUP_XP,(unsigned)minv(65535,(uint64_t)enemy_types[e.type].xp*(1+e.elite)*e.treasure_scale/256));
 } else emitEffect(e.x,e.y,FX_BURST,e.radius+7,14,1);
 rewardChallenger(e);
 onEnemyDestroyedConstructEffects(e);
 #include "enemy_death_hazards.inc"
 #include "enemy_death_dweller.inc"
 #include "enemy_death_summons.inc"
 #include "enemy_death_constructor.inc"
 {const Enemy &dead=e;
 #include "enemy_death_parts.inc"
 }
 if(player.mods[M_X_DEFLAGRATION]&&(e.burn_time||e.wake_time)&&rupture_depth_<3){++rupture_depth_;blast(e.x,e.y,38,25);for(unsigned j=0;j<MAX_ENEMIES;j++)if(enemies[j].active&&distance2(e.x,e.y,enemies[j].x,enemies[j].y)<38*38){enemies[j].burn_time=90;enemies[j].burn_damage=maxv(enemies[j].burn_damage,5);}--rupture_depth_;}
 if(range(player.body==B_RESEARCH?12:28)==0)emitPickup(e.x,e.y,PICKUP_HEAL,12);

 if(isBoss(e.type)){++run_stats.boss_kills;if(boss_index==(int)index)boss_index=-1;emitPickup(e.x,e.y,PICKUP_REROLL,2);emitPickup(e.x,e.y,PICKUP_HEAL,35);next_wave=minv(next_wave,60);
  if(e.type==E_STAR_EATER&&!(run_flags&RUN_ENDLESS)){victory=true;state=GameOver;for(unsigned j=0;j<MAX_ENEMIES;j++)enemies[j].active=0;clearBytes(boss_parts,sizeof(boss_parts));for(unsigned j=0;j<MAX_BULLETS;j++)if(bullets[j].owner)bullets[j].active=0;}
 }
 if(dailyMod(DAILY_GRACE)&&range(5)==0)emitPickup(e.x,e.y,PICKUP_POWER,1+range(3));
 if(e.type==E_STAR_EATER&&daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0)score=addSaturated(score,1000000);
 if(e.type==E_ASTEROID&&e.phase){
  static const uint16_t hull[]={10,60,240,720};static const uint8_t radii[]={4,8,13,19};
  unsigned children=3+range(3),phase=e.phase-1;
  for(unsigned j=0;j<children;j++){uint8_t a=(uint8_t)(j*256/children+range(20));int n=spawnEnemy(E_ASTEROID,pixel(e.x)+dirX(a)*3/FP,pixel(e.y)+dirY(a)*3/FP);
   if(n>=0){Enemy &child=enemies[n];child.phase=(uint8_t)phase;child.radius=radii[phase];child.hull=child.max_hull=hull[phase];child.vx=e.vx+dirX(a)/4;child.vy=e.vy+dirY(a)/4;}}
 }
 if(player.mods[M_SPLINTER])for(unsigned j=0;j<3;j++)emitBullet(e.x,e.y,(uint8_t)range(256),4,12*stats.damage/100,SHOT_SHRAPNEL,25,0,1);
 if((player.mods[M_RUPTURE]||(player.mods[M_PURGE]&&(e.burn_time||e.wake_time)))&&rupture_depth_<3){
  ++rupture_depth_;blast(e.x,e.y,20*stats.blast/100,maxv(1,e.max_hull*3/100));--rupture_depth_;
 }
}
int Game::spawnEnemy(unsigned type,int x,int y,unsigned elite) {
 if(type>=ENEMY_COUNT)return -1;
 if(player.mods[M_X_BRAVADO])elite=maxv(1,elite);
 unsigned required_parts=0;
 switch(type){case E_WORM:required_parts=elite==2?66u:elite==1?61u:31u;break;
 case E_BULWARK:required_parts=elite?13u:10u;break;
 case E_BOLT_THROWER:required_parts=elite?2u:0u;break;
 case E_SNAPPER:required_parts=elite==2?67u:elite==1?57u:29u;break;
 case E_ECLIPSE:required_parts=5;break;
 case E_STAR_EATER:case E_GLAUCUS:required_parts=29;break;
 case E_DREADNOUGHT:required_parts=6;break;case E_WARBRINGER:required_parts=12;break;
 case E_CERATOTITAN:required_parts=7;break;case E_STATION_OMEGA:required_parts=10;break;
 case E_CRUCIBLE:required_parts=3;break;default:break;}
 if(required_parts){unsigned free_parts=0;for(unsigned i=0;i<MAX_BOSS_PARTS;i++)free_parts+=boss_parts[i].max_hull==0;for(unsigned n=0;n<MAX_ENEMIES;n++)if(enemies[n].active&&(enemies[n].type==E_STAR_EATER||(enemies[n].type==E_BOLT_THROWER&&enemies[n].elite))){unsigned required=enemies[n].type==E_STAR_EATER?29u:2u;unsigned owned=0;for(unsigned j=0;j<MAX_BOSS_PARTS;j++)owned+=boss_parts[j].max_hull>0&&boss_parts[j].owner==n;unsigned reserve=owned<required?required-owned:0;free_parts=free_parts>reserve?free_parts-reserve:0;}if(free_parts<required_parts)return -1;}
 unsigned slot=MAX_ENEMIES;for(unsigned i=0;i<MAX_ENEMIES;i++)if(!enemies[i].active){slot=i;break;}
 if(slot==MAX_ENEMIES){++dropped_enemies;return -1;}
 Enemy &e=enemies[slot];clearBytes(&e,sizeof(e));const EnemyDef &def=enemy_types[type];
 e.active=1;e.leader=-1;e.generation=++enemy_serial_;if(!e.generation)e.generation=++enemy_serial_;e.damage_scale=e.treasure_scale=e.size_scale=256;e.summon_type=ENEMY_COUNT;e.type=(uint8_t)type;e.elite=(uint8_t)minv(2,elite);e.x=wrap(x*FP,worldWidth());e.y=wrap(y*FP,worldHeight());
 // setEnemyHull uses baseEnemyHullFactor=1. scaleWaves adds .00375
 // per wave beyond 120. Boss and rare-variant multipliers are separate.
 int late=maxv(0,wave-120);
 int hull=(int)minv(2000000,(int)((int64_t)def.hull*(800+late*3)/800));
 hull=(int)minv(2000000,(int)((int64_t)hull*(100+e.elite*60)/100));
 if(isBoss(type)&&(run_flags&RUN_NEMESIS))hull=hull*(run_flags&RUN_ANNIHILATION?350:run_flags&RUN_MAYHEM?340:320)/240;
 if(daily(DAILY_NOVA_BROTHERS))hull=(int)minv(2000000,(int64_t)hull*2);
 if(dailyMod(DAILY_TOUGHER))hull=(int)minv(2000000,(int64_t)hull*125/100);
 if(isBoss(type)){if(daily(DAILY_BOSS_RUSH))hull=(int)minv(2000000,(int64_t)hull*2);if(dailyMod(DAILY_VENGEANCE)){hull=(int)minv(2000000,(int64_t)hull*125/100);e.damage_scale=288;e.treasure_scale=384;}}
 e.hull=e.max_hull=maxv(1,hull);if(type<64)run_stats.seen_enemies|=uint64_t(1)<<(type&63);else run_stats.seen_enemies_hi|=uint64_t(1)<<(type&63);
 e.radius=(uint8_t)(type==E_JUGGERNAUT?(elite==1?27:elite==2?31:25):(def.radius+elite));e.angle=(uint8_t)range(256);e.fire_delay=20+range(40);
 if(type==E_JUGG_MISSILE||type==E_JUGG_MISSILE_HOMING)e.max_hull=type==E_JUGG_MISSILE?75:90;
 if(type==E_ASTEROID){e.phase=1;e.vx=dirX(e.angle)/4;e.vy=dirY(e.angle)/4;}
 if(isBoss(type)){boss_index=(int16_t)slot;e.fire_delay=80;updateBossParts(slot);}
 else if(type==E_WORM||type==E_BULWARK||type==E_SNAPPER||type==E_ECLIPSE)updateBossParts(slot);
 emitEffect(e.x,e.y,FX_SPAWN,e.radius+4,15,1);return (int)slot;
}
#include "daily.inc"
#include "waves.inc"
#include "boss_ai.inc"
#include "enemy_ai.inc"
#include "eclipse_shield.inc"
void Game::updateEnemies() {
 #include "enemy_part_status.inc"
 for(unsigned i=0;i<MAX_ENEMIES;i++){
  Enemy &e=enemies[i];if(!e.active)continue;
  const uint16_t entry_generation=e.generation;const EnemyDef &def=enemy_types[e.type];++e.age;if(e.flash)--e.flash;
  if(e.hard_shield_timer&&!--e.hard_shield_timer)e.hard_shield=0;
  if(e.vulnerability&&frame%20==0)--e.vulnerability;
  if(e.wake_time)--e.wake_time;
  if(e.burn_time){--e.burn_time;if(frame%15==0){hitEnemy(i,maxv(1,e.burn_damage)*(int)(e.burn_ignite?igniteDamagePercent():100)/100,true);if(!e.active)continue;}}
  int dx=delta(player.x,e.x,worldWidth()),dy=delta(player.y,e.y,worldHeight());
  if(player.mods[M_X_DECOY_SIGNAL])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_GUARDIAN&&distance2(e.x,e.y,constructs[c].x,constructs[c].y)<100*100){dx=delta(constructs[c].x,e.x,worldWidth());dy=delta(constructs[c].y,e.y,worldHeight());break;}
  uint8_t aim=direction(dx,dy);
  int32_t daily_oldx=e.x,daily_oldy=e.y;
  if(e.type==E_BOLT_THROWER&&e.elite)updateBossParts(i);
  if(isBoss(e.type))updateBoss(e);
  else if(!updateRegularEnemy(e)){
   // Every non-asteroid family has an explicit source-derived handler.
   // Asteroids keep their original seeded drift and rotation, without guns.
   if(e.type==E_ASTEROID){e.angle=(uint8_t)(e.angle+1);e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());}
  }
  if(!e.active||e.generation!=entry_generation)continue;
  if(dailyEnemySpeedPercent()!=100){unsigned extra=dailyEnemySpeedPercent()-100;e.x=wrap(e.x+delta(e.x,daily_oldx,worldWidth())*FP*(int)extra/100,worldWidth());e.y=wrap(e.y+delta(e.y,daily_oldy,worldHeight())*FP*(int)extra/100,worldHeight());if(isBoss(e.type))updateBossParts(i);}
  if(e.stun)--e.stun;
  int contact=e.radius+player.radius;
  if(!e.boss_hidden&&physicalCore(e.type)&&!player.mods[M_X_APOTHEOSIS]&&e.age>20&&distance2(e.x,e.y,player.x,player.y)<contact*contact){
   if(!player.invulnerable){int crash=12+approx(player.vx,player.vy)/18;
    if(player.body==B_HULLBREAKER)crash*=2;
    crash=crash*stats.crash_damage/100;
    int taken=(e.type==E_ASTEROID?(5+5*e.phase):def.contact)*crashContactPercent()/100;
    hitEnemy(i,crash);damagePlayer(taken,false,direction(delta(e.x,player.x,worldWidth()),delta(e.y,player.y,worldHeight())));
    if(e.active&&player.mods[M_X_PHANTOM_STRIKE]){player.x=playerDestination(e.x+dirX(player.angle)*(e.radius+player.radius+6),true);player.y=playerDestination(e.y+dirY(player.angle)*(e.radius+player.radius+6),false);player.angle=(uint8_t)(player.angle+128);uint16_t delay=player.fire_delay,fraction=fire_fraction_;forced_empowered_=true;fireWeapon();forced_empowered_=false;player.fire_delay=delay;fire_fraction_=fraction;}
    if(player.mods[M_SKIRMISH])player.invulnerable=maxv(player.invulnerable,14);
    if(player.body==B_VIPER)for(unsigned j=0;j<5;j++){int n=emitBullet(player.x,player.y,(uint8_t)(aim+128+(int)j*10-20),4,18,SHOT_DART,40);if(n>=0)bullets[n].homing=3;}
    player.vx-=dx*3;player.vy-=dy*3;
   }
  }
  if(e.active&&!e.boss_hidden&&physicalCore(e.type)&&segment_count&&frame%5==i%5){for(unsigned s=0;s<segment_count;s++)if(distance2(e.x,e.y,segment_x[s],segment_y[s])<(e.radius+5)*(e.radius+5)){hitEnemy(i,(10+approx(player.vx,player.vy)/24)*stats.crash_damage/100);break;}}
 }
 for(unsigned part=0;part<MAX_BOSS_PARTS;part++){BossPart &bp=boss_parts[part];if(!bp.active||bp.kind>=BP_CRUCIBLE_CORNER||enemies[bp.owner].boss_hidden)continue;
  if(bp.kind==BP_ECLIPSE_SHIELD&&sweptBossPart(bp,player.x,player.y,player.x,player.y,player.radius)){triggerEclipseShield(part,false,true);continue;}
  if(!player.invulnerable&&sweptBossPart(bp,player.x,player.y,player.x,player.y,player.radius)){const Enemy& part_owner=enemies[bp.owner];unsigned type=part_owner.type;int contact_damage=enemy_types[type].contact;
   if(type==E_SNAPPER){if(bp.kind==BP_SNAPPER_JAW_LEFT||bp.kind==BP_SNAPPER_JAW_RIGHT){if(!part_owner.phase)continue;contact_damage=60;}else contact_damage=10;contact_damage=contact_damage*part_owner.damage_scale/256;}
   if(type==E_ECLIPSE)contact_damage=15*part_owner.damage_scale/256;
   int damage=contact_damage*crashContactPercent()/100;hitBossPart(part,(20+approx(player.vx,player.vy)/12)*stats.crash_damage/100);damagePlayer(damage);}}
}
void Game::buildGrid() {
 for(unsigned i=0;i<256;i++)grid_[i]=-1;
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&!enemies[i].boss_hidden&&physicalCore(enemies[i].type)){int cell=pixel(enemies[i].x)/32+(pixel(enemies[i].y)/30)*((worldWidth()+31)/32);grid_next_[i]=grid_[cell];grid_[cell]=(int16_t)i;}
}
unsigned Game::shieldConstructPermille() const {
 if(!player.mods[M_X_SHIELDED_DRONES])return 1000;
 unsigned n=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)n+=constructs[i].active&&constructs[i].kind<=C_GUARDIAN;
 // calculateDroneMult 0x1400bbb1f: .3+.7*exp(-.09*N).
 static const uint16_t values[]={1000,940,885,834,788,746,708,673,641,611,585,560,538,517,499,481,466};
 static const uint16_t shock[]={1000,920,848,784,726,673,626,584,545,511,480,452,427,404,384,366,349};
 return player.shield_type==S_SHOCKWAVE?shock[n]:values[n];
}
unsigned Game::activeShieldRadius() const {
 unsigned rad=stats.shield_radius;if(player.mods[M_X_SANCTUARY])rad=rad*160/100;
 if(player.shield_type==S_SIPHON)rad=rad*(10000+siphon_clock_)/10000;
 return rad*(10000+shield_charge_)/10000*shieldConstructPermille()/1000;
}
unsigned Game::activeShieldEffect() const {
 unsigned power=stats.shield_effect;
 if(player.mods[M_CHANNELING])power=power*(100+armor_stacks_)/100;
 power=power*(10000+shield_charge_)/10000;
 power+=player.mods[M_X_ENERGIZED_SHIELDS]*approx(player.vx,player.vy)/20;
 if(player.shield_type==S_SIPHON)power=power*(10000+siphon_clock_)/10000;
 return power*shieldConstructPermille()/1000;
}
int Game::shieldDistance2(int32_t x,int32_t y) const {
 if(player.mods[M_X_SHIELDED_DRONES]){int d=0x3fffffff;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&constructs[i].kind<=C_GUARDIAN)d=minv(d,distance2(x,y,constructs[i].x,constructs[i].y));return d;}
 return distance2(x,y,player.mods[M_X_SANCTUARY]?sanctuary_x:player.x,player.mods[M_X_SANCTUARY]?sanctuary_y:player.y);
}
void Game::updateShields() {
 Player &p=player;if(p.shield<=0){siphon_clock_=0;return;}
 int rad=activeShieldRadius(),power=activeShieldEffect();
 bool siphon=p.shield_type==S_SIPHON;
 if(siphon){bool target=false;for(unsigned i=0;i<MAX_ENEMIES+MAX_BOSS_PARTS&&!target;i++)target=targetActive(i)&&shieldDistance2(targetX(i),targetY(i))<rad*rad;
  siphon_clock_=(uint16_t)(target?minv(4000,siphon_clock_+48):maxv(0,siphon_clock_-30));}
 bool transfer=p.mods[M_X_SHIELDED_DRONES];
 for(unsigned center=0;center<(transfer?(unsigned)MAX_CONSTRUCTS:1u);center++){
  if(transfer&&(!constructs[center].active||constructs[center].kind>C_GUARDIAN))continue;
  int32_t sx=transfer?constructs[center].x:p.mods[M_X_SANCTUARY]?sanctuary_x:p.x;
  int32_t sy=transfer?constructs[center].y:p.mods[M_X_SANCTUARY]?sanctuary_y:p.y;
  if(frame%6==0){
   unsigned targets=0;for(unsigned i=0;i<MAX_ENEMIES+MAX_BOSS_PARTS;i++)if(targetInRadius(i,sx,sy,rad))++targets;
   unsigned drained=0;
   for(unsigned i=0;i<MAX_ENEMIES+MAX_BOSS_PARTS;i++)if(targetInRadius(i,sx,sy,rad)){
    int before=targetHull(i);unsigned owner=targetOwner(i);
    if(p.shield_type==S_HALO){damageTarget(i,60*power/100,true);if(targetActive(i)){if(i<MAX_ENEMIES){Enemy &e=enemies[i];e.burn_time=90;e.burn_ignite=1;e.burn_damage=(int16_t)maxv(e.burn_damage,60*power/100);}else {BossPart &part=boss_parts[i-MAX_ENEMIES];part.burn_time=90;part.burn_damage=(int16_t)maxv(part.burn_damage,60*power/100);}}}
    else if(siphon)damageTarget(i,maxv(1,24*power/100/maxv(1,targets)),true);
    else if(p.mods[M_WEAPONIZED_SHIELDS])damageTarget(i,maxv(1,4*power/100),true);
    drained+=maxv(0,before-targetHull(i));
    if(p.shield_type==S_SHOCKWAVE&&i<MAX_ENEMIES){Enemy &e=enemies[owner];int polar=p.mods[M_X_POLAR_INVERSION]?-1:1;e.vx+=delta(e.x,sx,worldWidth())*4*polar;e.vy+=delta(e.y,sy,worldHeight())*4*polar;}
   }
   if(p.shield_type==S_HALO&&targets){ // haloHitsSelf: .1/source frame per burning solid.
    halo_fraction_=(uint16_t)minv(65000,halo_fraction_+targets*120*(int)incomingDamagePercent(true,true)/100);
    if(halo_fraction_>=100){unsigned cost=halo_fraction_/100;halo_fraction_%=100;p.shield=maxv(0,p.shield-(int)cost);p.shield_delay=stats.shield_cooldown;if(!p.shield)shieldBroken();}
   }
   if(siphon&&drained){siphon_fraction_+=drained*15;if(siphon_fraction_>=1000){p.shield=minv(shieldRecoveryCap(),p.shield+siphon_fraction_/1000);siphon_fraction_%=1000;}}
  }
  if(p.shield_type==S_ORBITAL){for(unsigned orb=0;orb<3;orb++){
   uint8_t a=(uint8_t)(frame*2+orb*256/3);int32_t ox=wrap(sx+dirX(a)*rad,worldWidth()),oy=wrap(sy+dirY(a)*rad,worldHeight());
   if(frame%5==0){for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&physicalCore(enemies[i].type)&&distance2(ox,oy,enemies[i].x,enemies[i].y)<(enemies[i].radius+5)*(enemies[i].radius+5))hitEnemy(i,6*power/100);for(unsigned part=0;part<MAX_BOSS_PARTS;part++)if(boss_parts[part].active&&boss_parts[part].kind<BP_CRUCIBLE_CORNER&&sweptBossPart(boss_parts[part],ox,oy,ox,oy,5))hitBossPart(part,6*power/100);}
   for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;i++)if(bullets[i].active&&bullets[i].owner&&distance2(ox,oy,bullets[i].x,bullets[i].y)<7*7)bullets[i].active=0;
  }}
 }
}
void Game::destroyHostileBullet(unsigned index) {
 if(index>=MAX_BULLETS||!bullets[index].active)return;
 const Bullet dead=bullets[index];bullets[index].active=0;
 if(!dead.owner)return;
 #include "enemy_projectile_death_hazards.inc"
 #include "enemy_projectile_death_parts.inc"
 #include "enemy_projectile_death_summons.inc"
 #include "enemy_projectile_death_celestial.inc"
 #include "enemy_projectile_death_eclipse.inc"
 #include "enemy_projectile_death_myrmidon.inc"
}
void Game::updateBullets() {
 unsigned vortex_count=0;for(unsigned n=0;n<FRIENDLY_BULLETS;n++)vortex_count+=bullets[n].active&&!bullets[n].owner&&bullets[n].kind==SHOT_VORTEX&&bullets[n].field_scale;
 uint8_t lodged[MAX_ENEMIES+MAX_BOSS_PARTS]={0};
 for(unsigned i=0;i<FRIENDLY_BULLETS;i++){const Bullet &b=bullets[i];if(b.active&&b.kind==SHOT_DART&&b.pierce==254&&b.last_hit>=0&&b.last_hit<MAX_ENEMIES+MAX_BOSS_PARTS)++lodged[b.last_hit];}
 for(unsigned i=0;i<MAX_BULLETS;i++) {
  Bullet &b=bullets[i];if(!b.active)continue;
  ++b.age;
  if(b.tracking){updateHomingStrike(b);if(!b.active)continue;}
  #include "enemy_projectiles_hazards.inc"
  #include "enemy_projectiles_dweller.inc"
  #include "enemy_projectiles_parts.inc"
  #include "enemy_projectiles_summons.inc"
  #include "enemy_projectiles_celestial.inc"
  #include "enemy_projectiles_pulsar.inc"
  #include "enemy_projectiles_remaining_flight.inc"
  #include "enemy_projectiles_myrmidon.inc"
  #include "enemy_projectiles_snapper.inc"
  #include "enemy_projectiles_eclipse.inc"
  #include "electrical_bolts.inc"
  if(!b.owner&&b.kind==SHOT_WAKE){updateScorchingWake(b);continue;}
  if(!b.owner&&b.kind==SHOT_DART&&b.pierce==254){
   int target=b.last_hit;bool part=target>=MAX_ENEMIES;
   if(target<0||target>=MAX_ENEMIES+MAX_BOSS_PARTS||!b.life||(!part&&!enemies[target].active)||(part&&!boss_parts[target-MAX_ENEMIES].active)){b.active=0;continue;}
   int32_t tx=part?boss_parts[target-MAX_ENEMIES].x:enemies[target].x,ty=part?boss_parts[target-MAX_ENEMIES].y:enemies[target].y;
   b.x=wrap(tx+b.vx,worldWidth());b.y=wrap(ty+b.vy,worldHeight());
   // dartBurn 0x1401626f0: damage*sizeMod*(1+.01*stuckCount) DPS.
   // Stored damage includes size once; global/burn modifiers apply in hitEnemy.
   unsigned amount=(unsigned)b.damage*(100u+lodged[target])+b.traveled;
   b.traveled=(uint16_t)(amount%3000);amount/=3000;
   if(amount){if(part)hitBossPart((unsigned)(target-MAX_ENEMIES),(int)amount,true);else hitEnemy((unsigned)target,(int)amount,true);}
   if(b.active&&!--b.life)b.active=0;
   continue;
  }
  if(b.life)--b.life;
  if(!b.life){
   if(b.owner){destroyHostileBullet(i);continue;}
   b.active=0;
   if(!b.owner&&(b.kind==SHOT_GRENADE||(b.kind==SHOT_SHRAPNEL&&b.blast))){
    int32_t bx=b.x,by=b.y;int bd=b.damage,br=b.blast;unsigned kind=b.kind;
    blast(bx,by,br,bd);
    if(kind==SHOT_GRENADE)for(unsigned j=0;j<3;j++){int n=emitBullet(bx,by,(uint8_t)(j*256/3+range(30)),2,maxv(1,bd/3),SHOT_SHRAPNEL,10);if(n>=0)bullets[n].blast=10;}
   }
   continue;
  }
  if(!b.owner&&b.kind==SHOT_VORTEX&&b.field_scale){updateVortexBullet(b,vortex_count);continue;}
  if(!b.owner&&b.kind==SHOT_VORTEX){
   b.x=wrap(b.x+b.vx,worldWidth());b.y=wrap(b.y+b.vy,worldHeight());
   if(frame%5==0){
    int reach=b.radius*2;
    for(unsigned j=0;j<MAX_ENEMIES+MAX_BOSS_PARTS;j++)if(targetInRadius(j,b.x,b.y,reach)){
     if(j<MAX_ENEMIES){int dx=delta(b.x,enemies[j].x,worldWidth()),dy=delta(b.y,enemies[j].y,worldHeight());int polar=player.mods[M_X_POLAR_INVERSION]?-1:1;enemies[j].vx+=clamp(dx,-20,20)*5*polar;enemies[j].vy+=clamp(dy,-20,20)*5*polar;}
     damageTarget(j,b.damage,true);
    }
   }
   if(b.life<25&&b.radius>5&&frame%4==0)--b.radius;
   continue;
  }
  if(!b.owner&&!b.tracking&&b.homing&&((frame+i)&3)==0){int target=nearestTarget(b.x,b.y,150);
   if(target>=0){uint8_t a=direction(delta(targetX(target),b.x,worldWidth()),delta(targetY(target),b.y,worldHeight()));
    b.angle=toward(b.angle,a,b.homing*3);
    b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
   }
  }
  if(b.kind==SHOT_FLAK&&!b.tracking){b.vx=b.vx*245/256;b.vy=b.vy*245/256;b.speed=(uint16_t)(b.speed*245/256);if(b.age%7==0&&b.radius>1)--b.radius;}
  if(b.kind==SHOT_BLASTER&&!b.tracking&&b.age%14==0&&b.radius>1)--b.radius;
  int32_t ox=b.x,oy=b.y;
  int move_percent=100;
  if(b.owner&&player.shield>0&&player.shield_type==S_TEMPORAL&&shieldDistance2(b.x,b.y)<(int)(activeShieldRadius()*activeShieldRadius()))move_percent=30;
  b.x=wrap(b.x+b.vx*move_percent/100,worldWidth());b.y=wrap(b.y+b.vy*move_percent/100,worldHeight());
  if(b.kind!=SHOT_MYRMIDON_BOMB)b.traveled=(uint16_t)minv(65535,b.traveled+approx(delta(b.x,ox,worldWidth()),delta(b.y,oy,worldHeight())));
  bool wrapped=absn(b.x-ox)>worldWidth()*FP/2||absn(b.y-oy)>worldHeight()*FP/2;
  if(wrapped){if(b.wraps){--b.wraps;b.damage=minv(2000000,b.damage);}else{if(b.owner)destroyHostileBullet(i);else b.active=0;continue;}}
  if(!b.owner&&player.mods[M_X_ANNIHILATION])for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;j++)if(bullets[j].active&&bullets[j].owner&&sweptHit(ox,oy,b.x,b.y,bullets[j].x,bullets[j].y,b.radius+bullets[j].radius)){
   b.damage-=bullets[j].damage;bullets[j].active=0;if(b.damage<=0){b.active=0;break;}}
  if(!b.active)continue;
  if(b.owner){
   if(player.shield>0&&player.shield_type==S_REFLECT&&shieldDistance2(b.x,b.y)<(int)(activeShieldRadius()*activeShieldRadius())){
    b.owner=0;b.damage=minv(2000000,b.damage*12*stats.shield_effect/100);b.homing=4;b.age=0;b.life=65;
    int target=nearestTarget(b.x,b.y);b.angle=target>=0?direction(delta(targetX(target),b.x,worldWidth()),delta(targetY(target),b.y,worldHeight())):(uint8_t)(b.angle+128);
    b.speed=(uint16_t)minv(16*FP,b.speed*3/2);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;continue;
   }
   if(daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0&&sweptHit(ox,oy,b.x,b.y,daily_egg_x,daily_egg_y,12+b.radius)){damageDailyEgg((unsigned)b.damage);destroyHostileBullet(i);continue;}
   if(interceptAdvancedProjectile(i,ox,oy))continue;
   if(sweptHit(ox,oy,b.x,b.y,player.x,player.y,player.radius+b.radius)){
    // Contact precedes Destroy: the secondary blast must not consume hit
    // grace before the projectile's direct damage reaches its target.
    int bullet_damage=b.damage;uint8_t bullet_angle=b.angle;
    damagePlayer(bullet_damage,false,(uint8_t)(bullet_angle+128));destroyHostileBullet(i);continue;
   }
   for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind!=C_BASTION&&sweptHit(ox,oy,b.x,b.y,constructs[c].x,constructs[c].y,constructs[c].radius+b.radius)){
    Construct &cr=constructs[c];bool ally=cr.kind==C_ALLY||cr.kind==C_INTERCEPTOR||cr.kind==C_GUARDIAN;
    if(ally&&(player.mods[M_X_EVASION]||player.mods[M_X_OUTMANEUVER])&&range(100)<35){
     if(player.mods[M_X_OUTMANEUVER]&&cr.kind==C_INTERCEPTOR)emitBullet(cr.x,cr.y,(uint8_t)(b.angle+128),5,18,SHOT_BLASTER,40);
     continue;
    }
    int damage=b.damage;
    if(player.mods[M_X_TACTICAL_LINK]&&distance2(cr.x,cr.y,player.x,player.y)<55*55)damage=damage*75/100;
    if(player.mods[M_X_SHIELDED_DRONES]&&cr.kind<=C_GUARDIAN&&player.shield>0){damage=(damage+1)/2;int absorbed=minv(player.shield,damage);player.shield-=absorbed;damage-=absorbed;player.shield_delay=stats.shield_cooldown;if(!player.shield)shieldBroken();}
    if(player.mods[M_X_NANOTECH_DISSEMINATION]&&cr.kind!=C_GUARDIAN)for(unsigned j=0;j<MAX_CONSTRUCTS;j++)if(constructs[j].active&&constructs[j].kind==C_GUARDIAN){damageConstruct(j,damage);damage=0;break;}
    if(cr.grace_invulnerable)damage=0;
    if(player.mods[M_X_SHIELDED_DRONES])damage=damage*90/100;
    damageConstruct(c,damage);
    if(player.mods[M_X_COUNTERPULSE]&&cr.kind==C_DEFENSE_DRONE)blast(cr.x,cr.y,25,18);
    if(player.mods[M_X_TACTICAL_BEACON]&&cr.kind==C_MINE&&cr.charges&&cr.fire_delay<3){blast(cr.x,cr.y,45,70*constructPower(C_MINE)/100);cr.fire_delay=30;if(!--cr.charges)cr.active=0;}
    destroyHostileBullet(i);break;
   }
   continue;
  }
  if(player.mods[M_X_TACTICAL_BEACON])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_MINE&&constructs[c].charges&&constructs[c].fire_delay<3&&sweptHit(ox,oy,b.x,b.y,constructs[c].x,constructs[c].y,constructs[c].radius+b.radius)){
   Construct &cr=constructs[c];blast(cr.x,cr.y,45,70*constructPower(C_MINE)/100);cr.fire_delay=30;if(!--cr.charges)cr.active=0;b.active=0;break;}
  if(!b.active)continue;
  bool part_collision=false;
  for(unsigned part=0;part<MAX_BOSS_PARTS&&b.active&&!part_collision;part++){
   BossPart &bp=boss_parts[part];if(!bp.active||bp.kind>=BP_CRUCIBLE_CORNER||enemies[bp.owner].boss_hidden||b.last_hit==(int)(MAX_ENEMIES+part))continue;
   if(!sweptBossPart(bp,ox,oy,b.x,b.y,b.radius))continue;
   int damage=b.damage;if(player.mods[M_SNIPE])damage=damage*(100+minv(80,b.traveled/2))/100;
   if((b.construct_source&128)&&enemies[bp.owner].vulnerability<100)++enemies[bp.owner].vulnerability;
   int32_t px=bp.x,py=bp.y;
   if(b.kind==SHOT_SPARK){bp.burn_time=(uint16_t)minv(300,bp.burn_time+90);bp.burn_damage=(int16_t)minv(500,absn(bp.burn_damage)+maxv(2,damage/5));}
   bool destroyed=damage>0&&hitBossPart(part,damage);
   if(b.kind==SHOT_DART&&!destroyed)destroyed=hitBossPart(part,damage/2,false,true);
   b.last_hit=(int16_t)(MAX_ENEMIES+part);
   if(b.blast){int portion=b.kind==SHOT_SPLIT||b.kind==SHOT_DART?50:b.kind==SHOT_GRENADE||b.kind==SHOT_MISSILE?100:15;blast(px,py,b.blast,damage*portion/100,b.last_hit);}
   if(b.kind==SHOT_DART&&!destroyed&&bp.active){b.pierce=254;b.life=(uint16_t)clamp(60*b.size_percent/100,1,60000);b.damage=(int)minv(2000000,(int)((int64_t)damage*b.size_percent/100));b.speed=0;b.traveled=0;b.vx=delta(b.x,bp.x,worldWidth())*FP;b.vy=delta(b.y,bp.y,worldHeight())*FP;b.blast=0;part_collision=true;continue;}
   if(b.kind==SHOT_RAIL&&destroyed){blast(px,py,13*stats.blast/100,damage/3,b.last_hit);for(unsigned sh=0;sh<3;sh++)emitBullet(px,py,(uint8_t)(b.angle+(int)sh*18-18),4,damage/5,SHOT_SHRAPNEL,18,0,1);}
   if(b.kind==SHOT_GRENADE){int bd=damage;b.active=0;for(unsigned sh=0;sh<3;sh++){int n=emitBullet(px,py,(uint8_t)(sh*256/3),2,maxv(1,bd/3),SHOT_SHRAPNEL,10);if(n>=0)bullets[n].blast=10;}part_collision=true;}
   else if(b.pierce&&destroyed){if(b.kind!=SHOT_DART)--b.pierce;}
   else if(b.bounces){--b.bounces;int next=nearestTarget(b.x,b.y);if(next>=0){b.angle=direction(delta(targetX(next),b.x,worldWidth()),delta(targetY(next),b.y,worldHeight()));b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;part_collision=true;}else b.active=0;}
   else b.active=0;
  }
  if(!b.active||part_collision)continue;
  int cx=pixel(b.x)/32,cy=pixel(b.y)/30,columns=(worldWidth()+31)/32,rows=(worldHeight()+29)/30;
  bool collision=false;
  for(int gy=-2;gy<=2&&!collision;gy++)for(int gx=-2;gx<=2&&!collision;gx++){
   int cellx=(cx+gx+columns)%columns,celly=(cy+gy+rows)%rows;
   for(int j=grid_[celly*columns+cellx];j>=0&&!collision;j=grid_next_[j]){
    Enemy &e=enemies[j];if(!e.active||e.boss_hidden||j==b.last_hit)continue;
    if(!sweptHit(ox,oy,b.x,b.y,e.x,e.y,b.radius+e.radius))continue;
    int damage=b.damage;if(player.mods[M_SNIPE])damage=damage*(100+minv(80,b.traveled/2))/100;
    bool destroy=false;uint16_t target_generation=e.generation;int32_t ex=e.x,ey=e.y;
    if(b.kind==SHOT_SPARK){e.burn_ignite=1;e.burn_time=(uint16_t)minv(300,e.burn_time+90);e.burn_damage=(int16_t)minv(500,e.burn_damage+maxv(2,damage/5));}
    if((b.construct_source&128)&&e.vulnerability<100)++e.vulnerability;
    hitEnemy((unsigned)j,damage);
    if(b.kind==SHOT_DART&&e.active&&e.generation==target_generation)hitEnemy((unsigned)j,damage/2,false,true);
    destroy=!e.active||e.generation!=target_generation;
    b.last_hit=(int16_t)j;
    if(b.blast){int portion=b.kind==SHOT_SPLIT||b.kind==SHOT_DART?50:b.kind==SHOT_GRENADE||b.kind==SHOT_MISSILE?100:15;blast(ex,ey,b.blast,damage*portion/100,j);}
    if(b.kind==SHOT_DART&&!destroy){b.pierce=254;b.life=(uint16_t)clamp(60*b.size_percent/100,1,60000);b.damage=(int)minv(2000000,(int)((int64_t)damage*b.size_percent/100));b.speed=0;b.traveled=0;b.vx=delta(b.x,e.x,worldWidth())*FP;b.vy=delta(b.y,e.y,worldHeight())*FP;b.blast=0;collision=true;continue;}
    if(b.kind==SHOT_RAIL&&destroy){
     blast(ex,ey,13*stats.blast/100,damage/3,j);
     for(unsigned sh=0;sh<3;sh++)emitBullet(ex,ey,(uint8_t)(b.angle+(int)sh*18-18),4,damage/5,SHOT_SHRAPNEL,18,0,1);
    }
    if(b.kind==SHOT_GRENADE){
     int bd=damage;b.active=0;
     for(unsigned sh=0;sh<3;sh++){int n=emitBullet(ex,ey,(uint8_t)(sh*256/3),2,maxv(1,bd/3),SHOT_SHRAPNEL,10);if(n>=0)bullets[n].blast=10;}
     collision=true;
    } else if(b.pierce&&destroy){if(b.kind!=SHOT_DART)--b.pierce;}
    else if(b.bounces){--b.bounces;int next=-1,dist=1000000;for(unsigned q=0;q<MAX_ENEMIES+MAX_BOSS_PARTS;q++)if(targetActive(q)&&(int)q!=j){int d=distance2(b.x,b.y,targetX(q),targetY(q));if(d<dist){dist=d;next=q;}}
     if(next>=0){b.angle=direction(delta(targetX(next),b.x,worldWidth()),delta(targetY(next),b.y,worldHeight()));b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;b.life=(uint16_t)minv(200,b.life+15);collision=true;}
     else {b.active=0;collision=true;}}
    else {b.active=0;collision=true;}
   }
  }
 }
}
#include "constructs.inc"
#include "electrical_shields.inc"
#include "scorching_wake.inc"
#include "myrmidon_projectiles.inc"
#include "persistent_effects.inc"
#include "vortex.inc"
#include "mod_source_rules.inc"
void Game::updatePickups() {
 for(unsigned i=0;i<MAX_PICKUPS;i++){
  Pickup &p=pickups[i];if(!p.active)continue;++p.age;
  int dx=delta(player.x,p.x,worldWidth()),dy=delta(player.y,p.y,worldHeight()),distance=approx(dx,dy);
  int magnet=player.body==B_RESEARCH?75:38;if(player.mods[M_X_LAST_STAND])magnet=magnet*3/2;if(daily(DAILY_SWORD))magnet*=3;
  if(distance<magnet||p.age>900){uint8_t a=direction(dx,dy);p.vx=p.vx*3/4+dirX(a);p.vy=p.vy*3/4+dirY(a);}
  else {p.vx=p.vx*245/256;p.vy=p.vy*245/256;}
  p.x=wrap(p.x+p.vx,worldWidth());p.y=wrap(p.y+p.vy,worldHeight());
  if(distance2(p.x,p.y,player.x,player.y)<(player.radius+5)*(player.radius+5)){
   p.active=0;
   if(p.kind==PICKUP_XP){unsigned amount=p.value*(last_stand_used_?2:1)*(100+experienceBonus())/100;xp=addSaturated(xp,amount);run_stats.experience_collected=addSaturated(run_stats.experience_collected,amount);}
   else if(p.kind==PICKUP_HEAL){int old=player.hull;player.hull=minv(hullRecoveryCap(),player.hull+p.value);emitFeedback(FeedbackHealing,frame,player.x,player.y,maxv(0,player.hull-old));}
   else if(p.kind==PICKUP_REROLL&&!player.mods[M_X_POWER_SPIKE])rerolls=(uint16_t)minv(99,rerolls+p.value);
   else if(p.kind==PICKUP_POWER){
    // value1/2/3 identifies original Hypercharge/ArmorFragment/MediCharge.
    // value0 remains a compatible Hypercharge for older synthetic fixtures.
    if(p.value==2)source_armor_ticks_=dailyMod(DAILY_GRACE)?188:150;
    else if(p.value==3)source_medi_ticks_=(uint16_t)minv(65535,(unsigned)source_medi_ticks_+30);
    else hyperboost_ticks_=dailyMod(DAILY_GRACE)?563:450; // original alarm duration900 source frames
   }
  }
 }
}
uint32_t Game::checksum() const {
 // Hash semantic simulation fields, avoiding compiler padding and the input
 // accumulator, which legitimately differs between rendering schedules.
 uint32_t hash=2166136261u;
#define HASH(v) do {hash^=(uint32_t)(v);hash*=16777619u;} while(0)
 HASH(daily_day);HASH(daily_variant);HASH(daily_modifier);HASH(daily_egg_hull);HASH(daily_boss_step_);
 for(unsigned i=0;i<MAX_COSMIC_OBJECTS;i++){const CosmicObject &c=cosmic_objects[i];HASH(c.active);if(c.active){HASH(c.x);HASH(c.y);HASH(c.hull);HASH(c.life);HASH(c.warning);}}
 HASH(state);HASH(run_flags);HASH(victory);HASH(banked_upgrades);HASH(frame);HASH(random_);HASH(score);HASH(kills);HASH(xp);HASH(level);HASH(wave);
 HASH(player.x);HASH(player.y);HASH(player.vx);HASH(player.vy);HASH(player.hull);HASH(player.shield);HASH(player.angle);
 HASH(player.weapon);HASH(player.body);HASH(player.shield_type);HASH(player.charge);HASH(player.missiles);
 for(unsigned i=0;i<MOD_COUNT;i++){HASH(player.mods[i]);HASH(banished_[i]);}
 HASH(run_stats.damage_dealt);HASH(run_stats.damage_taken);HASH(run_stats.experience_collected);HASH(fire_fraction_);HASH(regen_fraction_);
 HASH(options.auto_deploy_mask);HASH(options.assist_mode);HASH(enemy_serial_);HASH(wave_queue_count_);
 for(unsigned i=0;i<wave_queue_count_;i++){const WaveSpawnQueue &q=wave_queue_[i];HASH(q.delay);HASH(q.root_index);HASH(q.follower_index);HASH(q.repeat_index);HASH(q.leader);HASH(q.leader_generation);HASH(q.done);HASH(q.spec.enemy);HASH(q.spec.count);HASH(q.spec.follower);HASH(q.spec.follower_count);}
 for(unsigned i=0;i<MAX_ENEMIES;i++){const Enemy &e=enemies[i];HASH(e.active);if(e.active){HASH(e.x);HASH(e.y);HASH(e.hull);HASH(e.type);HASH(e.angle);HASH(e.age);HASH(e.phase);HASH(e.phase_timer);HASH(e.generation);HASH(e.leader);HASH(e.leader_generation);HASH(e.formation_angle);HASH(e.hard_shield);HASH(e.hard_shield_timer);HASH(e.heal_fraction);HASH(e.aura_fraction);HASH(e.burn_time);HASH(e.burn_damage);HASH(e.wake_time);HASH(e.challenger);HASH(e.damage_fraction);HASH(e.size_scale);}}
 for(unsigned i=0;i<MAX_BOSS_PARTS;i++){const BossPart &p=boss_parts[i];HASH(p.active);if(p.active){HASH(p.kind);HASH(p.owner);HASH(p.slot);HASH(p.x);HASH(p.y);HASH(p.hull);HASH(p.angle);HASH(p.burn_time);HASH(p.burn_damage);HASH(p.wake_time);}}
 for(unsigned i=0;i<MAX_BULLETS;i++){const Bullet &b=bullets[i];HASH(b.active);if(b.active){HASH(b.x);HASH(b.y);HASH(b.damage);HASH(b.life);HASH(b.kind);HASH(b.pierce);HASH(b.last_hit);HASH(b.traveled);HASH(b.construct_source);HASH(b.field_scale);HASH(b.tracking);HASH(b.tracking_weapon);HASH(b.tracking_seek);HASH(b.tracking_target);HASH(b.tracking_scale_x);HASH(b.tracking_scale_y);}}
 for(unsigned i=0;i<MAX_PICKUPS;i++){const Pickup &p=pickups[i];HASH(p.active);if(p.active){HASH(p.x);HASH(p.y);HASH(p.value);}}
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++){const Construct &c=constructs[i];HASH(c.active);if(c.active){HASH(c.x);HASH(c.y);HASH(c.kind);HASH(c.hull);HASH(c.weapon_charge);HASH(c.chain_percent);HASH(c.redirected_fraction);HASH(c.vortex_scale);HASH(c.vortex_rate);HASH(c.vortex_fraction);HASH(c.vortex_cost_fraction);const AdvancedConstructState &a=advanced_constructs[i];HASH(a.anchor_x);HASH(a.anchor_y);HASH(a.damage);HASH(a.duration);HASH(a.clock);HASH(a.state);HASH(a.side);HASH(a.heading);HASH(a.temporary);HASH(a.wielder);if(c.kind==C_INTERCEPTOR){const InheritedWeaponState &w=a.weapon;HASH(w.thermal_x);HASH(w.thermal_y);HASH(w.fire_fraction);HASH(w.stock_clock);HASH(w.magazine);HASH(w.charge_ticks);HASH(w.calibration);HASH(w.innate_fraction);HASH(w.charge_clock);HASH(w.release_delay);HASH(w.burst_left);HASH(w.burst_delay);HASH(w.weapon);HASH(w.charging);HASH(w.damage_fraction);HASH(w.reserved);}else for(unsigned h=0;h<4;h++){HASH(a.hit_mask[h]);HASH(a.hit_mask[h]>>32);}}}
 HASH(source_armor_ticks_);HASH(source_medi_ticks_);HASH(source_medi_fraction_);HASH(challenger_pending_);HASH(challenger_spawned_);HASH(challenger_delay_);HASH(challenger_source_);HASH(thermal_x_scale_);HASH(thermal_y_scale_);HASH(vortex_scale_);HASH(vortex_rate_);HASH(vortex_growth_fraction_);HASH(vortex_damage_fraction_);HASH(vortex_cost_fraction_);HASH(rancor_scale_);
 HASH(charged_scale_);HASH(innate_charge_fraction_);HASH(innate_cost_fraction_);HASH(discharge_clock_);HASH(discharge_self_fraction_);HASH(discharge_gemini_fraction_);
 HASH(maelstrom_state.charge_q16);HASH(maelstrom_state.charge_remainder);HASH(maelstrom_state.radius_q8);HASH(maelstrom_state.angle_q8);
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++){const PersistentField &f=persistent_fields[i];HASH(f.active);if(f.active){HASH(f.x);HASH(f.y);HASH(f.damage);HASH(f.radius);HASH(f.source_age);HASH(f.opacity);HASH(f.pushed_mask);HASH(f.pushed_mask>>32);HASH(f.burn);HASH(f.angle);HASH(f.reserved[0]);for(unsigned j=0;j<4;j++){HASH(f.hit_mask[j]);HASH(f.hit_mask[j]>>32);}for(unsigned j=0;j<MAX_ENEMIES;j++)HASH(f.owner_generation[j]);}}
#undef HASH
 return hash;
}
} // namespace nova
