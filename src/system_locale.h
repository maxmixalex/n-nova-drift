#pragma once
// OS preference protocol adapted from nSpire, Copyright (c) 2026 Max Alex.
// MIT license: third_party/licenses/nSpire-MIT.txt.
#include <stddef.h>
#include <stdint.h>

namespace nova {
// Read a bounded ASCII locale tag from OS data without consulting an OS image
// or hard-coding the language of the calculator used during development.
inline bool locale_text(char out[32],const uint16_t* text) {
    if(!text)return false;
    const bool quoted=*text=='"';if(quoted)++text;
    unsigned n=0;
    while(n<31&&text[n]&&(!quoted||text[n]!='"')) {
        const unsigned c=text[n];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return false;
        out[n]=char(c);++n;
    }
    if(!n||n==31||(quoted?(text[n]!='"'||text[n+1]):text[n]))return false;
    out[n]=0;return true;
}
inline bool locale_xml(char out[32],const uint8_t* data,unsigned bytes) {
    static const char open[]="<lang>",close[]="</lang>";
    for(unsigned at=0;at+sizeof(open)-1<bytes;++at) {
        unsigned k=0;while(k<sizeof(open)-1&&data[at+k]==uint8_t(open[k]))++k;
        if(k!=sizeof(open)-1)continue;
        uint16_t tag[32]={};unsigned n=0;at+=k;
        while(at<bytes&&data[at]!='<'&&n<31)tag[n++]=data[at++];
        if(at+sizeof(close)-1>bytes)return false;
        for(k=0;k<sizeof(close)-1;++k)if(data[at+k]!=uint8_t(close[k]))return false;
        return locale_text(out,tag);
    }
    return false;
}
struct LocaleArchiveEntry {const uint8_t* data;unsigned compressed,plain;bool deflated;};
inline bool locale_archive(const uint8_t* data,unsigned bytes,LocaleArchiveEntry& entry) {
    const auto u16=[](const uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8);};
    const auto u32=[&](const uint8_t* p){return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);};
    unsigned at=0;
    while(at<=bytes&&bytes-at>=30&&u32(data+at)==0x04034b50u) {
        const uint8_t* p=data+at;const unsigned flags=u16(p+6),method=u16(p+8);
        const uint32_t compressed=u32(p+18),plain=u32(p+22);
        const unsigned name=u16(p+26),extra=u16(p+28);
        if(flags&9u||name>bytes-at-30||extra>bytes-at-30-name)return false;
        const unsigned payload=at+30+name+extra;
        if(compressed>bytes-payload)return false;
        static const char wanted[]="settings.xml";
        bool match=name==sizeof(wanted)-1;
        for(unsigned i=0;match&&i<name;++i)match=p[30+i]==uint8_t(wanted[i]);
        if(match) {
            if(!plain||plain>8192||(method!=0&&method!=8)||(method==0&&compressed!=plain))return false;
            entry={data+payload,compressed,plain,method==8};return true;
        }
        at=payload+compressed;
    }
    return false;
}
}
