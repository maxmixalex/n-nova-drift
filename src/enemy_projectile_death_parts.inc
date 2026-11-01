// `dead` is a value snapshot: an explosion may immediately reuse its pool slot.
if(dead.kind==SHOT_CELESTIAL&&dead.last_hit>0)hostileBlast(dead.x,dead.y,maxv(1,dead.blast),dead.last_hit);
