#pragma once
#include "platform.h"

namespace nova {
bool app_init(const char* base_path);
void app_frame(const Input& input,uint32_t elapsed_ms);
void app_render(uint16_t* framebuffer);
bool app_quit();
#ifdef NOVA_TEST
class Game;
struct Career;
Game& app_test_game();
Career& app_test_career();
unsigned app_test_page();
unsigned app_test_selection();
unsigned app_test_flags();
unsigned app_test_language();
unsigned app_test_ending_ms();
unsigned app_test_stats_tab();
unsigned app_test_stats_page();
unsigned app_test_daily_day();
unsigned app_test_daily_score(unsigned day);
void app_test_set_page(unsigned page);
bool app_test_set_language(unsigned language);
#endif
}
