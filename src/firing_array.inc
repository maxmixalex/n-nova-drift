// shootSpecialCases3 0x1405d6c7f..0x1405d7a03 and
// specialCasesSwords 0x140692f83..0x1406942a5. This is an additive lerp:
// outer damage is 100%, not zero. A non-distributed ordinary shot starts at
// t=.5; the separate Sword spark routine starts at t=0.
namespace {
unsigned firingArrayPosition(int offset,int arc,unsigned count,unsigned fallback=500) {
 if(count<=1||arc<=0)return fallback;
 return (unsigned)clamp(1000-absn(turnDelta(0,(uint8_t)offset))*2000/arc,0,1000);
}
unsigned firingArraySize(unsigned position){return 700+position*3/5;}
void applyFiringArray(Bullet& shot,unsigned position) {
 const unsigned power=1000+position/5,size=firingArraySize(position);
 shot.damage=(int)minv(2000000,(int)((int64_t)shot.damage*power/1000));
 shot.blast=(uint8_t)minv(255,(unsigned)shot.blast*power/1000);
 shot.radius=(uint8_t)clamp((unsigned)shot.radius*size/1000,1,100);
 // Dart's stuck duration and burn scale consume this independently of the
 // collision radius. Preserve both source image scale and sizeMod.
 shot.size_percent=(uint16_t)clamp((unsigned)shot.size_percent*size/1000,1,65535);
}
}
