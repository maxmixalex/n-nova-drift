#include "boss_parts.inc"
// Bounded native boss state machines. Source phase tables and literal addresses
// are recorded in docs/evidence/boss_phase_tables.csv and star_eater_phases.csv.
// Source timers run at 60 Hz; this core runs at 30 Hz. Spatial values use the
// same 4.6:1 world reduction as the other native enemy/weapon implementations.
static unsigned novaBossIntro(unsigned type) {
 switch(type){
 case E_STATION_OMEGA:return 480;case E_CERATOTITAN:return 60; case E_SPITFIRE:return 200;
 case E_DREADNOUGHT:return 210; case E_DWELLER:return 180;
 case E_GLAUCUS:return 390; case E_CRUCIBLE:return 270; case E_SERAPH:return 400;
 default:return 120;
 }
}
static unsigned novaBossPhaseCount(unsigned type) {
 switch(type){
 case E_CERATOTITAN:return 14;case E_STAR_EATER:return 11;
 case E_CRUCIBLE:return 9;case E_SCION:return 5;
 case E_SERAPH:return 3;case E_WARBRINGER:return 3;
 default:return 4;
 }
}
static unsigned novaBossDuration(const Enemy &e,unsigned flags) {
 const unsigned p=e.phase;
 switch(e.type){
 case E_CERATOTITAN:{static const uint16_t d[]={1,120,160,17,210,360,300,120,17,120,17,120,17,260};return d[p%14];}
 case E_SPITFIRE:{static const uint16_t d[]={280,95,600,95};return d[p%4];}
 case E_DREADNOUGHT:{static const uint16_t d[]={690,300,900,300};return d[p%4];}
 case E_DWELLER:{static const uint16_t d[]={2220,300,2220,300};return d[p%4];}
 case E_GLAUCUS:{static const uint16_t d[]={525,525,525,750};return d[p%4];}
 case E_CRUCIBLE:{static const uint16_t d[]={1,750,65000,150,360,65000,520,65000,570};return d[p%9];}
 case E_SCION:{static const uint16_t d[]={180,210,120,120,240};return d[p%5];}
 case E_SERAPH:{static const uint16_t d[]={65000,65000,600};return d[p%3];}
 case E_WARBRINGER:{static const uint16_t d[]={180,180,180};return d[p%3];}
 case E_STATION_OMEGA:{static const uint16_t d[]={600,240,600,240};return d[p%4];}
 case E_STAR_EATER:{
  const unsigned recover=72-12*(2*!!(flags&RUN_NEMESIS)+!!(flags&RUN_ANNIHILATION));
  switch(p){case 0:return 300;case 1:return 285;case 2:return 120+recover;
   case 3:return e.form==2?380:305;case 4:return 60+recover;case 5:return 1200;
   case 6:return 60+recover;case 7:return e.form==2?600:460;
   case 8:return 120+recover;case 9:return 640;default:return 300;}
 }
 default:return 240;
 }
}

unsigned Game::bossDamagePercent(const Enemy &e,bool area) const {
 if(e.boss_hidden)return 0;
 switch(e.type){
  case E_CERATOTITAN:return area?50u:100u;
  case E_DREADNOUGHT:return 40;
  case E_STATION_OMEGA:return 0; // Invisible controller; the five wall/node pairs carry all hull.
  case E_WARBRINGER:return 200; // Exposed core is a source weaknessBonus=2 target.
  default:return 100;
 }
}

void Game::fireBoss(Enemy &e) {
 if(e.boss_hidden)return;
 int32_t ax=daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0?daily_egg_x:player.x,ay=daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0?daily_egg_y:player.y;
 if(player.mods[M_X_DECOY_SIGNAL])for(unsigned c=0;c<MAX_CONSTRUCTS;c++){
  const Construct &q=constructs[c];
  if(q.active&&q.kind==C_GUARDIAN&&distance2(e.x,e.y,q.x,q.y)<100*100){ax=q.x;ay=q.y;break;}
 }
 const uint8_t aim=direction(delta(ax,e.x,worldWidth()),delta(ay,e.y,worldHeight()));
 int damage=15*(10000+maxv(0,wave-120)*33)/10000;
 if(run_flags&RUN_NEMESIS)damage=damage*(run_flags&RUN_ANNIHILATION?350:run_flags&RUN_MAYHEM?340:320)/240;
 damage=damage*e.damage_scale/256;
 if(e.type==E_STAR_EATER){unsigned source_age=unsigned(e.age)*2;damage=damage*(100+minv(200,source_age>1800?(int(source_age)-1800)/45:0))/100;}
 const auto shot=[&](int32_t x,int32_t y,uint8_t a,int velocity100,int power,unsigned kind=SHOT_ENEMY,unsigned radius=3,unsigned life=110){
  int n=emitBullet(x,y,a,maxv(1,velocity100/230),maxv(1,power),kind,life,1,radius);
  if(n>=0){Bullet &b=bullets[n];b.speed=(uint16_t)clamp(velocity100*FP/230,1,16*FP);b.vx=dirX(a)*b.speed/FP;b.vy=dirY(a)*b.speed/FP;}
  return n;
 };
 const auto ring=[&](unsigned count,uint8_t start,int velocity100,int power,unsigned kind=SHOT_ENEMY,unsigned radius=3){
  for(unsigned i=0;i<count;++i)shot(e.x,e.y,(uint8_t)(start+i*256/count),velocity100,power,kind,radius);
 };
 const auto fan=[&](unsigned count,uint8_t center,int arc,int velocity100,int power,unsigned radius=3){
  for(unsigned i=0;i<count;++i){int offset=count>1?((int(i)*2-int(count)+1)*arc/(2*int(count-1))):0;shot(e.x,e.y,(uint8_t)(center+offset),velocity100,power,SHOT_ENEMY,radius);}
 };
 const auto beamFrom=[&](int32_t x,int32_t y,uint8_t a,int length,int power){
  int32_t x2=x+dirX(a)*length,y2=y+dirY(a)*length;
  emitEffect(x,y,FX_BEAM,3,9,1,x2,y2);
  if(sweptHit(x,y,wrap(x2,worldWidth()),wrap(y2,worldHeight()),player.x,player.y,player.radius+3))damagePlayer(power);
 };
 const auto beam=[&](uint8_t a,int length,int power){beamFrom(e.x,e.y,a,length,power);};
 const unsigned owner=unsigned(&e-enemies);
 switch(e.type){
 case E_CERATOTITAN:if(novaPartWeaponAlive(*this,e,BP_CERA_PINCER))ring(12,e.attack_angle,500,damage);break;
 case E_SPITFIRE:
  if(e.phase==0){int n=shot(e.x,e.y,aim,675,damage+10,SHOT_ENEMY,7,130);if(n>=0)bullets[n].homing=1;}
  else if(e.phase==2){
   int sweep=-82+int(e.action_count%10)*5;
   shot(e.x,e.y,(uint8_t)(aim+sweep),450,damage);shot(e.x,e.y,(uint8_t)(aim-sweep),450,damage);
  }break;
 case E_DREADNOUGHT:
  if(e.phase==0){
   for(unsigned k=0;k<MAX_BOSS_PARTS;++k){const BossPart &p=boss_parts[k];if(!p.active||p.owner!=owner||(p.kind!=BP_DREAD_GUN_LARGE&&p.kind!=BP_DREAD_GUN_SMALL))continue;
    for(unsigned j=0;j<4;++j)shot(p.x,p.y,(uint8_t)(aim+int(j)*5-7),900,damage+4);}
  }else if(e.phase==2){
   if(e.action_count%3==0){for(unsigned k=0;k<MAX_BOSS_PARTS;++k){const BossPart &p=boss_parts[k];if(p.active&&p.owner==owner&&p.kind==BP_DREAD_GUN_LARGE)beamFrom(p.x,p.y,e.attack_angle,145,damage+8);}}
   else for(unsigned j=0;j<6;++j){int n=shot(e.x,e.y,(uint8_t)(e.angle+j*256/6),450,damage+5,SHOT_MISSILE,4,150);if(n>=0)bullets[n].homing=2;}
  }break;
 case E_STATION_OMEGA:
  for(unsigned k=0;k<MAX_BOSS_PARTS;++k){const BossPart &p=boss_parts[k];if(!p.active||p.owner!=owner||p.kind!=BP_STATION_NODE)continue;
   uint8_t a=(uint8_t)(p.angle+e.action_count*2);
   if(e.phase==3)beamFrom(p.x,p.y,a,105,damage+4);
   else if(e.phase==1){for(unsigned j=0;j<3;++j)shot(p.x,p.y,(uint8_t)(aim+int(j)*8-8),675,damage);}
   else shot(p.x,p.y,a,750,damage);
  }break;
 case E_WARBRINGER:
  if(e.phase==0){for(unsigned k=0;k<MAX_BOSS_PARTS;++k){const BossPart &p=boss_parts[k];if(!p.active||p.owner!=owner||p.kind!=BP_WAR_BROADSIDE)continue;uint8_t a=(uint8_t)(e.angle+(p.mirror?64:-64));shot(p.x,p.y,(uint8_t)(a-5),1100,damage+3);shot(p.x,p.y,(uint8_t)(a+5),1100,damage+3);}}
  else if(e.phase==1){for(unsigned j=0;j<2;++j){int n=shot(e.x,e.y,(uint8_t)(aim+int(j)*12-6),650,damage+8,SHOT_MISSILE,4,110);if(n>=0)bullets[n].homing=2;}}
  else {for(unsigned k=0;k<MAX_BOSS_PARTS;++k){const BossPart &p=boss_parts[k];if(p.active&&p.owner==owner&&p.kind==BP_WAR_LASER)beamFrom(p.x,p.y,(uint8_t)(e.attack_angle+(int(p.slot)-9)*21),170,damage+12);}}
  break;
 case E_SERAPH:
  if(e.phase==0){
   // Alarm 7 spends the source ninety-orb budget in mirrored pairs, five
   // source frames apart. Preserve the half-point damage across each pair.
   int pairDamage=maxv(2,damage/5);int sweep=int(e.action_count)*256*3/720;
   for(unsigned j=0;j<2;++j){uint8_t side=(uint8_t)(e.angle+(j?64:-64));
    uint8_t a=(uint8_t)(e.angle+(j?-85:85)+(j?sweep:-sweep));
    shot(e.x+dirX(side)*15,e.y+dirY(side)*15,a,180+int(range(121)),j?pairDamage-pairDamage/2:pairDamage/2,SHOT_SERAPH_ORB,5,180);}
  }else if(e.phase==1){
   static const int8_t offset[]={-4,4,-2,2},mount[]={-10,10,-4,4};
   for(unsigned j=0;j<4;++j){uint8_t side=(uint8_t)(e.attack_angle+64);
    shot(e.x+dirX(side)*mount[j],e.y+dirY(side)*mount[j],(uint8_t)(e.attack_angle+offset[j]),1800,maxv(1,26*damage/15),SHOT_SERAPH_SNIPE,2,70);}
  }else{
   // These are short attached thermal lances, not screen-length lasers.
   // Source image scales .125/.22/.3125 and mounts 43/39/35 are mapped
   // to the same original-art/9.2 display geometry as the boss body.
   static const uint8_t length[]={8,14,20},mount[]={5,4,4};
   for(unsigned j=0;j<3;++j){uint8_t a=(uint8_t)(e.angle+(j<2?128:0));
    int32_t x=e.x+dirX(a)*mount[j],y=e.y+dirY(a)*mount[j];
    int32_t x2=x+dirX(a)*length[j],y2=y+dirY(a)*length[j];
    emitEffect(x,y,FX_SERAPH_LANCE,4,2,3,x2,y2);
    if(sweptHit(x,y,wrap(x2,worldWidth()),wrap(y2,worldHeight()),player.x,player.y,player.radius+3))e.summon_timer=(uint16_t)minv(60000,int(e.summon_timer)+4*damage);
   }
   if(e.summon_timer>=1000){unsigned immunity=player.invulnerable;damagePlayer(e.summon_timer/1000);e.summon_timer%=1000;if(!immunity)player.invulnerable=0;}
  }break;
 case E_SCION:
  if(e.phase==1){fan(3,aim,18,750,damage);}
  else if(e.phase==3)beam(e.attack_angle,160,damage+12);
  else if(e.phase==4){ring(6,(uint8_t)(e.action_count*17),500,damage);}
  break;
 case E_DWELLER:fan(5,aim,36,700,damage);break;
 case E_GLAUCUS:
  if(e.phase==0)ring(12,e.angle,350,damage,SHOT_VORTEX,5);
  else if(e.phase==1){shot(e.x,e.y,(uint8_t)(e.angle+e.action_count*11),800,damage);shot(e.x,e.y,(uint8_t)(e.angle-e.action_count*11),800,damage);}
  else if(e.phase==2)fan(30,aim,43,1000,damage,2);
  break;
 case E_CRUCIBLE:
  if(e.phase==1){
   for(unsigned j=0;j<MAX_ENEMIES;++j){const Enemy &q=enemies[j];
    if(q.active&&q.type==E_CRUCIBLE_DRONE&&q.leader==(int)owner&&q.leader_generation==e.generation){uint8_t a=direction(delta(ax,q.x,worldWidth()),delta(ay,q.y,worldHeight()));shot(q.x,q.y,a,1200,maxv(1,(damage*168+127)/255),SHOT_ENEMY,2,100);}
   }
  }
  else if(e.phase==2||e.phase==5||e.phase==7){
   // A moving hoop has a visible navigable opening; its projectiles are real
   // collision objects, rather than a cosmetic ring plus unavoidable damage.
   unsigned gap=(e.action_count*3+e.phase)%24;
   for(unsigned i=0;i<24;++i)if((i+24-gap)%24>=4)shot(e.x,e.y,(uint8_t)(i*256/24),320+int(e.action_count)*22,damage,SHOT_ENEMY,3,100);
  }
  break;
 case E_STAR_EATER:
  if(e.phase==1){for(unsigned j=0;j<20;++j){int v=900+int(range(1201));int off=-21+int(j)*42/19;shot(e.x,e.y,(uint8_t)(e.attack_angle+off),v,damage,SHOT_ENEMY,2,100);}}
  else if(e.phase==3){int sweep=dirY((uint8_t)(e.phase_age*256/maxv(1,novaBossDuration(e,run_flags)/2)))*57/FP;
   uint8_t a=(uint8_t)(e.attack_angle+sweep);if(e.form==2)beam(a,180,maxv(1,damage/2));else fan(3,a,35,1200,damage,2);
  }else if(e.phase==7){ring(32,(uint8_t)(e.attack_angle+e.action_count*2),750,damage,SHOT_ENEMY,2);}
  else if(e.phase==9){int n=shot(e.x,e.y,aim,750+int(range(201)),damage+6,SHOT_VORTEX,6,150);if(n>=0)bullets[n].homing=1;}
  break;
 default:break;
 }
 emitEffect(e.x,e.y,FX_MUZZLE,5,4,1);
}

void Game::updateBoss(Enemy &e) {
 const int width=worldWidth(),height=worldHeight();
 int dx=delta(daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0?daily_egg_x:player.x,e.x,width),dy=delta(daily(DAILY_COSMIC_EGG)&&daily_egg_hull>0?daily_egg_y:player.y,e.y,height);
 uint8_t aim=direction(dx,dy);
 const auto ticks=[&](unsigned source){return (uint16_t)clamp(int(source*10000u/(2u*(10000u+unsigned(maxv(0,wave-120))*44u))),1,65000);};
 const auto enter=[&](unsigned phase){
  e.phase=(uint8_t)(phase%novaBossPhaseCount(e.type));e.phase_age=0;e.phase_timer=ticks(novaBossDuration(e,run_flags));
  e.action_timer=0;e.action_count=0;e.fire_delay=0;e.boss_hidden=0;e.attack_angle=aim;e.target_x=player.x;e.target_y=player.y;
 };
 if(!e.form){
  e.form=1;enter(0);
  if(e.type==E_CRUCIBLE){unsigned owner=unsigned(&e-enemies);
   for(unsigned j=0;j<3;++j){uint8_t a=(uint8_t)(j*256/3);int n=spawnEnemy(E_CRUCIBLE_DRONE,pixel(e.x)+dirX(a)*46/FP,pixel(e.y)+dirY(a)*46/FP);if(n<0)break;Enemy &q=enemies[n];q.leader=(int8_t)owner;q.leader_generation=e.generation;q.formation_slot=(uint8_t)j;q.formation_count=3;q.formation_spacing=46;q.formation_angle=(int16_t)(unsigned(a)*256);q.hull=q.max_hull=e.max_hull/4;}
  }
 }
 if(e.hull<e.max_hull/2&&e.form==1){e.form=2;emitEffect(e.x,e.y,FX_RING,e.radius+12,20,1);if(e.type==E_STAR_EATER)enter(0);}
 // The source clock is reduced with temporal slowing, including telegraphs;
 // it must not keep firing at full speed while only movement is slowed.
 bool slow=e.stun||(player.shield>0&&player.shield_type==S_TEMPORAL&&approx(dx,dy)<stats.shield_radius);
 if(slow&&frame%3)return;
 int32_t move_x=width*FP/2,move_y=height*FP/3;
 int speed=FP/2,turn=2;bool direct=false;
 if(e.age<=ticks(novaBossIntro(e.type))){
  aim=direction(delta(move_x,e.x,width),delta(move_y,e.y,height));
  e.angle=toward(e.angle,aim,2);e.vx=dirX(e.angle)*2/3;e.vy=dirY(e.angle)*2/3;
  e.x=wrap(e.x+e.vx,width);e.y=wrap(e.y+e.vy,height);updateBossParts(unsigned(&e-enemies));return;
 }
 if(!e.phase_timer)enter(e.phase+1);
 const unsigned p=e.phase,t=e.phase_age;
 const auto cue=[&](uint8_t a,int length){if(t%5==0)emitEffect(e.x,e.y,FX_BEAM,1,6,3,e.x+dirX(a)*length,e.y+dirY(a)*length);};
 const auto attack=[&](unsigned every){if(!e.fire_delay){fireBoss(e);++e.action_count;e.fire_delay=ticks(every);}};
 const auto summon=[&](unsigned type,int ox,int oy){int child=spawnEnemy(type,pixel(e.x)+ox,pixel(e.y)+oy);if(child>=0&&e.type==E_DWELLER){enemies[child].leader=(int8_t)(&e-enemies);enemies[child].leader_generation=e.generation;enemies[child].formation=0;}emitEffect(wrap(e.x+ox*FP,width),wrap(e.y+oy*FP,height),FX_SPAWN,8,9,1);};
 if(e.fire_delay)--e.fire_delay;
 if(e.action_timer)--e.action_timer;
 switch(e.type){
 case E_CERATOTITAN:
  if(p==2||p==7||p==9||p==11){speed=FP/5;if(t<8)e.attack_angle=aim;cue(e.attack_angle,155);}
  else if(p==3||p==8||p==10||p==12){aim=e.attack_angle;speed=p==3?4725*FP/230:6125*FP/230;direct=true;}
  else if(p==5){speed=FP/5;if(!e.action_timer&&e.action_count<12){summon((e.form==2&&e.action_count%3==0)?E_MYRMIDON:E_SWARMLING,int(range(31))-15,int(range(31))-15);++e.action_count;e.action_timer=ticks(8);}}
  else if(p==6){speed=FP/4;if(t>=ticks(180)&&!e.action_count){fireBoss(e);++e.action_count;}}
  else if(p==13){speed=FP/3;if(t==ticks(60))fireBoss(e);}
  else {move_x=wrap(player.x-dirX(aim)*75,width);move_y=wrap(player.y-dirY(aim)*75,height);}
  break;
 case E_SPITFIRE:
  {int distance=approx(dx,dy);
   int offset=(p==1||p==3)?(p==1?64:-64):(distance<45?128:distance<100?64:20);
   aim=(uint8_t)(aim+offset);speed=(p==1||p==3)?2*FP:FP;
  }
  direct=true;
  if(p==0&&t>=ticks(e.form==2?170:110)&&!e.action_count){fireBoss(e);++e.action_count;}
  if(p==0&&t<ticks(e.form==2?170:110))cue(aim,100);
  if(p==2&&t>=ticks(120)){attack(60);}
  break;
 case E_DREADNOUGHT:
 case E_DWELLER:
  if(p&1){
   aim=e.attack_angle;speed=3*FP;direct=true;
   if(t>=ticks(90))e.boss_hidden=1;
   if(e.phase_timer==1){e.x=(e.attack_angle<128?12:width-12)*FP;e.y=(height/4+int(range(unsigned(height/2))))*FP;e.boss_hidden=0;emitEffect(e.x,e.y,FX_WARP,24,15,3);}
  }else{
   move_x=wrap(player.x-dirX(aim)*85,width);move_y=wrap(player.y-dirY(aim)*85,height);
   if(e.type==E_DREADNOUGHT){if(t<ticks(120)){e.attack_angle=aim;cue(aim,145);}else{if(p==2&&e.fire_delay<ticks(45)){e.attack_angle=aim;cue(aim,145);}attack(p==0?280:180);}}
   else{
    if(!e.action_timer){unsigned kind=e.action_count%6==0?E_DWELLER_WARRIOR:e.action_count%3==0?E_DWELLER_GUARD:e.action_count&1?E_DWELLER_SOLDIER:E_DWELLER_DRONE;
     summon(kind,int(range(41))-20,int(range(41))-20);++e.action_count;e.action_timer=ticks(120);}
    if(t>ticks(120))attack(240);
   }
  }break;
 case E_STATION_OMEGA:
  move_y=height*FP/2;speed=FP/4;e.angle=(uint8_t)(e.angle+1);
  if(p==3){if(t<ticks(90)){e.attack_angle=e.angle;cue(e.attack_angle,115);}else attack(90);}
  else attack(p==1?90:4);
  if((p==0||p==2)&&t%ticks(180)==0)summon(E_DRONE,(t&1)?25:-25,20);
  break;
 case E_WARBRINGER:
  if(p==0){aim=(uint8_t)(aim+64);speed=FP;direct=true;attack(15);}
  else if(p==1){move_x=wrap(player.x-dirX(aim)*95,width);move_y=wrap(player.y-dirY(aim)*95,height);attack(80);}
  else {speed=FP/6;if(t<ticks(90)){e.attack_angle=aim;cue(aim,170);}else attack(45);}
  break;
 case E_SERAPH:{
  const unsigned owner=unsigned(&e-enemies);
  const auto removeMinions=[&](unsigned kind){for(unsigned j=0;j<MAX_ENEMIES;++j){Enemy &q=enemies[j];if(q.active&&q.type==kind&&q.leader==(int)owner&&q.leader_generation==e.generation)q.active=0;}};
  const auto zones=[&](){
   removeMinions(E_SERAPH_ZONE_DRONE);
   // Alarm4 spawning=2: opposing pairs cross each of the four edge bands.
   static const uint8_t orientations[8]={1,1,3,3,4,4,2,2};
   static const uint8_t angles[8]={0,128,64,192,64,192,0,128};
   for(unsigned j=0;j<8;++j){
    int sx=j<2||j>=6?((j&1)?width*6-100:100):(j<4?200:width*6-200);
    int sy=j<2?200:j>=6?height*6-200:((j&1)?height*6-100:100);
    int32_t x=(sx*FP+((int)range(50*FP+1)-25*FP))/6,y=(sy*FP+((int)range(50*FP+1)-25*FP))/6;
    int n=spawnEnemy(E_SERAPH_ZONE_DRONE,pixel(x),pixel(y));if(n<0)break;
    Enemy &q=enemies[n];q.x=x;q.y=y;q.leader=(int8_t)owner;q.leader_generation=e.generation;q.formation=orientations[j];q.formation_slot=(uint8_t)j;q.formation_count=8;q.target_x=x;q.target_y=y;q.angle=angles[j];q.damage_scale=e.damage_scale;q.treasure_scale=e.treasure_scale;q.size_scale=e.size_scale;
   }
  };
  if(p==0){
   aim=(uint8_t)(aim+64);speed=FP;direct=true;
   if(t==0)removeMinions(E_SERAPH_DRONE);
   if(t==ticks(90))zones();
   if(t==ticks(120)){for(unsigned j=0;j<3;++j){uint8_t a=(uint8_t)(j*256/3);int n=spawnEnemy(E_SERAPH_DRONE,pixel(e.x)+dirX(a)*43/FP,pixel(e.y)+dirY(a)*43/FP);if(n<0)break;Enemy &q=enemies[n];q.leader=(int8_t)owner;q.leader_generation=e.generation;q.formation_spacing=43;q.formation_slot=(uint8_t)j;q.formation_count=3;q.formation_angle=(int16_t)(unsigned(a)*256);q.fire_delay=ticks(16+j*4);}}
   if(e.action_count<45&&t>=ticks(unsigned(e.action_count)*5)){fireBoss(e);++e.action_count;if(e.action_count==45)e.phase_timer=ticks(105);}
  }else if(p==1){
   speed=FP/8;
   if(t==ticks(90))zones();
   unsigned start=ticks(unsigned(e.action_count)*154)-1;
   if(e.action_count<7&&t>=start){
    if(t<start+ticks(60)){e.attack_angle=aim;for(int j=-1;j<=1;j+=2)cue((uint8_t)(aim+j*4),190);}
    else {fireBoss(e);++e.action_count;if(e.action_count==7)e.phase_timer=ticks(94);
     else if(e.action_count==3||e.action_count==5){emitEffect(e.x,e.y,FX_WARP,24,12,3);e.x=wrap(player.x+width*FP/3,width);e.y=wrap(player.y+height*FP/3,height);}}
   }
  }else{
   aim=(uint8_t)(aim+64);speed=1950*FP/230;direct=true;
   if(t==0)e.summon_timer=0;
   if(t==ticks(90))zones();
   fireBoss(e);
  }
  break;}

 case E_SCION:
  if(p==0||p==1){aim=(uint8_t)(aim+40);speed=FP;direct=true;if(p==1&&e.action_count<7)attack(30);}
  else if(p==2){speed=FP/8;e.attack_angle=aim;cue(aim,160);}
  else if(p==3){speed=FP/8;attack(30);}
  else {if(t==0){e.x=wrap(player.x+width*FP/3,width);e.y=wrap(player.y+height*FP/3,height);emitEffect(e.x,e.y,FX_WARP,25,15,3);}speed=FP/3;if(e.action_count<5)attack(36);}
  break;
 case E_GLAUCUS:
  if(p==0||p==1){aim=(uint8_t)(aim+52);speed=FP;direct=true;if(p==0){if(t<ticks(360))cue(aim,95);else if(!e.action_count){fireBoss(e);++e.action_count;}}else attack(20);}
  else if(p==2){move_x=wrap(player.x-dirX(aim)*60,width);move_y=wrap(player.y-dirY(aim)*60,height);speed=131*FP/230;if(t>=ticks(150)){if(e.fire_delay<ticks(80))cue(aim,100);attack(85);}}
  else{aim=(uint8_t)(aim+80);speed=(e.form==2?1400:1000)*FP/230;direct=true;}
  break;
 case E_CRUCIBLE:
  move_x=width*FP/2;move_y=height*FP/2;speed=FP/4;
  if(p==1){
   unsigned live=0;for(unsigned j=0;j<MAX_ENEMIES;++j){const Enemy &q=enemies[j];live+=q.active&&q.type==E_CRUCIBLE_DRONE&&q.leader==int(&e-enemies)&&q.leader_generation==e.generation;}
   if(!live)e.phase_timer=(uint16_t)minv(e.phase_timer,ticks(150));
   else if(t>=ticks(60))attack(unsigned(maxv(4,20-int(e.action_count)*3)));
  }
  else if(p==2||p==5||p==7){
   unsigned charge=p==7?120:210,interval=p==7?50:25,maximum=p==7?20:1;
   if(t<ticks(charge)){if(t%10==0)emitEffect(e.x,e.y,FX_RING,30,12,3);}
   else if(t>=ticks(charge+interval)&&e.action_count<maximum){unsigned before=e.action_count;attack(interval);
    if(e.action_count!=before&&e.action_count==maximum)e.phase_timer=ticks(180);}
  }
  break;
 case E_STAR_EATER:{
  move_x=width*FP/2;move_y=height*FP/5;speed=FP/3;
  if(p==0||p==2||p==4||p==6||p==8){e.attack_angle=aim;}
  if(p==1){if(t<ticks(60)){e.attack_angle=aim;cue(aim,150);}else attack(95);}
  else if(p==3){if(t<ticks(20)){e.attack_angle=aim;cue(aim,180);}else if(t<ticks(e.form==2?320:245))attack(e.form==2?18:12);}
  else if(p==5){
   unsigned chomps=e.form==2?4u:3u,period=ticks(1200/chomps),part=t%period;
   if(part<period*57/100){move_y=height*FP/5;e.attack_angle=aim;cue(aim,170);}
   else if(part<period*75/100){aim=e.attack_angle;speed=8*FP;direct=true;}
   else {move_y=height*FP/5;speed=3*FP;}
  }else if(p==7){
   int polar=player.mods[M_X_POLAR_INVERSION]?-1:1;
   player.vx-=clamp(dx,-50,50)*polar;player.vy-=clamp(dy,-50,50)*polar;
   if(t%10==0)emitEffect(e.x,e.y,FX_RING,55,12,3);
   if(t>=ticks(120)&&e.action_count<(e.form==2?3u:2u))attack(75);
  }else if(p==9){if(t<ticks(90))cue(aim,140);else attack(160);}
  else if(p==10){aim=192;speed=2*FP;direct=true;if(t>ticks(90))e.boss_hidden=1;if(e.phase_timer==1){e.x=width*FP/2;e.y=height*FP/5;e.boss_hidden=0;}}
  break;
 }
 default:break;
 }
 if(!direct)aim=direction(delta(move_x,e.x,width),delta(move_y,e.y,height));
 e.angle=toward(e.angle,aim,turn);
 if(direct){e.vx=dirX(aim)*speed/FP;e.vy=dirY(aim)*speed/FP;}
 else {e.vx=e.vx*7/8+dirX(e.angle)*speed/FP/8;e.vy=e.vy*7/8+dirY(e.angle)*speed/FP/8;}
 if(novaChainBoss(e.type)){e.x=clamp(e.x+e.vx,8*FP,(width-8)*FP);e.y=clamp(e.y+e.vy,8*FP,(height-8)*FP);}
 else {e.x=wrap(e.x+e.vx,width);e.y=wrap(e.y+e.vy,height);}
 updateBossParts(unsigned(&e-enemies));
 ++e.phase_age;if(e.phase_timer)--e.phase_timer;
}
