// Application-level regression uses a tiny, original synthetic pack in CI.
// Supplying a real data directory additionally exercises converted fonts/text.
#include "app.h"
#include "game.h"
#include "input_bindings.h"
#include "skins.h"
#include "nnova/pack_format.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs=std::filesystem;
using namespace nova;
static uint16_t framebuffer[320*240];
static uint32_t crc(const uint8_t* data,size_t n) {
    uint32_t c=0xffffffff;
    for(size_t i=0;i<n;++i){c^=data[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}
    return ~c;
}
static void write(const fs::path& p,const void* data,size_t bytes) {
    std::ofstream f(p,std::ios::binary);assert(f);f.write(static_cast<const char*>(data),std::streamsize(bytes));assert(f);
}
static void make_pack(const fs::path& directory) {
    using namespace nova::pack;
    fs::create_directories(directory);
    const uint8_t pixels[12]={255,255,255,255,255,255,255,255,255,255,255,255};
    std::vector<uint8_t> data(sizeof(DataHeader)+sizeof(pixels));
    DataHeader dh={};std::memcpy(dh.magic,"NNOVAP1",8);dh.version=Version;dh.index=0;dh.file_bytes=uint32_t(data.size());dh.payload_crc32=crc(pixels,sizeof(pixels));
    std::memcpy(data.data(),&dh,sizeof(dh));std::memcpy(data.data()+sizeof(dh),pixels,sizeof(pixels));
    write(directory/"nNova.gfx00.dat.tns",data.data(),data.size());
    CommonHeader h={};std::memcpy(h.magic,"NNOVAC1",8);h.version=Version;h.sprite_count=1;h.frame_count=1;h.pack_count=1;
    h.sprite_offset=sizeof(h);h.frame_offset=sizeof(h)+sizeof(SpriteRecord);h.file_bytes=h.frame_offset+sizeof(FrameRecord);h.screen_width=320;h.screen_height=240;
    SpriteRecord s={};s.frame_count=1;s.original_width=s.original_height=2;s.fps_q8=30*256;std::strcpy(s.name,"sPlayer");
    FrameRecord r={};r.offset=sizeof(DataHeader);r.stored_bytes=r.decoded_bytes=sizeof(pixels);r.width=r.height=2;r.origin_x=r.origin_y=1;r.canvas_width=r.canvas_height=2;r.codec=Raw565A8;
    std::vector<uint8_t> index(h.file_bytes);
    std::memcpy(index.data()+h.sprite_offset,&s,sizeof(s));std::memcpy(index.data()+h.frame_offset,&r,sizeof(r));
    h.index_crc32=crc(index.data()+sizeof(h),index.size()-sizeof(h));std::memcpy(index.data(),&h,sizeof(h));
    write(directory/"nNova.dat.tns",index.data(),index.size());
}
static void frame(Input in={}) {app_frame(in,34);app_render(framebuffer);}
static void tap(uint32_t key) {Input in={};in.pressed=key;in.held=key;frame(in);frame();}
static void initialize(const fs::path& root) {
    std::string path=(root/"data").string();
    platform::configure_host(path.c_str(),nullptr,0,true,false);
    assert(platform::initialize((root/"nNova.tns").string().c_str()));
    assert(app_init(platform::base_path()));frame();
}
int main(int argc,char** argv) {
    fs::path root=fs::temp_directory_path()/("nnova-app-regression-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    fs::remove_all(root);make_pack(root/"data");
    if(argc>1)for(const auto& entry:fs::directory_iterator(argv[1]))if(entry.path().extension()==".tns")
        fs::copy_file(entry.path(),root/"data"/entry.path().filename(),fs::copy_options::overwrite_existing);
    // The previous release's nine-word preferences migrate without losing scores.
    fs::create_directories(root/"state");uint32_t legacy_cfg[9]={0x4e4f5641u,2,2|32|128,777,9,2,0,0,0};legacy_cfg[8]=crc(reinterpret_cast<const uint8_t*>(legacy_cfg),32);write(root/"state"/"nNova.cfg.tns",legacy_cfg,sizeof(legacy_cfg));
    // A CRC-valid v2 career uses 64 counters. Migration preserves progress,
    // discovery and scores while adding a zero high discovery word.
    struct CareerV2 {uint32_t runs,total_score,total_kills,total_ticks,best_score,highest_hit;uint64_t distance,seen;uint32_t kills[64],points,last_seed,last_frame;uint16_t best_wave,rank,final_boss_kills,flags;};
    struct RecordV2 {uint32_t score,ticks,dealt,taken,kills,distance,highest;uint16_t wave,level,flags;uint8_t weapon,shield,body,reserved;};
    struct ProfileV2 {uint32_t magic,version,bytes,checksum;CareerV2 career;RecordV2 records[10];};
    ProfileV2 old_profile={};old_profile.magic=0x4e505246;old_profile.version=2;old_profile.bytes=sizeof(old_profile);old_profile.career.total_score=4321;old_profile.career.seen=uint64_t(1)<<63;old_profile.career.kills[63]=7;old_profile.career.distance=123456789012ull;old_profile.career.highest_hit=987;
    old_profile.checksum=crc(reinterpret_cast<const uint8_t*>(&old_profile.career),sizeof(old_profile)-offsetof(ProfileV2,career));write(root/"state"/"nNova.profile.tns",&old_profile,sizeof(old_profile));
    initialize(root);assert(app_test_game().state==Title);
    assert(app_test_career().total_score==4321&&app_test_career().enemySeen(63)&&!app_test_career().seen_enemies_hi&&app_test_career().enemy_kills[63]==7&&app_test_career().total_distance_q8==123456789012ull&&app_test_career().highest_hit==987);
    // New runs always enter the original loadout/challenge menu first.
    tap(ButtonConfirm);assert(app_test_page()==5&&app_test_game().state==Title);
    tap(ButtonConfirm);assert(app_test_game().state==Playing);
    {uint32_t cfg[14]={};std::ifstream file(root/"state"/"nNova.cfg.tns",std::ios::binary);file.read(reinterpret_cast<char*>(cfg),sizeof(cfg));assert(file&&cfg[1]==3&&cfg[3]==777&&cfg[8]==15&&cfg[10]==1);}
    Game& game=app_test_game();game.score=12345;
    // Physical 5 confirms menus only after the combat press has been released.
    Input held_five={};held_five.physical_valid=true;physical_set(held_five.physical_keys,Key5,true);
    frame(held_five);game.giveExperience(game.xp_next);assert(game.state==Upgrade&&game.banked_upgrades==1);
    frame(held_five);frame(held_five);assert(game.state==Upgrade&&game.banked_upgrades==1);
    frame();
    // Calculator 2 / 8 only navigate: no residual direct-select alias may spend
    // an upgrade, including fallback hosts which still send legacy Button2.
    unsigned selected=game.selected_offer,banked=game.banked_upgrades;
    Input digit_two={};digit_two.physical_valid=true;physical_set(digit_two.physical_keys,Key2,true);digit_two.held=ButtonDown|Button2;
    frame(digit_two);assert(game.state==Upgrade&&game.banked_upgrades==banked&&game.selected_offer==(selected+1)%game.offer_count);frame();
    Input digit_eight={};digit_eight.physical_valid=true;physical_set(digit_eight.physical_keys,Key8,true);digit_eight.held=ButtonUp|ButtonThrust;
    frame(digit_eight);assert(game.state==Upgrade&&game.banked_upgrades==banked&&game.selected_offer==selected);frame();
    tap(ButtonDown|Button2);assert(game.state==Upgrade&&game.banked_upgrades==banked);tap(ButtonUp);
    frame(held_five);assert(game.state==Playing&&game.banked_upgrades==0);frame();
    game.giveExperience(game.xp_next);assert(game.state==Upgrade&&game.banked_upgrades==1);
    tap(ButtonBack);assert(game.state==Playing&&game.banked_upgrades==1);
    tap(ButtonUpgrade);assert(game.state==Upgrade);
    tap(ButtonTab);assert(app_test_page()==7);tap(ButtonBack);assert(app_test_page()==0&&game.state==Upgrade);
    tap(ButtonConfirm);assert(game.state==Playing&&game.banked_upgrades==0);
    unsigned weapon=game.player.weapon;
    tap(ButtonPause);assert(game.state==Pause);
    tap(ButtonDown);tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);assert(game.state==Title);
    // Every original statistics category remains reachable without small text.
    app_test_set_page(8);frame();
    const unsigned stat_pages[]={3,2,3,4,1,2,1,1};
    for(unsigned tab=0;tab<8;++tab){assert(app_test_stats_tab()==tab&&app_test_stats_page()==0);
        for(unsigned sub=1;sub<stat_pages[tab];++sub){tap(ButtonDown);assert(app_test_stats_page()==sub);}
        tap(ButtonDown);assert(app_test_stats_page()==0);tap(ButtonRight);
    }
    app_test_set_page(0);frame();
    // Neither touchpad/mouse hover nor a stationary pointer owns menu focus.
    Input mouse={};mouse.pointer_present=true;mouse.pointer_x=90;mouse.pointer_y=205;
    frame(mouse);assert(app_test_selection()==0);
    mouse.held=ButtonDown;frame(mouse);mouse.held=0;frame(mouse);assert(app_test_selection()==1);
    tap(ButtonPointerMode);assert((app_test_flags()&64)!=0);
    frame(mouse);assert(app_test_selection()==1);
    mouse.held=ButtonUp;frame(mouse);mouse.held=0;frame(mouse);assert(app_test_selection()==0);
    tap(ButtonConfirm);assert(app_test_page()==20);tap(ButtonConfirm);assert(game.state==Playing&&game.score==12345);
    // Touch aim presses anywhere in the combat HUD fire, never open upgrade.
    game.banked_upgrades=1;frame();
    mouse.pointer_present=true;mouse.pointer_x=110;mouse.pointer_y=221;mouse.pointer_down=true;frame(mouse);
    assert(game.state==Playing);mouse.pointer_down=false;frame(mouse);game.banked_upgrades=0;
    tap(ButtonPause);tap(ButtonDown);tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);
    assert(game.state==Title);platform::shutdown();
    // A corrupted save must recover the last valid generation.
    {
        fs::path primary=root/"state"/"nNova.sav.tns";
        fs::copy_file(primary,root/"state"/"nNova.sav.tns.bak.tns",fs::copy_options::overwrite_existing);
        std::fstream corrupt(primary,std::ios::binary|std::ios::in|std::ios::out);assert(corrupt);
        corrupt.seekg(-1,std::ios::end);char value=0;corrupt.read(&value,1);value^=0x5a;
        corrupt.seekp(-1,std::ios::end);corrupt.write(&value,1);assert(corrupt);
    }
    initialize(root);tap(ButtonConfirm);assert(app_test_page()==20);tap(ButtonConfirm);assert(game.state==Playing&&game.score>=12345&&game.player.weapon==weapon);
    tap(ButtonPause);tap(ButtonDown);tap(ButtonConfirm);assert(app_test_page()==1);
    // Language changes leave the run untouched and persist in the new profile.
    if(fs::exists(root/"data"/"nNova.zhs.font.dat.tns")) {
        tap(ButtonRight);tap(ButtonRight);tap(ButtonConfirm);assert(app_test_page()==6);
        for(unsigned i=0;i<9;++i)tap(ButtonDown);
        tap(ButtonConfirm);assert(app_test_language()==9);
        assert(game.player.weapon==weapon&&game.state==Pause);tap(ButtonBack);
    }
    tap(ButtonBack);tap(ButtonBack);assert(game.state==Playing);
    game.player.invulnerable=0;game.player.shield=0;game.player.hull=1;game.damagePlayer(10000);assert(game.state==GameOver);frame();
    const unsigned completed=app_test_career().runs;assert(completed==1);frame();assert(app_test_career().runs==completed);
    assert(fs::exists(root/"state"/"nNova.profile.tns"));
    tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);assert(game.state==Title);
    tap(ButtonConfirm);assert(app_test_page()==5);
    for(unsigned i=0;i<9;++i)tap(ButtonDown);
    tap(ButtonConfirm);assert((app_test_flags()&16)==0); // Rank 0 cannot enable Wild.
    app_test_career().rank=14;tap(ButtonConfirm);
    assert((app_test_flags()&16)!=0&&!game.wild_mode);
    for(unsigned i=0;i<9;++i)tap(ButtonUp);
    tap(ButtonConfirm);
    assert(game.state==Playing&&game.score==0&&game.wild_mode);
    // Learned source content has independent navigation and returns correctly.
    tap(ButtonPause);for(unsigned i=0;i<3;++i)tap(ButtonDown);tap(ButtonConfirm);
    tap(ButtonDown);tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);assert(app_test_page()==10);
    tap(ButtonRight);frame();tap(ButtonBack);assert(app_test_page()==0);
    tap(ButtonDown);tap(ButtonConfirm);assert(app_test_page()==11);tap(ButtonConfirm);assert(app_test_page()==7);
    tap(ButtonBack);assert(app_test_page()==11);tap(ButtonBack);assert(app_test_page()==0);
    // Application-level rebinding persists and is reflected in combat input.
    app_test_set_page(16);frame();for(unsigned i=0;i<4;++i)tap(ButtonDown);tap(ButtonConfirm);
    Input physical={};physical.physical_valid=true;
    physical_set(physical.physical_keys,calculator_key(4,0),true);frame(physical);physical_set(physical.physical_keys,calculator_key(4,0),false);frame(physical);
    KeyBindings bindings;assert(load_key_bindings(bindings)&&bindings.key[BindFire]==calculator_key(4,0));
    platform::shutdown();initialize(root);assert(app_test_career().runs==completed);
    assert(load_key_bindings(bindings)&&bindings.key[BindFire]==calculator_key(4,0));
    app_test_set_page(0);game.start(987);game.state=Playing;frame();unsigned shots=game.run_stats.shots_fired;
    physical_set(physical.physical_keys,Key5,true);frame(physical);assert(game.run_stats.shots_fired==shots);
    physical_set(physical.physical_keys,Key5,false);frame(physical);physical_set(physical.physical_keys,calculator_key(4,0),true);frame(physical);assert(game.run_stats.shots_fired>shots);
    physical_set(physical.physical_keys,calculator_key(4,0),false);frame(physical);
    app_test_set_page(16);frame();for(unsigned i=0;i<9;++i)tap(ButtonDown);tap(ButtonConfirm);
    assert(load_key_bindings(bindings)&&bindings.key[BindFire]==Key5);
    // Original settings affect the live core, survive restart, and preserve
    // banked upgrades. Navigation crosses the newly paged settings rows.
    app_test_set_page(1);frame();tap(ButtonDown);tap(ButtonConfirm);
    assert(app_test_flags()&256); // Disable consecutive upgrading.
    app_test_set_page(0);game.state=Playing;game.giveExperience(game.xp_next);game.banked_upgrades=2;frame();tap(ButtonConfirm);
    assert(game.state==Playing&&game.banked_upgrades==1);
    app_test_set_page(1);frame();tap(ButtonRight);for(unsigned i=0;i<4;++i)tap(ButtonDown);tap(ButtonConfirm);
    assert(game.options.auto_deploy_mask==14);
    for(unsigned i=0;i<3;++i){tap(ButtonDown);tap(ButtonConfirm);}assert(game.options.auto_deploy_mask==0);
    tap(ButtonLeft);tap(ButtonDown);tap(ButtonDown);unsigned incoming=game.incomingDamagePercent();tap(ButtonConfirm);
    assert(app_test_page()==21);tap(ButtonDown);tap(ButtonConfirm);
    assert(game.options.assist_mode==1&&game.incomingDamagePercent()<incoming);
    platform::shutdown();initialize(root);
    assert(game.options.auto_deploy_mask==0&&game.options.assist_mode==1&&(app_test_flags()&256));
    {uint32_t cfg[14]={};std::ifstream file(root/"state"/"nNova.cfg.tns",std::ios::binary);file.read(reinterpret_cast<char*>(cfg),sizeof(cfg));assert(file&&cfg[1]==3&&cfg[8]==0&&cfg[10]==1);assert(cfg[13]==crc(reinterpret_cast<const uint8_t*>(cfg),52));}
    // Paused Only performs no periodic combat write; entering Pause saves.
    auto saved_bytes=[&](){std::ifstream file(root/"state"/"nNova.sav.tns",std::ios::binary);return std::string(std::istreambuf_iterator<char>(file),{});};
    const std::string before=saved_bytes();app_test_set_page(0);game.start(1234);game.next_wave=60000;game.player.invulnerable=60000;game.score=987654;
    frame();app_frame({},11000);app_render(framebuffer);assert(saved_bytes()==before);
    tap(ButtonPause);assert(saved_bytes()!=before);
    // Optional owner-supplied skin pack is bounded, checked and actually decoded.
    if(fs::exists(root/"data"/"nNova.skins.dat.tns")){
        static Skins custom;assert(custom.open()&&custom.count()>=1&&custom.select(1));
        assert(custom.image(0)&&custom.image(1)&&custom.image(0)->width<=72&&custom.image(0)->height<=72);
        assert(custom.jetAngle()!=0&&custom.jetDistance()!=0);assert(custom.select(0)&&!custom.image(0));
    }
    // UI-only victory states exercise the source controller, not a claimed
    // simulated boss kill. Combat/source boss death has separate core tests.
    platform::shutdown();initialize(root);app_test_set_page(0);game.start(67890);game.next_wave=60000;game.player.invulnerable=60000;
    frame();frame(held_five);game.state=GameOver;game.victory=true;frame(held_five);assert(app_test_page()==22);
    app_frame(held_five,2000);assert(app_test_page()==22); // old fire cannot skip
    frame();app_frame(held_five,400);assert(app_test_page()==22);app_frame(held_five,350);assert(app_test_page()==0&&game.state==GameOver);
    frame(held_five);assert(game.state==GameOver&&app_test_selection()==0); // no retry click-through
    unsigned victory_runs=app_test_career().runs;frame();assert(app_test_career().runs==victory_runs);
    tap(ButtonConfirm);assert(game.state==Playing);game.victory=true;game.state=GameOver;frame();assert(app_test_page()==22);
    app_frame({},37983);assert(app_test_page()==22);app_frame({},1);assert(app_test_page()==0&&game.state==GameOver);
    const unsigned automatic_runs=app_test_career().runs;frame();assert(app_test_career().runs==automatic_runs);
    // Daily calendar, complete rule details, deterministic retry, independent
    // best records and CRC backup recovery work through calculator navigation.
    app_test_career().rank=64;app_test_career().final_boss_kills=1;
    app_test_set_page(5);frame();for(unsigned i=0;i<12;++i){tap(ButtonDown);}tap(ButtonConfirm);
    assert(app_test_page()==23);const unsigned daily_day=app_test_daily_day();
    assert(daily_day>=18262&&daily_day<=47481);
    tap(ButtonTab);assert(app_test_page()==7);tap(ButtonBack);assert(app_test_page()==23);
    for(unsigned i=0;i<3;++i){tap(ButtonDown);}tap(ButtonConfirm);
    if(app_test_page()==4){tap(ButtonDown);tap(ButtonConfirm);}
    assert(app_test_page()==0&&(game.run_flags&RUN_DAILY)&&game.daily_day==daily_day&&game.seed==Game::dailySeedFor(daily_day));
    game.score=12345;game.state=GameOver;game.victory=false;frame();
    assert(app_test_daily_score(daily_day)==12345&&fs::exists(root/"state"/"nNova.daily.tns"));
    tap(ButtonConfirm);assert((game.run_flags&RUN_DAILY)&&game.daily_day==daily_day&&game.seed==Game::dailySeedFor(daily_day));
    app_test_set_page(23);frame();for(unsigned i=0;i<5;++i)tap(ButtonDown);tap(ButtonConfirm);
    assert(app_test_page()==24);tap(ButtonConfirm);assert(app_test_page()==23&&app_test_daily_day()==daily_day);
    {std::fstream file(root/"state"/"nNova.daily.tns",std::ios::in|std::ios::out|std::ios::binary);assert(file);file.put('!');}
    platform::shutdown();initialize(root);assert(app_test_daily_score(daily_day)==12345);
    platform::shutdown();fs::remove_all(root);
    std::puts("Application flow passed: loadout, banking, details, input ownership, combat pointer, CRC recovery, languages, death invalidation, wild mode, glossary, live settings, cfg migration, paused-only saving, optional skins, source ending timing and held-key skip isolation.");
}
