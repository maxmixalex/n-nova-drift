// Original Constructor support actors: 60 Hz source timers, 1/6 world scale.
// Drone Alarm1 heals every eligible nearby oEnemyStandard, not one target.
case E_CONSTRUCTOR_DRONE:{
 bool linked=e.leader>=0&&e.leader<MAX_ENEMIES&&enemies[(unsigned)e.leader].active&&enemies[(unsigned)e.leader].generation==e.leader_generation;
 if(!linked){e.hull=0;destroyEnemy(owner);return true;}
 if(!e.form){e.form=1;e.action_timer=45;}
 followEnemyLeader(e);e.angle=(uint8_t)((uint16_t)e.formation_angle/256);
 for(unsigned sub=0;sub<2;++sub){if(e.action_timer)--e.action_timer;if(e.action_timer)continue;e.action_timer=45;
  for(unsigned j=0;j<MAX_ENEMIES;++j){Enemy &a=enemies[j];
   if(!a.active||a.hull<=0||a.hull>=a.max_hull||a.type==E_CONSTRUCTOR_DRONE||a.type==E_ASTEROID||a.type==E_PULSAR_MINE||a.type==E_STEALTH_MINE||a.type==E_SINGULARITY_MINE||a.type==E_INTERCEPTOR_MINE||a.type==E_DWELLER_CHITIN||a.type==E_JUGG_MISSILE||a.type==E_JUGG_MISSILE_HOMING||a.type==E_CONSTRUCTOR_MISSILE)continue;
   // Source distance_to_object compares bounding boxes. Radius margins retain
   // that edge-to-edge meaning in this port's bounded collision geometry.
   unsigned reach=175/6+e.radius+a.radius;
   int64_t x=(int64_t)e.x-a.x,y=(int64_t)e.y-a.y;
   if(x*x+y*y>=(int64_t)reach*reach*FP*FP)continue;
   if(healEnemy(j,25)){emitEffect(a.x,a.y,FX_RING,a.radius+4,8,2);emitEffect(e.x,e.y,FX_BEAM,1,3,8,a.x,a.y);}
  }
 }
 return true;
}
case E_CONSTRUCTOR_TURRET:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=240;e.action_timer=0;e.phase_age=0;e.action_count=0;e.target_x=500;e.target_y=0;}
 e.angle=toward(e.angle,aim,1);
 e.vx=e.vx*9900/10000;e.vy=e.vy*9900/10000;e.x=wrap(e.x+e.vx,worldWidth());e.y=wrap(e.y+e.vy,worldHeight());
 bool linked=e.leader>=0&&e.leader<MAX_ENEMIES&&enemies[(unsigned)e.leader].active&&enemies[(unsigned)e.leader].generation==e.leader_generation;
 for(unsigned sub=0;sub<2;++sub){
  if(e.phase_age<65535)++e.phase_age;
  if(e.phase_timer)--e.phase_timer;
  if(!e.phase_timer){e.phase^=1;e.phase_timer=e.phase?180:240;e.action_timer=0;}
  if(e.phase){
   if(e.action_timer)--e.action_timer;
   if(!e.action_timer){
    uint8_t side=(uint8_t)(e.angle+((e.action_count++&1)?64:-64));
    int n=spawnEnemy(E_CONSTRUCTOR_MISSILE,pixel(e.x)+dirX(side)*7/(6*FP),pixel(e.y)+dirY(side)*7/(6*FP));
    if(n>=0){Enemy &m=enemies[n];m.angle=e.angle;m.damage_scale=e.damage_scale;m.treasure_scale=0;m.vx=dirX(m.angle)*8/3;m.vy=dirY(m.angle)*8/3;m.leader=-1;}
    e.action_timer=6;
   }
  }
  // Alarm3 creates a physical tether at source frame480. It follows both
  // live objects; it is not a forward gun or a cosmetic-only line.
  if(linked&&e.phase_age>=480){Enemy &leader=enemies[(unsigned)e.leader];e.target_x=minv(150000,e.target_x+500);
   unsigned damage_milli=(unsigned)e.target_x*4/1000;unsigned total=(unsigned)e.target_y+damage_milli;e.target_y=(int32_t)(total%1000);unsigned amount=total/1000;
   unsigned width=(unsigned)maxv(1,e.target_x*80/6000000);
   if(amount){if(sweptHit(e.x,e.y,leader.x,leader.y,player.x,player.y,player.radius+width))damagePlayer((int)amount,false,255,true);
    for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&sweptHit(e.x,e.y,leader.x,leader.y,constructs[c].x,constructs[c].y,constructs[c].radius+width))damageConstruct(c,(int)amount);
   }
  }
 }
 if(linked&&e.phase_age>=480){const Enemy &leader=enemies[(unsigned)e.leader];emitEffect(e.x,e.y,FX_BEAM,(unsigned)maxv(1,e.target_x*80/6000000),2,7,leader.x,leader.y);}
 return true;
}
case E_CONSTRUCTOR_MISSILE:{
 if(!e.form){e.form=1;e.treasure_scale=0;}
 int32_t ox=e.x,oy=e.y;e.vx=dirX(e.angle)*8/3;e.vy=dirY(e.angle)*8/3;e.x+=e.vx;e.y+=e.vy;
 if(e.x<0||e.y<0||e.x>=worldWidth()*FP||e.y>=worldHeight()*FP){e.active=0;return true;}
 bool contact=sweptHit(ox,oy,e.x,e.y,player.x,player.y,player.radius+e.radius);
 for(unsigned c=0;c<MAX_CONSTRUCTS&&!contact;++c)if(constructs[c].active)contact=sweptHit(ox,oy,e.x,e.y,constructs[c].x,constructs[c].y,constructs[c].radius+e.radius);
 if(contact){e.hull=0;destroyEnemy(owner);}
 return true;
}
