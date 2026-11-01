// Myrmidon: original oMyrmidonBomb -> oHammerheadBlastSpawner ->
// oHammerheadBurstWarning / oEnemyBlast. All image scales are Q12.
void Game::applyMyrmidonField(Bullet &b) {
 const auto near=[&](int32_t x,int32_t y,unsigned radius){
  int64_t dx=(int64_t)x-b.x,dy=(int64_t)y-b.y;
  int64_t reach=(int64_t)(b.radius+radius)*FP;return dx*dx+dy*dy<=reach*reach;
 };
 if(!b.owner){
  if(b.wraps)return;
  b.wraps=1;
  for(unsigned t=0;t<MAX_ENEMIES+MAX_BOSS_PARTS;++t)if(targetActive((int)t)&&near(targetX((int)t),targetY((int)t),targetRadius((int)t)))damageTarget((int)t,b.damage,false,true);
  return;
 }
 if(!b.wraps&&near(player.x,player.y,player.radius)){damagePlayer(b.damage,false,255,true);b.wraps=1;}
 for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&!(b.tracking_seek&(1u<<c))&&near(constructs[c].x,constructs[c].y,constructs[c].radius)){
  b.tracking_seek=(uint16_t)(b.tracking_seek|(1u<<c));damageConstruct(c,b.damage);
 }
}
int Game::spawnMyrmidonField(int32_t x,int32_t y,unsigned scale,int damage,unsigned warning,bool hostile) {
 scale=(unsigned)clamp((int)scale,1,46000);
 int slot=emitBullet(x,y,0,1,maxv(1,damage),SHOT_MYRMIDON_BLAST,100,hostile?1:0,1);
 if(slot<0){
  // Keep the damaging event under pool pressure; no heap or extra effect pool.
  Bullet field={};field.x=x;field.y=y;field.owner=hostile?1:0;field.damage=damage;
  field.radius=(uint8_t)clamp((int)((uint64_t)scale*14*200/(10*4096*6)),1,100);
  if(!warning)applyMyrmidonField(field);
  return -1;
 }
 Bullet &b=bullets[slot];b.x=x;b.y=y;b.vx=b.vy=0;b.speed=0;b.field_scale=scale;
 b.tracking_scale_x=0;b.tracking_scale_y=(uint16_t)warning;b.tracking_seek=0;b.wraps=0;
 b.pierce=warning?0:1;b.blast=0;b.homing=0;b.bounces=0;b.tracking=0;
 b.size_percent=(uint16_t)(warning?maxv(1,(int)scale/100):scale*14/10);
 return slot;
}
void Game::detonateMyrmidon(const Bullet &dead) {
 if(dead.kind!=SHOT_MYRMIDON_BOMB||dead.damage<=0||!dead.field_scale)return;
 // The original Destroy event suppresses its burst when outside the viewport.
 if(dead.x<0||dead.y<0||dead.x>=worldWidth()*FP||dead.y>=worldHeight()*FP)return;
 unsigned rarity=dead.construct_source<3?dead.construct_source:2;
 unsigned health=(unsigned)minv(4096,(int)((int64_t)dead.damage*4096/dead.field_scale));
 if(!health)return;
 unsigned size=dead.tracking_seek?dead.tracking_seek:256;
 unsigned spatial=(unsigned)((uint64_t)size*health/256);
 int original=(int)dead.field_scale,base=20+5*(int)rarity;
 int initial_damage=maxv(1,(int)((int64_t)14*original/base));
 spawnMyrmidonField(dead.x,dead.y,spatial*3/5,initial_damage,0,dead.owner!=0);
 int slot=emitBullet(dead.x,dead.y,0,1,1,SHOT_MYRMIDON_SPAWNER,100,dead.owner,1);
 if(slot>=0){
  Bullet &p=bullets[slot];p.vx=p.vy=0;p.speed=0;p.blast=0;p.tracking=0;p.bounces=0;
  // charges = 5 + 4*rarity*random_range(.8,1.2), used until <=0.
  unsigned count_q12=5*4096+4*rarity*(3277+range(1639));
  p.homing=(uint8_t)((count_q12+4095)/4096);p.tracking_scale_x=1;
  p.traveled=(uint16_t)minv(65535,(int)((uint64_t)(105+20*rarity)*FP*spatial/(6*4096)));
  p.size_percent=(uint16_t)minv(46000,(int)((uint64_t)(85+rarity)*spatial/100));
  p.damage=minv(2000000,maxv(1,(int)((int64_t)70*original*health/(base*4096))));
 }
 emitEffect(dead.x,dead.y,FX_BURST,12,10,5);
}
