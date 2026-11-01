#pragma once
#include "platform.h"

namespace nova {
bool app_init(const char* base_path);
void app_frame(const Input& input,uint32_t elapsed_ms);
void app_render(uint16_t* framebuffer);
bool app_quit();
#ifdef NOVA_TEST
class Game;
Game& app_test_game();
unsigned app_test_page();
unsigned app_test_selection();
unsigned app_test_flags();
#endif
}
