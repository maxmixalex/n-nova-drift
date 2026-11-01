case E_MYRMIDON:{
 if(!e.form){e.form=1;e.phase=0;e.phase_timer=205;e.phase_age=40;e.action_timer=180;e.target_x=(int32_t)e.angle*256;}
 move((uint8_t)(aim+(distance<100?64:0)),3*FP,2);
 #include "enemy_ai_myrmidon_support.inc"
 for(unsigned sub=0;sub<2;++sub){
  // enemyChargeUp decrements only its cooldown while it is positive.
  // An auto-turret shot can therefore pause, without resetting, a charge.
  if(e.phase_timer){--e.phase_timer;continue;}
  if(!e.phase){
   // Source 3000-pixel target range and the visible40-frame charge.
   if(distance>500||(player.body==B_SPECTRE&&player.cloak>130)){e.phase_timer=1;continue;}
   e.phase=1;e.phase_age=40;e.attack_angle=e.angle;
   emitEffect(e.x,e.y,FX_BEAM,1,20,5,e.x+dirX(e.angle)*160,e.y+dirY(e.angle)*160);
  }
  if(e.phase_age)--e.phase_age;
  if(!e.phase_age){
   unsigned life=(190+30*rarity+range(60)+1)/2;
   int slot=regularShot(e,e.angle,947+38*rarity,20+5*rarity,SHOT_MYRMIDON_BOMB,life,4);
   if(slot>=0){Bullet &b=bullets[slot];b.field_scale=(uint32_t)b.damage;b.size_percent=2048;
    b.tracking_scale_x=(uint16_t)(1229+410*rarity);b.tracking_scale_y=0;b.tracking_seek=e.size_scale;
    b.tracking_target=(int16_t)owner;b.last_hit=(int16_t)e.generation;b.traveled=(uint16_t)((unsigned)b.angle*256);
    b.bounces=b.angle;b.construct_source=(uint8_t)rarity;b.pierce=0;b.blast=0;b.homing=0;
   }
   e.phase=0;e.phase_timer=465;
  }
 }
 return true;
}
