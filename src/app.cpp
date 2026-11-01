#include "app.h"
#include "assets.h"
#include "font.h"
#include "game.h"
#include "localization.h"
#include "text_zh.h"
#include "render.h"
#include <stddef.h>

namespace nova {
namespace {
enum Page { HomePage,SettingsPage,ControlsPage,BuildPage,NewRunPage };
enum { AutoFire=1,FullEffects=2,ShowFps=4,Chinese=8,Wild=16 };
struct Preferences {
    uint32_t magic,version,flags,high_score,best_wave,runs,crc;
};
struct Snapshot {
    char magic[8];
    uint32_t version,bytes,valid,crc;
    Game game;
};
static Assets assets;
static Font font;
static Locale language;
static Game game;
static Renderer renderer;
static Preferences preferences;
static Snapshot snapshot;
static Page page,return_page;
static bool ready,quitting,has_save,gameover_recorded,pointer_visible;
static bool pointer_down,pointer_activity,pointer_steering,pending_ability;
static int selection,build_selection;
static int pointer_x,pointer_y;
static uint32_t clock_ms,autosave_ms,fps_ms,fps_frames,fps_value,toast_until;
static const char* toast;
static uint32_t demo_ticks;
static const uint16_t White=rgb(224,240,252),Muted=rgb(125,153,177);
static const uint16_t Cyan=rgb(83,210,245),Purple=rgb(172,128,245),Red=rgb(255,113,137);

static uint32_t crc32(const void* data,unsigned size) {
    const uint8_t* p=static_cast<const uint8_t*>(data);
    uint32_t crc=0xffffffffu;
    for(unsigned i=0;i<size;++i) {
        crc^=p[i];
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
}
static void bytes(void* dest,const void* source,unsigned size) {
    uint8_t* d=static_cast<uint8_t*>(dest);const uint8_t* s=static_cast<const uint8_t*>(source);
    for(unsigned i=0;i<size;++i)d[i]=s[i];
}
static bool chinese() { return (preferences.flags&Chinese)!=0 && font.ready(); }
static const char* tr(const char* english,const char* chinese_text) { return chinese()?chinese_text:english; }
static const char* item_description(const Offer& offer) {
    return chinese()?chineseDescription(offer):offerDescription(offer);
}
static const char* item_name(const Offer& offer,char* buffer,unsigned capacity) {
    if(!chinese()||!language.ready())return offerName(offer);
    const char* weaponKeys[WEAPON_COUNT]={"Standard","SplitShot","Flak","Railgun","Grenade","Salvo","Vortex","Pulse","Torrent","BladeDrone","Dart","ThermalLance","Swords"};
    const char* bodyKeys[BODY_COUNT]={"Standard","Assault","Hullbreaker","Stealth","Firefly","Sentinel","Engineer","Research","Battery","Architect","Viper","Carrier","Courser","Leviathan"};
    const char* shieldKeys[SHIELD_COUNT]={"Standard","Halo","Reflect","Bastion","Temporal","Amp","Warp","Helix","Siphon","Shockwave"};
    char key[96]={0};
    if(offer.kind==OFFER_MOD&&offer.id<MOD_COUNT){append(key,sizeof(key),"ModTitle");append(key,sizeof(key),mods[offer.id].source_key);}
    else if(offer.kind==OFFER_WEAPON&&offer.id<WEAPON_COUNT){append(key,sizeof(key),"GearTitleWeapon");append(key,sizeof(key),weaponKeys[offer.id]);}
    else if(offer.kind==OFFER_BODY&&offer.id<BODY_COUNT){append(key,sizeof(key),"GearTitleBody");append(key,sizeof(key),bodyKeys[offer.id]);}
    else if(offer.kind==OFFER_SHIELD&&offer.id<SHIELD_COUNT){append(key,sizeof(key),"GearTitleShield");append(key,sizeof(key),shieldKeys[offer.id]);}
    const char* value=language.get(key);
    if(!value)return offerName(offer);
    Locale::plain(buffer,capacity,value,true);return buffer;
}
static void notice(const char* message) { toast=message;toast_until=clock_ms+2500; }
static bool save_preferences() {
    preferences.crc=crc32(&preferences,offsetof(Preferences,crc));
    return platform::write_state("nNova.cfg.tns",&preferences,sizeof(preferences));
}
static bool save_run(bool valid=true) {
    const char magic[8]={'N','N','O','V','A','S','1',0};
    bytes(snapshot.magic,magic,8);
    snapshot.version=1;snapshot.bytes=sizeof(Game);snapshot.valid=valid?1:0;
    bytes(&snapshot.game,&game,sizeof(Game));
    if(snapshot.game.state==Pause)snapshot.game.state=Playing;
    snapshot.game.resetInput();
    snapshot.crc=crc32(&snapshot.game,sizeof(Game));
    bool ok=platform::write_state("nNova.sav.tns",&snapshot,sizeof(snapshot));
    if(ok)has_save=valid;
    else notice(tr("Could not save this run.","本局存档写入失败。"));
    autosave_ms=0;
    return ok;
}
static unsigned snapshot_status() {
    const char magic[8]={'N','N','O','V','A','S','1',0};
    for(unsigned i=0;i<8;++i)if(snapshot.magic[i]!=magic[i])return 0;
    if(snapshot.version!=1||snapshot.bytes!=sizeof(Game))return 0;
    // A valid death marker must never fall through to an earlier living run.
    if(snapshot.valid==0)return 2;
    return snapshot.valid==1&&
        snapshot.crc==crc32(&snapshot.game,sizeof(Game))&&snapshot.game.validState()&&
        (snapshot.game.state==Playing||snapshot.game.state==Upgrade||snapshot.game.state==Pause);
}
static bool load_run() {
    if(platform::read_state("nNova.sav.tns",&snapshot,sizeof(snapshot))) {
        unsigned status=snapshot_status();
        if(status)return status==1;
    }
    return platform::read_state("nNova.sav.tns.bak.tns",&snapshot,sizeof(snapshot))&&snapshot_status()==1;
}
static void record_gameover() {
    if(game.state!=GameOver||gameover_recorded)return;
    gameover_recorded=true;selection=0;
    if(game.score>preferences.high_score)preferences.high_score=game.score;
    if(game.wave>preferences.best_wave)preferences.best_wave=game.wave;
    save_preferences();save_run(false);has_save=false;
}
static void resume_saved() {
    if(!load_run()) { has_save=false;notice(tr("Save is missing or incompatible.","存档不存在或版本不兼容。"));return; }
    bytes(&game,&snapshot.game,sizeof(Game));
    if(game.state==Pause)game.state=Playing;
    game.resetInput();game.auto_fire=(preferences.flags&AutoFire)!=0;
    gameover_recorded=false;page=HomePage;selection=0;autosave_ms=0;
}
static void start_run() {
    game.start(platform::host_demo()?0x4e4f5641u:platform::seed()^clock_ms);
    game.wild_mode=(preferences.flags&Wild)!=0;
    game.auto_fire=(preferences.flags&AutoFire)!=0;
    gameover_recorded=false;page=HomePage;selection=0;autosave_ms=0;pending_ability=false;
    ++preferences.runs;save_preferences();save_run();
}
static void request_new_run() {
    if(has_save){page=NewRunPage;selection=0;}
    else start_run();
}
static bool hit(int x,int y,int w,int h) {
    return pointer_visible&&pointer_x>=x&&pointer_x<x+w&&pointer_y>=y&&pointer_y<y+h;
}
static bool pressed(const Input& input,uint32_t button) {return (input.pressed&button)!=0;}
static void move_menu(const Input& input,int count,int y,int height,int x=65,int width=190) {
    if(pressed(input,ButtonUp)){selection=(selection+count-1)%count;}
    if(pressed(input,ButtonDown)){selection=(selection+1)%count;}
    if(pointer_activity)for(int i=0;i<count;++i)if(hit(x,y+i*height,width,height))selection=i;
}
static bool activate(const Input& input,int y,int height,int x=65,int width=190) {
    return pressed(input,ButtonConfirm)||(input.pointer_pressed&&hit(x,y+selection*height,width,height));
}
static void return_from_page() {
    page=return_page;selection=0;
}
static void process_settings(const Input& in) {
    move_menu(in,6,52,27,28,264);
    if(pressed(in,ButtonBack)||pressed(in,ButtonPause)){return_from_page();return;}
    if(!activate(in,52,27,28,264)&&!pressed(in,ButtonLeft)&&!pressed(in,ButtonRight))return;
    if(selection==5){return_from_page();return;}
    unsigned flag=selection==0?AutoFire:selection==1?FullEffects:selection==2?ShowFps:selection==3?Wild:Chinese;
    preferences.flags^=flag;
    if(flag==Chinese) {
        if(!font.open((preferences.flags&Chinese)!=0)) {
            preferences.flags&=~Chinese;font.open(false);
            notice("Language font not found in data/.");
        }
        language.open((preferences.flags&Chinese)!=0);
    }
    game.auto_fire=(preferences.flags&AutoFire)!=0;
    if(!save_preferences())notice(tr("Settings could not be saved.","设置保存失败。"));
}
static int build_count() {
    int count=3;for(unsigned i=0;i<MOD_COUNT;++i)if(game.player.mods[i])++count;return count;
}
static Offer build_offer(int index) {
    if(index==0)return Offer{OFFER_WEAPON,game.player.weapon};
    if(index==1)return Offer{OFFER_SHIELD,game.player.shield_type};
    if(index==2)return Offer{OFFER_BODY,game.player.body};
    index-=3;
    for(unsigned i=0;i<MOD_COUNT;++i)if(game.player.mods[i]){
        if(index==0)return Offer{OFFER_MOD,uint8_t(i)};
        --index;
    }
    return Offer{OFFER_WEAPON,0};
}
static void process_build(const Input& in) {
    int count=build_count();
    if(pressed(in,ButtonUp))build_selection=(build_selection+count-1)%count;
    if(pressed(in,ButtonDown))build_selection=(build_selection+1)%count;
    if(pressed(in,ButtonLeft)){build_selection-=6;if(build_selection<0)build_selection=0;}
    if(pressed(in,ButtonRight)){build_selection+=6;if(build_selection>=count)build_selection=count-1;}
    int first=(build_selection/6)*6;
    if(pointer_activity)for(int i=0;i<6&&first+i<count;++i)
        if(hit(12,42+i*23,296,23))build_selection=first+i;
    if(pressed(in,ButtonBack)||pressed(in,ButtonPause)||
       (in.pointer_pressed&&hit(246,218,70,22)))return_from_page();
}
static void process_upgrade(const Input& in) {
    int old=selection;selection=game.selected_offer;
    move_menu(in,3,49,36,12,296);
    game.selected_offer=uint8_t(selection);selection=old;
    if(pressed(in,ButtonReroll)||(in.pointer_pressed&&hit(227,23,82,22))) {
        if(!game.rerollOffers())notice(tr("No rerolls left.","重掷次数不足。"));
    }
    int chosen=-1;
    if(pressed(in,Button1))chosen=0;
    if(pressed(in,Button2))chosen=1;
    if(pressed(in,Button3))chosen=2;
    if(pressed(in,ButtonConfirm))chosen=game.selected_offer;
    if(in.pointer_pressed&&hit(12,49+int(game.selected_offer)*36,296,34))chosen=game.selected_offer;
    if(chosen>=0 && game.chooseOffer(unsigned(chosen)))save_run();
    if(pressed(in,ButtonPause)||pressed(in,ButtonBack)||pressed(in,ButtonMenu)||pressed(in,ButtonUpgrade)) {
        if(game.bankUpgrade()){pending_ability=false;save_run();}
    }
}
static void process_pause(const Input& in) {
    if(pressed(in,ButtonUpgrade)&&game.banked_upgrades) {
        game.state=Playing;game.reopenUpgrade();selection=0;return;
    }
    move_menu(in,5,65,27,50,220);
    if(pressed(in,ButtonBack)||pressed(in,ButtonPause)||pressed(in,ButtonMenu)) {
        game.state=Playing;game.resetInput();selection=0;return;
    }
    if(!activate(in,65,27,50,220))return;
    if(selection==0){game.state=Playing;game.resetInput();selection=0;}
    else if(selection==1){page=BuildPage;return_page=HomePage;build_selection=0;}
    else if(selection==2){if(save_run()){game.state=Title;selection=0;}}
    else if(selection==3){page=SettingsPage;return_page=HomePage;selection=0;}
    else {if(save_run())quitting=true;}
}
static void process_home(const Input& in) {
    int count=has_save?5:4;
    move_menu(in,count,110,22,64,192);
    if(pressed(in,ButtonBack)){quitting=true;return;}
    if(!activate(in,110,22,64,192))return;
    int action=selection-(has_save?1:0);
    if(has_save&&selection==0)resume_saved();
    else if(action==0)request_new_run();
    else if(action==1){page=SettingsPage;return_page=HomePage;selection=0;}
    else if(action==2){page=ControlsPage;return_page=HomePage;selection=0;}
    else quitting=true;
}
static void process_gameover(const Input& in) {
    move_menu(in,3,135,26,55,210);
    if(pressed(in,ButtonBack)){game.state=Title;selection=0;return;}
    if(!activate(in,135,26,55,210))return;
    if(selection==0)start_run();
    else if(selection==1){game.state=Title;selection=0;}
    else quitting=true;
}
static void demo_frame(uint32_t elapsed) {
    if(game.state==Title){start_run();return;}
    if(game.state==GameOver){start_run();return;}
    if(game.state==Upgrade){game.chooseOffer((demo_ticks/90)%3);return;}
    GameInput in={};in.up=true;in.fire=true;
    int target=-1,best=0x7fffffff;
    for(unsigned i=0;i<MAX_ENEMIES;++i)if(game.enemies[i].active) {
        int dx=pixel(game.enemies[i].x-game.player.x),dy=pixel(game.enemies[i].y-game.player.y);
        int distance=dx*dx+dy*dy;
        if(distance<best){best=distance;target=int(i);}
    }
    if(target>=0) {in.pointer_valid=true;in.pointer_x=int16_t(pixel(game.enemies[target].x));
        in.pointer_y=int16_t(pixel(game.enemies[target].y));}
    else in.right=(demo_ticks/60)%2==0;
    if(game.player.weapon==W_SALVO||game.player.weapon==W_VORTEX)in.fire=(demo_ticks%90)>20;
    in.ability=demo_ticks%150==0;
    game.advance(elapsed,in);++demo_ticks;
}
static void panel(int x,int y,int w,int h) {
    renderer.canvas.shade(x,y,w,h,rgb(4,12,25),235);
    renderer.canvas.box(x,y,w,h,rgb(35,68,96));
}
static void button(int x,int y,int w,int h,const char* title,bool selected) {
    Canvas& c=renderer.canvas;
    if(selected) {
        c.fill(x,y,w,h,rgb(15,45,64));
        c.fill(x,y,2,h,Cyan);
        c.line(x+7,y+h/2-3,x+10,y+h/2,Cyan);
        c.line(x+10,y+h/2,x+7,y+h/2+3,Cyan);
    }
    c.text(x+17,y+(h-Canvas::lineHeight())/2,title,selected?White:Muted);
}
static void heading(const char* title) {
    renderer.canvas.centered(15,title,White);
    renderer.canvas.fill(16,37,288,1,rgb(35,68,96));
}
static void footer(const char* text) {
    renderer.canvas.text(10,222,text,Muted);
}
static void render_home() {
    renderer.titleArt(clock_ms);
    Canvas::setFont(&font);
    const char* items[5];int count=0;
    if(has_save)items[count++]=tr("CONTINUE","继续游戏");
    items[count++]=tr("NEW RUN","开始新游戏");items[count++]=tr("SETTINGS","设置");
    items[count++]=tr("CONTROLS","操作说明");items[count++]=tr("QUIT","退出");
    for(int i=0;i<count;++i)button(64,110+i*22,192,21,items[i],i==selection);
    char high[40]="BEST  ";appendNumber(high,sizeof(high),preferences.high_score);
    Canvas::setFont(0);renderer.canvas.text(8,228,"nNova 0.1",Muted);renderer.canvas.right(312,228,high,Muted);
}
static void render_settings() {
    Canvas& c=renderer.canvas;heading(tr("SETTINGS","设置"));
    const char* labels[]={tr("Automatic fire","自动开火"),tr("Visual effects","视觉效果"),
        tr("Frame counter","显示帧率"),tr("New run mode","新局模式"),tr("Language","语言"),tr("Back","返回")};
    for(int i=0;i<6;++i) {
        button(28,52+i*27,264,25,labels[i],selection==i);
        if(i<5) {
            const char* value=i==0?((preferences.flags&AutoFire)?tr("ON","开启"):tr("OFF","关闭")):
                i==1?((preferences.flags&FullEffects)?tr("FULL","完整"):tr("REDUCED","精简")):
                i==2?((preferences.flags&ShowFps)?tr("ON","开启"):tr("OFF","关闭")):
                i==3?((preferences.flags&Wild)?tr("WILD","狂野"):tr("CLASSIC","经典")):
                ((preferences.flags&Chinese)?"简体中文":"English");
            c.right(280,52+i*27+5,value,selection==i?Cyan:Muted);
        }
    }
    footer(tr("Enter: change    Esc: back","Enter：切换    Esc：返回"));
}
static void render_controls() {
    Canvas& c=renderer.canvas;heading(tr("CONTROLS","操作说明"));
    const char* left[]={"8 / W / X","4 / 6 (A / D)","2 / S","SPACE / Z","TAB","P / ESC","U","1 / 2 / 3","R"};
    const char* right[]={tr("Thrust","推进"),tr("Turn","转向"),tr("Brake","刹车"),tr("Fire / release charge","射击／释放蓄力"),
        tr("Dash / Blink","冲刺／闪现"),tr("Pause","暂停"),tr("Bank / spend upgrades","保留／使用升级"),tr("Choose upgrade","选择升级"),tr("Reroll upgrades","重掷升级")};
    for(int i=0;i<9;++i){c.text(19,46+i*16,left[i],Cyan);c.text(133,46+i*16,right[i],White);}
    c.wrapped(19,190,282,2,tr("Touchpad: point to aim; click to fire or select. Hold W / 8 / X to thrust.",
        "触摸板移动光标瞄准，按下开火或选择；按住 W、8 或 X 推进。"),Muted);
    footer(tr("Esc: back","Esc：返回"));
}
static void render_upgrade() {
    Canvas& c=renderer.canvas;panel(5,7,310,226);
    c.text(15,13,tr("EVOLVE","进化"),White);
    char level[30]="LEVEL ";appendNumber(level,sizeof(level),game.level);c.right(304,13,level,Purple);
    char rr[30]="R: ";appendNumber(rr,sizeof(rr),game.rerolls);
    c.right(303,31,rr,Cyan);c.text(15,31,tr("Choose one upgrade","选择一项升级"),Muted);
    for(unsigned i=0;i<3;++i) {
        int y=49+int(i)*36;
        if(i==game.selected_offer){c.fill(12,y,296,34,rgb(17,41,63));c.box(12,y,296,34,Cyan);}
        renderer.icon(game.offers[i],33,y+17,28);
        char name[128];
        c.text(54,y+2,item_name(game.offers[i],name,sizeof(name)),i==game.selected_offer?White:Muted);
        Canvas::setFont(0);c.text(54,y+21,offerCategory(game.offers[i]),Purple);
        char number[2]={char('1'+i),0};c.right(299,y+21,number,Cyan);Canvas::setFont(&font);
    }
    c.fill(16,161,288,1,rgb(35,68,96));
    c.wrapped(16,166,288,3,item_description(game.offers[game.selected_offer]),White);
    Canvas::setFont(0);c.text(16,221,"ENTER: CHOOSE  R: REROLL  ESC: BANK",Muted);
}
static void render_pause() {
    panel(40,23,240,192);renderer.canvas.centered(37,tr("PAUSED","已暂停"),White);
    renderer.canvas.fill(54,56,212,1,rgb(35,68,96));
    const char* items[]={tr("Resume","继续"),tr("Your build","查看构筑"),tr("Save and menu","保存并返回主菜单"),
        tr("Settings","设置"),tr("Save and quit","保存并退出")};
    for(int i=0;i<5;++i)button(50,65+i*27,220,25,items[i],selection==i);
}
static void render_build() {
    Canvas& c=renderer.canvas;heading(tr("YOUR BUILD","当前构筑"));
    int count=build_count(),first=(build_selection/6)*6;
    for(int i=0;i<6&&first+i<count;++i) {
        int at=first+i,y=42+i*23;Offer item=build_offer(at);
        if(at==build_selection)c.fill(12,y,296,22,rgb(17,41,63));
        char name[128];
        renderer.icon(item,25,y+11,19);c.text(43,y+3,item_name(item,name,sizeof(name)),at==build_selection?White:Muted);
        if(item.kind==OFFER_MOD&&game.player.mods[item.id]>1) {
            char rank[12]="x";appendNumber(rank,sizeof(rank),game.player.mods[item.id]);c.right(298,y+3,rank,Cyan);
        }
    }
    c.fill(16,184,288,1,rgb(35,68,96));
    c.wrapped(16,188,288,2,item_description(build_offer(build_selection)),White);
    Canvas::setFont(0);
    char position[30]={0};appendNumber(position,sizeof(position),unsigned(build_selection+1));append(position,sizeof(position)," / ");appendNumber(position,sizeof(position),unsigned(count));
    c.text(16,226,position,Muted);c.right(304,226,"ESC: BACK",Cyan);
}
static void render_gameover() {
    Canvas& c=renderer.canvas;panel(34,22,252,196);
    c.centered(35,tr("RUN ENDED","本局结束"),White);
    char score[30];decimal(score,game.score);c.centered(58,score,Cyan,2);
    char details[64]="WAVE ";appendNumber(details,sizeof(details),game.wave);append(details,sizeof(details),"    LEVEL ");appendNumber(details,sizeof(details),game.level);
    c.centered(94,details,Muted);
    const char* items[]={tr("Try again","重新开始"),tr("Main menu","主菜单"),tr("Quit","退出")};
    for(int i=0;i<3;++i)button(55,135+i*26,210,24,items[i],selection==i);
}
}

bool app_init(const char* base_path) {
    quitting=false;clock_ms=0;autosave_ms=0;fps_ms=0;fps_frames=0;fps_value=0;
    page=HomePage;return_page=HomePage;selection=0;pointer_visible=false;toast=0;toast_until=0;demo_ticks=0;
    preferences=Preferences{0x4e4f5641u,1,FullEffects,0,0,0,0};
    Preferences loaded={};
    bool profile_ok=platform::read_state("nNova.cfg.tns",&loaded,sizeof(loaded))&&loaded.magic==preferences.magic&&loaded.version==1&&
       loaded.crc==crc32(&loaded,offsetof(Preferences,crc));
    if(!profile_ok)profile_ok=platform::read_state("nNova.cfg.tns.bak.tns",&loaded,sizeof(loaded))&&loaded.magic==preferences.magic&&loaded.version==1&&
       loaded.crc==crc32(&loaded,offsetof(Preferences,crc));
    if(profile_ok) {
        preferences=loaded;preferences.flags&=31;
    }
    ready=assets.open(base_path);
    if(!font.open((preferences.flags&Chinese)!=0)){preferences.flags&=~Chinese;font.open(false);}
    language.open((preferences.flags&Chinese)!=0);
    game.start(1);game.state=Title;gameover_recorded=false;
    has_save=load_run();
    renderer.initialize(&assets);
    return ready;
}
void app_frame(const Input& in,uint32_t elapsed_ms) {
    clock_ms+=elapsed_ms;fps_ms+=elapsed_ms;++fps_frames;
    if(fps_ms>=1000){fps_value=fps_frames*1000/fps_ms;fps_ms=0;fps_frames=0;}
    pointer_activity=in.pointer_present&&(!pointer_visible||pointer_x!=in.pointer_x||pointer_y!=in.pointer_y||in.pointer_pressed);
    if(pointer_activity)pointer_steering=true;
    if(in.held&(ButtonLeft|ButtonRight))pointer_steering=false;
    pointer_visible=in.pointer_present;pointer_x=in.pointer_x;pointer_y=in.pointer_y;pointer_down=in.pointer_down;
    if(ready&&!assets.ready())ready=false;
    if(in.quit) {
        if(game.state==Playing||game.state==Pause||game.state==Upgrade)save_run();
        quitting=true;return;
    }
    if(!ready){if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm))quitting=true;return;}
    if(platform::host_demo()){demo_frame(elapsed_ms);return;}
    if(page==SettingsPage){process_settings(in);return;}
    if(page==ControlsPage){if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)||in.pointer_pressed)return_from_page();return;}
    if(page==BuildPage){process_build(in);return;}
    if(page==NewRunPage) {
        move_menu(in,2,132,28,60,200);
        if(pressed(in,ButtonBack)){page=HomePage;selection=0;return;}
        if(activate(in,132,28,60,200)){if(selection==1)start_run();else{page=HomePage;selection=0;}}
        return;
    }
    if(game.state==Title){process_home(in);return;}
    if(game.state==Pause){process_pause(in);return;}
    if(game.state==Upgrade){process_upgrade(in);return;}
    if(game.state==GameOver){record_gameover();process_gameover(in);return;}
    if(game.banked_upgrades&&(pressed(in,ButtonUpgrade)||(in.pointer_pressed&&hit(46,218,135,18)))) {
        game.reopenUpgrade();pending_ability=false;save_run();return;
    }
    if(pressed(in,ButtonPause)||pressed(in,ButtonBack)||pressed(in,ButtonMenu)) {
        game.state=Pause;selection=0;save_run();return;
    }
    GameInput control={};
    control.up=(in.held&(ButtonUp|ButtonThrust))!=0;
    control.down=(in.held&ButtonDown)!=0;control.left=(in.held&ButtonLeft)!=0;
    control.right=(in.held&ButtonRight)!=0;control.fire=(in.held&ButtonFire)!=0||in.pointer_down;
    pending_ability=pending_ability||pressed(in,ButtonTab);
    control.ability=pending_ability;
    control.pointer_valid=in.pointer_present&&pointer_steering;control.pointer_down=in.pointer_down;
    control.pointer_x=int16_t(in.pointer_x*game.worldWidth()/ScreenWidth);
    control.pointer_y=int16_t(in.pointer_y*game.worldHeight()/ScreenHeight);
    uint32_t previous_frame=game.frame;
    game.advance(elapsed_ms,control);
    if(game.frame!=previous_frame)pending_ability=false;
    autosave_ms+=elapsed_ms;
    if(game.state==GameOver)record_gameover();
    else if(autosave_ms>=10000 || game.state==Upgrade)save_run();
}
void app_render(uint16_t* framebuffer) {
    renderer.bind(framebuffer);Canvas& c=renderer.canvas;Canvas::setFont(&font);
    if(ready&&!assets.ready())ready=false;
    if(!ready) {
        c.clear(rgb(4,12,25));Canvas::setFont(0);
        c.centered(30,"NOVA DRIFT",White,2);
        c.centered(76,"DATA FILES NOT FOUND",Red);
        c.wrapped(20,100,280,4,assets.error(),White);
        c.wrapped(20,152,280,4,"Copy the complete nNova folder. Keep nNova.tns next to data/ and state/.",Muted);
        c.centered(215,"ENTER / ESC: EXIT",Cyan);return;
    }
    if(game.state==Title)renderer.background(clock_ms,true);
    else renderer.world(game,clock_ms,(preferences.flags&FullEffects)!=0);
    Canvas::setFont(&font);
    if(page==SettingsPage){renderer.background(clock_ms,false);render_settings();}
    else if(page==ControlsPage){renderer.background(clock_ms,false);render_controls();}
    else if(page==BuildPage){renderer.background(clock_ms,false);render_build();}
    else if(page==NewRunPage) {
        panel(42,60,236,129);c.centered(74,tr("START A NEW RUN?","开始新游戏？"),White);
        c.centered(98,tr("The saved run will be replaced.","当前存档将被替换。"),Muted);
        button(60,132,200,26,tr("Cancel","取消"),selection==0);
        button(60,160,200,26,tr("Start new run","开始新游戏"),selection==1);
    } else if(game.state==Title)render_home();
    else if(game.state==Pause)render_pause();
    else if(game.state==Upgrade)render_upgrade();
    else if(game.state==GameOver)render_gameover();
    if((preferences.flags&ShowFps)&&game.state==Playing) {
        Canvas::setFont(0);char fps[20]={0};appendNumber(fps,sizeof(fps),fps_value);append(fps,sizeof(fps)," FPS");c.text(182,225,fps,Muted);
    }
    if(toast && clock_ms<toast_until) {
        Canvas::setFont(&font);c.shade(5,204,310,18,rgb(2,9,19),245);c.centered(205,toast,White);
    }
    if(pointer_visible)renderer.pointer(pointer_x,pointer_y,pointer_down);
}
bool app_quit() {return quitting;}
#ifdef NOVA_TEST
Game& app_test_game() {return game;}
unsigned app_test_page() {return unsigned(page);}
unsigned app_test_selection() {return unsigned(selection);}
unsigned app_test_flags() {return preferences.flags;}
#endif
}
