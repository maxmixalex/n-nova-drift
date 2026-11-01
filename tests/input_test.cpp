#include "input_controller.h"
#include "system_locale.h"
#include "daily_calendar.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <initializer_list>
using namespace nova;

static Input keys(uint32_t held) {Input in{};in.held=held;return in;}
static Input pad(int x,int y,bool down=true) {
    Input in{};in.pointer_is_touchpad=true;in.pointer_present=true;
    in.pointer_x=int16_t(x);in.pointer_y=int16_t(y);in.pointer_down=down;return in;
}
static void release(InputController& c,unsigned context=1) {c.update({},context);}
static void matrix() {
    uint16_t rows[8]={};
    rows[6]=0x040;rows[5]=0x040;rows[2]=0x080;
    assert(decode_calculator_keys(rows)==(ButtonUp|ButtonThrust|ButtonFire|ButtonLeft));
    std::memset(rows,0,sizeof rows);rows[3]=0x088;rows[0]=0x080;rows[5]=0x004;
    assert(decode_calculator_keys(rows)==(ButtonTab|ButtonUpgrade|ButtonPointerMode|ButtonReroll));
    std::memset(rows,0,sizeof rows);rows[6]=0x0b8;
    assert(decode_calculator_keys(rows)==(ButtonDown|ButtonMenu|ButtonBack));
    // Clickpad arrow and click positions are NOT independent CX directions.
    std::memset(rows,0,sizeof rows);rows[7]=0xff;rows[6]=0x002;rows[5]=0x400;
    assert(decode_calculator_keys(rows)==0);
    rows[0]=3;assert(decode_calculator_keys(rows)==ButtonConfirm);
}
static void launch_and_transition() {
    InputController c;
    assert(!c.update(keys(ButtonConfirm),1).pressed); // launcher Enter still held
    assert(!c.update(keys(ButtonConfirm),1).held);
    release(c);assert(c.update(keys(ButtonConfirm),1).pressed==ButtonConfirm);
    // A held Enter cannot confirm a newly entered page, even if a caller
    // incorrectly supplies a repeated pressed bit on every poll.
    Input repeated=keys(ButtonConfirm);repeated.pressed=ButtonConfirm;
    assert(!c.update(repeated,2).held);assert(!c.update(repeated,2).pressed);
    release(c,2);assert(c.update(repeated,2).pressed==ButtonConfirm);
    release(c,2);
    assert(c.update(keys(ButtonFire|ButtonThrust),2).held==(ButtonFire|ButtonThrust));
    Input next=c.update(keys(ButtonFire|ButtonThrust|ButtonDown),3);
    assert(next.held==ButtonDown&&next.pressed==ButtonDown); // truly new key works
    next=c.update(keys(ButtonFire|ButtonThrust),3);assert(next.held==0);
    release(c,3);assert(c.update(keys(ButtonFire),3).pressed==ButtonFire);
}
static void keypad_ownership() {
    InputController c;release(c);
    Input out=c.update(pad(300,225,false),1);
    assert(!out.pointer_present&&!out.held&&!c.pointerActivity()); // hover ignored
    out=c.update(pad(160,0),1);
    assert(out.held==(ButtonUp|ButtonThrust)&&!out.pointer_down&&!out.pointer_pressed);
    out=c.update(pad(160,120),1);
    assert(out.held==(ButtonUp|ButtonThrust)&&!out.pressed); // sector remains latched
    out=c.update(pad(160,120),2);assert(!out.held); // held pad cannot enter next page
    out=c.update(pad(160,120),2);assert(!out.held);
    c.update(pad(160,120,false),2);
    out=c.update(pad(160,120),2);assert(out.pressed==ButtonConfirm);
    release(c,2);
    out=c.update(pad(319,239),2);
    assert(out.held==(ButtonRight|ButtonDown)&&!(out.held&ButtonConfirm));
    release(c,2);
    Input mouse=pad(160,0);mouse.pointer_is_touchpad=false;
    out=c.update(mouse,2);assert(!out.held&&!out.pointer_down); // explicit mode for mouse too
}
static void pointer_ownership() {
    InputController c;release(c);
    c.update(keys(ButtonPointerMode),1);assert(c.pointerMode());release(c);
    Input out=c.update(pad(160,0,false),1);
    assert(out.pointer_present&&!out.pressed&&!c.pointerActivity());
    out=c.update(pad(160,0),1);
    assert(out.pointer_pressed&&out.pointer_down&&!out.held&&c.pointerActivity());
    // Touchpad rim in aim mode NEVER becomes a key or a simultaneous menu move.
    out=c.update(pad(319,239),1);assert(!out.held&&!out.pointer_pressed);
    // Pressing a menu key while clicking owns focus; pointer hold remains useful
    // for firing during gameplay. Only a fresh click can activate a menu item.
    release(c);Input both=pad(90,170);both.held=ButtonDown;
    out=c.update(both,1);assert(out.pressed==ButtonDown&&!out.pointer_pressed&&!c.pointerActivity());
    both.held=0;out=c.update(both,1);assert(!out.pointer_pressed);
    out=c.update(both,2);assert(!out.pointer_down&&!out.pointer_pressed);
    c.update(pad(90,170,false),2);out=c.update(pad(90,170),2);assert(out.pointer_pressed);
    release(c,2);both=pad(300,50,false);both.held=ButtonLeft;
    out=c.update(both,2);assert(c.pointerMode()&&!c.pointerSteering()&&out.held==ButtonLeft);
    both.held=0;c.update(both,2);assert(c.pointerSteering()); // mode stays explicit
}
static void mode_change_release() {
    InputController c;release(c);c.update(pad(30,30,false),1);
    Input toggle=pad(30,30,false);toggle.held=ButtonPointerMode;
    assert(!c.update(toggle,1).pointer_present&&c.pointerMode());
    toggle.held=0;toggle.pointer_down=true;
    assert(!c.update(toggle,1).pointer_down); // old resting finger must lift
    release(c);assert(c.update(pad(30,30),1).pointer_pressed);
    toggle=pad(30,30);toggle.held=ButtonPointerMode;
    assert(!c.update(toggle,1).held&&!c.pointerMode());
    toggle.held=0;assert(!c.update(toggle,1).held); // no phantom direction on mode switch
    release(c);assert(c.update(pad(160,239),1).held==ButtonDown);
    c.setPointerMode(true);assert(!c.update(pad(160,239),1).pointer_down);
    release(c);assert(c.update(pad(160,239),1).pointer_pressed);
}
static void locale_parser() {
    char tag[32]={};
    assert(locale_text(tag,reinterpret_cast<const uint16_t*>(u"\"zh-CN\""))&&!std::strcmp(tag,"zh-CN"));
    assert(locale_text(tag,reinterpret_cast<const uint16_t*>(u"en-US"))&&!std::strcmp(tag,"en-US"));
    assert(!locale_text(tag,reinterpret_cast<const uint16_t*>(u"\"en-US\"trailing")));
    assert(!locale_text(tag,reinterpret_cast<const uint16_t*>(u"<bad>")));
    assert(!locale_text(tag,reinterpret_cast<const uint16_t*>(u"")));
    const char xml[]="<settings><lang>de-DE</lang></settings>";
    assert(locale_xml(tag,reinterpret_cast<const uint8_t*>(xml),sizeof(xml))&&!std::strcmp(tag,"de-DE"));
    assert(!locale_xml(tag,reinterpret_cast<const uint8_t*>(xml),24)); // truncated close tag
    std::vector<uint8_t> zip(30+12+sizeof(xml));
    zip[0]='P';zip[1]='K';zip[2]=3;zip[3]=4;zip[18]=zip[22]=sizeof(xml);zip[26]=12;
    std::memcpy(zip.data()+30,"settings.xml",12);std::memcpy(zip.data()+42,xml,sizeof(xml));
    LocaleArchiveEntry entry{};
    assert(locale_archive(zip.data(),unsigned(zip.size()),entry)&&!entry.deflated);
    assert(locale_xml(tag,entry.data,entry.plain)&&!std::strcmp(tag,"de-DE"));
    assert(!locale_archive(zip.data(),unsigned(zip.size()-1),entry));
    zip[6]=8;assert(!locale_archive(zip.data(),unsigned(zip.size()),entry)); // unsupported descriptor
    zip[6]=0;zip[22]=255;assert(!locale_archive(zip.data(),unsigned(zip.size()),entry)); // false decoded size
}
static Input physical(std::initializer_list<uint8_t> held) {
    Input in{};in.physical_valid=true;uint16_t rows[8]={};
    for(uint8_t key:held){physical_set(in.physical_keys,key,true);if(key<=88)rows[(key-1)/11]|=uint16_t(1u<<((key-1)%11));}
    in.held=decode_calculator_keys(rows);return in;
}
static void numeric_navigation_never_confirms() {
    InputController c;c.update(physical({}),1,false);
    Input out=c.update(physical({Key2}),1,false);assert(out.held==ButtonDown&&out.pressed==ButtonDown);
    c.update(physical({}),1,false);out=c.update(physical({Key8}),1,false);assert((out.held&ButtonUp)&&!(out.held&(ButtonConfirm|Button1|Button2|Button3)));
    c.update({},1,false);out=c.update(keys(ButtonDown|Button2),1,false);assert(out.held==ButtonDown);
    c.update({},1,false);out=c.update(keys(Button1|Button3),1,false);assert(!out.held&&!out.pressed);
}
static void numeric_confirm_release() {
    InputController c;c.update(physical({}),1,true);
    Input out=c.update(physical({Key5}),1,true);
    assert(out.held==ButtonFire&&out.pressed==ButtonFire);
    // Fire held while combat enters an upgrade cannot accept an offer.
    out=c.update(physical({Key5}),2,false);assert(!out.held&&!out.pressed);
    out=c.update(physical({Key5}),2,false);assert(!out.held&&!out.pressed);
    c.update(physical({}),2,false);
    out=c.update(physical({Key5}),2,false);assert(out.held==ButtonConfirm&&out.pressed==ButtonConfirm);
    out=c.update(physical({Key5}),3,true);assert(!out.held&&!out.pressed);
    c.update(physical({}),3,true);out=c.update(physical({Key5}),3,true);assert(out.held==ButtonFire);
    c.update(physical({}),3,true);c.update(physical({}),4,false);
    out=c.update(physical({KeySpace,KeyControl,calculator_key(0,5)}),4,false);
    assert(!(out.held&(ButtonFire|ButtonConfirm)));
    assert(!binding_key_allowed(BindAbility,Key5)&&!binding_key_allowed(BindReroll,Key5));
    c.update(physical({}),5,true);out=c.update(physical({Key5,Key8}),5,true);
    assert(out.held==(ButtonFire|ButtonThrust));
    out=c.update(physical({Key5,Key8,Key0}),5,true);assert(!out.held&&c.pointerMode());
    out=c.update(physical({Key5,Key8}),5,true);assert(!out.held);
    c.update(physical({}),5,true);out=c.update(physical({Key5,Key8}),5,true);
    assert(out.held==(ButtonFire|ButtonThrust));
    KeyBindings changed;changed.key[BindFire]=calculator_key(4,0);assert(c.setBindings(changed));
    c.update(physical({}),4,false);out=c.update(physical({Key5}),4,false);assert(out.pressed==ButtonConfirm);
}
static void rebindings() {
    InputController c;c.update(physical({}),1,true);
    Input out=c.update(physical({Key8,Key5,Key6}),1,true);
    assert((out.held&(ButtonThrust|ButtonFire|ButtonRight))==(ButtonThrust|ButtonFire|ButtonRight));
    c.update(physical({}),1,true);
    c.update(physical({KeyEnter}),2);assert(c.beginRebind(BindFire));
    out=c.update(physical({KeyEnter}),2);assert(!out.held&&c.capturingBinding());
    c.update(physical({}),2);
    c.update(physical({Key8}),2);assert(c.rebindStatus()==RebindStatus::Conflict&&c.capturingBinding());
    c.update(physical({}),2);c.update(physical({Key0}),2);
    assert(c.rebindStatus()==RebindStatus::Reserved&&!c.pointerMode()&&c.capturingBinding());
    c.update(physical({}),2);out=c.update(physical({KeyEscape}),2);
    assert(!out.pressed&&!c.capturingBinding()&&c.rebindStatus()==RebindStatus::Cancelled);
    c.update(physical({}),2);c.clearRebindStatus();assert(c.rebindStatus()==RebindStatus::Idle);
    assert(c.beginRebind(BindFire));const uint8_t keyF=calculator_key(4,0);
    out=c.update(physical({keyF}),2);assert(!out.held&&c.rebindStatus()==RebindStatus::Applied&&!c.capturingBinding());
    assert(c.bindings().key[BindFire]==keyF);
    assert(!c.update(physical({keyF}),3,true).held); // captured key cannot fire through the page
    c.update(physical({}),3,true);
    assert(c.update(physical({keyF}),3,true).held&ButtonFire);
    c.update(physical({}),3,true);
    assert(!(c.update(physical({Key5,KeySpace,KeyControl}),3,true).held&ButtonFire)); // old primary and aliases removed
    c.update(physical({}),3,true);
    // The same physical A changes meaning from custom Fire to menu Left.
    // Physical release gating must stop that role change becoming a new press.
    KeyBindings bindings=c.bindings();bindings.key[BindFire]=calculator_key(4,6);assert(c.setBindings(bindings));
    out=c.update(physical({calculator_key(4,6)}),3,true);assert((out.held&ButtonFire)&&!(out.held&ButtonLeft));
    out=c.update(physical({calculator_key(4,6)}),4,false);assert(!out.held&&!out.pressed);
    c.update(physical({}),4);out=c.update(physical({calculator_key(4,6)}),4);assert(out.pressed&ButtonLeft);
    c.update(physical({}),4);bindings.key[BindFire]=calculator_key(2,2);assert(c.setBindings(bindings));
    out=c.update(physical({calculator_key(2,2)}),5,true);assert((out.held&ButtonFire)&&!(out.held&ButtonPause)); // P alias cannot steal a binding
    c.update(physical({}),5,true);c.resetBindings();
    assert(c.bindings().key[BindFire]==Key5&&valid_bindings(c.bindings()));
    out=c.update(physical({KeyDelete}),6);assert(out.pressed==ButtonBanish);
    assert(!binding_key_allowed(BindReroll,Key2)&&!binding_key_allowed(BindFire,KeyEnter));
    assert(!std::strcmp(physical_key_name(KeyDelete),"Del")&&!std::strcmp(binding_action_name(BindFire),"Fire"));
}
static void touch_combat_channels() {
    InputController c;c.update(physical({}),1,true);c.setPointerMode(true);
    Input raw=physical({Key8});raw.pointer_is_touchpad=true;raw.pointer_present=true;
    raw.pointer_x=270;raw.pointer_y=30;raw.pointer_down=true;
    Input out=c.update(raw,1,true);
    assert((out.held&ButtonThrust)&&out.pointer_present&&out.pointer_down&&c.pointerSteering());
    raw=physical({Key8,Key7});raw.pointer_is_touchpad=true;raw.pointer_present=true;
    raw.pointer_x=40;raw.pointer_y=210;raw.pointer_down=true;
    out=c.update(raw,1,true);
    assert((out.held&(ButtonThrust|ButtonTab))==(ButtonThrust|ButtonTab));
    assert(out.pointer_present&&out.pointer_down&&c.pointerSteering());
    out=c.update(raw,2,false);assert(!out.held&&!out.pointer_down&&!out.pointer_pressed);
    c.update(physical({}),2);raw=physical({Key0});raw.pointer_is_touchpad=true;
    raw.pointer_present=true;raw.pointer_down=true;raw.pointer_x=0;raw.pointer_y=0;
    out=c.update(raw,2);assert(!c.pointerMode()&&!out.pointer_down&&!out.held);
    physical_set(raw.physical_keys,Key0,false);raw.held=0;
    out=c.update(raw,2);assert(!out.held&&!out.pointer_down);
}
static void numeric_axes_own_keypad_press() {
    InputController c;c.update(physical({}),1,true);
    Input raw=physical({Key4,Key5});raw.pointer_is_touchpad=true;
    raw.pointer_present=raw.pointer_down=true;raw.pointer_x=319;raw.pointer_y=0;
    Input out=c.update(raw,1,true);
    assert((out.held&(ButtonLeft|ButtonFire|ButtonUp|ButtonThrust))==(ButtonLeft|ButtonFire|ButtonUp|ButtonThrust));
    assert(!(out.held&ButtonRight)); // opposite rim cannot fight numeric 4
    physical_set(raw.physical_keys,Key4,false);out=c.update(raw,1,true);
    assert((out.held&(ButtonRight|ButtonFire))==(ButtonRight|ButtonFire)); // pad resumes without a mode switch
    c.update(physical({}),1,true);
    raw=physical({Key8,Key5});raw.pointer_is_touchpad=true;
    raw.pointer_present=raw.pointer_down=true;raw.pointer_x=319;raw.pointer_y=239;
    out=c.update(raw,1,true);
    assert((out.held&(ButtonThrust|ButtonFire|ButtonRight))==(ButtonThrust|ButtonFire|ButtonRight));
    assert(!(out.held&ButtonDown)); // numeric thrust overrides pad brake only
    c.update(physical({}),2,false);
    raw=physical({Key2});raw.pointer_is_touchpad=true;
    raw.pointer_present=raw.pointer_down=true;raw.pointer_x=160;raw.pointer_y=120;
    out=c.update(raw,2,false);assert(out.pressed==ButtonDown); // center+navigation cannot confirm
    // Releasing the direction while still holding center is not a fresh click.
    physical_set(raw.physical_keys,Key2,false);out=c.update(raw,2,false);
    assert(!(out.pressed&ButtonConfirm));
    c.update(physical({}),2,false);
    raw=physical({Key4});raw.pointer_is_touchpad=true;
    raw.pointer_present=raw.pointer_down=true;raw.pointer_x=319;raw.pointer_y=120;
    out=c.update(raw,2,false);assert(out.pressed==ButtonLeft);
    physical_set(raw.physical_keys,Key4,false);out=c.update(raw,2,false);
    assert(!out.pressed&&!out.held); // no late opposite menu move from the old pad press
}
static void binding_storage() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("nnova-keys-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    fs::create_directories(root);platform::configure_host(nullptr,nullptr,0,true,false);
    assert(platform::initialize((root/"nNova.tns").string().c_str()));
    InputController c;assert(!c.loadBindings());assert(c.saveBindings());
    KeyBindings changed=c.bindings();changed.key[BindFire]=calculator_key(4,0);assert(c.setBindings(changed));assert(c.saveBindings());
    InputController loaded;assert(loaded.loadBindings()&&loaded.bindings().key[BindFire]==calculator_key(4,0));
    const fs::path primary=root/"state/nNova.keys.tns";
    assert(fs::file_size(primary)==24);
    {std::fstream file(primary,std::ios::in|std::ios::out|std::ios::binary);assert(file);file.seekg(23);const char byte=char(file.get()^0x80);file.seekp(23);file.put(byte);}
    assert(loaded.loadBindings()&&loaded.bindings().key[BindFire]==Key5); // valid backup, never corrupt keys
    changed.key[BindFire]=KeyEnter;assert(!loaded.setBindings(changed));
    platform::shutdown();fs::remove_all(root);
}
static void daily_calendar() {
    assert(calendar_day({2020,1,1})==18262&&calendar_day({2099,12,31})==47481);
    assert(!calendar_day({2019,12,31})&&!calendar_day({2100,1,1})&&!calendar_day({2026,2,29}));
    assert(calendar_day({2024,2,29})+1==calendar_day({2024,3,1}));
    for(uint32_t day=18262;day<=47481;++day)assert(calendar_day(calendar_date(day))==day);
    char text[11];calendar_text(text,calendar_day({2026,11,1}));assert(!std::strcmp(text,"2026-11-01"));
}

int main() {
    matrix();launch_and_transition();keypad_ownership();pointer_ownership();mode_change_release();locale_parser();numeric_navigation_never_confirms();numeric_confirm_release();rebindings();touch_combat_channels();numeric_axes_own_keypad_press();binding_storage();daily_calendar();
    std::puts("Calculator input passed: numeric matrix, explicit ownership, held-page isolation, click-through, pad-sector latch, OS locale bounds, physical rebinding/cancellation/conflicts and CRC recovery.");
}
