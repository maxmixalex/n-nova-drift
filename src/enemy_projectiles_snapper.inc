// oScionLance follows the real shooter. It is a growing continuous area,
// not a colliding consumable projectile. X/Y keep separate source scales.
if(b.owner&&b.kind==SHOT_SNAPPER_BEAM){
 if(b.tracking_target<0||b.tracking_target>=MAX_ENEMIES){b.active=0;continue;}
 Enemy& source=enemies[(unsigned)b.tracking_target];
 if(!source.active||source.generation!=b.tracking_seek){b.active=0;continue;}
 b.x=source.x;b.y=source.y;b.angle=source.angle;b.vx=b.vy=0;
 if(!b.life||!--b.life){b.active=0;continue;}
 unsigned size=maxv(1,b.speed),source_age=b.age*2;
 for(unsigned sub=0;sub<2;++sub){
  if(source_age<=180){
   b.field_scale=(uint32_t)minv((uint64_t)2250000*size/256,(uint64_t)b.field_scale+4500*size/256);
   unsigned growth=b.tracking_scale_x+84*size;b.tracking_scale_x=(uint16_t)(growth%1000);
   b.tracking_scale_y=(uint16_t)minv(65535,minv(4096*size/256,b.tracking_scale_y+growth/1000));
  }else {
   // oScionLance EndStep0x140ceec4f subtracts.1 from Y each source frame;
   // X stops growing. The earlier generic isotropic shrink was incorrect.
   unsigned loss=b.tracking_scale_x+409600;b.tracking_scale_x=(uint16_t)(loss%1000);
   b.tracking_scale_y=(uint16_t)maxv(0,(int)b.tracking_scale_y-int(loss/1000));
  }
 }
 if(!b.tracking_scale_y){b.active=0;continue;}
 b.size_percent=(uint16_t)maxv(1,b.field_scale/10000);
 int length=(int)((uint64_t)531*b.field_scale/6000000),behind=(int)((uint64_t)10*b.field_scale/6000000),radius=(int)((uint64_t)41*b.tracking_scale_y/(6*4096));
 radius=clamp(radius,1,100);b.radius=(uint8_t)radius;
 const auto hit=[&](int32_t x,int32_t y,int r){int dx=(x-b.x)/FP,dy=(y-b.y)/FP;int along=(dx*dirX(b.angle)+dy*dirY(b.angle))/FP,across=(-dx*dirY(b.angle)+dy*dirX(b.angle))/FP;return along>=-behind-r&&along<=length+r&&absn(across)<=radius+r;};
 unsigned amount=(unsigned)b.damage+b.traveled;b.traveled=(uint16_t)(amount%256);amount/=256;
 if(amount&&hit(player.x,player.y,player.radius))damagePlayer((int)amount,false,(uint8_t)(b.angle+128),true);
 if(amount)for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(constructs[c].active&&hit(constructs[c].x,constructs[c].y,constructs[c].radius))damageConstruct(c,(int)amount);
 continue;
}
