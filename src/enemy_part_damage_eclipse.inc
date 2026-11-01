if(e.type==E_ECLIPSE){
 if(kind==BP_ECLIPSE_SHIELD)triggerEclipseShield(index,true);
 else if(kind==BP_ECLIPSE_CHASSIS){
  bool other=false;for(unsigned j=0;j<MAX_BOSS_PARTS;++j)if(j!=index&&boss_parts[j].active&&boss_parts[j].owner==owner&&boss_parts[j].kind==BP_ECLIPSE_CHASSIS)other=true;
  if(!other)e.hull=0;
  awardEnemyPart(owner,6,35,true,p.x,p.y);
 }else if(kind==BP_ECLIPSE_PANEL)awardEnemyPart(owner,6,35,true,p.x,p.y);
 // Shield destruction clears hardShielded on the core and all four parts;
 // their next hit is now admitted by the shared live-shield predicate.
}
