// Source: oUnlockMessage_Create_0 / setLevelExp / confirmLoadout. See evidence.
unsigned offerUnlockRank(unsigned kind,unsigned id) {
 switch(kind){
 case OFFER_WEAPON:switch(id){
  case W_THERMAL_LANCE:return 11;
  case W_BLADE:return 21;
  case W_SWORDS:return 25;
  case W_VORTEX:return 27;
  case W_TORRENT:return 37;
  case W_DART:return 49;
  default:return 0;}
 case OFFER_BODY:switch(id){
  case B_FIREFLY:return 3;
  case B_HULLBREAKER:return 9;
  case B_CARRIER:return 15;
  case B_ARCHITECT:return 29;
  case B_BATTERY:return 41;
  case B_VIPER:return 45;
  case B_COURSER:return 54;
  case B_RESEARCH:return 58;
  case B_LEVIATHAN:return 62;
  default:return 0;}
 case OFFER_SHIELD:switch(id){
  case S_AMP:return 6;
  case S_BASTION:return 18;
  case S_ORBITAL:return 23;
  case S_SIPHON:return 33;
  default:return 0;}
 case OFFER_MOD:switch(id){
  case M_TURRET:return 1;
  case M_ADAPTIVE_ARMOR:return 5;
  case M_KINETIC_BOOST:return 7;
  case M_ALLY:return 10;
  case M_STABILIZATION:return 13;
  case M_SKIRMISH:return 17;
  case M_CANDESCENCE:return 19;
  case M_ELEGANT_CONSTRUCTION:return 22;
  case M_EFFICIENCY:return 26;
  case M_X_PRIORITY_ZERO:return 28;
  case M_X_HIDDEN_POWER:return 30;
  case M_X_VITAL_BOND:return 31;
  case M_X_TEMPEST_BREAK:return 32;
  case M_X_SATURATION_FIRE:return 34;
  case M_X_MORTAR:return 35;
  case M_X_CELESTIAL_SURGE:return 36;
  case M_X_RANCOR:return 38;
  case M_X_CHARGED_MINES:return 39;
  case M_X_SANCTUARY:return 40;
  case M_X_ANNIHILATION:return 42;
  case M_X_APEX_MACHINERY:return 43;
  case M_X_DYING_STAR:return 44;
  case M_X_WARP_STRIKE:return 46;
  case M_X_BURNOUT_REACTORS:return 47;
  case M_X_LAST_STAND:return 48;
  case M_X_BARRAGE:return 51;
  case M_X_DISPLACEMENT:return 52;
  case M_X_LEAF_ON_THE_WIND:return 53;
  case M_X_SINGULAR_STRIKE_NEW:return 55;
  case M_X_DEFLAGRATION:return 56;
  case M_X_MASTERY:return 57;
  case M_X_CHARGED_SHIELDS:return 59;
  case M_X_INFUSE:return 60;
  case M_X_STRAFING_STRIKES:return 61;
  case M_X_APOTHEOSIS:return 63;
  case M_X_ATARAXIA:return 64;
  default:return 0;}
 default:return 65;}
}
unsigned modeUnlockRank(unsigned flag) {
 switch(flag){case RUN_DANGER:return 4;case RUN_ENEMY_TERRITORY:return 8;
 case RUN_ELITE:return 12;case RUN_WILD:return 14;case RUN_HOSTILE:return 16;
 case RUN_NEMESIS:return 20;case RUN_MAYHEM:return 24;case RUN_DRAFT:
 case RUN_ANNIHILATION:return 50;case RUN_ENDLESS:return 255;case RUN_PRACTICE:case RUN_DAILY:return 0;
 default:return 65;}
}
void Career::clear(){clearBytes(this,sizeof(*this));}
bool Career::valid() const {return !(seen_enemies_hi>>((ENEMY_COUNT-64)&63))&&rank<=64&&best_wave<=30000&&unlock_points_milli<1000000000u;}
unsigned Career::nextRankPoints() const {
 if(rank>=64)return 0;
 int decade=((int)rank-1)/10; // floor for rank zero has no effect after multiplying by zero.
 return 12+minv(8,3+decade)*rank;
}
bool Career::modeUnlocked(unsigned flag) const {
 return flag==RUN_ENDLESS?final_boss_kills!=0:rank>=modeUnlockRank(flag);
}
bool Career::offerUnlocked(unsigned kind,unsigned id) const {
 return rank>=offerUnlockRank(kind,id);
}
unsigned Career::award(const Game &g) {
 if(g.state!=GameOver||(g.run_flags&RUN_PRACTICE)||(!(g.run_flags&RUN_DAILY)&&last_seed==g.seed&&last_frame==g.frame))return 0;
 last_seed=g.seed;last_frame=g.frame;
 runs=addSaturated(runs,1);total_score=addSaturated(total_score,g.score);
 total_kills=addSaturated(total_kills,g.kills);total_ticks=addSaturated(total_ticks,g.frame);
 best_score=best_score>g.score?best_score:g.score;best_wave=(uint16_t)maxv(best_wave,g.wave);
 total_distance_q8+=g.run_stats.distance_q8;if(g.run_stats.highest_hit>highest_hit)highest_hit=g.run_stats.highest_hit;
 seen_enemies|=g.run_stats.seen_enemies;seen_enemies_hi|=g.run_stats.seen_enemies_hi;
 for(unsigned i=0;i<ENEMY_COUNT;i++)enemy_kills[i]=addSaturated(enemy_kills[i],g.run_stats.enemy_kills[i]);
 final_boss_kills=(uint16_t)minv(65535,final_boss_kills+g.run_stats.enemy_kills[E_STAR_EATER]);
 unsigned points=g.wave>=g.starting_wave?g.wave-g.starting_wave:0;
 if(g.run_flags&RUN_DRAFT){if(g.wave>180)points+=g.wave-180;}
 else {if(g.run_stats.boss_kills)points+=5+10*(g.run_stats.boss_kills-1);if(g.wave>120)points+=g.wave-120;}
 points+=30*g.run_stats.enemy_kills[E_STAR_EATER];
 unsigned bonus=0;for(unsigned bit=RUN_DANGER;bit<=RUN_ENEMY_TERRITORY;bit<<=1)if(g.run_flags&bit)++bonus;
 if(g.run_flags&RUN_MAYHEM)++bonus;
 unlock_points_milli=addSaturated(unlock_points_milli,points*(1000+125*bonus));
 unsigned oldrank=rank;
 while(rank<64&&unlock_points_milli>=nextRankPoints()*1000){unlock_points_milli-=nextRankPoints()*1000;++rank;}
 if(rank==64)unlock_points_milli=0;
 return rank-oldrank;
}
const char *Career::unlockKey(unsigned r) {
 static const char *const keys[]={
 "strModUnlockMessageTurret",
 "strUnlocksTrain",
 "strGearUnlockMessageBodyFirefly",
 "strUnlocksTitleDanger",
 "strModUnlockMessageAdaptiveArmor",
 "strGearUnlockMessageShieldAmp",
 "strModUnlockMessageKineticBoost",
 "strUnlocksTitleEnemyTerritory",
 "strGearUnlockMessageHullbreaker",
 "strModUnlockMessageAlly",
 "strGearUnlockMessageWeaponThermalLance",
 "strUnlocksTitleEliteEnemy",
 "strModUnlockMessageStabilization",
 "strUnlocksTitleWildMetamorphosis",
 "strGearUnlockMessageBodyCarrier",
 "strUnlocksTitleHostileUniverse",
 "strModUnlockMessageSkirmish",
 "strGearUnlockMessageShieldBastion",
 "strModUnlockMessageCandescence",
 "strUnlocksTitleNemesis",
 "strGearUnlockMessageWeaponBladeDrone",
 "strModUnlockMessageElegantConstruction",
 "strGearUnlockMessageShieldHelix",
 "strUnlocksTitleMayhem",
 "strGearUnlockMessageWeaponSwords",
 "strModUnlockMessageEfficiency",
 "strGearUnlockMessageVortex",
 "strModUnlockMessagePriorityZero",
 "strGearUnlockMessageBodyArchitect",
 "strModUnlockMessageHiddenPower",
 "strModUnlockMessageVitalBond",
 "strModUnlockMessageTempestBreak",
 "strGearUnlockMessageShieldSiphon",
 "strModUnlockMessageSaturationFire",
 "strModUnlockMessageMortar",
 "strModUnlockMessageCelestialSurge",
 "strGearUnlockMessageWeaponTorrent",
 "strModUnlockMessageRancor",
 "strModUnlockMessageChargedMines",
 "strModUnlockMessageSanctuary",
 "strGearUnlockMessageBodyBattery",
 "strModUnlockMessageAnnihilation",
 "strModUnlockMessageApexMachinery",
 "strModUnlockMessageDyingStar",
 "strGearUnlockMessageBodyViper",
 "strModUnlockMessageWarpStrike",
 "strModUnlockMessageBurnoutReactors",
 "strModUnlockMessageLastStand",
 "strGearUnlockMessageWeaponDart",
 "strUnlocksTitleAnnihilation",
 "strModUnlockMessageBarrage",
 "strModUnlockMessageDisplacement",
 "strModUnlockMessageLeafOnTheWind",
 "strGearUnlockMessageBodyCourser",
 "strModUnlockMessageSingularStrikeNew",
 "strModUnlockMessageDeflagration",
 "strModUnlockMessageMastery",
 "strGearUnlockMessageBodyResearch",
 "strModUnlockMessageChargedShields",
 "strModUnlockMessageInfuse",
 "strModUnlockMessageStrafingStrikes",
 "strGearUnlockMessageBodyLeviathan",
 "strModUnlockMessageApotheosis",
 "strModUnlockMessageAtaraxia",
 };
 return r>=1&&r<=64?keys[r-1]:"";
}
void Game::configureCareer(unsigned rank,bool defeated) {
 if(frame)return;
 account_rank=(uint8_t)minv(64,rank);final_boss_unlocked=defeated;
 rerolls=3;static const uint8_t gates[]={4,8,12,16,20,24,50};
 for(unsigned i=0;i<sizeof(gates);i++)if(rank>=gates[i])++rerolls;
}
