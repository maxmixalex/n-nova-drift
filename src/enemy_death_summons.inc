// Destruction is the common path for a shot, contact, or a timed fuse.
if(e.type==E_HIVE_DRONE_HOT){
 for(unsigned j=0;j<3;++j){int b=regularShot(e,(uint8_t)(e.attack_angle+j*256/3),9*FP,12,SHOT_SPIKE_MINI,30,1);if(b>=0)bullets[b].size_percent=2867;}
}
if(e.type==E_SHREDDER_MINE){
 static const int normal[6]={30,90,150,210,270,330};
 static const int elite[6]={0,45,120,180,240,315};
 static const int champion[9]={18,45,75,135,180,225,285,315,342};
 const int *angles=e.elite==2?champion:e.elite?elite:normal;unsigned count=e.elite==2?9:6;
 int offset=e.elite==2?140:e.elite?90:70;int32_t sx=e.x,sy=e.y;
 for(unsigned j=0;j<count;++j){uint8_t a=(uint8_t)(e.angle+angles[j]*256/360);e.x=wrap(sx+dirX(a)*offset/6,worldWidth());e.y=wrap(sy+dirY(a)*offset/6,worldHeight());
  int b=regularShot(e,a,20*FP,20,e.elite==2?SHOT_SPIKE_TRIPLE:SHOT_SPIKE,75,2);if(b>=0){bullets[b].size_percent=e.elite==2?2458:2867;bullets[b].last_hit=4096;}}
 hostileBlast(sx,sy,e.radius+7,(6+2*e.elite)*e.damage_scale/256);e.x=sx;e.y=sy;
}
if(e.type==E_PULSAR_MINE){
 // The separate original spawner fires sixteen three-way volleys after the mine dies.
 hostileBlast(e.x,e.y,25,25*e.damage_scale/256);
 int b=emitBullet(e.x,e.y,e.angle,0,10*e.damage_scale/256,SHOT_TRIANGLE_SPAWNER,30,1,1);
 if(b>=0){bullets[b].vx=bullets[b].vy=0;bullets[b].speed=0;bullets[b].pierce=48;bullets[b].last_hit=2;}
}
