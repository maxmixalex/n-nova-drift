#pragma once
#include <stddef.h>
#include <stdint.h>

namespace nova {
constexpr int ScreenWidth = 320;
constexpr int ScreenHeight = 240;

enum Button : uint32_t {
    ButtonUp = 1u << 0, ButtonDown = 1u << 1,
    ButtonLeft = 1u << 2, ButtonRight = 1u << 3,
    ButtonConfirm = 1u << 4, ButtonBack = 1u << 5,
    ButtonFire = 1u << 6, ButtonThrust = 1u << 7,
    ButtonPause = 1u << 8, ButtonMenu = 1u << 9,
    ButtonTab = 1u << 10, ButtonReroll = 1u << 11,
    Button1 = 1u << 12, Button2 = 1u << 13, Button3 = 1u << 14,
    ButtonUpgrade = 1u << 15,
    ButtonPointerMode = 1u << 16, // 0: explicit keypad / touch-aim mode switch
    ButtonBanish = 1u << 17,
};

struct Input {
    uint32_t held, pressed, released;
    int16_t pointer_x, pointer_y; // absolute screen coordinates
    bool pointer_present;      // a finger/mouse pointer is active
    bool pointer_down, pointer_pressed, pointer_released;
    bool pointer_is_touchpad;  // physical calculator pad, not a desktop mouse
    bool quit;                 // host window close only; Back belongs to App
    uint32_t physical_keys[3];  // CX 8x11 matrix IDs 1..88; desktop arrows 89..92
    bool physical_valid;       // synthetic callers may supply only held bits
};

namespace platform {
// Both backends share this API. Paths are relative to the launcher directory.
bool initialize(const char* launcher_path);
void shutdown();
Input poll();
uint32_t ticks_ms();
void wait_ms(uint32_t milliseconds);
void present(const uint16_t* rgb565);
uint32_t seed();
// UTC days since 1970, or zero when the calculator RTC has no plausible date.
// Never use the elapsed runtime counter as a calendar date.
uint32_t utc_day();
const char* base_path();
// Actual OS locale tag (for example en-US or zh-CN), or nullptr when unknown.
// The application chooses among installed packs and owns fallback policy.
const char* system_language();

using File = void;
File* open_data(const char* filename);
File* open_state(const char* filename, bool writing = false);
size_t read(File* file, void* destination, size_t bytes);
bool seek(File* file, uint32_t offset);
uint32_t length(File* file);
void close(File* file);
// State writes use a temporary sibling and backup to preserve the old save.
bool write_state(const char* filename, const void* data, size_t bytes);
bool read_state(const char* filename, void* data, size_t bytes);
// Host-only options; these are harmless no-ops on the calculator.
void configure_host(const char* data_directory, const char* screenshot_path,
                    uint32_t frame_limit, bool headless, bool demo);
bool host_demo();
}
}
