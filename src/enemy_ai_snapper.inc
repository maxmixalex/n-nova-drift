case E_SNAPPER:{
 const unsigned quickness=rarity>=2?1200:rarity?1000:750;
 if(!e.form){e.form=1;e.action_timer=120;e.phase_timer=0;e.phase=0;e.action_count=0;e.formation_count=2;e.formation_spacing=1000;e.target_x=(rarity>=2?26:rarity?77:102);e.target_y=(rarity>=2?10:rarity?20:12)*FP;e.formation_angle=(int16_t)((unsigned)e.angle*256);e.summon_timer=0;e.summon_type=ENEMY_COUNT;}
 for(unsigned sub=0;sub<2;++sub){
  if(e.phase_timer&&!--e.phase_timer){e.action_count=0;
   if(rarity>=2)for(unsigned shot=0;shot<7;++shot){int at=regularShot(e,(uint8_t)(e.angle+(int(shot)-3)*256/36),40*FP,24,SHOT_WORM_SPINE,100,3);if(at>=0)bullets[at].size_percent=70;}
  }
  if(e.summon_timer&&!--e.summon_timer){int at=emitBullet(e.x,e.y,e.angle,0,3,SHOT_SNAPPER_BEAM,180,1,1);if(at>=0){Bullet& b=bullets[at];b.vx=b.vy=0;b.speed=e.size_scale;b.tracking_target=(int16_t)owner;b.tracking_seek=e.generation;b.field_scale=(uint32_t)(75000ull*e.size_scale/256);b.tracking_scale_y=(uint16_t)(307u*e.size_scale/256);b.damage=(int)minv(2000000,(int64_t)3*e.damage_scale*(10000+maxv(0,wave-120)*33)/10000);}}
  if(e.action_timer)--e.action_timer;
  if(!e.action_timer){
   if(e.action_count==100){e.phase=1;e.phase_timer=25;e.action_timer=(uint16_t)(140*quickness/1000);}
   else {e.action_count=100;e.action_timer=(uint16_t)(180*quickness/1000);if(rarity>=2){e.summon_timer=46;emitEffect(e.x,e.y,FX_SWARM_CHARGE,1,23,9,e.x+dirX(e.angle)*25,e.y+dirY(e.angle)*25);}}
  }
  unsigned d=approx(delta(player.x,e.x,worldWidth()),delta(player.y,e.y,worldHeight()));
  int desired=e.phase?e.target_y+(int)d*6*FP*13/1000:FP/2+(int)d*6*FP*15/10000;
  e.target_x+=(desired-e.target_x)*2/15;
  uint8_t heading=aim;unsigned panic=e.formation_spacing;
  if(panic>1000){heading=(uint8_t)(aim+128);e.formation_spacing=(uint16_t)maxv(1000,(int)panic-100);}
  if(!e.phase||panic>1000){int angular=(int16_t)((uint16_t)heading*256-(uint16_t)e.formation_angle);int turn=(rarity>=2?120:rarity?125:150)*256*256/36000*(int)panic/1000;angular=clamp(angular,-turn,turn);e.formation_angle=(int16_t)((uint16_t)e.formation_angle+angular);e.angle=(uint8_t)((uint16_t)e.formation_angle/256);}
  int speed=(int)minv(32*FP,(int64_t)e.target_x*panic*(1600+7*maxv(0,wave-120))/(1000*1600*6));
  if(e.stun)speed/=3;
  if(player.shield>0&&player.shield_type==S_TEMPORAL&&shieldDistance2(e.x,e.y)<(int)(activeShieldRadius()*activeShieldRadius()))speed=speed*35/100;
  e.vx=dirX(e.angle)*speed*2/FP;e.vy=dirY(e.angle)*speed*2/FP;
  e.x=wrap(e.x+e.vx/2,worldWidth());e.y=wrap(e.y+e.vy/2,worldHeight());
 }
 updateBossParts(owner);
 return true;
}
