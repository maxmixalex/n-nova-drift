// Adapted from nSpire, Copyright (c) 2026 Max Alex; MIT license.
// See third_party/licenses/nSpire-MIT.txt.
#include <stddef.h>
#include <stdint.h>
// ARM926 traps/rotates unaligned word accesses. Only use word lanes when
// both addresses can be aligned together; short/differently aligned copies
// retain byte accesses. may_alias keeps these libc primitives valid for the
// object representations of every caller, not only uint32_t arrays.
using MemoryWord = uint32_t __attribute__((__may_alias__));
extern "C" {
void* memcpy(void*to,const void*from,size_t n){
    auto*d=(unsigned char*)to;const auto*s=(const unsigned char*)from;
    if(n>=16&&!((uintptr_t(d)^uintptr_t(s))&3u)){
        while(uintptr_t(d)&3u){*d++=*s++;--n;}
        auto*words=(MemoryWord*)d;const auto*source=(const MemoryWord*)s;
        while(n>=16){words[0]=source[0];words[1]=source[1];words[2]=source[2];words[3]=source[3];words+=4;source+=4;n-=16;}
        while(n>=4){*words++=*source++;n-=4;}
        d=(unsigned char*)words;s=(const unsigned char*)source;
    }
    while(n--)*d++=*s++;return to;
}
void* memset(void*to,int v,size_t n){
    auto*d=(unsigned char*)to;const auto byte=(unsigned char)v;
    if(n>=16){
        while(uintptr_t(d)&3u){*d++=byte;--n;}
        const uint32_t fill=uint32_t(byte)*0x01010101u;auto*words=(MemoryWord*)d;
        while(n>=16){words[0]=fill;words[1]=fill;words[2]=fill;words[3]=fill;words+=4;n-=16;}
        while(n>=4){*words++=fill;n-=4;}d=(unsigned char*)words;
    }
    while(n--)*d++=byte;return to;
}
void* memmove(void*to,const void*from,size_t n){auto*d=(unsigned char*)to;const auto*s=(const unsigned char*)from;if(d<s){for(size_t i=0;i<n;i++)d[i]=s[i];}else{while(n){--n;d[n]=s[n];}}return to;}
int memcmp(const void*a,const void*b,size_t n){
    auto*x=(const unsigned char*)a;auto*y=(const unsigned char*)b;
    if(n>=16&&!((uintptr_t(x)^uintptr_t(y))&3u)){
        while(uintptr_t(x)&3u){if(*x!=*y)return int(*x)-*y;++x;++y;--n;}
        const auto*first=(const MemoryWord*)x;const auto*second=(const MemoryWord*)y;
        // A mismatching word is inspected again in byte order below. Word
        // numeric order differs from first-unsigned-byte order on little endian.
        while(n>=4&&*first==*second){++first;++second;n-=4;}
        x=(const unsigned char*)first;y=(const unsigned char*)second;
    }
    for(size_t i=0;i<n;++i)if(x[i]!=y[i])return int(x[i])-y[i];return 0;
}
size_t strlen(const char*p){size_t n=0;while(p[n])n++;return n;}
}
