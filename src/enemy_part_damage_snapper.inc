// Called once from the destroyed-part branch. Source Destroy events do not
// forward HP damage to the500-HP head; they change its future behaviour.
if(e.type==E_SNAPPER){
 if(kind==BP_SNAPPER_JAW_LEFT||kind==BP_SNAPPER_JAW_RIGHT){
  e.target_y=e.target_y/2;e.phase=0;e.action_count=0;e.formation_spacing=4000;
  if(e.formation_count)--e.formation_count;
  // enemyInit argument6 is zero treasure; checkDestroyReduced(.4,false)
  // changes the destruction effect, not a nonexistent XP reward.
 }else if(kind==BP_SNAPPER_BASE){e.target_y=e.target_y*4/5;e.phase=0;e.action_count=0;e.formation_spacing=4000;}
 // Segment/base checkDestroyQuiet intentionally has no kill/XP reward.
}
