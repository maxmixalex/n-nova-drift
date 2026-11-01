#include "nnova/image_encode.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <numeric>
#include <chrono>
using namespace nova;
namespace fs=std::filesystem;
using Bytes=codec::ImageBytes;
static uint32_t crc(const uint8_t* p,size_t bytes) {
    uint32_t c=~0u;while(bytes--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;
}
static Bytes read(const fs::path& path) {
    std::ifstream f(path,std::ios::binary);assert(f);return Bytes(std::istreambuf_iterator<char>(f),{});
}
static void write(const fs::path& path,const Bytes& b) {
    std::ofstream f(path,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));assert(f);
}
static bool rejected(const Bytes& bytes,uint16_t codec,unsigned pixels) {
    try{(void)nova::codec::decode_image(bytes,codec,pixels*3);return false;}catch(const std::runtime_error&){return true;}
}
static void synthetic() {
    unsigned cases=0;
    for(unsigned count:{1u,2u,3u,17u,127u,128u,129u,255u,256u,513u,1025u})for(unsigned entries:{1u,2u,16u,17u,255u,256u,257u}) {
        Bytes raw;for(unsigned p=0;p<count;++p) {
            const unsigned value=p%entries;raw.push_back(uint8_t(value));raw.push_back(uint8_t(value>>8));raw.push_back(uint8_t(value*47));
        }
        for(bool alpha:{false,true}) {
            uint16_t format;Bytes encoded=codec::palette_candidate(raw,alpha,format);
            if(encoded.empty()) {assert(entries>256&&count>256);continue;}
            assert(codec::decode_image(encoded,format,raw.size())==raw);
            Bytes truncated=encoded;truncated.pop_back();assert(rejected(truncated,format,count));
            Bytes trailing=encoded;trailing.push_back(0);assert(rejected(trailing,format,count));++cases;
        }
        uint16_t format=pack::Raw565A8;Bytes selected=raw;
        codec::select_image_encoding(raw,selected,format);assert(selected.size()<=raw.size());
        assert(codec::decode_image(selected,format,raw.size())==raw);
        const Bytes planar=codec::planar_rle_candidate(raw);
        assert(codec::decode_image(planar,pack::PlanarRle565A8,raw.size())==raw);
        Bytes truncated=planar;truncated.pop_back();assert(rejected(truncated,pack::PlanarRle565A8,count));
        Bytes trailing=planar;trailing.push_back(0);assert(rejected(trailing,pack::PlanarRle565A8,count));
    }
    // Exactly two RGB colors with all 256 alpha values: alpha must survive
    // unchanged, including transparent nonzero RGB and intermediate coverage.
    Bytes raw;for(unsigned i=0;i<1024;++i){raw.push_back(uint8_t(i&1));raw.push_back(0xf8);raw.push_back(uint8_t(i/2));}
    uint16_t format=pack::Raw565A8;Bytes selected=raw;codec::select_image_encoding(raw,selected,format);
    assert(format==pack::Palette4_565_Alpha8&&codec::decode_image(selected,format,raw.size())==raw);
    assert(rejected(Bytes{0,0},pack::Palette4_565A8,1));
    assert(rejected(Bytes{17,0},pack::Palette4_565A8,1));
    assert(rejected(Bytes{1,0,0,0,255,1},pack::Palette4_565A8,1)); // invalid palette index
    assert(rejected(Bytes{1,0,0,0,255,16},pack::Palette4_565A8,1)); // dirty odd high nibble
    assert(rejected(Bytes{255,0,0},pack::Rle565A8,128));
    assert(rejected(Bytes{129,0,0,0,255},pack::PlanarRle565A8,1)); // color run overflows
    assert(rejected(Bytes{0,0,0,129,255},pack::PlanarRle565A8,1)); // alpha run overflows
    assert(!pack::supported_codec(pack::PlanarRle565A8,2)&&pack::supported_codec(pack::PlanarRle565A8,3));
    assert(rejected(Bytes{0,0,0},99,1));
    std::printf("Exact image codec passed: %u round trips, byte-for-byte alpha, strict malformed-stream rejection.\n",cases);
}
static void audit(const fs::path& source,const fs::path& target,bool palette=true) {
    Bytes index=read(source/"nNova.dat.tns");pack::CommonHeader h;assert(index.size()>=sizeof(h));std::memcpy(&h,index.data(),sizeof(h));
    assert(pack::supported_version(h.version)&&h.file_bytes==index.size());
    std::vector<Bytes> oldpacks;for(unsigned p=0;p<h.pack_count;++p){char name[32];std::snprintf(name,sizeof(name),"nNova.gfx%02u.dat.tns",p);oldpacks.push_back(read(source/name));}
    fs::create_directories(target);std::vector<Bytes> newpacks(1,Bytes(sizeof(pack::DataHeader)));
    std::map<uint64_t,pack::FrameRecord> shared;uint64_t oldbytes=0,newbytes=0,rawbytes=0;unsigned formats[7]={};
    for(unsigned i=0;i<h.frame_count;++i) {
        pack::FrameRecord f;const unsigned at=h.frame_offset+i*sizeof(f);assert(at+sizeof(f)<=index.size());std::memcpy(&f,index.data()+at,sizeof(f));
        assert(f.pack<oldpacks.size()&&f.offset<=oldpacks[f.pack].size()&&f.stored_bytes<=oldpacks[f.pack].size()-f.offset);
        Bytes encoded(oldpacks[f.pack].begin()+f.offset,oldpacks[f.pack].begin()+f.offset+f.stored_bytes);
        Bytes raw=codec::decode_image(encoded,f.codec,f.decoded_bytes);rawbytes+=raw.size();
        const uint64_t key=(uint64_t(f.pack)<<32)|f.offset;
        auto found=shared.find(key);
        if(found!=shared.end()) {
            f.pack=found->second.pack;f.offset=found->second.offset;f.stored_bytes=found->second.stored_bytes;f.codec=found->second.codec;
        } else {
            oldbytes+=encoded.size();if(palette)codec::select_image_encoding(raw,encoded,f.codec);newbytes+=encoded.size();++formats[f.codec];
            assert(codec::decode_image(encoded,f.codec,raw.size())==raw);
            if(newpacks.back().size()+encoded.size()>2*1024*1024)newpacks.emplace_back(sizeof(pack::DataHeader));
            f.pack=uint16_t(newpacks.size()-1);f.offset=uint32_t(newpacks.back().size());f.stored_bytes=uint32_t(encoded.size());
            newpacks.back().insert(newpacks.back().end(),encoded.begin(),encoded.end());shared.emplace(key,f);
        }
        // Verify every frame record after remapping, including duplicate frames.
        const Bytes& pack=newpacks[f.pack];Bytes stored(pack.begin()+f.offset,pack.begin()+f.offset+f.stored_bytes);
        assert(codec::decode_image(stored,f.codec,raw.size())==raw);
        std::memcpy(index.data()+at,&f,sizeof(f));
    }
    for(unsigned p=0;p<newpacks.size();++p) {
        auto& bytes=newpacks[p];pack::DataHeader dh{};std::memcpy(dh.magic,"NNOVAP1",8);dh.version=pack::Version;dh.index=p;dh.file_bytes=uint32_t(bytes.size());dh.payload_crc32=crc(bytes.data()+sizeof(dh),bytes.size()-sizeof(dh));
        std::memcpy(bytes.data(),&dh,sizeof(dh));char name[32];std::snprintf(name,sizeof(name),"nNova.gfx%02u.dat.tns",p);write(target/name,bytes);
    }
    h.version=pack::Version;h.pack_count=uint32_t(newpacks.size());h.index_crc32=crc(index.data()+sizeof(h),index.size()-sizeof(h));std::memcpy(index.data(),&h,sizeof(h));write(target/"nNova.dat.tns",index);
    std::printf("Real-pack audit: %u frames, %zu unique, %llu decoded bytes EXACT; payload %llu -> %llu, saved %llu (%.2f%%); %zu packs.\n",h.frame_count,shared.size(),(unsigned long long)rawbytes,(unsigned long long)oldbytes,(unsigned long long)newbytes,(unsigned long long)(oldbytes-newbytes),100.0*double(oldbytes-newbytes)/double(oldbytes),newpacks.size());
    std::printf("Unique codecs raw/rle/rgba4/rgba8/rgb4a8/rgb8a8/planar: %u %u %u %u %u %u %u\n",formats[0],formats[1],formats[2],formats[3],formats[4],formats[5],formats[6]);
}
static void compare_packs(const fs::path& first,const fs::path& second) {
    Bytes index[2]={read(first/"nNova.dat.tns"),read(second/"nNova.dat.tns")};pack::CommonHeader h[2];
    std::vector<Bytes> packs[2];const fs::path paths[2]={first,second};
    for(unsigned k=0;k<2;++k) {
        assert(index[k].size()>=sizeof(h[k]));std::memcpy(&h[k],index[k].data(),sizeof(h[k]));
        for(unsigned p=0;p<h[k].pack_count;++p){char name[32];std::snprintf(name,sizeof(name),"nNova.gfx%02u.dat.tns",p);packs[k].push_back(read(paths[k]/name));}
    }
    assert(h[0].frame_count==h[1].frame_count&&h[0].sprite_count==h[1].sprite_count);
    assert(!std::memcmp(index[0].data()+h[0].sprite_offset,index[1].data()+h[1].sprite_offset,h[0].sprite_count*sizeof(pack::SpriteRecord)));
    uint64_t decoded=0;
    for(unsigned i=0;i<h[0].frame_count;++i) {
        Bytes raw[2];pack::FrameRecord f[2];
        for(unsigned k=0;k<2;++k) {
            std::memcpy(&f[k],index[k].data()+h[k].frame_offset+i*sizeof(f[k]),sizeof(f[k]));
            const auto& r=f[k];const Bytes& p=packs[k][r.pack];
            raw[k]=codec::decode_image(Bytes(p.begin()+r.offset,p.begin()+r.offset+r.stored_bytes),r.codec,r.decoded_bytes);
        }
        assert(raw[0]==raw[1]);assert(f[0].width==f[1].width&&f[0].height==f[1].height&&f[0].origin_x==f[1].origin_x&&f[0].origin_y==f[1].origin_y);
        assert(f[0].canvas_width==f[1].canvas_width&&f[0].canvas_height==f[1].canvas_height&&f[0].original_frame==f[1].original_frame);decoded+=raw[0].size();
    }
    std::printf("Pack comparison: %u sprites, %u frames and %llu pixel bytes exactly equal; geometry, origins, FPS and original frame IDs preserved.\n",h[0].sprite_count,h[0].frame_count,(unsigned long long)decoded);
}
static void benchmark(const fs::path& source) {
    Bytes index=read(source/"nNova.dat.tns");pack::CommonHeader h{};std::memcpy(&h,index.data(),sizeof(h));
    std::vector<Bytes> packs;
    for(unsigned i=0;i<h.pack_count;++i){char name[32];std::snprintf(name,sizeof(name),"nNova.gfx%02u.dat.tns",i);packs.push_back(read(source/name));}
    std::map<uint64_t,pack::FrameRecord> unique;
    for(unsigned i=0;i<h.frame_count;++i){pack::FrameRecord f;std::memcpy(&f,index.data()+h.frame_offset+i*sizeof(f),sizeof(f));unique.emplace((uint64_t(f.pack)<<32)|f.offset,f);}
    // Exercise the runtime decoder into fixed planes: no file I/O, allocation,
    // resampling or palette construction occurs inside the timed section.
    std::vector<uint16_t> colors(320*240);Bytes alpha(320*240);volatile uint32_t checksum=0;
    double best=1e30;uint64_t encoded=0,decoded=0;
    for(const auto& item:unique){encoded+=item.second.stored_bytes;decoded+=item.second.decoded_bytes;}
    for(unsigned sample=0;sample<3;++sample){
        const auto started=std::chrono::steady_clock::now();
        for(unsigned repeat=0;repeat<10;++repeat)for(const auto& item:unique){const auto& f=item.second;
            codec::MemoryReader stream{packs[f.pack].data()+f.offset,f.stored_bytes};
            assert(codec::decode(stream,f.codec,colors.data(),alpha.data(),f.decoded_bytes/3));
            checksum=checksum+colors[repeat%(f.decoded_bytes/3)]+alpha[repeat%(f.decoded_bytes/3)];
        }
        const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()/10;
        if(elapsed<best)best=elapsed;
    }
    std::printf("Decode benchmark %s: %zu unique cache misses, %llu input bytes, %llu decoded bytes, %.3f ms/sweep (best of 3 x 10 sweeps; memory-only; checksum %u).\n",source.u8string().c_str(),unique.size(),(unsigned long long)encoded,(unsigned long long)decoded,best,unsigned(checksum));
}
int main(int argc,char** argv) {
    synthetic();
    if(argc==3&&!std::strcmp(argv[2],"--benchmark"))benchmark(argv[1]);
    else if(argc==4&&!std::strcmp(argv[3],"--compare"))compare_packs(argv[1],argv[2]);
    else if(argc==3||argc==4){assert(argc==3||!std::strcmp(argv[3],"--keep-codec"));audit(argv[1],argv[2],argc==3);}
    else assert(argc==1);
}
