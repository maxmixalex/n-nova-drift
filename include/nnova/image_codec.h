#pragma once
#include "pack_format.h"

namespace nova { namespace codec {
inline void copy_bytes(uint8_t* target,const uint8_t* source,unsigned count) {
#if defined(__GNUC__) || defined(__clang__)
    // Ndless supplies the alignment-safe freestanding memcpy implementation.
    __builtin_memcpy(target,source,count);
#else
    for(unsigned i=0;i<count;++i)target[i]=source[i];
#endif
}
template<class Reader,class Value>
bool decode_rle_plane(Reader& stream,Value* output,unsigned count) {
    unsigned p=0;
    while(p<count) {
        uint8_t command;
        if(!stream.byte(command))return false;
        unsigned n=unsigned(command&127)+1;
        if(n>count-p)return false;
        if(command&128) {
            uint16_t color=0;uint8_t alpha=0;
            if(sizeof(Value)==2?!stream.word(color):!stream.byte(alpha))return false;
            const Value value=Value(sizeof(Value)==2?color:alpha);
            while(n--)output[p++]=value;
        } else {
            // Packed color units and native cache words are both little endian.
            // Read literals as blocks, retaining the stream's boundary checks.
            if(!stream.read(reinterpret_cast<uint8_t*>(output+p),n*sizeof(Value)))return false;
            p+=n;
        }
    }
    return true;
}
// Reader needs byte/word/pixel, read(uint8_t*,unsigned), and finished().
// The same strict decoder is used by the converter and the ARM cache loader.
template<class Reader>
bool decode(Reader& stream,uint16_t codec,uint16_t* colors,uint8_t* alpha,unsigned count) {
    if(!count||!pack::supported_codec(codec))return false;
    if(codec==pack::PlanarRle565A8)
        return decode_rle_plane(stream,colors,count)&&decode_rle_plane(stream,alpha,count)&&stream.finished();
    if(codec<=pack::Rle565A8) {
        unsigned p=0;
        while(p<count) {
            uint8_t command=0;
            if(codec==pack::Rle565A8&&!stream.byte(command))return false;
            unsigned n=codec==pack::Raw565A8?1:unsigned(command&127)+1;
            if(n>count-p)return false;
            if(command&128) {
                uint16_t c;uint8_t a;if(!stream.pixel(c,a))return false;
                while(n--){colors[p]=c;alpha[p++]=a;}
            } else while(n--){if(!stream.pixel(colors[p],alpha[p]))return false;++p;}
        }
        return stream.finished();
    }
    const bool nibble=codec==pack::Palette4_565A8||codec==pack::Palette4_565_Alpha8;
    const bool separate_alpha=codec>=pack::Palette4_565_Alpha8;
    uint8_t lo,hi;
    if(!stream.byte(lo)||!stream.byte(hi))return false;
    const unsigned entries=unsigned(lo)|(unsigned(hi)<<8);
    if(!entries||entries>(nibble?16u:256u))return false;
    uint16_t palette_colors[256];uint8_t palette_alpha[256];
    for(unsigned i=0;i<entries;++i) {
        if(!stream.byte(lo)||!stream.byte(hi))return false;
        palette_colors[i]=uint16_t(unsigned(lo)|(unsigned(hi)<<8));
        if(!separate_alpha&&!stream.byte(palette_alpha[i]))return false;
    }
    uint8_t packed=0;
    for(unsigned p=0;p<count;++p) {
        if((!nibble||!(p&1))&&!stream.byte(packed))return false;
        const unsigned index=nibble?((p&1)?packed>>4:packed&15):packed;
        if(index>=entries)return false;
        colors[p]=palette_colors[index];
        if(!separate_alpha)alpha[p]=palette_alpha[index];
    }
    if(nibble&&(count&1)&&(packed&0xf0))return false;
    if(separate_alpha)for(unsigned p=0;p<count;++p)if(!stream.byte(alpha[p]))return false;
    return stream.finished();
}
struct MemoryReader {
    const uint8_t* data;unsigned bytes,pos=0;
    bool byte(uint8_t& value) {if(pos>=bytes)return false;value=data[pos++];return true;}
    bool word(uint16_t& value) {
        if(bytes-pos<2)return false;
        value=uint16_t(unsigned(data[pos])|(unsigned(data[pos+1])<<8));pos+=2;return true;
    }
    bool read(uint8_t* target,unsigned count) {
        if(count>bytes-pos)return false;
        copy_bytes(target,data+pos,count);pos+=count;return true;
    }
    bool pixel(uint16_t& color,uint8_t& alpha) {
        uint8_t lo,hi;if(!byte(lo)||!byte(hi)||!byte(alpha))return false;
        color=uint16_t(unsigned(lo)|(unsigned(hi)<<8));return true;
    }
    bool finished() const {return pos==bytes;}
};
}}
