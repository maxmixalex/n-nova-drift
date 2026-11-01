// Original SwordObject / ShieldBastion behaviours, with fixed controller pools.
// Source coordinates are mapped at 1/6 and two source frames per engine tick.
namespace {
unsigned advancedCap(uint64_t n,unsigned cap){return n>cap?cap:(unsigned)n;}
unsigned advancedRoot(uint64_t n){uint64_t bit=uint64_t(1)<<62,result=0;while(bit>n)bit>>=2;while(bit){if(n>=result+bit){n-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return (unsigned)result;}
unsigned advancedSmooth(unsigned x){x=minv(1024u,x);return x*x*(3072-2*x)/1048576;}
unsigned advancedPowerEighth(unsigned n){
 // n^1.125 = n * eighth-root(n); three integer square roots retain Q12.
 uint64_t root=advancedRoot((uint64_t)n<<24);
 root=advancedRoot(root<<12);root=advancedRoot(root<<12);
 return advancedCap((uint64_t)n*root/4096,2000000000);
}
}
bool Game::advancedConstructActive(unsigned kind) const {
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(constructs[i].active&&constructs[i].hull>0&&constructs[i].kind==kind)return true;
 return false;
}
unsigned Game::bastionResistancePercent() const {
 if(player.shield_type!=S_BASTION||!player.shield_energy||!advancedConstructActive(C_BASTION))return 0;
 unsigned effect=activeShieldEffect();
 // Original intensity=sqrt(effect*storedEnergy); reduction=.009*effect*intensity.
 unsigned intensity=advancedRoot((uint64_t)player.shield_energy*effect*100);
 return advancedCap((uint64_t)9*effect*intensity/100000,50);
}
void Game::attackSwords(int damage,unsigned reach,uint8_t angle,bool temporary) {
 unsigned rate=(unsigned)maxv(10,stats.fire_rate)*constructFireRatePercent(C_SWORD)/100;
 unsigned duration=(unsigned)clamp(4000/(int)minv(200u,rate),1,200);
 unsigned crash=crashContactPercent();
 unsigned crash_factor=100+(maxv(100u,crash)-100)/4;
 unsigned speed=(unsigned)approx(player.vx,player.vy)*3; // source pixels/frame
 uint64_t scaled=(uint64_t)maxv(1,damage)*maxv(1,constructPower(C_SWORD))*crash_factor/10000;
 scaled=scaled*(8000+speed*100/FP)/8000;
 if(rate>200)scaled=scaled*rate/200;
 if(temporary){
  // Loaded Mines own an independent temporary pair. No player sword is moved,
  // consumed or reset to make the mine attack.
  for(unsigned side=0;side<2;++side){int slot=spawnConstruct(C_SWORD);if(slot<0)break;
   AdvancedConstructState &a=advanced_constructs[slot];clearBytes(&a,sizeof(a));a.temporary=1;a.side=(uint8_t)side;
   a.anchor_x=player.x;a.anchor_y=player.y;a.heading=angle;a.duration=(uint16_t)duration;
   a.state=1;a.damage=(int32_t)advancedCap(scaled,2000000);constructs[slot].fire_delay=0;constructs[slot].chain_percent=(uint16_t)clamp(reach,8,100);
  }
  return;
 }
 unsigned ordinal=0;
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i){Construct &c=constructs[i];AdvancedConstructState &a=advanced_constructs[i];
  if(!c.active||c.hull<=0||c.kind!=C_SWORD||a.temporary||a.wielder)continue;
  if(a.state){++ordinal;continue;}
  clearBytes(a.hit_mask,sizeof(a.hit_mask));a.side=(uint8_t)(ordinal&1);a.heading=angle;
  a.clock=0;a.duration=(uint16_t)duration;a.damage=(int32_t)advancedCap(scaled,2000000);
  // chooseAttackMode selects a thrust for fast, nearly straight flight.
  // Source threshold is13*acceleration and turn momentum under2 degrees.
  bool straight=!previous_.left&&!previous_.right;
  a.state=(uint8_t)(straight&&speed>(unsigned)maxv(1,13*FP*stats.thrust/100)?2:1);
  a.anchor_x=player.x;a.anchor_y=player.y;c.chain_percent=(uint16_t)clamp(reach,8,100);
  c.fire_delay=(uint16_t)((ordinal/2)*2+(ordinal&1));++ordinal;
 }
}
void Game::releaseBastionEnergy(unsigned actual) {
 if(!actual||player.shield_type!=S_BASTION||!player.shield_energy)return;
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(constructs[i].active&&constructs[i].kind==C_BASTION&&advanced_constructs[i].state)return;
 unsigned effect=activeShieldEffect();unsigned intensity=advancedRoot((uint64_t)player.shield_energy*effect*100);
 unsigned scaler=advancedCap((uint64_t)intensity*maxv(1,constructPower(C_BASTION))*globalBlastRadiusPercent()/20000,1000000);
 unsigned damage=advancedCap((uint64_t)3*scaler*(player.mods[M_CONCENTRATED_BLAST]?135:100)/100,2000000);
 int32_t x=player.mods[M_X_SANCTUARY]?sanctuary_x:player.x,y=player.mods[M_X_SANCTUARY]?sanctuary_y:player.y;
 unsigned slot=MAX_PERSISTENT_FIELDS;
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;++i)if(!persistent_fields[i].active){slot=i;break;}
 if(slot==MAX_PERSISTENT_FIELDS){slot=0;for(unsigned i=1;i<MAX_PERSISTENT_FIELDS;++i)if(persistent_fields[i].source_age>persistent_fields[slot].source_age)slot=i;}
 Construct origin={};origin.x=x;origin.y=y;triggerBurnout(origin,(int)damage,1);
 PersistentField &wave=persistent_fields[slot];wave.reserved[0]=2;wave.reserved[1]=(uint8_t)minv(255,scaler*255/1000);wave.burn=0;wave.angle=player.angle;wave.radius=26;wave.opacity=wave.reserved[1];
 // Native .4 initial image scale, +.025*scaler*shield radius each frame.
 // 388 is the original canvas half-height, mapped to the same 1/6 scale.
 wave.base_radius=(uint16_t)maxv(1,advancedCap((uint64_t)scaler*388*activeShieldRadius()/(240*maxv(1,shields[S_BASTION].radius)),65535));
 // The source wave drains stored energy at5/source frame rather than
 // destroying the shield or arbitrarily limiting storage to200 damage.
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i)if(constructs[i].active&&constructs[i].kind==C_BASTION)advanced_constructs[i].state=1;
}
void Game::updateAdvancedConstruct(unsigned index) {
 Construct &c=constructs[index];AdvancedConstructState &a=advanced_constructs[index];
 if(c.kind==C_BASTION){
  if(player.shield_type!=S_BASTION||!constructLimit(C_BASTION)){c.active=0;return;}
  c.max_hull=player.max_shield;c.hull=player.shield;
  if(c.hull<=0){c.hull=0;return;}
  if(a.state){player.shield_energy=(uint16_t)maxv(0,player.shield_energy-10);if(!player.shield_energy)a.state=0;}
  unsigned effect=activeShieldEffect();
  // .00027 of maximum shield per source frame, scaled by shield effect.
  c.regen_fraction+=(unsigned)c.max_hull*54*effect;
  unsigned heal=c.regen_fraction/10000000;c.regen_fraction%=10000000;
  if(heal){player.shield=(int16_t)minv(shieldRecoveryCap(),player.shield+heal);c.hull=player.shield;}
  int32_t origin_x=player.mods[M_X_SANCTUARY]?sanctuary_x:player.x,origin_y=player.mods[M_X_SANCTUARY]?sanctuary_y:player.y;
  c.angle=player.angle;c.radius=(uint8_t)clamp((int)activeShieldRadius()*10/shields[S_BASTION].radius,5,60);
  // Original baseFollowDist85 source pixels. Bonus size has half influence on distance.
  int distance=85*FP/6+25*FP*35/600+(int(c.radius)-10)*FP/2;
  c.x=wrap(origin_x+dirX(c.angle)*distance/FP,worldWidth());c.y=wrap(origin_y+dirY(c.angle)*distance/FP,worldHeight());
  return;
 }
 if(c.kind!=C_SWORD)return;
 if(!a.temporary&&player.weapon!=W_SWORDS){c.active=0;return;}
 const Construct *wielder=nullptr;
 if(a.wielder){unsigned owner=a.wielder-1;
  if(owner>=MAX_CONSTRUCTS||!constructs[owner].active||constructs[owner].hull<=0||constructs[owner].kind!=C_INTERCEPTOR ){c.active=0;return;}
  wielder=&constructs[owner];
 }
 int32_t anchor_x=wielder?wielder->x:a.temporary?a.anchor_x:player.x,anchor_y=wielder?wielder->y:a.temporary?a.anchor_y:player.y;
 uint8_t base=a.state?a.heading:wielder?wielder->angle:player.angle;int side=a.side?1:-1;
 int offset_forward=-5,offset_side=side*12,angular=0;
 if(a.state&&!c.fire_delay){
  unsigned elapsed=minv((unsigned)a.duration,(unsigned)a.clock+1);
  unsigned progress=elapsed*1024/maxv(1,a.duration),smooth=advancedSmooth(progress);
  if(a.state==2){
   // Source thrust650, six attack frames plus4 lag/1 recovery, normalised by ROF.
   unsigned travel=progress<558?advancedSmooth(progress*1024/558):1024-advancedSmooth((progress-558)*1024/466);
   offset_forward+=(int)(travel*minv(108u,(unsigned)c.chain_percent*4)/1024);angular=0;
  }else{
   unsigned envelope=progress<307?advancedSmooth(progress*1024/307):progress>717?advancedSmooth((1024-progress)*1024/307):1024;
   unsigned swing=advancedSmooth(smooth);
   angular=side*(70-(int)(swing*140/1024));
   int reach=175*(int)c.chain_percent/(6*25);
   offset_forward+=dirX((uint8_t)angular)*reach*(int)envelope/(FP*1024);
   offset_side+=dirY((uint8_t)angular)*reach*(int)envelope/(FP*1024);
  }
  ++a.clock;
 }
 int co=dirX(base),sn=dirY(base);int32_t old_x=c.x,old_y=c.y;
 c.x=wrap(anchor_x+(offset_forward*co-offset_side*sn),worldWidth());c.y=wrap(anchor_y+(offset_forward*sn+offset_side*co),worldHeight());
 c.angle=(uint8_t)(base+angular);c.radius=(uint8_t)clamp(3*stats.size/100,2,15);
 if(a.state&&!c.fire_delay){
  // Original double-edged SwordObject carries its own hitList until recovery.
  int half=(int)clamp(23*stats.size/100,5,55);
  int32_t tip_x=wrap(c.x+dirX(c.angle)*half,worldWidth()),tip_y=wrap(c.y+dirY(c.angle)*half,worldHeight());
  int32_t back_x=wrap(c.x-dirX(c.angle)*half/3,worldWidth()),back_y=wrap(c.y-dirY(c.angle)*half/3,worldHeight());
  for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS;++target){
   uint64_t bit=uint64_t(1)<<(target%64);if(a.hit_mask[target/64]&bit)continue;
   if(!targetSwept((int)target,back_x,back_y,tip_x,tip_y,c.radius)&&!targetSwept((int)target,old_x,old_y,c.x,c.y,c.radius))continue;
   a.hit_mask[target/64]|=bit;damageTarget((int)target,a.damage*(a.state==2?2:1));
  }
  if(a.clock>=a.duration){a.state=0;a.clock=0;if(a.temporary)c.hull=0;}
 }
}
bool Game::interceptAdvancedProjectile(unsigned bullet,int32_t ox,int32_t oy) {
 if(bullet>=MAX_BULLETS||!bullets[bullet].active||!bullets[bullet].owner)return false;
 Bullet &b=bullets[bullet];
 for(unsigned i=0;i<MAX_CONSTRUCTS;++i){Construct &c=constructs[i];if(!c.active||c.hull<=0)continue;
  AdvancedConstructState &a=advanced_constructs[i];
  if(c.kind==C_BASTION){
   bool hit=false;int32_t center_x=c.x-dirX(c.angle)*c.radius,center_y=c.y-dirY(c.angle)*c.radius;
   // A physical open arc, sampled more finely than its source bitmap pixels
   // after downscaling. Rear and transparent centre remain uncovered.
   for(int j=-6;j<=6&&!hit;++j){uint8_t angle=(uint8_t)(c.angle+j*7);int32_t x=wrap(center_x+dirX(angle)*c.radius,worldWidth()),y=wrap(center_y+dirY(angle)*c.radius,worldHeight());
    hit=sweptHit(ox,oy,b.x,b.y,x,y,2+b.radius);
   }
   if(hit){int damage=b.damage;destroyHostileBullet(bullet);damageConstruct(i,damage);return true;}
  }else if(c.kind==C_SWORD&&a.state&&!a.temporary&&!c.fire_delay){
   int half=(int)clamp(23*stats.size/100,5,55);bool hit=false;
   for(int j=-1;j<=3&&!hit;++j)hit=sweptHit(ox,oy,b.x,b.y,wrap(c.x+dirX(c.angle)*half*j/3,worldWidth()),wrap(c.y+dirY(c.angle)*half*j/3,worldHeight()),c.radius+b.radius);
   if(!hit)continue;
   unsigned power=advancedPowerEighth((unsigned)maxv(1,b.damage));
   unsigned count=(unsigned)clamp((int)(((uint64_t)power*maxv(1,weapons[W_SWORDS].projectiles+stats.projectiles)+25)/50),1,48);
   unsigned damage=(unsigned)maxv(1,a.damage/2);int32_t x=b.x,y=b.y;destroyHostileBullet(bullet);
   if(daily(DAILY_SWORD))return true;
   for(unsigned j=0;j<count;++j){int spread=((int)j*2-(int)count+1)*minv(32u,15+5*count)/(int)maxv(1u,count);
    int at=emitBullet(x,y,(uint8_t)(a.heading+spread),6,(int)damage,SHOT_SPARK,40,0,2);if(at>=0)bullets[at].homing=2;
   }
   return true;
  }
 }
 return false;
}

void Game::updateSwordWielder(unsigned index,int target) {
 if(index>=MAX_CONSTRUCTS)return;
 Construct &w=constructs[index];
 bool enabled=w.active&&w.hull>0&&w.kind==C_INTERCEPTOR&&player.weapon==W_SWORDS&&!player.mods[M_X_HIDDEN_POWER]&&!daily(DAILY_SWORD);
 unsigned count=0;
 for(unsigned j=0;j<MAX_CONSTRUCTS;++j)if(constructs[j].active&&constructs[j].kind==C_SWORD&&advanced_constructs[j].wielder==index+1){
  if(!enabled)constructs[j].active=0;else if(constructs[j].hull>0)++count;
 }
 if(!enabled)return;
 // Register a permanent controller per living wielder. Its blades assemble
 // independently, and never borrow or advance the player's existing pair.
 unsigned interval=(unsigned)maxv(1,12000/maxv(10,stats.assembly));
 if(count<2){
  w.weapon_charge=minv(60000,w.weapon_charge+1);
  if(w.weapon_charge>=interval){
   for(unsigned side=0;side<2;++side){bool present=false;
    for(unsigned j=0;j<MAX_CONSTRUCTS;++j)if(constructs[j].active&&constructs[j].hull>0&&constructs[j].kind==C_SWORD&&advanced_constructs[j].wielder==index+1&&advanced_constructs[j].side==side)present=true;
    if(present)continue;
    int at=spawnConstruct(C_SWORD);if(at<0)break;
    Construct &s=constructs[at];AdvancedConstructState &a=advanced_constructs[at];a.wielder=(uint8_t)(index+1);a.side=(uint8_t)side;a.heading=w.angle;s.x=w.x;s.y=w.y;s.angle=w.angle;s.fire_delay=0;s.chain_percent=25;++count;
   }
   if(count>=2)w.weapon_charge=0;
  }
 }else w.weapon_charge=0;
 if(target<0||w.fire_delay)return;
 unsigned rate=(unsigned)maxv(10,stats.fire_rate)*constructFireRatePercent(C_INTERCEPTOR)*10/(player.mods[M_X_OVERPOWER]?2400:3200);
 unsigned duration=(unsigned)clamp(4000/(int)minv(200u,rate),1,200);
 uint64_t damage=(uint64_t)weaponDamage()*maxv(1,constructPower(C_INTERCEPTOR))/100;
 unsigned crash=crashContactPercent();damage=damage*(100+(maxv(100u,crash)-100)/4)/100;
 unsigned speed=(unsigned)approx(w.vx,w.vy)*3;
 damage=damage*(8000+speed*100/FP)/8000;if(rate>200)damage=damage*rate/200;
 bool attacking=false;
 for(unsigned j=0;j<MAX_CONSTRUCTS;++j){Construct &s=constructs[j];AdvancedConstructState &a=advanced_constructs[j];
  if(!s.active||s.hull<=0||s.kind!=C_SWORD||a.wielder!=index+1||a.state)continue;
  clearBytes(a.hit_mask,sizeof(a.hit_mask));a.clock=0;a.duration=(uint16_t)duration;a.heading=w.angle;a.state=1;a.damage=(int32_t)advancedCap(damage,2000000);a.anchor_x=w.x;a.anchor_y=w.y;
  s.fire_delay=a.side;s.chain_percent=25;attacking=true;
 }
 if(attacking)w.fire_delay=(uint16_t)duration;
}
