#include "assets.h"
#include "platform.h"
#include "nnova/image_codec.h"

namespace nova {
namespace {
int compare(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return (unsigned char)*a - (unsigned char)*b;
}
bool magic(const char* a, const char* b) {
    for (unsigned i = 0; i != 8; ++i) if (a[i] != b[i]) return false;
    return true;
}
uint32_t crc_update(uint32_t c, const void* data, unsigned count) {
    static const uint32_t table[16] = {
        0x00000000,0x1db71064,0x3b6e20c8,0x26d930ac,
        0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,
        0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,
        0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c
    };
    const uint8_t* bytes = (const uint8_t*)data;
    while (count--) {
        c ^= *bytes++;
        c = (c >> 4) ^ table[c & 15];
        c = (c >> 4) ^ table[c & 15];
    }
    return c;
}
void graphics_name(char* filename, unsigned pack) {
    filename[9] = char('0' + pack / 10);
    filename[10] = char('0' + pack % 10);
}
bool read_all(platform::File* f, void* p, unsigned n) {
    uint8_t* out = (uint8_t*)p;
    while (n) {
        unsigned got = (unsigned)platform::read(f, out, n);
        if (!got || got > n) return false;
        out += got; n -= got;
    }
    return true;
}
struct Stream {
    platform::File* f;
    unsigned left, pos, fill;
    uint8_t bytes[768];
    bool byte(uint8_t& b) {
        if (pos == fill) {
            if (!left) return false;
            fill = left < sizeof(bytes) ? left : sizeof(bytes);
            if (!read_all(f, bytes, fill)) return false;
            left -= fill; pos = 0;
        }
        b = bytes[pos++]; return true;
    }
    bool pixel(uint16_t& c, uint8_t& a) {
        uint8_t l, h;
        if (!byte(l) || !byte(h) || !byte(a)) return false;
        c = uint16_t(l | (uint16_t(h) << 8)); return true;
    }
    bool word(uint16_t& value) {
        if(fill-pos>=2){value=uint16_t(unsigned(bytes[pos])|(unsigned(bytes[pos+1])<<8));pos+=2;return true;}
        uint8_t lo,hi;if(!byte(lo)||!byte(hi))return false;
        value=uint16_t(unsigned(lo)|(unsigned(hi)<<8));return true;
    }
    bool read(uint8_t* target,unsigned count) {
        while(count) {
            if(pos==fill){if(!left)return false;fill=left<sizeof(bytes)?left:sizeof(bytes);if(!read_all(f,bytes,fill))return false;left-=fill;pos=0;}
            const unsigned available=fill-pos,n=count<available?count:available;
            codec::copy_bytes(target,bytes+pos,n);target+=n;pos+=n;count-=n;
        }
        return true;
    }
    bool finished() const { return left == 0 && pos == fill; }
};
}

bool Assets::open(const char* base_path) {
    (void)base_path; // platform::open_data has already selected the game folder.
    close();
    platform::File* f = platform::open_data("nNova.dat.tns");
    if (!f) { error_ = "Missing data/nNova.dat.tns"; return false; }
    bool ok = read_all(f, &header_, sizeof(header_));
    ok = ok && magic(header_.magic, "NNOVAC1\0") && pack::supported_version(header_.version)
        && header_.sprite_count > 0 && header_.sprite_count <= pack::MaxSprites
        && header_.frame_count > 0 && header_.frame_count <= pack::MaxFrames
        && header_.pack_count > 0 && header_.pack_count <= pack::MaxPacks
        && header_.screen_width == 320 && header_.screen_height == 240
        && header_.file_bytes == platform::length(f)
        && header_.sprite_offset == sizeof(header_)
        && header_.frame_offset == header_.sprite_offset + header_.sprite_count * sizeof(pack::SpriteRecord)
        && header_.file_bytes == header_.frame_offset + header_.frame_count * sizeof(pack::FrameRecord);
    if (ok) ok = read_all(f, sprites_, header_.sprite_count * sizeof(pack::SpriteRecord))
              && read_all(f, frames_, header_.frame_count * sizeof(pack::FrameRecord));
    platform::close(f);
    if (ok) {
        uint32_t crc = crc_update(~0u, sprites_, header_.sprite_count * sizeof(pack::SpriteRecord));
        crc = crc_update(crc, frames_, header_.frame_count * sizeof(pack::FrameRecord));
        ok = ~crc == header_.index_crc32;
    }
    for (unsigned i = 0; ok && i < header_.sprite_count; ++i) {
        const pack::SpriteRecord& s = sprites_[i];
        ok = s.name[0] && s.name[63] == 0 && s.frame_count > 0 && s.first_frame <= header_.frame_count
             && s.frame_count <= header_.frame_count - s.first_frame
             && (i == 0 || compare(sprites_[i-1].name, s.name) < 0);
    }
    for (unsigned i = 0; ok && i < header_.frame_count; ++i) {
        const pack::FrameRecord& r = frames_[i];
        ok = r.pack < header_.pack_count && r.width > 0 && r.height > 0
             && r.width <= 320 && r.height <= 240
             && r.canvas_width > 0 && r.canvas_width <= 320
             && r.canvas_height > 0 && r.canvas_height <= 240
             && r.decoded_bytes == uint32_t(r.width) * r.height * 3
             && r.decoded_bytes <= pack::CacheBytes && r.offset >= sizeof(pack::DataHeader)
             && r.stored_bytes > 0 && r.stored_bytes <= r.decoded_bytes * 2
             && pack::supported_codec(r.codec,header_.version);
    }
    if (!ok) { close(); error_ = "Invalid or incompatible nNova.dat.tns"; return false; }
    uint32_t pack_lengths[pack::MaxPacks];
    for (unsigned i = 0; i < header_.pack_count; ++i) {
        char filename[] = "nNova.gfx00.dat.tns"; graphics_name(filename, i);
        platform::File* file = platform::open_data(filename);
        if (!file) { close(); error_ = "Missing graphics data pack"; return false; }
        pack::DataHeader data_header;
        const uint32_t length = platform::length(file);
        ok = read_all(file, &data_header, sizeof(data_header))
             && magic(data_header.magic, "NNOVAP1\0")
             && data_header.version == header_.version && data_header.index == i
             && data_header.file_bytes == length && length >= sizeof(data_header);
        platform::close(file);
        if (!ok) { close(); error_ = "Invalid graphics data pack"; return false; }
        pack_lengths[i] = length;
    }
    for (unsigned i = 0; i < header_.frame_count; ++i) {
        const pack::FrameRecord& r = frames_[i];
        if (r.offset > pack_lengths[r.pack] || r.stored_bytes > pack_lengths[r.pack] - r.offset) {
            close(); error_ = "Truncated graphics data pack"; return false;
        }
    }
    ready_ = true; error_ = ""; return true;
}

void Assets::close() {
    ready_ = false; header_.sprite_count = header_.frame_count = header_.pack_count = 0;
    clear_cache(); error_ = "Asset pack not open";
}
void Assets::clear_cache() {
    used_ = 0;
    for (unsigned i = 0; i < pack::MaxFrames; ++i) cached_[i].pixels = 0;
}
unsigned Assets::frames(unsigned sprite) const {
    return ready_ && sprite < header_.sprite_count ? sprites_[sprite].frame_count : 0;
}
unsigned Assets::playback_fps_q8(unsigned sprite) const {
    return ready_ && sprite < header_.sprite_count ? sprites_[sprite].fps_q8 : 0;
}
const char* Assets::name(unsigned sprite) const {
    return ready_ && sprite < header_.sprite_count ? sprites_[sprite].name : "";
}
int Assets::index_of(const char* name) const {
    if (!ready_ || !name) return -1;
    unsigned lo = 0, hi = header_.sprite_count;
    while (lo < hi) {
        unsigned m = lo + ((hi - lo) >> 1);
        int c = compare(name, sprites_[m].name);
        if (c == 0) return int(m);
        if (c < 0) hi = m; else lo = m + 1;
    }
    return -1;
}
const Sprite* Assets::find(const char* name, unsigned frame) {
    int id = index_of(name); return id < 0 ? 0 : find_id(unsigned(id), frame);
}
const Sprite* Assets::find_id(unsigned sprite, unsigned frame) {
    if (!ready_ || sprite >= header_.sprite_count || !sprites_[sprite].frame_count) return 0;
    const pack::SpriteRecord& s = sprites_[sprite];
    const unsigned id = s.first_frame + frame % s.frame_count;
    Sprite& result = cached_[id];
    if (result.pixels) return &result;
    const pack::FrameRecord& r = frames_[id];
    unsigned at = (used_ + 3) & ~3u;
    if (at + r.decoded_bytes > sizeof(arena_)) { clear_cache(); at = 0; }
    char filename[] = "nNova.gfx00.dat.tns";
    graphics_name(filename, r.pack);
    platform::File* f = platform::open_data(filename);
    if (!f) { ready_ = false; error_ = "Missing graphics data pack"; return 0; }
    pack::DataHeader dh;
    bool ok = read_all(f, &dh, sizeof(dh)) && magic(dh.magic, "NNOVAP1\0")
        && dh.version == header_.version && dh.index == r.pack && dh.file_bytes == platform::length(f)
        && r.offset <= dh.file_bytes && r.stored_bytes <= dh.file_bytes - r.offset
        && platform::seek(f, r.offset);
    const unsigned count = unsigned(r.width) * r.height;
    uint16_t* colors = (uint16_t*)(arena_ + at);
    uint8_t* alpha = arena_ + at + count * 2;
    Stream stream = {f, r.stored_bytes, 0, 0, {0}};
    ok = ok && codec::decode(stream,r.codec,colors,alpha,count);
    platform::close(f);
    if (!ok) { ready_ = false; error_ = "Corrupt graphics data pack"; return 0; }
    result.pixels = colors; result.alpha = alpha; result.width = r.width; result.height = r.height;
    result.origin_x = r.origin_x; result.origin_y = r.origin_y;
    result.original_width = s.original_width; result.original_height = s.original_height;
    result.canvas_width = r.canvas_width; result.canvas_height = r.canvas_height;
    used_ = at + r.decoded_bytes; error_ = "";
    return &result;
}
}
