#include "skins.h"
#include "platform.h"
#include "nnova/image_codec.h"
namespace nova { namespace {
bool all(platform::File* f,void* p,unsigned size){return platform::read(f,p,size)==size;}
uint32_t crc(uint32_t value,const uint8_t* p,unsigned size){while(size--){value^=*p++;for(unsigned bit=0;bit<8;++bit)value=(value>>1)^(0xedb88320u&(0u-(value&1u)));}return value;}
struct SkinStream {platform::File* f;unsigned left,at,fill;uint8_t data[512];
 bool byte(uint8_t& b){if(at==fill){if(!left)return false;fill=left<sizeof(data)?left:sizeof(data);if(!all(f,data,fill))return false;left-=fill;at=0;}b=data[at++];return true;}
 bool pixel(uint16_t& c,uint8_t& a){uint8_t lo,hi;if(!byte(lo)||!byte(hi)||!byte(a))return false;c=uint16_t(lo|unsigned(hi)<<8);return true;}
 bool word(uint16_t& value){if(fill-at>=2){value=uint16_t(unsigned(data[at])|(unsigned(data[at+1])<<8));at+=2;return true;}uint8_t lo,hi;if(!byte(lo)||!byte(hi))return false;value=uint16_t(unsigned(lo)|(unsigned(hi)<<8));return true;}
 bool read(uint8_t* target,unsigned count){while(count){if(at==fill){if(!left)return false;fill=left<sizeof(data)?left:sizeof(data);if(!all(f,data,fill))return false;left-=fill;at=0;}unsigned n=count<fill-at?count:fill-at;codec::copy_bytes(target,data+at,n);target+=n;at+=n;count-=n;}return true;}
 bool finished()const{return !left&&at==fill;}
};
}
bool Skins::open(){
 count_=selected_=0;for(unsigned i=0;i<3;++i)views_[i]={};
 platform::File* f=platform::open_data("nNova.skins.dat.tns");if(!f)return false;
 skinpack::Header h{};bool ok=all(f,&h,sizeof(h));const char magic[8]={'N','N','O','V','A','K','1',0};
 for(unsigned i=0;i<8;++i)if(h.magic[i]!=magic[i])ok=false;
 unsigned metadata=sizeof(h)+h.skin_count*sizeof(skinpack::Skin)+h.image_count*sizeof(skinpack::Image);
 ok=ok&&h.version==1&&h.skin_count>0&&h.skin_count<=skinpack::MaxSkins&&h.image_count==h.skin_count*3&&h.file_bytes>=metadata&&h.file_bytes<1024*1024;
 if(ok)ok=all(f,records_,h.skin_count*sizeof(records_[0]))&&all(f,images_,h.image_count*sizeof(images_[0]));
 if(ok){uint8_t buffer[512];uint32_t checksum=~0u;unsigned left=h.file_bytes-sizeof(h);ok=platform::seek(f,sizeof(h));
  while(ok&&left){unsigned n=left<sizeof(buffer)?left:sizeof(buffer);ok=all(f,buffer,n);if(ok)checksum=crc(checksum,buffer,n);left-=n;}ok=ok&&~checksum==h.payload_crc32;}
 for(unsigned i=0;ok&&i<h.skin_count;++i){const auto& record=records_[i];bool end=false;for(unsigned j=0;j<48;++j)if(!record.name[j])end=true;ok=record.name[0]&&end&&record.first_image==i*3;}
 for(unsigned i=0;ok&&i<h.image_count;++i){const auto& im=images_[i];if(!im.stored_bytes){ok=i%3==2&&!im.width&&!im.height;continue;}
  ok=im.width&&im.height&&im.width<=72&&im.height<=72&&im.canvas_width&&im.canvas_height&&im.canvas_width<=72&&im.canvas_height<=72&&im.decoded_bytes==unsigned(im.width)*im.height*3&&pack::supported_codec(im.codec)&&im.offset>=metadata&&im.offset<=h.file_bytes&&im.stored_bytes<=h.file_bytes-im.offset;}
 platform::close(f);if(ok)count_=h.skin_count;return ok;
}
bool Skins::select(unsigned choice){
 if(choice>count_)return false;
 if(choice==selected_)return true;
 if(!choice){selected_=0;return true;}
 platform::File* f=platform::open_data("nNova.skins.dat.tns");if(!f)return false;bool ok=true;
 for(unsigned i=0;i<3&&ok;++i){const auto& im=images_[(choice-1)*3+i];views_[i]={};if(!im.stored_bytes)continue;
  ok=platform::seek(f,im.offset);SkinStream stream{f,im.stored_bytes,0,0,{}};
  if(ok)ok=codec::decode(stream,im.codec,colors_[i],alpha_[i],unsigned(im.width)*im.height);
  uint32_t checksum=~0u;for(unsigned p=0;ok&&p<unsigned(im.width)*im.height;++p){uint8_t pixel[3]={uint8_t(colors_[i][p]),uint8_t(colors_[i][p]>>8),alpha_[i][p]};checksum=crc(checksum,pixel,3);}ok=ok&&~checksum==im.decoded_crc32;
  if(ok)views_[i]={colors_[i],alpha_[i],im.width,im.height,im.origin_x,im.origin_y,im.source_extent,im.source_extent,im.canvas_width,im.canvas_height};
 }
 platform::close(f);selected_=ok?choice:0;return ok;
}
}
