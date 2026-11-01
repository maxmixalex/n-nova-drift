// oSwarmling / oSwarmlingGreen Other10 + enemyChargeUp. The beam's
// 60 Hz charge, growth, duration, recovery and parallel cooldown use two
// source steps per simulation tick. Swarm steering uses bounded local spacing.
case E_SWARMLING:case E_SWARMLING_GREEN:{
 const bool green=e.type==E_SWARMLING_GREEN;
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=0;e.fire_delay=0;e.heal_fraction=0;}
 uint8_t heading=distance>41?aim:distance<36?(uint8_t)(aim+128):(uint8_t)(aim+((owner&1)?64:-64));
 // Keep the source target band (215..245 pixels) and local separation without
 // allocating a flock list. The source's floating steering easing is adapted.
 int separation_x=0,separation_y=0;
 for(unsigned j=0;j<MAX_ENEMIES;j++)if(j!=owner&&enemies[j].active&&(enemies[j].type==E_SWARMLING||enemies[j].type==E_SWARMLING_GREEN)){
  int sx=delta(e.x,enemies[j].x,worldWidth()),sy=delta(e.y,enemies[j].y,worldHeight());
  if(sx*sx+sy*sy<12*12){separation_x+=sx;separation_y+=sy;}
 }
 if(separation_x||separation_y)heading=toward(heading,direction(separation_x,separation_y),16);
 // Direction of travel and the attached attack direction are independent.
 uint8_t facing=e.angle;move(heading,3*FP,3);e.angle=toward(facing,aim,5);
 unsigned visual_length=0;bool charging=false;
 for(unsigned sub=0;sub<2;sub++){
  if(e.fire_delay)--e.fire_delay;
  if(!e.phase&&!e.fire_delay){e.phase=1;e.phase_timer=(uint16_t)(green?7:6);e.attack_angle=e.angle;}
  if(e.phase==1){
   charging=true;
   if(e.phase_timer)--e.phase_timer;
   if(!e.phase_timer){e.phase=2;e.action_timer=0;e.phase_timer=(uint16_t)(15+(green?10:0));e.fire_delay=(uint16_t)(green?25:20);++e.action_count;}
   continue;
  }
  if(e.phase==2){
   if(e.action_timer<15){
    // Original beam scale=.075+age*.25*beamSize; SPRT canvas642.
    unsigned scale_q100000=7500+e.action_timer*(1750+131*rarity);
    unsigned length_q8=(unsigned)((uint64_t)642*scale_q100000*FP/600000);
    unsigned width_q8=(unsigned)((uint64_t)230*(7000+525*rarity)*FP/1200000);
    visual_length=length_q8;
    int32_t end_x=e.x+(int64_t)dirX(e.angle)*length_q8/FP,end_y=e.y+(int64_t)dirY(e.angle)*length_q8/FP;
    unsigned damage=(unsigned)((uint64_t)((green?750:600)+125*rarity)*e.damage_scale*(10000+33*maxv(0,wave-120))/2560000);
    unsigned total=e.heal_fraction+damage;e.heal_fraction=(uint16_t)(total%1000);
    if(total>=1000){
     if(lineHit(e.x,e.y,end_x,end_y,player.x,player.y,player.radius+(width_q8+255)/256))damagePlayer((int)(total/1000),false,(uint8_t)(e.angle+128),true);
     for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active&&lineHit(e.x,e.y,end_x,end_y,constructs[c].x,constructs[c].y,constructs[c].radius+(width_q8+255)/256))damageConstruct(c,(int)(total/1000));
    }
   }
   ++e.action_timer;
   if(e.phase_timer)--e.phase_timer;
   if(!e.phase_timer)e.phase=0;
  }
 }
 if(visual_length)emitEffect(e.x,e.y,FX_SWARM_BEAM,2,1,green?1:0,e.x+(int64_t)dirX(e.angle)*visual_length/FP,e.y+(int64_t)dirY(e.angle)*visual_length/FP);
 else if(charging)emitEffect(e.x,e.y,FX_SWARM_CHARGE,6,1,green?1:0,e.x+dirX(e.angle)*8,e.y+dirY(e.angle)*8);
 return true;
}
