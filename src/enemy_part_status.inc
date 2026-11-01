// Part ignition must not be redirected to a nonphysical controller. This
// loop runs once per tick, independently of attachment/pose updates.
for(unsigned part=0;part<MAX_BOSS_PARTS;++part){BossPart &p=boss_parts[part];
 if(!p.active||p.owner>=MAX_ENEMIES||!enemies[p.owner].active)continue;
 if(p.wake_time)--p.wake_time;
 if(!p.burn_time)continue;
 --p.burn_time;
 if(frame%15==0)hitBossPart(part,maxv(1,absn(p.burn_damage))*(int)(p.burn_damage>0?igniteDamagePercent():100)/100,true);
}
