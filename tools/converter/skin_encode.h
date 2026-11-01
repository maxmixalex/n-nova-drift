#pragma once
// Include after resize/encode_rle and the converter's checked file helpers.
#include "nnova/skin_format.h"
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "third_party/stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
namespace skin_converter {
static std::vector<std::string> row(const std::string& line){
    std::vector<std::string> fields;std::string field;bool quote=false;
    for(size_t i=0;i<line.size();++i){char c=line[i];
        if(c=='"'){if(quote&&i+1<line.size()&&line[i+1]=='"'){field+='"';++i;}else quote=!quote;}
        else if(c==','&&!quote){fields.push_back(field);field.clear();}else if(c!='\r')field+=c;
    }
    require(!quote,"Unclosed quote in custom_skins.csv");fields.push_back(field);return fields;
}
struct Memory {
    const Bytes& bytes;size_t at,end;
    bool byte(uint8_t& b){if(at>=end)return false;b=bytes[at++];return true;}
    bool pixel(uint16_t& c,uint8_t& a){uint8_t lo,hi;if(!byte(lo)||!byte(hi)||!byte(a))return false;c=uint16_t(lo|unsigned(hi)<<8);return true;}
    bool word(uint16_t& value){if(end-at<2)return false;value=uint16_t(unsigned(bytes[at])|(unsigned(bytes[at+1])<<8));at+=2;return true;}
    bool read(uint8_t* target,unsigned count){if(count>end-at)return false;codec::copy_bytes(target,bytes.data()+at,count);at+=count;return true;}
    bool finished()const{return at==end;}
};
static bool verify(const fs::path& directory){
    fs::path path=directory/"nNova.skins.dat.tns";if(!fs::exists(path))return false;
    Bytes data=read_file(path);skinpack::Header h{};
    require(data.size()>=sizeof(h),"Truncated skin pack");std::memcpy(&h,data.data(),sizeof(h));
    require(!std::memcmp(h.magic,"NNOVAK1\0",8)&&h.version==skinpack::Version&&h.skin_count>0&&h.skin_count<=skinpack::MaxSkins&&h.image_count==h.skin_count*3&&h.file_bytes==data.size()&&h.payload_crc32==crc32(data.data()+sizeof(h),data.size()-sizeof(h)),"Invalid skin pack header or CRC");
    size_t images_at=sizeof(h)+h.skin_count*sizeof(skinpack::Skin),payload_at=images_at+h.image_count*sizeof(skinpack::Image);
    require(payload_at<=data.size(),"Truncated skin metadata");
    for(unsigned i=0;i<h.skin_count;++i){skinpack::Skin s{};std::memcpy(&s,data.data()+sizeof(h)+i*sizeof(s),sizeof(s));require(s.first_image==i*3&&s.name[0]&&std::memchr(s.name,0,sizeof(s.name)),"Invalid skin name or image reference");}
    for(unsigned i=0;i<h.image_count;++i){skinpack::Image im{};std::memcpy(&im,data.data()+images_at+i*sizeof(im),sizeof(im));
        if(!im.stored_bytes){require(i%3==2&&!im.width&&!im.height&&!im.decoded_bytes,"Missing required skin image");continue;}
        require(im.width&&im.height&&im.width<=skinpack::MaxDimension&&im.height<=skinpack::MaxDimension&&im.canvas_width&&im.canvas_height&&im.canvas_width<=skinpack::MaxDimension&&im.canvas_height<=skinpack::MaxDimension&&im.decoded_bytes==unsigned(im.width)*im.height*3&&im.offset>=payload_at&&im.offset<=data.size()&&im.stored_bytes<=data.size()-im.offset,"Invalid skin image boundary");
        unsigned count=unsigned(im.width)*im.height;std::vector<uint16_t> colors(count);Bytes alpha(count);Memory input{data,im.offset,size_t(im.offset)+im.stored_bytes};require(codec::decode(input,im.codec,colors.data(),alpha.data(),count),"Invalid skin image codec");
        Bytes pixels;pixels.reserve(count*3);for(unsigned p=0;p<count;++p){pixels.push_back(uint8_t(colors[p]));pixels.push_back(uint8_t(colors[p]>>8));pixels.push_back(alpha[p]);}
        require(crc32(pixels.data(),pixels.size())==im.decoded_crc32,"Skin image changed during compression");
    }
    return true;
}
}
static bool verify_skins(const fs::path& directory){return skin_converter::verify(directory);}
static void write_skins(const fs::path& source,const fs::path& output){
    const fs::path directory=source/"custom_skins",csv=directory/"custom_skins.csv",destination=output/"nNova.skins.dat.tns";
    if(!fs::exists(csv)){if(fs::exists(destination))fs::remove(destination);return;}
    const Bytes csv_bytes=read_file(csv);std::istringstream lines(std::string(csv_bytes.begin(),csv_bytes.end()));std::string line;std::getline(lines,line);
    std::vector<skinpack::Skin> skins;std::vector<skinpack::Image> images;std::vector<Bytes> payloads;
    while(std::getline(lines,line)){
        if(line.empty()||line=="\r")continue;
        auto fields=skin_converter::row(line);
        require(fields.size()==6&&skins.size()<skinpack::MaxSkins&&!fields[0].empty()&&fields[0].size()<48,"Invalid skin CSV row or skin count exceeds 16");
        skinpack::Skin skin{};std::memcpy(skin.name,fields[0].data(),fields[0].size());skin.first_image=uint32_t(images.size());
        auto integer=[](const std::string& value){if(value.empty())return 0;size_t used=0;int result=std::stoi(value,&used);require(used==value.size()&&result>=-32768&&result<=32767,"Invalid skin jet coordinate");return result;};
        skin.jet_angle=int16_t(integer(fields[4]));skin.jet_distance=int16_t(integer(fields[5]));
        for(unsigned kind=0;kind<3;++kind){skinpack::Image im{};Bytes encoded;
            if(fields[kind+1].empty()){require(kind==2,"Body and Force Armor artwork are required");images.push_back(im);payloads.push_back(encoded);continue;}
            const fs::path relative=fs::u8path(fields[kind+1]);require(!relative.is_absolute()&&relative.filename()==relative,"Skin artwork must be a PNG filename in custom_skins");
            Bytes png=read_file(directory/relative);require(png.size()<32u*1024*1024,"Skin PNG exceeds 32 MiB");int w=0,h=0,channels=0;
            require(stbi_info_from_memory(png.data(),int(png.size()),&w,&h,&channels)&&w>0&&h>0&&w<=2048&&h<=2048,"Invalid or oversized skin PNG");
            unsigned char* decoded=stbi_load_from_memory(png.data(),int(png.size()),&w,&h,&channels,4);require(decoded!=nullptr,"Cannot decode skin PNG: "+relative.u8string());
            std::vector<Pixel> texture(size_t(w)*h);std::memcpy(texture.data(),decoded,texture.size()*4);stbi_image_free(decoded);
            Texture t{0,0,unsigned(w),unsigned(h)};PageItem item{0,0,uint16_t(w),uint16_t(h),0,0,uint16_t(w),uint16_t(h),uint16_t(w),uint16_t(h),0};
            SourceSprite sprite{fields[0],unsigned(w),unsigned(h),w/2,h/2,0,{}};double scale=std::min({1.0,double(skinpack::MaxDimension)/w,double(skinpack::MaxDimension)/h});
            Resized resized=resize(texture,t,item,sprite,Sizing{scale,scale,"custom skin"});
            im.width=resized.width;im.height=resized.height;im.origin_x=resized.ox;im.origin_y=resized.oy;im.canvas_width=uint16_t(std::max(1,int(std::lround(w*scale))));im.canvas_height=uint16_t(std::max(1,int(std::lround(h*scale))));im.source_extent=uint16_t(std::max(w,h));im.decoded_bytes=uint32_t(resized.pixels.size());im.decoded_crc32=crc32(resized.pixels.data(),resized.pixels.size());
            encoded=encode_rle(resized.pixels);im.codec=pack::Rle565A8;if(encoded.size()>=resized.pixels.size()){encoded=resized.pixels;im.codec=pack::Raw565A8;}codec::select_image_encoding(resized.pixels,encoded,im.codec);
            im.stored_bytes=uint32_t(encoded.size());images.push_back(im);payloads.push_back(std::move(encoded));
        }
        skins.push_back(skin);
    }
    require(!skins.empty(),"No skins in custom_skins.csv");skinpack::Header header{};std::memcpy(header.magic,"NNOVAK1\0",8);header.version=skinpack::Version;header.skin_count=uint32_t(skins.size());header.image_count=uint32_t(images.size());
    Bytes result(sizeof(header)+skins.size()*sizeof(skinpack::Skin)+images.size()*sizeof(skinpack::Image),0);
    for(unsigned i=0;i<images.size();++i){if(!payloads[i].empty())images[i].offset=uint32_t(result.size());result.insert(result.end(),payloads[i].begin(),payloads[i].end());}
    std::memcpy(result.data()+sizeof(header),skins.data(),skins.size()*sizeof(skinpack::Skin));std::memcpy(result.data()+sizeof(header)+skins.size()*sizeof(skinpack::Skin),images.data(),images.size()*sizeof(skinpack::Image));
    header.file_bytes=uint32_t(result.size());header.payload_crc32=crc32(result.data()+sizeof(header),result.size()-sizeof(header));std::memcpy(result.data(),&header,sizeof(header));write_file(destination,result);require(verify_skins(output),"Skin pack verification failed");
    std::cout<<"Custom skins: "<<skins.size()<<" bodies, "<<images.size()<<" images, "<<result.size()<<" bytes\n";
}
