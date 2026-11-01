// Source event rules and remaining geometry mappings: docs/minor-enemies.md.
// Each actor uses its existing fixed state; no per-frame allocation.
case E_HIVE_DRONE:case E_HIVE_DRONE_HOT:{
 const bool hot=e.type==E_HIVE_DRONE_HOT;
 const int base=hot?896:717; // 3.5 / 2.8 source px per 60 Hz frame, Q8.
 if(!e.form){e.form=1;e.target_x=maxv(base,approx(e.vx,e.vy)*3);e.attack_angle=(uint8_t)range(256);}
 e.target_x=maxv(base,e.target_x-92); // moveSpeed=max(baseMoveSpeed,moveSpeed-.18), twice.
 e.angle=toward(e.angle,aim,hot?5:4);
 int speed=e.target_x/3;
 if(e.stun)speed/=3;
 if(player.shield>0&&player.shield_type==S_TEMPORAL&&shieldDistance2(e.x,e.y)<(int)(activeShieldRadius()*activeShieldRadius()))speed=speed*35/100;
 e.vx=dirX(e.angle)*speed/FP;e.vy=dirY(e.angle)*speed/FP;
 // Source declumps every three source frames. Keep a compact, bounded separation pass.
 if((e.age*2)%3<2)for(unsigned j=0;j<MAX_ENEMIES;++j)if(j!=owner&&enemies[j].active&&(enemies[j].type==E_HIVE_DRONE||enemies[j].type==E_HIVE_DRONE_HOT)){
  int sx=delta(e.x,enemies[j].x,worldWidth()),sy=delta(e.y,enemies[j].y,worldHeight());
  int gap=e.radius+enemies[j].radius;
  if(absn(sx)<gap&&absn(sy)<gap&&sx*sx+sy*sy<gap*gap){e.vx+=clamp(sx,-3,3)*8;e.vy+=clamp(sy,-3,3)*8;}
 }
 e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());return true;
}
case E_HOST_DRONE_3:{
 if(e.leader<0){destroyEnemy(owner);return true;}
 if(e.leader>=0&&(e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation)){destroyEnemy(owner);return true;}
 if(e.leader>=0)followEnemyLeader(e);
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=150;e.target_x=e.angle*256;e.target_y=0;e.heal_fraction=0;}
 int turn=(int16_t)(aim*256-e.target_x);int rate=e.phase==2||e.phase==3?145:1092; // .4 or3 degrees/source-frame.
 e.target_x=(uint16_t)(e.target_x+clamp(turn,-rate,rate));e.angle=(uint8_t)(e.target_x/256);
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase_timer){
  if(e.phase==0){e.phase=1;e.phase_timer=75;}
  else if(e.phase==1){e.phase=2;e.phase_timer=59;e.phase_age=0;e.target_y=0;}
  else if(e.phase==2){e.phase=3;e.phase_timer=16;}
  else {e.phase=0;e.phase_timer=135;}
 }
 if(e.phase==1){if((e.phase_timer&3)==0)emitEffect(e.x,e.y,FX_RING,3+(75-e.phase_timer)/12,4,3);}
 if(e.phase==2){
  // oEnemyBeamFront grows .25 x-scale/frame to lengthMult10*sizeMod.75.
  unsigned age=59-e.phase_timer;int length=minv(594,6+(int)age*40);
  unsigned halfwidth=age<3?1:2;
  if(e.phase_timer<=4)halfwidth=1;
  int32_t x2=e.x+dirX(e.angle)*length,y2=e.y+dirY(e.angle)*length;
  emitEffect(e.x,e.y,FX_BEAM,halfwidth,2,3,x2,y2);
  unsigned milli=300u*e.damage_scale/256; // .15 HP/source-frame, independent of normal hit grace.
  if(lineHit(e.x,e.y,x2,y2,player.x,player.y,player.radius+halfwidth)){
   e.heal_fraction=(uint16_t)(e.heal_fraction+milli);if(e.heal_fraction>=1000){damagePlayer(e.heal_fraction/1000,false,-1,true);e.heal_fraction%=1000;}
  }
  e.target_y+=(int)milli;unsigned whole=(unsigned)e.target_y/1000;e.target_y%=1000;
  if(whole)for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&lineHit(e.x,e.y,x2,y2,constructs[c].x,constructs[c].y,constructs[c].radius+halfwidth))damageConstruct(c,(int)whole);
 }
 return true;
}
case E_CELESTIAL_DRONE:{
 if(e.leader<0||e.leader>=MAX_ENEMIES||!enemies[(unsigned)e.leader].active||enemies[(unsigned)e.leader].generation!=e.leader_generation){destroyEnemy(owner);return true;}
 Enemy &leader=enemies[(unsigned)e.leader];unsigned slots=maxv(1,e.formation_count);
 unsigned period_q8=leader.elite==2?2560:4096+2560*e.formation_slot/(slots+1);
 if(!e.form){e.form=1;e.target_x=13107;e.target_y=4*65536;e.fire_delay=(uint16_t)((period_q8+60*256+511)/512);e.action_timer=0;}
 for(unsigned sub=0;sub<2;++sub){
  e.target_x+=((leader.phase==1?65536:13107)-e.target_x)/70;
  e.target_y+=((leader.phase==1?12:4)*65536-e.target_y)/35;
  e.formation_angle=(int16_t)(e.formation_angle+e.target_y/360);
 }
 uint8_t orbit=(uint8_t)((uint16_t)e.formation_angle/256+e.formation_slot*256/slots);
 if(leader.phase==2){e.x=wrap(e.x+delta(leader.x,e.x,worldWidth())*FP*15/64,worldWidth());e.y=wrap(e.y+delta(leader.y,e.y,worldHeight())*FP*15/64,worldHeight());e.phase_timer=55;}
 else {
  int32_t ex=(int32_t)((int64_t)dirX(orbit)*20*e.target_x/65536),ey=dirY(orbit)*20;
  int32_t tx=wrap(leader.x+(ex*dirX(leader.angle)-ey*dirY(leader.angle))/FP,worldWidth());
  int32_t ty=wrap(leader.y+(ex*dirY(leader.angle)+ey*dirX(leader.angle))/FP,worldHeight());
  if(e.phase_timer){--e.phase_timer;e.x=wrap(e.x+delta(tx,e.x,worldWidth())*FP/8,worldWidth());e.y=wrap(e.y+delta(ty,e.y,worldHeight())*FP/8,worldHeight());}
  else {e.x=tx;e.y=ty;}
 }
 e.angle=leader.angle; // Source Step copies the leader image_angle.
 if(e.fire_delay)--e.fire_delay;
 if(!e.fire_delay){
  unsigned next=period_q8+e.action_timer;e.fire_delay=(uint16_t)maxv(1,next/512);e.action_timer=(uint16_t)(next%512);
  if(leader.phase==0){int b=regularShot(e,e.angle,18*FP,18,SHOT_CELESTIAL_DRONE,180,3);if(b>=0){bullets[b].size_percent=2662;bullets[b].last_hit=2662;bullets[b].field_scale=65536;}}
 }
 return true;
}
case E_PULSAR_MINE:{ // sPulsarMine / hullPulsarMine belongs to oTriangleMine, not oPulsarBarrageMine.
 if(!e.form){e.form=1;e.angle=(uint8_t)range(256);e.phase_timer=60;e.attack_angle=(uint8_t)(range(2)?1:0);}
 int rotation=e.attack_angle?3:-3;e.angle=(uint8_t)(e.angle+rotation);
 e.vx=e.vx*1521/1600;e.vy=e.vy*1521/1600; // .975 per source frame.
 e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());
 if(e.phase_timer)--e.phase_timer;
 if(!e.phase_timer||distance<=e.radius+player.radius){destroyEnemy(owner);return true;}
 return true;
}
case E_SHREDDER_MINE:{
 if(!e.form){e.form=1;e.phase=0;e.phase_age=0;e.target_x=358;e.target_y=250;}
 // Source radius175 is a bbox edge distance; compact circles preserve the clear warning interval.
 if(!e.phase&&distance<=30+e.radius+player.radius){e.phase=1;e.phase_age=0;}
 if(e.phase){e.phase_age=(uint16_t)minv(65535,e.phase_age+2);e.target_x=maxv(0,358-(int)e.phase_age*256/30);e.target_y=250+(int)e.phase_age*50;
  if((e.phase_age%10)<2)emitEffect(e.x,e.y,FX_RING,e.radius+4,5,1);
  if(e.target_y>=6000){destroyEnemy(owner);return true;}
 }
 move(aim,e.target_x,1);return true;
}
