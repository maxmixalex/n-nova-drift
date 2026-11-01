if(b.owner&&b.kind==SHOT_TRIANGLE_SPAWNER){
 if(b.life)--b.life;
 for(unsigned sub=0;sub<2&&b.pierce;++sub){b.last_hit-=2;
  if(b.last_hit<=0){b.last_hit+=5;
   static const int angles[3]={330,210,90};
   for(unsigned j=0;j<3;++j){uint8_t a=(uint8_t)(b.angle+angles[j]*256/360);int shot=emitBullet(b.x,b.y,a,1,0,SHOT_PULSAR_MINE,240,1,2);
    if(shot>=0){Bullet &p=bullets[shot];p.speed=154;p.vx=dirX(a)*154/FP;p.vy=dirY(a)*154/FP;p.last_hit=(int16_t)b.damage;p.size_percent=100;}}
   b.pierce=(uint8_t)(b.pierce-3);
  }
 }
 if(!b.life||!b.pierce)b.active=0;
 continue;
}
if(b.owner&&b.kind==SHOT_PULSAR_MINE){
 if(b.age>=5)b.damage=b.last_hit;
 b.speed=(uint16_t)minv(64*FP,((unsigned)b.speed*10201u+5000)/10000);
 b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
}
if(b.owner&&(b.kind==SHOT_SPIKE||b.kind==SHOT_SPIKE_MINI||b.kind==SHOT_SPIKE_TRIPLE)){
 if(b.kind==SHOT_SPIKE_TRIPLE&&b.age>=13){destroyHostileBullet(i);continue;}
 if(b.kind==SHOT_SPIKE_MINI){b.size_percent=(uint16_t)maxv(0,(int)b.size_percent-102);}
 else if(b.kind==SHOT_SPIKE&&b.age>=15){
  b.vx=dirX(b.angle)*b.speed*197/(FP*200);b.vy=dirY(b.angle)*b.speed*197/(FP*200);
  for(unsigned sub=0;sub<2;++sub){unsigned decay=(unsigned)maxv(1,b.last_hit)*26624u/4096000u;b.size_percent=(uint16_t)maxv(0,(int)b.size_percent-(int)decay);b.last_hit=(int16_t)minv(30000,(int)b.last_hit*101/100);}
 }
 if(b.size_percent<=103){b.active=0;continue;}
}

if(b.owner&&b.kind==SHOT_CELESTIAL_DRONE){
 // Source .65 x/y scale, .00044/.001 shrink, rates *=1.175 each two source frames.
 b.speed=(uint16_t)((unsigned)b.speed*9801u/10000);b.vx=dirX(b.angle)*b.speed/FP;b.vy=dirY(b.angle)*b.speed/FP;
 b.field_scale=(uint32_t)minv(100000000,(int)((uint64_t)b.field_scale*1175/1000));
 unsigned dx=(unsigned)((uint64_t)b.field_scale*360448/6553600000ull);
 unsigned dy=(unsigned)((uint64_t)b.field_scale*8192/65536000ull);
 b.size_percent=(uint16_t)maxv(0,(int)b.size_percent-(int)dx);b.last_hit=(int16_t)maxv(0,(int)b.last_hit-(int)dy);
 if(b.size_percent<=103||b.last_hit<=103){b.active=0;continue;}
}
