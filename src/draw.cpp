#include "draw.h"
#include "font.h"

namespace nova {
static const Font* activeFont;
void Canvas::setFont(const Font* f) { activeFont=f&&f->ready()?f:0; }
int Canvas::lineHeight() { return activeFont?activeFont->line_height():10; }

// Five-column bitmap alphabet for the emergency UI and small HUD labels.
// Game text can use the converted font; this alphabet also keeps missing-data
// errors readable before any content pack has been opened.
static const uint8_t kGlyphs[96][5] = {
    {0,0,0,0,0},{0,0,95,0,0},{0,7,0,7,0},{20,127,20,127,20},
    {36,42,127,42,18},{35,19,8,100,98},{54,73,85,34,80},{0,5,3,0,0},
    {0,28,34,65,0},{0,65,34,28,0},{20,8,62,8,20},{8,8,62,8,8},
    {0,80,48,0,0},{8,8,8,8,8},{0,96,96,0,0},{32,16,8,4,2},
    {62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
    {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
    {54,73,73,73,54},{6,73,73,41,30},{0,54,54,0,0},{0,86,54,0,0},
    {8,20,34,65,0},{20,20,20,20,20},{0,65,34,20,8},{2,1,81,9,6},
    {50,73,121,65,62},{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},
    {127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},
    {127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},
    {127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},
    {127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},
    {1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},
    {99,20,8,20,99},{7,8,112,8,7},{97,81,73,69,67},{0,127,65,65,0},
    {2,4,8,16,32},{0,65,65,127,0},{4,2,1,2,4},{64,64,64,64,64},
    {0,1,2,4,0},{32,84,84,84,120},{127,72,68,68,56},{56,68,68,68,32},
    {56,68,68,72,127},{56,84,84,84,24},{8,126,9,1,2},{12,82,82,82,62},
    {127,8,4,4,120},{0,68,125,64,0},{32,64,68,61,0},{127,16,40,68,0},
    {0,65,127,64,0},{124,4,24,4,120},{124,8,4,4,120},{56,68,68,68,56},
    {124,20,20,20,8},{8,20,20,24,124},{124,8,4,4,8},{72,84,84,84,32},
    {4,63,68,64,32},{60,64,64,32,124},{28,32,64,32,28},{60,64,48,64,60},
    {68,40,16,40,68},{12,80,80,80,60},{68,100,84,76,68},{0,8,54,65,0},
    {0,0,127,0,0},{0,65,54,8,0},{8,4,8,16,8},{0,0,0,0,0}
};

void Canvas::clear(uint16_t color) {
    for (int i = 0; i < ScreenW * ScreenH; ++i) pixels[i] = color;
}

void Canvas::pixel(int x, int y, uint16_t color) {
    if (unsigned(x) < unsigned(ScreenW) && unsigned(y) < unsigned(ScreenH))
        pixels[y * ScreenW + x] = color;
}

uint16_t Canvas::blend(uint16_t d, uint16_t s, unsigned a) {
    if (a >= 255) return s;
    unsigned inv = 256 - a;
    unsigned r = (((d >> 11) * inv + (s >> 11) * a) >> 8) & 31;
    unsigned g = ((((d >> 5) & 63) * inv + ((s >> 5) & 63) * a) >> 8) & 63;
    unsigned b = (((d & 31) * inv + (s & 31) * a) >> 8) & 31;
    return uint16_t((r << 11) | (g << 5) | b);
}

void Canvas::alphaPixel(int x, int y, uint16_t color, unsigned a) {
    if (unsigned(x) < unsigned(ScreenW) && unsigned(y) < unsigned(ScreenH)) {
        uint16_t& d = pixels[y * ScreenW + x];
        d = blend(d, color, a);
    }
}

void Canvas::fill(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > ScreenW) w = ScreenW - x;
    if (y + h > ScreenH) h = ScreenH - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; ++yy) {
        uint16_t* row = pixels + yy * ScreenW + x;
        for (int xx = 0; xx < w; ++xx) row[xx] = color;
    }
}

void Canvas::shade(int x, int y, int w, int h, uint16_t color, unsigned a) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > ScreenW) w = ScreenW - x;
    if (y + h > ScreenH) h = ScreenH - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; ++yy) {
        uint16_t* row = pixels + yy * ScreenW + x;
        for (int xx = 0; xx < w; ++xx) row[xx] = blend(row[xx], color, a);
    }
}

void Canvas::box(int x, int y, int w, int h, uint16_t c) {
    fill(x, y, w, 1, c); fill(x, y+h-1, w, 1, c);
    fill(x, y, 1, h, c); fill(x+w-1, y, 1, h, c);
}

void Canvas::line(int x, int y, int tx, int ty, uint16_t c) {
    // Bound work before Bresenham: callers may hand us an offscreen projectile.
    if ((x < 0 && tx < 0) || (x >= ScreenW && tx >= ScreenW) ||
        (y < 0 && ty < 0) || (y >= ScreenH && ty >= ScreenH)) return;
    int dx = tx - x; if (dx < 0) dx = -dx;
    int dy = ty - y; if (dy < 0) dy = -dy;
    if (dx > 1024 || dy > 1024) return;
    int sx = x < tx ? 1 : -1, sy = y < ty ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        pixel(x, y, c);
        if (x == tx && y == ty) break;
        int e = err * 2;
        if (e > -dy) { err -= dy; x += sx; }
        if (e < dx) { err += dx; y += sy; }
    }
}

void Canvas::circle(int cx, int cy, int r, uint16_t c) {
    if (r <= 0 || r > 400) return;
    int x = r, y = 0, e = 1-r;
    while (x >= y) {
        pixel(cx+x,cy+y,c); pixel(cx+y,cy+x,c);
        pixel(cx-y,cy+x,c); pixel(cx-x,cy+y,c);
        pixel(cx-x,cy-y,c); pixel(cx-y,cy-x,c);
        pixel(cx+y,cy-x,c); pixel(cx+x,cy-y,c);
        ++y;
        if (e < 0) e += 2*y+1;
        else { --x; e += 2*(y-x)+1; }
    }
}

void Canvas::disk(int cx, int cy, int r, uint16_t c) {
    if (r < 0 || r > 400) return;
    int x = r, y = 0, e = 1-r;
    while (x >= y) {
        fill(cx-x,cy+y,2*x+1,1,c); fill(cx-x,cy-y,2*x+1,1,c);
        fill(cx-y,cy+x,2*y+1,1,c); fill(cx-y,cy-x,2*y+1,1,c);
        ++y;
        if (e < 0) e += 2*y+1;
        else { --x; e += 2*(y-x)+1; }
    }
}

void Canvas::glow(int x, int y, int radius, uint16_t c) {
    if (radius < 1) { pixel(x, y, c); return; }
    int r2 = radius*radius;
    for (int dy=-radius;dy<=radius;++dy) for(int dx=-radius;dx<=radius;++dx) {
        int distance = dx*dx+dy*dy;
        if (distance <= r2) alphaPixel(x+dx,y+dy,c,unsigned(200*(r2-distance+1)/(r2+1)));
    }
}

void Canvas::bar(int x, int y, int w, int h, int value, int maximum, uint16_t c) {
    fill(x, y, w, h, rgb(22,32,44));
    if (maximum <= 0) return;
    if (value < 0) value = 0;
    if (value > maximum) value = maximum;
    while (maximum > 4000000) { maximum >>= 1; value >>= 1; }
    fill(x, y, w*value/maximum, h, c);
}

void Canvas::sprite(int x, int y, const uint16_t* colors, const uint8_t* alpha,
                    int w, int h, int ox, int oy, int cosine, int sine,
                    int scale, uint16_t tint, unsigned opacity, bool additive) {
    if (!colors || w <= 0 || h <= 0 || scale <= 0 || scale > 1024) return;
    int xmin=1024,ymin=1024,xmax=-1024,ymax=-1024;
    const int xx[4]={-ox,w-ox,-ox,w-ox};
    const int yy[4]={-oy,-oy,h-oy,h-oy};
    for (int i=0;i<4;++i) {
        int dx=((xx[i]*cosine-yy[i]*sine)*scale)/65536;
        int dy=((xx[i]*sine+yy[i]*cosine)*scale)/65536;
        if(dx<xmin)xmin=dx;
        if(dx>xmax)xmax=dx;
        if(dy<ymin)ymin=dy;
        if(dy>ymax)ymax=dy;
    }
    xmin+=x-1; xmax+=x+1; ymin+=y-1; ymax+=y+1;
    if(xmin<0)xmin=0;
    if(ymin<0)ymin=0;
    if(xmax>=ScreenW)xmax=ScreenW-1;
    if(ymax>=ScreenH)ymax=ScreenH-1;
    if(xmin>xmax || ymin>ymax)return;
    int ic=cosine*256/scale, is=sine*256/scale;
    bool direct=cosine==256 && sine==0 && scale==256;
    unsigned tr=tint>>11,tg=(tint>>5)&63,tb=tint&31;
    for(int dy=ymin;dy<=ymax;++dy) {
        int sxq=(xmin-x)*ic+(dy-y)*is+ox*256;
        int syq=-(xmin-x)*is+(dy-y)*ic+oy*256;
        uint16_t* dest=pixels+dy*ScreenW+xmin;
        for(int dx=xmin;dx<=xmax;++dx,++dest,sxq+=ic,syq-=is) {
            int sx=direct?dx-x+ox:sxq>>8;
            int sy=direct?dy-y+oy:syq>>8;
            if(unsigned(sx)>=unsigned(w)||unsigned(sy)>=unsigned(h))continue;
            unsigned at=unsigned(sy*w+sx);
            unsigned a=alpha?alpha[at]:255;
            if(!a)continue;
            if(opacity<255)a=a*opacity/255;
            uint16_t c=colors[at];
            if(tint!=65535) {
                unsigned r=(c>>11)*tr/31;
                unsigned g=((c>>5)&63)*tg/63;
                unsigned b=(c&31)*tb/31;
                c=uint16_t((r<<11)|(g<<5)|b);
            }
            if(additive) {
                unsigned r=(*dest>>11)+((c>>11)*a/255);
                unsigned g=((*dest>>5)&63)+(((c>>5)&63)*a/255);
                unsigned b=(*dest&31)+((c&31)*a/255);
                if(r>31)r=31;
                if(g>63)g=63;
                if(b>31)b=31;
                *dest=uint16_t((r<<11)|(g<<5)|b);
            } else *dest=a==255?c:blend(*dest,c,a);
        }
    }
}

void Canvas::text(int x, int y, const char* s, uint16_t c, int scale) {
    if (!s || scale < 1 || scale > 4) return;
    int left = x;
    while (*s) {
        unsigned ch=next_utf8(s);
        if (ch == '\n') { y+=lineHeight()*scale; x=left; continue; }
        if (activeFont) {
            const GlyphRecord* glyph=activeFont->glyph(ch);
            if(!glyph)continue;
            const uint8_t* a=activeFont->pixels(*glyph);
            int gx=x+glyph->bearing_x*scale;
            int gy=y+(activeFont->pixel_height()-glyph->bearing_y)*scale;
            for(unsigned row=0;row<glyph->height;++row) for(unsigned col=0;col<glyph->width;++col) {
                unsigned opacity=a[row*glyph->width+col];
                if(!opacity)continue;
                for(int dy=0;dy<scale;++dy)for(int dx=0;dx<scale;++dx)
                    alphaPixel(gx+int(col)*scale+dx,gy+int(row)*scale+dy,c,opacity);
            }
            x+=glyph->advance*scale;
            continue;
        }
        if (ch < 32 || ch > 126) ch = '?';
        if (x > -6*scale && x < ScreenW && y > -7*scale && y < ScreenH) {
            const uint8_t* g=kGlyphs[ch-32];
            for (int col=0;col<5;++col) for (int row=0;row<7;++row)
                if (g[col] & (1<<row)) fill(x+col*scale,y+row*scale,scale,scale,c);
        }
        x+=6*scale;
    }
}

int Canvas::textWidth(const char* s, int scale) {
    if (!s) return 0;
    int n=0, max=0;
    while(*s) {
        unsigned ch=next_utf8(s);
        if (ch=='\n') { if(n>max)max=n; n=0; }
        else if(activeFont){const GlyphRecord* g=activeFont->glyph(ch);if(g)n+=g->advance;}
        else n+=6;
    }
    if(n>max)max=n;
    return (activeFont?max:(max?max-1:0))*scale;
}

void Canvas::centered(int y, const char* s, uint16_t c, int scale) {
    text((ScreenW-textWidth(s,scale))/2,y,s,c,scale);
}
void Canvas::right(int x, int y, const char* s, uint16_t c, int scale) {
    text(x-textWidth(s,scale),y,s,c,scale);
}

int Canvas::wrapped(int x, int y, int width, int maxLines, const char* s, uint16_t c) {
    if (!s || width < 6 || maxLines < 1) return 0;
    int lines=0;
    while(*s && lines<maxLines) {
        char lineBuffer[256];
        const char* scan=s;
        const char* cut=s;
        const char* space=0;
        int measured=0;
        while(*scan && *scan!='\n') {
            const char* before=scan;
            unsigned ch=next_utf8(scan);
            int advance=6;
            if(activeFont){const GlyphRecord* g=activeFont->glyph(ch);advance=g?g->advance:0;}
            if(measured+advance>width || scan-s>254) { scan=before;break; }
            measured+=advance;cut=scan;
            if(ch==' ')space=before;
        }
        if(*scan && *scan!='\n' && space && space>s)cut=space;
        if(cut==s){cut=s;next_utf8(cut);}
        unsigned count=unsigned(cut-s);
        for(unsigned i=0;i<count;++i)lineBuffer[i]=s[i];
        lineBuffer[count]=0;
        text(x,y+lines*lineHeight(),lineBuffer,c);
        s=cut;if(*s=='\n')++s;
        while(*s==' ')++s;
        ++lines;
    }
    return lines;
}

void decimal(char* out, unsigned value) {
    char reverse[12]; unsigned n=0;
    do { reverse[n++]=char('0'+value%10); value/=10; } while(value);
    for(unsigned i=0;i<n;++i)out[i]=reverse[n-i-1];
    out[n]=0;
}
void append(char* out, unsigned cap, const char* text) {
    if(!cap)return;
    unsigned n=0; while(n+1<cap && out[n])++n;
    while(n+1<cap && *text)out[n++]=*text++;
    out[n]=0;
}
void appendNumber(char* out, unsigned cap, unsigned value) {
    char b[12];decimal(b,value);append(out,cap,b);
}

} // namespace nova
