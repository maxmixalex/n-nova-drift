// Application-level regression uses a tiny, original synthetic pack in CI.
// Supplying a real data directory additionally exercises converted fonts/text.
#include "app.h"
#include "game.h"
#include "nnova/pack_format.h"
#include <cassert>
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
    fs::path root=fs::temp_directory_path()/"nnova-app-regression";
    fs::remove_all(root);make_pack(root/"data");
    if(argc>1) {
        for(const auto& entry:fs::directory_iterator(argv[1]))if(entry.path().extension()==".tns")
            fs::copy_file(entry.path(),root/"data"/entry.path().filename(),fs::copy_options::overwrite_existing);
    }
    initialize(root);
    assert(app_test_game().state==Title);
    tap(ButtonConfirm);assert(app_test_game().state==Playing);
    Game& game=app_test_game();game.score=12345;
    game.giveExperience(game.xp_next);assert(game.state==Upgrade&&game.banked_upgrades==1);
    tap(ButtonBack);assert(game.state==Playing&&game.banked_upgrades==1);
    tap(ButtonUpgrade);assert(game.state==Upgrade);
    tap(ButtonConfirm);assert(game.state==Playing&&game.banked_upgrades==0);
    unsigned weapon=game.player.weapon;
    tap(ButtonPause);assert(game.state==Pause);
    tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);assert(game.state==Title);
    // A stationary pointer must not continually undo keyboard navigation.
    Input mouse={};mouse.pointer_present=true;mouse.pointer_x=90;mouse.pointer_y=205;
    frame(mouse);assert(app_test_selection()==4);
    mouse.pressed=ButtonUp;mouse.held=ButtonUp;frame(mouse);
    mouse.pressed=mouse.held=0;frame(mouse);assert(app_test_selection()==3);
    tap(ButtonBack);assert(app_quit());platform::shutdown();
    // Corruption with the right length must recover the last valid generation.
    {
        fs::path primary=root/"state"/"nNova.sav.tns";
        fs::copy_file(primary,root/"state"/"nNova.sav.tns.bak.tns",fs::copy_options::overwrite_existing);
        std::fstream corrupt(primary,std::ios::binary|std::ios::in|std::ios::out);assert(corrupt);
        corrupt.seekg(-1,std::ios::end);char value=0;corrupt.read(&value,1);value^=0x5a;
        corrupt.seekp(-1,std::ios::end);corrupt.write(&value,1);assert(corrupt);
    }
    initialize(root);tap(ButtonConfirm);
    assert(game.state==Playing&&game.score==12345&&game.player.weapon==weapon);
    // Pause -> settings: Wild is for new runs; changing it must not rewrite this run.
    tap(ButtonPause);tap(ButtonDown);tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);
    assert(app_test_page()==1);
    tap(ButtonDown);tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);
    assert((app_test_flags()&16)!=0&&!game.wild_mode);
    if(fs::exists(root/"data"/"nNova.zhs.font.dat.tns")) {
        tap(ButtonDown);tap(ButtonConfirm);assert((app_test_flags()&8)!=0);
    }
    tap(ButtonBack);tap(ButtonBack);assert(game.state==Playing);
    game.player.invulnerable=0;game.player.shield=0;game.player.hull=1;
    game.damagePlayer(10000);assert(game.state==GameOver);frame();
    tap(ButtonDown);tap(ButtonConfirm);assert(game.state==Title);
    // After a death the first item starts a new game, rather than reviving a backup.
    tap(ButtonConfirm);assert(game.state==Playing&&game.score==0&&game.wild_mode);
    platform::shutdown();
    fs::remove_all(root);
    std::puts("Application flow passed: input, banking, save/resume, CRC backup recovery, mode settings, death invalidation.");
}
