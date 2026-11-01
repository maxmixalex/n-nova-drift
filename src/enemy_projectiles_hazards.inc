// Source projectile families. This hook runs before common single-hit motion.
if(b.owner&&b.kind==SHOT_ENEMY_SHIELD){
 int target=b.last_hit;
 if(target<0||target>=MAX_ENEMIES||!enemies[target].active||enemies[target].generation!=b.size_percent||enemies[target].hard_shield>0||!b.life){b.active=0;continue;}
 Enemy &recipient=enemies[target];if(!--b.life){b.active=0;continue;}
 uint8_t a=direction(delta(recipient.x,b.x,worldWidth()),delta(recipient.y,b.y,worldHeight()));b.angle=toward(b.angle,a,12);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 int32_t ox=b.x,oy=b.y;b.x=wrap(b.x+b.vx,worldWidth());b.y=wrap(b.y+b.vy,worldHeight());
 if(sweptHit(ox,oy,b.x,b.y,recipient.x,recipient.y,recipient.radius+b.radius)){
  recipient.hard_shield=recipient.max_hull;recipient.hard_shield_timer=(uint16_t)(120+30*recipient.elite);emitEffect(recipient.x,recipient.y,FX_RING,recipient.radius+4,15,2);b.active=0;
 }continue; // A shielding carrier never hurts the player or constructs.
}
if(b.owner&&(b.kind==SHOT_PULSEBUG||b.kind==SHOT_PULSEBUG_HOMING)){
 unsigned decay=b.kind==SHOT_PULSEBUG?950625:985056;
 b.speed=(uint16_t)((uint64_t)b.speed*decay/1000000);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 b.radius=(uint8_t)maxv(1,(int)b.radius); // Sprite size decay is rendered by age.
}
if(b.owner&&(b.kind==SHOT_INTERCEPTOR_MISSILE||b.kind==SHOT_PULSEBUG_HOMING||b.kind==SHOT_STATION_DRONE)){
 if(b.kind==SHOT_INTERCEPTOR_MISSILE)b.speed=(uint16_t)minv(24*FP,b.speed+26); // .15/source frame /3 spatial mapping, twice.
 uint8_t a=direction(delta(player.x,b.x,worldWidth()),delta(player.y,b.y,worldHeight()));
 b.angle=toward(b.angle,a,b.kind==SHOT_PULSEBUG_HOMING?2:3);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
}
if(b.owner&&b.kind==SHOT_BEAMCASTER){
 // Create scales .25; .0025 per source frame from both sprite axes.
 b.size_percent=(uint16_t)maxv(1,100-2*(int)b.age);if(b.age>=45){destroyHostileBullet(i);continue;}
}
if(b.owner&&b.kind==SHOT_TRACER_WAKE){
 if(!b.life||!--b.life){b.active=0;continue;}
 b.size_percent=(uint16_t)maxv(1,100-2*(int)b.age);
 b.x=wrap(b.x+b.vx,worldWidth());b.y=wrap(b.y+b.vy,worldHeight());
 unsigned total=(unsigned)maxv(0,b.damage)+b.traveled;b.traveled=(uint16_t)(total%30);unsigned amount=total/30;
 if(amount){if(distance2(b.x,b.y,player.x,player.y)<(b.radius+player.radius)*(b.radius+player.radius))damagePlayer((int)amount,false,(uint8_t)(b.angle+128),true);
  for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&distance2(b.x,b.y,constructs[c].x,constructs[c].y)<(b.radius+constructs[c].radius)*(b.radius+constructs[c].radius))damageConstruct(c,(int)amount);
 }continue;
}
if(b.owner&&b.kind==SHOT_TRACER){
 if(b.age==1)b.last_hit=9830; // Source Alarm1 yscale=.3 in Q15.
 for(unsigned sub=0;sub<2;sub++)b.last_hit=(int16_t)maxv(0,int(b.last_hit)*99/100-131);
 b.size_percent=(uint16_t)maxv(1,int(b.last_hit)*100/9830);
 if(b.last_hit<819){destroyHostileBullet(i);continue;}
 // Trace actors leave persistent burning wakes rather than generic bullets.
 if((b.age&1)==1){int n=emitBullet(b.x,b.y,b.angle,1,100,SHOT_TRACER_WAKE,45,1,3);if(n>=0){Bullet &wake=bullets[n];wake.speed=137;wake.vx=dirX(b.angle)*137/FP;wake.vy=dirY(b.angle)*137/FP;wake.damage=b.bounces?67:100;}}
 bool ray_combined=false;int32_t nx=wrap(b.x+b.vx,worldWidth()),ny=wrap(b.y+b.vy,worldHeight());
 for(unsigned orb=FRIENDLY_BULLETS;orb<MAX_BULLETS;orb++)if(orb!=i&&bullets[orb].active&&bullets[orb].owner&&bullets[orb].kind==SHOT_TRACER_BALL&&!bullets[orb].pierce&&sweptHit(b.x,b.y,nx,ny,bullets[orb].x,bullets[orb].y,b.radius+bullets[orb].radius)){
  Bullet ray=b,ball=bullets[orb];bullets[orb].active=0;b.active=0;ray_combined=true;
  unsigned count=4+2*minv(2,ball.homing);
  for(unsigned j=0;j<count;j++){int n=emitBullet(ball.x,ball.y,(uint8_t)(j*256/count),2,maxv(1,ball.damage),SHOT_TRACER_BALL,75,1,3);if(n>=0){bullets[n].pierce=1;bullets[n].blast=(uint8_t)maxv(1,ball.blast/6);bullets[n].construct_source=ball.construct_source;bullets[n].size_percent=67;}}
  for(unsigned j=0;j<4;j++){int n=emitBullet(ball.x,ball.y,(uint8_t)(ray.angle+((int)j*16-24)*256/360),maxv(1,ray.speed/FP),maxv(1,ray.damage/2),SHOT_TRACER,45,1,1);if(n>=0){bullets[n].speed=ray.speed;bullets[n].vx=dirX(bullets[n].angle)*ray.speed/FP;bullets[n].vy=dirY(bullets[n].angle)*ray.speed/FP;bullets[n].bounces=1;bullets[n].wraps=1;bullets[n].construct_source=ray.construct_source;}}
  break;
 }
 if(ray_combined)continue;
}
