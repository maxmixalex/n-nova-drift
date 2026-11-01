// Original Celestial warning + shrinking damaging burst. The field has a
// per-target hit list and never collides while the150-source-frame warning runs.
if(b.owner&&b.kind==SHOT_CELESTIAL_WARNING){
 if(b.life)--b.life;
 if(!b.life){b.active=0;continue;}
 unsigned source_age=(unsigned)b.age*2;
 if(!b.pierce&&source_age<150){
  for(unsigned sub=0;sub<2;++sub)b.tracking_scale_x=(uint16_t)minv(65535,(int)b.tracking_scale_x+((int)(b.field_scale/16)-(int)b.tracking_scale_x)/50);
  b.size_percent=(uint16_t)maxv(1,(unsigned)b.tracking_scale_x*100/4096);
  continue;
 }
 if(!b.pierce){b.pierce=1;b.age=0;b.tracking_seek=0;b.wraps=0;source_age=0;}
 if(source_age>=12){b.active=0;continue;}
 unsigned scale=(unsigned)((uint64_t)b.field_scale*(14-source_age)/10);
 b.size_percent=(uint16_t)maxv(1,scale*100/65536);b.radius=(uint8_t)clamp((int)((uint64_t)scale*200/(6*65536)),1,100);
 if(!b.wraps&&distance2(b.x,b.y,player.x,player.y)<(int)((b.radius+player.radius)*(b.radius+player.radius))){damagePlayer(b.damage,false,255,true);b.wraps=1;}
 for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&!(b.tracking_seek&(1u<<c))&&distance2(b.x,b.y,constructs[c].x,constructs[c].y)<(int)((b.radius+constructs[c].radius)*(b.radius+constructs[c].radius))){b.tracking_seek=(uint16_t)(b.tracking_seek|(1u<<c));damageConstruct(c,b.damage);}
 continue;
}
if(b.owner&&(b.kind==SHOT_CELESTIAL_STRIKE||b.kind==SHOT_CELESTIAL_STRIKE_MINI)){
 unsigned target=b.tracking_scale_x;unsigned size=(unsigned)b.size_percent*10;
 size=minv(target,size+100);b.size_percent=(uint16_t)maxv(1,(size+5)/10);
 b.radius=(uint8_t)clamp((int)(size*(b.kind==SHOT_CELESTIAL_STRIKE?20:16)/1000),2,25);
 int32_t vx=dirX(b.angle)*b.speed/FP,vy=dirY(b.angle)*b.speed/FP;
 if(b.kind==SHOT_CELESTIAL_STRIKE_MINI){
  // Elliptical orbit around the independently advancing base point. Compute
  // offset differences, avoiding a second world-position field per projectile.
  unsigned old_age=(b.age-1)*2;uint8_t old_angle=(uint8_t)((uint16_t)b.tracking_target/256);
  int old_r=(int)(75*FP+old_age*640)/6;
  int ox=dirX(old_angle)*old_r/(4*FP),oy=dirY(old_angle)*old_r/FP;
  for(unsigned sub=0;sub<2;++sub){int delta=(int)(b.field_scale/360);b.tracking_target=(int16_t)(b.tracking_target+(b.homing==2?-delta:delta));b.field_scale=b.field_scale*9925/10000;}
  uint8_t a=(uint8_t)((uint16_t)b.tracking_target/256);int radius=(int)(75*FP+b.age*1280)/6;
  int nx=dirX(a)*radius/(4*FP),ny=dirY(a)*radius/FP;int dx=nx-ox,dy=ny-oy;
  vx+=(dx*dirX(b.angle)-dy*dirY(b.angle))/FP;vy+=(dx*dirY(b.angle)+dy*dirX(b.angle))/FP;
 }
 b.vx=vx;b.vy=vy;
 // Source disposal margin is200 source pixels; screen-bound actor invariants
 // cull at the mapped viewport edge instead of wrapping an attack back in.
 if(b.x+b.vx<0||b.y+b.vy<0||b.x+b.vx>=worldWidth()*FP||b.y+b.vy>=worldHeight()*FP){b.active=0;continue;}
}
