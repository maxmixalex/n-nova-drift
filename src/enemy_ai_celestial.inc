// Celestial Other10 / Alarms0..9: source phase1/2/3 -> engine0/1/2.
// Quarter-source-frame charge clocks preserve the native .25/.5 timings.
case E_CELESTIAL:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=1800;e.phase_age=0;e.fire_delay=0;e.action_timer=0;e.action_count=0;e.formation_count=0;e.summon_timer=100;e.summon_interval=(uint16_t)(rarity==2?8:17);e.formation_slot=(uint8_t)range(4);e.formation_angle=(int16_t)range(65536);e.target_x=0;e.target_y=0;}
 unsigned count=2+rarity;
 if(e.formation_count<count&&summon(E_CELESTIAL_DRONE,e.formation_count,count,20)>=0)++e.formation_count;
 const unsigned charge=rarity==2?170:rarity==1?185:225;
 for(unsigned sub=0;sub<2;++sub){
  if(e.phase_age<65535)++e.phase_age;
  if(e.phase_timer>4)e.phase_timer-=4;else e.phase_timer=0;
  if(!e.phase_timer){
   e.phase=(uint8_t)((e.phase+1)%3);e.phase_age=0;e.action_timer=0;e.attack_angle=aim;
   if(e.phase==1){e.phase_timer=1320;e.fire_delay=60;e.action_count=(uint8_t)(4+rarity);}
   else if(e.phase==2){e.phase_timer=160;e.fire_delay=0;}
   else {e.phase_timer=1800;e.fire_delay=0;e.summon_timer=100;e.summon_interval=(uint16_t)(rarity==2?8:17);e.formation_slot=(uint8_t)((e.formation_slot+1)&3);e.target_x=0;e.target_y=0;}
  }
  if(e.phase==1){
   if(e.action_timer){
    if(e.action_timer>4)e.action_timer-=4;else{
     e.action_timer=0;
     for(unsigned shot=0;shot<3;++shot){bool mini=shot<2;int n=regularShot(e,e.attack_angle,(mini?7:14)*FP,mini?20:50,mini?SHOT_CELESTIAL_STRIKE_MINI:SHOT_CELESTIAL_STRIKE,180,mini?3:5);
      if(n>=0){Bullet &b=bullets[n];b.size_percent=(uint16_t)(mini?5:10);b.tracking_scale_x=(uint16_t)(mini?350:575);b.tracking_scale_y=0;b.field_scale=5*65536;b.tracking_target=0;b.homing=(uint8_t)(shot==1?2:1);b.last_hit=(int16_t)maxv(1,30*e.damage_scale/256);b.blast=0;}
     }
     emitEffect(e.x,e.y,FX_MUZZLE,14,5,1);e.vx-=dirX(e.attack_angle);e.vy-=dirY(e.attack_angle);
    }
   }
   if(e.action_count){
    if(e.fire_delay>4)e.fire_delay-=4;else{
     --e.action_count;e.attack_angle=aim;e.action_timer=(uint16_t)charge;e.fire_delay=(uint16_t)(charge+28);
     if(!e.action_count)e.phase_timer=360; // Last charge -> native Alarm0=90.
     emitEffect(e.x,e.y,FX_RING,12,(charge+7)/8,1);
    }
   }
  }else if(e.phase==0){
   // The normal variant relies on its drones; native Alarm5 is a rare attack.
   if(rarity&&e.phase_age>=100){
    if(e.fire_delay)--e.fire_delay;
    if(!e.fire_delay){uint8_t a=(uint8_t)(e.angle+((e.action_count++&1)?85:-85));int n=regularShot(e,a,10*FP,18,SHOT_CELESTIAL_DRONE,120,2);
     if(n>=0){bullets[n].size_percent=2662;bullets[n].last_hit=2662;bullets[n].field_scale=65536;}
     e.fire_delay=(uint16_t)maxv(1,e.summon_interval);if(e.summon_interval>1)--e.summon_interval;
    }
   }
   if(e.summon_timer)--e.summon_timer;
   if(!e.summon_timer){
    e.summon_timer=(uint16_t)maxv(1,range(6000)/1000+range(28000)/1000);
    int32_t x=0,y=0;unsigned progress=(unsigned)e.target_x;
    // Native line interpolation and outward/inward spiral phases. Bounds are
    // mapped to the active viewport, so warning positions remain on screen.
    if(e.formation_slot<2){
     x=(int32_t)((uint64_t)minv(progress,65536u)*(worldWidth()-1)*FP/65536);
     y=(int32_t)((uint64_t)(e.formation_slot?65536-minv(progress,65536u):minv(progress,65536u))*(worldHeight()-1)*FP/65536);
     e.target_x+=(int32_t)(1311+range(3933));
     if(e.target_x>65536)e.target_x=0;
    }else{
     unsigned diagonal=isqrt32((uint32_t)(worldWidth()*worldWidth()+worldHeight()*worldHeight()));
     int increment=maxv(1,(int)diagonal*55/3500),dist=e.formation_slot==2?e.target_y:maxv(0,(int)diagonal*55/100-e.target_y);
     uint8_t a=(uint8_t)((uint16_t)e.formation_angle/256);
     x=worldWidth()*FP/2+dirX(a)*dist;y=worldHeight()*FP/2+dirY(a)*dist;
     e.target_y+=increment;e.formation_angle=(int16_t)(e.formation_angle+(int)(8+range(8))*65536/360);
     if(e.target_y>(int)diagonal*55/100)e.target_y=0;
    }
    if(x>=0&&y>=0&&x<worldWidth()*FP&&y<worldHeight()*FP){
     unsigned scale=32768+range(42599)*e.size_scale/256;
     int n=emitBullet(x,y,0,0,(int)((uint64_t)80*scale*e.damage_scale/(65536*256)),SHOT_CELESTIAL_WARNING,90,1,1);
     if(n>=0){Bullet &b=bullets[n];b.vx=b.vy=0;b.speed=0;b.field_scale=scale;b.tracking_scale_x=(uint16_t)maxv(1,scale/1600);b.tracking_seek=0;b.pierce=0;b.wraps=0;b.size_percent=1;}
    }
   }
  }
 }
 if(e.phase==2)move(e.attack_angle,45*FP,2);
 else if(e.phase==1)move(e.angle,0,2);
 else move((uint8_t)(aim+64),rarity==1?1920:1728,2);
 return true;
}
