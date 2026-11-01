// Included after the enemy is marked inactive. No child keeps a pointer to its
// parent; generation identifiers prevent an old projectile acquiring a new one.
if(dead.type==E_JUGG_MISSILE||dead.type==E_JUGG_MISSILE_HOMING){
 const bool homing=dead.type==E_JUGG_MISSILE_HOMING;
 hostileBlast(dead.x,dead.y,homing?12:10,(homing?55:30)*dead.damage_scale/256);
}
