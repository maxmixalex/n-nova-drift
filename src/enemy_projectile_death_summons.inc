if(dead.kind==SHOT_SPIKE_TRIPLE){
 // Native Destroy emits three oSpike at0/120/240 degrees,11 damage,18 speed.
 for(unsigned j=0;j<3;++j){uint8_t a=(uint8_t)(dead.angle+j*256/3);
  int32_t sx=wrap(dead.x+dirX(a)*10/6,worldWidth()),sy=wrap(dead.y+dirY(a)*10/6,worldHeight());
  int shot=emitBullet(sx,sy,a,6,maxv(1,dead.damage*11/20),SHOT_SPIKE,75,1,2);
  if(shot>=0){Bullet &p=bullets[shot];p.speed=6*FP;p.vx=dirX(a)*p.speed/FP;p.vy=dirY(a)*p.speed/FP;p.size_percent=2867;p.last_hit=4096;}
 }
}
