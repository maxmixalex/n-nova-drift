// oTracerBall begins bursting at source Alarm0=60. Split children retain
// their inherited armed state; a primary ray combo itself is non-explosive.
if(dead.kind==SHOT_TRACER_BALL&&(dead.age>=30||dead.pierce))hostileBlast(dead.x,dead.y,dead.pierce?12:20,maxv(1,dead.blast));
