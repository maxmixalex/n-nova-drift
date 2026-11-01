#include "localization.h"
#include "platform.h"
#include "nnova/native_strings.h"
#include "packed_file.h"

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
bool Locale::open(unsigned language) {
    ready_=false;
    language_=language<Count?language:0;
    char filename[]="nNova.eng.locale.dat.tns";
    const char* languageCode=code(language_);
    for(unsigned i=0;i<3;++i)filename[6+i]=languageCode[i];
    return openFile(filename);
}
bool Locale::openFile(const char* filename) {
    ready_=false;
    platform::File* f=platform::open_data(filename);
    if(!f)return false;
    uint32_t size=0;
    if(!read_packed(f,data_,sizeof(data_),size)||size<sizeof(Header))return false;
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
    if(!key)return 0;
    // Native additions are translated in every supported language, and remain
    // available even before an owner's locale pack has been opened.
    for(unsigned i=0;i<native_text::EntryCount;++i)
        if(compare(native_text::entries[i].key,key)==0)
            return native_text::entries[i].values[language_<Count?language_:0];
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
const char* Locale::text(const char* key,const char* fallback) const {
    const char* value=get(key);return value&&*value?value:fallback;
}
const char* Locale::code(unsigned language) {
    return native_text::LanguageCodes[language<Count?language:0];
}
const char* Locale::name(unsigned language) {
    return native_text::LanguageNames[language<Count?language:0];
}
int Locale::fromSystem(const char* input) {
    if(!input||!*input)return -1;
    char tag[32]={0};unsigned n=0;
    for(;input[n]&&n+1<sizeof(tag);++n){char c=input[n];tag[n]=c>='A'&&c<='Z'?char(c+32):c;}
    const char* iso2[Count]={"en","de","es","fr","ja","ko","pt","ru","pl","zh","zh"};
    for(unsigned i=0;i<Count;++i)if(tag[0]==code(i)[0]&&tag[1]==code(i)[1]&&tag[2]==code(i)[2])return int(i);
    if(tag[0]=='z'&&tag[1]=='h') {
        for(unsigned i=2;tag[i];++i)
            if((tag[i]=='t'&&tag[i+1]=='w')||(tag[i]=='h'&&tag[i+1]=='k')||
               (i+3<sizeof(tag)&&tag[i]=='h'&&tag[i+1]=='a'&&tag[i+2]=='n'&&tag[i+3]=='t'))return 10;
        return 9;
    }
    for(unsigned i=0;i<9;++i)if(tag[0]==iso2[i][0]&&tag[1]==iso2[i][1])return int(i);
    return -1;
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
        if(*source=='\r'){++source;continue;}
        if(source[0]=='['&&source[1]=='$'){
            while(*source&&*source!=']')++source;
            if(*source)++source;
            continue;
        }
        if(source[0]=='\\'&&(source[1]=='r'||source[1]=='n')) {
            if(source[1]=='n')out[n++]='\n';
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
unsigned Locale::format(char* out,unsigned capacity,const char* source,bool rich) {
    struct Value { const char* key;const char* plain;const char* percent; };
    static const Value values[]={
#include "source_values.inc"
    };
    unsigned n=0,unresolved=0;if(!capacity)return 0;
    while(source&&*source&&n+1<capacity) {
        if(*source=='\r'){++source;continue;}
        if(source[0]=='['&&source[1]=='$') {
            const char* token=source+2;const char* end=token;
            while(*end&&*end!=']')++end;
            if(rich&&n+3<capacity) {
                char style=0;
                if(token[0]=='='&&token[1]=='*')style='D';
                else if(token[0]=='='&&token[1]=='/')style='W';
                else if(token[0]=='='&&token[1]=='r'&&token[2]=='g'&&token[3]=='b')
                    style=token[5]=='8'?'B':token[5]=='2'&&token[6]=='5'&&token[7]=='0'?'R':'W';
                if(style){out[n++]=27;out[n++]=style;}
            }
            if(token[0]=='='&&token[1]=='{') {
                token+=2;bool percent=false;char sign=0;
                while(*token=='%'||*token=='+'||*token=='-') {
                    if(*token=='%')percent=true;else sign=*token;
                    ++token;
                }
                char key[96]={0};unsigned k=0;
                while(token<end&&*token!='}'&&k+1<sizeof(key))key[k++]=*token++;
                unsigned low=0,high=sizeof(values)/sizeof(values[0]);
                while(low<high){unsigned mid=low+(high-low)/2;if(compare(values[mid].key,key)<0)low=mid+1;else high=mid;}
                const char* value="?";
                if(low<sizeof(values)/sizeof(values[0])&&compare(values[low].key,key)==0)
                    value=percent?values[low].percent:values[low].plain;
                else ++unresolved;
                if(sign&&n+1<capacity&&*value!='-')out[n++]=sign;
                while(*value&&n+1<capacity)out[n++]=*value++;
                if(percent&&n+1<capacity)out[n++]='%';
            }
            source=*end?end+1:end;continue;
        }
        if(source[0]=='\\'&&(source[1]=='r'||source[1]=='n')) {
            if(source[1]=='n'&&(!n||out[n-1]!='\n'))out[n++]='\n';
            source+=2;continue;
        }
        out[n++]=*source++;
    }
    out[n]=0;return unresolved;
}
}
