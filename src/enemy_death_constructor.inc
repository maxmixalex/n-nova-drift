// oConstructorMissile_Destroy -> spawnEnemyBlast, with source blastDamage25.
if(e.type==E_CONSTRUCTOR_MISSILE){
 int damage=(int)((int64_t)25*e.damage_scale/256*(10000+maxv(0,wave-120)*33)/10000);
 hostileBlast(e.x,e.y,20,maxv(1,damage));emitEffect(e.x,e.y,FX_BURST,20,10,3);
}
