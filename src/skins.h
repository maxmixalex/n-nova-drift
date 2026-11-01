#pragma once
#include "assets.h"
#include "nnova/skin_format.h"
namespace nova {
class Skins {
public:
 bool open();
 bool select(unsigned choice); // 0=default, 1..count=custom skin.
 unsigned count()const{return count_;}
 const char* name(unsigned index)const{return index<count_?records_[index].name:"";}
 const Sprite* image(unsigned kind)const{return selected_&&kind<3&&views_[kind].width?&views_[kind]:nullptr;}
 int jetAngle()const{return selected_?records_[selected_-1].jet_angle:0;}
 int jetDistance()const{return selected_?records_[selected_-1].jet_distance:0;}
private:
 unsigned count_,selected_;
 skinpack::Skin records_[skinpack::MaxSkins];
 skinpack::Image images_[skinpack::MaxSkins*3];
 Sprite views_[3];
 uint16_t colors_[3][skinpack::MaxPixels];uint8_t alpha_[3][skinpack::MaxPixels];
};
}
