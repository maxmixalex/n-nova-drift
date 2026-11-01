#pragma once
// Host-side exact image encoding selection. This header is never included by ARM.
#include "image_codec.h"
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace nova { namespace codec {
using ImageBytes=std::vector<uint8_t>;
inline ImageBytes planar_rle_candidate(const ImageBytes& raw) {
    if(raw.empty()||raw.size()%3)throw std::runtime_error("Invalid RGB565+A8 source length");
    ImageBytes result;result.reserve(raw.size());
    const size_t count=raw.size()/3;
    for(unsigned plane=0;plane<2;++plane) {
        const unsigned width=plane?1:2,channel=plane?2:0;
        const auto equal=[&](size_t a,size_t b) {
            return raw[a*3+channel]==raw[b*3+channel] && (width==1||raw[a*3+1]==raw[b*3+1]);
        };
        const auto append_unit=[&](size_t p) {
            result.push_back(raw[p*3+channel]);if(width==2)result.push_back(raw[p*3+1]);
        };
        size_t p=0;
        while(p<count) {
            size_t repeat=1;while(p+repeat<count&&repeat<128&&equal(p,p+repeat))++repeat;
            if(repeat>=2){result.push_back(uint8_t(128+repeat-1));append_unit(p);p+=repeat;}
            else {
                const size_t start=p++;
                while(p<count&&p-start<128){if(p+1<count&&equal(p,p+1))break;++p;}
                result.push_back(uint8_t(p-start-1));for(size_t at=start;at<p;++at)append_unit(at);
            }
        }
    }
    return result;
}
inline ImageBytes palette_candidate(const ImageBytes& raw,bool separate_alpha,uint16_t& codec) {
    if(raw.empty()||raw.size()%3)throw std::runtime_error("Invalid RGB565+A8 source length");
    const size_t count=raw.size()/3;
    std::unordered_map<uint32_t,unsigned> lookup;lookup.reserve(257);
    std::vector<uint32_t> palette;palette.reserve(256);ImageBytes indices;indices.reserve(count);
    for(size_t p=0;p<count;++p) {
        const uint32_t color=uint32_t(raw[p*3])|(uint32_t(raw[p*3+1])<<8)|
            (separate_alpha?0u:(uint32_t(raw[p*3+2])<<16));
        auto found=lookup.find(color);unsigned index;
        if(found==lookup.end()) {
            if(palette.size()==256)return {};
            index=unsigned(palette.size());palette.push_back(color);lookup.emplace(color,index);
        } else index=found->second;
        indices.push_back(uint8_t(index));
    }
    const bool nibble=palette.size()<=16;
    codec=separate_alpha?(nibble?pack::Palette4_565_Alpha8:pack::Palette8_565_Alpha8):
        (nibble?pack::Palette4_565A8:pack::Palette8_565A8);
    ImageBytes result;result.reserve(2+palette.size()*(separate_alpha?2:3)+(nibble?(count+1)/2:count)+(separate_alpha?count:0));
    result.push_back(uint8_t(palette.size()));result.push_back(uint8_t(palette.size()>>8));
    for(uint32_t value:palette) {
        result.push_back(uint8_t(value));result.push_back(uint8_t(value>>8));
        if(!separate_alpha)result.push_back(uint8_t(value>>16));
    }
    if(nibble)for(size_t p=0;p<count;p+=2)result.push_back(uint8_t(indices[p]|(p+1<count?indices[p+1]<<4:0)));
    else result.insert(result.end(),indices.begin(),indices.end());
    if(separate_alpha)for(size_t p=0;p<count;++p)result.push_back(raw[p*3+2]);
    return result;
}
// Start with the existing smaller raw/RLE choice. Replace it only with an
// exactly reversible representation that is strictly smaller.
inline void select_image_encoding(const ImageBytes& raw,ImageBytes& encoded,uint16_t& codec) {
    for(bool separate_alpha:{false,true}) {
        uint16_t candidate_codec=0;ImageBytes candidate=palette_candidate(raw,separate_alpha,candidate_codec);
        if(!candidate.empty()&&candidate.size()<encoded.size()) {encoded.swap(candidate);codec=candidate_codec;}
    }
    ImageBytes planar=planar_rle_candidate(raw);
    // A second plane walk must save more than a marginal number of bytes.
    // Keep established codecs for tiny gains, reducing cold-decode work.
    if(planar.size()+32<=encoded.size()&&planar.size()*100<=encoded.size()*95){encoded.swap(planar);codec=pack::PlanarRle565A8;}
}
inline ImageBytes decode_image(const ImageBytes& encoded,uint16_t codec,size_t bytes) {
    if(!bytes||bytes%3||bytes>320u*240u*3u)throw std::runtime_error("Invalid decoded image size");
    const unsigned count=unsigned(bytes/3);std::vector<uint16_t> colors(count);ImageBytes alpha(count),out(bytes);
    MemoryReader reader{encoded.data(),unsigned(encoded.size())};
    if(!decode(reader,codec,colors.data(),alpha.data(),count))throw std::runtime_error("Invalid image codec stream");
    for(unsigned p=0;p<count;++p){out[p*3]=uint8_t(colors[p]);out[p*3+1]=uint8_t(colors[p]>>8);out[p*3+2]=alpha[p];}
    return out;
}
}}
