if(b.owner&&(b.kind==SHOT_ECLIPSE_TORRENT||b.kind==SHOT_ECLIPSE_SHOT)){
 for(unsigned sub=0;sub<2;++sub){
  if(b.kind==SHOT_ECLIPSE_TORRENT){b.speed=(uint16_t)(b.speed*995/1000);b.tracking_scale_x=(uint16_t)maxv(0,(int)b.tracking_scale_x-20);b.tracking_scale_y=(uint16_t)maxv(0,(int)b.tracking_scale_y-20);
   b.field_scale=(b.field_scale+b.tracking_weapon*6u)&65535u;int angular=dirY((uint8_t)(b.field_scale/256))*b.tracking_seek/(256*360);b.angle=(uint8_t)(b.last_hit+angular);
  }else {b.speed=(uint16_t)(b.speed*99/100);b.tracking_scale_x=(uint16_t)(b.tracking_scale_x*975/1000);b.tracking_scale_y=(uint16_t)(b.tracking_scale_y*975/1000);}
 }
 if(b.tracking_scale_x<=103||b.tracking_scale_y<=103){b.active=0;continue;}
 b.radius=(uint8_t)clamp(73u*maxv(b.tracking_scale_x,b.tracking_scale_y)/(12*4096),1,100);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
}
if(b.owner&&b.kind==SHOT_ECLIPSE_ORB){
 // GameMaker's friction subtracts .065 source pixels/frame, it is not a ratio.
 b.speed=(uint16_t)maxv(0,(int)b.speed-11);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
}
if(b.owner&&b.kind==SHOT_ECLIPSE_VORTEX){
 if(!b.life||!--b.life){b.active=0;continue;}
 b.speed=(uint16_t)maxv(0,(int)b.speed-11);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;b.x=wrap(b.x+b.vx,worldWidth());b.y=wrap(b.y+b.vy,worldHeight());
 for(unsigned sub=0;sub<2;++sub){b.field_scale=minv(64000000u,b.field_scale+400u+250u*minv(2,b.tracking_weapon));if(b.age>300)b.homing=(uint8_t)maxv(0,(int)b.homing-13);}
 if(!b.homing){b.active=0;continue;}
 b.tracking_scale_x=b.tracking_scale_y=(uint16_t)minv(65535u,(uint32_t)((uint64_t)b.field_scale*4096/1000000));b.radius=(uint8_t)clamp((uint64_t)651*b.field_scale/12000000,1,100);
 unsigned total=(unsigned)b.damage+b.traveled;b.traveled=(uint16_t)(total%256);unsigned amount=total/256;
 if(amount){if(distance2(b.x,b.y,player.x,player.y)<(b.radius+player.radius)*(b.radius+player.radius))damagePlayer((int)amount,false,(uint8_t)(b.angle+128),true);
  for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&distance2(b.x,b.y,constructs[c].x,constructs[c].y)<(b.radius+constructs[c].radius)*(b.radius+constructs[c].radius))damageConstruct(c,(int)amount);}
 continue;
}
if(b.owner&&(b.kind==SHOT_ECLIPSE_REFLECT||b.kind==SHOT_ECLIPSE_BURST)){
 if(!b.life||!--b.life){b.active=0;continue;}
 for(unsigned sub=0;sub<2;++sub){++b.traveled;b.tracking_scale_x=(uint16_t)minv(65535,b.tracking_scale_x+b.tracking_seek);b.tracking_scale_y=(uint16_t)minv(65535,b.tracking_scale_y+b.tracking_seek);
  if(b.traveled>(b.kind==SHOT_ECLIPSE_REFLECT?12:10))b.homing=(uint8_t)maxv(0,(int)b.homing-(b.kind==SHOT_ECLIPSE_REFLECT?51:19));
  const auto hit=[&](int32_t x,int32_t y,unsigned r){if(b.kind==SHOT_ECLIPSE_BURST){int radius=506*b.tracking_scale_x/(12*4096);return distance2(b.x,b.y,x,y)<(radius+(int)r)*(radius+(int)r);}
   int32_t endx=b.x+(int64_t)dirX(b.angle)*540*b.tracking_scale_x/(6*4096),endy=b.y+(int64_t)dirY(b.angle)*540*b.tracking_scale_x/(6*4096);return lineHit(b.x,b.y,endx,endy,x,y,r+37*b.tracking_scale_y/(6*4096));};
  if(!(b.field_scale&1)&&hit(player.x,player.y,player.radius)){damagePlayer(b.damage,false,(uint8_t)(b.angle+128));b.field_scale|=1;}
  for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&!(b.field_scale&(2u<<c))&&hit(constructs[c].x,constructs[c].y,constructs[c].radius)){damageConstruct(c,b.damage);b.field_scale|=2u<<c;}
 }
 if(!b.homing)b.active=0;
 continue;
}
