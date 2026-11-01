#pragma once
#include <stdint.h>

// All integers are little endian. No game pixels or text are embedded here.
// Format is deliberately simple enough for the freestanding ARM926 loader.
namespace nova { namespace pack {
static constexpr uint32_t Version = 3;
inline bool supported_version(uint32_t value) { return value >= 1 && value <= Version; }
static constexpr uint32_t MaxSprites = 1600;
static constexpr uint32_t MaxFrames = 4096;
static constexpr uint32_t CacheBytes = 2u * 1024u * 1024u;
static constexpr uint32_t MaxPacks = 32;
enum Codec : uint16_t {
    Raw565A8 = 0, Rle565A8 = 1,
    Palette4_565A8 = 2, Palette8_565A8 = 3,
    Palette4_565_Alpha8 = 4, Palette8_565_Alpha8 = 5,
    PlanarRle565A8 = 6,
};
inline bool supported_codec(uint16_t value) { return value <= PlanarRle565A8; }
inline bool supported_codec(uint16_t value,uint32_t version) {
    return supported_version(version) && value <= (version == 1 ? Rle565A8 : version == 2 ? Palette8_565_Alpha8 : PlanarRle565A8);
}

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
// Version 2 also permits exact per-frame palettes (no color quantization):
// [uint16 entry count][palette][indices][optional alpha plane]. RGBA palettes
// contain three-byte pixels; RGB palettes contain uint16 colors followed by an
// unchanged byte-per-pixel alpha plane. Four-bit indices use the low nibble
// first; an unused final high nibble must be zero. Entry count is 1..16/256.
// Version 3 adds independent RLE planes: a complete RGB565 stream followed
// by a complete alpha8 stream. Each plane uses the same count/run convention
// as pixel RLE, with two-byte color units and one-byte alpha units. The known
// pixel count delimits both streams; there are no extra lengths or buffers.
// New launchers also accept version-1 and version-2 packs.
}}
