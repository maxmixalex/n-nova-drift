// Independent multipart enemies. Literal timers and source setup are recorded
// in docs/evidence/regular_ai.csv. Source frames are paired at 30 Hz; source
// distances/velocities use the ordinary-enemy 1/6 spatial mapping.
case E_BULWARK:{
 if(!e.form){e.form=1;e.phase_timer=180;e.action_timer=10;e.target_x=e.target_y=0;}
 if(e.phase_timer)--e.phase_timer;
 if(e.action_timer)--e.action_timer;
 if(e.target_y>0)--e.target_y;
 if(e.age<90){move(aim,(16*FP*(90-int(e.age)))/90,1);}
 else if(e.phase==1){move(e.attack_angle,51,1);if(!e.phase_timer){e.phase=2;e.phase_timer=(uint16_t)((15+5*rarity+1)/2);}}
 else if(e.phase==2){move(e.attack_angle,5760,1);if(!e.phase_timer){e.phase=0;e.phase_timer=180;}}
 else {move(aim,51,1);if(!e.phase_timer&&distance<100&&distance>15&&absn((int8_t)(aim-e.angle))<20){e.phase=1;e.phase_timer=9;e.attack_angle=aim;emitEffect(e.x,e.y,FX_BEAM,1,9,3,e.x+dirX(aim)*120,e.y+dirY(aim)*120);}}
 updateBossParts(owner);
 for(unsigned j=0;j<MAX_BOSS_PARTS;++j){BossPart &p=boss_parts[j];if(!p.active||p.owner!=owner||p.kind!=BP_BULWARK_BARRIER)continue;
  // Counter energy is actual damage received AFTER the source .25 defenses.
  // Apply two original .995 decay steps and the volatility accumulator.
  for(unsigned step=0;step<2;++step){p.timer=(uint16_t)maxv(0,int(p.timer)*995/1000-1);e.target_x=minv(1000000,e.target_x+256+int(p.timer)*256/375);}
  if(!p.timer){e.target_x=0;e.action_timer=10;}
  unsigned alignment=(unsigned)absn((int8_t)(aim-e.angle));
  bool ready=(int64_t)e.target_x*(256-alignment)>=750*256*256||!e.action_timer;
  if(!e.target_y&&ready&&p.timer&&alignment<77){
   // enemyCounterStoredDamage: power=min(150,sqrt(stored)*raritySize*
   // enemySizeModifier), where raritySize=1-.2*(2-rarity). The source
   // for-loop is i<4+power; its angular step divides by that real count.
   // scaleEnemyStats defaults its independent size modifier to 1. Sprite
   // xyScale(.35/.425/.5) is applied separately and must not enter power.
   unsigned power_q8=(unsigned)minv(150*FP,(int64_t)isqrt32(uint32_t(p.timer)<<16)*(600+200*rarity)/1000);
   unsigned count_q8=4*FP+power_q8,count=(count_q8+FP-1)/FP;
   for(unsigned shot=0;shot<count;++shot){uint8_t a=(uint8_t)(e.angle-1075*256/3600+(int64_t)shot*215*256*256/(360*count_q8));int b=regularShot(e,a,25*FP,maxv(1,power_q8/(2*FP)),SHOT_CELESTIAL,105,4);if(b>=0){bullets[b].blast=(uint8_t)minv(100,8+power_q8/(4*FP));bullets[b].last_hit=(int16_t)minv(30000,power_q8*12/(10*FP));}}
   emitEffect(p.x,p.y,FX_RING,p.radius,17,1);p.timer=0;e.target_x=0;e.target_y=120;e.action_timer=10;
  }
 }
 return true;
}
case E_JUGGERNAUT:{
 const unsigned charge_duration=rarity==2?120:rarity?80:60;
 const unsigned charge_frequency=rarity==2?800:rarity?600:400;
 const unsigned burst_count=rarity==2?7:rarity?5:4;
 const unsigned burst_frequency=rarity==2?75:rarity?80:90;
 const unsigned source_rof=rarity==2?3:rarity?4:5;
 const unsigned barrage_duration=rarity==2?600:400,barrage_downtime=rarity==2?300:400;
 if(!e.form){e.form=1;e.phase_timer=200;e.action_timer=250;e.fire_delay=0;e.action_count=0;e.target_x=11*FP;e.target_y=200;e.summon_timer=500;e.formation_count=1;e.formation_spacing=10;}
 if(e.phase_timer)--e.phase_timer;
 if(e.phase&&!e.phase_timer){e.phase=0;e.phase_timer=(uint16_t)(charge_frequency/2);}
 if(!e.phase&&!e.phase_timer&&absn((int8_t)(aim-e.angle))<24){e.phase=1;e.phase_timer=(uint16_t)(charge_duration/2);e.formation_spacing=10;e.attack_angle=e.angle;}
 int target_speed=e.phase?(rarity==1?6016:5120):(rarity==1?320:256);
 e.target_x+=(target_speed-e.target_x)/(e.phase?(rarity==1?30:40):25);
 move(e.phase?e.attack_angle:aim,e.target_x,1);
 e.formation_angle=(int16_t)(e.formation_angle+(rarity==2?546:4369)); // 1.5 / 12 degrees per source frame.
 if(e.action_timer)--e.action_timer;
 if(!e.action_timer&&!e.action_count){e.action_count=(uint8_t)burst_count;e.fire_delay=0;}
 if(e.action_count){
  if(e.fire_delay)--e.fire_delay;
  if(!e.fire_delay){uint8_t gun=(uint8_t)((uint16_t)e.formation_angle/256);
   if(!rarity){for(unsigned k=0;k<3;++k)regularShot(e,(uint8_t)(gun+k*256/3),4*FP,22,SHOT_JUGGERNAUT,190,3);}
   else regularShot(e,e.angle,10*FP,22,SHOT_JUGGERNAUT,160,3);
   --e.action_count;e.fire_delay=(uint16_t)((source_rof+(e.action_count&1))/2);if(!e.action_count)e.action_timer=(uint16_t)((burst_frequency+1)/2);
  }
 }
 if(e.phase){if(e.formation_spacing)--e.formation_spacing;else{for(unsigned k=0;k<2;++k)regularShot(e,(uint8_t)(e.angle+(k?64:-64)),640,2,SHOT_JUGGERNAUT_ORB,30,5);e.formation_spacing=4;}}
 if(e.summon_timer)--e.summon_timer;
 if(!e.summon_timer){e.formation_count^=1;e.summon_timer=(uint16_t)((e.formation_count?barrage_duration:barrage_downtime)/2);}
 if(e.target_y>0)--e.target_y;
 if(e.formation_count&&!e.target_y){
  const unsigned kind=rarity==2?E_JUGG_MISSILE_HOMING:E_JUGG_MISSILE;
  uint8_t side=(uint8_t)(e.angle+(e.formation_slot?64:-64));
  int child=spawnEnemy(kind,pixel(e.x)+dirX(side)*21/FP,pixel(e.y)+dirY(side)*21/FP);
  if(child>=0){Enemy &m=enemies[child];m.angle=(uint8_t)(e.angle+(e.formation_slot?85:-85));m.damage_scale=e.damage_scale;m.treasure_scale=0;m.target_x=640;m.leader=-1;e.formation_slot^=1;e.target_y=(rarity==2?24:rarity?10:12)/2;}
 }
 return true;
}
case E_JUGG_MISSILE:case E_JUGG_MISSILE_HOMING:{
 const bool homing=e.type==E_JUGG_MISSILE_HOMING;
 if(!e.form){e.form=1;if(!e.target_x)e.target_x=640;e.radius=homing?4:3;e.treasure_scale=0;}
 if(e.age>25){unsigned turn=homing?minv(7,1+(e.age-25)/35):1;e.angle=toward(e.angle,aim,turn);}
 e.vx=dirX(e.angle)*e.target_x/(3*FP);e.vy=dirY(e.angle)*e.target_x/(3*FP);
 e.x+=e.vx;e.y+=e.vy;
 if(e.x<0||e.y<0||e.x>=worldWidth()*FP||e.y>=worldHeight()*FP){e.active=0;return true;}
 if(distance<e.radius+player.radius||e.age>=300)destroyEnemy(owner);
 return true;
}
case E_WORM:{
 if(!e.form){e.form=1;e.target_x=0;e.target_y=896;e.summon_timer=0;}
 e.target_x=(e.target_x+419)&65535; // +/-1.15 source degrees, two source frames.
 unsigned recovery=rarity==2?128:51;e.target_y=maxv(256,e.target_y-int(recovery));
 int swing=dirY((uint8_t)(e.target_x/256))*57/FP;
 move((uint8_t)(aim+swing),(rarity==2?832:rarity?1024:768)*e.target_y/256,rarity==2?3:1);
 updateBossParts(owner);
 for(unsigned j=0;j<MAX_BOSS_PARTS;++j){BossPart &p=boss_parts[j];if(!p.active||p.owner!=owner)continue;
  if(p.kind==BP_WORM_HEAD){
   int pd=approx(delta(player.x,p.x,worldWidth()),delta(player.y,p.y,worldHeight()));
   if(!p.timer&&pd<100){p.timer=1;emitEffect(p.x,p.y,FX_RING,8,18,3);}
   if(!p.timer)continue;
   uint8_t a=p.angle;int32_t x2=p.x+dirX(a)*110,y2=p.y+dirY(a)*110;
   if(p.timer<=70){if((p.timer&3)==0)emitEffect(p.x,p.y,FX_BEAM,1,4,3,x2,y2);}
   else if(p.timer<=125){
    emitEffect(p.x,p.y,FX_BEAM,3,2,1,x2,y2);
    if(sweptHit(p.x,p.y,wrap(x2,worldWidth()),wrap(y2,worldHeight()),player.x,player.y,player.radius+2))e.summon_timer=(uint16_t)minv(60000,int(e.summon_timer)+1200*e.damage_scale/256);
    if(frame%5==0)for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&sweptHit(p.x,p.y,wrap(x2,worldWidth()),wrap(y2,worldHeight()),constructs[c].x,constructs[c].y,constructs[c].radius+2))damageConstruct(c,maxv(1,6*e.damage_scale/256));
   }
   if(++p.timer>275)p.timer=0;
  }else if(p.kind==BP_WORM_BODY&&rarity==1){
   if(p.timer)--p.timer;
   if(!p.timer){p.timer=279;emitEffect(p.x,p.y,FX_RING,4,12,3);}
   else if(p.timer==256){int32_t ox=e.x,oy=e.y;e.x=p.x;e.y=p.y;int b=regularShot(e,(uint8_t)(p.angle+64),14*FP,12,SHOT_WORM_SPINE,90,2);if(b>=0){bullets[b].x=wrap(p.x+dirX((uint8_t)(p.angle+64))*7,worldWidth());bullets[b].y=wrap(p.y+dirY((uint8_t)(p.angle+64))*7,worldHeight());}e.x=ox;e.y=oy;p.timer=225;
   }
  }
 }
 if(e.summon_timer>=1000){damagePlayer(e.summon_timer/1000,false,-1,true);e.summon_timer%=1000;}
 return true;
}
