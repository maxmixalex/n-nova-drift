#include "game.h"
namespace nova {
namespace {
const int16_t sine_quarter[65]={0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,
 98,104,109,115,121,126,132,137,142,147,152,157,162,167,172,177,
 181,185,190,194,198,202,206,209,213,216,220,223,226,229,231,234,
 237,239,241,243,245,247,248,250,251,252,253,254,255,255,256,256,256};
int absn(int x) {return x<0?-x:x;}
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
int Game::worldWidth() const {return WIDTH+64*player.mods[M_X_FARSIGHT];}
int Game::worldHeight() const {return HEIGHT+48*player.mods[M_X_FARSIGHT];}
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
uint32_t Game::random() {uint32_t x=random_;x^=x<<13;x^=x>>17;x^=x<<5;random_=x;return x;}
unsigned Game::range(unsigned m) {return m?random()%m:0;}
void Game::resetInput() {clearBytes(&previous_,sizeof(previous_));accumulator_=0;}
void Game::start(uint32_t run_seed) {
 clearBytes(this,sizeof(*this));
 seed=run_seed?run_seed:0x4e4f5641u;random_=seed;state=Playing;
 player.x=worldWidth()*FP/2;player.y=worldHeight()*FP/2;player.angle=192;
 sanctuary_x=player.x;sanctuary_y=player.y;
 player.weapon=W_BLASTER;player.body=B_STANDARD;player.shield_type=S_STANDARD;
 level=1;xp_next=8;rerolls=4;next_wave=1;boss_index=-1;auto_fire=false;
 recalculate();player.hull=player.max_hull;player.shield=player.max_shield;
 player.invulnerable=60;player.missiles=4;
 for(unsigned i=0;i<MAX_SEGMENTS;i++){segment_x[i]=player.x;segment_y[i]=player.y;}
}
bool Game::validState() const {
 if(state<Title||state>GameOver||player.weapon>=WEAPON_COUNT||player.body>=BODY_COUNT||
    player.shield_type>=SHIELD_COUNT||selected_offer>=OFFER_COUNT||level<1||level>999||
    wave>30000||xp_next==0||xp_next>1000000||player.max_hull<=0||
    player.max_hull>10000||player.max_shield<0||player.max_shield>10000||
    player.hull<0||player.hull>player.max_hull||player.shield<0||
    player.shield>player.max_shield||segment_count>MAX_SEGMENTS||!random_||
    accumulator_>=1000||banked_upgrades>999||boss_index<-1||boss_index>=MAX_ENEMIES)return false;
 if(player.x<0||player.x>=worldWidth()*FP||player.y<0||player.y>=worldHeight()*FP||
    absn(player.vx)>32*FP||absn(player.vy)>32*FP)return false;
 for(unsigned i=0;i<MOD_COUNT;i++)if(player.mods[i]>mods[i].max_rank)return false;
 for(unsigned i=0;i<OFFER_COUNT;i++) {
  const Offer &o=offers[i];if(o.kind>OFFER_SHIELD)return false;
  unsigned limit=o.kind==OFFER_MOD?(unsigned)MOD_COUNT:o.kind==OFFER_WEAPON?(unsigned)WEAPON_COUNT:o.kind==OFFER_BODY?(unsigned)BODY_COUNT:(unsigned)SHIELD_COUNT;
  if(o.id>=limit)return false;
 }
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active){const Enemy &e=enemies[i];if(e.type>=ENEMY_COUNT||e.x<0||e.x>=worldWidth()*FP||e.y<0||e.y>=worldHeight()*FP||e.radius>64||e.hull<0||e.hull>e.max_hull||e.max_hull<=0||absn(e.vx)>64*FP||absn(e.vy)>64*FP)return false;}
 for(unsigned i=0;i<MAX_BULLETS;i++)if(bullets[i].active){const Bullet &b=bullets[i];if(b.kind>SHOT_BLADE||b.owner>1||b.radius>100||b.x<0||b.x>=worldWidth()*FP||b.y<0||b.y>=worldHeight()*FP||absn(b.vx)>64*FP||absn(b.vy)>64*FP)return false;}
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active){const Construct &c=constructs[i];if(c.kind>C_BLADE||c.x<0||c.x>=worldWidth()*FP||c.y<0||c.y>=worldHeight()*FP||c.radius>64)return false;}
 return true;
}
unsigned Game::enemyCount() const {unsigned n=0;for(unsigned i=0;i<MAX_ENEMIES;i++)n+=enemies[i].active!=0;return n;}
unsigned Game::bulletCount() const {unsigned n=0;for(unsigned i=0;i<MAX_BULLETS;i++)n+=bullets[i].active!=0;return n;}
unsigned Game::constructCount() const {unsigned n=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)n+=constructs[i].active!=0;return n;}
bool Game::canTakeMod(unsigned id) const {
 if(id>=MOD_COUNT||!mods[id].implemented||banished_[id]||player.mods[id]>=mods[id].max_rank)return false;
 if(player.mods[M_X_HIDDEN_POWER]&&(id<=M_CONVERGENCE||(id>=M_DRONES&&id<=M_OVERCLOCK)))return false;
 if(mods[id].category==2&&!wild_mode)return false;
 if(id==M_X_THE_VOID&&!player.mods[M_X_EVOLUTIONARY_NICHE])return false;
 const ModDef &m=mods[id];
 if(m.prerequisite_a>=0&&!player.mods[m.prerequisite_a])return false;
 if(m.prerequisite_b>=0&&!player.mods[m.prerequisite_b])return false;
 if((id==M_DEFENSE_DRONES&&player.mods[M_ASSAULT_DRONES])||
    (id==M_ASSAULT_DRONES&&player.mods[M_DEFENSE_DRONES]))return false;
 if((id==M_GUARDIAN&&player.mods[M_INTERCEPTOR])||(id==M_INTERCEPTOR&&player.mods[M_GUARDIAN]))return false;
 if(player.mods[M_RAPID_RECONSTRUCTION]&&id>=M_SHIELD_DURABILITY&&id<=M_EMERGENCY_SYSTEMS)return false;
 if(id==M_RAPID_RECONSTRUCTION&&player.mods[M_OMNI_SHIELD])return false;
 return true;
}
bool Game::takeMod(unsigned id) {
 if(!canTakeMod(id))return false;
 ++player.mods[id];int oldh=player.max_hull,olds=player.max_shield;
 if(mods[id].max_rank>1&&id!=M_X_THE_VOID){
  unsigned bonus=revelation_+3*(player.mods[M_X_EXPLOSIVE_GROWTH]&&!banished_[M_X_EXPLOSIVE_GROWTH])+player.mods[M_X_OBSESSION];
  if(bonus){player.mods[id]=(uint8_t)minv(mods[id].max_rank,player.mods[id]+bonus);extra_ranks_=(uint8_t)minv(40,extra_ranks_+player.mods[M_X_OBSESSION]);revelation_=0;
   if(player.mods[M_X_EXPLOSIVE_GROWTH]&&!banished_[M_X_EXPLOSIVE_GROWTH]){banished_[id]=1;banished_[M_X_EXPLOSIVE_GROWTH]=1;}}
 }
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
 if(id==M_X_CHALLENGER)spawnEnemy(E_SPITFIRE,pixel(player.x)+worldWidth()/2,pixel(player.y),2);
 if(id==M_X_HIDDEN_POWER){for(unsigned i=0;i<MAX_CONSTRUCTS;i++)constructs[i].active=0;burst_left_=burst_delay_=0;}
 recalculate();player.hull=minv(player.max_hull,player.hull+maxv(0,player.max_hull-oldh));
 player.shield=minv(player.max_shield,player.shield+maxv(0,player.max_shield-olds));
 return true;
}
bool Game::equipWeapon(unsigned id) {if(id>=WEAPON_COUNT)return false;player.weapon=(uint8_t)id;player.charge=0;player.fire_delay=0;player.missiles=4;recalculate();return true;}
bool Game::equipBody(unsigned id) {
 if(id>=BODY_COUNT)return false;
 int old=player.max_hull;player.body=(uint8_t)id;
 recalculate();player.hull=clamp(player.hull+player.max_hull-old,1,player.max_hull);
 player.shield=minv(player.shield,player.max_shield);
 // Reassemble under the new body's construct limits.
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)constructs[i].active=0;
 for(unsigned i=0;i<MAX_SEGMENTS;i++){segment_x[i]=player.x;segment_y[i]=player.y;}
 return true;
}
bool Game::equipShield(unsigned id) {if(id>=SHIELD_COUNT||player.mods[M_RAPID_RECONSTRUCTION])return false;player.shield_type=(uint8_t)id;recalculate();player.shield=player.max_shield;player.shield_delay=0;return true;}
void Game::recalculate() {
 const uint8_t *m=player.mods;const BodyDef &b=bodies[player.body];
 clearBytes(&stats,sizeof(stats));
 stats.damage=100+15*m[M_MAGNITUDE]+30*m[M_PAYLOAD]+5*m[M_VELOCITY]+10*m[M_CONVERGENCE];
 stats.fire_rate=100+20*m[M_RAPID_FIRE]-15*m[M_PAYLOAD];
 stats.velocity=100+20*m[M_VELOCITY]+15*m[M_SNIPE]+15*m[M_GUIDANCE]-10*m[M_HOMING_STRIKE];
 stats.blast=100+25*m[M_BLAST_RADIUS]+45*m[M_HIGH_EXPLOSIVE]-25*m[M_CONCENTRATED_BLAST]+15*m[M_MAGNITUDE];
 stats.size=100+15*m[M_MAGNITUDE]+20*m[M_PAYLOAD];
 stats.thrust=b.thrust*(100+25*m[M_THRUSTERS]+15*m[M_STREAMLINE]+10*m[M_AGILITY]+10*m[M_POWER_RESERVES]-15*m[M_JUGGERNAUT])/100;
 stats.turn=3+2*m[M_AGILITY]+m[M_STABILIZATION];
 stats.plating=3*m[M_ABSORPTION];
 stats.homing=m[M_TARGETING]+2*m[M_GUIDANCE]+3*m[M_HOMING_STRIKE]+m[M_CONVERGENCE];
 stats.projectiles=2*m[M_VOLLEY];
 stats.shield_radius=shields[player.shield_type].radius*(100+30*m[M_SHIELD_RADIUS]-15*m[M_FOCUSED_SHIELDS]+30*m[M_RADIANT_SHIELDS])/100;
 stats.shield_effect=100+35*m[M_FOCUSED_SHIELDS]+60*m[M_WEAPONIZED_SHIELDS];
 stats.shield_cooldown=shields[player.shield_type].cooldown*(100-20*m[M_SHIELD_COOLDOWN])/100;
 stats.regen=m[M_REGENERATION]+5*m[M_RAPID_RECONSTRUCTION];
 stats.construct_damage=100+30*m[M_ADVANCED_ENGINEERING]+25*m[M_ELEGANT_CONSTRUCTION]+20*m[M_OVERCLOCK];
 stats.assembly=100+30*m[M_ADVANCED_ENGINEERING]+35*m[M_OVERSEER];
 stats.crash_damage=100+35*m[M_JUGGERNAUT]+35*m[M_SKIRMISH];
 int hullpercent=100+20*m[M_HULL_STRENGTH]+35*m[M_JUGGERNAUT]+50*m[M_RAPID_RECONSTRUCTION]-40*m[M_OMNI_SHIELD]+15*m[M_PURIFICATION]+10*m[M_CORE_SHIELDING];
 int shieldpercent=100+20*m[M_SHIELD_DURABILITY]+60*m[M_OMNI_SHIELD]+15*m[M_RADIANT_SHIELDS];
 stats.damage+=5*m[M_X_MASTERY]+20*m[M_X_HEAVY_CALIBER]+20*m[M_X_PROPULSIVE_MUNITIONS]+15*m[M_X_RAMPAGE];
 stats.velocity+=5*m[M_X_MASTERY]-10*m[M_X_HEAVY_CALIBER]+15*m[M_X_ANNIHILATION];
 stats.size+=5*m[M_X_MASTERY]+20*m[M_X_HEAVY_CALIBER]+15*m[M_X_ANNIHILATION];
 stats.blast+=5*m[M_X_MASTERY]+25*m[M_X_POLAR_INVERSION];
 stats.fire_rate+=5*m[M_X_MASTERY]+8*m[M_X_DOUBLE_TAP]-5*m[M_X_ENERGIZED_SHIELDS];
 stats.projectiles+=m[M_X_TWIN_STRIKE];
 stats.plating+=2*m[M_X_DEFIANCE];stats.regen+=2*m[M_X_HYPERMETABOLISM];
 stats.thrust=stats.thrust*(100+12*m[M_X_SLIPSTREAM]+10*m[M_X_EVOLUTIONARY_NICHE]+20*m[M_X_BRAVADO]+30*m[M_X_HIDDEN_POWER])/100;
 stats.turn+=m[M_X_SLIPSTREAM];
 stats.shield_effect+=10*m[M_X_EVOLUTIONARY_NICHE]+20*m[M_X_BRAVADO];
 stats.shield_cooldown=stats.shield_cooldown*maxv(20,100-30*m[M_X_FLASH_SHIELDING]-5*m[M_X_ENERGIZED_SHIELDS])/100;
 stats.construct_damage+=40*m[M_X_SUPPORT_SPECIALIST];
 stats.assembly-=5*m[M_X_ENERGIZED_SHIELDS];
 hullpercent+=15*m[M_X_RAMPAGE]+10*m[M_X_EVOLUTIONARY_NICHE]+20*m[M_X_BRAVADO]+30*m[M_X_HIDDEN_POWER]-8*m[M_X_HYPERMETABOLISM]-15*m[M_X_SUPPORT_SPECIALIST]-3*extra_ranks_;
 shieldpercent+=15*m[M_X_RAMPAGE]+30*m[M_X_HIDDEN_POWER]-20*m[M_X_FLASH_SHIELDING]-8*m[M_X_HYPERMETABOLISM]-15*m[M_X_SUPPORT_SPECIALIST]-3*extra_ranks_;
 if(m[M_X_ATARAXIA]){int points=minv(12,banked_upgrades);stats.damage+=points*8;hullpercent+=points*4;shieldpercent+=points*4;stats.thrust+=points*4;}
 if(m[M_X_APEX_MACHINERY]){stats.construct_damage*=2;stats.assembly/=2;}
 if(m[M_X_BURNOUT_REACTORS])stats.assembly*=2;
 if(m[M_X_ROCKET_DRONES])stats.assembly+=30;
 if(m[M_X_SPECIALIST_MINE]||m[M_X_SPECIALIST_TURRET]||m[M_X_SPECIALIST_DRONE]||m[M_X_SPECIALIST_ALLY])stats.assembly=stats.assembly*70/100;
 hullpercent=maxv(15,hullpercent);shieldpercent=maxv(10,shieldpercent);
 if(m[M_VOLLEY]){stats.damage=stats.damage*85/100;stats.velocity=stats.velocity*85/100;}
 if(m[M_FUSILLADE]){stats.damage=stats.damage*75/100;stats.velocity=stats.velocity*75/100;}
 if(player.body==B_ASSAULT){stats.fire_rate+=20;stats.velocity+=20;}
 if(player.body==B_HULLBREAKER){stats.fire_rate-=20;stats.crash_damage+=90;stats.regen++;}
 if(player.body==B_SENTINEL){stats.fire_rate-=15;stats.shield_effect+=45;stats.shield_radius=stats.shield_radius*125/100;}
 if(player.body==B_ENGINEER)stats.assembly+=50;
 if(player.body==B_ARCHITECT){stats.construct_damage+=160;stats.assembly+=60;}
 if(player.body==B_BATTERY){stats.projectiles+=2;stats.damage+=25;stats.fire_rate-=20;}
 if(player.body==B_COURSER){stats.projectiles+=2;stats.damage=stats.damage*75/100;stats.fire_rate+=15;}
 if(m[M_OVERSEER])stats.thrust=stats.thrust*85/100;
 segment_count=player.body==B_LEVIATHAN?(uint8_t)minv(MAX_SEGMENTS,4+level):0;
 player.max_hull=(b.hull+segment_count*4)*hullpercent/100+research_bonus_;
 if(segment_count)stats.damage+=segment_count*2;
 player.max_shield=m[M_RAPID_RECONSTRUCTION]?0:shields[player.shield_type].capacity*shieldpercent/100*b.shield_percent/100+research_bonus_;
 if(m[M_X_CONVERSION]){if(player.max_hull<player.max_shield){int half=player.max_hull/2;player.max_hull-=half;player.max_shield+=half;}else{int half=player.max_shield/2;player.max_shield-=half;player.max_hull+=half;}}
 player.radius=(uint8_t)clamp(b.radius+m[M_JUGGERNAUT]+m[M_X_GRANDEUR]*2-m[M_X_WINNOW]-m[M_X_REGRESSION],3,15);
 player.hull=minv(player.hull,player.max_hull);player.shield=minv(player.shield,player.max_shield);
}
void Game::giveExperience(uint32_t amount) {xp+=amount;checkLevel();}
void Game::bonusLevels(unsigned count) {level=(uint16_t)minv(999,level+count);banked_upgrades=(uint16_t)minv(999,banked_upgrades+count);xp_next=8+level*5+(level*level)/4;}
void Game::checkLevel() {if(state==Playing&&xp>=xp_next&&level<999) {xp-=xp_next;bonusLevels(1);recalculate();beginUpgrade();}}
bool Game::bankUpgrade() {if(state!=Upgrade||!banked_upgrades)return false;state=Playing;resetInput();recalculate();return true;}
bool Game::reopenUpgrade() {if(state!=Playing||!banked_upgrades)return false;state=Upgrade;resetInput();return true;}
void Game::beginUpgrade() {state=Upgrade;selected_offer=0;generateOffers();}
void Game::generateOffers() {
 int gear=-1;if(level==2)gear=OFFER_WEAPON;else if(level==3)gear=OFFER_SHIELD;else if(level==4)gear=OFFER_BODY;
 for(unsigned i=0;i<OFFER_COUNT;i++) {
  bool done=false;
  for(unsigned attempt=0;attempt<160&&!done;attempt++) {
   Offer o;int kind=gear;
   if(kind<0)kind=(level%7==0&&i==2)?(int)range(3)+1:OFFER_MOD;
   if(kind==OFFER_SHIELD&&player.mods[M_RAPID_RECONSTRUCTION])kind=OFFER_MOD;
   if(kind==OFFER_WEAPON&&player.mods[M_X_HIDDEN_POWER])kind=OFFER_BODY;
   o.kind=(uint8_t)kind;
   if(kind==OFFER_MOD){o.id=(uint8_t)range(MOD_COUNT);if(!canTakeMod(o.id))continue;
    if(transmog_&&(mods[o.id].category!=2||mods[o.id].max_rank<=1))continue;
    if(wild_mode&&!transmog_&&player.mods[M_X_CHAOTIC_AMBITION]&&i==2&&mods[o.id].category!=2)continue;}
   else if(kind==OFFER_WEAPON){o.id=(uint8_t)range(WEAPON_COUNT);if(o.id==player.weapon)continue;}
   else if(kind==OFFER_BODY){o.id=(uint8_t)range(BODY_COUNT);if(o.id==player.body)continue;}
   else {o.id=(uint8_t)range(SHIELD_COUNT);if(o.id==player.shield_type)continue;}
   bool duplicate=false;for(unsigned j=0;j<i;j++)if(offers[j].kind==o.kind&&offers[j].id==o.id)duplicate=true;
   if(!duplicate){offers[i]=o;done=true;}
  }
  if(!done) {offers[i].kind=OFFER_WEAPON;offers[i].id=(uint8_t)((player.weapon+1+i)%WEAPON_COUNT);}
 }
}
bool Game::chooseOffer(unsigned i) {
 if(state!=Upgrade||i>=OFFER_COUNT)return false;
 const Offer o=offers[i];bool ok=false;
 if(o.kind==OFFER_MOD)ok=takeMod(o.id);else if(o.kind==OFFER_WEAPON)ok=equipWeapon(o.id);
 else if(o.kind==OFFER_BODY)ok=equipBody(o.id);else if(o.kind==OFFER_SHIELD)ok=equipShield(o.id);
 if(!ok)return false;
 if(banked_upgrades)--banked_upgrades;
 if(transmog_&&o.id!=M_X_TRANSMOGRIFICATION)--transmog_;
 state=Playing;player.invulnerable=maxv(player.invulnerable,30);resetInput();recalculate();
 if(banked_upgrades)beginUpgrade();else checkLevel();return true;
}
bool Game::rerollOffers() {
 if(state!=Upgrade||rerolls==0||transmog_||player.mods[M_X_POWER_SPIKE])return false;
 --rerolls;generateOffers();
 // Research's original banishment UI is not reproduced: this explicit port
// alternative rewards spent rerolls without claiming banishment support.
 if(player.body==B_RESEARCH){research_bonus_+=2;recalculate();player.hull=minv(player.hull+2,player.max_hull);}
 return true;
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
  if((in.left&&!previous_.left)||(in.up&&!previous_.up))selected_offer=(selected_offer+OFFER_COUNT-1)%OFFER_COUNT;
  if((in.right&&!previous_.right)||(in.down&&!previous_.down))selected_offer=(selected_offer+1)%OFFER_COUNT;
  if(ability)rerollOffers();
  if(confirm)chooseOffer(selected_offer);
  previous_=in;return;
 }
 if(pause||back){state=Pause;previous_=in;return;}
 ++frame;if(wave_banner)--wave_banner;if(damage_flash)--damage_flash;
 untouched_=(uint16_t)minv(2400,untouched_+1);
 if(maelstrom_&&frame%15==0)--maelstrom_;
 if(kill_buff_)--kill_buff_;
 for(unsigned i=0;i<MAX_EFFECTS;i++)if(effects[i].active&&--effects[i].life==0)effects[i].active=0;
 updatePlayer(in);if(state==GameOver){previous_=in;return;}
 updateWaves();updateEnemies();buildGrid();updateShields();updateConstructs();updateBullets();updatePickups();
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
 if(slot==end){++dropped_bullets;if(!owner)return -1;
  slot=begin;for(unsigned i=begin+1;i<end;i++)if(bullets[i].age>bullets[slot].age)slot=i;
 }
 Bullet &b=bullets[slot];clearBytes(&b,sizeof(b));b.active=1;b.owner=(uint8_t)owner;
 b.x=wrap(x,worldWidth());b.y=wrap(y,worldHeight());b.angle=angle;b.speed=(uint16_t)clamp(speed*FP,1,16*FP);b.vx=dirX(angle)*b.speed/FP;b.vy=dirY(angle)*b.speed/FP;
 b.damage=(int16_t)clamp(damage,1,2000);b.kind=(uint8_t)kind;b.life=(uint16_t)life;
 b.radius=(uint8_t)clamp(radius,1,32);b.last_hit=-1;
 if(!owner) {b.homing=(uint8_t)stats.homing;
  b.bounces=player.mods[M_X_RICOCHET];
  b.wraps=player.mods[M_X_WARP_STRIKE];
  if(player.mods[M_BLAST_RADIUS])b.blast=8;
  if(kind==SHOT_SPLIT)b.blast=11;
  if(kind==SHOT_GRENADE)b.blast=25;
  if(kind==SHOT_MISSILE){b.blast=12;b.homing=(uint8_t)maxv(3,b.homing);}
  if(kind==SHOT_RAIL)b.pierce=5;
  if(kind==SHOT_DART)b.pierce=2;
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
int Game::nearestEnemy(int32_t x,int32_t y,int radius) const {
 int best=-1,dist=radius*radius;
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active){int d=distance2(x,y,enemies[i].x,enemies[i].y);if(d<dist){dist=d;best=(int)i;}}
 return best;
}
void Game::updatePlayer(const GameInput &in) {
 Player &p=player;const uint8_t *m=p.mods;
 if(p.invulnerable)--p.invulnerable;
 if(p.ability_delay)--p.ability_delay;
 if(p.fire_delay)--p.fire_delay;
 int turn=stats.turn;
 // Cursor aiming follows the visible straight line; toroidal shortest-path
 // targeting is reserved for autonomous actors and collision calculations.
 if(in.pointer_valid){p.angle=toward(p.angle,direction(in.pointer_x*FP-p.x,in.pointer_y*FP-p.y),turn*2);}
 else {if(in.left)p.angle=(uint8_t)(p.angle-turn);if(in.right)p.angle=(uint8_t)(p.angle+turn);}
 p.thrusting=in.up||m[M_X_RAMPAGE];p.firing=in.fire||in.pointer_down||auto_fire;
 if(m[M_OVERSEER]||m[M_X_HIDDEN_POWER])p.firing=false;
 // Holding down/brake suppresses auto-fire, allowing charge/stockpile/cloak.
 if(auto_fire&&in.down&&!in.fire&&!in.pointer_down)p.firing=false;
 int thrust=stats.thrust;if(p.shield>0&&p.shield_type==S_AMP)thrust=thrust*125/100;
 if(kill_buff_&&m[M_WARPATH])thrust+=20;
 if(p.firing&&(p.weapon==W_TORRENT||p.weapon==W_THERMAL_LANCE||p.weapon==W_VORTEX))thrust=thrust*70/100;
 if(p.thrusting){p.vx+=dirX(p.angle)*thrust/1400;p.vy+=dirY(p.angle)*thrust/1400;}
 int drag=in.down?(m[M_STABILIZATION]?205:225):(m[M_STREAMLINE]||m[M_X_SLIPSTREAM]||m[M_X_REGRESSION]?255:253);
 p.vx=p.vx*drag/256;p.vy=p.vy*drag/256;
 int speed=approx(p.vx,p.vy),maximum=(3*FP)*stats.thrust/100;
 if(dash_ticks_){--dash_ticks_;maximum+=3*FP;}
 if(speed>maximum){p.vx=p.vx*maximum/speed;p.vy=p.vy*maximum/speed;}
 int32_t oldx=p.x,oldy=p.y;
 p.x=wrap(p.x+p.vx,worldWidth());p.y=wrap(p.y+p.vy,worldHeight());
 if(m[M_X_WARP_STRIKE]&&(absn(p.x-oldx)>worldWidth()*FP/2||absn(p.y-oldy)>worldHeight()*FP/2)){p.shield=p.shield*95/100;damagePlayer(maxv(1,p.hull/20),true);}
 sanctuary_x=wrap(sanctuary_x+delta(p.x,sanctuary_x,worldWidth())*FP/40,worldWidth());
 sanctuary_y=wrap(sanctuary_y+delta(p.y,sanctuary_y,worldHeight())*FP/40,worldHeight());
 if(in.ability&&!previous_.ability&&!p.ability_delay) {
  if(p.body==B_ARCHITECT){for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active){blast(constructs[i].x,constructs[i].y,32,80*stats.construct_damage/100);constructs[i].active=0;}}
  uint8_t dash_angle=p.angle;
  if(m[M_X_STRAFE])dash_angle=(uint8_t)(p.angle+(in.left?-64:in.right?64:0));
  if(m[M_BLINK]) {int32_t ox=p.x,oy=p.y;p.x=wrap(p.x+dirX(dash_angle)*65,worldWidth());p.y=wrap(p.y+dirY(dash_angle)*65,worldHeight());blast(ox,oy,22,45);blast(p.x,p.y,22,45);emitEffect(ox,oy,FX_WARP,25,12,3);
   if(m[M_X_DISPLACEMENT]){emitEffect(ox,oy,FX_BEAM,5,12,3,ox+dirX(dash_angle)*65,oy+dirY(dash_angle)*65);for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active&&sweptHit(ox,oy,p.x,p.y,enemies[e].x,enemies[e].y,enemies[e].radius+5))hitEnemy(e,100*stats.damage/100);}}
  else {p.vx+=dirX(dash_angle)*3;p.vy+=dirY(dash_angle)*3;}
  dash_ticks_=12;
  p.invulnerable=maxv(p.invulnerable,12);p.ability_delay=m[M_POWER_RESERVES]?75:120;
 }
 if(!p.firing){calibration_=minv(180,calibration_+1);if(p.body==B_SPECTRE&&speed<2*FP)p.cloak=minv(180,p.cloak+2);}
 else if(p.weapon!=W_VORTEX&&p.weapon!=W_THERMAL_LANCE)p.cloak=p.cloak>4?p.cloak-4:0;
 if(speed>=2*FP)p.cloak=p.cloak>3?p.cloak-3:0;
 if(p.weapon==W_SALVO&&!p.firing) {if(++p.missile_clock>=12+p.missiles/3){p.missile_clock=0;p.missiles=minv(48,p.missiles+1+stats.projectiles);}}
 if(p.weapon==W_VORTEX) {
  if(p.firing){p.charge=minv(150,p.charge+1);if(p.charge>25&&frame%15==0)damagePlayer(1+p.charge/60,true);}
  else if(p.charge)releaseVortex();
 } else if(p.weapon==W_THERMAL_LANCE) {
  if(p.firing)p.charge=minv(90,p.charge+2);else p.charge=0;
  if(p.firing&&!p.fire_delay)fireWeapon();
 } else {
  if(!p.firing)p.charge=minv(180,p.charge+1);
  if(p.firing&&!p.fire_delay)fireWeapon();
 }
 if(!m[M_X_HIDDEN_POWER]&&!m[M_OVERSEER]&&burst_left_&&burst_delay_&&!--burst_delay_){uint16_t save=p.fire_delay;fireWeapon();p.fire_delay=save;if(burst_left_)--burst_left_;if(burst_left_)burst_delay_=3;}
 if(m[M_SIEGE]&&p.firing&&frame%20==0&&p.charge>90)damagePlayer(2,true);
 if(m[M_SIEGE]&&p.firing)p.charge=minv(180,p.charge+1);
 if(m[M_X_SATURATION_FIRE]&&p.firing)calibration_=(uint16_t)minv(180,calibration_+3);
 if(m[M_X_STRAFING_STRIKES]&&dash_ticks_&&frame%3==0){for(unsigned side=0;side<2;side++)emitBullet(p.x,p.y,(uint8_t)(p.angle+(side?64:-64)),6,18*stats.damage/100,SHOT_BLASTER,40);}
 if(p.shield_delay) {unsigned decrement=1+(m[M_EFFICIENCY]&&!p.firing);p.shield_delay=p.shield_delay>decrement?p.shield_delay-decrement:0;}
 if(!p.shield_delay&&p.shield<p.max_shield&&(!m[M_X_TEMPEST_BREAK]||p.shield==0)){bool was_down=p.shield==0;p.shield=p.max_shield;
  if(was_down&&p.shield_type==S_SHOCKWAVE){blast(p.x,p.y,stats.shield_radius*2,65*stats.shield_effect/100);for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&distance2(p.x,p.y,enemies[i].x,enemies[i].y)<stats.shield_radius*stats.shield_radius*4)enemies[i].vulnerability=35;}
  emitEffect(p.x,p.y,FX_RING,stats.shield_radius,14,2);}
 int regen=stats.regen+(m[M_EFFICIENCY]&&!p.firing?1:0)+(m[M_AEGIS]&&speed<FP?1:0);
 if(regen&&++regen_clock_>=30){regen_clock_=0;p.hull=minv(p.max_hull,p.hull+regen);}
 if(m[M_REGENERATIVE_SHIELDS]&&p.shield>0&&frame%30==0)p.shield=minv(p.max_shield,p.shield+2);
 if(p.shield_type==S_BASTION&&p.shield>0&&frame%15==0)p.shield=minv(p.max_shield,p.shield+2);
 if(segment_count){
  int32_t prevx=p.x,prevy=p.y;
  for(unsigned i=0;i<segment_count;i++){int dx=delta(prevx,segment_x[i],worldWidth()),dy=delta(prevy,segment_y[i],worldHeight());int d=approx(dx,dy);
   if(d>7){segment_x[i]=wrap(segment_x[i]+dx*FP*(d-7)/maxv(1,d),worldWidth());segment_y[i]=wrap(segment_y[i]+dy*FP*(d-7)/maxv(1,d),worldHeight());}
   prevx=segment_x[i];prevy=segment_y[i];}
 }
 if(p.thrusting&&(p.body==B_FIREFLY||m[M_DEADLY_WAKE])&&frame%5==0){int32_t tx=wrap(p.x-dirX(p.angle)*15,worldWidth()),ty=wrap(p.y-dirY(p.angle)*15,worldHeight());
  emitEffect(tx,ty,FX_BURST,10,8,1);
  for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&distance2(tx,ty,enemies[i].x,enemies[i].y)<22*22)hitEnemy(i,p.body==B_FIREFLY?5:3,true);}
 if(p.thrusting&&m[M_X_SCORCHING_WAKE]&&frame%12==0)emitBullet(p.x,p.y,(uint8_t)(p.angle+128),2,5*m[M_X_SCORCHING_WAKE],SHOT_SPARK,36,0,3);
 if(m[M_X_CELESTIAL_SURGE]&&in.down&&!previous_.down&&speed>FP){int n=emitBullet(p.x,p.y,p.angle,4,25*stats.damage/100,SHOT_VORTEX,50,0,12);if(n>=0)bullets[n].homing=0;}
 if(frame%15==0&&(m[M_X_DYING_STAR]||(m[M_X_LANCE]&&speed>2*FP))){int radius=m[M_X_DYING_STAR]?80:25;
  emitEffect(p.x,p.y,FX_RING,radius,15,1);for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active&&distance2(p.x,p.y,enemies[e].x,enemies[e].y)<radius*radius)hitEnemy(e,m[M_X_DYING_STAR]?8:6,true);}
 if(frame%30==0){int self=3*m[M_X_DYING_STAR]+m[M_X_SOLAR_HEART]+m[M_X_APOTHEOSIS]+(m[M_X_LANCE]&&speed>2*FP?2:0);if(self)damagePlayer(self,true);}
 if(m[M_X_DISCORD]&&frame%8==0){uint8_t a=(uint8_t)(frame*5);int32_t ox=wrap(p.x+dirX(a)*34,worldWidth()),oy=wrap(p.y+dirY(a)*22,worldHeight());emitEffect(ox,oy,FX_BURST,4,10,3);int e=nearestEnemy(ox,oy,55);if(e>=0){emitEffect(ox,oy,FX_BEAM,1,5,3,ox+delta(enemies[e].x,ox,worldWidth())*FP,oy+delta(enemies[e].y,oy,worldHeight())*FP);hitEnemy(e,9);}}
}
void Game::fireWeapon() {
 Player &p=player;const uint8_t *m=p.mods;const WeaponDef &w=weapons[p.weapon];
 uint8_t fire_angle=(uint8_t)(p.angle+(m[M_X_PROPULSIVE_MUNITIONS]?128:0));
 int rate=stats.fire_rate;
 if(kill_buff_&&m[M_WARPATH])rate+=25;
 if(m[M_ADRENAL])rate+=60*(p.max_hull-p.hull)/maxv(1,p.max_hull);
 if(m[M_SIEGE])rate+=minv(90,p.charge);
 p.fire_delay=(uint16_t)clamp(w.cooldown*100/maxv(20,rate),2,100);
 int damage=w.damage*stats.damage/100;
 damage+=20*m[M_X_ANNIHILATION]+3*m[M_X_SOLAR_HEART];
 if(m[M_X_REBUKE]&&stored_damage_){damage+=stored_damage_;stored_damage_=0;}
 if(m[M_X_INFUSE]&&p.shield>0)damage+=p.shield*stats.shield_effect/400;
 if(m[M_X_SATURATION_FIRE])damage=damage*(100+calibration_/4)/100;
 if(p.body==B_ASSAULT&&range(5)==0)damage=damage*160/100;
 if(p.body==B_SPECTRE)damage=damage*(100+p.cloak)/100;
 if(p.shield>0&&p.shield_type==S_AMP)damage=damage*130/100;
 if(m[M_KINETIC_BOOST])damage=damage*(100+minv(80,approx(p.vx,p.vy)/12))/100;
 if(m[M_CHARGED_SHOT])damage=damage*(100+minv(120,p.charge))/100;
 int projectiles=w.projectiles+stats.projectiles;
 if(p.weapon==W_FLAK)projectiles=w.projectiles+stats.projectiles*2;
 if(m[M_FUSILLADE])projectiles+=w.projectiles;
 projectiles=clamp(projectiles,1,24);
 if(m[M_X_SINGULAR_STRIKE_NEW]){damage=damage*(125+25*projectiles)/100;projectiles=1;}
 if(m[M_X_BARRAGE]){p.fire_delay=(uint16_t)maxv(1,p.fire_delay/maxv(2,projectiles));projectiles=1;damage+=4;}
 int spread=w.spread;
 if(m[M_FOCUS_FIRE])spread/=2;
 if(m[M_CONVERGENCE])spread=spread*65/100;
 spread=spread*100/(100+15*m[M_X_TWIN_STRIKE]+5*m[M_X_MASTERY]);
 if(m[M_FIRING_ARRAY])spread=spread*135/100;
 if(p.body==B_ASSAULT)spread=spread*80/100;
 int speed=maxv(1,w.speed*stats.velocity/100);
 if(m[M_CALIBRATE]){speed+=calibration_/60;spread=spread*(100-calibration_/3)/100;}
 int radius=clamp(w.radius*stats.size/100,1,10);
 if(m[M_X_SINGULAR_STRIKE_NEW])radius=minv(16,radius*2);
 bool ordinary=true;
 if(p.weapon==W_PULSE){
  int outer=40*stats.blast/100,inner=20*stats.blast/100;
  blast(p.x,p.y,outer,damage);blast(p.x,p.y,inner,damage);
  emitEffect(p.x,p.y,FX_RING,outer,18,3);
 } else if(p.weapon==W_BLADE){
  unsigned count=0;for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&constructs[i].kind==C_BLADE)++count;
  if(count<(unsigned)minv(6,projectiles))spawnConstruct(C_BLADE);
  ordinary=false;
 } else if(p.weapon==W_THERMAL_LANCE){
  ordinary=false;
  if(p.charge<24){
   if(frame%6==0)emitBullet(p.x,p.y,(uint8_t)(fire_angle+(int)range(17)-8),4,maxv(1,damage/2),SHOT_SPARK,24,0,1);
  } else {
   int length=55+p.charge/2;
   int32_t ex=wrap(p.x+dirX(fire_angle)*length,worldWidth()),ey=wrap(p.y+dirY(fire_angle)*length,worldHeight());
   emitEffect(p.x,p.y,FX_BEAM,2+radius/2,4,1,p.x+dirX(fire_angle)*length,p.y+dirY(fire_angle)*length);
   for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&sweptHit(p.x,p.y,ex,ey,enemies[i].x,enemies[i].y,enemies[i].radius+3)){
    enemies[i].vulnerability=(uint8_t)minv(60,enemies[i].vulnerability+2);hitEnemy(i,damage,true);
   }
  }
 } else if(p.weapon==W_SWORDS){
  ordinary=false;int reach=25*stats.size/100;
  damage+=approx(p.vx,p.vy)/20;
  emitEffect(p.x,p.y,FX_SWORD,reach,10,2,fire_angle,0);
  for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&distance2(p.x,p.y,enemies[i].x,enemies[i].y)<(reach+enemies[i].radius)*(reach+enemies[i].radius)){
   uint8_t a=direction(delta(enemies[i].x,p.x,worldWidth()),delta(enemies[i].y,p.y,worldHeight()));if(absn(turnDelta(fire_angle,a))<70)hitEnemy(i,damage);
  }
  for(unsigned i=0;i<MAX_BULLETS;i++)if(bullets[i].active&&bullets[i].owner&&distance2(p.x,p.y,bullets[i].x,bullets[i].y)<reach*reach){
   bullets[i].active=0;emitBullet(p.x,p.y,(uint8_t)(fire_angle+(int)range(25)-12),6,damage/3,SHOT_SPARK,35,0,2);
  }
 }
 if(ordinary){
  unsigned kind=SHOT_BLASTER;
  switch(p.weapon){case W_SPLIT_SHOT:kind=SHOT_SPLIT;break;case W_FLAK:kind=SHOT_FLAK;break;case W_RAILGUN:kind=SHOT_RAIL;break;
   case W_GRENADE:kind=SHOT_GRENADE;break;case W_SALVO:kind=SHOT_MISSILE;break;case W_PULSE:kind=SHOT_SPARK;damage=maxv(1,damage/3);break;
   case W_TORRENT:kind=SHOT_TORRENT;break;case W_DART:kind=SHOT_DART;break;default:break;}
  if(p.weapon==W_SALVO){projectiles=minv(4,1+stats.projectiles);
   if(p.missiles){projectiles=minv(projectiles,p.missiles);p.missiles-=projectiles;}
   else p.fire_delay=maxv(p.fire_delay,14);}
  if(p.weapon==W_RAILGUN)damage=damage*stats.velocity/100;
  for(int i=0;i<projectiles;i++){
   int offset=(i*2-(projectiles-1))*spread/2;
   if(p.weapon==W_FLAK||p.weapon==W_SALVO)offset+=(int)range(9)-4;
   if(p.weapon==W_TORRENT)offset+=dirY((uint8_t)(frame*8))/30;
   uint8_t a=(uint8_t)(fire_angle+offset);
   int shotdamage=damage;
   if(m[M_FIRING_ARRAY])shotdamage=damage*(150-absn(i*2-(projectiles-1))*75/maxv(1,projectiles))/100;
   int32_t x=p.x+dirX(fire_angle)*(p.radius+3),y=p.y+dirY(fire_angle)*(p.radius+3);
   if(p.body==B_BATTERY||p.body==B_COURSER||p.weapon==W_BLASTER){int side=(i&1)?4:-4;x+=dirX((uint8_t)(fire_angle+64))*side;y+=dirY((uint8_t)(fire_angle+64))*side;}
   int slot=emitBullet(x,y,a,speed,shotdamage,kind,w.life,0,radius);
   if(slot>=0){bullets[slot].vx+=p.vx/3;bullets[slot].vy+=p.vy/3;}
  }
 }
 if((m[M_BURST_FIRE]||m[M_X_DOUBLE_TAP])&&!m[M_X_BARRAGE]&&!burst_left_&&p.weapon!=W_THERMAL_LANCE&&p.weapon!=W_SWORDS&&p.weapon!=W_BLADE){burst_left_=(uint8_t)(2*m[M_BURST_FIRE]+m[M_X_DOUBLE_TAP]);burst_delay_=3;p.fire_delay=p.fire_delay*(burst_left_+1)+6;}
 if(m[M_DISCHARGE]&&p.shield>3){blast(p.x,p.y,stats.shield_radius,6*stats.shield_effect/100);p.shield-=2;p.shield_delay=maxv(p.shield_delay,60);}
 emitEffect(p.x+dirX(fire_angle)*8,p.y+dirY(fire_angle)*8,FX_MUZZLE,4,3,0);
 if(!m[M_SIEGE]&&p.weapon!=W_THERMAL_LANCE)p.charge=0;
 calibration_=calibration_>20?calibration_-20:0;
 if(m[M_X_MORTAR])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_MINE&&constructs[c].age<60){constructs[c].vx=dirX(fire_angle)*7;constructs[c].vy=dirY(fire_angle)*7;constructs[c].life=60;}
 if(m[M_X_HEAVY_CALIBER]||m[M_X_PROPULSIVE_MUNITIONS]){int recoil=m[M_X_HEAVY_CALIBER]*20+100*m[M_X_PROPULSIVE_MUNITIONS];if(m[M_X_POLAR_INVERSION])recoil=-recoil;p.vx-=dirX(fire_angle)*recoil/FP;p.vy-=dirY(fire_angle)*recoil/FP;}
 if(m[M_X_ANNIHILATION])damagePlayer(2+projectiles/3,true);
}
void Game::releaseVortex() {
 Player &p=player;int charge=p.charge;p.charge=0;if(charge<8)return;
 int radius=7+charge/9,damage=(4+charge/15)*stats.damage/100;
 int slot=emitBullet(p.x,p.y,p.angle,1,damage,SHOT_VORTEX,70+charge/2,0,radius);
 if(slot>=0){bullets[slot].homing=0;bullets[slot].blast=0;bullets[slot].pierce=255;}
 p.fire_delay=15;
}
void Game::damagePlayer(int amount,bool self_damage) {
 Player &p=player;const uint8_t *m=p.mods;
 if(state!=Playing||amount<=0||p.hull<=0||(!self_damage&&p.invulnerable))return;
 if(m[M_X_RANCOR]&&p.charge)p.charge=(uint16_t)minv(180,p.charge+amount*3);
 if(self_damage){
  amount=maxv(1,amount*maxv(10,100-10*m[M_X_DEFIANCE]-15*m[M_X_MASOCHISM])/100);
  if(m[M_X_MEDI_CHARGE])for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&constructs[i].kind==C_GUARDIAN){constructs[i].hull-=amount;return;}
  if(p.body==B_FIREFLY)amount=maxv(1,amount/2);
  if(m[M_CORE_SHIELDING])amount=maxv(1,amount*60/100);
  p.hull=maxv(0,p.hull-amount);
 }
 else {
  amount=amount*(100+10*m[M_X_MASOCHISM]+15*m[M_X_FARSIGHT])/100;
  if(m[M_X_SHIELDED_DRONES])amount=amount*75/100;
  if(m[M_X_TACTICAL_LINK]||m[M_X_FORMATION_RAMPART])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&distance2(p.x,p.y,constructs[c].x,constructs[c].y)<55*55){amount=amount*80/100;break;}
  untouched_=0;if(m[M_X_REBUKE])stored_damage_=(uint16_t)minv(200,stored_damage_+amount);
  if(m[M_X_MAELSTROM])maelstrom_=(uint16_t)minv(150,maelstrom_+amount);
  if(p.shield>0){
   if(p.body==B_SENTINEL)amount=amount*75/100;
   if(m[M_AEGIS]&&approx(p.vx,p.vy)<FP)amount=amount*80/100;
   if(m[M_BARRIER])amount=minv(amount,30);
   if(m[M_CHANNELING]||m[M_X_GEMINI]){int direct=maxv(1,amount/(m[M_X_GEMINI]?2:3));p.hull=maxv(0,p.hull-direct);amount-=direct;}
   if(p.shield_type==S_BASTION)p.shield_energy=minv(200,p.shield_energy+amount);
   int absorbed=minv(p.shield,amount);p.shield-=absorbed;amount-=absorbed;
   if(!m[M_X_TEMPEST_BREAK]||p.shield==0)p.shield_delay=stats.shield_cooldown;
   if(m[M_X_REFLEXIVE_SHIELDS])blast(p.x,p.y,stats.shield_radius,amount/2+4);
   if(m[M_X_GALVANIC_OUTBURST])for(unsigned j=0;j<(unsigned)(2+m[M_X_GALVANIC_OUTBURST]);j++){int n=emitBullet(p.x,p.y,(uint8_t)range(256),4,8*m[M_X_GALVANIC_OUTBURST],SHOT_SPARK,45);if(n>=0)bullets[n].homing=6;}
   if(p.shield==0)shieldBroken();
  }
  if(amount>0){
   amount=maxv(1,amount-stats.plating-(m[M_ADAPTIVE_ARMOR]&&damage_flash?3:0));
   if(m[M_FORCE_ARMOR]&&amount>12)amount=amount*75/100;
   p.hull=maxv(0,p.hull-amount);
   if(amount>=12&&m[M_X_RETRIBUTION])spawnConstruct(C_MINE);
   if(amount>=12&&m[M_X_OUTRAGE_MODULE]&&p.hull>0&&!m[M_X_HIDDEN_POWER])fireWeapon();
   if(p.shield_type==S_BASTION&&p.shield_energy){blast(p.x,p.y,35,p.shield_energy);p.shield_energy=0;}
   if(p.body==B_VIPER){for(unsigned i=0;i<7;i++){int s=emitBullet(p.x,p.y,(uint8_t)(i*256/7),4,12+level/2,SHOT_DART,42,0,2);if(s>=0)bullets[s].homing=4;}}
  }
  p.invulnerable=maxv(p.invulnerable,7);damage_flash=18;
 }
 preventDeath();
}
void Game::preventDeath() {
 if(player.hull>0)return;
 if(player.mods[M_X_LAST_STAND]&&!last_stand_used_){last_stand_used_=1;player.hull=maxv(1,player.max_hull/2);player.shield=0;player.shield_delay=stats.shield_cooldown;player.invulnerable=120;blast(player.x,player.y,65,100);emitEffect(player.x,player.y,FX_WARP,40,30,3);return;}
 player.hull=0;state=GameOver;emitEffect(player.x,player.y,FX_BURST,35,30,1);
}
void Game::shieldBroken() {
 Player &p=player;emitEffect(p.x,p.y,FX_SHIELD_BREAK,stats.shield_radius,18,3);
 if(p.mods[M_VOLATILE_SHIELDS])blast(p.x,p.y,stats.shield_radius*2,55*stats.shield_effect/100);
 if(p.mods[M_EMERGENCY_SYSTEMS])p.invulnerable=maxv(p.invulnerable,45);
 if(p.mods[M_X_TEMPEST_BREAK]){blast(p.x,p.y,75,70*stats.shield_effect/100);for(unsigned i=0;i<12;i++)emitBullet(p.x,p.y,(uint8_t)(i*256/12),3,12,SHOT_SPARK,55);}
 if(p.mods[M_X_DEATH_BLOSSOM])for(unsigned i=0;i<16;i++){int n=emitBullet(p.x,p.y,(uint8_t)(i*16),maxv(3,weapons[p.weapon].speed),weapons[p.weapon].damage*stats.damage/100,SHOT_BLASTER,45);if(n>=0)bullets[n].blast=10;}
 if(p.shield_type==S_WARP){int32_t ox=p.x,oy=p.y;int distance=stats.shield_radius*2;
  p.x=wrap(p.x+dirX(p.angle)*distance,worldWidth());p.y=wrap(p.y+dirY(p.angle)*distance,worldHeight());
  blast(ox,oy,25,50*stats.shield_effect/100);blast(p.x,p.y,25,50*stats.shield_effect/100);
  emitEffect(ox,oy,FX_WARP,25,16,3);p.invulnerable=maxv(p.invulnerable,35);
 }
}
void Game::hitEnemy(unsigned index,int damage,bool burn) {
 if(index>=MAX_ENEMIES||!enemies[index].active||damage<=0)return;
 Enemy &e=enemies[index];const uint8_t *m=player.mods;
 if(m[M_X_HIDDEN_POWER])damage=damage*180/100;
 if(m[M_X_LEAF_ON_THE_WIND])damage=damage*(100+minv(80,untouched_/15))/100;
 if(burn){damage=damage*(100+35*m[M_CANDESCENCE])/100;
  damage=damage*(100+25*m[M_X_SOLAR_HEART])/100;
  if(m[M_PURIFICATION])damage=damage*(100+70*(player.max_hull-player.hull)/maxv(1,player.max_hull))/100;
  if(m[M_CORROSION])e.vulnerability=(uint8_t)minv(60,e.vulnerability+2);
 } else {if(m[M_BLITZ]&&e.hull>e.max_hull*9/10)damage=damage*140/100;}
 damage=damage*(100+e.vulnerability)/100;
 if(e.type==E_BULWARK&&!burn){uint8_t a=direction(delta(player.x,e.x,worldWidth()),delta(player.y,e.y,worldHeight()));if(absn(turnDelta(e.angle,a))<45)damage=maxv(1,damage/3);}
 e.hull=(int16_t)maxv(0,e.hull-damage);e.flash=3;
 if(!burn&&m[M_INCENDIARY]){e.burn_time=maxv(e.burn_time,90);e.burn_damage=maxv(e.burn_damage,damage/10+1);}
 if(m[M_TERMINATE]&&e.hull<e.max_hull*8/100)e.hull=0;
 if(e.hull<=0)destroyEnemy(index);
}
void Game::blast(int32_t x,int32_t y,int radius,int damage,int excluded) {
 radius=clamp(radius,1,100);
 if(player.mods[M_CONCENTRATED_BLAST])damage=damage*145/100;
 emitEffect(x,y,FX_RING,radius,12,1);
 for(unsigned i=0;i<MAX_ENEMIES;i++)if((int)i!=excluded&&enemies[i].active){
  int reach=radius+enemies[i].radius;
  if(distance2(x,y,enemies[i].x,enemies[i].y)<=reach*reach)hitEnemy(i,damage);
 }
}
void Game::destroyEnemy(unsigned index) {
 if(!enemies[index].active)return;
 Enemy e=enemies[index];
 enemies[index].active=0;++kills;score+=enemy_types[e.type].xp*10*(e.elite+1)+wave;
 kill_buff_=150;
 emitEffect(e.x,e.y,FX_BURST,e.radius+7,14,e.type>=E_SCION?3:1);
 emitPickup(e.x,e.y,PICKUP_XP,enemy_types[e.type].xp*(1+e.elite));
 if(player.mods[M_X_PRIORITY_ZERO]){construct_clock_=(uint16_t)minv(1000,construct_clock_+15);for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active)constructs[c].hull=minv(constructs[c].max_hull,constructs[c].hull+1);}
 if(player.mods[M_X_SPONTANEOUS_GENERATION]&&range(100)<(unsigned)player.mods[M_X_SPONTANEOUS_GENERATION]*12){int c=spawnConstruct(C_SWARM);if(c>=0)constructs[c].life=300;}
 if(player.mods[M_X_DEFLAGRATION]&&e.burn_time&&rupture_depth_<3){++rupture_depth_;blast(e.x,e.y,38,25);for(unsigned j=0;j<MAX_ENEMIES;j++)if(enemies[j].active&&distance2(e.x,e.y,enemies[j].x,enemies[j].y)<38*38){enemies[j].burn_time=90;enemies[j].burn_damage=maxv(enemies[j].burn_damage,5);}--rupture_depth_;}
 if(range(player.body==B_RESEARCH?12:28)==0)emitPickup(e.x,e.y,PICKUP_HEAL,12);
 if(player.mods[M_ESSENCE_SAP]&&kills%3==0)player.hull=minv(player.max_hull,player.hull+2);
 if(e.type>=E_SCION){boss_index=-1;emitPickup(e.x,e.y,PICKUP_REROLL,2);emitPickup(e.x,e.y,PICKUP_HEAL,35);next_wave=minv(next_wave,60);}
 if(e.type==E_ASTEROID&&e.radius>=9){for(unsigned j=0;j<2;j++){int n=spawnEnemy(E_ASTEROID,pixel(e.x)+(int)j*8-4,pixel(e.y));if(n>=0){enemies[n].radius=5;enemies[n].hull=enemies[n].max_hull=12;enemies[n].vx=(j?1:-1)*FP;enemies[n].vy=FP;}}}
 if(player.mods[M_SPLINTER])for(unsigned j=0;j<3;j++)emitBullet(e.x,e.y,(uint8_t)range(256),4,12*stats.damage/100,SHOT_SHRAPNEL,25,0,1);
 if((player.mods[M_RUPTURE]||(player.mods[M_PURGE]&&e.burn_time))&&rupture_depth_<3){
  ++rupture_depth_;blast(e.x,e.y,20*stats.blast/100,maxv(8,e.max_hull*15/100));--rupture_depth_;
 }
}
int Game::spawnEnemy(unsigned type,int x,int y,unsigned elite) {
 if(type>=ENEMY_COUNT)return -1;
 if(player.mods[M_X_BRAVADO])elite=maxv(1,elite);
 unsigned slot=MAX_ENEMIES;for(unsigned i=0;i<MAX_ENEMIES;i++)if(!enemies[i].active){slot=i;break;}
 if(slot==MAX_ENEMIES){++dropped_enemies;return -1;}
 Enemy &e=enemies[slot];clearBytes(&e,sizeof(e));const EnemyDef &def=enemy_types[type];
 e.active=1;e.type=(uint8_t)type;e.elite=(uint8_t)minv(2,elite);e.x=wrap(x*FP,worldWidth());e.y=wrap(y*FP,worldHeight());
 int scaling=100+maxv(0,wave-5)*3;if(type>=E_SCION)scaling=100+maxv(0,wave-60)*2;
 e.hull=e.max_hull=(int16_t)clamp(def.hull*scaling/100*(100+e.elite*60)/100,1,30000);
 e.radius=(uint8_t)(def.radius+elite);e.angle=(uint8_t)range(256);e.fire_delay=20+range(40);
 if(type==E_ASTEROID){e.vx=dirX(e.angle);e.vy=dirY(e.angle);}
 if(type>=E_SCION){boss_index=(int16_t)slot;e.fire_delay=80;}
 emitEffect(e.x,e.y,FX_SPAWN,e.radius+4,15,1);return (int)slot;
}
const char *Game::waveName() const {
 if(wave%10==0&&wave>0)return "BOSS INBOUND";
 static const char *const names[]={"ASTEROID BELT","SWARM","VANGUARD","PINCER","CROSS FIRE","ARMADA","HUNTERS","GUARD DETAIL","SINE WAVE"};
 return names[wave%9];
}
void Game::updateWaves() {
 if(next_wave)--next_wave;
 unsigned alive=enemyCount();
 if(wave&&alive==0&&next_wave>45)next_wave=45;
 if(next_wave)return;
 if(boss_index>=0&&enemies[boss_index].active){next_wave=30;return;}
 if(alive>38){next_wave=30;return;}
 ++wave;wave_banner=90;
 next_wave=(uint16_t)maxv(180,360-minv(180,wave*3));
 if(wave%10==0){
  static const uint8_t bosses[]={E_SCION,E_STATION_OMEGA,E_DREADNOUGHT,E_CERATOTITAN,E_DWELLER,E_GLAUCUS,E_WARBRINGER,E_SERAPH,E_CRUCIBLE,E_STAR_EATER};
  unsigned boss=bosses[(wave/10-1)%10];
  spawnEnemy(boss,worldWidth()/2,30);next_wave=900;
  for(unsigned i=0;i<(unsigned)minv(5,1+wave/20);i++)spawnEnemy(E_DRONE,30+i*50,15);
  return;
 }
 unsigned count=(unsigned)minv(13,3+wave/3),formation=wave%9;
 for(unsigned i=0;i<count;i++) {
  unsigned type;
  if(wave<3)type=E_ASTEROID;
  else switch(formation){
   case 0:type=E_ASTEROID;break;case 1:type=E_SWARMLING;break;case 2:type=E_WASP;break;
   case 3:type=i&1?E_SKIRMISHER:E_DRONE;break;case 4:type=E_DRONE;break;
   case 5:type=wave>12?E_FRIGATE:E_SPITFIRE;break;case 6:type=E_SKIRMISHER;break;
   case 7:type=i==0?E_BULWARK:E_MINE;break;default:type=E_SERPENT;break;
  }
  if(wave>12&&i==0)type=E_CHERUB;
  if(wave>18&&i%3==0)type=E_WARDEN+(wave+i)%6;
  int x,y;
  if(formation==3){x=(i&1)?worldWidth()-10:10;y=30+(int)(i/2)*25;}
  else if(formation==4){x=(int)range(worldWidth());y=(i&1)?worldHeight()-8:8;}
  else if(formation==7){x=worldWidth()/2+(int)i*13;y=20+(int)(i&1)*15;}
  else {uint8_t a=(uint8_t)(i*256/count+range(24));x=worldWidth()/2+dirX(a)*(worldWidth()/2-15)/FP;y=worldHeight()/2+dirY(a)*(worldHeight()/2-12)/FP;}
  unsigned elite=(wave>14&&range(100)<(unsigned)minv(30,wave))?1:0;
  if(wave>35&&range(25)==0)elite=2;
  spawnEnemy(type,x,y,elite);
 }
}
void Game::fireEnemy(Enemy &e) {
 int32_t aimx=player.x,aimy=player.y;
 if(player.mods[M_X_DECOY_SIGNAL])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_GUARDIAN&&distance2(e.x,e.y,constructs[c].x,constructs[c].y)<100*100){aimx=constructs[c].x;aimy=constructs[c].y;break;}
 uint8_t aim=direction(delta(aimx,e.x,worldWidth()),delta(aimy,e.y,worldHeight()));
 int damage=6+wave/9+e.elite*3,speed=2+(wave>25?1:0);
 unsigned count=1;int spread=0;
 unsigned kind=SHOT_ENEMY;
 switch(e.type){
  case E_WASP:count=2;spread=8;break;
  case E_SPITFIRE:count=5;spread=11;break;
  case E_FRIGATE:count=3;spread=13;speed=3;break;
  case E_SERPENT:aim=(uint8_t)(aim+dirY((uint8_t)e.age)/14);count=2;spread=16;break;
  case E_MINE:count=6;spread=43;aim=(uint8_t)e.age;break;
  case E_WARDEN:count=2;spread=18;kind=SHOT_VORTEX;speed=1;damage+=6;break;
  case E_JUGGERNAUT:count=3;spread=12;damage+=8;break;
  case E_MINELAYER:count=3;spread=20;break;
  case E_TRACER:aim=direction(delta(player.x+player.vx*12,e.x,worldWidth()),delta(player.y+player.vy*12,e.y,worldHeight()));speed=4;break;
  case E_BEAMCASTER:{
   int32_t ex=wrap(e.x+dirX(e.phase)*150,worldWidth()),ey=wrap(e.y+dirY(e.phase)*150,worldHeight());
   emitEffect(e.x,e.y,FX_BEAM,4,10,1,e.x+dirX(e.phase)*150,e.y+dirY(e.phase)*150);
   if(sweptHit(e.x,e.y,ex,ey,player.x,player.y,player.radius+4))damagePlayer(damage+16);
   return;
  }
  case E_SCION:count=e.phase==0?5:8;spread=e.phase==0?9:32;aim=e.phase==0?aim:(uint8_t)(e.age*2);break;
  case E_DREADNOUGHT:count=e.phase==0?3:7;spread=e.phase==0?10:37;damage+=6;speed=3;break;
  case E_STATION_OMEGA:count=10;spread=26;aim=(uint8_t)(e.age/2);break;
  case E_WARBRINGER:count=7;spread=10;speed=e.phase?4:3;damage+=5;break;
  case E_SERAPH:count=e.phase?12:4;spread=e.phase?21:12;aim=e.phase?(uint8_t)(e.age*3):aim;break;
  case E_STAR_EATER:count=14;spread=18;aim=(uint8_t)(e.age*(e.phase+1));damage+=5;break;
  case E_DWELLER:count=e.phase?9:5;spread=10;speed=3;break;
  case E_CERATOTITAN:count=6;spread=18;aim=(uint8_t)(aim+(e.age%2?16:-16));break;
  case E_GLAUCUS:count=8;spread=20;aim=(uint8_t)(e.age*2);kind=e.phase?SHOT_VORTEX:SHOT_ENEMY;speed=e.phase?1:3;break;
  case E_CRUCIBLE:count=12;spread=21;aim=(uint8_t)(e.age/2);damage+=4;break;
  default:break;
 }
 for(unsigned i=0;i<count;i++)emitBullet(e.x,e.y,(uint8_t)(aim+((int)i*2-(int)count+1)*spread/2),speed,damage,kind,e.type>=E_SCION?140:100,1,kind==SHOT_VORTEX?6:e.type>=E_SCION?3:2);
}
void Game::updateEnemies() {
 for(unsigned i=0;i<MAX_ENEMIES;i++){
  Enemy &e=enemies[i];if(!e.active)continue;
  const EnemyDef &def=enemy_types[e.type];++e.age;if(e.flash)--e.flash;
  if(e.vulnerability&&frame%20==0)--e.vulnerability;
  if(e.burn_time){--e.burn_time;if(frame%15==0){hitEnemy(i,maxv(1,e.burn_damage),true);if(!e.active)continue;}}
  int dx=delta(player.x,e.x,worldWidth()),dy=delta(player.y,e.y,worldHeight()),dist=approx(dx,dy);
  if(player.mods[M_X_DECOY_SIGNAL])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_GUARDIAN&&distance2(e.x,e.y,constructs[c].x,constructs[c].y)<100*100){dx=delta(constructs[c].x,e.x,worldWidth());dy=delta(constructs[c].y,e.y,worldHeight());dist=approx(dx,dy);break;}
  uint8_t aim=direction(dx,dy);int speed=def.speed*FP*(100+e.elite*15)/100;
  if(e.type>=E_SCION){
   e.phase=(uint8_t)(e.hull<e.max_hull/2?1:0);
   if(e.type==E_STAR_EATER&&e.hull<e.max_hull/4)e.phase=2;
   int tx=worldWidth()/2+dirX((uint8_t)(e.age/3))*80/FP;
   int ty=worldHeight()/2+dirY((uint8_t)(e.age/4))*60/FP;
   aim=direction(delta(tx*FP,e.x,worldWidth()),delta(ty*FP,e.y,worldHeight()));speed=FP/2+e.phase*FP/5;
   if(e.type==E_SERAPH&&e.age%210==0){e.x=wrap((int32_t)range(worldWidth())*FP,worldWidth());e.y=wrap((int32_t)range(worldHeight())*FP,worldHeight());emitEffect(e.x,e.y,FX_WARP,30,15,3);}
   if(e.type==E_STATION_OMEGA&&e.age%160==0)spawnEnemy(E_DRONE,pixel(e.x)+20,pixel(e.y));
   if(e.type==E_DWELLER&&e.age%150==0){spawnEnemy(E_SWARMLING,pixel(e.x)-15,pixel(e.y));spawnEnemy(E_WASP,pixel(e.x)+15,pixel(e.y));}
   if(e.type==E_CERATOTITAN&&e.age%180<35){aim=direction(dx,dy);speed=3*FP;}
   if(e.type==E_GLAUCUS&&e.age%210==0){e.x=wrap(player.x+worldWidth()*FP/3,worldWidth());e.y=wrap(player.y+worldHeight()*FP/3,worldHeight());emitEffect(e.x,e.y,FX_WARP,32,20,3);}
   if(e.type==E_CRUCIBLE&&e.age%120==0){emitEffect(e.x,e.y,FX_RING,65,25,1);for(unsigned c=0;c<6;c++)spawnEnemy(E_MINE,pixel(e.x)+dirX((uint8_t)(c*256/6))*45/FP,pixel(e.y)+dirY((uint8_t)(c*256/6))*45/FP);}
   if(e.type==E_STAR_EATER){
    if(e.age%180<70){int polar=player.mods[M_X_POLAR_INVERSION]?-1:1;player.vx+=clamp(-dx,-30,30)/2*polar;player.vy+=clamp(-dy,-30,30)/2*polar;}
    if(e.age%180==100){emitEffect(e.x,e.y,FX_RING,68,20,3);if(dist<68)damagePlayer(15);}
   }
  } else if(e.type==E_ASTEROID){
   e.angle=(uint8_t)(e.angle+1);
  } else if(e.type==E_MINE){speed=FP/3;}
  else if(e.type==E_DRONE||e.type==E_SPITFIRE||e.type==E_BULWARK){if(dist<65)aim=(uint8_t)(aim+70);if(dist<35)aim=(uint8_t)(aim+58);}
  else if(e.type==E_SKIRMISHER){if(e.age%120<35){aim=e.angle;speed=4*FP;}else if(dist<75)aim=(uint8_t)(aim+55);}
  else if(e.type==E_SERPENT)aim=(uint8_t)(aim+dirY((uint8_t)(e.age*3))/10);
  else if(e.type==E_WARDEN||e.type==E_BEAMCASTER||e.type==E_MINELAYER||e.type==E_CHERUB){if(dist<90)aim=(uint8_t)(aim+72);}
  else if(e.type==E_TRACER)aim=(uint8_t)(aim+dirY((uint8_t)(e.age*4))/6);
  if(e.type==E_MINELAYER&&e.age%150==0)spawnEnemy(E_MINE,pixel(e.x),pixel(e.y));
  if(e.type==E_CHERUB&&e.age%30==0){emitEffect(e.x,e.y,FX_RING,40,12,2);for(unsigned j=0;j<MAX_ENEMIES;j++)if(j!=i&&enemies[j].active&&distance2(e.x,e.y,enemies[j].x,enemies[j].y)<65*65)enemies[j].hull=minv(enemies[j].max_hull,enemies[j].hull+5);}
  if(player.body==B_SPECTRE&&player.cloak>130&&e.type<E_SCION){aim=(uint8_t)(e.age/4);speed/=2;}
  if(player.shield>0&&player.shield_type==S_TEMPORAL&&dist<stats.shield_radius){speed=speed*35/100;}
  if(e.stun){--e.stun;speed/=3;}
  if(e.type!=E_ASTEROID){e.angle=toward(e.angle,aim,e.type>=E_SCION?2:3);e.vx=e.vx*7/8+dirX(e.angle)*speed/FP/8;e.vy=e.vy*7/8+dirY(e.angle)*speed/FP/8;}
  e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());
  if(e.fire_delay)--e.fire_delay;
  if(e.type==E_BEAMCASTER&&e.fire_delay==15){e.phase=direction(dx,dy);emitEffect(e.x,e.y,FX_BEAM,1,15,1,e.x+dirX(e.phase)*150,e.y+dirY(e.phase)*150);}
  if(def.fire_delay&&!e.fire_delay&&e.age>30){fireEnemy(e);e.fire_delay=(uint16_t)maxv(8,def.fire_delay-(e.phase?4:0)-e.elite*5);}
  int contact=e.radius+player.radius;
  if(!player.mods[M_X_APOTHEOSIS]&&e.age>20&&distance2(e.x,e.y,player.x,player.y)<contact*contact){
   if(!player.invulnerable){int crash=12+approx(player.vx,player.vy)/18;
    if(player.body==B_HULLBREAKER)crash*=2;
    crash=crash*stats.crash_damage/100;
    int taken=def.contact;if(player.body==B_HULLBREAKER||player.body==B_VIPER)taken=taken*60/100;
    hitEnemy(i,crash);damagePlayer(taken);
    if(e.active&&player.mods[M_X_PHANTOM_STRIKE]){player.x=wrap(e.x+dirX(player.angle)*(e.radius+player.radius+6),worldWidth());player.y=wrap(e.y+dirY(player.angle)*(e.radius+player.radius+6),worldHeight());player.angle=(uint8_t)(player.angle+128);fireWeapon();}
    if(player.mods[M_SKIRMISH])player.invulnerable=maxv(player.invulnerable,14);
    if(player.body==B_VIPER)for(unsigned j=0;j<5;j++){int n=emitBullet(player.x,player.y,(uint8_t)(aim+128+(int)j*10-20),4,18,SHOT_DART,40);if(n>=0)bullets[n].homing=3;}
    player.vx-=dx*3;player.vy-=dy*3;
   }
  }
  if(e.active&&segment_count&&frame%5==i%5){for(unsigned s=0;s<segment_count;s++)if(distance2(e.x,e.y,segment_x[s],segment_y[s])<(e.radius+5)*(e.radius+5)){hitEnemy(i,(10+approx(player.vx,player.vy)/24)*stats.crash_damage/100);break;}}
 }
}
void Game::buildGrid() {
 for(unsigned i=0;i<256;i++)grid_[i]=-1;
 for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active){int cell=pixel(enemies[i].x)/32+(pixel(enemies[i].y)/30)*(worldWidth()/32);grid_next_[i]=grid_[cell];grid_[cell]=(int16_t)i;}
}
void Game::updateShields() {
 Player &p=player;if(p.shield<=0)return;
 int32_t sx=p.mods[M_X_SANCTUARY]?sanctuary_x:p.x,sy=p.mods[M_X_SANCTUARY]?sanctuary_y:p.y;
 int rad=stats.shield_radius,power=stats.shield_effect;
 if(p.mods[M_X_SANCTUARY])rad=rad*160/100;
 if(p.mods[M_X_CHARGED_SHIELDS]){rad+=p.charge/10;power+=p.charge/2;}
 power+=p.mods[M_X_ENERGIZED_SHIELDS]*approx(p.vx,p.vy)/20;
 int rad2=rad*rad;
 if(frame%6==0){
  unsigned drains=0;
  for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&distance2(sx,sy,enemies[i].x,enemies[i].y)<rad2){
   if(p.shield_type==S_HALO||p.shield_type==S_SIPHON||p.mods[M_WEAPONIZED_SHIELDS]){
    int damage=(p.shield_type==S_SIPHON?5:4)*power/100;hitEnemy(i,maxv(1,damage),true);++drains;
   }
   if(p.shield_type==S_SHOCKWAVE){int polar=p.mods[M_X_POLAR_INVERSION]?-1:1;enemies[i].vx+=delta(enemies[i].x,sx,worldWidth())*4*polar;enemies[i].vy+=delta(enemies[i].y,sy,worldHeight())*4*polar;}
   if(p.mods[M_X_MAELSTROM]&&maelstrom_)hitEnemy(i,2+maelstrom_/15,true);
  }
  if(drains&&p.shield_type==S_HALO){p.shield=maxv(0,p.shield-1);p.shield_delay=stats.shield_cooldown;if(!p.shield)shieldBroken();}
  if(drains&&p.shield_type==S_SIPHON&&frame%30==0)p.shield=minv(p.max_shield,p.shield+1);
 }
 if(p.shield_type==S_ORBITAL){
  for(unsigned orb=0;orb<3;orb++){
   uint8_t a=(uint8_t)(frame*2+orb*256/3);int32_t ox=wrap(sx+dirX(a)*rad,worldWidth()),oy=wrap(sy+dirY(a)*rad,worldHeight());
   if(frame%5==0)for(unsigned i=0;i<MAX_ENEMIES;i++)if(enemies[i].active&&distance2(ox,oy,enemies[i].x,enemies[i].y)<(enemies[i].radius+5)*(enemies[i].radius+5))hitEnemy(i,6*power/100);
   for(unsigned i=0;i<MAX_BULLETS;i++)if(bullets[i].active&&bullets[i].owner&&distance2(ox,oy,bullets[i].x,bullets[i].y)<7*7)bullets[i].active=0;
  }
 }
}
void Game::updateBullets() {
 for(unsigned i=0;i<MAX_BULLETS;i++) {
  Bullet &b=bullets[i];if(!b.active)continue;
  ++b.age;
  if(b.life)--b.life;
  if(!b.life){
   b.active=0;
   if(!b.owner&&(b.kind==SHOT_GRENADE||(b.kind==SHOT_SHRAPNEL&&b.blast))){
    int32_t bx=b.x,by=b.y;int bd=b.damage,br=b.blast;unsigned kind=b.kind;
    blast(bx,by,br,bd);
    if(kind==SHOT_GRENADE)for(unsigned j=0;j<3;j++){int n=emitBullet(bx,by,(uint8_t)(j*256/3+range(30)),2,maxv(1,bd/3),SHOT_SHRAPNEL,10);if(n>=0)bullets[n].blast=10;}
   }
   continue;
  }
  if(!b.owner&&b.kind==SHOT_VORTEX){
   b.x=wrap(b.x+b.vx,worldWidth());b.y=wrap(b.y+b.vy,worldHeight());
   if(frame%5==0){
    int reach=b.radius*2;
    for(unsigned j=0;j<MAX_ENEMIES;j++)if(enemies[j].active&&distance2(b.x,b.y,enemies[j].x,enemies[j].y)<(reach+enemies[j].radius)*(reach+enemies[j].radius)){
     int dx=delta(b.x,enemies[j].x,worldWidth()),dy=delta(b.y,enemies[j].y,worldHeight());
     int polar=player.mods[M_X_POLAR_INVERSION]?-1:1;enemies[j].vx+=clamp(dx,-20,20)*5*polar;enemies[j].vy+=clamp(dy,-20,20)*5*polar;
     hitEnemy(j,b.damage,true);
    }
   }
   if(b.life<25&&b.radius>5&&frame%4==0)--b.radius;
   continue;
  }
  if(!b.owner&&b.homing&&((frame+i)&3)==0){int target=nearestEnemy(b.x,b.y,150);
   if(target>=0){uint8_t a=direction(delta(enemies[target].x,b.x,worldWidth()),delta(enemies[target].y,b.y,worldHeight()));
    b.angle=toward(b.angle,a,b.homing*3);
    b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
   }
  }
  if(b.kind==SHOT_FLAK){b.vx=b.vx*245/256;b.vy=b.vy*245/256;b.speed=(uint16_t)(b.speed*245/256);if(b.age%7==0&&b.radius>1)--b.radius;}
  if(b.kind==SHOT_BLASTER&&b.age%14==0&&b.radius>1)--b.radius;
  int32_t ox=b.x,oy=b.y;
  int move_percent=100;
  if(b.owner&&player.shield>0&&player.shield_type==S_TEMPORAL&&distance2(b.x,b.y,player.x,player.y)<stats.shield_radius*stats.shield_radius)move_percent=30;
  b.x=wrap(b.x+b.vx*move_percent/100,worldWidth());b.y=wrap(b.y+b.vy*move_percent/100,worldHeight());
  bool wrapped=absn(b.x-ox)>worldWidth()*FP/2||absn(b.y-oy)>worldHeight()*FP/2;
  if(wrapped){if(b.wraps){--b.wraps;b.damage=(int16_t)minv(2000,b.damage*125/100);}else{b.active=0;continue;}}
  if(!b.owner&&player.mods[M_X_ANNIHILATION])for(unsigned j=FRIENDLY_BULLETS;j<MAX_BULLETS;j++)if(bullets[j].active&&bullets[j].owner&&sweptHit(ox,oy,b.x,b.y,bullets[j].x,bullets[j].y,b.radius+bullets[j].radius)){
   b.damage-=bullets[j].damage;bullets[j].active=0;if(b.damage<=0){b.active=0;break;}}
  if(!b.active)continue;
  if(b.owner){
   if(player.shield>0&&player.shield_type==S_REFLECT&&distance2(b.x,b.y,player.x,player.y)<stats.shield_radius*stats.shield_radius){
    b.owner=0;b.damage=(int16_t)minv(2000,b.damage*2*stats.shield_effect/100);b.homing=4;b.age=0;b.life=65;
    int target=nearestEnemy(b.x,b.y);b.angle=target>=0?direction(delta(enemies[target].x,b.x,worldWidth()),delta(enemies[target].y,b.y,worldHeight())):(uint8_t)(b.angle+128);
    b.speed=4*FP;b.vx=dirX(b.angle)*4;b.vy=dirY(b.angle)*4;continue;
   }
   // Bastion only covers the forward arc. Side/rear shots bypass its shield.
   if(sweptHit(ox,oy,b.x,b.y,player.x,player.y,player.radius+b.radius)){
    b.active=0;
    if(player.shield_type==S_BASTION&&player.shield>0){
     uint8_t incoming=direction(-b.vx,-b.vy);
     if(absn(turnDelta(player.angle,incoming))>55){
      if(!player.invulnerable){player.hull=maxv(0,player.hull-maxv(1,b.damage-stats.plating));player.invulnerable=7;damage_flash=15;
       preventDeath();}
     } else damagePlayer(b.damage);
    } else damagePlayer(b.damage);
    continue;
   }
   for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&sweptHit(ox,oy,b.x,b.y,constructs[c].x,constructs[c].y,constructs[c].radius+b.radius)){
    Construct &cr=constructs[c];bool ally=cr.kind==C_ALLY||cr.kind==C_INTERCEPTOR||cr.kind==C_GUARDIAN;
    if(ally&&(player.mods[M_X_EVASION]||player.mods[M_X_OUTMANEUVER])&&range(100)<35){
     if(player.mods[M_X_OUTMANEUVER]&&cr.kind==C_INTERCEPTOR)emitBullet(cr.x,cr.y,(uint8_t)(b.angle+128),5,18,SHOT_BLASTER,40);
     continue;
    }
    int damage=b.damage;
    if(player.mods[M_X_TACTICAL_LINK]&&distance2(cr.x,cr.y,player.x,player.y)<55*55)damage=damage*75/100;
    if(player.mods[M_X_SHIELDED_DRONES]&&player.shield>0){int absorbed=minv(player.shield,damage);player.shield-=absorbed;damage-=absorbed;player.shield_delay=stats.shield_cooldown;if(!player.shield)shieldBroken();}
    if(player.mods[M_X_NANOTECH_DISSEMINATION]&&cr.kind!=C_GUARDIAN)for(unsigned j=0;j<MAX_CONSTRUCTS;j++)if(constructs[j].active&&constructs[j].kind==C_GUARDIAN){constructs[j].hull-=damage;damage=0;break;}
    cr.hull-=damage;
    if(player.mods[M_X_COUNTERPULSE]&&cr.kind==C_DEFENSE_DRONE)blast(cr.x,cr.y,25,18);
    if(player.mods[M_X_TACTICAL_BEACON]&&cr.kind==C_MINE&&cr.charges&&cr.fire_delay<3){blast(cr.x,cr.y,45,70*constructPower(C_MINE)/100);cr.fire_delay=30;if(!--cr.charges)cr.active=0;}
    b.active=0;break;
   }
   continue;
  }
  if(player.mods[M_X_TACTICAL_BEACON])for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&constructs[c].kind==C_MINE&&constructs[c].charges&&constructs[c].fire_delay<3&&sweptHit(ox,oy,b.x,b.y,constructs[c].x,constructs[c].y,constructs[c].radius+b.radius)){
   Construct &cr=constructs[c];blast(cr.x,cr.y,45,70*constructPower(C_MINE)/100);cr.fire_delay=30;if(!--cr.charges)cr.active=0;b.active=0;break;}
  if(!b.active)continue;
  int cx=pixel(b.x)/32,cy=pixel(b.y)/30,columns=worldWidth()/32,rows=(worldHeight()+29)/30;
  bool collision=false;
  for(int gy=-2;gy<=2&&!collision;gy++)for(int gx=-2;gx<=2&&!collision;gx++){
   int cellx=(cx+gx+columns)%columns,celly=(cy+gy+rows)%rows;
   for(int j=grid_[celly*columns+cellx];j>=0&&!collision;j=grid_next_[j]){
    Enemy &e=enemies[j];if(!e.active||j==b.last_hit)continue;
    if(!sweptHit(ox,oy,b.x,b.y,e.x,e.y,b.radius+e.radius))continue;
    int damage=b.damage;if(player.mods[M_SNIPE])damage=damage*(100+minv(90,b.age*2))/100;
    bool destroy=damage>=e.hull;int32_t ex=e.x,ey=e.y;
    if(b.kind==SHOT_DART||b.kind==SHOT_SPARK){e.burn_time=(uint16_t)minv(300,e.burn_time+90);e.burn_damage=(int16_t)minv(500,e.burn_damage+maxv(2,damage/5));}
    hitEnemy((unsigned)j,damage);
    b.last_hit=(int16_t)j;
    if(b.blast)blast(ex,ey,b.blast,b.kind==SHOT_SPLIT?damage/2:damage,j);
    if(b.kind==SHOT_RAIL&&destroy){
     blast(ex,ey,13*stats.blast/100,damage/3,j);
     for(unsigned sh=0;sh<3;sh++)emitBullet(ex,ey,(uint8_t)(b.angle+(int)sh*18-18),4,damage/5,SHOT_SHRAPNEL,18,0,1);
    }
    if(b.kind==SHOT_GRENADE){
     int bd=damage;b.active=0;
     for(unsigned sh=0;sh<3;sh++){int n=emitBullet(ex,ey,(uint8_t)(sh*256/3),2,maxv(1,bd/3),SHOT_SHRAPNEL,10);if(n>=0)bullets[n].blast=10;}
     collision=true;
    } else if(b.pierce&&destroy){--b.pierce;}
    else if(b.bounces){--b.bounces;int next=-1,dist=1000000;for(unsigned q=0;q<MAX_ENEMIES;q++)if(enemies[q].active&&(int)q!=j){int d=distance2(b.x,b.y,enemies[q].x,enemies[q].y);if(d<dist){dist=d;next=q;}}
     if(next>=0){b.angle=direction(delta(enemies[next].x,b.x,worldWidth()),delta(enemies[next].y,b.y,worldHeight()));b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;b.life=(uint16_t)minv(200,b.life+15);collision=true;}
     else {b.active=0;collision=true;}}
    else {b.active=0;collision=true;}
   }
  }
 }
}
int Game::constructPower(unsigned kind) const {
 const uint8_t *m=player.mods;int power=stats.construct_damage;
 if((kind==C_MINE&&m[M_X_SPECIALIST_MINE])||(kind==C_TURRET&&m[M_X_SPECIALIST_TURRET])||
    ((kind==C_DRONE||kind==C_DEFENSE_DRONE)&&m[M_X_SPECIALIST_DRONE])||
    ((kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN)&&m[M_X_SPECIALIST_ALLY]))power=power*160/100;
 if((kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN)&&m[M_X_EVOLUTION])power=power*(100+level*3)/100;
 return power;
}
int Game::spawnConstruct(unsigned kind) {
 if(kind>C_BLADE)return -1;
 if(player.mods[M_X_HIDDEN_POWER])return -1;
 if(player.body==B_ARCHITECT&&constructCount())return -1;
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(!constructs[i].active){
  Construct &c=constructs[i];clearBytes(&c,sizeof(c));c.active=1;c.kind=(uint8_t)kind;c.x=player.x;c.y=player.y;c.angle=player.angle;
  c.radius=kind==C_TURRET?6:kind==C_SWARM?3:4;
  int hull=kind==C_TURRET?90:kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN?65:kind==C_MINE?15:30;
  hull=hull*(100+40*player.mods[M_GRACE_PROTOCOL]+25*player.mods[M_ELEGANT_CONSTRUCTION])/100;
  hull=hull*constructPower(kind)/maxv(1,stats.construct_damage);
  if(player.mods[M_X_APEX_MACHINERY])hull*=2;
  if((kind==C_DRONE||kind==C_DEFENSE_DRONE)&&player.mods[M_X_FLEET]){unsigned drones=0;for(unsigned j=0;j<MAX_CONSTRUCTS;j++)if(constructs[j].active&&(constructs[j].kind==C_DRONE||constructs[j].kind==C_DEFENSE_DRONE)&&constructs[j].promoted)++drones;if(drones<2){c.promoted=1;hull*=2;}}
  if(player.body==B_ARCHITECT)hull*=3;
  c.hull=c.max_hull=(int16_t)hull;c.life=kind==C_MINE?150:kind==C_TURRET?600:kind==C_BLADE?150:0;
  if(kind==C_MINE&&player.mods[M_X_TACTICAL_BEACON]){c.hull=c.max_hull=(int16_t)minv(10000,hull*8);c.life=900;c.radius=10;c.charges=3;}
  c.fire_delay=10;c.vx=dirX(c.angle)*2;c.vy=dirY(c.angle)*2;
  emitEffect(c.x,c.y,FX_SPAWN,8,8,2);
  if((kind==C_DRONE||kind==C_DEFENSE_DRONE)&&player.mods[M_X_ROCKET_DRONES])damagePlayer(2,true);
  return (int)i;
 }
 return -1;
}
void Game::updateConstructs() {
 const uint8_t *m=player.mods;
 unsigned wanted[9]={0,0,0,0,0,0,0,0,0},count[9]={0,0,0,0,0,0,0,0,0};
 unsigned added=player.body==B_ENGINEER?1:0,mult=m[M_OVERSEER]?2:1;
 if(m[M_DRONES])wanted[m[M_DEFENSE_DRONES]?C_DEFENSE_DRONE:C_DRONE]=(2+added)*mult;
 if(m[M_MINES])wanted[C_MINE]=(2+2*m[M_MINEFIELD]+added)*mult;
 if(m[M_X_TACTICAL_BEACON])wanted[C_MINE]=1;
 if(m[M_TURRET])wanted[C_TURRET]=1+added+m[M_OVERSEER];
 if(m[M_ALLY])wanted[m[M_GUARDIAN]?C_GUARDIAN:m[M_INTERCEPTOR]?C_INTERCEPTOR:C_ALLY]=1+added+m[M_WINGMAN]+m[M_OVERSEER];
 if(player.body==B_CARRIER)wanted[C_SWARM]=(unsigned)minv(7,3+level/8);
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active){
  Construct &c=constructs[i];
  if(c.kind==C_DRONE&&m[M_DEFENSE_DRONES])c.kind=C_DEFENSE_DRONE;
  if(c.kind==C_ALLY&&(m[M_INTERCEPTOR]||m[M_GUARDIAN]))c.kind=m[M_GUARDIAN]?C_GUARDIAN:C_INTERCEPTOR;
  ++count[c.kind];
 }
 if(++construct_clock_>=(unsigned)maxv(8,45*100/maxv(50,stats.assembly))){
  construct_clock_=0;
  // The highest advanced construct is preferred by Architect's single slot.
  static const uint8_t order[]={C_ALLY,C_INTERCEPTOR,C_GUARDIAN,C_TURRET,C_DRONE,C_DEFENSE_DRONE,C_SWARM,C_MINE};
  for(unsigned k=0;k<8;k++){unsigned kind=order[k];if(count[kind]<wanted[kind]){if(spawnConstruct(kind)>=0)break;}}
 }
 if(m[M_MINEFIELD]&&frame%30==0&&count[C_MINE]<wanted[C_MINE])spawnConstruct(C_MINE);
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++){
  Construct &c=constructs[i];if(!c.active)continue;++c.age;if(c.fire_delay)--c.fire_delay;
  if(c.life)--c.life;
  bool expired=(c.kind==C_TURRET||c.kind==C_MINE||c.kind==C_BLADE||(c.kind==C_SWARM&&player.body!=B_CARRIER))&&c.life==0;
  if(c.hull<=0||expired){
   unsigned kind=c.kind;int32_t x=c.x,y=c.y;c.active=0;
   if(kind==C_MINE){blast(x,y,30*stats.blast/100,42*constructPower(kind)/100);
    for(unsigned j=0;j<5;j++){int n=emitBullet(x,y,(uint8_t)(j*256/5),2,12*constructPower(kind)/100,SHOT_SHRAPNEL,12);if(n>=0)bullets[n].blast=9;}
    if(m[M_X_CHARGED_MINES]){unsigned count=(unsigned)minv(16,weapons[player.weapon].projectiles+stats.projectiles);for(unsigned j=0;j<count;j++){int n=emitBullet(x,y,(uint8_t)(c.angle+((int)j*2-(int)count+1)*8),maxv(3,weapons[player.weapon].speed),weapons[player.weapon].damage*(100+minv(150,c.age))/100,SHOT_BLASTER,50);if(n>=0)bullets[n].blast=15;}}}
   else if(m[M_SELF_DESTRUCTION])blast(x,y,23,35*stats.construct_damage/100);
   if((kind==C_DRONE||kind==C_DEFENSE_DRONE)&&m[M_X_PREDATOR_DRONES]){int target=nearestEnemy(x,y);if(target>=0){int n=emitBullet(x,y,direction(delta(enemies[target].x,x,worldWidth()),delta(enemies[target].y,y,worldHeight())),5,50*constructPower(kind)/100,SHOT_MISSILE,70);if(n>=0)bullets[n].homing=8;}}
   if((kind==C_DEFENSE_DRONE&&m[M_X_FLOTILLA])||(kind==C_GUARDIAN&&m[M_X_NANOTECH_DISSEMINATION]))emitPickup(x,y,PICKUP_HEAL,kind==C_GUARDIAN?12:6);
   continue;
  }
  int target=nearestEnemy(c.x,c.y,c.kind==C_TURRET?140:220);
  if(c.kind==C_MINE){
   int drag=m[M_X_MORTAR]?250:220;c.vx=c.vx*drag/256;c.vy=c.vy*drag/256;c.x=wrap(c.x+c.vx,worldWidth());c.y=wrap(c.y+c.vy,worldHeight());
   if(c.age>15&&target>=0&&distance2(c.x,c.y,enemies[target].x,enemies[target].y)<24*24){
    if(m[M_X_TACTICAL_BEACON]){if(!c.fire_delay&&c.charges){blast(c.x,c.y,45,70*constructPower(C_MINE)/100);c.fire_delay=30;if(!--c.charges)c.active=0;}continue;}
    c.life=1;
    if(m[M_LOADED_MINES])for(unsigned j=0;j<8;j++)emitBullet(c.x,c.y,(uint8_t)(j*32),5,18*stats.damage/100,SHOT_BLASTER,35);
   }
   continue;
  }
  if(frame%30==0){
   bool ally=c.kind==C_ALLY||c.kind==C_INTERCEPTOR||c.kind==C_GUARDIAN;
   int heal=(ally&&m[M_X_NANOTECH]?2:0)+(m[M_X_VITAL_BOND]?stats.regen:0);
   c.hull=minv(c.max_hull,c.hull+heal);
   if(m[M_X_BURNOUT_REACTORS])c.hull-=maxv(1,c.max_hull/30);
   if(c.kind==C_GUARDIAN&&m[M_X_MEDI_CHARGE]&&frame%180==0)emitPickup(c.x,c.y,PICKUP_HEAL,6);
  }
  if(c.kind==C_DRONE||c.kind==C_DEFENSE_DRONE||c.kind==C_SWARM||c.kind==C_GUARDIAN){
   uint8_t orbit=(uint8_t)(frame*(c.kind==C_SWARM?2:1)+i*37);int radius=c.kind==C_SWARM?27:22;
   int32_t tx=wrap(player.x+dirX(orbit)*radius,worldWidth()),ty=wrap(player.y+dirY(orbit)*radius,worldHeight());
   if(c.kind==C_DRONE&&m[M_X_BATTALION_DRONES]){int side=(i&1)?1:-1;int offset=15+(int)(i/2)*8;tx=wrap(player.x+dirX((uint8_t)(player.angle+side*64))*offset,worldWidth());ty=wrap(player.y+dirY((uint8_t)(player.angle+side*64))*offset,worldHeight());}
   c.x=wrap(c.x+delta(tx,c.x,worldWidth())*FP/3,worldWidth());c.y=wrap(c.y+delta(ty,c.y,worldHeight())*FP/3,worldHeight());
  } else if(c.kind==C_ALLY||c.kind==C_INTERCEPTOR||c.kind==C_BLADE){
   int32_t tx=player.x,ty=player.y;
   if(target>=0&&(c.kind!=C_BLADE||c.age<85)){tx=enemies[target].x;ty=enemies[target].y;}
   int dx=delta(tx,c.x,worldWidth()),dy=delta(ty,c.y,worldHeight());c.angle=toward(c.angle,direction(dx,dy),c.kind==C_BLADE?7:4);
   int speed=c.kind==C_INTERCEPTOR||c.kind==C_BLADE?3:2;
   if(approx(dx,dy)>35||c.kind==C_BLADE){c.vx=dirX(c.angle)*speed;c.vy=dirY(c.angle)*speed;}else{c.vx=c.vx*240/256;c.vy=c.vy*240/256;}
   c.x=wrap(c.x+c.vx,worldWidth());c.y=wrap(c.y+c.vy,worldHeight());
   if(c.kind==C_BLADE){if(c.age>85&&distance2(c.x,c.y,player.x,player.y)<12*12)c.life=1;
    if(frame%6==0)for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active&&distance2(c.x,c.y,enemies[e].x,enemies[e].y)<(enemies[e].radius+6)*(enemies[e].radius+6))hitEnemy(e,22*stats.damage/100*stats.construct_damage/100);}
  }
  bool defender=c.kind==C_DEFENSE_DRONE||c.kind==C_GUARDIAN||(c.kind==C_TURRET&&m[M_POINT_DEFENSE]);
  if(defender){
   for(unsigned b=0;b<MAX_BULLETS;b++)if(bullets[b].active&&bullets[b].owner&&distance2(c.x,c.y,bullets[b].x,bullets[b].y)<20*20){
    bullets[b].active=0;if(c.kind==C_TURRET&&m[M_COUNTER_ARTILLERY])blast(c.x,c.y,22,18);break;
   }
   if(c.kind!=C_TURRET&&frame%90==i%90)player.hull=minv(player.max_hull,player.hull+1);
  }
  bool recovering=c.kind==C_SWARM&&player.shield>0&&player.shield<player.max_shield;
  if(recovering){if((frame+i*7)%120==0)player.shield=minv(player.max_shield,player.shield+1);}
  else if(target>=0&&!c.fire_delay){
   c.angle=direction(delta(enemies[target].x,c.x,worldWidth()),delta(enemies[target].y,c.y,worldHeight()));
   if(c.kind==C_DRONE&&m[M_X_BATTALION_DRONES])c.angle=player.angle;
   int damage=c.kind==C_TURRET?10:c.kind==C_ALLY||c.kind==C_INTERCEPTOR?14:c.kind==C_SWARM?4:8;
   unsigned shots=c.kind==C_ALLY||c.kind==C_INTERCEPTOR?3:1,kind=SHOT_BLASTER;
   if(c.kind==C_TURRET&&m[M_WAR_MACHINE]){damage=22;kind=SHOT_MISSILE;}
   if(c.kind==C_DRONE&&m[M_ASSAULT_DRONES]){damage=12;kind=SHOT_MISSILE;}
   if(c.promoted)damage*=2;
   if(c.kind==C_INTERCEPTOR&&m[M_X_OVERPOWER]){damage=weapons[player.weapon].damage;shots=(unsigned)minv(8,weapons[player.weapon].projectiles+stats.projectiles);kind=player.weapon==W_RAILGUN?SHOT_RAIL:player.weapon==W_DART?SHOT_DART:player.weapon==W_GRENADE?SHOT_GRENADE:player.weapon==W_SALVO?SHOT_MISSILE:SHOT_BLASTER;}
   for(unsigned j=0;j<shots;j++){int n=emitBullet(c.x,c.y,(uint8_t)(c.angle+((int)j*2-(int)shots+1)*5),4,damage*constructPower(c.kind)/100,kind,45);
    if(n>=0&&(c.kind==C_ALLY||c.kind==C_INTERCEPTOR))bullets[n].blast=m[M_X_ECHO_STRIKE]?20:8;
   }
   c.fire_delay=(uint16_t)(c.kind==C_TURRET?10:c.kind==C_INTERCEPTOR?16:c.kind==C_SWARM?30:24);
   if(m[M_OVERCLOCK])c.fire_delay=c.fire_delay*70/100;
   if(c.kind==C_DRONE&&m[M_ASSAULT_DRONES])c.fire_delay=c.fire_delay*70/100;
   if(c.kind==C_TURRET&&m[M_COUNTER_ARTILLERY])c.fire_delay=c.fire_delay*65/100;
   if(c.kind==C_SWARM&&player.shield==0)c.fire_delay/=2;
   if(m[M_X_BURNOUT_REACTORS])c.fire_delay=(uint16_t)maxv(2,c.fire_delay*(50+c.hull*50/maxv(1,c.max_hull))/100);
  }
  if(c.kind==C_TURRET&&frame%30==0)--c.hull;
  if(frame%6==0)for(unsigned e=0;e<MAX_ENEMIES;e++)if(enemies[e].active&&distance2(c.x,c.y,enemies[e].x,enemies[e].y)<(c.radius+enemies[e].radius)*(c.radius+enemies[e].radius)){
   if(c.kind!=C_BLADE)c.hull-=2;
   break;
  }
 }
}
void Game::updatePickups() {
 for(unsigned i=0;i<MAX_PICKUPS;i++){
  Pickup &p=pickups[i];if(!p.active)continue;++p.age;
  int dx=delta(player.x,p.x,worldWidth()),dy=delta(player.y,p.y,worldHeight()),distance=approx(dx,dy);
  int magnet=player.body==B_RESEARCH?75:38;
  if(distance<magnet||p.age>900){uint8_t a=direction(dx,dy);p.vx=p.vx*3/4+dirX(a);p.vy=p.vy*3/4+dirY(a);}
  else {p.vx=p.vx*245/256;p.vy=p.vy*245/256;}
  p.x=wrap(p.x+p.vx,worldWidth());p.y=wrap(p.y+p.vy,worldHeight());
  if(distance2(p.x,p.y,player.x,player.y)<(player.radius+5)*(player.radius+5)){
   p.active=0;
   if(p.kind==PICKUP_XP)xp+=p.value*(last_stand_used_?2:1);
   else if(p.kind==PICKUP_HEAL)player.hull=minv(player.max_hull,player.hull+p.value);
   else if(p.kind==PICKUP_REROLL&&!player.mods[M_X_POWER_SPIKE])rerolls=(uint16_t)minv(99,rerolls+p.value);
   else {kill_buff_=300;player.shield=minv(player.max_shield,player.shield+20);}
  }
 }
}
uint32_t Game::checksum() const {
 // Hash semantic simulation fields, avoiding compiler padding and the input
 // accumulator, which legitimately differs between rendering schedules.
 uint32_t hash=2166136261u;
#define HASH(v) do {hash^=(uint32_t)(v);hash*=16777619u;} while(0)
 HASH(state);HASH(frame);HASH(random_);HASH(score);HASH(kills);HASH(xp);HASH(level);HASH(wave);
 HASH(player.x);HASH(player.y);HASH(player.vx);HASH(player.vy);HASH(player.hull);HASH(player.shield);HASH(player.angle);
 HASH(player.weapon);HASH(player.body);HASH(player.shield_type);HASH(player.charge);HASH(player.missiles);
 for(unsigned i=0;i<MOD_COUNT;i++)HASH(player.mods[i]);
 for(unsigned i=0;i<MAX_ENEMIES;i++){const Enemy &e=enemies[i];HASH(e.active);if(e.active){HASH(e.x);HASH(e.y);HASH(e.hull);HASH(e.type);HASH(e.angle);HASH(e.age);}}
 for(unsigned i=0;i<MAX_BULLETS;i++){const Bullet &b=bullets[i];HASH(b.active);if(b.active){HASH(b.x);HASH(b.y);HASH(b.damage);HASH(b.life);HASH(b.kind);}}
 for(unsigned i=0;i<MAX_PICKUPS;i++){const Pickup &p=pickups[i];HASH(p.active);if(p.active){HASH(p.x);HASH(p.y);HASH(p.value);}}
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++){const Construct &c=constructs[i];HASH(c.active);if(c.active){HASH(c.x);HASH(c.y);HASH(c.kind);HASH(c.hull);}}
#undef HASH
 return hash;
}
} // namespace nova
