// Original Dweller subordinate state machines. All timers below are in source
// 60 Hz frames; two substeps preserve odd durations at the port's 30 Hz tick.
// Evidence addresses and the remaining spatial adaptations are documented in
// docs/evidence/dweller_ai.csv. No original executable bytes are distributed.
case E_DWELLER_DRONE:{
 if(!e.form){e.form=1;e.target_x=384+(int)range(7*FP);e.target_y=250;e.action_timer=0;e.phase=0;}
 if(!e.phase&&(distance<(394+5)/6+e.radius+player.radius||e.hull<e.max_hull/4)){e.phase=1;e.phase_age=0;}
 if(e.phase){
  e.phase_age=(uint16_t)(e.phase_age+2);e.target_y+=270;e.target_x=maxv(0,e.target_x-56);
  unsigned flash=maxv(1,10-int(e.phase_age/8));if((e.phase_age/flash)&1)e.flash=2;
  if(e.target_y>6000){
   e.hull=0;destroyEnemy(owner);return true;
  }
 }
 int wiggle=dirY((uint8_t)(e.age*4))*3/256;
 unsigned turn=e.phase?maxv(1,3-int(e.phase_age/15)):3;
 move((uint8_t)(aim-64+wiggle),e.target_x,turn);return true;
}
case E_DWELLER_GUARD:case E_DWELLER_SOLDIER:{
 bool soldier=e.type==E_DWELLER_SOLDIER;
 if(!e.form){e.form=1;e.phase=0;e.action_timer=soldier?350:120;e.phase_timer=0;e.target_x=(soldier?4*FP:1152);e.target_y=0;}
 // The Soldier inherits the charge attack, but replaces the Guard's strafe
 // callback with an empty movement callback. A formation leader still moves it.
 if(soldier){if(e.leader>=0&&e.formation)followEnemyLeader(e);else {e.vx=e.vx*31/32;e.vy=e.vy*31/32;e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());}e.angle=toward(e.angle,aim,3);}
 else {
  if(distance>100)e.target_x=minv(1901,e.target_x+5);else e.target_x=maxv(0,e.target_x*970/1000);
  move(aim,e.target_x,3);
 }
 for(unsigned sub=0;sub<2;++sub){
  if(!e.phase){if(e.action_timer)--e.action_timer;if(!e.action_timer){e.phase=1;e.phase_timer=65;e.attack_angle=aim;}}
  else {
   e.attack_angle=e.angle;
   if(e.phase_timer)--e.phase_timer;
   if(!e.phase_timer){
    int shot=regularShot(e,e.angle,16*FP,14,SHOT_DWELLER_GUARD,150,3);
    if(shot>=0){Bullet &b=bullets[shot];b.x=wrap(b.x+dirX(e.angle)*2,worldWidth());b.y=wrap(b.y+dirY(e.angle)*2,worldHeight());b.size_percent=100;b.last_hit=32767;b.traveled=0;}
    e.phase=0;e.action_timer=soldier?600:360;
   }
  }
 }
 if(e.phase&&!(e.age%5))emitEffect(e.x+dirX(e.angle)*3,e.y+dirY(e.angle)*3,FX_RING,2+(65-e.phase_timer)/10,6,3);
 return true;
}
case E_DWELLER_WARRIOR:{
 static const uint16_t duration[]={300,90,565,90};
 if(!e.form){e.form=1;e.phase=2;e.phase_timer=565;e.fire_delay=150;e.action_timer=150;e.action_count=0;e.attack_angle=range(2)?64:192;e.target_x=2*FP;e.phase_age=0;}
 // flank=-1 takes the range-holding branch of flankPlayerSmoothRange:
 // clamp(-5 + .01*sourceDistance, -4, 6), eased by 1/40/frame.
 int wanted=clamp(-5*FP+distance*6*FP/100,-4*FP,6*FP);
 e.target_x+=(wanted-e.target_x)/20;
 e.angle=toward(e.angle,aim,2);
 int velocity=e.target_x/3;e.vx=(e.vx*7+dirX(e.angle)*velocity/FP)/8;e.vy=(e.vy*7+dirY(e.angle)*velocity/FP)/8;
 if(e.formation&&e.leader>=0)followEnemyLeader(e);else {e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());}
 for(unsigned sub=0;sub<2;++sub){
  if(e.fire_delay){--e.fire_delay;continue;}
  ++e.phase_age;
  if(e.phase==0&&e.action_count<2){
   if(e.action_timer)--e.action_timer;
   if(!e.action_timer){
    uint8_t launch=(uint8_t)(e.angle+(e.action_count?85:-85));
    int missile=spawnEnemy(E_DWELLER_CHITIN,pixel(e.x)+dirX(launch)*2/FP,pixel(e.y)+dirY(launch)*2/FP);
    if(missile>=0){Enemy &q=enemies[missile];q.angle=launch;q.damage_scale=e.damage_scale;q.treasure_scale=0;q.target_x=4*FP;q.vx=dirX(launch)*4/3;q.vy=dirY(launch)*4/3;}
    ++e.action_count;e.action_timer=1;
   }
  }else if(e.phase==2&&e.action_count<3){
   if(e.action_timer)--e.action_timer;
   if(!e.action_timer){
    int shot=regularShot(e,aim,8*FP,65,SHOT_DWELLER_ARC,300,2);
    if(shot>=0){Bullet &b=bullets[shot];b.size_percent=10;b.last_hit=655;b.traveled=0;b.blast=0;b.pierce=0;b.homing=0;}
    ++e.action_count;e.action_timer=150;
   }
  }
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer){e.phase=(uint8_t)((e.phase+1)&3);e.phase_timer=duration[e.phase];e.phase_age=0;e.action_timer=150;e.action_count=0;}
 }
 if(!e.fire_delay&&e.phase==2&&e.action_count<3&&!(e.age%8))emitEffect(e.x+dirX(e.angle)*5,e.y+dirY(e.angle)*5,FX_RING,3+(150-e.action_timer)/20,8,3);
 return true;
}
case E_DWELLER_CHITIN:{
 if(!e.form){e.form=1;e.phase=0;e.target_y=250;e.action_timer=25;e.phase_age=0;if(!e.target_x)e.target_x=4*FP;e.treasure_scale=0;}
 if(e.action_timer)--e.action_timer;
 if(!e.phase&&(distance<(275+5)/6+e.radius+player.radius||e.hull<e.max_hull/4)){e.phase=1;e.phase_age=0;}
 if(e.phase){e.phase_age=(uint16_t)(e.phase_age+2);e.target_y+=270;if((e.phase_age/maxv(1,12-int(e.phase_age/8)))&1)e.flash=2;
  if(e.target_y>6000){e.hull=0;destroyEnemy(owner);return true;}}
 // Alarm 0 starts turning after 50 source frames. The native missile then
 // ramps .5 -> .75 degrees/frame and adds .26 source velocity/frame.
 // missile(1,.125) applies friction only when enemySpeed is slowed below 1.
 unsigned turn=0;if(!e.action_timer){unsigned fine=(unsigned)e.formation_angle+182+minv(91,maxv(0,(int)e.age-25)*91/200);turn=fine/256;e.formation_angle=(int16_t)(fine%256);}
 e.target_x=minv(64*FP,e.target_x+133);
 move(aim,e.target_x,turn);return true;
}
