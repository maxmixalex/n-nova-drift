case E_ECLIPSE:{
 if(!e.form){e.form=1;e.action_timer=350;e.phase_timer=0;e.phase_age=0;e.phase=0;e.target_x=525;e.target_y=0;e.fire_delay=2;e.action_count=0;e.attack_angle=0;e.formation_angle=(int16_t)((unsigned)e.angle*256);e.formation_spacing=102;}
 int shield=eclipseShield(owner);unsigned chassis=0;int panels[2]={-1,-1};
 for(unsigned p=0;p<MAX_BOSS_PARTS;++p)if(boss_parts[p].active&&boss_parts[p].owner==owner){const BossPart& q=boss_parts[p];if(q.kind==BP_ECLIPSE_CHASSIS)++chassis;else if(q.kind==BP_ECLIPSE_PANEL&&q.slot>=2&&q.slot<=3)panels[q.slot-2]=(int)p;}
 for(unsigned sub=0;sub<2;++sub){
  if(e.phase_age&&!--e.phase_age){e.attack_angle^=1;if(e.attack_angle)e.phase_age=120;}
  if(e.phase_timer&&!--e.phase_timer){
   unsigned count=4+rarity+(e.action_count&1);e.action_count^=1;
   for(unsigned n=0;n<count;++n){uint8_t angle=(uint8_t)(e.angle+((int)n*2-(int)count+1)*100*256/(720*(int)(count-1)));int at=regularShot(e,angle,9*FP,50,SHOT_ECLIPSE_BOLT,300,5);if(at>=0){Bullet& b=bullets[at];b.tracking_scale_x=b.tracking_scale_y=(uint16_t)(3351u*e.size_scale/256);}}
   e.phase=0;
  }
  if(e.target_y>0&&!--e.target_y)for(unsigned side=0;side<2;++side)if(panels[side]>=0){
   uint8_t a=(uint8_t)(e.angle+(side?64:-64));int at=regularShot(e,a,5*FP,40,SHOT_ECLIPSE_ORB,30,8);
   if(at>=0){Bullet& b=bullets[at];b.tracking_scale_x=b.tracking_scale_y=(uint16_t)(1302u*e.size_scale/256);b.tracking_weapon=(uint8_t)rarity;b.size_percent=(uint16_t)(318u*e.size_scale/2560);}
  }
  if(e.action_timer&&!--e.action_timer){e.action_timer=350;e.phase=1;e.phase_timer=180;e.phase_age=60;}
  if(e.target_x>0&&!--e.target_x){e.target_x=525;e.target_y=180;}
  if(e.fire_delay)--e.fire_delay;
  if(!e.fire_delay){e.fire_delay=(uint16_t)(5-rarity);
   if(shield<0&&e.attack_angle){unsigned side=e.action_count&1;e.action_count^=1;if(panels[side]>=0){
    const BossPart& panel=boss_parts[panels[side]];const BossPartArt& art=bossPartArt(panel.kind);
    int cx=(int)((int64_t)art.center_x*panel.source_scale/256),cy=(int)((int64_t)art.center_y*panel.source_scale/256);if(panel.mirror)cy=-cy;
    int32_t px=panel.x-(cx*dirX(e.angle)-cy*dirY(e.angle))/FP,py=panel.y-(cx*dirY(e.angle)+cy*dirX(e.angle))/FP;
    for(unsigned n=0;n<2;++n){int degrees=n?(side?-215:215):(side?45:-45);uint8_t a=(uint8_t)(e.angle+degrees*256/360);
     int at=regularShot(e,a,12*FP+(int)range(3*FP+1),12,SHOT_ECLIPSE_TORRENT,60,4);
     if(at>=0){Bullet& b=bullets[at];int offset_degrees=n?60:75,offset=n?(side?-135:135):(side?-10:10);uint8_t mount=(uint8_t)(e.angle+offset_degrees*256/360);
      b.x=wrap(px+(int64_t)dirX(mount)*offset*e.size_scale/(6*256),worldWidth());b.y=wrap(py+(int64_t)dirY(mount)*offset*e.size_scale/(6*256),worldHeight());
      b.tracking_scale_x=(uint16_t)(2662u*e.size_scale/256);b.tracking_scale_y=(uint16_t)(2048u*e.size_scale/256);b.last_hit=a;b.tracking_seek=(uint16_t)(640+range(513));b.tracking_weapon=(uint8_t)(70+range(48));b.field_scale=range(65536);
     }
    }
   }}
  }
  // Native enemyHuntPlayer consumes rotMod; losing one chassis subtracts .0025.
  int difference=(int16_t)((uint16_t)aim*256-(uint16_t)e.formation_angle);
  e.formation_angle=(int16_t)((uint16_t)e.formation_angle+difference*(chassis>1?75:50)/10000);e.angle=(uint8_t)((uint16_t)e.formation_angle/256);
  if(e.phase&&shield>=0)e.formation_spacing=(uint16_t)(e.formation_spacing*99/100);
  else if(e.formation_spacing>346)e.formation_spacing=(uint16_t)(e.formation_spacing*96/100);
  else e.formation_spacing=(uint16_t)minv(346,e.formation_spacing+2);
  int speed=(int)((int64_t)e.formation_spacing*(1600+7*maxv(0,wave-120))/4800);if(e.stun)speed/=3;
  if(player.shield>0&&player.shield_type==S_TEMPORAL&&shieldDistance2(e.x,e.y)<(int)(activeShieldRadius()*activeShieldRadius()))speed=speed*35/100;
  e.vx=dirX(e.angle)*speed/FP;e.vy=dirY(e.angle)*speed/FP;e.x=wrap(e.x+e.vx/2,worldWidth());e.y=wrap(e.y+e.vy/2,worldHeight());
 }
 updateBossParts(owner);
 if(e.phase_timer){unsigned scale=(180-e.phase_timer)*4096/400;emitEffect(e.x,e.y,FX_PULSAR_CHARGE,1,1,0,(int32_t)scale,e.angle);}
 if(e.target_y>0){unsigned scale=(180-e.target_y)*4096/400;for(unsigned side=0;side<2;++side)if(panels[side]>=0){uint8_t a=(uint8_t)(e.angle+(side?64:-64));int distance=90+15*rarity;emitEffect(wrap(e.x+dirX(a)*distance/6,worldWidth()),wrap(e.y+dirY(a)*distance/6,worldHeight()),FX_PULSAR_CHARGE,1,1,0,(int32_t)scale,a);}}
 return true;
}
