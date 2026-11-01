#include "platform.h"
#include "input_controller.h"
#include "system_locale.h"

#if defined(NOVA_NDLESS)
extern "C" {
void* nd_open(const char*, const char*);
unsigned nd_read(void*, unsigned, unsigned, void*);
unsigned nd_write(const void*, unsigned, unsigned, void*);
int nd_close(void*);
int nd_seek(void*, long, int);
long nd_tell(void*);
int nd_mkdir(const char*);
int nd_unlink(const char*);
int nd_rename(const char*, const char*);
unsigned nd_hwtype();
unsigned nd_revision();
int nd_has_syscall(unsigned);
void* nd_calc_address();
int nd_math_to_string(void*,void*,uint16_t**);
void nd_os_free(void*);
int nd_inflate_init(void*,int,const char*,unsigned);
int nd_inflate(void*,int);
int nd_inflate_end(void*);
int nd_lcd_init(int);
void (*nd_blitter(int))(void*);
int nd_touch_read(unsigned, unsigned, void*);
int nd_touch_write(unsigned, unsigned, const void*);
}
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <sys/stat.h>
#if defined(NOVA_SDL)
#include <SDL.h>
#endif
#endif

namespace nova { namespace platform {
namespace {
char root_path[512];
uint32_t previous_buttons;
bool previous_pointer;
int16_t pointer_x = 160, pointer_y = 120;

bool copy_text(char* out, size_t capacity, const char* value) {
    if (!value || !capacity) return false;
    size_t i = 0;
    for (; value[i]; ++i) { if (i + 1 >= capacity) return false; out[i] = value[i]; }
    out[i] = 0;
    return true;
}
size_t text_length(const char* text) { size_t n=0; while (text[n]) ++n; return n; }
bool append(char* out, size_t capacity, const char* suffix) {
    const size_t used = text_length(out);
    return used < capacity && copy_text(out + used, capacity - used, suffix);
}
bool safe_name(const char* name) {
    if (!name || !*name) return false;
    for (size_t i=0; name[i]; ++i)
        if (name[i]=='/' || name[i]=='\\' || (name[i]=='.' && name[i+1]=='.')) return false;
    return true;
}
void make_root(const char* launcher) {
    if (!launcher || !copy_text(root_path,sizeof root_path,launcher)) { root_path[0]='.'; root_path[1]=0; return; }
    int last=-1;
    for (int i=0; root_path[i]; ++i) if (root_path[i]=='/' || root_path[i]=='\\') last=i;
    if (last < 0) { root_path[0]='.'; root_path[1]=0; }
    else if (!last) root_path[1]=0;
    else root_path[last]=0;
}

#if defined(NOVA_NDLESS)
void (*screen_blit)(void*);
uint32_t timer_load, timer_control, timer_last, clock_ms, clock_fraction;
uint16_t pad_width, pad_height;
bool timer_active, screen_active;
uint32_t timer_raw() { return *reinterpret_cast<volatile uint32_t*>(0x900D0004u); }
bool key(unsigned offset, unsigned mask) {
    return (*reinterpret_cast<volatile uint16_t*>(0x900E0000u + offset) & mask) != 0;
}
void initialize_touchpad() {
    // Ndless' public I2C protocol: capabilities page 16, OS report page 4.
    // Interrupts must be restored after returning the device to page 4.
    uint32_t cpsr;
    asm volatile("mrs %0,cpsr" : "=r"(cpsr));
    asm volatile("msr cpsr_c,%0" :: "r"(cpsr | 0xC0) : "memory");
    uint8_t page=16, dimensions[4]={0,0,0,0};
    bool ok=nd_touch_write(255,255,&page)!=0;
    if (ok) ok=nd_touch_read(4,7,dimensions)!=0;
    page=4;
    const bool restored=nd_touch_write(255,255,&page)!=0;
    asm volatile("msr cpsr_c,%0" :: "r"(cpsr) : "memory");
    pad_width=ok&&restored?uint16_t((unsigned(dimensions[0])<<8)|dimensions[1]):0;
    pad_height=ok&&restored?uint16_t((unsigned(dimensions[2])<<8)|dimensions[3]):0;
}
void* raw_open(const char* path,const char* mode) { return nd_open(path,mode); }
size_t raw_read(void* file,void* data,size_t count) { return nd_read(data,1,unsigned(count),file); }
size_t raw_write(void* file,const void* data,size_t count) { return nd_write(data,1,unsigned(count),file); }
int raw_close(void* file) { return nd_close(file); }
int raw_seek(void* file,long offset,int origin) { return nd_seek(file,offset,origin); }
long raw_tell(void* file) { return nd_tell(file); }
void raw_mkdir(const char* path) { (void)nd_mkdir(path); }
int raw_unlink(const char* path) { return nd_unlink(path); }
int raw_rename(const char* from,const char* to) { return nd_rename(from,to); }
#else
char host_data_path[512], screenshot_path[512];
uint32_t host_frame_limit, host_frame_count;
bool host_headless, demo_enabled, host_quit;
uint16_t last_frame[ScreenWidth*ScreenHeight];
std::chrono::steady_clock::time_point started;
#if defined(NOVA_SDL)
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
uint32_t keyboard_buttons;
uint8_t keyboard_counts[18],physical_counts[93];
uint32_t keyboard_physical[3];
bool mouse_seen, mouse_down;
uint8_t physical_for_key(SDL_Keycode code) {
    static const uint8_t letters[26]={
        calculator_key(4,6),calculator_key(4,5),calculator_key(4,4),calculator_key(4,2),calculator_key(4,1),calculator_key(4,0),
        calculator_key(3,6),calculator_key(3,5),calculator_key(3,4),calculator_key(3,2),calculator_key(3,1),calculator_key(3,0),
        calculator_key(2,6),calculator_key(2,5),calculator_key(2,4),calculator_key(2,2),calculator_key(2,1),calculator_key(2,0),
        calculator_key(1,6),calculator_key(1,5),calculator_key(1,4),calculator_key(1,2),calculator_key(1,1),calculator_key(1,0),
        calculator_key(0,6),calculator_key(0,5)};
    static const uint8_t digits[10]={Key0,Key1,Key2,Key3,Key4,Key5,Key6,Key7,Key8,Key9};
    if(code>=SDLK_a&&code<=SDLK_z)return letters[code-SDLK_a];
    if(code>=SDLK_0&&code<=SDLK_9)return digits[code-SDLK_0];
    switch(code) {
    case SDLK_KP_0:return Key0;case SDLK_KP_1:return Key1;case SDLK_KP_2:return Key2;
    case SDLK_KP_3:return Key3;case SDLK_KP_4:return Key4;case SDLK_KP_5:return Key5;
    case SDLK_KP_6:return Key6;case SDLK_KP_7:return Key7;case SDLK_KP_8:return Key8;case SDLK_KP_9:return Key9;
    case SDLK_RETURN:case SDLK_KP_ENTER:return KeyEnter;case SDLK_ESCAPE:return KeyEscape;
    case SDLK_SPACE:return KeySpace;case SDLK_TAB:return KeyTab;case SDLK_HOME:return KeyHome;
    case SDLK_LSHIFT:case SDLK_RSHIFT:return KeyShift;case SDLK_LCTRL:case SDLK_RCTRL:return KeyControl;
    case SDLK_UP:return KeyHostUp;case SDLK_DOWN:return KeyHostDown;case SDLK_LEFT:return KeyHostLeft;case SDLK_RIGHT:return KeyHostRight;
    case SDLK_MINUS:case SDLK_KP_MINUS:return KeyMinus;case SDLK_PLUS:case SDLK_KP_PLUS:return calculator_key(6,2);
    case SDLK_DELETE:case SDLK_BACKSPACE:return KeyDelete;case SDLK_MENU:return KeyMenu;
    case SDLK_PERIOD:case SDLK_KP_PERIOD:return calculator_key(5,4);case SDLK_COMMA:return calculator_key(7,10);
    case SDLK_EQUALS:return calculator_key(4,7);case SDLK_SLASH:case SDLK_KP_DIVIDE:return calculator_key(3,8);
    case SDLK_ASTERISK:case SDLK_KP_MULTIPLY:return calculator_key(4,8);case SDLK_QUOTE:return calculator_key(5,0);
    case SDLK_LEFTPAREN:return calculator_key(5,5);case SDLK_RIGHTPAREN:return calculator_key(5,3);
    default:return 0;
    }
}
uint32_t button_for_key(SDL_Keycode code) {
    switch (code) {
    case SDLK_UP: case SDLK_w: case SDLK_8: case SDLK_KP_8: return ButtonUp|ButtonThrust;
    case SDLK_DOWN: case SDLK_s: return ButtonDown;
    case SDLK_LEFT: case SDLK_a: case SDLK_4: case SDLK_KP_4: return ButtonLeft;
    case SDLK_RIGHT: case SDLK_d: case SDLK_6: case SDLK_KP_6: return ButtonRight;
    case SDLK_RETURN: case SDLK_KP_ENTER: return ButtonConfirm;
    case SDLK_ESCAPE: return ButtonBack;
    case SDLK_5: case SDLK_KP_5: case SDLK_LCTRL: case SDLK_RCTRL:
    case SDLK_SPACE: case SDLK_z: return ButtonFire;
    case SDLK_LSHIFT: case SDLK_RSHIFT: case SDLK_x: return ButtonThrust;
    case SDLK_p: return ButtonPause;
    case SDLK_m: return ButtonMenu;
    case SDLK_7: case SDLK_KP_7: case SDLK_TAB: return ButtonTab;
    case SDLK_MINUS: case SDLK_KP_MINUS: case SDLK_r: return ButtonReroll;
    case SDLK_1: case SDLK_KP_1: return 0;
    case SDLK_2: case SDLK_KP_2: return ButtonDown;
    case SDLK_3: case SDLK_KP_3: return 0;
    case SDLK_9: case SDLK_KP_9: case SDLK_u: return ButtonUpgrade;
    case SDLK_0: case SDLK_KP_0: return ButtonPointerMode;
    case SDLK_DELETE: case SDLK_BACKSPACE: return ButtonBanish;
    default: return 0;
    }
}
void keyboard_event(SDL_Keycode code,bool down) {
    const uint8_t physical=physical_for_key(code);
    if(physical) {
        if(down){if(physical_counts[physical]<255)++physical_counts[physical];}
        else if(physical_counts[physical])--physical_counts[physical];
        physical_set(keyboard_physical,physical,physical_counts[physical]!=0);
    }
    const uint32_t mask=button_for_key(code);
    for (unsigned bit=0;bit<18;++bit) if (mask&(1u<<bit)) {
        if (down) { if (keyboard_counts[bit]<255) ++keyboard_counts[bit]; }
        else if (keyboard_counts[bit]) --keyboard_counts[bit];
        if (keyboard_counts[bit]) keyboard_buttons|=1u<<bit;
        else keyboard_buttons&=~(1u<<bit);
    }
}
#endif
void* raw_open(const char* path,const char* mode) { return fopen(path,mode); }
size_t raw_read(void* file,void* data,size_t count) { return fread(data,1,count,static_cast<FILE*>(file)); }
size_t raw_write(void* file,const void* data,size_t count) { return fwrite(data,1,count,static_cast<FILE*>(file)); }
int raw_close(void* file) { return fclose(static_cast<FILE*>(file)); }
int raw_seek(void* file,long offset,int origin) { return fseek(static_cast<FILE*>(file),offset,origin); }
long raw_tell(void* file) { return ftell(static_cast<FILE*>(file)); }
void raw_mkdir(const char* path) {
#ifdef _WIN32
    (void)_mkdir(path);
#else
    (void)mkdir(path,0755);
#endif
}
int raw_unlink(const char* path) { return remove(path); }
int raw_rename(const char* from,const char* to) { return rename(from,to); }
void save_screenshot() {
    if (!*screenshot_path) return;
    FILE* file=fopen(screenshot_path,"wb");
    if (!file) { fprintf(stderr,"Cannot write screenshot: %s\n",screenshot_path); return; }
    fprintf(file,"P6\n320 240\n255\n");
    for (unsigned i=0;i<ScreenWidth*ScreenHeight;++i) {
        const unsigned p=last_frame[i];
        const unsigned char rgb[3]={static_cast<unsigned char>(((p>>11)&31)*255/31),
            static_cast<unsigned char>(((p>>5)&63)*255/63),static_cast<unsigned char>((p&31)*255/31)};
        fwrite(rgb,1,3,file);
    }
    fclose(file);
}
#endif

bool path(char* output,const char* directory,const char* name) {
    if (!safe_name(name)) return false;
#if !defined(NOVA_NDLESS)
    if (*host_data_path && directory[0]=='d')
        return copy_text(output,512,host_data_path) && append(output,512,"/") && append(output,512,name);
#endif
    return copy_text(output,512,root_path) && append(output,512,"/") &&
           append(output,512,directory) && append(output,512,"/") && append(output,512,name);
}
void ensure_state_directory() {
    char directory[512];
    if (copy_text(directory,sizeof directory,root_path) && append(directory,sizeof directory,"/state")) raw_mkdir(directory);
}
bool exact_state(const char* filename,void* data,size_t bytes) {
    char location[512];
    if (!path(location,"state",filename)) return false;
    void* file=raw_open(location,"rb"); if (!file) return false;
    const bool sized=raw_seek(file,0,2)==0 && raw_tell(file)==long(bytes) && raw_seek(file,0,0)==0;
    const bool good=sized && raw_read(file,data,bytes)==bytes;
    return raw_close(file)==0 && good;
}
}

bool initialize(const char* launcher_path) {
    make_root(launcher_path);
    previous_buttons=0; previous_pointer=false;pointer_x=160;pointer_y=120;
#if defined(NOVA_NDLESS)
    // CX/CX II; old monochrome Clickpad hardware cannot show this color game.
    if (nd_revision()<2022 || nd_hwtype()==0) return false;
    volatile uint32_t* timer=reinterpret_cast<volatile uint32_t*>(0x900D0000u);
    timer_load=timer[0]; timer_control=timer[2]; timer[2]=0; timer[0]=0xffffffffu; timer[2]=0x82;
    timer_last=timer[1]; clock_ms=clock_fraction=0; timer_active=true;
    if (!nd_lcd_init(0)) { shutdown(); return false; }
    screen_active=true; screen_blit=nd_blitter(0);
    if (!screen_blit) { shutdown(); return false; }
    initialize_touchpad();
#else
    started=std::chrono::steady_clock::now();host_frame_count=0;host_quit=false;
#if defined(NOVA_SDL)
    keyboard_buttons=0;for (auto& count:keyboard_counts) count=0;
    for(auto& count:physical_counts)count=0;
    for(auto& bits:keyboard_physical)bits=0;
    mouse_seen=mouse_down=false;
#endif
    if (*host_data_path) {
        copy_text(root_path,sizeof root_path,host_data_path);
        size_t n=text_length(root_path);
        if (n>5 && root_path[n-5]=='/' && root_path[n-4]=='d' && root_path[n-3]=='a' && root_path[n-2]=='t' && root_path[n-1]=='a') root_path[n-5]=0;
    }
#if defined(NOVA_SDL)
    if (!host_headless) {
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0) { fprintf(stderr,"SDL: %s\n",SDL_GetError()); return false; }
        window=SDL_CreateWindow("N-Nova Drift - TI-Nspire preview",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_RESIZABLE);
        renderer=window?SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC):nullptr;
        if (window && !renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
        texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,320,240):nullptr;
        if (!texture) { fprintf(stderr,"SDL: %s\n",SDL_GetError()); shutdown(); return false; }
        SDL_RenderSetLogicalSize(renderer,320,240);
    }
#else
    if (!host_headless) { fprintf(stderr,"This build has no SDL2. Use --headless --frames N, or install SDL2 development files and rebuild.\n"); return false; }
#endif
#endif
    ensure_state_directory();
    return true;
}

void shutdown() {
#if defined(NOVA_NDLESS)
    if (timer_active) {
        // Allow the exit key to settle before returning to the OS document list.
        const uint32_t start=ticks_ms(); uint32_t quiet=start;
        while (ticks_ms()-start<1500) {
            bool any=false;
            for (unsigned off=0x10;off<0x20;off+=2) any=key(off,0xffff)||any;
            const uint32_t now=ticks_ms(); if (any) quiet=now; else if (now-quiet>=100) break;
        }
    }
    if (screen_active) { nd_lcd_init(-1); screen_active=false; }
    if (timer_active) {
        volatile uint32_t* timer=reinterpret_cast<volatile uint32_t*>(0x900D0000u);
        timer[2]=0; timer[0]=timer_load; timer[2]=timer_control; timer_active=false;
    }
#else
    save_screenshot();
#if defined(NOVA_SDL)
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    texture=nullptr; renderer=nullptr; window=nullptr; SDL_Quit();
#endif
#endif
}

uint32_t ticks_ms() {
#if defined(NOVA_NDLESS)
    const uint32_t now=timer_raw(), delta=timer_last-now; timer_last=now;
    const uint32_t partial=(delta&32767u)*1000u+clock_fraction;
    clock_ms+=(delta>>15)*1000u+(partial>>15); clock_fraction=partial&32767u;
    return clock_ms;
#else
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count());
#endif
}
void wait_ms(uint32_t milliseconds) {
#if defined(NOVA_NDLESS)
    const uint32_t start=ticks_ms(); while (ticks_ms()-start<milliseconds) {}
#else
    if (!host_headless) std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}
Input poll() {
    Input input{};
    input.pointer_x=pointer_x; input.pointer_y=pointer_y;
#if defined(NOVA_NDLESS)
    uint16_t rows[8];
    for (unsigned i=0;i<8;++i)
        rows[i]=*reinterpret_cast<volatile uint16_t*>(0x900E0010u+i*2);
    const uint32_t buttons=decode_calculator_keys(rows);
    input.physical_valid=true;
    for(unsigned row=0;row<8;++row)for(unsigned col=0;col<11;++col)
        if(rows[row]&(1u<<col))physical_set(input.physical_keys,calculator_key(row,col),true);
    input.pointer_is_touchpad=true;
    uint8_t report[12]={};
    if (pad_width && pad_height && nd_touch_read(0,10,report)) {
        const unsigned x=(unsigned(report[2])<<8)|report[3], y=(unsigned(report[4])<<8)|report[5];
        if (x<=pad_width && y<=pad_height) {
            input.pointer_present=report[0]!=0||report[10]!=0;
            if (input.pointer_present) {
                pointer_x=static_cast<int16_t>(x*319u/pad_width);
                pointer_y=static_cast<int16_t>((pad_height-y)*239u/pad_height);
                input.pointer_x=pointer_x; input.pointer_y=pointer_y;
            }
            input.pointer_down=report[10]!=0;
            // Keep the pad separate from the matrix. InputController chooses
            // either latched direction keys OR pointing; never both.
        }
    }
    input.held=buttons;
#else
    input.quit=host_quit || (host_frame_limit && host_frame_count>=host_frame_limit);
#if defined(NOVA_SDL)
    if (!host_headless) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_QUIT) input.quit=host_quit=true;
            if (event.type==SDL_KEYDOWN && !event.key.repeat) keyboard_event(event.key.keysym.sym,true);
            if (event.type==SDL_KEYUP) keyboard_event(event.key.keysym.sym,false);
            if (event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_FOCUS_LOST) {
                keyboard_buttons=0;for(auto& count:keyboard_counts)count=0;
                for(auto& count:physical_counts)count=0;
                for(auto& bits:keyboard_physical)bits=0;
                mouse_seen=mouse_down=false;
            }
            if (event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN || event.type==SDL_MOUSEBUTTONUP) {
                int x,y; SDL_GetMouseState(&x,&y);
                float lx,ly; SDL_RenderWindowToLogical(renderer,x,y,&lx,&ly);
                pointer_x=static_cast<int16_t>(lx<0?0:lx>319?319:lx);
                pointer_y=static_cast<int16_t>(ly<0?0:ly>239?239:ly);
                mouse_seen=true;
                if (event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT) mouse_down=true;
                if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) mouse_down=false;
            }
        }
        input.held=keyboard_buttons;input.physical_valid=true;
        for(unsigned i=0;i<3;++i)input.physical_keys[i]=keyboard_physical[i];
        input.pointer_present=mouse_seen; input.pointer_down=mouse_down;
        input.pointer_x=pointer_x; input.pointer_y=pointer_y;
    }
#endif
#endif
    input.pressed=input.held&~previous_buttons; input.released=previous_buttons&~input.held;
    input.pointer_pressed=input.pointer_down&&!previous_pointer;
    input.pointer_released=!input.pointer_down&&previous_pointer;
    previous_buttons=input.held; previous_pointer=input.pointer_down;
    return input;
}
void present(const uint16_t* rgb565) {
#if defined(NOVA_NDLESS)
    if (screen_blit) screen_blit(const_cast<uint16_t*>(rgb565));
#else
    memcpy(last_frame,rgb565,sizeof last_frame); ++host_frame_count;
#if defined(NOVA_SDL)
    if (texture) { SDL_UpdateTexture(texture,nullptr,rgb565,320*2); SDL_RenderClear(renderer); SDL_RenderCopy(renderer,texture,nullptr,nullptr); SDL_RenderPresent(renderer); }
#endif
#endif
}
uint32_t seed() {
#if defined(NOVA_NDLESS)
    return *reinterpret_cast<volatile uint32_t*>(0x90090000u)^timer_raw()^0x4e4f5641u;
#else
    return static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())^0x4e4f5641u;
#endif
}
uint32_t utc_day() {
#if defined(NOVA_NDLESS)
    const uint32_t seconds=*reinterpret_cast<volatile uint32_t*>(0x90090000u);
#else
    const auto now=std::chrono::system_clock::now().time_since_epoch();
    const auto raw=std::chrono::duration_cast<std::chrono::seconds>(now).count();
    if(raw<0||raw>0xffffffffll)return 0;
    const uint32_t seconds=static_cast<uint32_t>(raw);
#endif
    // A CX reset/hibernation may clear the RTC. Only a configured 2020..2099
    // clock is accepted; the application offers a saved, editable date otherwise.
    return seconds>=1577836800u&&seconds<=4102444799u?seconds/86400u:0;
}
const char* base_path() { return root_path; }
const char* system_language() {
#if defined(NOVA_NDLESS)
    static char tag[32];
    if (nd_has_syscall(339)&&nd_has_syscall(342)) {
        // calc_cmd is a five-argument OS function; call its address directly
        // instead of passing the fifth argument through a four-register SWI.
        using Evaluate=int(*)(void*,void*,const uint16_t*,void*,void*);
        const auto evaluate=reinterpret_cast<Evaluate>(nd_calc_address());
        static const uint16_t query[]={'g','e','t','L','a','n','g','I','n','f','o','(',')',0};
        void* expression=nullptr;uint16_t* text=nullptr;int offset=0;bool ok=false;
        if (evaluate&&!evaluate(nullptr,nullptr,query,&expression,&offset)&&expression&&
            !nd_math_to_string(expression,nullptr,&text))ok=locale_text(tag,text);
        if (text&&static_cast<void*>(text)!=expression)nd_os_free(text);
        if (expression)nd_os_free(expression);
        if (ok)return tag;
    }
    // Older supported OS versions do not export the expression formatter.
    // Read only the real current preference, using its bounded settings entry.
    static uint8_t settings[16384+8192];
    void* file=nd_open("/phoenix/syst/settings/current.zip","rb");
    if (!file)return nullptr;
    const unsigned count=nd_read(settings,1,16384,file);nd_close(file);
    LocaleArchiveEntry entry{};
    if (!locale_archive(settings,count,entry))return nullptr;
    if (!entry.deflated)return locale_xml(tag,entry.data,entry.plain)?tag:nullptr;
    struct Stream {
        const uint8_t* next_in;uint32_t avail_in,total_in;
        uint8_t* next_out;uint32_t avail_out,total_out;
        void* message;void* state;void* allocate;void* release;void* opaque;
        int32_t data_type;uint32_t adler,reserved;
    } stream{};
    static_assert(sizeof(Stream)==56,"ARM32 OS zlib ABI");
    stream.next_in=entry.data;stream.avail_in=entry.compressed;
    stream.next_out=settings+16384;stream.avail_out=entry.plain;
    if (nd_inflate_init(&stream,-15,"1.2.3",sizeof stream)!=0)return nullptr;
    const int result=nd_inflate(&stream,4);nd_inflate_end(&stream);
    return result==1&&stream.total_out==entry.plain&&!stream.avail_in&&
        locale_xml(tag,settings+16384,entry.plain)?tag:nullptr;
#else
    const char* value=getenv("LC_ALL");
    if (!value||!*value)value=getenv("LC_MESSAGES");
    if (!value||!*value)value=getenv("LANG");
    return value&&*value?value:nullptr;
#endif
}
File* open_data(const char* filename) { char location[512]; return path(location,"data",filename)?raw_open(location,"rb"):nullptr; }
File* open_state(const char* filename,bool writing) { char location[512]; return path(location,"state",filename)?raw_open(location,writing?"wb":"rb"):nullptr; }
size_t read(File* file,void* destination,size_t bytes) { return file?raw_read(file,destination,bytes):0; }
bool seek(File* file,uint32_t offset) { return file && offset<=0x7fffffffu && raw_seek(file,long(offset),0)==0; }
uint32_t length(File* file) {
    if (!file) return 0;
    const long old=raw_tell(file);
    if (old<0 || raw_seek(file,0,2)!=0) return 0;
    const long size=raw_tell(file);
    if (raw_seek(file,old,0)!=0 || size<0) return 0;
    return static_cast<uint32_t>(size);
}
void close(File* file) { if (file) raw_close(file); }
bool write_state(const char* filename,const void* data,size_t bytes) {
    char destination[512], temporary[512], backup[512];
    if (!path(destination,"state",filename) || !copy_text(temporary,sizeof temporary,destination) ||
        !append(temporary,sizeof temporary,".tmp.tns") || !copy_text(backup,sizeof backup,destination) ||
        !append(backup,sizeof backup,".bak.tns")) return false;
    ensure_state_directory();
    void* file=raw_open(temporary,"wb"); if (!file) return false;
    const bool written=raw_write(file,data,bytes)==bytes;
    if (raw_close(file)!=0 || !written) { raw_unlink(temporary); return false; }
    // A rename may not replace a Nucleus file. Preserve a previous generation.
    bool had_old=false;
    if (void* old=raw_open(destination,"rb")) { raw_close(old); had_old=true; }
    // If an earlier interrupted commit left only the backup, retain it until
    // the new temporary generation has actually reached the final name.
    if (had_old) raw_unlink(backup);
    if (had_old && raw_rename(destination,backup)!=0) { raw_unlink(temporary); return false; }
    if (raw_rename(temporary,destination)==0) return true;
    if (had_old) raw_rename(backup,destination);
    raw_unlink(temporary); return false;
}
bool read_state(const char* filename,void* data,size_t bytes) {
    if (exact_state(filename,data,bytes)) return true;
    char backup[256];
    return copy_text(backup,sizeof backup,filename) && append(backup,sizeof backup,".bak.tns") && exact_state(backup,data,bytes);
}
void configure_host(const char* data_directory,const char* screenshot,uint32_t frames,bool headless,bool demo) {
#if defined(NOVA_NDLESS)
    (void)data_directory; (void)screenshot; (void)frames; (void)headless; (void)demo;
#else
    if (data_directory) copy_text(host_data_path,sizeof host_data_path,data_directory);
    if (screenshot) copy_text(screenshot_path,sizeof screenshot_path,screenshot);
    host_frame_limit=frames; host_headless=headless; demo_enabled=demo;
#endif
}
bool host_demo() {
#if defined(NOVA_NDLESS)
    return false;
#else
    return demo_enabled;
#endif
}
} }
