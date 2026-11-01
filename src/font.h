#pragma once
#include <stdint.h>

namespace nova {
struct FontHeader {
    char magic[8];
    uint32_t version, count, pixel_height, line_height;
    uint32_t records_offset, pixels_offset, file_bytes;
};
struct GlyphRecord {
    uint32_t codepoint, offset;
    uint16_t width, height;
    int16_t bearing_x, bearing_y, advance;
    uint16_t reserved;
};
static_assert(sizeof(FontHeader)==36,"font header layout");
static_assert(sizeof(GlyphRecord)==20,"glyph record layout");

class Font {
public:
    bool open(unsigned language);
    void close() { valid_=false; }
    bool ready() const { return valid_; }
    int line_height() const;
    int pixel_height() const;
    const GlyphRecord* glyph(uint32_t codepoint) const;
    bool contains(uint32_t codepoint) const;
    const uint8_t* pixels(const GlyphRecord& glyph) const { return data_+glyph.offset; }
private:
    alignas(4) uint8_t data_[1024*1024];
    bool valid_;
};

uint32_t next_utf8(const char*& text);
}
