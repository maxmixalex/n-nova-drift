#pragma once
#include <stdint.h>
namespace nova { namespace skinpack {
enum { Version=1, MaxSkins=16, ImagesPerSkin=3, MaxDimension=72, MaxPixels=72*72 };
struct Header { char magic[8];uint32_t version,skin_count,image_count,file_bytes,payload_crc32,reserved; };
struct Skin { char name[48];uint32_t first_image;int16_t jet_angle,jet_distance;uint32_t reserved[2]; };
struct Image { uint32_t offset,stored_bytes,decoded_bytes;uint16_t width,height;int16_t origin_x,origin_y;uint16_t canvas_width,canvas_height,codec,source_extent;uint32_t decoded_crc32; };
static_assert(sizeof(Header)==32&&sizeof(Skin)==64&&sizeof(Image)==32,"skin pack ABI");
} }
