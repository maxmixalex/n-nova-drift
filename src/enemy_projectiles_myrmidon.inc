if(b.kind==SHOT_MYRMIDON_SPAWNER){
 if(b.life)--b.life;
 if(!b.life||!b.homing){b.active=0;continue;}
 for(unsigned sub=0;sub<2&&b.active;++sub){
  if(b.tracking_scale_x)--b.tracking_scale_x;
  if(b.tracking_scale_x)continue;
  --b.homing;
  unsigned distance=(unsigned)b.traveled*isqrt32(range(65536))/256;
  uint8_t direction_=(uint8_t)range(256);
  int32_t x=b.x+dirX(direction_)*(int)distance/FP,y=b.y+dirY(direction_)*(int)distance/FP;
  unsigned scale=819+(unsigned)((uint64_t)range(1147)*b.size_percent/4096);
  unsigned delay=25+range(18000)/1000;
  int damage=maxv(1,(int)((int64_t)b.damage*scale/4096));
  spawnMyrmidonField(x,y,scale,damage,delay,b.owner!=0);
  b.tracking_scale_x=(uint16_t)(1+range(4));
  if(!b.homing)b.active=0;
 }
 continue;
}
if(b.kind==SHOT_MYRMIDON_BLAST){
 if(b.life)--b.life;
 if(!b.life){b.active=0;continue;}
 for(unsigned sub=0;sub<2&&b.active;++sub){
  if(!b.pierce){
   b.size_percent=(uint16_t)((int)b.size_percent+((int)b.field_scale-(int)b.size_percent)/50);
   if(++b.tracking_scale_x<b.tracking_scale_y)continue;
   b.pierce=1;b.tracking_scale_x=0;b.tracking_seek=0;b.wraps=0;
  }
  unsigned age=b.tracking_scale_x;
  if(age>=12){b.active=0;break;}
  b.size_percent=(uint16_t)((uint64_t)b.field_scale*(14-age)/10);
  b.radius=(uint8_t)clamp((int)((uint64_t)b.size_percent*200/(4096*6)),1,100);
  applyMyrmidonField(b);++b.tracking_scale_x;
 }
 continue;
}
if(b.kind==SHOT_MYRMIDON_BOMB){
 if(!b.owner&&b.homing){ // The common Reflect shield has just changed ownership.
  b.traveled=(uint16_t)((unsigned)b.angle*256);b.homing=0;b.tracking_target=-1;b.last_hit=-1;
  b.field_scale=(uint32_t)maxv(1,b.damage);b.pierce=1;
 }
 unsigned rarity=b.construct_source<3?b.construct_source:2;
 unsigned target_scale=(unsigned)((uint64_t)(2048+717*rarity)*maxv(1,b.tracking_seek)/256);
 for(unsigned sub=0;sub<2;++sub){
  unsigned age=(unsigned)b.tracking_scale_y+1;
  if(age==30)b.tracking_scale_x=2048; // Alarm0: canTurn, rotSpeed=.5 degrees.
  if(age>=30){
   uint8_t target=b.angle;bool found=false;
   if(b.owner){
    if(!(player.body==B_SPECTRE&&player.cloak>130)){target=direction(delta(player.x,b.x,worldWidth()),delta(player.y,b.y,worldHeight()));found=true;}
   }else {int t=nearestTarget(b.x,b.y);if(t>=0){target=direction(delta(targetX(t),b.x,worldWidth()),delta(targetY(t),b.y,worldHeight()));found=true;}}
   if(found){int turn=(int)(int16_t)((uint16_t)((unsigned)target*256)-b.traveled);int cap=maxv(0,(int)b.tracking_scale_x*16/360);b.traveled=(uint16_t)(b.traveled+clamp(turn,-cap,cap));}
  }
  b.angle=(uint8_t)(b.traveled/256);
  b.speed=(uint16_t)minv(64*FP,((int)b.speed*1005+500)/1000);
  ++b.tracking_scale_y;
  if(!b.pierce){
   if(b.tracking_scale_x<6144){
    if(b.size_percent<target_scale)b.size_percent=(uint16_t)minv(65535,(int)b.size_percent+82);
    b.tracking_scale_x=(uint16_t)(b.tracking_scale_x+138);
   }
   bool alive=b.tracking_target>=0&&b.tracking_target<MAX_ENEMIES&&enemies[b.tracking_target].active&&enemies[b.tracking_target].generation==(uint16_t)b.last_hit;
   if(b.tracking_scale_y>=53&&!alive)b.pierce=1;
  }else if(b.tracking_scale_x)b.tracking_scale_x=(uint16_t)maxv(0,(int)b.tracking_scale_x*995/1000-5);
 }
 b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 b.radius=(uint8_t)clamp(((int)b.size_percent*40+4096*6-1)/(4096*6),1,100);
 int32_t nx=b.x+b.vx,ny=b.y+b.vy;
 if(nx<0||ny<0||nx>=worldWidth()*FP||ny>=worldHeight()*FP){b.active=0;continue;}
 if(!b.owner){
  int32_t ox=b.x,oy=b.y;b.x=nx;b.y=ny;
  if(b.life)--b.life;
  bool hit=false;
  for(unsigned target=0;target<MAX_ENEMIES+MAX_BOSS_PARTS&&!hit;++target)if(targetSwept((int)target,ox,oy,nx,ny,b.radius)){
   damageTarget((int)target,b.damage);hit=true;
  }
  if(hit||!b.life){Bullet dead=b;b.active=0;detonateMyrmidon(dead);}
  continue;
 }
 // Hostile contacts, Reflect, Bastion interception and lifetime share the
 // ordinary collision pipeline after the two source-frame state updates.
}
