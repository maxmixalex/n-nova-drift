if(dead.kind==SHOT_CELESTIAL_STRIKE||dead.kind==SHOT_CELESTIAL_STRIKE_MINI){
 // Native impact creates a separate blast instance. Deferring its collision
 // to the next update preserves the direct20/50 hit plus the independent30 blast.
 int n=emitBullet(dead.x,dead.y,0,0,maxv(1,dead.last_hit),SHOT_CELESTIAL_WARNING,8,1,1);
 if(n>=0){Bullet &burst=bullets[n];burst.pierce=1;burst.vx=burst.vy=0;burst.speed=0;burst.field_scale=32768;burst.size_percent=70;burst.tracking_seek=0;burst.wraps=0;}
}
