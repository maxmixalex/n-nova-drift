// Original oPulsar Other10 and Alarm1/2/3/4/5/7/9. Native alarm units
// are retained here; two substeps avoid rounding 15/45-frame phases.
case E_PULSAR:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=120;e.action_timer=0;e.fire_delay=0;e.target_x=(int32_t)range(360);}
 move((uint8_t)(aim+(distance<90?((owner&1)?64:-64):0)),(576+38*rarity)*(e.phase==4&&e.phase_age<45?7:1),2);
 for(unsigned sub=0;sub<2;sub++){
  if(e.phase==1||e.phase==2)++e.phase_age;
  if(e.phase_timer)--e.phase_timer;
  if(e.phase==0&&!e.phase_timer){e.phase=1;e.phase_timer=30;e.phase_age=0;}
  else if(e.phase==1&&!e.phase_timer){e.phase=2;e.action_count=(uint8_t)(14+9*rarity);e.fire_delay=1;}
  else if(e.phase==2){
   if(e.fire_delay)--e.fire_delay;
   if(!e.fire_delay){
    e.target_x=(e.target_x+120)%360;uint8_t a=(uint8_t)(e.target_x*256/360);
    int shot=regularShot(e,a,333+(int)range(180),0,SHOT_PULSAR_BARRAGE,150,2);
    if(shot>=0){Bullet &b=bullets[shot];b.damage=0;b.x=wrap(e.x+dirX(a)*25/4,worldWidth());b.y=wrap(e.y+dirY(a)*25/4,worldHeight());
     b.traveled=(uint16_t)minv(65535,(int64_t)10*e.damage_scale*(10000+33*maxv(0,wave-120))/2560000);
     b.homing=(uint8_t)(45+25*rarity+(rarity==2?5:0));b.size_percent=1024;b.last_hit=737;b.field_scale=5000;b.tracking_seek=128;b.tracking_scale_x=(uint16_t)(a*256);b.tracking_target=0;b.tracking_scale_y=0;
    }
    if(e.action_count)--e.action_count;
    if(!e.action_count){e.phase=3;e.phase_timer=45;}
    else e.fire_delay=(uint16_t)(4+range(5));
   }
  }else if(e.phase==3&&!e.phase_timer){
   e.phase=4;e.phase_timer=(uint16_t)(rarity?480:330);e.phase_age=0;e.action_count=(uint8_t)(rarity?3:0);e.fire_delay=(uint16_t)(rarity?130:0);e.action_timer=(uint16_t)(rarity?0:220);
  }else if(e.phase==4){
   ++e.phase_age;
   if(e.fire_delay&&!--e.fire_delay&&e.action_count){
    int damage=(int)((uint64_t)(rarity==2?24:16)*e.damage_scale*(10000+33*maxv(0,wave-120))/2560000);
    int shot=emitBullet(e.x,e.y,e.angle,0,damage,SHOT_PULSAR_PULSE,15,1,3);
    if(shot>=0){Bullet &b=bullets[shot];b.vx=b.vy=0;b.speed=0;b.size_percent=(uint16_t)((325+125*rarity)*4096/1000);b.field_scale=0;b.traveled=0;b.homing=1;b.tracking_target=(int16_t)owner;b.tracking_seek=e.generation;}
    if(!--e.action_count)e.action_timer=60;else e.fire_delay=15;
   }
   if(e.action_timer&&!--e.action_timer){if(summon(E_PULSAR_MINE,0,1,0,0)<0)e.action_timer=1;}
   if(!e.phase_timer){e.phase=1;e.phase_timer=30;e.phase_age=0;}
  }
 }
 if((e.phase==1||e.phase==2)&&e.phase_age<110+55*rarity){
  unsigned scale=minv(737,e.phase_age*6144/1000); // .0015 growth, .18 cap, Q12.
  static const int mount[4]={135,-135,165,-165};
  for(unsigned j=0;j<4;j++){uint8_t a=(uint8_t)(e.angle+mount[j]*256/360);int distance=j<2?40:45;
   emitEffect(wrap(e.x+dirX(a)*distance/6,worldWidth()),wrap(e.y+dirY(a)*distance/6,worldHeight()),FX_PULSAR_CHARGE,1,1,0,(int32_t)scale,e.angle);
  }
 }else if(e.phase==4&&rarity&&e.phase_age<180){
  unsigned scale=minv(1638,e.phase_age*12288/1000); // .003 growth, .4 cap.
  emitEffect(e.x,e.y,FX_PULSAR_CHARGE,1,1,1,(int32_t)scale,e.angle);
 }
 return true;
}
