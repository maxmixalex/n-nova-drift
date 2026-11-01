#include "font.h"
#include "platform.h"

namespace nova {
bool Font::open(bool chinese) {
    valid_=false;
    platform::File* file=platform::open_data(chinese?"nNova.zhs.font.dat.tns":"nNova.eng.font.dat.tns");
    if(!file)return false;
    uint32_t size=platform::length(file);
    if(size<sizeof(FontHeader)||size>sizeof(data_)){platform::close(file);return false;}
    bool success=platform::read(file,data_,size)==size;
    platform::close(file);
    if(!success)return false;
    const FontHeader& header=*reinterpret_cast<const FontHeader*>(data_);
    const char expected[8]={'N','N','O','V','A','F','1',0};
    for(unsigned i=0;i<8;++i)if(header.magic[i]!=expected[i])return false;
    if(header.version!=1||header.file_bytes!=size||header.count==0||header.count>8192||
       header.pixel_height<6||header.pixel_height>24||header.line_height<6||header.line_height>32||
       (header.records_offset&3)||header.records_offset<sizeof(FontHeader)||
       header.records_offset>size||header.count>(size-header.records_offset)/sizeof(GlyphRecord)||
       header.pixels_offset<header.records_offset+header.count*sizeof(GlyphRecord)||
       header.pixels_offset>size)return false;
    const GlyphRecord* records=reinterpret_cast<const GlyphRecord*>(data_+header.records_offset);
    uint32_t previous=0;
    for(uint32_t i=0;i<header.count;++i) {
        const GlyphRecord& r=records[i];
        if((i&&r.codepoint<=previous)||r.codepoint>0x10ffff||r.width>48||r.height>48||
           r.advance<0||r.advance>48||r.bearing_x< -48||r.bearing_x>48||
           r.bearing_y< -48||r.bearing_y>48||r.offset<header.pixels_offset||
           r.offset>size||uint32_t(r.width)*r.height>size-r.offset)return false;
        previous=r.codepoint;
    }
    valid_=true;
    return true;
}
int Font::line_height() const {
    return valid_?int(reinterpret_cast<const FontHeader*>(data_)->line_height):10;
}
int Font::pixel_height() const {
    return valid_?int(reinterpret_cast<const FontHeader*>(data_)->pixel_height):7;
}
const GlyphRecord* Font::glyph(uint32_t codepoint) const {
    if(!valid_)return 0;
    const FontHeader& h=*reinterpret_cast<const FontHeader*>(data_);
    const GlyphRecord* records=reinterpret_cast<const GlyphRecord*>(data_+h.records_offset);
    uint32_t low=0,high=h.count;
    while(low<high) {
        uint32_t middle=low+(high-low)/2;
        if(records[middle].codepoint<codepoint)low=middle+1;
        else high=middle;
    }
    if(low<h.count&&records[low].codepoint==codepoint)return records+low;
    return codepoint=='?'?0:glyph('?');
}
uint32_t next_utf8(const char*& text) {
    const unsigned char* p=reinterpret_cast<const unsigned char*>(text);
    unsigned lead=*p++;
    if(!lead)return 0;
    if(lead<0x80){++text;return lead;}
    unsigned continuation=0,value=0,minimum=0;
    if(lead>=0xc2&&lead<=0xdf){continuation=1;value=lead&31;minimum=0x80;}
    else if(lead>=0xe0&&lead<=0xef){continuation=2;value=lead&15;minimum=0x800;}
    else if(lead>=0xf0&&lead<=0xf4){continuation=3;value=lead&7;minimum=0x10000;}
    else {++text;return '?';}
    for(unsigned i=0;i<continuation;++i) {
        if((p[i]&0xc0)!=0x80){++text;return '?';}
        value=(value<<6)|(p[i]&63);
    }
    if(value<minimum||value>0x10ffff||(value>=0xd800&&value<=0xdfff)){++text;return '?';}
    text+=continuation+1;
    return value;
}
}
