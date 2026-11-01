#pragma once
#include <stdint.h>

// All integers are little endian. No game pixels or text are embedded here.
// Format is deliberately simple enough for the freestanding ARM926 loader.
namespace nova { namespace pack {
static constexpr uint32_t Version = 1;
static constexpr uint32_t MaxSprites = 1600;
static constexpr uint32_t MaxFrames = 4096;
static constexpr uint32_t CacheBytes = 2u * 1024u * 1024u;
static constexpr uint32_t MaxPacks = 32;
enum Codec : uint16_t { Raw565A8 = 0, Rle565A8 = 1 };

struct CommonHeader {
    char magic[8];                 // NNOVAC1\0
    uint32_t version;
    uint32_t sprite_count;
    uint32_t frame_count;
    uint32_t pack_count;
    uint32_t sprite_offset;
    uint32_t frame_offset;
    uint32_t file_bytes;
    uint32_t source_bytes;
    uint32_t source_crc32;
    uint32_t index_crc32;           // bytes after CommonHeader
    uint16_t screen_width, screen_height;
    uint32_t reserved[3];
};
struct SpriteRecord {
    uint32_t first_frame;
    uint16_t frame_count;
    uint16_t original_width, original_height;
    uint16_t fps_q8;               // frames/sec * 256; zero for static sprites
    char name[64];                 // sorted bytewise, zero terminated
};
struct FrameRecord {
    uint32_t offset;               // absolute byte offset in nNova.gfxNN.dat.tns
    uint32_t stored_bytes;
    uint32_t decoded_bytes;        // width * height * 3
    uint16_t pack;
    uint16_t width, height;
    int16_t origin_x, origin_y;     // position relative to cropped bitmap
    uint16_t codec;
    uint16_t canvas_width, canvas_height; // scaled full sprite before border trim
    uint16_t original_frame;
    uint16_t reserved;
};
struct DataHeader {
    char magic[8];                 // NNOVAP1\0
    uint32_t version, index, file_bytes, payload_crc32;
    uint32_t reserved[2];
};
static_assert(sizeof(CommonHeader) == 64, "common format packing");
static_assert(sizeof(SpriteRecord) == 76, "sprite format packing");
static_assert(sizeof(FrameRecord) == 32, "frame format packing");
static_assert(sizeof(DataHeader) == 32, "data format packing");

// Each raw pixel is [RGB565 little endian, alpha byte]. RLE packets begin
// with a count byte: 0..127 = count+1 literal pixels; 128..255 = count-127
// copies of the following single pixel. Decoding occurs on first cache use,
// never on every draw. Runtime expands into separate RGB565 / alpha planes.
}}
