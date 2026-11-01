// nNova resource converter. The game data remains on the owner's computer.
// This parser reads only the documented, bounds-checked resource portions of
// GameMaker FORM files; it neither runs nor decompiles executable game code.
#include "nnova/pack_format.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"
#include "ui_glyphs.h"

namespace fs = std::filesystem;
using Bytes = std::vector<uint8_t>;
using namespace nova;

static void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
static fs::path input_path(const char* text) {
    // Windows main reconstructs arguments from the original UTF-16 command
    // line; both platforms therefore reach this boundary with UTF-8 paths.
    return fs::u8path(text);
}
static fs::path executable_path(const char* argv0) {
#ifdef _WIN32
    std::wstring path(32768,L'\0');
    DWORD size=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    if(size>0&&size<path.size()){path.resize(size);return fs::path(path);}
#else
    std::array<char,4096> path{};
    auto size=readlink("/proc/self/exe",path.data(),path.size()-1);
    if(size>0&&size<ssize_t(path.size()))return fs::u8path(std::string(path.data(),size_t(size)));
#endif
    return fs::absolute(input_path(argv0));
}
static Bytes read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    require(bool(file), "Cannot open " + path.u8string());
    auto n = file.tellg();
    require(n >= 0 && uint64_t(n) < 0x80000000ull, "File is too large: " + path.u8string());
    Bytes bytes(static_cast<size_t>(n)); file.seekg(0);
    if (!bytes.empty()) file.read(reinterpret_cast<char*>(bytes.data()), n);
    require(bool(file), "Cannot read " + path.u8string()); return bytes;
}
static void write_file(const fs::path& path, const Bytes& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    require(bool(file), "Cannot create " + path.u8string());
    file.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    require(bool(file), "Cannot finish " + path.u8string());
}
template<class T> static void append(Bytes& b, const T& value) {
    const auto* p = reinterpret_cast<const uint8_t*>(&value); b.insert(b.end(), p, p + sizeof(value));
}
static uint32_t crc32(const uint8_t* data, size_t size) {
    static uint32_t table[256] = {};
    static bool initialized = false;
    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xedb88320u & (0u-(c&1)));
            table[i] = c;
        }
        initialized = true;
    }
    uint32_t c = ~0u;
    for (size_t i = 0; i < size; ++i) c = table[(c ^ data[i]) & 255] ^ (c >> 8);
    return ~c;
}
static uint64_t hash64(const Bytes& b) {
    uint64_t h = 14695981039346656037ull;
    for (uint8_t v : b) { h ^= v; h *= 1099511628211ull; }
    return h;
}
struct Reader {
    const Bytes& b;
    void range(uint64_t p, uint64_t n) const {
        require(p <= b.size() && n <= b.size() - p, "Game data pointer is outside the file");
    }
    uint16_t u16(uint64_t p) const { range(p,2); return uint16_t(b[p] | unsigned(b[p+1]) << 8); }
    uint32_t u32(uint64_t p) const { range(p,4); return uint32_t(b[p]) | uint32_t(b[p+1])<<8 | uint32_t(b[p+2])<<16 | uint32_t(b[p+3])<<24; }
    float f32(uint64_t p) const { uint32_t x=u32(p); float f; std::memcpy(&f,&x,4); return f; }
    std::string str(uint32_t p) const {
        range(p,1); auto end = std::find(b.begin()+p, b.end(), uint8_t(0));
        require(end != b.end() && size_t(end-(b.begin()+p)) < 1048576, "Invalid GameMaker string");
        return std::string(b.begin()+p,end);
    }
};
struct Chunk { uint32_t start, size; };
struct SourceSprite {
    std::string name; uint32_t width, height; int32_t ox, oy; float fps;
    std::vector<uint32_t> frames;
};
struct Texture { uint32_t offset, size, width, height; };
struct PageItem { uint16_t x,y,w,h,tx,ty,tw,th,bw,bh; int16_t texture; };
struct GameData {
    Bytes bytes; Reader r; std::map<std::string,Chunk> chunks;
    std::vector<SourceSprite> sprites; std::vector<Texture> textures;
    explicit GameData(const fs::path& file):bytes(read_file(file)),r{bytes} {
        r.range(0,8); require(std::memcmp(bytes.data(),"FORM",4)==0, "Expected a GameMaker FORM data.win");
        require(uint64_t(r.u32(4))+8==bytes.size(), "GameMaker FORM size mismatch");
        for (uint64_t p=8;p<bytes.size();) {
            r.range(p,8); uint32_t n=r.u32(p+4); r.range(p+8,n);
            std::string type(bytes.begin()+p,bytes.begin()+p+4);
            require(!chunks.count(type),"Duplicate FORM chunk: "+type);
            chunks[type]={uint32_t(p+8),n}; p+=8+uint64_t(n);
        }
        for (const char* type:{"SPRT","TPAG","TXTR","STRG"}) require(chunks.count(type),std::string("Missing ")+type+" chunk");
        auto sc=chunks.at("SPRT");uint32_t ns=r.u32(sc.start);
        require(ns<=100000,"Unreasonable sprite count");r.range(sc.start+4,uint64_t(ns)*4);
        for(uint32_t i=0;i<ns;++i) {
            uint32_t p=r.u32(sc.start+4+i*4);if(!p)continue;r.range(p,60);
            SourceSprite s;s.name=r.str(r.u32(p));s.width=r.u32(p+4);s.height=r.u32(p+8);
            s.ox=int32_t(r.u32(p+48));s.oy=int32_t(r.u32(p+52));s.fps=30;
            require(s.name.size()<64 && s.width>0 && s.height>0 && s.width<=32768 && s.height<=32768,"Unsupported sprite geometry/name: "+s.name);
            uint32_t list=p+56;
            if(r.u32(list)==0xffffffffu) {
                uint32_t version=r.u32(p+60),type=r.u32(p+64);
                require(version>=1 && version<=3 && type==0,"Unsupported non-bitmap sprite: "+s.name);
                s.fps=r.f32(p+68);uint32_t speed_type=r.u32(p+72);
                require(std::isfinite(s.fps) && s.fps>=0,"Invalid sprite frame rate");
                if(speed_type==1)s.fps*=60;
                list=p+76+(version>=2?4:0)+(version>=3?4:0);
            }
            uint32_t count=r.u32(list);require(count<=4096,"Too many frames: "+s.name);
            r.range(list+4,uint64_t(count)*4);
            for(uint32_t f=0;f<count;++f) {uint32_t tp=r.u32(list+4+f*4);if(tp)s.frames.push_back(tp);}
            if(!s.frames.empty())sprites.push_back(std::move(s));
        }
        std::sort(sprites.begin(),sprites.end(),[](const auto&a,const auto&b){return a.name<b.name;});
        auto tc=chunks.at("TXTR");uint32_t nt=r.u32(tc.start);
        require(nt<=10000,"Unreasonable texture count");r.range(tc.start+4,uint64_t(nt)*4);
        for(uint32_t i=0;i<nt;++i) {
            uint32_t p=r.u32(tc.start+4+i*4);r.range(p,28);
            Texture t{r.u32(p+24),r.u32(p+8),r.u32(p+12),r.u32(p+16)};
            require(t.width>0 && t.height>0 && t.width<=16384 && t.height<=16384,"Unsupported texture dimensions");
            if(t.offset)r.range(t.offset,t.size);
            textures.push_back(t);
        }
    }
    PageItem page(uint32_t p) const {
        const auto& c=chunks.at("TPAG");
        require(p>=c.start && uint64_t(p)+22<=uint64_t(c.start)+c.size,"Sprite points outside TPAG");
        PageItem x{r.u16(p),r.u16(p+2),r.u16(p+4),r.u16(p+6),r.u16(p+8),r.u16(p+10),r.u16(p+12),r.u16(p+14),r.u16(p+16),r.u16(p+18),int16_t(r.u16(p+20))};
        require(x.texture>=0 && unsigned(x.texture)<textures.size() && x.w && x.h && x.tw && x.th,"Invalid texture page item");
        const auto&t=textures[size_t(x.texture)];
        require(uint32_t(x.x)+x.w<=t.width && uint32_t(x.y)+x.h<=t.height,"Texture crop is out of bounds");
        return x;
    }
};
struct Pixel { uint8_t r,g,b,a; };
static Pixel color565(uint16_t x) {
    return {uint8_t(((x>>11)*255+15)/31),uint8_t((((x>>5)&63)*255+31)/63),uint8_t(((x&31)*255+15)/31),255};
}
static std::vector<Pixel> decode_dxt5(const GameData& game,const Texture&t) {
    const Reader&r=game.r;const auto&b=game.bytes;uint32_t p=t.offset;
    require(p && t.size>=128 && std::memcmp(b.data()+p,"DDS ",4)==0,"Referenced texture is not supported DDS");
    require(r.u32(p+4)==124 && r.u32(p+12)==t.height && r.u32(p+16)==t.width,"Invalid DDS header");
    require(std::memcmp(b.data()+p+84,"DXT5",4)==0,"Only DXT5/BC3 is supported in this source build");
    unsigned cols=(t.width+3)/4,rows=(t.height+3)/4;
    require(uint64_t(cols)*rows*16+128<=t.size,"Truncated DXT5 texture");
    std::vector<Pixel> out(size_t(t.width)*t.height);
    uint64_t block=p+128;
    for(unsigned by=0;by<rows;++by)for(unsigned bx=0;bx<cols;++bx,block+=16) {
        uint8_t alpha[8]={b[block],b[block+1],0,0,0,0,0,0};
        if(alpha[0]>alpha[1])for(unsigned k=2;k<8;++k)alpha[k]=uint8_t(((8-k)*alpha[0]+(k-1)*alpha[1])/7);
        else {for(unsigned k=2;k<6;++k)alpha[k]=uint8_t(((6-k)*alpha[0]+(k-1)*alpha[1])/5);alpha[6]=0;alpha[7]=255;}
        uint64_t abits=0;for(unsigned k=0;k<6;++k)abits|=uint64_t(b[block+2+k])<<(8*k);
        Pixel cs[4]={color565(r.u16(block+8)),color565(r.u16(block+10)),{}, {}};
        cs[2]={uint8_t((2*cs[0].r+cs[1].r)/3),uint8_t((2*cs[0].g+cs[1].g)/3),uint8_t((2*cs[0].b+cs[1].b)/3),255};
        cs[3]={uint8_t((cs[0].r+2*cs[1].r)/3),uint8_t((cs[0].g+2*cs[1].g)/3),uint8_t((cs[0].b+2*cs[1].b)/3),255};
        uint32_t cbits=r.u32(block+12);
        for(unsigned k=0;k<16;++k) {
            unsigned x=bx*4+k%4,y=by*4+k/4;if(x>=t.width||y>=t.height)continue;
            Pixel c=cs[(cbits>>(2*k))&3];c.a=alpha[(abits>>(3*k))&7];out[size_t(y)*t.width+x]=c;
        }
    }
    return out;
}
static bool has(const std::string&s,const char*text){return s.find(text)!=std::string::npos;}
struct Sizing { double x,y; std::string category; };
static Sizing sizing(const SourceSprite&s) {
    const auto&n=s.name;double w=48,h=48;std::string c="effect";
    if(n.rfind("bgStars",0)==0)return {320.0/s.width,240.0/s.height,"background"};
    if(n=="sNovaDriftLogo"){w=240;h=60;c="title";}
    else if(n=="sPixeljam"||n=="sChimeric"){w=160;h=60;c="credit";}
    else if(n=="sMenuPlanetRings"){w=270;h=190;c="menu";}
    else if(n=="sMenuPlanet"){w=160;h=160;c="menu";}
    else if(n=="sMenuNebula"){w=320;h=240;c="menu";}
    else if(n=="sMenuStar"){w=150;h=150;c="menu";}
    else if(has(n,"Planet")||has(n,"Nebula")||has(n,"AchiEndingStar")){w=240;h=200;c="background";}
    else if(n.rfind("sHex",0)==0||n.rfind("sMode",0)==0||n.rfind("sUnlock",0)==0){w=24;h=28;c="icon";}
    else if(n.rfind("sPlayer",0)==0){w=26;h=26;c="player";}
    else if(has(n,"AsteroidSmall")){w=18;h=18;c="asteroid";}
    else if(has(n,"AsteroidMedium")){w=28;h=28;c="asteroid";}
    else if(has(n,"AsteroidLarge")){w=40;h=40;c="asteroid";}
    else if(has(n,"AsteroidMega")){w=60;h=60;c="asteroid";}
    else if(n.rfind("s_power_",0)==0||n=="s_bounty_get"){w=9;h=9;c="pickup";}
    else if(has(n,"Dreadnought")||has(n,"BulwarkBody")||has(n,"Juggernaut")||n=="sSeraph"||n=="sScion"||n=="sDweller"||n=="sSpitfire"){w=64;h=72;c="boss";}
    else if(has(n,"StarEater")){w=84;h=84;c="boss";}
    else if(has(n,"Eclipse")||has(n,"Warbringer")){w=52;h=52;c="boss";}
    else if(has(n,"Construct")||has(n,"Drone")||n=="sSwarmling"||n=="sPulsebug"){w=22;h=24;c="actor";}
    else if(n=="sInterceptor"||n=="sVanguard"||n=="sSine"||has(n,"Hammerhead")||n=="sWarden"||n=="sMineLayer"||n=="sPulsar"||n=="sSpikeball"){w=30;h=32;c="enemy";}
    else if(has(n,"Bullet")||has(n,"DefaultWeapon")||has(n,"SplitShot")||has(n,"Dart")||has(n,"Salvo")||has(n,"Flak")||has(n,"Shot")||has(n,"Shard")||has(n,"Missile")||has(n,"Projectile")){w=19;h=12;c="projectile";}
    else if(has(n,"Thrust")||has(n,"Thruster")){w=18;h=18;c="thrust";}
    else if(has(n,"ExpBar")||has(n,"Divider")||has(n,"Banner")){w=260;h=22;c="ui";}
    else if(has(n,"Controls")||has(n,"Tutorial")){w=240;h=120;c="ui";}
    else if(has(n,"Selecta")||has(n,"Cooldown")||n=="sReroll"){w=28;h=28;c="ui";}
    double scale=std::min({1.0,w/s.width,h/s.height});return {scale,scale,c};
}
struct Resized { uint16_t width,height;int16_t ox,oy;Bytes pixels; };
static Resized resize(const std::vector<Pixel>&texture,const Texture&t,const PageItem&p,const SourceSprite&s,const Sizing&scale) {
    int w=std::max(1,int(std::lround(p.tw*scale.x))),h=std::max(1,int(std::lround(p.th*scale.y)));
    require(w<=320&&h<=240,"Output image exceeds display bounds: "+s.name);
    std::vector<Pixel> out(size_t(w)*h);int minx=w,miny=h,maxx=-1,maxy=-1;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x) {
        double left=double(x)*p.w/w,right=double(x+1)*p.w/w,top=double(y)*p.h/h,bottom=double(y+1)*p.h/h;
        double a=0,rr=0,gg=0,bb=0,weight=(right-left)*(bottom-top);
        for(int yy=int(top);yy<int(std::ceil(bottom));++yy)for(int xx=int(left);xx<int(std::ceil(right));++xx) {
            double coverage=(std::min(right,double(xx+1))-std::max(left,double(xx)))*(std::min(bottom,double(yy+1))-std::max(top,double(yy)));
            const Pixel&c=texture[(size_t(p.y)+yy)*t.width+p.x+xx];double aw=coverage*c.a;
            a+=aw;rr+=aw*c.r;gg+=aw*c.g;bb+=aw*c.b;
        }
        Pixel c{};c.a=uint8_t(std::clamp(int(std::lround(a/weight)),0,255));
        if(c.a&&a>0){c.r=uint8_t(std::clamp(int(std::lround(rr/a)),0,255));c.g=uint8_t(std::clamp(int(std::lround(gg/a)),0,255));c.b=uint8_t(std::clamp(int(std::lround(bb/a)),0,255));minx=std::min(minx,x);maxx=std::max(maxx,x);miny=std::min(miny,y);maxy=std::max(maxy,y);}
        out[size_t(y)*w+x]=c;
    }
    if(maxx<minx){minx=miny=0;maxx=maxy=0;}
    Resized result{uint16_t(maxx-minx+1),uint16_t(maxy-miny+1),int16_t(std::lround((s.ox-p.tx)*scale.x)-minx),int16_t(std::lround((s.oy-p.ty)*scale.y)-miny),{}};
    result.pixels.reserve(size_t(result.width)*result.height*3);
    for(int y=miny;y<=maxy;++y)for(int x=minx;x<=maxx;++x){const Pixel&c=out[size_t(y)*w+x];uint16_t rgb=c.a?uint16_t(((c.r*31+127)/255)<<11|((c.g*63+127)/255)<<5|((c.b*31+127)/255)):0;result.pixels.push_back(uint8_t(rgb));result.pixels.push_back(uint8_t(rgb>>8));result.pixels.push_back(c.a);}
    return result;
}
static Bytes encode_rle(const Bytes&raw) {
    size_t n=raw.size()/3,p=0;Bytes encoded;
    auto equal=[&](size_t a,size_t b){return raw[a*3]==raw[b*3]&&raw[a*3+1]==raw[b*3+1]&&raw[a*3+2]==raw[b*3+2];};
    while(p<n){size_t run=1;while(p+run<n&&run<128&&equal(p,p+run))++run;
        if(run>=3){encoded.push_back(uint8_t(128+run-1));encoded.insert(encoded.end(),raw.begin()+p*3,raw.begin()+p*3+3);p+=run;}
        else {size_t start=p;p+=run;while(p<n&&p-start<128){run=1;while(p+run<n&&run<3&&equal(p,p+run))++run;if(run>=3)break;size_t take=std::min(run,128-(p-start));p+=take;}
            encoded.push_back(uint8_t(p-start-1));encoded.insert(encoded.end(),raw.begin()+start*3,raw.begin()+p*3);}
    }return encoded;
}
static Bytes decode_rle(const Bytes&in,size_t expected) {
    Bytes out;size_t p=0;while(p<in.size()){uint8_t c=in[p++];size_t n=(c&127)+1;require(out.size()+n*3<=expected,"RLE exceeds destination");
        if(c&128){require(p+3<=in.size(),"Truncated RLE packet");for(size_t i=0;i<n;++i)out.insert(out.end(),in.begin()+p,in.begin()+p+3);p+=3;}
        else{require(p+n*3<=in.size(),"Truncated RLE literal");out.insert(out.end(),in.begin()+p,in.begin()+p+n*3);p+=n*3;}}
    require(out.size()==expected,"RLE output size mismatch");return out;
}
struct PackWriter {
    fs::path dir;Bytes bytes;unsigned id=0;uint64_t total=0;unsigned pack_count=0;
    static constexpr size_t Limit=2u*1024u*1024u;
    explicit PackWriter(fs::path d):dir(std::move(d)),bytes(sizeof(pack::DataHeader),0){}
    void flush(){if(bytes.size()==sizeof(pack::DataHeader))return;pack::DataHeader h{};std::memcpy(h.magic,"NNOVAP1\0",8);h.version=pack::Version;h.index=id;h.file_bytes=uint32_t(bytes.size());h.payload_crc32=crc32(bytes.data()+sizeof(h),bytes.size()-sizeof(h));std::memcpy(bytes.data(),&h,sizeof(h));char name[64];std::snprintf(name,sizeof(name),"nNova.gfx%02u.dat.tns",id);write_file(dir/name,bytes);total+=bytes.size();++id;++pack_count;bytes.assign(sizeof(h),0);}
    void add(const Bytes&data,pack::FrameRecord&record){if(bytes.size()+data.size()>Limit)flush();require(id<pack::MaxPacks,"More than 32 graphics packs; reduce resource dimensions");record.pack=uint16_t(id);record.offset=uint32_t(bytes.size());record.stored_bytes=uint32_t(data.size());bytes.insert(bytes.end(),data.begin(),data.end());}
};
static std::set<uint32_t> codepoints(const Bytes&bytes) {
    std::set<uint32_t> result;for(uint32_t c=32;c<127;++c)result.insert(c);result.insert(0xfffd);
    for(size_t i=0;i<bytes.size();){uint8_t c=bytes[i++];uint32_t cp=c;unsigned extra=0;if(c<128){}else if((c&224)==192){cp=c&31;extra=1;}else if((c&240)==224){cp=c&15;extra=2;}else if((c&248)==240){cp=c&7;extra=3;}else continue;
        bool ok=i+extra<=bytes.size();for(unsigned j=0;ok&&j<extra;++j){uint8_t d=bytes[i++];ok=(d&192)==128;cp=(cp<<6)|(d&63);}if(ok&&cp>=32&&cp<=0x10ffff&&cp!=0xfeff)result.insert(cp);}
    return result;
}
struct FontHeader {char magic[8];uint32_t version,count,pixel_height,line_height,records_offset,pixels_offset,file_bytes;};
struct GlyphRecord {uint32_t codepoint,offset;uint16_t width,height;int16_t bearing_x,bearing_y,advance;uint16_t reserved;};
static_assert(sizeof(FontHeader)==36&&sizeof(GlyphRecord)==20,"Font layout");
static unsigned write_font(const fs::path&font_path,const Bytes&locale,const fs::path&out) {
    Bytes data=read_file(font_path);stbtt_fontinfo info{};int offset=stbtt_GetFontOffsetForIndex(data.data(),0);
    require(offset>=0&&stbtt_InitFont(&info,data.data(),offset),"Cannot initialize original game font: "+font_path.u8string());
    auto points=codepoints(locale);
    // Native calculator UI has a few words absent from the original PC CSV.
    if(out.filename().u8string().find(".zhs.")!=std::string::npos) {
        const std::string extra=NativeUiGlyphs;
        auto added=codepoints(Bytes(extra.begin(),extra.end()));points.insert(added.begin(),added.end());
    }
    require(points.size()<=8192,"Font needs more than 8192 glyphs");
    float scale=stbtt_ScaleForPixelHeight(&info,12);std::vector<GlyphRecord> records;Bytes pixels;
    uint32_t base=uint32_t(sizeof(FontHeader)+points.size()*sizeof(GlyphRecord));
    for(uint32_t cp:points){int glyph=stbtt_FindGlyphIndex(&info,int(cp));if(!glyph)glyph=stbtt_FindGlyphIndex(&info,'?');int w,h,x,y,advance,bearing;stbtt_GetGlyphHMetrics(&info,glyph,&advance,&bearing);unsigned char*bitmap=stbtt_GetGlyphBitmap(&info,scale,scale,glyph,&w,&h,&x,&y);
        require(w>=0&&h>=0&&w<256&&h<256,"Unexpected font glyph size");GlyphRecord g{cp,base+uint32_t(pixels.size()),uint16_t(w),uint16_t(h),int16_t(x),int16_t(-y),int16_t(std::max(1,int(std::lround(advance*scale)))),0};records.push_back(g);if(w&&h){require(bitmap,"Font rasterization failed");pixels.insert(pixels.end(),bitmap,bitmap+w*h);}stbtt_FreeBitmap(bitmap,nullptr);}
    FontHeader header{};std::memcpy(header.magic,"NNOVAF1\0",8);header.version=1;header.count=uint32_t(records.size());header.pixel_height=12;header.line_height=14;header.records_offset=sizeof(header);header.pixels_offset=base;header.file_bytes=base+uint32_t(pixels.size());
    Bytes file;append(file,header);for(const auto&g:records)append(file,g);file.insert(file.end(),pixels.begin(),pixels.end());require(file.size()<=1048576,"Font exceeds 1 MiB cache");write_file(out,file);std::cout<<"  "<<out.filename().u8string()<<": "<<records.size()<<" glyphs, "<<file.size()<<" bytes\n";return unsigned(records.size());
}
static std::vector<std::vector<std::string>> parse_csv(const Bytes&b){std::vector<std::vector<std::string>>rows;std::vector<std::string>row;std::string cell;bool quoted=false;size_t p=(b.size()>=3&&b[0]==239&&b[1]==187&&b[2]==191)?3:0;
    for(;p<b.size();++p){char c=char(b[p]);if(c=='"'){if(quoted&&p+1<b.size()&&b[p+1]=='"'){cell+='"';++p;}else quoted=!quoted;}else if(c==','&&!quoted){row.push_back(cell);cell.clear();}else if((c=='\n'||c=='\r')&&!quoted){if(c=='\r'&&p+1<b.size()&&b[p+1]=='\n')++p;row.push_back(cell);cell.clear();rows.push_back(row);row.clear();}else cell+=c;}
    if(!cell.empty()||!row.empty()){row.push_back(cell);rows.push_back(row);}return rows;
}
struct TextHeader {char magic[8];uint32_t version,count,records_offset,strings_offset,file_bytes;};
struct TextRecord {uint32_t key_offset,value_offset;};
static void write_text_pack(const Bytes&csv,const fs::path&path){std::map<std::string,std::string>entries;for(const auto&row:parse_csv(csv))if(row.size()>=2&&!row[0].empty()&&row[0]!="key")entries[row[0]]=row[1];
    TextHeader h{};std::memcpy(h.magic,"NNOVAL1\0",8);h.version=1;h.count=uint32_t(entries.size());h.records_offset=sizeof(h);h.strings_offset=h.records_offset+h.count*sizeof(TextRecord);std::vector<TextRecord>rs;Bytes strings;
    for(const auto&e:entries){TextRecord r{h.strings_offset+uint32_t(strings.size()),0};strings.insert(strings.end(),e.first.begin(),e.first.end());strings.push_back(0);r.value_offset=h.strings_offset+uint32_t(strings.size());strings.insert(strings.end(),e.second.begin(),e.second.end());strings.push_back(0);rs.push_back(r);}h.file_bytes=h.strings_offset+uint32_t(strings.size());Bytes out;append(out,h);for(const auto&r:rs)append(out,r);out.insert(out.end(),strings.begin(),strings.end());write_file(path,out);}

template<class T> static T object_at(const Bytes&b,size_t p) {
    require(p<=b.size()&&sizeof(T)<=b.size()-p,"Truncated binary record");
    T result;std::memcpy(&result,b.data()+p,sizeof(T));return result;
}
static void verify_pack(const fs::path&dir) {
    Bytes index=read_file(dir/"nNova.dat.tns");auto h=object_at<pack::CommonHeader>(index,0);
    require(std::memcmp(h.magic,"NNOVAC1\0",8)==0&&h.version==pack::Version,"Invalid index magic/version");
    require(h.file_bytes==index.size()&&h.sprite_count<=pack::MaxSprites&&h.frame_count<=pack::MaxFrames&&h.pack_count<=pack::MaxPacks,"Invalid index counts/size");
    require(h.sprite_offset==sizeof(h)&&h.frame_offset==h.sprite_offset+h.sprite_count*sizeof(pack::SpriteRecord)&&h.file_bytes==h.frame_offset+h.frame_count*sizeof(pack::FrameRecord),"Invalid index layout");
    require(h.index_crc32==crc32(index.data()+sizeof(h),index.size()-sizeof(h)),"Index CRC mismatch");
    std::string previous;unsigned total_frames=0;
    for(unsigned i=0;i<h.sprite_count;++i){auto s=object_at<pack::SpriteRecord>(index,h.sprite_offset+i*sizeof(pack::SpriteRecord));require(s.name[63]==0&&s.name[0]&&(!i||previous<s.name),"Sprite names are not sorted/terminated");previous=s.name;require(s.first_frame==total_frames&&s.frame_count&&s.frame_count<=h.frame_count-total_frames,"Invalid sprite frame range");total_frames+=s.frame_count;}
    require(total_frames==h.frame_count,"Unreferenced frame records");
    std::vector<Bytes> packs;
    for(unsigned i=0;i<h.pack_count;++i){char name[64];std::snprintf(name,sizeof(name),"nNova.gfx%02u.dat.tns",i);Bytes b=read_file(dir/name);auto p=object_at<pack::DataHeader>(b,0);require(std::memcmp(p.magic,"NNOVAP1\0",8)==0&&p.version==pack::Version&&p.index==i&&p.file_bytes==b.size()&&b.size()<=PackWriter::Limit,"Invalid graphics pack header");require(p.payload_crc32==crc32(b.data()+sizeof(p),b.size()-sizeof(p)),"Graphics pack CRC mismatch");packs.push_back(std::move(b));}
    for(unsigned i=0;i<h.frame_count;++i){auto f=object_at<pack::FrameRecord>(index,h.frame_offset+i*sizeof(pack::FrameRecord));require(f.pack<packs.size()&&f.width&&f.height&&f.width<=320&&f.height<=240&&f.canvas_width&&f.canvas_height,"Invalid frame geometry");require(f.decoded_bytes==uint32_t(f.width)*f.height*3&&f.codec<=pack::Rle565A8,"Invalid frame codec/size");const Bytes&b=packs[f.pack];require(f.offset>=sizeof(pack::DataHeader)&&f.offset<=b.size()&&f.stored_bytes<=b.size()-f.offset,"Frame points outside graphics pack");Bytes payload(b.begin()+f.offset,b.begin()+f.offset+f.stored_bytes);if(f.codec==pack::Raw565A8)require(payload.size()==f.decoded_bytes,"Raw pixel length mismatch");else decode_rle(payload,f.decoded_bytes);}
    unsigned fonts=0,languages=0;
    for(const auto&entry:fs::directory_iterator(dir)){std::string n=entry.path().filename().u8string();
        if(has(n,".font.dat.tns")){Bytes b=read_file(entry.path());auto f=object_at<FontHeader>(b,0);require(std::memcmp(f.magic,"NNOVAF1\0",8)==0&&f.version==1&&f.count<=8192&&f.records_offset==sizeof(f)&&f.pixels_offset==f.records_offset+f.count*sizeof(GlyphRecord)&&f.file_bytes==b.size()&&f.file_bytes<=1048576,"Invalid font pack");uint32_t last=0;
            for(unsigned i=0;i<f.count;++i){auto g=object_at<GlyphRecord>(b,f.records_offset+i*sizeof(GlyphRecord));require((!i||g.codepoint>last)&&g.offset>=f.pixels_offset&&g.offset<=b.size()&&uint64_t(g.width)*g.height<=b.size()-g.offset,"Invalid glyph record");last=g.codepoint;}++fonts;
        }else if(has(n,".locale.dat.tns")){Bytes b=read_file(entry.path());Reader r{b};auto t=object_at<TextHeader>(b,0);require(std::memcmp(t.magic,"NNOVAL1\0",8)==0&&t.version==1&&t.count<=65536&&t.records_offset==sizeof(t)&&t.strings_offset==t.records_offset+t.count*sizeof(TextRecord)&&t.file_bytes==b.size(),"Invalid locale pack");std::string previous_key;
            for(unsigned i=0;i<t.count;++i){auto v=object_at<TextRecord>(b,t.records_offset+i*sizeof(TextRecord));require(v.key_offset>=t.strings_offset&&v.value_offset>=t.strings_offset,"Invalid locale pointer");std::string key=r.str(v.key_offset);r.str(v.value_offset);require(!i||previous_key<key,"Locale keys are not sorted");previous_key=key;}++languages;
        }
    }
    std::cout<<"Verified "<<h.sprite_count<<" sprites, "<<h.frame_count<<" frames, "<<h.pack_count<<" graphics packs, "<<fonts<<" fonts, "<<languages<<" languages.\n";
}
static void self_test() {
    const char*known="123456789";require(crc32(reinterpret_cast<const uint8_t*>(known),9)==0xcbf43926u,"CRC known-answer test failed");
    uint32_t state=0x421337u;unsigned cases=0;
    for(unsigned size=1;size<=1025;++size)for(unsigned pattern=0;pattern<4;++pattern){Bytes raw;for(unsigned i=0;i<size;++i){state^=state<<13;state^=state>>17;state^=state<<5;uint32_t pixel=pattern==0?0:pattern==1?0x7effff:pattern==2?(i/127)%3:state;raw.push_back(uint8_t(pixel));raw.push_back(uint8_t(pixel>>8));raw.push_back(uint8_t(pixel>>16));}require(decode_rle(encode_rle(raw),raw.size())==raw,"RLE round-trip failed");++cases;}
    bool rejected=false;try{decode_rle(Bytes{255,0,0},384);}catch(const std::runtime_error&){rejected=true;}require(rejected,"Truncated repeat was not rejected");
    rejected=false;try{decode_rle(Bytes{127,0,0,0},384);}catch(const std::runtime_error&){rejected=true;}require(rejected,"Truncated literal was not rejected");
    const std::string csv="key,English\r\nExample,\"line, with \"\"quotes\"\"\"\r\n";auto rows=parse_csv(Bytes(csv.begin(),csv.end()));require(rows.size()==2&&rows[1].size()==2&&rows[1][1]=="line, with \"quotes\"","CSV quoting test failed");
    std::cout<<"Self-test passed: "<<cases<<" codec patterns, malformed streams, CRC and CSV quoting.\n";
}
struct Options {fs::path source=".",output;bool verify=false,all_locales=false;unsigned compact=0;};
static Options options(int argc,char**argv){Options o;bool source=false;for(int i=1;i<argc;++i){std::string a=argv[i];if(a=="--help"||a=="-h"){std::cout<<"nNova converter - run beside your own Nova Drift data.win\n\nUsage: converter [GAME_DIRECTORY] [--output DIRECTORY] [--verify]\n                 [--all-locales] [--compact-animation MAX_FRAMES]\n\nDefault output: GAME_DIRECTORY/nNova/data\nNo source argument: find data.win in CWD, then beside this executable.\nDefault languages: English and Simplified Chinese.\nDefault conversion preserves every animation frame.\nAlso: converter --verify-pack DIRECTORY | converter --self-test\nNo Python, GameMaker runner, network, or game executable is needed.\n";std::exit(0);}else if(a=="--output"){require(++i<argc,"--output needs a directory");o.output=input_path(argv[i]);}else if(a=="--verify")o.verify=true;else if(a=="--all-locales")o.all_locales=true;else if(a=="--full-animation")o.compact=0;else if(a=="--compact-animation"){require(++i<argc,"--compact-animation needs a frame limit");o.compact=unsigned(std::stoul(argv[i]));require(o.compact>=2&&o.compact<=4096,"Frame limit must be 2..4096");}else if(!a.empty()&&a[0]=='-')throw std::runtime_error("Unknown option: "+a);else{require(!source,"Only one game directory is accepted");o.source=input_path(argv[i]);source=true;}}if(!source&&!fs::is_regular_file(o.source/"data.win")){fs::path beside=executable_path(argv[0]).parent_path();if(fs::is_regular_file(beside/"data.win"))o.source=beside;}if(o.output.empty())o.output=o.source/"nNova"/"data";return o;}

static int convert_main(int argc,char**argv)try {
    if(argc==2&&std::string(argv[1])=="--self-test"){self_test();return 0;}
    if(argc==3&&std::string(argv[1])=="--verify-pack"){verify_pack(input_path(argv[2]));return 0;}
    const Options opt=options(argc,argv);const uint16_t endian=1;require(*reinterpret_cast<const uint8_t*>(&endian)==1,"Converter needs a little-endian computer");
    std::cout<<"nNova resource converter (C++17)\nReading "<<(opt.source/"data.win").u8string()<<"\n";
    GameData game(opt.source/"data.win");fs::create_directories(opt.output);
    require(game.sprites.size()<=pack::MaxSprites,"Too many sprites for this launcher version");
    struct Job{size_t source,output;unsigned frame;PageItem page;Sizing scale;};
    std::vector<std::vector<Job>> jobs(game.textures.size());std::vector<pack::SpriteRecord> sprites;std::vector<pack::FrameRecord> frames;
    std::ofstream inventory(opt.output/"sprites.csv");inventory<<"id,name,original_width,original_height,frames,fps,category\n";
    for(size_t i=0;i<game.sprites.size();++i){const auto&s=game.sprites[i];Sizing size=sizing(s);pack::SpriteRecord record{};record.first_frame=uint32_t(frames.size());record.original_width=uint16_t(s.width);record.original_height=uint16_t(s.height);record.fps_q8=uint16_t(std::clamp(int(std::lround(s.fps*256)),0,65535));std::memcpy(record.name,s.name.c_str(),s.name.size()+1);
        unsigned count=unsigned(s.frames.size());if(opt.compact)count=std::min(count,opt.compact);record.frame_count=uint16_t(count);
        for(unsigned f=0;f<count;++f){unsigned source_frame=count==s.frames.size()?f:unsigned(uint64_t(f)*(s.frames.size()-1)/(count-1));PageItem p=game.page(s.frames[source_frame]);jobs[size_t(p.texture)].push_back({i,frames.size(),source_frame,p,size});frames.emplace_back();}
        inventory<<i<<','<<s.name<<','<<s.width<<','<<s.height<<','<<count<<','<<s.fps<<','<<size.category<<'\n';sprites.push_back(record);}
    inventory.close();require(frames.size()<=pack::MaxFrames,"Too many animation frames for this launcher version");
    std::cout<<"Converting "<<sprites.size()<<" sprites, "<<frames.size()<<" frames from "<<game.textures.size()<<" texture pages.\n";
    PackWriter writer(opt.output);uint64_t raw_bytes=0;unsigned duplicate_frames=0,converted=0;
    struct Duplicate{Bytes bytes;pack::FrameRecord record;};std::unordered_map<uint64_t,std::vector<Duplicate>> duplicates;
    for(size_t t=0;t<jobs.size();++t){if(jobs[t].empty())continue;auto pixels=decode_dxt5(game,game.textures[t]);
        for(const auto&j:jobs[t]){const auto&s=game.sprites[j.source];Resized image=resize(pixels,game.textures[t],j.page,s,j.scale);Bytes encoded=encode_rle(image.pixels);uint16_t codec=pack::Rle565A8;if(encoded.size()>=image.pixels.size()){encoded=image.pixels;codec=pack::Raw565A8;}
            if(opt.verify&&codec==pack::Rle565A8)require(decode_rle(encoded,image.pixels.size())==image.pixels,"Lossless codec verification failed");
            pack::FrameRecord r{};r.decoded_bytes=uint32_t(image.pixels.size());r.width=image.width;r.height=image.height;r.origin_x=image.ox;r.origin_y=image.oy;r.codec=codec;r.canvas_width=uint16_t(std::max(1,int(std::lround(s.width*j.scale.x))));r.canvas_height=uint16_t(std::max(1,int(std::lround(s.height*j.scale.y))));r.original_frame=uint16_t(j.frame);raw_bytes+=image.pixels.size();
            uint64_t key=hash64(encoded)^uint64_t(r.width)<<48^uint64_t(r.height)<<32^uint64_t(codec)<<60;auto&bucket=duplicates[key];bool found=false;
            for(const auto&d:bucket)if(d.bytes==encoded&&d.record.width==r.width&&d.record.height==r.height&&d.record.codec==codec){r.pack=d.record.pack;r.offset=d.record.offset;r.stored_bytes=d.record.stored_bytes;found=true;++duplicate_frames;break;}
            if(!found){writer.add(encoded,r);bucket.push_back({std::move(encoded),r});}frames[j.output]=r;++converted;
        }
        if(t%16==0||converted==frames.size())std::cout<<"  texture "<<t+1<<'/'<<game.textures.size()<<": "<<converted<<'/'<<frames.size()<<" frames\n";
    }
    writer.flush();pack::CommonHeader header{};std::memcpy(header.magic,"NNOVAC1\0",8);header.version=pack::Version;header.sprite_count=uint32_t(sprites.size());header.frame_count=uint32_t(frames.size());header.pack_count=writer.pack_count;header.sprite_offset=sizeof(header);header.frame_offset=header.sprite_offset+header.sprite_count*sizeof(pack::SpriteRecord);header.file_bytes=header.frame_offset+header.frame_count*sizeof(pack::FrameRecord);header.source_bytes=uint32_t(game.bytes.size());header.source_crc32=crc32(game.bytes.data(),game.bytes.size());header.screen_width=320;header.screen_height=240;
    Bytes index(sizeof(header),0);for(const auto&s:sprites)append(index,s);for(const auto&f:frames)append(index,f);header.index_crc32=crc32(index.data()+sizeof(header),index.size()-sizeof(header));std::memcpy(index.data(),&header,sizeof(header));write_file(opt.output/"nNova.dat.tns",index);
    std::cout<<"Converting original font and localization files.\n";Bytes english=read_file(opt.source/"localization.csv"),chinese=read_file(opt.source/"chineseSimplified.csv");
    unsigned eng_glyphs=write_font(opt.source/"RobotoCondensed-Regular.ttf",english,opt.output/"nNova.eng.font.dat.tns");
    unsigned zhs_glyphs=write_font(opt.source/"NotoSansSC-Medium.ttf",chinese,opt.output/"nNova.zhs.font.dat.tns");
    const std::pair<const char*,const char*>languages[]={{"localization.csv","eng"},{"chineseSimplified.csv","zhs"},{"chineseTraditional.csv","zht"},{"japanese.csv","jpn"},{"korean.csv","kor"},{"german.csv","deu"},{"french.csv","fra"},{"spanish.csv","spa"},{"russian.csv","rus"},{"polish.csv","pol"},{"brazilianPortuguese.csv","por"}};
    for(const auto&l:languages) {
        fs::path destination=opt.output/(std::string("nNova.")+l.second+".locale.dat.tns");
        bool selected=opt.all_locales||std::string(l.second)=="eng"||std::string(l.second)=="zhs";
        if(selected&&fs::exists(opt.source/l.first))write_text_pack(read_file(opt.source/l.first),destination);
        else if(!selected)fs::remove(destination); // remove only our known optional outputs
    }
    for(const char*license:{"Roboto_LICENSE.txt","NotoSansSC-Medium_OFL.txt"})if(fs::exists(opt.source/license))fs::copy_file(opt.source/license,opt.output/license,fs::copy_options::overwrite_existing);
    std::ofstream report(opt.output/"conversion-report.txt");report<<"nNova conversion report\nSource: owner-supplied GameMaker data.win\nSource bytes: "<<header.source_bytes<<"\nSource CRC32: "<<std::hex<<std::setw(8)<<std::setfill('0')<<header.source_crc32<<std::dec<<"\nSprites: "<<sprites.size()<<"\nFrames: "<<frames.size()<<"\nUnique stored frames: "<<frames.size()-duplicate_frames<<"\nGraphics packs: "<<writer.pack_count<<"\nIndex bytes: "<<index.size()<<"\nGraphics bytes: "<<writer.total<<"\nUncompressed resized pixels: "<<raw_bytes<<"\nEnglish glyphs: "<<eng_glyphs<<"\nSimplified Chinese glyphs: "<<zhs_glyphs<<"\nTarget: 320 x 240, straight RGB565 plus full 8-bit alpha\nResampling: area filter with premultiplied alpha; only fully transparent borders trimmed\nStorage: lossless adaptive raw/RLE, identical frames shared\nAnimation: "<<(opt.compact?"explicit compact sampling":"all source frames preserved")<<"\nRuntime: decompression only on cache miss, bounded 2 MiB pixel cache\nMusic/audio: intentionally not converted; calculator build has no audio output\nLocalization: "<<(opt.all_locales?"all available CSV languages exported":"English/Simplified Chinese only")<<"; English/Simplified Chinese glyph packs baked\n";
    require(bool(report),"Cannot write conversion report");
    if(opt.verify)verify_pack(opt.output);
    std::cout<<"Done: "<<opt.output.u8string()<<"\nGraphics + index: "<<writer.total+index.size()<<" bytes; "<<duplicate_frames<<" identical frames shared.\nCopy all *.dat.tns files into nNova/data beside nNova.tns.\n";return 0;
}catch(const std::exception&e){std::cerr<<"Conversion failed: "<<e.what()<<"\nNo game executable was run.\n";return 1;}


int main(int argc,char**argv) {
#ifdef _WIN32
    (void)argc; (void)argv;
    // The CRT's narrow argv can already contain '?' for characters outside
    // the machine's active code page. Read the unchanged wide command line.
    // Dynamic lookup keeps the executable's existing build/link commands and
    // loads only the Windows system DLL, never a DLL beside the game files.
    struct WideArguments {
        HMODULE shell=nullptr;
        LPWSTR* values=nullptr;
        ~WideArguments() {
            if(values)LocalFree(values);
            if(shell)FreeLibrary(shell);
        }
    } wide;
    try {
        wide.shell=LoadLibraryExW(L"shell32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        require(wide.shell!=nullptr,"Cannot load the Windows command-line parser");
        using ParseArguments=LPWSTR* (WINAPI*)(LPCWSTR,int*);
        auto parse=reinterpret_cast<ParseArguments>(GetProcAddress(wide.shell,"CommandLineToArgvW"));
        require(parse!=nullptr,"Cannot locate the Windows command-line parser");
        int count=0;
        wide.values=parse(GetCommandLineW(),&count);
        require(wide.values!=nullptr&&count>0,"Cannot parse the Windows command line");
        std::vector<std::string> arguments;
        arguments.reserve(size_t(count));
        for(int i=0;i<count;++i) {
            int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.values[i],-1,nullptr,0,nullptr,nullptr);
            require(bytes>0,"Invalid Unicode command-line argument");
            std::string text(size_t(bytes),'\0');
            require(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.values[i],-1,text.data(),bytes,nullptr,nullptr)>0,
                    "Cannot encode the Unicode command line");
            text.pop_back();arguments.push_back(std::move(text));
        }
        std::vector<char*> pointers;
        pointers.reserve(size_t(count)+1);
        for(auto&argument:arguments)pointers.push_back(argument.data());
        pointers.push_back(nullptr);
        SetConsoleOutputCP(CP_UTF8);
        return convert_main(count,pointers.data());
    }catch(const std::exception&e) {
        std::cerr<<"Converter startup failed: "<<e.what()<<"\n";
        return 1;
    }
#else
    return convert_main(argc,argv);
#endif
}
