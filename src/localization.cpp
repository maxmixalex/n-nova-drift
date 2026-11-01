#include "localization.h"
#include "platform.h"

namespace nova {
namespace {
struct Header {char magic[8];uint32_t version,count,records,strings,bytes;};
struct Entry {uint32_t key,value;};
int compare(const char* a,const char* b) {
    while(*a && *a==*b){++a;++b;}
    return int(static_cast<unsigned char>(*a))-int(static_cast<unsigned char>(*b));
}
bool terminated(const uint8_t* data,uint32_t offset,uint32_t size) {
    for(uint32_t i=offset;i<size;++i)if(!data[i])return true;
    return false;
}
}
bool Locale::open(bool chinese) {
    ready_=false;
    platform::File* f=platform::open_data(chinese?"nNova.zhs.locale.dat.tns":"nNova.eng.locale.dat.tns");
    if(!f)return false;
    uint32_t size=platform::length(f);
    if(size<sizeof(Header)||size>sizeof(data_)){platform::close(f);return false;}
    bool ok=platform::read(f,data_,size)==size;platform::close(f);
    if(!ok)return false;
    const Header& h=*reinterpret_cast<const Header*>(data_);
    const char magic[8]={'N','N','O','V','A','L','1',0};
    for(unsigned i=0;i<8;++i)if(h.magic[i]!=magic[i])return false;
    if(h.version!=1||h.bytes!=size||h.records<sizeof(Header)||(h.records&3)||h.records>size||
       h.count>(size-h.records)/sizeof(Entry)||h.strings<h.records+h.count*sizeof(Entry)||h.strings>size)return false;
    const Entry* entries=reinterpret_cast<const Entry*>(data_+h.records);
    const char* previous=0;
    for(unsigned i=0;i<h.count;++i) {
        const Entry& e=entries[i];
        if(e.key<h.strings||e.key>=size||e.value<h.strings||e.value>=size||
           !terminated(data_,e.key,size)||!terminated(data_,e.value,size))return false;
        const char* key=reinterpret_cast<const char*>(data_+e.key);
        if(previous&&compare(previous,key)>=0)return false;
        previous=key;
    }
    ready_=true;return true;
}
const char* Locale::get(const char* key) const {
    if(!ready_)return 0;
    const Header& h=*reinterpret_cast<const Header*>(data_);
    const Entry* entries=reinterpret_cast<const Entry*>(data_+h.records);
    unsigned low=0,high=h.count;
    while(low<high){unsigned mid=low+(high-low)/2;const char* candidate=reinterpret_cast<const char*>(data_+entries[mid].key);
        if(compare(candidate,key)<0)low=mid+1;else high=mid;}
    if(low<h.count&&compare(reinterpret_cast<const char*>(data_+entries[low].key),key)==0)
        return reinterpret_cast<const char*>(data_+entries[low].value);
    return 0;
}
void Locale::plain(char* out,unsigned cap,const char* input,bool title) {
    if(!cap)return;
    if(!input){out[0]=0;return;}
    // Titles use a label prefix ("WEAPON: "/"武器：") that is redundant in cards.
    const char* source=input;
    if(title)for(const char* p=input;*p;++p) {
        if(*p==':'&&p[1]){source=p+1;break;}
        if(static_cast<unsigned char>(p[0])==0xef &&
           static_cast<unsigned char>(p[1])==0xbc &&
           static_cast<unsigned char>(p[2])==0x9a && p[3]){source=p+3;break;}
    }
    while(*source==' ')++source;
    unsigned n=0;
    while(*source && n+1<cap) {
        if(source[0]=='['&&source[1]=='$'){
            while(*source&&*source!=']')++source;
            if(*source)++source;
            continue;
        }
        if(source[0]=='\\'&&(source[1]=='r'||source[1]=='n')) {
            if(source[1]=='n')out[n++]=' ';
            source+=2;continue;
        }
        out[n++]=*source++;
    }
    if(title) {
        while(n&&(out[n-1]==' '||out[n-1]==':'))--n;
        if(n>=3&&static_cast<unsigned char>(out[n-3])==0xef&&
           static_cast<unsigned char>(out[n-2])==0xbc&&static_cast<unsigned char>(out[n-1])==0x9a)n-=3;
    }
    out[n]=0;
}
}
