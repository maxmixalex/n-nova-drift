#include "localization.h"
#include "font.h"
#include "assets.h"
#include "content.h"
#include "platform.h"
#include "nnova/native_strings.h"
#include <cstdio>
#include <string>
#include <filesystem>
using namespace nova;
static Locale locale;
static Font font;
static Assets assets;
static unsigned asset_lookups,asset_missing;
static void check_asset(const char* category,unsigned id,const char* name) {
    ++asset_lookups;
    const Sprite* sprite=name&&*name?assets.find(name):nullptr;
    if(!sprite||!sprite->pixels||!sprite->width||!sprite->height){
        std::printf("Missing/unreadable sprite %s[%u]: %s\n",category,id,name?name:"(null)");++asset_missing;
    }
}
static void check_assets() {
    if(!assets.open()){std::printf("Cannot load asset catalog: %s\n",assets.error());++asset_missing;return;}
    for(unsigned i=0;i<ENEMY_COUNT;++i)check_asset("enemy",i,enemy_types[i].sprite);
    const unsigned sizes[]={MOD_COUNT,WEAPON_COUNT,BODY_COUNT,SHIELD_COUNT};
    const char* names[]={"mod","weapon","body","shield"};
    for(unsigned kind=0;kind<4;++kind)for(unsigned i=0;i<sizes[kind];++i){
        Offer offer={(uint8_t)kind,(uint8_t)i};check_asset(names[kind],i,offerSprite(offer));
    }
    assets.close();
}
static unsigned missing,unresolved,lookups;
static void check(const char* key,bool required=true) {
    const char* s=locale.get(key);if(!s){if(required){std::printf("Missing key %s %s\n",Locale::code(locale.index()),key);++missing;}return;}
    char text[65536];unsigned n=Locale::format(text,sizeof(text),s);
    if(n){std::printf("Unresolved %s %s: %u\n",Locale::code(locale.index()),key,n);unresolved+=n;}
    const char* p=text;
    while(*p){unsigned cp=next_utf8(p);if(cp>=32&&!font.contains(cp)){std::printf("Missing glyph %s %s U+%04X\n",Locale::code(locale.index()),key,cp);++missing;}}
    ++lookups;
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    platform::configure_host(argv[1],nullptr,0,true,false);
    if(!platform::initialize("/tmp/nnova-locale-audit/nNova.tns"))return 3;
    check_assets();
    for(unsigned lang=0;lang<Locale::Count;++lang){
        if(!locale.open(lang)||!font.open(lang)){std::printf("Cannot load %s\n",Locale::code(lang));++missing;continue;}
        for(unsigned i=0;i<native_text::EntryCount;++i)check(native_text::entries[i].key);
        const char* basic[]={"MenuStart","MenuSettings","MenuExit","SettingsTitle","SettingsLanguage","UpgradeChooseMod","UpgradeChooseGear","DrawEndScoreTitleWeapon","DrawEndScoreTitleBody","DrawEndScoreTitleShields","DrawEndScoreTitleConstructs","GUIWave","GUILevel","GUIScore"};
        for(const char* k:basic)check(k);
        const char* interface_keys[]={
#include "ui_locale_keys.inc"
        };
        for(const char* k:interface_keys)check(k);
        for(unsigned m=0;m<MOD_COUNT;++m){
            for(const char* part:{"Title","Short","Full"}){
                const char* tail=m==M_SHIELD_RADIUS&&part[0]!='T'?"ShieldRadius":mods[m].source_key;
                std::string key=std::string("Mod")+part+tail;check(key.c_str(),!(m==M_X_THE_VOID&&part[0]=='S'));
            }
        }
        const char* weapons[]={"Standard","SplitShot","Flak","Railgun","Grenade","Salvo","Vortex","Pulse","Torrent","BladeDrone","Dart","ThermalLance","Swords"};
        const char* bodies[]={"Standard","Assault","Hullbreaker","Stealth","Firefly","Sentinel","Engineer","Research","Battery","Architect","Viper","Carrier","Courser","Leviathan"};
        const char* shields[]={"Standard","Halo","Reflect","Bastion","Temporal","Amp","Warp","Helix","Siphon","Shockwave"};
        for(const char* part:{"Title","Short","Full"}){
            for(const char* k:weapons)check((std::string("Gear")+part+"Weapon"+(std::string(k)=="BladeDrone"&&part[0]!='T'?"Blade":k)).c_str());
            for(const char* k:bodies)check((std::string("Gear")+part+"Body"+k).c_str());
            for(const char* k:shields)check((std::string("Gear")+part+"Shield"+k).c_str());
        }
        // The source changelog is deliberately English in every language. Its
        // entire raw text must still be drawable by every selected font.
        if(!locale.openFile("nNova.changelog.dat.tns")){std::printf("Missing changelog %s\n",Locale::code(lang));++missing;}
        else {const char* p=locale.get("OwnerChangelog");if(!p)++missing;else while(*p){unsigned cp=next_utf8(p);if(cp>=32&&!font.contains(cp)){std::printf("Changelog glyph %s U+%04X\n",Locale::code(lang),cp);++missing;}}++lookups;}
        for(unsigned name=0;name<Locale::Count;++name){const char* p=Locale::name(name);while(*p){unsigned cp=next_utf8(p);if(!font.contains(cp)){std::printf("Language name missing %s U+%04X\n",Locale::code(lang),cp);++missing;}}}
    }
    platform::shutdown();std::printf("Audited %u localized interface/content entries: %u missing keys/glyphs, %u unresolved value references.\n",lookups,missing,unresolved);
    std::printf("Audited %u runtime sprite references (%u enemies, %u gear, %u mod offers): %u missing/unreadable assets.\n",asset_lookups,(unsigned)ENEMY_COUNT,(unsigned)(WEAPON_COUNT+BODY_COUNT+SHIELD_COUNT),(unsigned)MOD_COUNT,asset_missing);
    return missing||unresolved||asset_missing?1:0;
}
