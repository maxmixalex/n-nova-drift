// Included per active Bullet after ++age, before ordinary lifetime/motion.
if(b.owner&&b.kind==SHOT_DWELLER_GUARD){
 // Source trueSpeed -= .035/frame. After frame ten, Y decay starts at
 // .00018 and multiplies by 1.025/frame. last_hit holds remaining Q15 Y;
 // homing/pierce hold a Q20 decay rate, which is unused for hostile shots.
 if(b.age==1){b.last_hit=26214;b.homing=0;b.pierce=189;}
 b.speed=(uint16_t)maxv(0,(int)b.speed-6);
 b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 if(b.age>5){
  unsigned rate=(unsigned(b.homing)<<8)|b.pierce;
  for(unsigned sub=0;sub<2;++sub){rate=maxv(1,(rate*1025+500)/1000);b.last_hit=(int16_t)maxv(0,(int)b.last_hit-(int)((rate+16)/32));}
  b.homing=(uint8_t)(rate>>8);b.pierce=(uint8_t)rate;
  b.size_percent=(uint16_t)maxv(1,int(b.last_hit)*100/26214);
  b.radius=(uint8_t)maxv(1,3*int(b.last_hit)/26214);
  if(!b.last_hit){b.active=0;continue;}
 }
}
if(b.owner&&b.kind==SHOT_DWELLER_ARC){
 // oDwellerWarriorArc inherits oEnemyAoE, not oEnemyProjectile: contact is
 // continuous and the arc survives it. Damage is DPS; traveled is /30 carry.
 // last_hit holds Q15 size. homing/pierce hold the post-lifespan shrink rate.
 if(b.age==1){b.last_hit=3277;b.homing=10;b.pierce=61;}
 for(unsigned sub=0;sub<2;++sub){
  unsigned source_age=(b.age-1)*2+sub+1;
  if(source_age>175){unsigned rate=(unsigned(b.homing)<<8)|b.pierce;rate=(rate*10125+5000)/10000;b.homing=(uint8_t)(rate>>8);b.pierce=(uint8_t)rate;b.last_hit=(int16_t)maxv(0,(int)b.last_hit-(int)((rate+16)/32));}
  else if(b.last_hit<24576)b.last_hit=(int16_t)minv(24576,b.last_hit+328);
 }
 if(!b.last_hit||!b.life||!--b.life){b.active=0;continue;}
 b.size_percent=(uint16_t)maxv(1,int(b.last_hit)*100/32768);
 b.radius=(uint8_t)maxv(1,(329*int(b.last_hit)+196607)/393216);
 b.speed=(uint16_t)maxv(FP/3,(int)b.speed-3);
 b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 int slow=100;if(player.shield>0&&player.shield_type==S_TEMPORAL&&shieldDistance2(b.x,b.y)<(int)(activeShieldRadius()*activeShieldRadius()))slow=30;
 int32_t ox=b.x,oy=b.y;b.x+=b.vx*slow/100;b.y+=b.vy*slow/100;
 // The source AoE leaves the arena instead of wrapping. Cull at the
 // visible edge to preserve the shared bounded-coordinate save invariant.
 if(b.x<0||b.x>=worldWidth()*FP||b.y<0||b.y>=worldHeight()*FP){b.active=0;continue;}
 unsigned amount=(unsigned)b.damage+b.traveled;b.traveled=(uint16_t)(amount%30);amount/=30;
 if(amount){
  if(sweptHit(ox,oy,b.x,b.y,player.x,player.y,b.radius+player.radius))damagePlayer((int)amount,false,(uint8_t)(b.angle+128),true);
  for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&sweptHit(ox,oy,b.x,b.y,constructs[c].x,constructs[c].y,b.radius+constructs[c].radius))damageConstruct(c,(int)amount);
 }
 continue;
}
