// Original triggerScorchingWake and oScorchingWake use a persistent player-burn
// object, not an on-hit three-second damage effect. Q12 retains its small scale
// and source speed while the rest of the arena keeps Q8 positions.
void Game::triggerScorchingWake(){
 Player &p=player;unsigned count=p.mods[M_X_SCORCHING_WAKE];
 if(!count||!p.thrusting||frame%3)return; // six original 60 Hz frames.
 unsigned source_speed=isqrt32((uint32_t)((int64_t)p.vx*p.vx+(int64_t)p.vy*p.vy))*48u;
 for(unsigned k=0;k<count;++k){
  int32_t x=p.x,y=p.y;uint8_t heading=p.angle;
  if(segment_count){unsigned selected=range(segment_count*3/2+1);if(selected<segment_count){
   x=segment_x[selected];y=segment_y[selected];int32_t tx=selected?segment_x[selected-1]:p.x,ty=selected?segment_y[selected-1]:p.y;
   heading=direction(delta(tx,x,worldWidth()),delta(ty,y,worldHeight()));
  }}
  uint8_t angle=(uint8_t)(heading+128+int(range(65))-32);
  unsigned scale=512+range(1536),speed=7168+range(12288)+minv(20480u,source_speed*55/100);
  int slot=emitBullet(x,y,angle,1,180,SHOT_WAKE,150,0,1);if(slot<0)continue;
  Bullet &b=bullets[slot];b.speed=(uint16_t)speed;b.size_percent=(uint16_t)maxv(1u,scale/10);b.traveled=(uint16_t)scale;b.last_hit=(int16_t)scale;
  b.homing=b.pierce=b.blast=b.bounces=b.wraps=b.construct_source=0;
  // Native ShieldId order; source shield enum/color evidence is recorded
  // separately. Blue is the source Standard shield's initial color.
  static const uint8_t colors[SHIELD_COUNT]={0,6,1,6,3,4,2,0,1,4};
  b.pierce=colors[p.shield_type];
  b.vx=(int32_t)((int64_t)dirX(angle)*speed/(FP*48));b.vy=(int32_t)((int64_t)dirY(angle)*speed/(FP*48));
  // The source batches recursive ranks every six frames, interpolating the
  // individual emission positions by k * (6 / rank) source steps.
  int offset=(int)(k*6*65536u/count);
  b.x=wrap(x+(int32_t)(((int64_t)b.vx*offset+(int64_t)p.vx*(65536-offset))/131072),worldWidth());
  b.y=wrap(y+(int32_t)(((int64_t)b.vy*offset+(int64_t)p.vy*(65536-offset))/131072),worldHeight());
 }
}
void Game::updateScorchingWake(Bullet &b){
 if(!b.life){b.active=0;return;}
 for(unsigned sub=0;sub<2&&b.active;++sub){
  unsigned source_age=(unsigned)(b.age-1)*2+sub+1;
  int target=b.traveled,maximum=b.last_hit;
  if(source_age>=45){
   target=target*96/100;maximum=maximum*96/100;
   if(b.size_percent<=102){b.active=0;break;} // source sizeDestroy: .025.
   b.size_percent=(uint16_t)target;
  }else b.size_percent=(uint16_t)((4u*b.size_percent+(unsigned)target)/5);
  if(!b.homing){target=maxv(0,target-123);if(target<=maximum*6/10)b.homing=1;}
  else {target+=123;if(target>=maximum)b.homing=0;}
  b.traveled=(uint16_t)target;b.last_hit=(int16_t)maximum;
  if(b.speed>4096)b.speed=(uint16_t)(b.speed-51); // >1: -.0125/source frame.
  b.speed=(uint16_t)((unsigned)b.speed*98/100);
  b.vx=(int32_t)((int64_t)dirX(b.angle)*b.speed/(FP*48));b.vy=(int32_t)((int64_t)dirY(b.angle)*b.speed/(FP*48));
  int32_t ox=b.x,oy=b.y,nx=b.x+b.vx/2,ny=b.y+b.vy/2;
  if(nx<0||ny<0||nx>=worldWidth()*FP||ny>=worldHeight()*FP){b.active=0;break;}
  b.x=nx;b.y=ny;
  // The source art's pivot is near its front. Its 257x266 collision bounds
  // are centered approximately 90 source pixels behind that pivot.
  int offset=(int)((unsigned)b.size_percent*90*FP/(4096*6));
  int32_t dx=(int32_t)((int64_t)dirX(b.angle)*offset/FP),dy=(int32_t)((int64_t)dirY(b.angle)*offset/FP);
  b.radius=(uint8_t)clamp(((unsigned)b.size_percent*133+24575)/24576,1u,100u);
  unsigned corrosion=0;if(player.mods[M_CORROSION]){unsigned fraction=b.blast+2u;corrosion=fraction/15;b.blast=(uint8_t)(fraction%15);} // .13333/source frame.
  for(unsigned t=0;t<MAX_ENEMIES+MAX_BOSS_PARTS;++t){
   if(!targetActive(t)||!targetSwept(t,ox-dx,oy-dy,nx-dx,ny-dy,b.radius))continue;
   unsigned owner=targetOwner(t);uint16_t generation=enemies[owner].generation;unsigned vulnerability=enemies[owner].vulnerability;
   if(t<MAX_ENEMIES)enemies[t].wake_time=(uint16_t)igniteDurationTicks();
   else boss_parts[t-MAX_ENEMIES].wake_time=(uint16_t)igniteDurationTicks();
   damageTarget(t,3,true); // source 180 / 60, per overlapping burn object.
   if(player.mods[M_CORROSION]&&enemies[owner].active&&enemies[owner].generation==generation)enemies[owner].vulnerability=(uint8_t)minv(60u,vulnerability+corrosion);
  }
 }
 if(b.active&&!--b.life)b.active=0;
}
