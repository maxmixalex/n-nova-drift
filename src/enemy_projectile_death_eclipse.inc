if(dead.kind==SHOT_ECLIPSE_ORB){
 int at=emitBullet(dead.x,dead.y,dead.angle,0,maxv(1,dead.damage*256/20),SHOT_ECLIPSE_VORTEX,311,1,8);
 if(at>=0){Bullet& field=bullets[at];field.field_scale=150000;field.tracking_scale_x=field.tracking_scale_y=614;field.tracking_weapon=dead.tracking_weapon;field.speed=dead.speed;field.vx=dead.vx;field.vy=dead.vy;field.homing=255;}
 emitEffect(dead.x,dead.y,FX_BURST,10,12,0);
}
