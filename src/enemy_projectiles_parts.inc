// Included once per bullet after age increment. Source AoE is a persistent
// beam-derived hazard: touching it does not consume the orb as a normal shot.
if(b.owner&&b.kind==SHOT_JUGGERNAUT_ORB){
 if(b.life)--b.life;
 if(!b.life){b.active=0;continue;}
 b.x=wrap(b.x+b.vx,worldWidth());b.y=wrap(b.y+b.vy,worldHeight());
 b.size_percent=(uint16_t)maxv(20,105-int(b.age)*3/10);
 if(frame%5==0){int radius=b.radius+player.radius;
  if(distance2(b.x,b.y,player.x,player.y)<=radius*radius){damagePlayer(maxv(1,b.damage*10),false,-1,true);}
  for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active){radius=b.radius+constructs[c].radius;if(distance2(b.x,b.y,constructs[c].x,constructs[c].y)<=radius*radius)damageConstruct(c,maxv(1,b.damage*10));}
 }
 continue;
}
if(b.owner&&b.kind==SHOT_JUGGERNAUT){
 // oJuggernautShot shrinks .00075 per source frame from .325.
 b.size_percent=(uint16_t)maxv(1,100-int(b.age)*6/13);
 if(b.age>=217){destroyHostileBullet(i);continue;}
}
if(b.owner&&b.kind==SHOT_CELESTIAL){
 // Counter-shot uses 25px/source-frame and .99 speed decay per source frame.
 b.vx=b.vx*9801/10000;b.vy=b.vy*9801/10000;b.speed=(uint16_t)(b.speed*9801/10000);
 if(b.age>40)b.size_percent=(uint16_t)maxv(1,100-int(b.age-40)*3);
}
