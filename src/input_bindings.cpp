#include "input_bindings.h"
#include "input_controller.h"
#include <stddef.h>

namespace nova {
namespace {
// The physical IDs are matrix positions, not host keycodes or text input.
const char* const names[88]={
    "Return","Enter",nullptr,"(-)","Space","Z","Y","0","?!","Home",nullptr,
    "X","W","V","3","U","T","S","1","pi","Trig","10^x",
    "R","Q","P","6","O","N","M","4","EE","x^2",nullptr,
    "L","K","J","9","I","H","G","7","/","exp",nullptr,
    "F","E","D",nullptr,"C","B","A","=","*","^",nullptr,
    "'","Var","-",")",".","(","5","Catalog","Fraction","Del","Scratchpad",
    "Flag",nullptr,"+","Doc","2","Menu","8","Esc","|","Tab",nullptr,
    nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,"Shift","Ctrl",","
};
constexpr uint32_t actions[BindCount]={ButtonThrust,ButtonDown,ButtonLeft,ButtonRight,ButtonFire,ButtonTab,ButtonUpgrade,ButtonReroll,ButtonBanish};
int owner(const KeyBindings& bindings,uint8_t key) {
    for(unsigned i=0;i<BindCount;++i)if(bindings.key[i]==key)return int(i);
    return -1;
}
bool alias(const uint32_t* physical,const KeyBindings& bindings,unsigned action,uint8_t key) {
    const KeyBindings defaults{};
    return bindings.key[action]==defaults.key[action]&&physical_down(physical,key)&&
        (owner(bindings,key)<0||owner(bindings,key)==int(action));
}
uint32_t crc32(const void* pointer,unsigned bytes) {
    const uint8_t* data=static_cast<const uint8_t*>(pointer);uint32_t crc=~0u;
    while(bytes--){crc^=*data++;for(unsigned i=0;i<8;++i)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}return ~crc;
}
struct BindingFile {uint32_t magic,version;uint8_t keys[BindCount],reserved[3];uint32_t crc;};
static_assert(sizeof(BindingFile)==24,"portable key-binding state");
bool decode_file(const BindingFile& file,KeyBindings& out) {
    if(file.magic!=0x4e4b4559u||file.version!=1||file.crc!=crc32(&file,offsetof(BindingFile,crc)))return false;
    KeyBindings candidate;for(unsigned i=0;i<BindCount;++i)candidate.key[i]=file.keys[i];
    if(!valid_bindings(candidate))return false;
    out=candidate;return true;
}
}
const char* physical_key_name(uint8_t key) {
    if(key&&key<=88)return names[key-1];
    if(key==KeyHostUp)return "Up";
    if(key==KeyHostDown)return "Down";
    if(key==KeyHostLeft)return "Left";
    if(key==KeyHostRight)return "Right";
    return nullptr;
}
const char* binding_action_name(BindAction action) {
    static const char* const labels[BindCount]={"Thrust","Brake","Turn left","Turn right","Fire","Dash / Blink","Upgrades","Reroll","Banish"};
    return action<BindCount?labels[action]:"";
}
bool binding_key_allowed(BindAction action,uint8_t key) {
    if(action>=BindCount||!key||key>88||!physical_key_name(key))return false;
    if(key==Key0||key==KeyEnter||key==KeyReturn||key==KeyEscape||key==KeyMenu||key==KeyDoc||key==KeyHome)return false;
    // These menu keys must keep a single meaning when an action is also used
    // on upgrade/detail pages. Flight actions can use the numeric directions.
    if(action>=BindAbility&&(key==Key1||key==Key2||key==Key3||key==Key4||key==Key5||key==Key6||key==Key8))return false;
    return true;
}
bool valid_bindings(const KeyBindings& bindings) {
    for(unsigned i=0;i<BindCount;++i) {
        if(!binding_key_allowed(BindAction(i),bindings.key[i]))return false;
        for(unsigned j=0;j<i;++j)if(bindings.key[i]==bindings.key[j])return false;
    }return true;
}
uint32_t bound_buttons(const uint32_t* physical,const KeyBindings& bindings,bool gameplay) {
    uint16_t rows[8]={};
    for(unsigned row=0;row<8;++row)for(unsigned col=0;col<11;++col)
        if(physical_down(physical,calculator_key(row,col)))rows[row]|=uint16_t(1u<<col);
    uint32_t buttons=decode_calculator_keys(rows);
    buttons&=~(ButtonTab|ButtonUpgrade|ButtonReroll|ButtonBanish);
    if(owner(bindings,calculator_key(2,2))>=0)buttons&=~ButtonPause;
    if(gameplay)buttons&=~(ButtonUp|ButtonDown|ButtonLeft|ButtonRight|ButtonThrust|ButtonFire);
    // Optional host/letter aliases may never compete with an explicitly bound
    // menu action. Numeric menu navigation and confirm/back are always fixed.
    if(!gameplay) {
        // Numeric 5 is the fixed menu confirm, independent of a custom Fire
        // binding. Space/Ctrl/Z remain gameplay aliases, never menu confirm.
        buttons&=~ButtonFire;
        if(physical_down(physical,Key5))buttons|=ButtonConfirm;
        const uint8_t nav[]={calculator_key(1,1),calculator_key(1,6),calculator_key(4,6),calculator_key(4,2)};
        const uint32_t nav_mask[]={ButtonUp|ButtonThrust,ButtonDown,ButtonLeft,ButtonRight};
        const uint8_t numeric[]={Key8,Key2,Key4,Key6};
        for(unsigned i=0;i<4;++i)if(owner(bindings,nav[i])>=BindAbility&&!physical_down(physical,numeric[i]))buttons&=~nav_mask[i];
        if(physical_down(physical,KeyHostUp))buttons|=ButtonUp;
        if(physical_down(physical,KeyHostDown))buttons|=ButtonDown;
        if(physical_down(physical,KeyHostLeft))buttons|=ButtonLeft;
        if(physical_down(physical,KeyHostRight))buttons|=ButtonRight;
    }
    for(unsigned i=gameplay?0:BindAbility;i<BindCount;++i)if(physical_down(physical,bindings.key[i]))buttons|=actions[i];
    if(gameplay) {
        if(alias(physical,bindings,BindThrust,calculator_key(1,1))||alias(physical,bindings,BindThrust,calculator_key(1,0))||alias(physical,bindings,BindThrust,KeyShift)||alias(physical,bindings,BindThrust,KeyHostUp))buttons|=ButtonThrust;
        if(alias(physical,bindings,BindBrake,calculator_key(1,6))||alias(physical,bindings,BindBrake,KeyHostDown))buttons|=ButtonDown;
        if(alias(physical,bindings,BindLeft,calculator_key(4,6))||alias(physical,bindings,BindLeft,KeyHostLeft))buttons|=ButtonLeft;
        if(alias(physical,bindings,BindRight,calculator_key(4,2))||alias(physical,bindings,BindRight,KeyHostRight))buttons|=ButtonRight;
        if(alias(physical,bindings,BindFire,KeySpace)||alias(physical,bindings,BindFire,calculator_key(0,5))||alias(physical,bindings,BindFire,KeyControl))buttons|=ButtonFire;
    }
    if(alias(physical,bindings,BindAbility,KeyTab))buttons|=ButtonTab;
    if(alias(physical,bindings,BindUpgrade,calculator_key(1,4)))buttons|=ButtonUpgrade;
    if(alias(physical,bindings,BindReroll,calculator_key(2,0)))buttons|=ButtonReroll;
    return buttons;
}
bool load_key_bindings(KeyBindings& bindings) {
    BindingFile file{};
    if(platform::read_state("nNova.keys.tns",&file,sizeof(file))&&decode_file(file,bindings))return true;
    return platform::read_state("nNova.keys.tns.bak.tns",&file,sizeof(file))&&decode_file(file,bindings);
}
bool save_key_bindings(const KeyBindings& bindings) {
    if(!valid_bindings(bindings))return false;
    BindingFile file{};file.magic=0x4e4b4559u;file.version=1;
    for(unsigned i=0;i<BindCount;++i)file.keys[i]=bindings.key[i];
    file.crc=crc32(&file,offsetof(BindingFile,crc));return platform::write_state("nNova.keys.tns",&file,sizeof(file));
}
}
