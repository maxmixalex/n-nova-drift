// Original source timing and formulas: docs/evidence/persistent-effects.csv.
// Included inside namespace nova after Game's other simulation methods.
namespace {
unsigned fieldResistance(const Enemy &e) {
 // enemyInit @0x1402840dd: 1-clamp(hullMax/1000,0,.8).
 unsigned value=1000u-(unsigned)minv(800,maxv(0,enemy_types[e.type].hull));
 if(e.type==E_STAR_EATER)value=25;
 else if(e.type==E_ECLIPSE)value=0;
 else if(e.type==E_JUGGERNAUT)value=e.elite>=2?100:e.elite?125:150;
 else if(e.type==E_WORM)value=e.elite>=2?300:e.elite?400:500;
 else if(e.type==E_MYRMIDON)value=value*(1400-150*minv(2,e.elite))/1000;
 else if(e.type==E_PULSAR)value=value*125/100;
 return value;
}
uint32_t fieldMulDiv(uint64_t a,uint32_t b,uint32_t denominator,uint32_t ceiling) {
 if(!b||!denominator)return 0;
 if(a>(uint64_t)ceiling*denominator/b)return ceiling;
 return (uint32_t)(a*b/denominator);
}
uint64_t fieldPowerSevenTenths(uint64_t value) {
 // Integer interpolation of x^.7, normalized to [1,2). Its table contains
 // mathematical constants, not game data. Maximum interpolation error <.01%.
 static const uint32_t mantissa[]={65536,66963,68377,69779,71168,72546,73913,75270,76616,77951,79277,80594,81901,83200,84490,85772,87045,88310,89568,90818,92061,93297,94526,95748,96963,98172,99374,100571,101761,102945,104124,105296,106464};
 if(!value)return 0;
 int exponent=0;while(value>=131072){value>>=1;++exponent;}while(value<65536){value<<=1;--exponent;}
 unsigned offset=(unsigned)value-65536,index=offset/2048,fraction=offset%2048;
 uint64_t result=mantissa[index]+(uint64_t)(mantissa[index+1]-mantissa[index])*fraction/2048;
 while(exponent>0){result=result*106464/65536;--exponent;}
 while(exponent<0){result=result*65536/106464;++exponent;}
 return result;
}
uint32_t fieldDistance(int32_t dx,int32_t dy) {
 // The current battlefield is below 1024 pixels across. Downscaling before
 // the integer root also accommodates a chain whose tail remains offscreen.
 unsigned shift=0;uint32_t x=(uint32_t)absn(dx),y=(uint32_t)absn(dy);
 while(x>32767||y>32767){x>>=1;y>>=1;++shift;}
 return isqrt32(x*x+y*y)<<shift;
}
}
void Game::blinkBurst(int32_t x,int32_t y,int radius,int damage) {
 // playerBlink creates spawnBurstGrowDamaging with knockBack=damage;
 // burstHitsSolid uses the same (.125*damage*resistance)^.7 impulse as
 // the other damaging bursts. Keep endpoint conversion in one place.
 blast(x,y,radius,damage);
 uint64_t pushed=0;
 for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS;target++){
  if(!targetActive((int)target)||!targetInRadius((int)target,x,y,(unsigned)radius))continue;
  unsigned owner=targetOwner((int)target);uint64_t bit=uint64_t(1)<<owner;
  if(pushed&bit)continue;
  pushed|=bit;Enemy &enemy=enemies[owner];
  uint64_t input=(uint64_t)damage*fieldResistance(enemy)*65536/(8ull*1000);
  uint64_t speed=fieldPowerSevenTenths(input)/(3*256);
  if(speed>64*FP)speed=64*FP;
  uint8_t angle=direction(delta(targetX((int)target),x,worldWidth()),delta(targetY((int)target),y,worldHeight()));
  enemy.vx=clamp(enemy.vx+dirX(angle)*(int)speed/FP,-64*FP,64*FP);
  enemy.vy=clamp(enemy.vy+dirY(angle)*(int)speed/FP,-64*FP,64*FP);
 }
}
void Game::triggerBurnout(const Construct &dead,int damage,int radius) {
 unsigned slot=MAX_PERSISTENT_FIELDS;
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++)if(!persistent_fields[i].active){slot=i;break;}
 if(slot==MAX_PERSISTENT_FIELDS){
  // More than a full construct pool detonated inside one 17-source-frame
  // burst lifetime. Retire the oldest visual after resolving its remaining
  // expansion once; an occupied fixed pool must not silently lose damage.
  slot=0;for(unsigned i=1;i<MAX_PERSISTENT_FIELDS;i++)if(persistent_fields[i].source_age>persistent_fields[slot].source_age)slot=i;
  PersistentField &old=persistent_fields[slot];
  unsigned final_radius=old.reserved[0]==1?(unsigned)minv(4096,(unsigned)old.base_radius*13):old.reserved[0]==2?(unsigned)minv(4096,26+(unsigned)old.base_radius*34/1000):(unsigned)old.base_radius*885/1000;
  for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS;target++)if(!(old.hit_mask[target/64]&(uint64_t(1)<<(target%64)))&&targetInRadius((int)target,old.x,old.y,final_radius)){
   if(old.reserved[0]==2&&(int64_t)(targetX((int)target)-old.x)*dirX(old.angle)+(int64_t)(targetY((int)target)-old.y)*dirY(old.angle)<0)continue;
   old.hit_mask[target/64]|=uint64_t(1)<<(target%64);damageTarget((int)target,old.damage,old.burn!=0,true);
  }
 }
 PersistentField &f=persistent_fields[slot];clearBytes(&f,sizeof(f));
 f.x=dead.x;f.y=dead.y;f.damage=maxv(0,damage);f.base_radius=(uint16_t)clamp(radius,1,65535);f.radius=0;f.opacity=255;f.active=1;f.angle=(uint8_t)range(256);f.burn=player.mods[M_X_BURNOUT_REACTORS]?1:0;
 for(unsigned i=0;i<MAX_ENEMIES;i++)f.owner_generation[i]=enemies[i].generation;
}
void Game::recordMaelstromDamage(unsigned amount) {
 if(!player.mods[M_X_MAELSTROM]||!amount)return;
 // Exact .0085 per actual point of damage, retaining the division remainder.
 uint64_t scaled=(uint64_t)amount*557056u+maelstrom_state.charge_remainder;
 uint64_t next=(uint64_t)maelstrom_state.charge_q16+scaled/1000;
 maelstrom_state.charge_q16=(uint32_t)(next>UINT32_MAX?UINT32_MAX:next);
 maelstrom_state.charge_remainder=(uint16_t)(scaled%1000);
}
void Game::updatePersistentEffects() {
 static_assert(MAX_ENEMIES==64&&MAX_ENEMIES+MAX_BOSS_PARTS<=256,"update persistent hit-set capacity with target pools");
 for(unsigned i=0;i<MAX_PERSISTENT_FIELDS;i++){
  PersistentField &f=persistent_fields[i];if(!f.active)continue;
  if(f.reserved[0]==2){f.x=player.mods[M_X_SANCTUARY]?sanctuary_x:player.x;f.y=player.mods[M_X_SANCTUARY]?sanctuary_y:player.y;f.angle=player.angle;}
  // Object slots can be reused while the same burst is alive. Its hit list
  // excludes an already-hit instance, not a new enemy in that array slot.
  for(unsigned owner=0;owner<MAX_ENEMIES;owner++)if(f.owner_generation[owner]!=enemies[owner].generation){
   f.owner_generation[owner]=enemies[owner].generation;f.pushed_mask&=~(uint64_t(1)<<owner);f.hit_mask[owner/64]&=~(uint64_t(1)<<(owner%64));
   for(unsigned p=0;p<MAX_BOSS_PARTS;p++)if(boss_parts[p].owner==owner){unsigned t=MAX_ENEMIES+p;f.hit_mask[t/64]&=~(uint64_t(1)<<(t%64));}
  }
  for(unsigned sub=0;sub<2&&f.active;sub++){
   ++f.source_age;
   // spawnFireBurst .005 scale, +.055/source frame. Alarm 7 starts .1
   // alpha loss per source frame; after ten fade frames the object is gone.
   if(f.reserved[0]==1){
    if(f.source_age>=51){f.active=0;f.opacity=0;break;}
    f.radius=(uint16_t)minv(4096,(unsigned)f.base_radius*(2u+f.source_age)/4);
    f.opacity=(uint8_t)(f.source_age<=11?255:(51-f.source_age)*255/40);
   }else if(f.reserved[0]==2){
    int alpha=(int)f.reserved[1]-(f.source_age>15?(f.source_age-15)*255/20:0);
    if(f.source_age>=35||alpha<=0){f.active=0;f.opacity=0;break;}
    f.radius=(uint16_t)minv(4096,26+(unsigned)f.base_radius*f.source_age/1000);f.opacity=(uint8_t)alpha;
   }else{
    if(f.source_age>=17){f.active=0;f.opacity=0;break;}
    f.radius=(uint16_t)maxv(1,(int)((unsigned)f.base_radius*(5+55*f.source_age)/1000));
    f.opacity=(uint8_t)(f.source_age<=7?255:(17-f.source_age)*255/10);
   }
   for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS;target++){
    uint64_t bit=uint64_t(1)<<(target%64);
    if((f.hit_mask[target/64]&bit)||!targetInRadius((int)target,f.x,f.y,f.radius))continue;
    if(f.reserved[0]==2&&(int64_t)(targetX((int)target)-f.x)*dirX(f.angle)+(int64_t)(targetY((int)target)-f.y)*dirY(f.angle)<0)continue;
    f.hit_mask[target/64]|=bit;
    unsigned owner=targetOwner((int)target);Enemy &enemy=enemies[owner];
    if(!(f.pushed_mask&(uint64_t(1)<<owner))){
     f.pushed_mask|=uint64_t(1)<<owner;
     // burstHitsSolid: push=(.125*burnDamage*res)^.7 * knockbackMod.
     // Convert its source pixels/frame to this engine's Q8/30 Hz impulse.
     uint64_t input=(uint64_t)f.damage*(f.burn?burnDamagePercent():100)*fieldResistance(enemy)*65536/(8ull*100*1000);
     uint64_t speed=fieldPowerSevenTenths(input)/(3*256);
     if(speed>64*FP)speed=64*FP;
     uint8_t angle=direction(targetX((int)target)-f.x,targetY((int)target)-f.y);
     int64_t vx=(int64_t)enemy.vx+dirX(angle)*(int64_t)speed/FP,vy=(int64_t)enemy.vy+dirY(angle)*(int64_t)speed/FP;
     enemy.vx=(int32_t)(vx>64*FP?64*FP:vx<-64*FP?-64*FP:vx);
     enemy.vy=(int32_t)(vy>64*FP?64*FP:vy<-64*FP?-64*FP:vy);
    }
    damageTarget((int)target,f.damage,f.burn!=0,true);
   }
  }
 }
 MaelstromState &m=maelstrom_state;
 bool enabled=player.mods[M_X_MAELSTROM]&&player.shield>0&&player.max_shield>0&&!player.mods[M_X_SHIELDED_DRONES];
 m.active=enabled?1:0;
 if(!enabled){m.charge_q16=0;m.charge_remainder=0;m.radius_q8=0;return;}
 unsigned effect=activeShieldEffect();
 unsigned radius_multiplier=(unsigned)((uint64_t)activeShieldRadius()*1000/maxv(1,shields[player.shield_type].radius));
 int32_t cx=player.mods[M_X_SANCTUARY]?sanctuary_x:player.x,cy=player.mods[M_X_SANCTUARY]?sanctuary_y:player.y;
 for(unsigned sub=0;sub<2;sub++){
  // Original oPowers Step runs at 60 Hz. Q16 keeps the full sub-point charge;
  // .00083333 is rounded only at the 1/65536 arithmetic boundary.
  uint32_t faded=(uint32_t)((uint64_t)m.charge_q16*993/1000);
  m.charge_q16=faded>55?faded-55:0;
  uint64_t strength=3277+(uint64_t)m.charge_q16*effect/100;
  m.radius_q8=125*FP/6+fieldMulDiv((uint64_t)m.charge_q16*radius_multiplier,500*FP,65536u*1000*6,UINT32_MAX-125*FP/6);
  uint32_t large_goal=fieldMulDiv(strength,15,8*16,UINT32_MAX),small_goal=fieldMulDiv(strength,57,40*16,UINT32_MAX);
  m.large_scale_q12=(uint32_t)(((uint64_t)m.large_scale_q12*9+large_goal)/10);
  m.small_scale_q12=(uint32_t)(((uint64_t)m.small_scale_q12*9+small_goal)/10);
  unsigned large_rot=fieldMulDiv(strength,7,2*256,6*256);
  unsigned small_rot=fieldMulDiv(strength,7,256,65535);
  m.rot_q8=(uint16_t)((m.rot_q8*9+large_rot)/10);m.small_rot_q8=(uint16_t)((m.small_rot_q8*9+small_rot)/10);
  // Public angles are screen-clockwise turns, matching renderer angle units.
  unsigned base_angle=2*256*effect/100;
  m.angle_q8=(uint16_t)(m.angle_q8-(base_angle+m.rot_q8)*256/360);
  m.small_angle_q8=(uint16_t)(m.small_angle_q8-(base_angle+m.small_rot_q8)*256/360);
  auto displacement=[&](int32_t x,int32_t y,unsigned resistance,int32_t &ox,int32_t &oy){
   ox=oy=0;int32_t dx=x-cx,dy=y-cy;uint32_t d=fieldDistance(dx,dy);
   if(!resistance||!d||d>m.radius_q8)return;
   uint64_t weight=65536;const unsigned inner=125*FP/6;
   if(d>inner){if(m.radius_q8<=inner)return;weight=(uint64_t)(m.radius_q8-d)*65536/(m.radius_q8-inner);}
   weight=(uint64_t)fieldMulDiv(strength,(uint32_t)weight,65536,65536000/resistance)*resistance/1000;
   if(weight>65536)weight=65536;
   uint64_t source_distance=(uint64_t)d*6;if(source_distance<256)source_distance=256;
   // Reduce the decimal constant before multiplying shield power; preserving
   // its Q16 value avoids an int64 intermediate overflow above 400% power.
   int64_t tangent=(1319468915ll*65536/1000000)*256*effect/(100*source_distance);
   int64_t radial=(int64_t)32768*radius_multiplier/1000;
   int64_t sx=(tangent*dy+radial*dx)/maxv(1,d),sy=(-tangent*dx+radial*dy)/maxv(1,d);
   int sign=player.mods[M_X_POLAR_INVERSION]?-1:1;
   ox=(int32_t)(sign*sx*(int64_t)weight/(6ll*65536*256));oy=(int32_t)(sign*sy*(int64_t)weight/(6ll*65536*256));
  };
  // Attached parts use one displacement for their common owner. The closest
  // real part is chosen, so an invisible controller cannot attract or repel
  // from empty space and a 65-piece serpent does not receive 65 impulses.
  for(unsigned owner=0;owner<MAX_ENEMIES;owner++){
   Enemy &e=enemies[owner];if(!e.active||e.boss_hidden)continue;
   int target=physicalCore(e.type)?(int)owner:-1;uint32_t best=target<0?UINT32_MAX:fieldDistance(e.x-cx,e.y-cy);
   for(unsigned p=0;p<MAX_BOSS_PARTS;p++)if(boss_parts[p].active&&boss_parts[p].owner==owner&&targetActive((int)(MAX_ENEMIES+p))){uint32_t d=fieldDistance(boss_parts[p].x-cx,boss_parts[p].y-cy);if(d<best){best=d;target=(int)(MAX_ENEMIES+p);}}
   if(target<0)continue;
   int32_t ox,oy;displacement(targetX(target),targetY(target),fieldResistance(e),ox,oy);
   if(!ox&&!oy)continue;
   int32_t nx=e.x+ox,ny=e.y+oy;
   nx=clamp(nx,0,worldWidth()*FP-1);ny=clamp(ny,0,worldHeight()*FP-1);
   ox=nx-e.x;oy=ny-e.y;e.x=nx;e.y=ny;
   for(unsigned p=0;p<MAX_BOSS_PARTS;p++)if(boss_parts[p].active&&boss_parts[p].owner==owner){boss_parts[p].x+=ox;boss_parts[p].y+=oy;}
  }
  for(unsigned i=FRIENDLY_BULLETS;i<MAX_BULLETS;i++){
   Bullet &b=bullets[i];if(!b.active||!b.owner)continue;int32_t ox,oy;displacement(b.x,b.y,1000,ox,oy);b.x+=ox;b.y+=oy;
   if(b.x<0||b.y<0||b.x>=worldWidth()*FP||b.y>=worldHeight()*FP)b.active=0;
  }
 }
}
