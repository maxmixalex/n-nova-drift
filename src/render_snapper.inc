// The source lance has independent X/Y scales and keeps its original pivot.
if(kind==SHOT_SNAPPER_BEAM&&bullets_[kind]>=0){
 const Sprite* lance=assets_->find_id((unsigned)bullets_[kind],0);
 if(lance&&lance->canvas_width&&lance->canvas_height){
  int sx=(int)((uint64_t)lance->original_width*b.field_scale*projection/(uint64_t(6000000)*lance->canvas_width));
  int sy=(int)((uint64_t)lance->original_height*b.tracking_scale_y*projection/(uint64_t(6*4096)*lance->canvas_height));
  if(sx>0&&sy>0)canvas.sprite(x,y,lance->pixels,lance->alpha,lance->width,lance->height,lance->origin_x,lance->origin_y,dirX(angle),dirY(angle),sx,65535,255,true,0,false,sy);
 }
 continue;
}
