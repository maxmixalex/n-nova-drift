if(!b.owner&&b.kind==SHOT_DISCHARGE){
 // A child placed in a later pool slot must not recursively run again during
 // the same native tick. This also makes behavior independent of free-slot order.
 if(b.speed==(frame&255)){--b.age;continue;}
 if(!b.life){b.active=0;continue;}
 if(b.bounces){
  unsigned left=b.bounces;b.bounces=0;uint8_t angle=b.angle;
  int32_t origin_x=b.x,origin_y=b.y,x=b.x+b.vx,y=b.y+b.vy;
  unsigned length=isqrt32((uint32_t)((int64_t)b.vx*b.vx+(int64_t)b.vy*b.vy));
  unsigned width=b.size_percent;int previous_child=-1;
  // Two original Alarm_0 steps per native tick. Intermediate nodes remain
  // alive and damaging; only the final child continues the chain next tick.
  for(unsigned step=0;step<2&&left;++step){
   uint8_t parent_angle=angle;
   length=maxv(1,length*85/100);width=maxv(1,width*85/100);
   int target=nearestTarget(x,y,maxv(8,length*4/FP));
   if(target>=0){uint8_t desired=direction(delta(targetX(target),x,worldWidth()),delta(targetY(target),y,worldHeight()));angle=(uint8_t)(desired+int(range(57))-28);}
   else angle=(uint8_t)(angle+int(range(115))-57);
   int child=emitElectricalNode(x,y,angle,length,(unsigned)b.damage,(step==1||left==1)?left-1:0,width);
   if(child<0){if(previous_child>=0)bullets[previous_child].bounces=(uint8_t)left;else b.bounces=(uint8_t)left;break;}
   --left;previous_child=child;
   if(left&&range(5)==0){
    unsigned span=(unsigned)maxv(0,b.last_hit),fork_links=1+(range(span+1)+128)/256;
    emitElectricalNode(origin_x,origin_y,(uint8_t)(parent_angle+int(range(115))-57),length,(unsigned)b.damage,fork_links,width);
   }
   origin_x=bullets[child].x;origin_y=bullets[child].y;
   x=origin_x+bullets[child].vx;y=origin_y+bullets[child].vy;
  }
 }
 unsigned amount=(unsigned)b.damage+b.traveled;b.traveled=(uint16_t)(amount%512);amount/=512;
 if(amount){
  int32_t x2=b.x+b.vx,y2=b.y+b.vy;
  for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS;++target)if(targetActive(target)){
   int32_t tx=targetX(target),ty=targetY(target);unsigned r=targetRadius(target)+b.radius;
   if(absn(tx-b.x)>absn(b.vx)+(int)r*FP||absn(ty-b.y)>absn(b.vy)+(int)r*FP)continue;
   bool hit=target<MAX_ENEMIES?lineHit(b.x,b.y,x2,y2,tx,ty,r):targetSwept(target,b.x,b.y,x2,y2,b.radius);
   if(hit)damageTarget(target,(int)amount,true);
  }
 }
 b.size_percent=(uint16_t)maxv(1,(unsigned)b.size_percent*9025/10000);b.radius=(uint8_t)maxv(1,((unsigned)b.size_percent+255)/256);
 --b.life;if(!b.life)b.active=0;
 continue;
}
