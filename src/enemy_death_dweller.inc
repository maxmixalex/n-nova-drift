// Called once after the dead enemy's slot is deactivated. The source Drone
// only blasts after its proximity countdown; shooting it early is safe.
if((e.type==E_DWELLER_DRONE&&e.target_y>6000)||e.type==E_DWELLER_CHITIN){
 int damage=(int)((int64_t)(e.type==E_DWELLER_DRONE?18:8)*e.damage_scale/256*(10000+maxv(0,wave-120)*33)/10000);
 hostileBlast(e.x,e.y,e.type==E_DWELLER_DRONE?45:52,maxv(1,damage));
}
if(e.type==E_DWELLER_DRONE||e.type==E_DWELLER_SOLDIER||e.type==E_DWELLER_GUARD||e.type==E_DWELLER_WARRIOR){
 // Controller Create: 17500/{25,60,6,.75}. Carry thirds on the parent so
 // multiple subordinate deaths conserve the source fractional reductions.
 unsigned thirds=e.type==E_DWELLER_DRONE?2100:e.type==E_DWELLER_SOLDIER?875:e.type==E_DWELLER_GUARD?8750:70000;
 if(e.leader>=0&&e.leader<MAX_ENEMIES){Enemy &parent=enemies[(unsigned)e.leader];
  if(parent.active&&parent.type==E_DWELLER&&parent.generation==e.leader_generation){
   unsigned carried=thirds+parent.heal_fraction%3;parent.heal_fraction=(uint16_t)(carried%3);
   parent.hull=maxv(0,parent.hull-(int)(carried/3));
   // Native phaseTimer -= damage/40, clamp to 60 source frames. The boss
   // controller stores 30 Hz ticks, so this is damage/80 and a 30-tick floor.
   if(parent.phase_timer>30)parent.phase_timer=(uint16_t)maxv(30,(int)parent.phase_timer-(int)(thirds/240));
   if(!parent.hull)destroyEnemy((unsigned)e.leader);
  }
 }
}
