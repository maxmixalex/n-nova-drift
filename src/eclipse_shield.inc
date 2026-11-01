int Game::eclipseShield(unsigned owner) const {
 if(owner>=MAX_ENEMIES||!enemies[owner].active||enemies[owner].type!=E_ECLIPSE)return -1;
 for(unsigned i=0;i<MAX_BOSS_PARTS;++i)if(boss_parts[i].active&&boss_parts[i].owner==owner&&boss_parts[i].kind==BP_ECLIPSE_SHIELD)return (int)i;
 return -1;
}
bool Game::targetObstructedByEclipse(int target) const {
 if(!targetActive(target))return false;
 unsigned owner=targetOwner(target);if(enemies[owner].type!=E_ECLIPSE)return false;
 int shield=eclipseShield(owner);return shield>=0&&target!=(int)MAX_ENEMIES+shield;
}
void Game::triggerEclipseShield(unsigned index,bool broken,bool contact){
 if(index>=MAX_BOSS_PARTS)return;
 BossPart& p=boss_parts[index];if(p.owner>=MAX_ENEMIES)return;Enemy& e=enemies[p.owner];
 // Player contact has its own canFlash/Alarm cooldown and applies at all rarities.
 if(contact){if(p.local_y)return;p.local_y=4;}
 else {if(!e.elite||(!broken&&p.timer))return;p.timer=3;}
 uint8_t a=direction(player.x-p.x,player.y-p.y);
 if(broken||contact){int at=regularShot(e,e.angle,0,contact?20*(e.elite+1):40,SHOT_ECLIPSE_BURST,12,1);if(at>=0){Bullet& b=bullets[at];b.x=wrap(p.x+dirX(e.angle)*25/6,worldWidth());b.y=wrap(p.y+dirY(e.angle)*25/6,worldHeight());b.speed=0;b.vx=b.vy=0;b.tracking_scale_x=b.tracking_scale_y=(uint16_t)(p.source_scale*6u*(contact?2u:3u)/32u);b.tracking_seek=contact?74:307;b.homing=255;b.field_scale=0;}}
 else {int at=regularShot(e,a,0,5,SHOT_ECLIPSE_REFLECT,9,1);if(at>=0){Bullet& b=bullets[at];int distance=100+33*e.elite;b.x=wrap(p.x+dirX(a)*distance/6,worldWidth());b.y=wrap(p.y+dirY(a)*distance/6,worldHeight());b.speed=0;b.vx=b.vy=0;b.tracking_scale_x=1331;b.tracking_scale_y=2458;b.tracking_seek=61;b.homing=255;b.field_scale=0;}
  if(e.elite>=2){int shot=regularShot(e,a,17*FP,10,SHOT_ECLIPSE_SHOT,80,6);if(shot>=0){Bullet& b=bullets[shot];b.x=p.x;b.y=p.y;b.tracking_scale_x=b.tracking_scale_y=4096;}}
 }
}
