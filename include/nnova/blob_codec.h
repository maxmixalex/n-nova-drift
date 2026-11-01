#pragma once
// Lossless envelope for fonts and locale dictionaries. Decoded only on load.
// Stream: varuint literal count, literal bytes, varuint match count,
// then uint16 little-endian backward distance when match count is nonzero.
// A zero match count terminates the stream; all source/output bytes must match.
#include <stddef.h>
#include <stdint.h>
namespace nova { namespace blobcodec {
struct Header { char magic[8]; uint32_t version,decoded_bytes,encoded_bytes,decoded_crc32; };
static_assert(sizeof(Header)==24,"Compressed blob header");
inline uint32_t crc32(const uint8_t* p,size_t n) {
    uint32_t c=~uint32_t(0);
    while(n--){c^=*p++;for(unsigned bit=0;bit<8;++bit)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}
    return ~c;
}
inline bool is_compressed(const uint8_t* p,size_t n) {
    const char magic[8]={'N','N','O','V','A','Z','1',0};
    if(n<8)return false;
    for(unsigned i=0;i<8;++i)if(p[i]!=uint8_t(magic[i]))return false;
    return true;
}
inline uint32_t u32(const uint8_t* p) { return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24; }
inline bool read_var(const uint8_t* p,size_t n,size_t& at,uint32_t& value) {
    value=0;
    for(unsigned shift=0;shift<=28;shift+=7){
        if(at>=n)return false;
        const uint8_t b=p[at++];
        if(shift==28&&(b&0xf0))return false;
        value|=uint32_t(b&127u)<<shift;
        if(!(b&128u))return true;
    }
    return false;
}
inline bool decode(const uint8_t* src,size_t source_bytes,uint8_t* dst,size_t capacity,size_t& decoded_bytes) {
    decoded_bytes=0;
    if(!is_compressed(src,source_bytes)||source_bytes<sizeof(Header))return false;
    const uint32_t version=u32(src+8),expected=u32(src+12),encoded=u32(src+16),checksum=u32(src+20);
    if(version!=1||expected>capacity||encoded!=source_bytes-sizeof(Header))return false;
    const uint8_t* p=src+sizeof(Header);size_t at=0,out=0;
    for(;;){
        uint32_t literal=0,match=0;
        if(!read_var(p,encoded,at,literal)||literal>encoded-at||literal>expected-out)return false;
        for(uint32_t i=0;i<literal;++i)dst[out++]=p[at++];
        if(!read_var(p,encoded,at,match))return false;
        if(!match){
            if(at!=encoded||out!=expected||crc32(dst,out)!=checksum)return false;
            decoded_bytes=out;return true;
        }
        if(match<4||match>expected-out||encoded-at<2)return false;
        const unsigned distance=unsigned(p[at])|(unsigned(p[at+1])<<8);at+=2;
        if(!distance||distance>out)return false;
        for(uint32_t i=0;i<match;++i){dst[out]=dst[out-distance];++out;}
    }
}
} }
