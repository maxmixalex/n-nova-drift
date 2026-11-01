// oSeraphShot Step: grow its X axis before incrementing the source timer.
// Scale uses millionths to retain the .000075 curvature without per-tick loss.
if(b.owner&&b.kind==SHOT_SERAPH_ZONE){
 unsigned source_age=(unsigned)(b.age-1)*2;
 int64_t scale=(int64_t)b.field_scale+15000-75*(int64_t)(source_age*2+1);
 if(scale<=25000){b.active=0;continue;}
 b.field_scale=(uint32_t)minv(16000000,(int)scale);
 b.size_percent=(uint16_t)((uint64_t)b.field_scale*4096/1000000);
}
