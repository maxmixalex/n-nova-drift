// Every Interceptor uses the shared weapon implementation. Only the firing
// context is swapped; the Game, collision pools and player's charge are not.
void Game::updateInterceptorWeapon(unsigned index,int target) {
 if(index>=MAX_CONSTRUCTS)return;
 Construct &c=constructs[index];if(!c.active||c.hull<=0||c.kind!=C_INTERCEPTOR)return;
 InheritedWeaponState &w=advanced_constructs[index].weapon;
 if(w.weapon!=player.weapon+1){clearBytes(&w,sizeof(w));w.weapon=(uint8_t)(player.weapon+1);w.thermal_x=100000;w.thermal_y=250000;c.weapon_charge=0;c.vortex_scale=100000;c.vortex_rate=12375000;c.vortex_fraction=0;c.vortex_cost_fraction=0;}
 bool ready=target>=0;const uint8_t *m=player.mods;
 unsigned construct_rate=constructFireRatePercent(C_INTERCEPTOR);
 if(m[M_OVERCLOCK]&&distance2(c.x,c.y,player.x,player.y)<90*90)construct_rate=construct_rate*3/2;
 if(m[M_X_BURNOUT_REACTORS])construct_rate=construct_rate*(300-c.hull*100/maxv(1,c.max_hull))/200;
 // Native regular explosive rounds are independent of the integrated weapon.
 if(w.reserved)--w.reserved;
 if(ready&&!w.reserved){
  unsigned rounds=3+1+(m[M_X_OVERPOWER]?2:0);
  for(unsigned j=0;j<rounds;++j){int shot=emitBullet(c.x,c.y,(uint8_t)(c.angle+((int)j*2-(int)rounds+1)*5),4,50*constructPower(C_INTERCEPTOR)/100,SHOT_BLASTER,45);
   if(shot>=0){bullets[shot].blast=m[M_X_ECHO_STRIKE]?20:8;bullets[shot].construct_source=(uint8_t)(index+1);}}
  w.reserved=(uint16_t)maxv(1,3000/maxv(1,construct_rate));
 }
 if(player.weapon==W_SWORDS){updateSwordWielder(index,target);return;}
 Player saved_player=player;Stats saved_stats=stats;
 const uint16_t old_fire_fraction=fire_fraction_,old_stored=stored_damage_,old_calibration=calibration_,old_innate=innate_charge_fraction_;
 const uint8_t old_burst=burst_left_,old_burst_delay=burst_delay_;
 const uint32_t old_charge=charged_scale_,old_vortex=vortex_scale_,old_vortex_rate=vortex_rate_,old_vortex_fraction=vortex_growth_fraction_,old_rancor=rancor_scale_,old_tx=thermal_x_scale_,old_ty=thermal_y_scale_;
 const uint16_t old_vortex_cost=vortex_cost_fraction_,old_vortex_damage=vortex_damage_fraction_;
 const bool old_empowered=forced_empowered_;
 uint8_t was_active[FRIENDLY_BULLETS];for(unsigned b=0;b<FRIENDLY_BULLETS;++b)was_active[b]=bullets[b].active;
 player.x=c.x;player.y=c.y;player.vx=c.vx;player.vy=c.vy;player.angle=c.angle;player.radius=c.radius;player.hull=c.hull;player.max_hull=c.max_hull;
 player.fire_delay=c.fire_delay;player.charge=w.charge_ticks;player.missiles=w.magazine;player.missile_clock=w.stock_clock;
 stats.damage=(int16_t)clamp((int)((int64_t)stats.damage*constructPower(C_INTERCEPTOR)/100),1,32767);
 // integratedMult is a fire-time multiplier; it never changes the projectile
 // family or creates fake magazine rounds for a different weapon.
 unsigned integrated=m[M_X_OVERPOWER]?2400:3200;
 stats.fire_rate=(int16_t)clamp((int)((int64_t)stats.fire_rate*construct_rate/100),10,32767);
 fire_fraction_=w.fire_fraction;burst_left_=w.burst_left;burst_delay_=w.burst_delay;calibration_=w.calibration;charged_scale_=c.weapon_charge;
 stored_damage_=0;forced_empowered_=false;rancor_scale_=0;thermal_x_scale_=w.thermal_x;thermal_y_scale_=w.thermal_y;innate_charge_fraction_=w.innate_fraction;
 vortex_scale_=c.vortex_scale;vortex_rate_=c.vortex_rate;vortex_growth_fraction_=c.vortex_fraction;vortex_cost_fraction_=c.vortex_cost_fraction;vortex_damage_fraction_=w.damage_fraction;
 // Preserve the exact3.2/2.4 cadence in Q16 instead of truncating an
 // effective100/2.4 fire-rate stat to41 and slowly losing fractional shots.
 auto scale_cadence=[&](uint16_t fraction_before){
  uint64_t period=(uint64_t)player.fire_delay*65536+fire_fraction_;
  if(period>=fraction_before)period-=fraction_before;
  period=period*integrated/1000+fraction_before;
  player.fire_delay=(uint16_t)minv(60000,(unsigned)(period/65536));fire_fraction_=(uint16_t)(period%65536);
 };
 auto shoot=[&](){uint16_t fraction=fire_fraction_;fireWeapon(0,true);scale_cadence(fraction);};
 if(w.release_delay)--w.release_delay;
 bool charged=m[M_CHARGED_SHOT]&&!m[M_X_CHARGED_MINES]&&player.weapon!=W_THERMAL_LANCE&&player.weapon!=W_VORTEX;
 if(player.weapon==W_SALVO){
  // Keep an actual independent stockpile. Empty magazines collect the next
  // source interval; releasing a volley consumes each real stored missile.
  if(!ready||!player.missiles){
   unsigned period=(unsigned)((uint64_t)384000*integrated/(1000*maxv(20,stats.fire_rate)));if(m[M_X_SINGULAR_STRIKE_NEW])period+=960;if(hasHyperboost())period=period*3/4;
   unsigned stock=4*(weapons[W_SALVO].projectiles+stats.projectiles+(m[M_FUSILLADE]?weapons[W_SALVO].projectiles+1:0));if(m[M_X_SINGULAR_STRIKE_NEW])stock=1;
   unsigned clock=player.missile_clock+256;while(clock>=period){clock-=period;player.missiles=(uint16_t)minv(4095,player.missiles+stock);}player.missile_clock=(uint16_t)clock;
  }
 }
 player.firing=ready&&!w.release_delay;
 if(player.weapon==W_VORTEX){
  int32_t before_hull=c.hull;uint16_t before_cost=c.vortex_cost_fraction,before_fraction=c.redirected_fraction;
  if(player.firing){
   if(!w.charging){c.vortex_scale=100000;c.vortex_rate=12375000;c.vortex_fraction=0;c.vortex_cost_fraction=0;w.charge_clock=0;w.charging=1;}
   updateVortexMine(index,1000,1);vortex_scale_=c.vortex_scale;vortex_rate_=c.vortex_rate;vortex_growth_fraction_=c.vortex_fraction;vortex_cost_fraction_=c.vortex_cost_fraction;
   player.charge=(uint16_t)minv(65535,++w.charge_clock);
   for(unsigned sub=0;sub<2;++sub)vortexField(vortexChargeX(),vortexChargeY(),vortex_scale_,weaponDamage()*1000,w.damage_fraction,0);
  }
  uint64_t regen_milli=(m[M_X_VITAL_BOND]?(unsigned)c.max_hull*regenerationPercent()*7/50:0)+(m[M_X_NANOTECH]?(unsigned)c.max_hull*20:0);
  int64_t cost_micro=(int64_t)(before_hull-c.hull)*1000000+((int)c.redirected_fraction-before_fraction)*1000+(int)c.vortex_cost_fraction-before_cost;
  bool economical=w.charge_clock>60&&cost_micro>=0&&(uint64_t)cost_micro*3>=regen_milli*50;
  bool release=economical&&(unsigned)maxv(0,c.hull)*1000>range((unsigned)maxv(1,c.max_hull)*1000/30+1);
  if(w.charging&&(!player.firing||release)){
   uint16_t fraction=fire_fraction_;releaseVortex(0,true);scale_cadence(fraction);w.charging=0;w.charge_clock=0;w.release_delay=player.fire_delay;
  }
 }else if(player.weapon==W_THERMAL_LANCE){
  if(player.firing){
   unsigned modifier=(1000+vortexChargeModifier(true))/2;
   for(unsigned sub=0;sub<2;++sub){unsigned tx=(unsigned)minv(64000000,100000+(uint64_t)825*modifier),ty=(unsigned)minv(64000000,250000+(uint64_t)330*modifier);thermal_x_scale_=(uint32_t)((int64_t)thermal_x_scale_+((int64_t)tx-thermal_x_scale_)/50);thermal_y_scale_=(uint32_t)((int64_t)thermal_y_scale_+((int64_t)ty-thermal_y_scale_)/50);}
   innate_charge_fraction_=(uint16_t)(innate_charge_fraction_+(m[M_CHARGED_SHOT]&&!m[M_X_CHARGED_MINES]?240:200));player.charge=(uint16_t)minv(90,player.charge+innate_charge_fraction_/100);innate_charge_fraction_%=100;
   for(unsigned shot=0;!player.fire_delay&&shot<64;++shot)shoot();
  }else {player.charge=0;innate_charge_fraction_=0;thermal_x_scale_=100000;thermal_y_scale_=250000;}
 }else if(charged){
  bool release=false;
  if(player.firing){
   ++w.charge_clock;player.charge=(uint16_t)minv(65535,player.charge+1);int rate=maxv(20,(int)((uint64_t)stats.fire_rate*1000/integrated));if(hasHyperboost())rate=rate*4/3;
   for(unsigned sub=0;sub<2;++sub){int64_t varying=(int64_t)14000*rate*charged_scale_/100000000;int increment=(int)(6000+varying*(2500000-(int)charged_scale_)/2500000);charged_scale_+=(unsigned)maxv(increment,1250*rate/100);charged_scale_=minv(charged_scale_,20000000);
    // Step0 0x140925f57: native irandom(30*integratedMult*
    // construct/evolution fire-time multipliers)==0, not a fixed hold duration.
    if(player.charge>1&&!range(30*integrated/(10*maxv(1,construct_rate))+1))release=true;
   }
  }
  if(player.charge&&!player.fire_delay&&(!player.firing||release)){shoot();w.charge_clock=0;}
 }else if(player.firing){for(unsigned shot=0;!player.fire_delay&&shot<64;++shot)shoot();}
 if(burst_left_&&burst_delay_&&!--burst_delay_){uint16_t delay=player.fire_delay;shoot();player.fire_delay=delay;if(burst_left_)--burst_left_;if(burst_left_)burst_delay_=1;}
 if(!player.firing)calibration_=(uint16_t)minv(180,calibration_+1);
 c.fire_delay=player.fire_delay;c.weapon_charge=charged_scale_;c.vortex_scale=vortex_scale_;c.vortex_rate=vortex_rate_;c.vortex_fraction=vortex_growth_fraction_;c.vortex_cost_fraction=vortex_cost_fraction_;
 w.fire_fraction=fire_fraction_;w.stock_clock=player.missile_clock;w.magazine=player.missiles;w.charge_ticks=player.charge;w.burst_left=burst_left_;w.burst_delay=burst_delay_;w.calibration=calibration_;w.thermal_x=thermal_x_scale_;w.thermal_y=thermal_y_scale_;w.innate_fraction=innate_charge_fraction_;
 for(unsigned b=0;b<FRIENDLY_BULLETS;++b)if(!was_active[b]&&bullets[b].active)bullets[b].construct_source=(uint8_t)(index+1);
 player=saved_player;stats=saved_stats;fire_fraction_=old_fire_fraction;stored_damage_=old_stored;calibration_=old_calibration;innate_charge_fraction_=old_innate;
 burst_left_=old_burst;burst_delay_=old_burst_delay;charged_scale_=old_charge;vortex_scale_=old_vortex;vortex_rate_=old_vortex_rate;vortex_growth_fraction_=old_vortex_fraction;vortex_cost_fraction_=old_vortex_cost;vortex_damage_fraction_=old_vortex_damage;rancor_scale_=old_rancor;thermal_x_scale_=old_tx;thermal_y_scale_=old_ty;forced_empowered_=old_empowered;
}
