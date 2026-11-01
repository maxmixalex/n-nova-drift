// Source dischargeSpawn and triggerTempestBreak build oChain burn objects.
// Damage is expressed per four 60 Hz source frames (burnMod=4), not as an
// instantaneous circular blast. Each node starts fading after ten source frames and extends
// a shrinking chain; nodes use the existing bounded projectile pool.
int Game::emitElectricalNode(int32_t x,int32_t y,uint8_t a,unsigned length_q8,unsigned damage_q8,unsigned links,unsigned thickness){
 if(x<0||y<0||x>=worldWidth()*FP||y>=worldHeight()*FP)return -1;
 int slot=emitBullet(x,y,a,1,(int)maxv(1,damage_q8),SHOT_DISCHARGE,7,0,maxv(1,(thickness+255)/256));
 if(slot<0)return slot;
 Bullet &b=bullets[slot];b.vx=(int32_t)((int64_t)dirX(a)*length_q8/FP);b.vy=(int32_t)((int64_t)dirY(a)*length_q8/FP);
 b.speed=(uint16_t)(frame&255);b.homing=0;b.pierce=(uint8_t)range(256);b.blast=0;b.wraps=0;
 b.last_hit=(int16_t)minv(32767u,(unsigned)((uint64_t)60000000*256/(maxv(1,weaponRateMilli())*20000ull)));
 b.bounces=(uint8_t)minv(255,links);b.size_percent=(uint16_t)clamp(thickness,1u,8192u);b.construct_source=0;b.traveled=0;
 return slot;
}
void Game::triggerDischarge(){
 Player &p=player;if(!p.mods[M_DISCHARGE]||p.shield<=0||discharge_clock_)return;
 const unsigned rof_milli=(unsigned)minv((uint64_t)60000000,(uint64_t)60000000/maxv(1,weaponRateMilli()));
 const unsigned scale_million=775000+(unsigned)((uint64_t)rof_milli*675/100);
 unsigned size_q8=maxv(1,unsigned(p.radius)*256/bodies[B_STANDARD].radius);
 unsigned length=(unsigned)minv((uint64_t)(worldWidth()/2-2)*FP,(uint64_t)310*4*scale_million*size_q8/60000000);
 unsigned power=activeShieldEffect();
 unsigned base_milli=3400+(unsigned)((uint64_t)2050*rof_milli/1000);
 unsigned damage=(unsigned)minv((uint64_t)2000000,(uint64_t)base_milli*power*256/100000);
 // The original collision box is 62 source pixels high. Convert its
 // half-width with the actual vertical scale, independently of chain length.
 unsigned width=(unsigned)maxv((uint64_t)1,(uint64_t)62*scale_million*power*256/1200000000);bool emitted=false;
 bool transfer=p.mods[M_X_SHIELDED_DRONES]!=0;
 for(unsigned center=0;center<(transfer?(unsigned)MAX_CONSTRUCTS:1u);++center){
  if(transfer&&(!constructs[center].active||constructs[center].kind>C_GUARDIAN))continue;
  int32_t x=transfer?constructs[center].x:p.x,y=transfer?constructs[center].y:p.y;
  unsigned factor=transfer?(unsigned)maxv(1,constructPower(constructs[center].kind)):100u;
  bool extra=!p.mods[M_X_INFUSE]&&range(300)<factor;
  int target=nearestTarget(x,y,200);uint8_t aim=target>=0?direction(delta(targetX(target),x,worldWidth()),delta(targetY(target),y,worldHeight())):(uint8_t)range(256);
  for(unsigned root=0;root<1u+extra;++root){
   unsigned links=(unsigned)minv((uint64_t)255,((uint64_t)(8000+range(rof_milli/5+1))*factor*(extra?60:100)+5000000)/10000000);
   uint8_t a=root?(uint8_t)range(256):target>=0?(uint8_t)(aim+int(range(15))-7):aim;
   emitted|=emitElectricalNode(x,y,a,(unsigned)minv((uint64_t)(worldWidth()/2-2)*FP,(uint64_t)length*factor/100),(unsigned)minv((uint64_t)2000000,(uint64_t)damage*factor/100),links,maxv(1,width*factor/100))>=0;
  }
 }
 if(!emitted)return;
 discharge_clock_=6; // dischargeInternalCooldown=12 source frames.
 // Keep milli-hull through Medi-Charge and self-damage/defense modifiers.
 // Gemini's additional half hull cost does not replace the shield cost.
 if(p.mods[M_X_GEMINI]){
  unsigned gemini=(unsigned)((uint64_t)rof_milli*15/200);
  if(transfer){for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&constructs[c].kind<=C_GUARDIAN){
   uint64_t total=(uint64_t)constructs[c].redirected_fraction+gemini;constructs[c].redirected_fraction=(uint16_t)(total%1000);
   if(total>=1000)damageConstruct(c,(int)(total/1000));
  }}else damageHullSelf(gemini);
 }
 damageShieldSelf((unsigned)((uint64_t)rof_milli*15/100));
}
void Game::triggerTempest(int32_t x,int32_t y,unsigned power){
 // Source random_range(2,4) is truncated: two or three initial roots.
 // Each has round(random_range(12,24)) links; the random sequence is not
 // a fixed three-bolt/count recipe.
 unsigned roots=2+range(2);uint8_t start=(uint8_t)range(256);
 int target=nearestTarget(x,y);uint8_t aim=target>=0?direction(delta(targetX(target),x,worldWidth()),delta(targetY(target),y,worldHeight())):start;
 unsigned size_q8=maxv(1,unsigned(player.radius)*256/bodies[B_STANDARD].radius);
 unsigned length=(unsigned)minv((uint64_t)(worldWidth()/2-2)*FP,(uint64_t)2015*size_q8/60);
 unsigned damage=(unsigned)minv((uint64_t)2000000,(uint64_t)372400*power*256/100000);
 unsigned width=maxv(1u,(unsigned)((uint64_t)62*13*power*256/12000));
 for(unsigned k=0;k<roots;++k){unsigned links=12+(range(65536)*12+32768)/65536;uint8_t a=target>=0?(uint8_t)(aim+int(range(15))-7):start;emitElectricalNode(x,y,a,length,damage,links,width);}
}
