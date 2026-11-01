// Native oMinelayer Destroy creates normal mines, not a Pulsebug squadron.
if(e.type==E_MINELAYER){
 for(unsigned n=0;n<MAX_ENEMIES;n++)if(enemies[n].active&&(enemies[n].type==E_MINE_SHIELD||enemies[n].type==E_MINE_DRONE)&&enemies[n].leader==(int)index&&enemies[n].leader_generation==e.generation)enemies[n].active=0;
 unsigned count=3+3*e.elite;
 for(unsigned j=0;j<count;j++){uint8_t a=(uint8_t)(e.angle+j*256/count);int n=spawnEnemy(E_STEALTH_MINE,pixel(e.x)+dirX(a)*4/FP,pixel(e.y)+dirY(a)*4/FP);if(n>=0){Enemy &m=enemies[n];m.angle=a;m.vx=dirX(a);m.vy=dirY(a);m.damage_scale=e.damage_scale;}}
}
if((e.type==E_STEALTH_MINE||e.type==E_INTERCEPTOR_MINE||e.type==E_SINGULARITY_MINE)&&e.phase){
 int damage=(int)((int64_t)(e.type==E_SINGULARITY_MINE?50:30)*e.damage_scale/256*(10000+maxv(0,wave-120)*33)/10000);
 hostileBlast(e.x,e.y,e.type==E_SINGULARITY_MINE?28:20,maxv(1,damage));
}
