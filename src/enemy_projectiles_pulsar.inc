if(b.owner&&b.kind==SHOT_PULSAR_BARRAGE){
 for(unsigned sub=0;sub<2;sub++){
  unsigned age=(b.age-1)*2+sub+1;
  if(age>=5)b.damage=b.traveled;
  unsigned speed=(unsigned)b.speed*1007+b.tracking_scale_y;b.speed=(uint16_t)minv(64*FP,speed/1000);b.tracking_scale_y=(uint16_t)(speed%1000);
  if(age==50)b.tracking_seek=128;
  if(age>=50){
   int difference=(int16_t)((int)direction(player.x-b.x,player.y-b.y)*256-b.tracking_scale_x);
   int turn=maxv(1,(int)b.tracking_seek*256/360);b.tracking_scale_x=(uint16_t)(b.tracking_scale_x+clamp(difference,-turn,turn));b.angle=(uint8_t)(b.tracking_scale_x/256);
  }
  unsigned turn_fraction=(unsigned)maxv(0,b.tracking_target)+b.homing*2432/10;b.tracking_target=(int16_t)(turn_fraction%10000);b.tracking_seek=(uint16_t)minv(65535,b.tracking_seek+turn_fraction/10000);
  if(age>=240){unsigned sx=(unsigned)((uint64_t)b.field_scale*4096/1000000),sy=(unsigned)((uint64_t)b.field_scale*4096/1250000);b.size_percent=(uint16_t)maxv(0,(int)b.size_percent-(int)sx);b.last_hit=(int16_t)maxv(0,b.last_hit-(int)sy);if((age&1)==0)b.field_scale=(uint32_t)minv(100000000,(int)((uint64_t)b.field_scale*115/100));}
 }
 b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 if(b.size_percent<=103||b.last_hit<=103||b.x+b.vx<0||b.x+b.vx>=worldWidth()*FP||b.y+b.vy<0||b.y+b.vy>=worldHeight()*FP){b.active=0;continue;}
}
if(b.owner&&b.kind==SHOT_PULSAR_PULSE){
 if(b.homing){
  if(b.tracking_target<0||b.tracking_target>=MAX_ENEMIES||!enemies[b.tracking_target].active||enemies[b.tracking_target].generation!=b.tracking_seek){b.active=0;continue;}
  const Enemy &leader=enemies[b.tracking_target];b.x=leader.x;b.y=leader.y;b.angle=leader.angle;
 }
 // A growing, forward-facing source lance, not a circular explosion. Its
// source sprite bbox is x29..565/y26..98 with pivot25/63.
 for(unsigned sub=0;sub<2;sub++){
  ++b.traveled;b.size_percent=(uint16_t)minv(65535,b.size_percent+82);
  unsigned width=(unsigned)((uint64_t)37*b.size_percent*FP/(4096*6));
  int32_t end_x=b.x+(int64_t)dirX(b.angle)*540*b.size_percent/(4096*6),end_y=b.y+(int64_t)dirY(b.angle)*540*b.size_percent/(4096*6);
  if(!(b.field_scale&1)&&lineHit(b.x,b.y,end_x,end_y,player.x,player.y,player.radius+(width+255)/256)){damagePlayer(b.damage,false,(uint8_t)(b.angle+128));b.field_scale|=1;}
  for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&!(b.field_scale&(2u<<c))&&lineHit(b.x,b.y,end_x,end_y,constructs[c].x,constructs[c].y,constructs[c].radius+(width+255)/256)){damageConstruct(c,b.damage);b.field_scale|=2u<<c;}
 }
 if(b.life)--b.life;
 if(!b.life)b.active=0;
 continue;
}
