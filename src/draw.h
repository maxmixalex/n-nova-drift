#pragma once
#include <stdint.h>

namespace nova {
class Font;

constexpr int ScreenW = 320;
constexpr int ScreenH = 240;

constexpr uint16_t rgb(unsigned r, unsigned g, unsigned b) {
    return uint16_t(((r & 248u) << 8) | ((g & 252u) << 3) | (b >> 3));
}

class Canvas {
public:
    uint16_t* pixels;
    void bind(uint16_t* buffer) { pixels = buffer; }
    void clear(uint16_t color);
    void pixel(int x, int y, uint16_t color);
    void alphaPixel(int x, int y, uint16_t color, unsigned alpha);
    void fill(int x, int y, int w, int h, uint16_t color);
    void shade(int x, int y, int w, int h, uint16_t color, unsigned alpha);
    void box(int x, int y, int w, int h, uint16_t color);
    void line(int x0, int y0, int x1, int y1, uint16_t color);
    void circle(int x, int y, int radius, uint16_t color);
    void disk(int x, int y, int radius, uint16_t color);
    void glow(int x, int y, int radius, uint16_t color);
    void sprite(int x, int y, const uint16_t* colors, const uint8_t* alpha,
                int width, int height, int originX, int originY,
                int cosine = 256, int sine = 0, int scale = 256,
                uint16_t tint = 65535, unsigned opacity = 255, bool additive = false,
                unsigned lighting = 0, bool mirrorY = false, int scaleY = 0);
    void bar(int x, int y, int w, int h, int value, int maximum, uint16_t color);
    void text(int x, int y, const char* string, uint16_t color, int scale = 1);
    void clipped(int x,int y,int width,const char* string,uint16_t color);
    void centered(int y, const char* string, uint16_t color, int scale = 1);
    void right(int x, int y, const char* string, uint16_t color, int scale = 1);
    int wrapped(int x, int y, int width, int maxLines, const char* string, uint16_t color);
    int wrappedPage(int x,int y,int width,int maxLines,int skipLines,const char* string,uint16_t color);
    static int textWidth(const char* string, int scale = 1);
    static void setFont(const Font* font);
    static int lineHeight();
    static uint16_t blend(uint16_t dest, uint16_t source, unsigned alpha);
};

void decimal(char* output, unsigned value);
void append(char* output, unsigned capacity, const char* text);
void appendNumber(char* output, unsigned capacity, unsigned value);

} // namespace nova
