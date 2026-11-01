// Construct mechanics; included within namespace nova after the core helpers.
// Limits are shared by deployment and the Support Specialist / Overseer costs.
// Mines and carrier swarms are basic; drones are standard; turrets/allies advanced.
unsigned Game::constructLimit(unsigned kind) const {
 const uint8_t *m=player.mods;if(kind>=C_KIND_COUNT||(m[M_X_HIDDEN_POWER]&&kind!=C_BASTION))return 0;
 unsigned basic=player.body==B_ENGINEER?2:0,advanced=player.body==B_ENGINEER?1:0;
 unsigned doubled=m[M_OVERSEER]?2:1;
 switch(kind){
 case C_DRONE:case C_DEFENSE_DRONE:
  if(!m[M_DRONES]||(kind==C_DEFENSE_DRONE)!=bool(m[M_DEFENSE_DRONES]))return 0;
  return (3+basic+m[M_ASSAULT_DRONES]+m[M_X_FLEET]+m[M_ADVANCED_ENGINEERING]+2*m[M_X_SPECIALIST_DRONE])*doubled;
 case C_MINE:return !m[M_MINES]?0:m[M_X_TACTICAL_BEACON]?1:(unsigned)maxv(1,(3+basic+3*m[M_MINEFIELD]+m[M_X_SPECIALIST_MINE]-m[M_X_CHARGED_MINES])*doubled);
 case C_TURRET:return m[M_TURRET]?1+advanced+m[M_OVERSEER]:0;
 case C_ALLY:case C_INTERCEPTOR:case C_GUARDIAN:
  if(!m[M_ALLY]||kind!=(unsigned)(m[M_GUARDIAN]?C_GUARDIAN:m[M_INTERCEPTOR]?C_INTERCEPTOR:C_ALLY))return 0;
  return 1+advanced+m[M_OVERSEER];
 case C_SWARM:return player.body==B_CARRIER?(4+level/2)*doubled:0;
 case C_SWORD:return player.weapon==W_SWORDS?2*(weaponCannons()+advanced+m[M_OVERSEER]):0;
 case C_BASTION:return player.shield_type==S_BASTION&&!m[M_RAPID_RECONSTRUCTION]?1:0;
 case C_BLADE:return player.weapon==W_BLADE?(unsigned)maxv(1,weapons[W_BLADE].projectiles+stats.projectiles):0;
 default:return 0;
 }
}
unsigned Game::combinedConstructLimits() const {
 return constructLimit(C_DRONE)+constructLimit(C_DEFENSE_DRONE)+constructLimit(C_TURRET)+
        constructLimit(C_ALLY)+constructLimit(C_INTERCEPTOR)+constructLimit(C_GUARDIAN)+constructLimit(C_SWORD)/2+constructLimit(C_BASTION);
}
void Game::damageConstruct(unsigned index,int damage) {
 if(index>=MAX_CONSTRUCTS||damage<=0)return;
 Construct &c=constructs[index];if(!c.active||c.grace_invulnerable||c.hull<=0)return;
 if(c.kind==C_BASTION){
  damage=maxv(1,(int)((int64_t)damage*incomingDamagePercent(true,false)/100));
  if(player.mods[M_BARRIER])damage=minv(damage,25);
  unsigned lost=(unsigned)minv(player.shield,damage);player.shield=(int16_t)maxv(0,player.shield-damage);c.hull=player.shield;
  if(!advanced_constructs[index].state&&!player.mods[M_X_SHIELDED_DRONES])player.shield_energy=(uint16_t)minv(65535,player.shield_energy+(lost*3+2)/4);
  run_stats.damage_taken=addSaturated(run_stats.damage_taken,lost);recordMaelstromDamage(lost);
  emitFeedback(FeedbackDamage,frame,c.x,c.y,lost);
  if(!player.shield){releaseBastionEnergy(lost);player.shield_energy=0;shieldBroken();assembly_clock_[C_BASTION]=0;}
  return;
 }
 if(c.kind==C_SWORD&&daily(DAILY_SWORD))return;
 if(daily(DAILY_NOVA_BROTHERS)&&(c.kind==C_ALLY||c.kind==C_INTERCEPTOR||c.kind==C_GUARDIAN)&&!daily_damage_redirect_){daily_damage_redirect_=1;damagePlayer(maxv(1,damage*40/100));daily_damage_redirect_=0;}
 if(c.kind==C_SWORD)damage=maxv(1,damage-2); // source SwordObject plating2
 int remaining=(int)c.hull-damage;
 if(remaining<=0&&(c.kind==C_DRONE||c.kind==C_DEFENSE_DRONE)&&player.mods[M_GRACE_PROTOCOL]&&!c.grace_cooldown){
  c.hull=1;c.grace_invulnerable=TICK_RATE;c.grace_cooldown=15*TICK_RATE;
 }else c.hull=(int16_t)maxv(-32768,remaining);
}
unsigned Game::constructHullPercent(unsigned kind) const {
 const uint8_t *m=player.mods;unsigned value=100+25*m[M_ELEGANT_CONSTRUCTION];
 if(kind==C_DEFENSE_DRONE)value=value*250/100;
 if(kind==C_GUARDIAN)value=value*150/100;
 if(daily(DAILY_NOVA_BROTHERS)&&(kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN))value=value*175/100;
 if(kind==C_MINE&&m[M_MINEFIELD])value/=2;
 if((kind==C_DRONE||kind==C_DEFENSE_DRONE)&&m[M_ADVANCED_ENGINEERING])value=value*(100+2*level)/100;
 if((kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN)&&m[M_X_EVOLUTION])value=value*(800+level*10)/800;
 if(kind==C_INTERCEPTOR&&m[M_WINGMAN])value=value*3/4;
 if(m[M_X_APEX_MACHINERY])value=value*130/100;
 if(player.body==B_ARCHITECT)value=value*130/100;
 return value;
}
unsigned Game::constructFireRatePercent(unsigned kind) const {
 const uint8_t *m=player.mods;unsigned value=100;
 // allyGetDelay subtracts the source .4 daily integrated bonus from the
 // ordinary 1.0 fire-time multiplier of the unique Super Nova Brother.
 if(daily(DAILY_NOVA_BROTHERS)&&(kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN))value=value*100/60;
 if(m[M_X_SUPPORT_SPECIALIST])value=value*125/100;
 if(m[M_X_APEX_MACHINERY])value=value*130/100;
 if(m[M_X_BURNOUT_REACTORS])value=value*130/100;
 if(player.body==B_ARCHITECT)value=value*150/100;
 if(kind==C_DRONE&&m[M_ASSAULT_DRONES])value=value*120/100;
 if((kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN)&&m[M_X_EVOLUTION])value=value*(400+3*level)/400;
 if(kind==C_INTERCEPTOR&&m[M_WINGMAN])value=value*3/4;
 if(kind==C_TURRET&&m[M_COUNTER_ARTILLERY])value=value*100/65;
 return value;
}
int Game::constructPower(unsigned kind) const {
 const uint8_t *m=player.mods;int power=stats.construct_damage;
 bool drone=kind==C_DRONE||kind==C_DEFENSE_DRONE;
 bool ally=kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN;
 if((kind==C_MINE&&m[M_X_SPECIALIST_MINE])||(kind==C_TURRET&&m[M_X_SPECIALIST_TURRET])||
    (drone&&m[M_X_SPECIALIST_DRONE])||(ally&&m[M_X_SPECIALIST_ALLY]))power=power*115/100;
 if(ally&&daily(DAILY_NOVA_BROTHERS))power*=2;
 if(drone&&m[M_ADVANCED_ENGINEERING])power=power*(100+2*level)/100;
 if(kind==C_MINE){power=power*(100-20*m[M_MINEFIELD])/100;if(player.body==B_ARCHITECT)power=power*130/100;}
 return power;
}
int Game::spawnConstruct(unsigned kind) {
 if(kind>=C_KIND_COUNT||(player.mods[M_X_HIDDEN_POWER]&&kind!=C_BASTION))return -1;
 if(player.body==B_ARCHITECT&&constructCount()&&kind!=C_SWORD)return -1;
 const uint8_t *m=player.mods;
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(!constructs[i].active){
  Construct &c=constructs[i];clearBytes(&c,sizeof(c));c.active=1;c.kind=(uint8_t)kind;c.x=player.x;c.y=player.y;c.angle=player.angle;
  for(unsigned j=0;j<MAX_CONSTRUCTS;++j)if(constructs[j].active&&constructs[j].kind==C_SWORD&&advanced_constructs[j].wielder==i+1)constructs[j].active=0;
  clearBytes(&advanced_constructs[i],sizeof(advanced_constructs[i]));
  c.radius=kind==C_TURRET?6:kind==C_SWARM?3:4;
  int hull=kind==C_SWORD?100:kind==C_BASTION?player.max_shield:kind==C_TURRET?150:kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN?120:kind==C_SWARM?8:kind==C_BLADE?70:25;
  if(kind!=C_BASTION)hull=(int)((int64_t)hull*constructHullPercent(kind)/100);
  c.hull=c.max_hull=(int16_t)clamp(hull,1,32767);
  if(kind==C_BASTION){player.shield=c.hull;player.shield_delay=0;}
  if(kind==C_SWORD){unsigned n=0;for(unsigned j=0;j<i;++j)if(constructs[j].active&&constructs[j].kind==C_SWORD)++n;advanced_constructs[i].side=(uint8_t)(n&1);}
  // Turrets die from their real 5 hull/s degeneration; regeneration can prolong
  // their life. They do not additionally have an arbitrary 30-second timeout.
  c.life=kind==C_MINE?300:kind==C_BLADE?150:0;
  if(kind==C_MINE&&m[M_X_TACTICAL_BEACON]){c.hull=c.max_hull=(int16_t)minv(10000,hull*8);c.life=900;c.radius=10;c.charges=3;}
  c.fire_delay=10;c.vx=dirX(c.angle)*2;c.vy=dirY(c.angle)*2;
  emitEffect(c.x,c.y,FX_SPAWN,8,8,2);
  return (int)i;
 }
 return -1;
}
void Game::updateConstructs() {
 const uint8_t *m=player.mods;
 unsigned wanted[C_KIND_COUNT]={},count[C_KIND_COUNT]={};
 for(unsigned k=0;k<C_KIND_COUNT;k++)wanted[k]=constructLimit(k);
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active){
  Construct &c=constructs[i];
  if(c.kind==C_DRONE&&m[M_DEFENSE_DRONES])c.kind=C_DEFENSE_DRONE;
  if(c.kind==C_ALLY&&(m[M_INTERCEPTOR]||m[M_GUARDIAN]))c.kind=m[M_GUARDIAN]?C_GUARDIAN:C_INTERCEPTOR;
  if(c.kind==C_SWORD&&(advanced_constructs[i].temporary||advanced_constructs[i].wielder))continue;
  if(c.kind!=C_SWARM||!c.charges)++count[c.kind];
 }
 // Independent assembly progress remains ready when auto deployment is off.
 // Minefield saves up the complete cluster before automatic deployment.
 static const uint16_t base_assembly[C_KIND_COUNT]={150,150,60,900,750,750,750,30,45,120,450};
 static const uint8_t order[]={C_BASTION,C_SWORD,C_ALLY,C_INTERCEPTOR,C_GUARDIAN,C_TURRET,C_DRONE,C_DEFENSE_DRONE,C_SWARM,C_MINE};
 for(unsigned k=0;k<sizeof(order);k++){
  unsigned kind=order[k];if(!wanted[kind]||count[kind]>=wanted[kind]){assembly_clock_[kind]=0;continue;}
  unsigned missing=wanted[kind]-count[kind];
  unsigned interval=base_assembly[kind]*100/maxv(10,stats.assembly);
  bool drone=kind==C_DRONE||kind==C_DEFENSE_DRONE;
  bool ally=kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN;
  bool specialist=m[M_X_SPECIALIST_MINE]||m[M_X_SPECIALIST_DRONE]||m[M_X_SPECIALIST_TURRET]||m[M_X_SPECIALIST_ALLY];
  bool matching=(kind==C_MINE&&m[M_X_SPECIALIST_MINE])||(drone&&m[M_X_SPECIALIST_DRONE])||
                (kind==C_TURRET&&m[M_X_SPECIALIST_TURRET])||(ally&&m[M_X_SPECIALIST_ALLY]);
  if(specialist&&!matching)interval=interval*5/3;
  if(kind==C_MINE&&m[M_MINEFIELD])interval=interval*2/3;
  if(kind==C_MINE&&m[M_LOADED_MINES])interval*=2;
  if(kind==C_MINE&&m[M_X_SPECIALIST_MINE])interval=interval*100/115;
  if(drone&&m[M_X_SPECIALIST_DRONE])interval=interval*5/6;
  interval=(unsigned)clamp(interval,1,60000);
  unsigned capacity=(unsigned)minv(60000,interval*minv(MAX_CONSTRUCTS,missing));
  assembly_clock_[kind]=(uint16_t)minv(capacity,assembly_clock_[kind]+1+(construct_clock_?1:0));
  unsigned mask=drone?1:kind==C_MINE?2:kind==C_TURRET?4:ally?8:0;
  bool automatic=!mask||(options.auto_deploy_mask&mask);
  unsigned ready=assembly_clock_[kind]/interval;
  if(kind==C_SWORD)ready=minv(missing,ready*2);
  if(kind==C_BASTION&&player.shield>0)ready=1;
  if(kind==C_MINE&&m[M_MINEFIELD]&&automatic&&!construct_deploy_requested_&&ready<missing)continue;
  if(!automatic&&!construct_deploy_requested_)continue;
  // Subsumption is an explicit early deployment, never an automatic hull tax.
  bool unassembled=drone&&m[M_X_ROCKET_DRONES]&&construct_deploy_requested_&&!ready;
  if(unassembled)ready=1;
  for(unsigned j=0;j<ready&&j<missing;j++){
   int c=spawnConstruct(kind);if(c<0)break;
   if(unassembled){player.hull=(int16_t)maxv(1,player.hull-player.hull/10-5);if(player.shield)player.shield=(int16_t)maxv(1,player.shield-player.shield/10-5);assembly_clock_[kind]=0;}
   else if(kind!=C_SWORD||!(j&1))assembly_clock_[kind]=(uint16_t)maxv(0,(int)assembly_clock_[kind]-(int)interval);
   if(kind==C_MINE&&m[M_MINEFIELD]){constructs[c].angle=(uint8_t)(player.angle+j*256/maxv(1,ready));constructs[c].vx=dirX(constructs[c].angle)*2;constructs[c].vy=dirY(constructs[c].angle)*2;}
  }
 }
 construct_deploy_requested_=false;
 if(construct_clock_)--construct_clock_;
 if(fleet_promotion_clock_)--fleet_promotion_clock_;
 unsigned commanders=0;
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++)if(constructs[i].active&&constructs[i].promoted){
  if(constructs[i].hull>0)++commanders;
  else if(!m[M_GRACE_PROTOCOL]||constructs[i].grace_cooldown)fleet_promotion_clock_=2*TICK_RATE;
 }
 if(m[M_X_FLEET]&&!fleet_promotion_clock_)for(unsigned i=0;i<MAX_CONSTRUCTS&&commanders<2;i++){
  Construct &c=constructs[i];if(!c.active||c.hull<=0||c.promoted||(c.kind!=C_DRONE&&c.kind!=C_DEFENSE_DRONE))continue;
  c.promoted=1;c.promotion_ticks=0;c.hull=(int16_t)minv(32767,c.hull*3);c.max_hull=(int16_t)minv(32767,c.max_hull*3);++commanders;
  emitEffect(c.x,c.y,FX_RING,c.radius+5,10,2);
 }
 for(unsigned i=0;i<MAX_CONSTRUCTS;i++){
  Construct &c=constructs[i];if(!c.active)continue;if(c.age<65535)++c.age;if(c.fire_delay)--c.fire_delay;
  if(c.promoted&&c.promotion_ticks<65535)++c.promotion_ticks;
  if(c.grace_cooldown)--c.grace_cooldown;
  if(c.grace_invulnerable)--c.grace_invulnerable;
  if(c.life)--c.life;
  if(c.kind==C_BASTION)c.hull=player.shield;
  bool expired=(c.kind==C_MINE||c.kind==C_BLADE)&&c.life==0;
  // Catch a destroy request from an effect as well as ordinary damage routes.
  if(c.hull<=0&&(c.kind==C_DRONE||c.kind==C_DEFENSE_DRONE)&&m[M_GRACE_PROTOCOL]&&!c.grace_cooldown){c.hull=1;c.grace_invulnerable=TICK_RATE;c.grace_cooldown=15*TICK_RATE;}
  if(c.hull<=0||expired){
   if(c.kind==C_MINE&&m[M_MINEFIELD]&&c.chain_percent<=100){
    // oConstructMine Destroy collects the connected mine neighborhood before
    // killing it. The source includes the initiating mine in its count.
    uint16_t members=(uint16_t)(1u<<i);uint8_t queue[MAX_CONSTRUCTS];unsigned head=0,tail=1;queue[0]=(uint8_t)i;
    while(head<tail){const Construct &origin=constructs[queue[head++]];unsigned reach=10+origin.radius;
     for(unsigned next=0;next<MAX_CONSTRUCTS;next++){Construct &other=constructs[next];
      if((members&(1u<<next))||!other.active||other.kind!=C_MINE||other.chain_percent>100)continue;
      if(distance2(origin.x,origin.y,other.x,other.y)>(int)((reach+other.radius)*(reach+other.radius)))continue;
      members=(uint16_t)(members|(1u<<next));queue[tail++]=(uint8_t)next;
     }
    }
    for(unsigned k=0;k<tail;k++){Construct &linked=constructs[queue[k]];linked.chain_percent=(uint16_t)(100+4*tail);linked.hull=0;linked.life=1;}
   }
   const Construct dead=c;unsigned kind=dead.kind;int32_t x=dead.x,y=dead.y;c.active=0;
   if(daily(DAILY_NOVA_BROTHERS)&&(kind==C_ALLY||kind==C_INTERCEPTOR||kind==C_GUARDIAN)){
    // destroyConstruct source lines90..98: losing the Brother halves each
    // surviving defense, subtracts10, then clamps it to at least1.
    if(player.hull>0)player.hull=(int16_t)maxv(1,player.hull/2-10);
    if(player.shield>0)player.shield=(int16_t)maxv(1,player.shield/2-10);
    emitEffect(player.x,player.y,FX_HIT,16,12,1);
   }
   if(kind==C_INTERCEPTOR)for(unsigned j=0;j<MAX_CONSTRUCTS;++j)if(constructs[j].active&&constructs[j].kind==C_SWORD&&advanced_constructs[j].wielder==i+1)constructs[j].active=0;
   if(dead.promoted&&m[M_X_FLEET])fleet_promotion_clock_=2*TICK_RATE;
   if(kind==C_MINE){unsigned chain=maxv(100,dead.chain_percent);blast(x,y,30*stats.blast/100,250*constructPower(kind)*chain/10000);
    unsigned bomblets=m[M_LOADED_MINES]?0:6+range(4);for(unsigned j=0;j<bomblets;j++){int n=emitBullet(x,y,(uint8_t)(j*256/bomblets),2,125*constructPower(kind)*chain/10000,SHOT_SHRAPNEL,12);if(n>=0)bullets[n].blast=9;}
    if(m[M_LOADED_MINES]){
     // Reuse the real weapon paths (including immediate beams / blast / swords),
     // with the mine as origin. The player's magazine, charge and burst cadence
     // are restored after the entire detonation, not borrowed by the mine.
     Player saved_player=player;Stats saved_stats=stats;
     uint16_t saved_fraction=fire_fraction_,saved_stored=stored_damage_,saved_calibration=calibration_;
     uint8_t saved_burst=burst_left_,saved_burst_delay=burst_delay_;uint32_t saved_charge=charged_scale_;
     uint32_t saved_vortex=vortex_scale_,saved_vortex_rate=vortex_rate_,saved_vortex_fraction=vortex_growth_fraction_,saved_rancor=rancor_scale_;
     uint16_t saved_vortex_cost=vortex_cost_fraction_;
     bool saved_empowered=forced_empowered_;
     unsigned nominal=weaponProjectiles();if(player.weapon==W_BLASTER||player.weapon==W_TORRENT)nominal/=2;
     unsigned rate=weaponRateMilli();
     if(player.weapon==W_SALVO){
      unsigned stock_period=384000u/maxv(20,stats.fire_rate);if(m[M_X_SINGULAR_STRIKE_NEW])stock_period+=960;if(hasHyperboost())stock_period=stock_period*3/4;
      unsigned stock=4*(weapons[W_SALVO].projectiles+stats.projectiles+(m[M_FUSILLADE]?weapons[W_SALVO].projectiles+1:0));if(m[M_X_SINGULAR_STRIKE_NEW])stock=1;
      rate=(uint32_t)((uint64_t)stock*7680000/maxv(1,stock_period));nominal=1;
     }
     unsigned construct_rate=constructFireRatePercent(C_MINE);
     unsigned effect=100-20*m[M_MINEFIELD];if(m[M_X_SPECIALIST_MINE])effect=effect*115/100;if(player.body==B_ARCHITECT)effect=effect*130/100;effect=effect*chain/100;
     unsigned count=(unsigned)(((2000ull+(uint64_t)(player.weapon==W_PULSE?1:nominal)*rate*construct_rate/100)*effect+50000)/100000);
     // Charged Mines native projectile allocation adds ceil(C*instances/
     // fireTimeApproximated) before the radial emission loop.
     if(m[M_X_CHARGED_MINES]&&!m[M_X_SINGULAR_STRIKE_NEW]&&player.weapon!=W_VORTEX&&player.weapon!=W_THERMAL_LANCE)
      count+=(unsigned)(((uint64_t)dead.weapon_charge*count*rate+59999999999ull)/60000000000ull);
     count=(unsigned)clamp(count,1,65535);unsigned instances=(unsigned)minv(FRIENDLY_BULLETS,count);
     uint8_t old_active[FRIENDLY_BULLETS];for(unsigned n=0;n<FRIENDLY_BULLETS;n++)old_active[n]=bullets[n].active;
     player.x=x;player.y=y;player.vx=dead.vx;player.vy=dead.vy;player.radius=dead.radius;player.firing=true;
     stored_damage_=0;burst_left_=burst_delay_=0;forced_empowered_=false;
     for(unsigned j=0;j<instances;j++){
      unsigned multiplicity=count/instances+(j<count%instances?1:0);
      stats=saved_stats;stats.damage=(int16_t)clamp((int)((int64_t)saved_stats.damage*constructPower(C_MINE)*multiplicity/100),1,32767);
      player.angle=(uint8_t)(dead.angle+j*256/instances);player.fire_delay=0;fire_fraction_=0;player.missiles=1;
      player.charge=(uint16_t)minv(player.weapon==W_THERMAL_LANCE?90:150,dead.charge_ticks/4);
      charged_scale_=dead.weapon_charge;calibration_=saved_calibration;
      if(player.weapon==W_VORTEX){vortex_scale_=dead.vortex_scale?dead.vortex_scale:100000;rancor_scale_=0;releaseVortex(1,true);}else fireWeapon(player.weapon==W_PULSE?0:1,true);
     }
     for(unsigned n=0;n<FRIENDLY_BULLETS;n++)if(!old_active[n]&&bullets[n].active)bullets[n].construct_source=(uint8_t)(i+1);
     player=saved_player;stats=saved_stats;fire_fraction_=saved_fraction;stored_damage_=saved_stored;calibration_=saved_calibration;
     burst_left_=saved_burst;burst_delay_=saved_burst_delay;charged_scale_=saved_charge;forced_empowered_=saved_empowered;
     vortex_scale_=saved_vortex;vortex_rate_=saved_vortex_rate;vortex_growth_fraction_=saved_vortex_fraction;rancor_scale_=saved_rancor;vortex_cost_fraction_=saved_vortex_cost;
    }
   }

   if(m[M_SELF_DESTRUCTION]){
    // Both original branches use .005/.055/.1/7 growth/fade parameters.
    // Burnout changes the damage category; it does not add a long DPS field.
    unsigned scale=700+isqrt32((uint32_t)maxv(1,dead.max_hull)*4000)*globalBlastRadiusPercent()/100;
    int radius=clamp((int)(scale*30/1000),1,100);
    int damage=120+dead.max_hull*3*(m[M_CONCENTRATED_BLAST]?135:100)/100;
    triggerBurnout(dead,damage,radius);
   }
   if((kind==C_DRONE||kind==C_DEFENSE_DRONE)&&m[M_X_PREDATOR_DRONES]){int target=nearestTarget(x,y);if(target>=0){int n=emitBullet(x,y,direction(delta(targetX(target),x,worldWidth()),delta(targetY(target),y,worldHeight())),5,50*constructPower(kind)/100,SHOT_MISSILE,70);if(n>=0)bullets[n].homing=8;}}
   if((kind==C_DEFENSE_DRONE&&m[M_X_FLOTILLA])||(kind==C_GUARDIAN&&m[M_X_NANOTECH_DISSEMINATION]))emitPickup(x,y,PICKUP_HEAL,kind==C_GUARDIAN?12:6);
   continue;
  }
  if(c.kind==C_BASTION){updateAdvancedConstruct(i);continue;}
  int target=nearestTarget(c.x,c.y,c.kind==C_TURRET?140:220);
  bool ally=c.kind==C_ALLY||c.kind==C_INTERCEPTOR||c.kind==C_GUARDIAN;
  unsigned regen_milli=0;
  if(m[M_X_VITAL_BOND])regen_milli=(unsigned)c.max_hull*regenerationPercent()*7/50;
  if(ally&&m[M_X_NANOTECH])regen_milli+=(unsigned)c.max_hull*20;
  if(c.hull<c.max_hull){c.regen_fraction+=regen_milli;unsigned heal=c.regen_fraction/(TICK_RATE*1000);c.regen_fraction%=TICK_RATE*1000;c.hull=(int16_t)minv(c.max_hull,c.hull+heal);}else c.regen_fraction=0;
  unsigned decay_milli=(c.kind==C_TURRET?5000u:0)+(m[M_X_BURNOUT_REACTORS]?8000u:0)+(c.kind==C_SWARM&&c.charges?750u:0);
  bool innate_charge=c.kind==C_MINE&&m[M_LOADED_MINES]&&(player.weapon==W_VORTEX||player.weapon==W_THERMAL_LANCE);
  if(innate_charge){
   decay_milli+=4000;
   unsigned effect=100-20*m[M_MINEFIELD];if(m[M_X_SPECIALIST_MINE])effect=effect*115/100;if(player.body==B_ARCHITECT)effect=effect*130/100;
   if(player.weapon==W_VORTEX){unsigned rate=(m[M_X_CHARGED_MINES]?1000u:750u)*effect/100;rate=rate*constructFireRatePercent(C_MINE)/100;updateVortexMine(i,rate);}
   else {unsigned charge_rate=(m[M_X_CHARGED_MINES]?4u:3u)*2;
    unsigned charge=c.promotion_ticks+charge_rate*effect*constructFireRatePercent(C_MINE);
    c.promotion_ticks=(uint16_t)(charge%10000);c.charge_ticks=(uint16_t)minv(360,c.charge_ticks+charge/10000);
   }
  }

  if(c.kind==C_MINE&&m[M_X_CHARGED_MINES]){
   decay_milli+=2000;int rate=maxv(20,stats.fire_rate);if(hasHyperboost())rate=rate*4/3;
   for(unsigned sub=0;sub<2;sub++){int64_t varying=(int64_t)14000*rate*c.weapon_charge/100000000;
    int increment=(int)(6000+varying*(2500000-(int)c.weapon_charge)/2500000);c.weapon_charge+=(unsigned)maxv(increment,1250*rate/100);c.weapon_charge=(uint32_t)minv(c.weapon_charge,20000000);}
  }
  unsigned decay=c.decay_fraction+decay_milli;c.decay_fraction=(uint16_t)(decay%(TICK_RATE*1000));
  if(decay>=TICK_RATE*1000)damageConstruct(i,decay/(TICK_RATE*1000));
  if(c.kind==C_SWORD){updateAdvancedConstruct(i);continue;}
  if(c.kind==C_GUARDIAN&&m[M_X_MEDI_CHARGE]&&frame%180==0)emitPickup(c.x,c.y,PICKUP_HEAL,6);
  if(c.kind==C_MINE){
   int drag=m[M_X_MORTAR]?250:220;c.vx=c.vx*drag/256;c.vy=c.vy*drag/256;c.x=wrap(c.x+c.vx,worldWidth());c.y=wrap(c.y+c.vy,worldHeight());
   int trigger=m[M_X_CHARGED_MINES]&&target>=0?c.radius+targetRadius(target):24*(100-20*m[M_MINEFIELD])/100;
   if(c.age>15&&target>=0&&targetInRadius(target,c.x,c.y,m[M_X_CHARGED_MINES]?c.radius:(unsigned)trigger)){
    if(m[M_X_TACTICAL_BEACON]){if(!c.fire_delay&&c.charges){blast(c.x,c.y,45,70*constructPower(C_MINE)/100);c.fire_delay=30;if(!--c.charges)c.hull=0;}continue;}
    c.life=1;
   }
   continue;
  }
  if(c.kind==C_DRONE||c.kind==C_DEFENSE_DRONE||c.kind==C_SWARM||c.kind==C_GUARDIAN){
   uint8_t orbit=(uint8_t)(frame*(c.kind==C_SWARM?2:1)+i*37);int radius=c.kind==C_SWARM?27:22;
   int32_t tx=wrap(player.x+dirX(orbit)*radius,worldWidth()),ty=wrap(player.y+dirY(orbit)*radius,worldHeight());
   if(c.kind==C_DRONE&&m[M_X_BATTALION_DRONES]){int side=(i&1)?1:-1;int offset=15+(int)(i/2)*8;tx=wrap(player.x+dirX((uint8_t)(player.angle+side*64))*offset,worldWidth());ty=wrap(player.y+dirY((uint8_t)(player.angle+side*64))*offset,worldHeight());}
   c.x=wrap(c.x+delta(tx,c.x,worldWidth())*FP/3,worldWidth());c.y=wrap(c.y+delta(ty,c.y,worldHeight())*FP/3,worldHeight());
  } else if(c.kind==C_ALLY||c.kind==C_INTERCEPTOR||c.kind==C_BLADE){
   int32_t tx=player.x,ty=player.y;
   if(target>=0&&(c.kind!=C_BLADE||c.age<85)){tx=targetX(target);ty=targetY(target);}
   int dx=delta(tx,c.x,worldWidth()),dy=delta(ty,c.y,worldHeight());c.angle=toward(c.angle,direction(dx,dy),c.kind==C_BLADE?7:4);
   int speed=c.kind==C_INTERCEPTOR||c.kind==C_BLADE?3:2;
   if(approx(dx,dy)>35||c.kind==C_BLADE){c.vx=dirX(c.angle)*speed;c.vy=dirY(c.angle)*speed;}else{c.vx=c.vx*240/256;c.vy=c.vy*240/256;}
   if(daily(DAILY_NOVA_BROTHERS)&&(c.kind==C_ALLY||c.kind==C_INTERCEPTOR)){c.vx=c.vx*115/100;c.vy=c.vy*115/100;}
   c.x=wrap(c.x+c.vx,worldWidth());c.y=wrap(c.y+c.vy,worldHeight());
   if(c.kind==C_BLADE){if(c.age>85&&distance2(c.x,c.y,player.x,player.y)<12*12)c.life=1;
    if(frame%6==0)for(unsigned e=0;e<MAX_ENEMIES+MAX_BOSS_PARTS;e++)if(targetInRadius(e,c.x,c.y,6))damageTarget(e,50*stats.damage/100*stats.construct_damage/100);}
  }
  bool defender=c.kind==C_DEFENSE_DRONE||c.kind==C_GUARDIAN||(c.kind==C_TURRET&&m[M_POINT_DEFENSE]);
  if(defender){
   for(unsigned b=0;b<MAX_BULLETS;b++)if(bullets[b].active&&bullets[b].owner&&distance2(c.x,c.y,bullets[b].x,bullets[b].y)<20*20){
    bullets[b].active=0;if(c.kind==C_TURRET&&m[M_COUNTER_ARTILLERY])blast(c.x,c.y,22,18);break;
   }
   if(c.kind!=C_TURRET&&frame%90==i%90)player.hull=minv(hullRecoveryCap(),player.hull+1);
  }
  if(c.kind==C_INTERCEPTOR){updateInterceptorWeapon(i,target);continue;}
  bool recovering=c.kind==C_SWARM&&player.shield>0&&player.shield<player.max_shield;
  if(recovering){if((frame+i*7)%120==0)player.shield=minv(shieldRecoveryCap(),player.shield+1);}
  else if(target>=0&&!c.fire_delay){
   c.angle=direction(delta(targetX(target),c.x,worldWidth()),delta(targetY(target),c.y,worldHeight()));
   if(c.kind==C_DRONE&&m[M_X_BATTALION_DRONES])c.angle=player.angle;
   int damage=c.kind==C_TURRET?35:c.kind==C_ALLY||c.kind==C_INTERCEPTOR?50:c.kind==C_SWARM?8:c.kind==C_BLADE?25:30;
   unsigned shots=c.kind==C_ALLY||c.kind==C_INTERCEPTOR?3:1,kind=SHOT_BLASTER;
   if(daily(DAILY_NOVA_BROTHERS)&&(c.kind==C_ALLY||c.kind==C_INTERCEPTOR))shots+=2;
   if(c.kind==C_TURRET&&m[M_WAR_MACHINE]){damage=22;kind=SHOT_MISSILE;}
   if(c.kind==C_DRONE&&m[M_ASSAULT_DRONES]){damage=30;kind=SHOT_MISSILE;}
   if(c.promoted)damage*=3;

   for(unsigned j=0;j<shots;j++){int n=emitBullet(c.x,c.y,(uint8_t)(c.angle+((int)j*2-(int)shots+1)*5),4,damage*constructPower(c.kind)/100,kind,45);
    if(n>=0){bullets[n].construct_source=(uint8_t)((i+1)|(c.promoted?0x80:0));if(c.kind==C_ALLY||c.kind==C_INTERCEPTOR)bullets[n].blast=m[M_X_ECHO_STRIKE]?20:8;}
   }
   c.fire_delay=(uint16_t)(c.kind==C_TURRET?5:c.kind==C_ALLY||c.kind==C_INTERCEPTOR?30:c.kind==C_SWARM?20:15);
   if(m[M_OVERCLOCK]&&(c.kind==C_TURRET||c.kind==C_ALLY||c.kind==C_INTERCEPTOR)&&distance2(c.x,c.y,player.x,player.y)<90*90)c.fire_delay=c.fire_delay*2/3;
   c.fire_delay=(uint16_t)maxv(1,c.fire_delay*100/maxv(1,constructFireRatePercent(c.kind)));
   if(c.kind==C_SWARM&&player.shield==0)c.fire_delay=(uint16_t)maxv(1,c.fire_delay/2);
   if(m[M_X_BURNOUT_REACTORS])c.fire_delay=(uint16_t)maxv(1,c.fire_delay*200/(300-c.hull*100/maxv(1,c.max_hull)));
  }
  if(frame%6==0)for(unsigned e=0;e<MAX_ENEMIES+MAX_BOSS_PARTS;e++)if(targetInRadius(e,c.x,c.y,c.radius)){
   if(c.kind!=C_BLADE)damageConstruct(i,2);
   break;
  }
 }
}

namespace {
// H^.075 evaluated by an integer interpolation between powers of two.
// Multiplied by H below, this preserves the native H^1.075 probability with
// <0.43% relative error throughout the signed 32-bit hull range, no libm.
uint32_t spontaneousHullFactor(uint32_t hull) {
 static const uint32_t factors[32]={1048576u,1104529u,1163468u,1225552u,1290948u,1359835u,1432397u,1508831u,1589344u,1674153u,1763488u,1857589u,1956712u,2061124u,2171108u,2286960u,2408995u,2537542u,2672947u,2815579u,2965821u,3124080u,3290784u,3466384u,3651354u,3846194u,4051431u,4267619u,4495344u,4735220u,4987896u,5254055u};
 if(hull<2)return factors[0];
 unsigned shift=0;uint32_t base=1;while(shift<30&&base<=hull/2){base<<=1;++shift;}
 uint32_t fraction=(uint32_t)(((uint64_t)(hull-base)<<16)/base);
 return factors[shift]+(uint32_t)(((uint64_t)(factors[shift+1]-factors[shift])*fraction)>>16);
}
}
void Game::onEnemyDestroyedConstructEffects(const Enemy &e) {
 if(player.mods[M_X_PRIORITY_ZERO]){construct_clock_=(uint16_t)minv(1000,construct_clock_+15);for(unsigned c=0;c<MAX_CONSTRUCTS;c++)if(constructs[c].active)constructs[c].hull=minv(constructs[c].max_hull,constructs[c].hull+1);}
 unsigned stacks=player.mods[M_X_SPONTANEOUS_GENERATION];
 if(!stacks||player.mods[M_X_HIDDEN_POWER]||e.type==E_ASTEROID||e.max_hull<=0)return;
 // spontaneousGeneration: each unit of .00035*H^1.075*stacks has a
 // 75% spawn chance; the fractional final unit scales that chance.
 uint64_t chance=(((uint64_t)(unsigned)e.max_hull*spontaneousHullFactor((unsigned)e.max_hull))>>20)*350*stacks;
 for(unsigned trial=0;chance&&trial<80;trial++){
  unsigned unit=chance>=1000000?1000000:(unsigned)chance;chance-=unit;
  if(range(1000000)>=unit*3/4)continue;
  int slot=spawnConstruct(C_SWARM);if(slot<0)break;
  Construct &c=constructs[slot];c.x=e.x;c.y=e.y;c.charges=1;c.angle=(uint8_t)range(256);c.life=0;
  c.vx=e.vx/2+dirX(c.angle);c.vy=e.vy/2+dirY(c.angle);
 }
}

#include "advanced_constructs.inc"

#include "inherited_weapons.inc"
