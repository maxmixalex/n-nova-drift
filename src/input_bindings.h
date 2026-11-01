#pragma once
#include "platform.h"

namespace nova {
constexpr uint8_t calculator_key(unsigned row,unsigned col) {return uint8_t(row*11+col+1);}
enum PhysicalKey : uint8_t {
    KeyReturn=calculator_key(0,0),KeyEnter=calculator_key(0,1),KeySpace=calculator_key(0,4),
    Key0=calculator_key(0,7),KeyHome=calculator_key(0,9),
    Key3=calculator_key(1,3),Key1=calculator_key(1,7),Key6=calculator_key(2,3),Key4=calculator_key(2,7),
    Key9=calculator_key(3,3),Key7=calculator_key(3,7),KeyMinus=calculator_key(5,2),
    Key5=calculator_key(5,6),KeyDelete=calculator_key(5,9),KeyDoc=calculator_key(6,3),
    Key2=calculator_key(6,4),KeyMenu=calculator_key(6,5),Key8=calculator_key(6,6),KeyEscape=calculator_key(6,7),
    KeyTab=calculator_key(6,9),KeyShift=calculator_key(7,8),KeyControl=calculator_key(7,9),
    KeyHostUp=89,KeyHostDown,KeyHostLeft,KeyHostRight,
};
inline bool physical_down(const uint32_t* words,uint8_t key) {
    return key&&key<=96&&(words[(key-1)/32]&(1u<<((key-1)%32)));
}
inline void physical_set(uint32_t* words,uint8_t key,bool down) {
    if(!key||key>96)return;
    const uint32_t mask=1u<<((key-1)%32);
    if(down)words[(key-1)/32]|=mask;else words[(key-1)/32]&=~mask;
}
enum BindAction : uint8_t { BindThrust,BindBrake,BindLeft,BindRight,BindFire,BindAbility,BindUpgrade,BindReroll,BindBanish,BindCount };
struct KeyBindings {
    uint8_t key[BindCount]={Key8,Key2,Key4,Key6,Key5,Key7,Key9,KeyMinus,KeyDelete};
};
enum class RebindStatus : uint8_t { Idle,Waiting,Applied,Cancelled,Reserved,Conflict };
const char* physical_key_name(uint8_t key); // ASCII calculator key legend, nullptr for a nonexistent key
const char* binding_action_name(BindAction action);
bool binding_key_allowed(BindAction action,uint8_t key);
bool valid_bindings(const KeyBindings& bindings);
uint32_t bound_buttons(const uint32_t* physical,const KeyBindings& bindings,bool gameplay);
bool load_key_bindings(KeyBindings& bindings);
bool save_key_bindings(const KeyBindings& bindings);
}
