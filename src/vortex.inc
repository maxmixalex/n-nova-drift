// Source-backed charge, cost, decay and attraction. Values use millionths;
// the two substeps preserve 60 Hz source timing in the 30 Hz simulation.
namespace {
const uint32_t VORTEX_SCALE_LIMIT=64000000;
uint64_t vortexSqrt64(uint64_t n) {
 uint64_t r=0,bit=uint64_t(1)<<62;
 while(bit>n)bit>>=2;
 while(bit){if(n>=r+bit){n-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}
 return r;
}
uint32_t vortexSelfCostMicro(uint32_t scale) {
 // max(0,.0085*(1+C)^4.5-.05) HP per original frame.
 // Q16 exponentiation avoids libm and bounds every multiplication below 2^63.
 uint64_t x=65536+(uint64_t)minv(VORTEX_SCALE_LIMIT,scale)*65536/1000000;
 uint64_t p=x*x/65536;p=p*p/65536;
 p=p*vortexSqrt64(x*65536)/65536;
 uint64_t value=p*8500/65536;
 return value<=50000?0u:(uint32_t)(value-50000>2000000000ull?2000000000ull:value-50000);
}
void vortexChargeStep(uint32_t &scale,uint32_t &rate,uint32_t &fraction,unsigned time_permille,unsigned modifiers_permille) {
 if(!rate)rate=12375000; // rate is nanounits; target .00075/source frame.
 unsigned lerp=(1000+modifiers_permille)/2;
 uint64_t increment=(uint64_t)rate*time_permille*lerp/1000+fraction;
 uint64_t next=(uint64_t)scale+increment/1000000;
 fraction=(uint32_t)(increment%1000000);scale=(uint32_t)(next>VORTEX_SCALE_LIMIT?VORTEX_SCALE_LIMIT:next);
 if(rate>750000)rate-=(rate-750000+25)/50;
 else if(rate<750000)rate+=(750000-rate+25)/50;
}
unsigned vortexResistance(const Enemy &e) {
 if(e.type==E_STAR_EATER)return 0; // source segment / appendage res is zero.
 if(e.type==E_ASTEROID)return 1000-(unsigned)clamp(e.max_hull*10,50,900);
 if(e.type==E_JUGGERNAUT)return e.elite>=2?100:e.elite?125:150;
 if(e.type==E_WORM)return e.elite>=2?300:e.elite?400:500;
 if(e.type==E_ECLIPSE)return 0;
 unsigned value=1000u-(unsigned)clamp(e.max_hull,0,800);
 if(e.type==E_MYRMIDON)value=value*(1400-150*minv(2,e.elite))/1000;
 if(e.type==E_PULSAR)value=value*125/100;
 return value;
}
}
unsigned Game::storedChargeMicro() const {return charged_scale_+(player.mods[M_X_RANCOR]?rancor_scale_:0);}
void Game::recordRancorDamage(unsigned milli) {
 if(!player.mods[M_X_RANCOR]||!player.firing||!milli)return;
 unsigned coefficient=0,denominator=1200000;
 if(player.weapon==W_VORTEX){coefficient=5500;denominator=maxv(1200000,vortex_scale_);}
 else if(player.weapon==W_THERMAL_LANCE){coefficient=17500;denominator=maxv(1200000,thermal_x_scale_);}
 else if(player.mods[M_CHARGED_SHOT]&&!player.mods[M_X_CHARGED_MINES]){coefficient=17500;denominator=maxv(1200000,charged_scale_);}
 if(!coefficient)return;
 uint64_t next=(uint64_t)rancor_scale_+(uint64_t)milli*coefficient*1000/denominator;
 rancor_scale_=(uint32_t)(next>VORTEX_SCALE_LIMIT?VORTEX_SCALE_LIMIT:next);
}
void Game::updateRancor() {
 if(!player.mods[M_X_RANCOR]){rancor_scale_=0;return;}
 const unsigned falloff=player.weapon==W_VORTEX?667:1667;
 for(unsigned sub=0;sub<2;sub++){uint32_t value=(uint32_t)((uint64_t)rancor_scale_*999/1000);rancor_scale_=value>falloff?value-falloff:0;}
}
unsigned Game::vortexChargeModifier(bool construct_origin) const {
 uint64_t value=(uint64_t)maxv(1,stats.size)*maxv(1,stats.blast)*maxv(1,stats.fire_rate)/1000;
 if(player.mods[M_CHARGED_SHOT]&&(!player.mods[M_X_CHARGED_MINES]||construct_origin))value=value*120/100;
 if(hasHyperboost())value=value*4/3;
 if(kill_buff_&&player.mods[M_WARPATH])value=value*125/100;
 return (unsigned)(value>1000000?1000000:value);
}
int32_t Game::vortexChargeX() const {unsigned offset=853+(uint64_t)(vortex_scale_+rancor_scale_)*3200/1000000;return wrap(player.x+(int64_t)dirX(player.angle)*offset/FP,worldWidth());}
int32_t Game::vortexChargeY() const {unsigned offset=853+(uint64_t)(vortex_scale_+rancor_scale_)*3200/1000000;return wrap(player.y+(int64_t)dirY(player.angle)*offset/FP,worldHeight());}
void Game::applyVortexGravity(int32_t x,int32_t y,unsigned scale,unsigned released_count) {
 const unsigned count=maxv(1,released_count);
 int32_t moved_x[MAX_ENEMIES]={0},moved_y[MAX_ENEMIES]={0};
 // Native with(oSolid) uses point_distance, without a range cutoff or wrapping.
 // A multipart owner is translated once as a group in this bounded port.
 for(unsigned i=0;i<MAX_ENEMIES;i++){
  Enemy &e=enemies[i];if(!e.active||e.boss_hidden)continue;
  unsigned resistance=vortexResistance(e);if(!resistance)continue;
  int32_t dx=x-e.x,dy=y-e.y;uint64_t distance=vortexSqrt64((int64_t)dx*dx+(int64_t)dy*dy);if(!distance)continue;
  // (41*C)^2 * res / source_distance, then cap 10 source pixels.
  uint64_t square=(uint64_t)scale*scale/1000000;
  uint64_t pre=square*1681*resistance/(distance*count);
  uint64_t source_q8=pre>=234375000?10*FP:pre*65536/6000000000ull;
  if(source_q8>10*FP)source_q8=10*FP;
  int32_t step=(int32_t)(source_q8/6);if(player.mods[M_X_POLAR_INVERSION])step=-step;
  int32_t move_x=(int32_t)((int64_t)dx*step/(int64_t)distance),move_y=(int32_t)((int64_t)dy*step/(int64_t)distance);
  int32_t nx=clamp(e.x+move_x,0,worldWidth()*FP-1),ny=clamp(e.y+move_y,0,worldHeight()*FP-1);move_x=nx-e.x;move_y=ny-e.y;e.x=nx;e.y=ny;
  moved_x[i]=move_x;moved_y[i]=move_y;
 }
 // Translate attachments in one linear pass, avoiding enemies*parts work for
 // every overlapping field. Owner displacement is independent of this pass.
 for(unsigned part=0;part<MAX_BOSS_PARTS;part++)if(boss_parts[part].active){BossPart &p=boss_parts[part];p.x+=moved_x[p.owner];p.y+=moved_y[p.owner];}
}
void Game::vortexField(int32_t x,int32_t y,unsigned scale,unsigned base_milli,uint16_t &fraction,unsigned released_count) {
 // Step damage=max(.2,baseDamage*C*(C+.5)), applied every source frame.
 uint64_t damage=(uint64_t)base_milli*scale/1000000*(scale+500000)/1000000;
 if(damage<200)damage=200;
 if(damage>2000000000ull)damage=2000000000ull;
 damage+=fraction;fraction=(uint16_t)(damage%1000);unsigned whole=(unsigned)(damage/1000);
 unsigned radius=(unsigned)minv(100,(uint64_t)scale*21/1000000);
 for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS;target++)if(targetInRadius((int)target,x,y,radius)){
  if(target<MAX_ENEMIES)enemies[target].wake_time=maxv(enemies[target].wake_time,igniteDurationTicks());
  else boss_parts[target-MAX_ENEMIES].wake_time=maxv(boss_parts[target-MAX_ENEMIES].wake_time,igniteDurationTicks());
  if(whole)damageTarget((int)target,(int)whole,true);
 }
 applyVortexGravity(x,y,scale,released_count);
}
void Game::updateVortexCharge() {
 Player &p=player;
 if(p.firing){
  if(!p.charge){vortex_scale_=100000;vortex_rate_=12375000;vortex_growth_fraction_=0;vortex_damage_fraction_=0;}
  p.charge=(uint16_t)minv(65535,p.charge+1);
  for(unsigned sub=0;sub<2&&p.hull>0;sub++){
   vortexChargeStep(vortex_scale_,vortex_rate_,vortex_growth_fraction_,1000,vortexChargeModifier());
   uint64_t cost=(uint64_t)vortexSelfCostMicro(vortex_scale_)+vortex_cost_fraction_;
   vortex_cost_fraction_=(uint16_t)(cost%1000);unsigned milli=(unsigned)(cost/1000);
   if(milli){
    if(p.mods[M_X_GEMINI]&&!p.mods[M_X_SHIELDED_DRONES]&&p.shield>0&&(uint64_t)p.shield*1000*incomingDamagePercent(true,true)/100>milli/2){damageShieldSelf(milli/2);damageHullSelf(milli-milli/2);}
    else damageHullSelf(milli);
   }
   unsigned scale=(unsigned)minv(VORTEX_SCALE_LIMIT,vortex_scale_+rancor_scale_);
   unsigned base=weaponDamage()*1000;if(p.shield>0&&p.shield_type==S_AMP)base=base*120/100;
   vortexField(vortexChargeX(),vortexChargeY(),scale,base,vortex_damage_fraction_,0);
  }
 } else {
  if(p.charge)releaseVortex();
  vortex_scale_=(uint32_t)((uint64_t)vortex_scale_*9025/10000);
 }
}
void Game::releaseVortex(unsigned projectile_override,bool construct_origin) {
 Player &p=player;if(!p.charge&&!construct_origin)return;
 unsigned scale=(unsigned)minv(VORTEX_SCALE_LIMIT,vortex_scale_+(construct_origin?0:rancor_scale_));
 unsigned count=projectile_override?projectile_override:weaponProjectiles();count=(unsigned)clamp(count,1,FRIENDLY_BULLETS);
 int damage=(int)weaponDamage()*1000;if(p.shield>0&&p.shield_type==S_AMP)damage=damage*120/100;
 if(p.body==B_SPECTRE)damage=(int)((int64_t)damage*(100+p.cloak*3*(int)bodyPowerPercent()/180)/100);
 int speed=maxv(1,weapons[W_VORTEX].speed*stats.velocity/100),spread=weaponSpreadStep();
 for(unsigned j=0;j<count;j++){
  uint8_t angle=(uint8_t)(p.angle+((int)j*2-(int)count+1)*spread/2);
  int slot=emitBullet(p.x,p.y,angle,speed,damage,SHOT_VORTEX,65535,0,(unsigned)minv(100,(uint64_t)scale*21/1000000));if(slot<0)continue;
  Bullet &b=bullets[slot];b.field_scale=scale;b.bounces=(uint8_t)minv(255,count+2*p.mods[M_BURST_FIRE]+p.mods[M_X_DOUBLE_TAP]);b.homing=(uint8_t)stats.homing;b.pierce=255;b.blast=0;b.wraps=255;b.size_percent=(uint16_t)minv(65535,scale/10000);
 }
 if(!construct_origin&&p.mods[M_HOMING_STRIKE])fireHomingStrike(p.angle,(int)count,speed,damage,65535,(int)minv(100,(uint64_t)scale*21/1000000));
 p.charge=0;vortex_rate_=12375000;vortex_growth_fraction_=0;vortex_cost_fraction_=0;
 if(!construct_origin){rancor_scale_=0;if(p.mods[M_DISCHARGE])triggerDischarge();}
 p.fire_delay=(uint16_t)weaponCooldownTicks();
}
void Game::updateVortexBullet(Bullet &b,unsigned released_count) {
 if(!b.field_scale){b.active=0;return;}
 if(b.homing&&!b.tracking){int target=nearestTarget(b.x,b.y);if(target>=0)b.angle=toward(b.angle,direction(targetX(target)-b.x,targetY(target)-b.y),b.homing);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;}
 for(unsigned sub=0;sub<2&&b.active;sub++){
  unsigned extra=(maxv(1,b.bounces)-1)*900;
  unsigned decay=(unsigned)clamp((int)(b.field_scale/500+extra),1575,(int)(7875+extra));
  b.field_scale=b.field_scale>decay?b.field_scale-decay:0;
  if(b.field_scale<=25000){b.active=0;break;}
  b.x=wrap(b.x+b.vx/2,worldWidth());b.y=wrap(b.y+b.vy/2,worldHeight());
  b.radius=(uint8_t)minv(100,(uint64_t)b.field_scale*21/1000000);b.size_percent=(uint16_t)minv(65535,b.field_scale/10000);
  vortexField(b.x,b.y,b.field_scale,(unsigned)maxv(0,b.damage),b.traveled,released_count);
 }
}
void Game::updateVortexMine(unsigned index,unsigned rate_permille,unsigned cost_multiplier) {
 Construct &c=constructs[index];if(!c.vortex_rate){c.vortex_scale=100000;c.vortex_rate=12375000;}
 for(unsigned sub=0;sub<2&&c.hull>0;sub++){
  vortexChargeStep(c.vortex_scale,c.vortex_rate,c.vortex_fraction,rate_permille,vortexChargeModifier(true));
  uint64_t cost=(uint64_t)vortexSelfCostMicro(c.vortex_scale)*cost_multiplier+c.vortex_cost_fraction;
  c.vortex_cost_fraction=(uint16_t)(cost%1000);unsigned milli=(unsigned)(cost/1000);
  unsigned total=milli+c.redirected_fraction;c.redirected_fraction=(uint16_t)(total%1000);if(total>=1000)damageConstruct(index,(int)(total/1000));
 }
 c.charge_ticks=(uint16_t)minv(65535,c.vortex_scale/1000);
}
