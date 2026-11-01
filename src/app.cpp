#include "app.h"
#include "assets.h"
#include "font.h"
#include "game.h"
#include "daily_calendar.h"
#include "input_controller.h"
#include "localization.h"
#include "render.h"
#include <stddef.h>

namespace nova {
namespace {
enum Page { HomePage,SettingsPage,ControlsPage,BuildPage,NewRunPage,
    ModePage,LanguagePage,DetailPage,StatsPage,ScoresPage,LearnPage,GlossaryPage,
    CatalogPage,BestiaryPage,CreditsPage,ChangelogPage,RebindPage,ProgressPage,SuperModsPage,ResetPage,ResumePage,AssistPage,EndingPage,DailyPage,DailyScoresPage };
enum { AutoFire=1,FullEffects=2,ShowFps=4,Chinese=8,Wild=16,
    BrightAsteroids=32,PointerAim=64,EnemyHullBars=128,
    DisableUpgradeChain=256,AssistMode=512,HideCursor=1024,DisableShake=2048,
    DisableFlash=4096,DisableShockwave=8192,ShowDamage=16384,ShowHealing=32768,
    ShowSelfDamage=65536,EnhancedVisibility=131072,SpeedrunMode=262144 };
struct Preferences {
    uint32_t magic,version,flags,high_score,best_wave,runs,language,run_flags;
    uint32_t auto_deploy,ataraxia_reminders,autosave,background_detail,skin,crc;
};
struct PreferencesV2 { uint32_t magic,version,flags,high_score,best_wave,runs,language,run_flags,crc; };
struct LegacyPreferences { uint32_t magic,version,flags,high_score,best_wave,runs,crc; };
struct Snapshot { char magic[8];uint32_t version,bytes,valid,crc;Game game; };
static Assets assets;
static Font font;
static Locale language;
static Game game;
static Renderer renderer;
static Skins skins;
static uint16_t ataraxia_prompt_level;
static InputController input_controller;
static Preferences preferences;
static Snapshot snapshot;
struct RunRecord { uint32_t score,ticks,damage_dealt,damage_taken,kills,distance_q8,highest_hit;uint16_t wave,level,flags;uint8_t weapon,shield,body,reserved; };
struct Profile { uint32_t magic,version,bytes,crc;Career career;RunRecord records[10]; };
// The previous v2 schema used a 64-species catalogue. Preserve its lower discovery word
// and every aggregate when adding a second word; the actor pool is unchanged.
struct CareerV2 {
 uint32_t runs,total_score,total_kills,total_ticks,best_score,highest_hit;
 uint64_t total_distance_q8,seen_enemies;
 uint32_t enemy_kills[64],unlock_points_milli,last_seed,last_frame;
 uint16_t best_wave,rank,final_boss_kills,flags;
};
struct ProfileV2 {uint32_t magic,version,bytes,crc;CareerV2 career;RunRecord records[10];};
static Profile profile;
struct DailyRecord { uint32_t day;RunRecord run; };
struct DailyProfile {uint32_t magic,version,last_day;DailyRecord records[16];uint32_t crc;};
static DailyProfile daily_profile;
static CalendarDate daily_date;
static bool daily_date_unset;
static uint32_t pending_run_day;
static bool reset_progress;
static int progress_selection,learn_tab;
static unsigned gained_ranks;
static Career& career=profile.career;
static Page page,return_page,detail_return,controls_return,reference_return,credits_return,scores_return,progress_return;
static bool ready,quitting,has_save,gameover_recorded,pointer_visible,ending_seen;
static bool pointer_down,pointer_activity,pending_ability,incompatible_save;
static bool language_available[Locale::Count];
static int selection,build_selection,detail_selection,detail_scroll,detail_lines,stats_tab,stats_subpage,controls_tab;
static int upgrade_selection,detail_mode,detail_visible;
static int reference_enemy,catalog_tab;
static int settings_tab;static int scores_tab;
static bool detail_tree;
static char reference_key[96],reference_title[192];
static char reference_body[256];
static int pointer_x,pointer_y;
static uint32_t clock_ms,autosave_ms,fps_ms,fps_frames,fps_value,toast_until,demo_ticks;
static char toast[160];
static Offer detail_offer;
static const uint16_t White=rgb(233,237,251),Muted=rgb(163,169,194);
static const uint16_t Cyan=rgb(84,184,255),Red=rgb(250,50,105);
static const uint16_t Panel=rgb(6,6,20);
#include "item_tags.inc"
struct ModeChoice { uint16_t flag;const char* title;const char* description;const char* sprite; };
static const ModeChoice mode_choices[]={
    {RUN_DANGER,"UnlocksTitleDanger","UnlocksDanger","sModeDangerZone"},
    {RUN_ENEMY_TERRITORY,"UnlocksTitleEnemyTerritory","UnlocksEnemyTerritory","sModeEnemyTerritory"},
    {RUN_ELITE,"UnlocksTitleEliteEnemy","UnlocksEliteEnemy","sModeEliteEnemy"},
    {RUN_HOSTILE,"UnlocksTitleHostileUniverse","UnlocksHostileUniverse","sModeHostileUniverse"},
    {RUN_NEMESIS,"UnlocksTitleNemesis","UnlocksNemesis","sModeNemesis"},
    {RUN_MAYHEM,"UnlocksTitleMayhem","UnlocksMayhem","sModeMayhem"},
    {RUN_ANNIHILATION,"UnlocksTitleAnnihilation","UnlocksAnnihilation","sModeAnnihilation"},
    {RUN_ENDLESS,"LoadoutEndless","LoadoutEndlessDesc","sModeEndless"},
    {RUN_WILD,"UnlocksTitleWildMetamorphosis","UnlocksWildMetamorphosis","sModeWildMetamorphosis"},
    {RUN_DRAFT,"UnlocksTitleDraft","UnlocksDraft","sModeDraft"},
    {RUN_PRACTICE,"LoadoutPractice","LoadoutPracticeDesc","sModePractice"},
    {RUN_DAILY,"UnlocksTitleDailyChallenge","UnlocksDailyChallenge","sModeDailyChallenge"}
};
static constexpr int ModeCount=sizeof(mode_choices)/sizeof(mode_choices[0]);
static const char* const glossary_keys[]={
#include "glossary_keys.inc"
};
static const char* glossary_title(unsigned i) {
    const char* aliases[]={"GUIWave","DefineKeyboardControlsReroll","DrawEndScoreTitleBurnDamage","DefineKeyboardControlsThrust","DrawEndScoreTitleShields"};
    unsigned n=sizeof(glossary_keys)/sizeof(glossary_keys[0]);return i>=n-5?language.text(aliases[i-(n-5)]):language.text(glossary_keys[i]);
}
static constexpr int GlossaryCount=sizeof(glossary_keys)/sizeof(glossary_keys[0]);
static const char* settings_titles[6]={"SettingsTitleGameplay","SettingsTitleControls","SettingsTitleLanguage","SettingsTitleVideo","SettingsTitleAudio","SettingsTitleAccount"};
static const uint8_t settings_rows[6][16]={
    {10,15,16,17,18},
    {4,0,6,11,19,20,21,22,23},
    {5},
    {1,2,3,24,25,26,27,28,29,30,31,32,33},
    {34},
    {12,8,13,14}
};
static const uint8_t settings_counts[6]={5,9,1,13,1,4};
static const char* const setting_keys[]={
 "SettingsToggleAutoFiring","UiVisualEffects","SettingsAsteroidBrightness","UiFrameCounter","UiInputMode","SettingsLanguage","SettingsShowControls","SettingsBack","MenuScore","CreditsTitle","SettingsDisplayEnemyHull","SettingsCustomControls","DrawEndScoreProgress","SettingsResetScores","SettingsResetProgress",
 "SettingsDisableUpgradeChaining","SettingsAssistMode","SettingsAtaraxiaReminders","SettingsAutoSaveBehavior","SettingsAutoDrone","SettingsAutoMine","SettingsAutoTurret","SettingsAutoAlly","SettingsHideCursor","SettingsDisableShake","SettingsDisableFlash","SettingsDisableShockwaveEffects","SettingsShowDamage","SettingsShowHealing","SettingsShowSelfDamage","SettingsBackgroundDetail","SettingsEnhancedVisibility","SettingsSpeedrunMode","SettingsCustomSkins","UiAudioOutput"
};
static const char* const setting_descriptions[]={
 "SettingsToggleAutoFiringDesc","","SettingsAsteroidBrightnessDesc","","","","SettingsShowControlsDesc","","","","SettingsDisplayEnemyHullDesc","SettingsCustomControlsDesc","","SettingsResetScoresDesc","SettingsResetProgressDesc",
 "SettingsDisableUpgradeChainingDesc","SettingsAssistModeDesc","SettingsAtaraxiaRemindersDesc","SettingsAutoSaveBehaviorDesc","SettingsAutoDroneDesc","SettingsAutoMineDesc","SettingsAutoTurretDesc","SettingsAutoAllyDesc","SettingsHideCursorDesc","SettingsDisableShakeDesc","SettingsDisableFlashDesc","SettingsDisableShockwaveEffectsDesc","SettingsShowDamageDesc","SettingsShowHealingDesc","SettingsShowSelfDamageDesc","SettingsBackgroundDetailDesc","SettingsEnhancedVisibilityDesc","SettingsSpeedrunModeDesc","SettingsCustomSkinsDesc","UiAudioDetails"
};
static const uint32_t setting_flags[]={AutoFire,FullEffects,BrightAsteroids,ShowFps,PointerAim,0,0,0,0,0,EnemyHullBars,0,0,0,0,DisableUpgradeChain,AssistMode,0,0,0,0,0,0,HideCursor,DisableShake,DisableFlash,DisableShockwave,ShowDamage,ShowHealing,ShowSelfDamage,0,EnhancedVisibility,SpeedrunMode,0,0};
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
static const char* tr(const char* key,const char* fallback="") {return language.text(key,fallback);}
static void notice(const char* message) {
    toast[0]=0;append(toast,sizeof(toast),message);toast_until=clock_ms+2500;
}
static bool save_preferences() {
    preferences.crc=crc32(&preferences,offsetof(Preferences,crc));
    return platform::write_state("nNova.cfg.tns",&preferences,sizeof(preferences));
}
static void apply_settings() {
    game.auto_fire=(preferences.flags&AutoFire)!=0;
    game.options.auto_deploy_mask=uint8_t(preferences.auto_deploy);
    game.options.assist_mode=(preferences.flags&AssistMode)?1:0;
    input_controller.setPointerMode((preferences.flags&PointerAim)!=0);
    renderer.configure(DisplayOptions{uint8_t(preferences.background_detail),!(preferences.flags&DisableShake),!(preferences.flags&DisableFlash),!(preferences.flags&DisableShockwave),bool(preferences.flags&ShowDamage),bool(preferences.flags&ShowHealing),bool(preferences.flags&ShowSelfDamage),bool(preferences.flags&EnhancedVisibility),bool(preferences.flags&SpeedrunMode)},&skins);
}
static bool valid_preferences(const Preferences& p){return p.magic==0x4e4f5641u&&p.version==3&&p.language<Locale::Count&&p.run_flags<4096&&p.auto_deploy<16&&p.ataraxia_reminders<3&&p.autosave<3&&p.background_detail<4&&p.skin<=skinpack::MaxSkins&&p.crc==crc32(&p,offsetof(Preferences,crc));}
static bool load_preferences() {
    preferences=Preferences{0x4e4f5641u,3,FullEffects|BrightAsteroids|EnemyHullBars,0,0,0,0,0,15,1,1,3,0,0};
    const char* names[]={"nNova.cfg.tns","nNova.cfg.tns.bak.tns"};
    for(unsigned i=0;i<2;++i){Preferences loaded={};if(platform::read_state(names[i],&loaded,sizeof(loaded))&&valid_preferences(loaded)){preferences=loaded;preferences.flags&=524287u;return true;}}
    for(unsigned i=0;i<2;++i){PreferencesV2 old={};if(platform::read_state(names[i],&old,sizeof(old))&&old.magic==preferences.magic&&old.version==2&&old.language<Locale::Count&&old.crc==crc32(&old,offsetof(PreferencesV2,crc))){
        preferences.flags=old.flags&255;preferences.high_score=old.high_score;preferences.best_wave=old.best_wave;preferences.runs=old.runs;preferences.language=old.language;preferences.run_flags=old.run_flags;return true;}}
    for(unsigned i=0;i<2;++i){LegacyPreferences old={};if(platform::read_state(names[i],&old,sizeof(old))&&old.magic==preferences.magic&&old.version==1&&old.crc==crc32(&old,offsetof(LegacyPreferences,crc))){
        preferences.flags=(old.flags&31)|BrightAsteroids|EnemyHullBars;preferences.high_score=old.high_score;preferences.best_wave=old.best_wave;preferences.runs=old.runs;preferences.language=(old.flags&Chinese)?9:0;if(old.flags&Wild)preferences.run_flags|=RUN_WILD;return true;}}
    return false;
}
static bool save_profile() {
    profile.magic=0x4e505246;profile.version=3;profile.bytes=sizeof(Profile);
    profile.crc=crc32(&profile.career,sizeof(Profile)-offsetof(Profile,career));
    return platform::write_state("nNova.profile.tns",&profile,sizeof(profile));
}
static bool valid_profile() {
    return profile.magic==0x4e505246&&profile.version==3&&profile.bytes==sizeof(Profile)&&career.valid()&&
        profile.crc==crc32(&profile.career,sizeof(Profile)-offsetof(Profile,career));
}
static void load_profile() {
    if(platform::read_state("nNova.profile.tns",&profile,sizeof(profile))&&valid_profile())return;
    if(platform::read_state("nNova.profile.tns.bak.tns",&profile,sizeof(profile))&&valid_profile())return;
    const char* names[]={"nNova.profile.tns","nNova.profile.tns.bak.tns"};
    for(unsigned f=0;f<2;++f){ProfileV2 old={};
        if(!platform::read_state(names[f],&old,sizeof(old))||old.magic!=0x4e505246||old.version!=2||old.bytes!=sizeof(old)||old.crc!=crc32(&old.career,sizeof(old)-offsetof(ProfileV2,career)))continue;
        profile=Profile{};career.clear();const CareerV2& c=old.career;
        career.runs=c.runs;career.total_score=c.total_score;career.total_kills=c.total_kills;career.total_ticks=c.total_ticks;career.best_score=c.best_score;career.highest_hit=c.highest_hit;
        career.total_distance_q8=c.total_distance_q8;career.seen_enemies=c.seen_enemies;career.seen_enemies_hi=0;
        for(unsigned i=0;i<64;++i)career.enemy_kills[i]=c.enemy_kills[i];
        career.unlock_points_milli=c.unlock_points_milli;career.last_seed=c.last_seed;career.last_frame=c.last_frame;career.best_wave=c.best_wave;career.rank=c.rank;career.final_boss_kills=c.final_boss_kills;career.flags=c.flags;
        if(!career.valid())continue;
        for(unsigned i=0;i<10;++i)profile.records[i]=old.records[i];
        // Defer writing until the next ordinary profile save, keeping the old
        // file intact; atomic write_state will retain it as the backup then.
        return;
    }
    profile=Profile{};career.clear();
}
static bool valid_daily_profile(const DailyProfile& p) {
    if(p.magic!=0x4e444159u||p.version!=1||p.crc!=crc32(&p,offsetof(DailyProfile,crc)))return false;
    if(p.last_day&&(p.last_day<18262||p.last_day>47481))return false;
    uint32_t previous=0xffffffffu;
    for(const DailyRecord& r:p.records) {
        if(r.day&&(r.day<18262||r.day>47481||r.day>=previous||r.run.weapon>=WEAPON_COUNT||r.run.shield>=SHIELD_COUNT||r.run.body>=BODY_COUNT))return false;
        previous=r.day;
    }
    return true;
}
static void load_daily_profile() {
    daily_profile=DailyProfile{};
    const char* names[]={"nNova.daily.tns","nNova.daily.tns.bak.tns"};
    for(const char* name:names) {
        if(platform::read_state(name,&daily_profile,sizeof(daily_profile))&&valid_daily_profile(daily_profile))return;
    }
    daily_profile=DailyProfile{};
}
static bool save_daily_profile() {
    daily_profile.magic=0x4e444159u;daily_profile.version=1;
    daily_profile.crc=crc32(&daily_profile,offsetof(DailyProfile,crc));
    return platform::write_state("nNova.daily.tns",&daily_profile,sizeof(daily_profile));
}
static RunRecord run_record() {
    return RunRecord{game.score,game.frame,game.run_stats.damage_dealt,game.run_stats.damage_taken,game.kills,game.run_stats.distance_q8,game.run_stats.highest_hit,game.wave,game.level,game.run_flags,game.player.weapon,game.player.shield_type,game.player.body,0};
}
static void record_daily() {
    if(!(game.run_flags&RUN_DAILY)||game.daily_day<18262||game.daily_day>47481)return;
    unsigned at=16;
    for(unsigned i=0;i<16;++i)if(daily_profile.records[i].day==game.daily_day){at=i;break;}
    if(at<16&&daily_profile.records[at].run.score>=game.score)return;
    if(at==16) {
        for(unsigned i=0;i<16;++i)if(daily_profile.records[i].day<game.daily_day){at=i;break;}
        if(at==16)return; // Keep the sixteen newest challenge dates.
        for(unsigned i=15;i>at;--i)daily_profile.records[i]=daily_profile.records[i-1];
    }
    daily_profile.records[at]=DailyRecord{game.daily_day,run_record()};
    if(!save_daily_profile())notice(tr("UiSaveFailed"));
}
static bool save_run(bool valid=true) {
    const char magic[8]={'N','N','O','V','A','S','1',0};bytes(snapshot.magic,magic,8);
    snapshot.version=2;snapshot.bytes=sizeof(Game);snapshot.valid=valid?1:0;
    bytes(&snapshot.game,&game,sizeof(Game));
    if(snapshot.game.state==Pause)snapshot.game.state=Playing;
    snapshot.game.resetInput();snapshot.crc=crc32(&snapshot.game,sizeof(Game));
    bool ok=platform::write_state("nNova.sav.tns",&snapshot,sizeof(snapshot));
    if(ok){has_save=valid;incompatible_save=false;}else notice(tr("UiSaveFailed"));
    autosave_ms=0;return ok;
}
static void autosave_run(){if(preferences.autosave)save_run();}
static unsigned snapshot_status() {
    const char magic[8]={'N','N','O','V','A','S','1',0};
    for(unsigned i=0;i<8;++i)if(snapshot.magic[i]!=magic[i])return 0;
    if(snapshot.version!=2||snapshot.bytes!=sizeof(Game))return 0;
    if(snapshot.valid==0)return 2;
    return snapshot.valid==1&&snapshot.crc==crc32(&snapshot.game,sizeof(Game))&&
        snapshot.game.validState()&&(snapshot.game.state==Playing||snapshot.game.state==Upgrade||snapshot.game.state==Pause);
}
static bool load_run() {
    if(platform::read_state("nNova.sav.tns",&snapshot,sizeof(snapshot))) {
        unsigned status=snapshot_status();if(status)return status==1;
    }
    return platform::read_state("nNova.sav.tns.bak.tns",&snapshot,sizeof(snapshot))&&snapshot_status()==1;
}
static void record_gameover() {
    if(game.state!=GameOver||gameover_recorded)return;
    gameover_recorded=true;selection=0;gained_ranks=0;
    record_daily();
    if(!(game.run_flags&RUN_PRACTICE)&&((game.run_flags&RUN_DAILY)||career.last_seed!=game.seed||career.last_frame!=game.frame)) {
        gained_ranks=career.award(game);
        for(unsigned at=0;!(game.run_flags&RUN_DAILY)&&at<10;++at)if(game.score>profile.records[at].score) {
            for(unsigned j=9;j>at;--j)profile.records[j]=profile.records[j-1];
            profile.records[at]=run_record();break;
        }
        if(!save_profile())notice(tr("UiSaveFailed"));
    }
    if(!(game.run_flags&(RUN_PRACTICE|RUN_DAILY))) {
        if(game.score>preferences.high_score)preferences.high_score=game.score;
        if(game.wave>preferences.best_wave)preferences.best_wave=game.wave;
    }
    save_preferences();save_run(false);has_save=false;
}
static void resume_saved() {
    if(!load_run()){has_save=false;notice(tr("UiSaveUnavailable"));return;}
    bytes(&game,&snapshot.game,sizeof(Game));if(game.state==Pause)game.state=Playing;
    game.resetInput();apply_settings();ataraxia_prompt_level=game.level;
    gameover_recorded=false;ending_seen=false;page=HomePage;selection=0;autosave_ms=0;pending_ability=false;
}
static void start_run(uint32_t daily=0) {
    game.start(daily?Game::dailySeedFor(daily):platform::host_demo()?0x4e4f5641u:platform::seed()^clock_ms);
    game.configureCareer(career.rank,career.final_boss_kills!=0);
    game.configureRun(uint16_t(preferences.run_flags&~RUN_DAILY));
    if(daily){game.configureDaily(daily);daily_profile.last_day=daily;if(!save_daily_profile())notice(tr("UiSaveFailed"));}
    apply_settings();ataraxia_prompt_level=0;
    gameover_recorded=false;ending_seen=false;page=HomePage;selection=0;upgrade_selection=0;autosave_ms=0;pending_ability=false;
    ++preferences.runs;save_preferences();if(preferences.autosave)save_run();else save_run(false);
}
static bool switch_language(unsigned at) {
    unsigned previous=preferences.language;
    if(!language.open(at)||!font.open(at)) {
        language.open(previous);font.open(previous);notice(tr("UiLanguageUnavailable"));return false;
    }
    preferences.language=at;
    if(at==9)preferences.flags|=Chinese;else preferences.flags&=~Chinese;
    renderer.setText(&language,&font);return true;
}
static bool hit(int x,int y,int w,int h) {
    return pointer_visible&&pointer_x>=x&&pointer_x<x+w&&pointer_y>=y&&pointer_y<y+h;
}
static bool pressed(const Input& input,uint32_t button){return (input.pressed&button)!=0;}
static void move_menu(const Input& in,int count,int y,int height,int x=16,int width=288,int first=0,int shown=99) {
    if(pressed(in,ButtonUp))selection=(selection+count-1)%count;
    if(pressed(in,ButtonDown))selection=(selection+1)%count;
    if(pointer_activity)for(int i=first;i<count&&i<first+shown;++i)
        if(hit(x,y+(i-first)*height,width,height))selection=i;
}
static bool activate(const Input& in,int y,int height,int x=16,int width=288,int first=0) {
    return pressed(in,ButtonConfirm)||(in.pointer_pressed&&hit(x,y+(selection-first)*height,width,height));
}
static void return_from_page(){page=return_page;selection=0;}
static void open_detail(Offer offer,Page origin) {
    detail_offer=offer;detail_return=origin;detail_selection=selection;
    detail_mode=-1;detail_tree=false;detail_scroll=0;detail_lines=0;page=DetailPage;
}
static void open_reference(const char* title,const char* key,Page origin) {
    reference_key[0]=reference_title[0]=0;append(reference_key,sizeof(reference_key),key);append(reference_title,sizeof(reference_title),title);
    detail_mode=-2;detail_return=origin;detail_selection=selection;detail_scroll=detail_lines=0;page=DetailPage;
}
static int catalog_count() {
    return catalog_tab==0?WEAPON_COUNT:catalog_tab==1?SHIELD_COUNT:catalog_tab==2?BODY_COUNT:catalog_tab==3?MOD_COUNT:ModeCount;
}
static Offer catalog_offer(int index) {
    return Offer{uint8_t(catalog_tab==0?OFFER_WEAPON:catalog_tab==1?OFFER_SHIELD:catalog_tab==2?OFFER_BODY:OFFER_MOD),uint8_t(index)};
}
static const char* enemy_name(unsigned enemy) {
    static const char* boss_keys[]={"EnemyNameScion","EnemyNameDreadnought","EnemyNameStationOmega","EnemyNameWarbringer","EnemyNameSeraph","EnemyNameStarEater","EnemyNameDweller","EnemyNameCeratotitan","EnemyNameGlaucus","EnemyNameCrucible"};
    if(enemy>=ENEMY_COUNT)return "";
    if(enemy==E_SPITFIRE)return tr("EnemyNameSpitfire",enemy_types[enemy].name);
    if(enemy>=E_SCION&&enemy-E_SCION<sizeof(boss_keys)/sizeof(boss_keys[0]))return tr(boss_keys[enemy-E_SCION],enemy_types[enemy].name);
    static const char* common[]={"UiAsteroid","UiSwarmling","UiStationDrone","UiVanguard","EnemyNameSpitfire","UiInterceptor","UiBulwark","UiHammerhead","UiSine","UiPulsebug","UiWarden","UiBeamcaster","UiCherub","UiJuggernaut","UiMinelayer","UiTracer"};
    static const char* extra[]={"UiHost","UiHive","UiHiveDrone","UiMyrmidon","UiPulsar","UiPulsarMine","UiSnapper","UiCelestial","UiCelestialDrone","UiEclipse","UiConstructor","UiConstructorDrone","UiStealthMine","UiShredderMine","UiSerpent","UiDwellerDrone","UiDwellerSoldier","UiDwellerGuard","UiDwellerWarrior","UiGyrogun","UiBoltThrower","UiCrabNest"};
    if(enemy>=E_SINGULARITY_MINE){static char name[192];name[0]=0;
        const char* family=enemy==E_CONSTRUCTOR_MISSILE?"UiConstructor":enemy==E_DWELLER_CHITIN?"EnemyNameDweller":enemy==E_JUGG_MISSILE||enemy==E_JUGG_MISSILE_HOMING?"UiJuggernaut":enemy==E_INTERCEPTOR_MINE?"UiInterceptor":enemy==E_SINGULARITY_MINE?"GearTitleWeaponVortex":"UiMinelayer";
        const char* category=enemy==E_MINE_DRONE?"UiStationDrone":enemy==E_MINE_SHIELD?"DrawEndScoreTitleShields":enemy==E_SINGULARITY_MINE||enemy==E_INTERCEPTOR_MINE?"ModTitleMines":"GlossaryProjectile";
        append(name,sizeof(name),tr(family));append(name,sizeof(name)," ");append(name,sizeof(name),tr(category));
        if(enemy==E_JUGG_MISSILE||enemy==E_JUGG_MISSILE_HOMING){append(name,sizeof(name)," ");appendNumber(name,sizeof(name),enemy==E_JUGG_MISSILE?1:2);}return name;
    }
    if(enemy==E_CRUCIBLE_DRONE){static char name[160];name[0]=0;append(name,sizeof(name),tr("EnemyNameCrucible"));append(name,sizeof(name)," ");append(name,sizeof(name),tr("UiStationDrone"));return name;}
    if(enemy>=48&&enemy<56){static char summon_names[8][160];unsigned at=enemy-48;char* text=summon_names[at];text[0]=0;
        const char* families[]={"UiHost","UiHost","UiHost","UiHiveDrone","UiConstructor","UiSwarmling","EnemyNameSeraph","EnemyNameSeraph"};append(text,160,tr(families[at]));
        if(at<3||at>=6){append(text,160," ");append(text,160,tr("UiStationDrone"));}
        else if(at==4){append(text,160," ");append(text,160,tr("ModTitleTurret"));}
        append(text,160," ");appendNumber(text,160,at<3?at+1:at==6?1:2);return text;
    }
    if(enemy>=E_HOST&&enemy-E_HOST<sizeof(extra)/sizeof(extra[0]))return tr(extra[enemy-E_HOST]);
    return enemy<sizeof(common)/sizeof(common[0])?tr(common[enemy]):enemy_types[enemy].name;
}
static void item_key(char* key,unsigned cap,const Offer& offer,const char* type) {
    static const char* weaponKeys[WEAPON_COUNT]={"Standard","SplitShot","Flak","Railgun","Grenade","Salvo","Vortex","Pulse","Torrent","BladeDrone","Dart","ThermalLance","Swords"};
    static const char* bodyKeys[BODY_COUNT]={"Standard","Assault","Hullbreaker","Stealth","Firefly","Sentinel","Engineer","Research","Battery","Architect","Viper","Carrier","Courser","Leviathan"};
    static const char* shieldKeys[SHIELD_COUNT]={"Standard","Halo","Reflect","Bastion","Temporal","Amp","Warp","Helix","Siphon","Shockwave"};
    key[0]=0;
    if(offer.kind==OFFER_MOD&&offer.id<MOD_COUNT){
        append(key,cap,"Mod");append(key,cap,type);
        append(key,cap,offer.id==M_SHIELD_RADIUS&&type[0]!='T'?"ShieldRadius":mods[offer.id].source_key);
    }
    else {
        append(key,cap,"Gear");append(key,cap,type);
        if(offer.kind==OFFER_WEAPON&&offer.id<WEAPON_COUNT){append(key,cap,"Weapon");append(key,cap,offer.id==W_BLADE&&type[0]!='T'?"Blade":weaponKeys[offer.id]);}
        if(offer.kind==OFFER_BODY&&offer.id<BODY_COUNT){append(key,cap,"Body");append(key,cap,bodyKeys[offer.id]);}
        if(offer.kind==OFFER_SHIELD&&offer.id<SHIELD_COUNT){append(key,cap,"Shield");append(key,cap,shieldKeys[offer.id]);}
    }
}
static const char* item_name(const Offer& offer,char* buffer,unsigned capacity) {
    char key[96];item_key(key,sizeof(key),offer,"Title");
    const char* value=language.get(key);if(!value)return offerName(offer);
    Locale::plain(buffer,capacity,value,true);return buffer;
}
static const char* item_description(const Offer& offer,char* buffer,unsigned capacity,bool full=false) {
    char key[96];item_key(key,sizeof(key),offer,full?"Full":"Short");
    const char* value=language.get(key);
    if(!value&&full){item_key(key,sizeof(key),offer,"Short");value=language.get(key);}
    if(!value&&!full){item_key(key,sizeof(key),offer,"Full");value=language.get(key);}
    if(!value)return offerDescription(offer);
    if(full)Locale::format(buffer,capacity,value,true);else Locale::plain(buffer,capacity,value);
    return buffer;
}
static const ItemTags& item_tags(const Offer& o) {
    static const ItemTags empty={0,{0}};
    if(o.kind==OFFER_MOD&&o.id<MOD_COUNT)return modTags[o.id];
    if(o.kind==OFFER_WEAPON&&o.id<WEAPON_COUNT)return weaponTags[o.id];
    if(o.kind==OFFER_BODY&&o.id<BODY_COUNT)return bodyTags[o.id];
    if(o.kind==OFFER_SHIELD&&o.id<SHIELD_COUNT)return shieldTags[o.id];
    return empty;
}
// White source tag strips with clear translated type. Rows participate in the
// description's vertical scrolling so eight tags never cover the body text.
static int render_tags(const Offer& o,int y,int skip,int capacity) {
    const ItemTags& tags=item_tags(o);if(!tags.count)return 0;
    Canvas& c=renderer.canvas;int row=0,x=13;
    if(skip==0&&capacity>0)c.text(x,y,tr("UiTags"),Muted);
    row=1;
    for(unsigned i=0;i<tags.count;++i){
        const char* label=tr(tagKeys[tags.ids[i]]);int width=Canvas::textWidth(label)+12;
        if(width>292)width=292;
        if(x+width>307){++row;x=13;}
        if(row>=skip&&row<skip+capacity){int py=y+(row-skip)*Canvas::lineHeight();
            c.fill(x+2,py,width-4,15,White);c.fill(x,py+3,width,9,White);
            c.clipped(x+6,py,width-12,label,Panel);
        }
        x+=width+5;
    }
    return row+2;
}
static int build_count(){int count=3;for(unsigned i=0;i<MOD_COUNT;++i)if(game.player.mods[i])++count;return count;}
static Offer build_offer(int index) {
    if(index==0)return Offer{OFFER_WEAPON,game.player.weapon};
    if(index==1)return Offer{OFFER_SHIELD,game.player.shield_type};
    if(index==2)return Offer{OFFER_BODY,game.player.body};
    index-=3;
    for(unsigned i=0;i<MOD_COUNT;++i)if(game.player.mods[i]){if(index==0)return Offer{OFFER_MOD,uint8_t(i)};--index;}
    return Offer{OFFER_WEAPON,0};
}
static void open_daily();
static void process_home(const Input& in) {
    move_menu(in,11,44,16,7,234);
    if(!has_save&&selection==0)selection=pressed(in,ButtonUp)?10:1;
    if(pressed(in,ButtonBack)){quitting=true;return;}
    if(!activate(in,44,16,7,234))return;
    int action=selection;
    if(action==0&&has_save){if(load_run()){page=ResumePage;selection=0;}else{has_save=false;notice(tr("UiSaveUnavailable"));}}
    else if(action==1){page=ModePage;selection=0;}
    else if(action==2){page=CatalogPage;catalog_tab=0;reference_return=HomePage;selection=0;}
    else if(action==3){page=LearnPage;return_page=HomePage;learn_tab=0;detail_scroll=0;selection=0;}
    else if(action==4){page=GlossaryPage;reference_return=HomePage;selection=0;}
    else if(action==5){
        reference_title[0]=0;append(reference_title,sizeof(reference_title),tr("MenuChangelog","Changelog"));
        if(language.openFile("nNova.changelog.dat.tns")){page=ChangelogPage;reference_return=HomePage;detail_scroll=0;}
        else{language.open(preferences.language);notice(tr("UiUnavailable"));}
    }
    else if(action==6){page=SettingsPage;return_page=HomePage;selection=0;}
    else if(action==7){page=ScoresPage;scores_return=HomePage;selection=0;}
    else if(action==8){page=CreditsPage;credits_return=HomePage;detail_scroll=0;selection=0;}
    else if(action==9){
        reference_title[0]=reference_body[0]=0;append(reference_title,sizeof(reference_title),tr("MenuDiscord","Discord"));
        append(reference_body,sizeof(reference_body),"https://discord.gg/Zp2MkPJ");detail_mode=-4;detail_tree=false;
        detail_return=HomePage;detail_selection=selection;detail_scroll=0;page=DetailPage;
    }else if(action==10)quitting=true;
}
static void process_mode(const Input& in) {
    int first=selection/4*4;move_menu(in,ModeCount+2,41,33,12,296,first,4);
    if(pressed(in,ButtonBack)){page=HomePage;selection=0;return;}
    if(pressed(in,ButtonTab)&&selection>0&&selection<=ModeCount) {
        detail_mode=selection-1;detail_return=ModePage;detail_selection=selection;detail_scroll=detail_lines=0;page=DetailPage;return;
    }
    first=selection/4*4;
    if(!activate(in,41,33,12,296,first)&&!pressed(in,ButtonLeft)&&!pressed(in,ButtonRight))return;
    if(selection==0) {
        if(!activate(in,41,33,12,296,first))return;
        pending_run_day=0;
        if(has_save||incompatible_save){page=NewRunPage;selection=0;}else start_run();
    } else if(selection<=ModeCount){
        unsigned flag=mode_choices[selection-1].flag;if(!career.modeUnlocked(flag)){notice(tr(flag?"LoadoutLocked":"UiUnavailable"));return;}preferences.run_flags^=flag;
        if(flag==RUN_DAILY){preferences.run_flags&=~RUN_DAILY;open_daily();return;}
        const unsigned base=RUN_DANGER|RUN_ENEMY_TERRITORY|RUN_ELITE|RUN_HOSTILE|RUN_NEMESIS;
        if((flag==RUN_MAYHEM||flag==RUN_ANNIHILATION)&&(preferences.run_flags&flag))preferences.run_flags|=base;
        if(flag==RUN_ANNIHILATION&&(preferences.run_flags&flag))preferences.run_flags|=RUN_MAYHEM;
        if(!(preferences.run_flags&flag)&&(flag&base))preferences.run_flags&=~(RUN_MAYHEM|RUN_ANNIHILATION);
        if(flag==RUN_MAYHEM&&!(preferences.run_flags&flag))preferences.run_flags&=~RUN_ANNIHILATION;
        if(preferences.run_flags&RUN_WILD)preferences.flags|=Wild;else preferences.flags&=~Wild;
        save_preferences();
    }
    else {page=HomePage;selection=0;}
}
static void process_settings(const Input& in) {
    if(pressed(in,ButtonLeft)||pressed(in,ButtonRight)) {
        settings_tab=(settings_tab+(pressed(in,ButtonLeft)?5:1))%6;selection=0;return;
    }
    if(pointer_activity)for(int tab=0;tab<6;++tab)if(hit(8+(tab%3)*103,35+(tab/3)*19,100,19)){settings_tab=tab;selection=0;return;}
    if(pressed(in,ButtonBack)||pressed(in,ButtonPause)){return_from_page();return;}
    if(pressed(in,ButtonTab)){unsigned item=settings_rows[settings_tab][selection];if(*setting_descriptions[item])open_reference(tr(setting_keys[item]),setting_descriptions[item],SettingsPage);return;}
    int count=settings_counts[settings_tab];int first=selection/4*4;
    move_menu(in,count,85,26,12,296,first,4);first=selection/4*4;
    if(!activate(in,85,26,12,296,first))return;
    unsigned item=settings_rows[settings_tab][selection];
    if(item==5){page=LanguagePage;selection=int(preferences.language);return;}
    if(item==6){page=ControlsPage;controls_return=SettingsPage;controls_tab=0;selection=0;return;}
    if(item==8){page=ScoresPage;scores_return=SettingsPage;selection=0;return;}
    if(item==9){page=CreditsPage;credits_return=SettingsPage;detail_scroll=0;selection=0;return;}
    if(item==11){page=RebindPage;selection=0;input_controller.clearRebindStatus();return;}
    if(item==12){page=ProgressPage;progress_return=SettingsPage;progress_selection=career.rank<64?career.rank:63;return;}
    if(item==13||item==14){page=ResetPage;reset_progress=item==14;selection=0;return;}
    if(item==16&&!(preferences.flags&AssistMode)){page=AssistPage;selection=0;return;}
    if(item==17)preferences.ataraxia_reminders=(preferences.ataraxia_reminders+1)%3;
    else if(item==18)preferences.autosave=(preferences.autosave+1)%3;
    else if(item>=19&&item<=22)preferences.auto_deploy^=1u<<(item-19);
    else if(item==30)preferences.background_detail=(preferences.background_detail+1)%4;
    else if(item==33){if(!skins.count()){notice(tr("UiUnavailable"));return;}unsigned next=(preferences.skin+1)%(skins.count()+1);if(skins.select(next))preferences.skin=next;else{notice(tr("UiUnavailable"));return;}}
    else if(item==34){open_reference(tr("SettingsTitleAudio"),"UiAudioDetails",SettingsPage);return;}
    else preferences.flags^=setting_flags[item];
    apply_settings();if(!save_preferences())notice(tr("UiSaveFailed"));
}
static void process_languages(const Input& in) {
    int first=selection/8*8;move_menu(in,Locale::Count,44,21,28,264,first,8);
    if(pressed(in,ButtonBack)){page=SettingsPage;selection=0;return;}
    first=selection/8*8;
    if(activate(in,44,21,28,264,first)&&switch_language(unsigned(selection)))save_preferences();
}
static void process_learn(const Input& in) {
    if(pressed(in,ButtonLeft)){learn_tab=(learn_tab+19)%20;detail_scroll=0;}
    if(pressed(in,ButtonRight)){learn_tab=(learn_tab+1)%20;detail_scroll=0;}
    if(pressed(in,ButtonUp)&&detail_scroll>0)--detail_scroll;
    if(pressed(in,ButtonDown)&&detail_scroll+detail_visible<detail_lines)++detail_scroll;
    if(pressed(in,ButtonTab)){page=BestiaryPage;reference_return=LearnPage;selection=0;return;}
    if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)){page=HomePage;selection=3;}
}
static int super_count(){int n=0;for(unsigned i=0;i<MOD_COUNT;++i)if(mods[i].category==1)++n;return n;}
static Offer super_offer(int at){for(unsigned i=0;i<MOD_COUNT;++i)if(mods[i].category==1){if(!at--)return Offer{OFFER_MOD,uint8_t(i)};}return Offer{OFFER_MOD,0};}
static void process_progress(const Input& in) {
    if(pressed(in,ButtonUp))progress_selection=(progress_selection+63)%64;
    if(pressed(in,ButtonDown))progress_selection=(progress_selection+1)%64;
    if(pressed(in,ButtonLeft)){progress_selection-=4;if(progress_selection<0)progress_selection=0;}
    if(pressed(in,ButtonRight)){progress_selection+=4;if(progress_selection>63)progress_selection=63;}
    int first=progress_selection/4*4;
    if(pointer_activity)for(int i=0;i<4;++i)if(hit(12,103+i*25,296,25))progress_selection=first+i;
    if(pressed(in,ButtonTab)){page=CatalogPage;catalog_tab=0;selection=0;reference_return=ProgressPage;return;}
    if(pressed(in,ButtonConfirm)) {
        const char* key=Career::unlockKey(unsigned(progress_selection+1));if(key[0]=='s'&&key[1]=='t'&&key[2]=='r')key+=3;
        open_reference(tr("DrawEndScoreNextReward"),key,ProgressPage);return;
    }
    if(pressed(in,ButtonBack)){page=progress_return;selection=0;}
}
static void process_rebind(const Input& in) {
    if(input_controller.capturingBinding())return;
    if(input_controller.rebindStatus()==RebindStatus::Applied){if(!input_controller.saveBindings())notice(tr("UiSaveFailed"));input_controller.clearRebindStatus();return;}
    if(input_controller.rebindStatus()==RebindStatus::Cancelled){input_controller.clearRebindStatus();return;}
    int first=selection/5*5;move_menu(in,BindCount+1,44,25,12,296,first,5);
    if(pressed(in,ButtonBack)){page=SettingsPage;selection=3;return;}
    first=selection/5*5;
    if(activate(in,44,25,12,296,first)) {
        if(selection==BindCount){input_controller.resetBindings();if(!input_controller.saveBindings())notice(tr("UiSaveFailed"));}
        else input_controller.beginRebind(BindAction(selection));
    }
}
static void process_reference_list(const Input& in) {
    int count=page==GlossaryPage?GlossaryCount:page==BestiaryPage?ENEMY_COUNT:page==SuperModsPage?super_count():catalog_count();
    if(page==CatalogPage) {
        if(pressed(in,ButtonLeft)||pressed(in,ButtonRight)){catalog_tab=(catalog_tab+(pressed(in,ButtonLeft)?4:1))%5;selection=0;return;}
        if(pressed(in,ButtonTab)){page=ProgressPage;progress_return=CatalogPage;progress_selection=career.rank<64?career.rank:63;return;}
        if(pressed(in,ButtonUp))selection=(selection+count-1)%count;
        if(pressed(in,ButtonDown))selection=(selection+1)%count;
        int first=selection/12*12;
        if(pointer_activity)for(int i=0;i<12&&first+i<count;++i)if(hit(28+(i%6)*47,66+(i/6)*49,44,44))selection=first+i;
        if(pressed(in,ButtonBack)){page=reference_return;selection=0;return;}
        if(!pressed(in,ButtonConfirm)&&!in.pointer_pressed)return;
        if(catalog_tab==4){detail_mode=selection;detail_return=CatalogPage;detail_selection=selection;detail_scroll=detail_lines=0;page=DetailPage;}
        else open_detail(catalog_offer(selection),CatalogPage);
        return;
    }
    int first=selection/6*6;move_menu(in,count,43,25,12,296,first,6);
    if(page==GlossaryPage&&pressed(in,ButtonTab)){page=BestiaryPage;reference_return=GlossaryPage;selection=0;return;}
    if(pressed(in,ButtonBack)){page=reference_return;selection=0;return;}
    first=selection/6*6;
    if(!activate(in,43,25,12,296,first)&&!pressed(in,ButtonTab))return;
    if(page==GlossaryPage){char key[96]={0};append(key,sizeof(key),glossary_keys[selection]);append(key,sizeof(key),"Def");open_reference(glossary_title(unsigned(selection)),key,GlossaryPage);}
    else if(page==BestiaryPage){reference_enemy=selection;detail_mode=-3;detail_return=BestiaryPage;detail_selection=selection;page=DetailPage;}
    else open_detail(super_offer(selection),SuperModsPage);
}
static void process_build(const Input& in) {
    int count=build_count();
    if(pressed(in,ButtonUp))build_selection=(build_selection+count-1)%count;
    if(pressed(in,ButtonDown))build_selection=(build_selection+1)%count;
    if(pressed(in,ButtonLeft)){build_selection-=6;if(build_selection<0)build_selection=0;}
    if(pressed(in,ButtonRight)){build_selection+=6;if(build_selection>=count)build_selection=count-1;}
    int first=build_selection/6*6;
    if(pointer_activity)for(int i=0;i<6&&first+i<count;++i)
        if(hit(10,42+i*25,300,25))build_selection=first+i;
    if(pressed(in,ButtonConfirm)||pressed(in,ButtonTab)||
       (in.pointer_pressed&&hit(10,42+(build_selection-first)*25,300,25)))open_detail(build_offer(build_selection),BuildPage);
    else if(pressed(in,ButtonBack)||pressed(in,ButtonPause))return_from_page();
}
static const int hex_x[7]={53,87,36,70,104,53,87};
static const int hex_y[7]={64,64,94,94,94,124,124};
static int offer_x(unsigned i){return game.gearSelectionComplete()?hex_x[i]:i==0?48:i==1?91:70;}
static int offer_y(unsigned i){return game.gearSelectionComplete()?hex_y[i]:i<2?81:119;}
static void process_upgrade(const Input& in) {
    bool canBanish=game.player.body==B_RESEARCH&&game.gearSelectionComplete();
    int offers=game.offer_count;bool initial=!game.gearSelectionComplete();
    int count=initial?offers:offers+4+(canBanish?1:0);int selected=upgrade_selection;
    if(selected<0||selected>=count)selected=0;
    if(pressed(in,ButtonLeft)||pressed(in,ButtonUp))selected=(selected+count-1)%count;
    if(pressed(in,ButtonRight)||pressed(in,ButtonDown))selected=(selected+1)%count;
    if(pointer_activity) {
        for(int i=0;i<offers&&i<7;++i)if(hit(offer_x(i)-18,offer_y(i)-20,36,40))selected=i;
        if(!initial&&hit(34,161,36,38))selected=offers;
        if(!initial&&hit(70,161,36,38))selected=offers+1;
        if(!initial&&hit(52,190,36,38))selected=offers+2;
        if(!initial&&hit(10,22,125,22))selected=offers+3;
        if(canBanish&&hit(144,178,166,19))selected=offers+4;
    }
    upgrade_selection=selected;
    if(selected<offers)game.selected_offer=uint8_t(selected);
    if(pressed(in,ButtonReroll)||(in.pointer_pressed&&hit(190,22,126,19))) {
        if(!game.rerollOffers())notice(tr("UiRerollUnavailable"));
    }
    if((pressed(in,ButtonTab)||(in.pointer_pressed&&hit(143,193,164,23)))&&selected<offers) {
        open_detail(game.offers[game.selected_offer],HomePage);return;
    }
    int chosen=-1;
    if(pressed(in,ButtonConfirm))chosen=selected;
    if(in.pointer_pressed) {
        if(selected<offers&&hit(offer_x(unsigned(selected))-18,offer_y(unsigned(selected))-20,36,40))chosen=selected;
        else if(selected==offers&&hit(34,161,36,38))chosen=selected;
        else if(selected==offers+1&&hit(70,161,36,38))chosen=selected;
        else if(selected==offers+2&&hit(52,190,36,38))chosen=selected;
        else if(selected==offers+3&&hit(10,22,125,22))chosen=selected;
        else if(canBanish&&selected==offers+4&&hit(144,178,166,19))chosen=selected;
    }
    if(canBanish&&(chosen==offers+4||pressed(in,ButtonBanish))){
        if(!game.banishOffer(game.selected_offer))notice(tr("UiRerollUnavailable"));
        else autosave_run();
        upgrade_selection=game.selected_offer;return;
    }
    if(chosen>=offers){
        const unsigned kinds[4]={OFFER_WEAPON,OFFER_SHIELD,OFFER_BODY,OFFER_MOD};
        game.prepareGearOffers(kinds[chosen-offers]);upgrade_selection=0;return;
    }
    if(chosen>=0&&game.chooseOffer(unsigned(chosen))){
        if(game.state==Upgrade&&((preferences.flags&DisableUpgradeChain)||(game.player.mods[M_X_ATARAXIA]&&preferences.ataraxia_reminders==2)))game.bankUpgrade();
        ataraxia_prompt_level=game.level;autosave_run();return;}
    if(pressed(in,ButtonPause)||pressed(in,ButtonBack)||pressed(in,ButtonMenu)||pressed(in,ButtonUpgrade))
        if(game.bankUpgrade()){pending_ability=false;ataraxia_prompt_level=game.level;autosave_run();}
}
static void process_pause(const Input& in) {
    if(pressed(in,ButtonUpgrade)&&game.banked_upgrades){game.state=Playing;game.reopenUpgrade();selection=0;return;}
    if(pressed(in,ButtonTab)||(in.pointer_pressed&&hit(12,170,296,26))){page=StatsPage;return_page=HomePage;stats_tab=stats_subpage=0;return;}
    if(pressed(in,ButtonLeft)||(in.pointer_pressed&&hit(12,42,110,117))){page=BuildPage;return_page=HomePage;build_selection=0;return;}
    move_menu(in,4,52,26,127,187);
    if(pressed(in,ButtonBack)||pressed(in,ButtonPause)||pressed(in,ButtonMenu)){game.state=Playing;game.resetInput();selection=0;return;}
    if(!activate(in,52,26,127,187))return;
    if(selection==0){game.state=Playing;game.resetInput();selection=0;}
    else if(selection==1){page=SettingsPage;return_page=HomePage;selection=0;}
    else if(selection==2){page=SuperModsPage;reference_return=HomePage;selection=0;}
    else if(save_run()){game.state=Title;selection=0;}
}
static void process_gameover(const Input& in) {
    if(pressed(in,ButtonTab)){page=ProgressPage;progress_return=HomePage;progress_selection=gained_ranks?int(career.rank-gained_ranks):int(career.rank<64?career.rank:63);return;}
    move_menu(in,4,133,23,93,220);
    if(pressed(in,ButtonBack)){game.state=Title;selection=0;return;}
    if(!activate(in,133,23,93,220))return;
    if(selection==0)start_run((game.run_flags&RUN_DAILY)?game.daily_day:0);
    else if(selection==1){page=StatsPage;return_page=HomePage;stats_tab=stats_subpage=0;}
    else if(selection==2){game.state=Title;selection=0;}
    else quitting=true;
}
static void demo_frame(uint32_t elapsed) {
    if(game.state==Title||game.state==GameOver){start_run();return;}
    if(game.state==Upgrade){game.chooseOffer((demo_ticks/90)%OFFER_COUNT);return;}
    GameInput in={};in.up=true;in.fire=true;
    int target=-1,best=0x7fffffff;
    for(unsigned i=0;i<MAX_ENEMIES;++i)if(game.enemies[i].active){
        int dx=pixel(game.enemies[i].x-game.player.x),dy=pixel(game.enemies[i].y-game.player.y);
        int distance=dx*dx+dy*dy;if(distance<best){best=distance;target=int(i);}
    }
    if(target>=0){in.pointer_valid=true;in.pointer_x=int16_t(pixel(game.enemies[target].x));in.pointer_y=int16_t(pixel(game.enemies[target].y));}
    else in.right=(demo_ticks/60)%2==0;
    if(game.player.weapon==W_SALVO||game.player.weapon==W_VORTEX)in.fire=(demo_ticks%90)>20;
    in.ability=demo_ticks%150==0;game.advance(elapsed,in);++demo_ticks;
}
static void panel(int x,int y,int w,int h) {renderer.canvas.shade(x,y,w,h,Panel,235);}
static void divider(int x,int y,int width) {
    if(!renderer.ui("sDivider",x+width/2,y,width))renderer.canvas.fill(x,y,width,1,rgb(83,85,115));
}
static void button(int x,int y,int w,int h,const char* title,bool selected,int reserved=0) {
    Canvas& c=renderer.canvas;
    if(selected) {
        c.shade(x+14,y,w-14,h,rgb(41,33,71),130);
        if(!renderer.ui("sMenuCursor",x+6,y+h/2,13)){
            c.line(x+1,y+h/2-3,x+8,y+h/2,White);c.line(x+8,y+h/2,x+1,y+h/2+3,White);
        }
    }
    c.clipped(x+19,y+(h-Canvas::lineHeight())/2,w-22-reserved,title,selected?White:Muted);
}
static void heading(const char* title) {
    renderer.canvas.centered(8,title,White);divider(12,33,296);
}
static void footer(const char* action,const char* key="5 / Enter") {
    Canvas& c=renderer.canvas;c.shade(0,220,320,20,Panel,235);
    char line[160]={0};append(line,sizeof(line),key);append(line,sizeof(line),": ");append(line,sizeof(line),action);
    c.clipped(8,222,200,line,Muted);
    line[0]=0;append(line,sizeof(line),"Esc: ");append(line,sizeof(line),tr("UiBack"));c.right(312,222,line,Muted);
}
static void value_label(int x,int y,const char* key,unsigned value,uint16_t color) {
    char line[128]={0};append(line,sizeof(line),tr(key,key));append(line,sizeof(line)," ");appendNumber(line,sizeof(line),value);
    renderer.canvas.text(x,y,line,color);
}
#include "app_daily.inc"
static void render_home() {
    renderer.titleArt(clock_ms);Canvas::setFont(&font);
    const char* keys[]={"MenuResumeRun","MenuStart","MenuUnlocks","MenuLearn","MenuGlossary","MenuChangelog","MenuSettings","MenuScore","CreditsTitle","MenuDiscord","MenuExit"};
    const char* fallback[]={"Resume Run","Game Start","Unlocks","Learn","Glossary","Changelog","Settings","Scores","Credits","Discord","Exit"};
    for(int i=0;i<11;++i){
        int y=44+i*16;bool selected=i==selection&&(i||has_save);
        if(selected){renderer.uiStrip("sBarFlashBlue",-24,y-2,240,19,220);renderer.uiStrip("sGUIGradientBlue",0,y+7,246,2,255);}
        renderer.canvas.clipped(9,y,234,tr(keys[i],fallback[i]),!i&&!has_save?rgb(95,99,121):selected?White:Muted);
    }
    char mode[128]="0: ";append(mode,sizeof(mode),tr(input_controller.pointerMode()?"UiTouchAim":"UiKeypad"));
    renderer.canvas.clipped(8,222,190,mode,Muted);renderer.canvas.right(312,222,Locale::name(preferences.language),Muted);
}
static void render_mode() {
    Canvas& c=renderer.canvas;heading(tr("LoadoutChallenge","CHALLENGE MODES"));
    int first=selection/4*4;
    for(int row=0;row<4&&first+row<ModeCount+2;++row) {
        int i=first+row,y=41+row*33;
        if(i==0){renderer.ui("sStart",32,y+16,34);button(46,y,262,33,tr("LoadoutStart","START"),selection==i);continue;}
        if(i==ModeCount+1){button(12,y,296,33,tr("UiBack"),selection==i);continue;}
        const ModeChoice& mode=mode_choices[i-1];bool unlocked=career.modeUnlocked(mode.flag);bool enabled=unlocked&&(preferences.run_flags&mode.flag)!=0;
        if(selection==i)c.shade(10,y,300,32,rgb(38,29,65),160);
        renderer.ui(mode.sprite,32,y+16,30,65535,unlocked?(enabled?255:170):70);
        if(enabled)renderer.ui("sChallengeModeSelecta",32,y+16,34);
        c.clipped(54,y+6,201,tr(mode.title,mode.title),selection==i?White:Muted);
        c.right(305,y+6,unlocked?(mode.flag==RUN_DAILY?tr("UiChoose"):enabled?tr("UiOn"):tr("UiOff")):tr(mode.flag?"UiLocked":"UiUnavailable"),enabled?Cyan:Muted);
    }
    divider(12,177,296);panel(12,181,296,36);
    const char* description=selection>0&&selection<=ModeCount?tr(mode_choices[selection-1].description):tr("LoadoutRisk","GREATER RISK");
    c.wrapped(16,182,288,2,description,Muted);
    char position[40]={0};appendNumber(position,sizeof(position),unsigned(selection+1));append(position,sizeof(position)," / ");appendNumber(position,sizeof(position),ModeCount+2);c.right(304,24,position,Muted);
    footer(tr("UiChange"));
}
static const char* setting_label(unsigned i) {return tr(setting_keys[i]);}
static void render_settings() {
    Canvas& c=renderer.canvas;heading(tr("SettingsTitle","SETTINGS"));
    for(int tab=0;tab<6;++tab) {
        int x=8+(tab%3)*103,y=36+(tab/3)*19;
        if(tab==settings_tab)renderer.uiStrip("sBarFlashBlue",x-4,y-1,107,20,180);
        c.clipped(x,y,99,tr(settings_titles[tab]),tab==settings_tab?White:Cyan);
    }
    divider(12,77,296);int first=selection/4*4;
    for(int row=first;row<settings_counts[settings_tab]&&row<first+4;++row) {
        unsigned i=settings_rows[settings_tab][row];int y=85+(row-first)*26;const char* value="";
        if(i==1)value=(preferences.flags&FullEffects)?tr("UiFull"):tr("UiReduced");
        else if(i==4)value=(preferences.flags&PointerAim)?tr("UiTouchAim"):tr("UiKeypad");
        else if(i==5)value=Locale::name(preferences.language);
        else if(i==17){const char* values[]={"SettingsAutoSaveAlways","SettingsOnce","SettingsNever"};value=tr(values[preferences.ataraxia_reminders]);}
        else if(i==18){const char* values[]={"SettingsAutoSaveNever","SettingsAutoSaveNormal","SettingsAutoSaveAlways"};value=tr(values[preferences.autosave]);}
        else if(i>=19&&i<=22)value=tr(preferences.auto_deploy&(1u<<(i-19))?"UiOn":"UiOff");
        else if(i==30){const char* values[]={"SettingsNone","SettingsLow","SettingsMedium","SettingsHigh"};value=tr(values[preferences.background_detail]);}
        else if(i==33)value=preferences.skin?skins.name(preferences.skin-1):tr("SettingsNormal");
        else if(i==34)value=tr("UiOff");
        else if(setting_flags[i])value=tr(preferences.flags&setting_flags[i]?"UiOn":"UiOff");
        int reserved=*value?Canvas::textWidth(value)+12:0;if(reserved>110)reserved=110;
        button(12,y,296,24,setting_label(i),selection==row,reserved);
        if(*value){c.shade(307-reserved,y,reserved,23,Panel,250);c.clipped(307-reserved+3,y+3,reserved-5,value,selection==row?Cyan:Muted);}
    }
    unsigned item=settings_rows[settings_tab][selection];divider(12,192,296);
    if(settings_tab==4)c.wrapped(16,119,288,4,tr("UiAudioDetails"),Muted);
    const char* description=setting_descriptions[item];
    if(*description){c.clipped(12,194,220,tr(description),Muted);char help[96]={0};append(help,sizeof(help),physical_key_name(input_controller.bindings().key[BindAbility]));append(help,sizeof(help),": ");append(help,sizeof(help),tr("UiDetails"));c.right(308,194,help,Cyan);}
    char position[40]={0};appendNumber(position,sizeof(position),unsigned(selection+1));append(position,sizeof(position)," / ");appendNumber(position,sizeof(position),settings_counts[settings_tab]);c.right(309,21,position,Muted);
    footer(tr(settings_tab==4?"UiDetails":"UiChange"));
}
static void render_languages() {
    Canvas& c=renderer.canvas;heading(tr("SettingsTitleLanguage","LANGUAGE"));
    int first=selection/8*8;
    for(int i=first;i<Locale::Count&&i<first+8;++i) {
        int y=44+(i-first)*21;button(28,y,264,21,Locale::name(unsigned(i)),selection==i);
        if(unsigned(i)==preferences.language)c.right(285,y+2,"*",Cyan);
        else if(!language_available[i])c.right(285,y+2,"-",Muted);
    }
    char position[32]={0};appendNumber(position,sizeof(position),unsigned(selection+1));append(position,sizeof(position)," / 11");c.right(304,25,position,Muted);
    footer(tr("UiChoose"));
}
static void render_controls() {
    Canvas& c=renderer.canvas;heading(tr("SettingsTitleControls","CONTROLS"));
    char turnKeys[48]={0};append(turnKeys,sizeof(turnKeys),physical_key_name(input_controller.bindings().key[BindLeft]));append(turnKeys,sizeof(turnKeys)," / ");append(turnKeys,sizeof(turnKeys),physical_key_name(input_controller.bindings().key[BindRight]));
    const char* keys0[]={physical_key_name(input_controller.bindings().key[BindThrust]),turnKeys,physical_key_name(input_controller.bindings().key[BindBrake]),physical_key_name(input_controller.bindings().key[BindFire]),physical_key_name(input_controller.bindings().key[BindAbility]),"0"};
    const char* labels0[]={tr("DefineKeyboardControlsThrust","Thrust"),tr("UiTurn"),tr("UiBrake"),tr("DefineKeyboardControlsFire","Fire"),tr("UiAbility"),tr("UiInputMode")};
    const char* keys1[]={"8 / 2 / 4 / 6","5 / Enter","Esc",physical_key_name(input_controller.bindings().key[BindUpgrade]),physical_key_name(input_controller.bindings().key[BindReroll]),physical_key_name(input_controller.bindings().key[BindAbility]),physical_key_name(input_controller.bindings().key[BindBanish])};
    const char* labels1[]={tr("DefineKeyboardControlsMenu","Menu"),tr("UiChoose"),tr("UiBack"),tr("DefineKeyboardControlsUpgrade","Upgrade"),tr("DefineKeyboardControlsReroll","Reroll"),tr("UiDetails"),tr("TutorialBanish2")};
    for(int i=0;i<(controls_tab?7:6);++i){int y=controls_tab?43+i*19:48+i*22;c.text(12,y,controls_tab?keys1[i]:keys0[i],Cyan);c.clipped(132,y,178,controls_tab?labels1[i]:labels0[i],White);}
    c.wrapped(12,185,296,2,input_controller.pointerMode()?tr("UiMoveTouch"):tr("UiPressTouch"),Muted);
    footer(tr("UiPage"),"4 / 6");
}
static void render_upgrade() {
    int offers=game.offer_count;bool initial=!game.gearSelectionComplete();Canvas& c=renderer.canvas;renderer.menuBackground(clock_ms,false);Canvas::setFont(&font);
    c.clipped(12,3,260,tr("UpgradeUpgrade","UPGRADE"),White);
    char number[24];decimal(number,game.banked_upgrades);c.right(307,3,number,Cyan);
    char rr[128]={0};append(rr,sizeof(rr),tr("UpgradeRerolls","Rerolls:"));append(rr,sizeof(rr)," ");appendNumber(rr,sizeof(rr),game.rerolls);
    c.right(307,22,rr,Cyan);divider(12,21,130);
    c.clipped(12,25,124,initial?tr("UpgradeChooseGear"):tr("UpgradeChooseMod","CHOOSE MOD"),upgrade_selection==offers+3?Cyan:White);
    if(upgrade_selection==offers+3)divider(12,41,122);
    for(int i=0;i<offers&&i<7;++i){
        renderer.icon(game.offers[i],offer_x(i),offer_y(i),41);
        if(int(i)==upgrade_selection)renderer.selectIcon(offer_x(i),offer_y(i),48,true);
    }
    if(!initial){c.clipped(10,150,123,tr("UpgradeChooseGear","CHANGE GEAR"),White);
    renderer.icon(Offer{OFFER_WEAPON,game.player.weapon},52,180,36);
    renderer.icon(Offer{OFFER_SHIELD,game.player.shield_type},88,180,36);
    renderer.icon(Offer{OFFER_BODY,game.player.body},70,209,36);
    if(upgrade_selection==offers)renderer.selectIcon(52,180,43);
    if(upgrade_selection==offers+1)renderer.selectIcon(88,180,43);
    if(upgrade_selection==offers+2)renderer.selectIcon(70,209,43);
    }else{c.wrapped(10,154,123,4,tr("TutorialDecline2"),Muted);}
    panel(140,46,174,148);
    char name[192],description[2048];
    int titleLines=c.wrapped(146,49,162,3,item_name(game.offers[game.selected_offer],name,sizeof(name)),White);
    int y=49+titleLines*Canvas::lineHeight()+2;divider(146,y,162);
    int lines=((game.player.body==B_RESEARCH?174:188)-y)/Canvas::lineHeight();if(lines<1)lines=1;
    c.wrapped(146,y+4,162,lines,item_description(game.offers[game.selected_offer],description,sizeof(description)),Cyan);
    if(game.player.body==B_RESEARCH) {
        if(upgrade_selection==offers+4)c.shade(144,178,166,19,rgb(50,30,54),220);
        c.clipped(146,178,162,tr("TutorialBanish2","Banish Mod"),upgrade_selection==offers+4?White:Red);
    }
    char action[160]={0};append(action,sizeof(action),physical_key_name(input_controller.bindings().key[BindAbility]));append(action,sizeof(action),": ");append(action,sizeof(action),tr("UiDetails"));c.clipped(146,197,164,action,White);
    footer(tr("UiChoose"));
}
static void render_pause() {
    renderer.menuBackground(clock_ms,false);Canvas::setFont(&font);heading(tr("PauseMenuPaused","PAUSED"));
    renderer.icon(Offer{OFFER_WEAPON,game.player.weapon},47,75,42);renderer.icon(Offer{OFFER_SHIELD,game.player.shield_type},87,75,42);renderer.icon(Offer{OFFER_BODY,game.player.body},67,111,42);
    char hint[128]="4: ";append(hint,sizeof(hint),tr("UiYourBuild"));renderer.canvas.clipped(12,140,112,hint,Cyan);
    const char* items[]={tr("PauseMenuResumeGame","Resume Run"),tr("MenuSettings","Settings"),tr("PauseMenuShowSupermods","Show Super Mods"),tr("PauseMenuSaveAndQuit","Save and Quit")};
    for(int i=0;i<4;++i)button(127,52+i*26,187,25,items[i],selection==i);
    divider(12,168,296);hint[0]=0;append(hint,sizeof(hint),physical_key_name(input_controller.bindings().key[BindAbility]));append(hint,sizeof(hint),": ");append(hint,sizeof(hint),tr("ScoreData"));renderer.canvas.text(12,176,hint,Cyan);
    value_label(12,199,"GUIScore",game.score,Muted);value_label(207,199,"GUIWave",game.wave,Muted);footer(tr("UiChoose"));
}
static void render_build() {
    Canvas& c=renderer.canvas;heading(tr("UiYourBuild"));
    int count=build_count(),first=build_selection/6*6;
    for(int row=0;row<6&&first+row<count;++row) {
        int at=first+row,y=42+row*25;Offer item=build_offer(at);
        if(at==build_selection)c.shade(10,y,300,25,rgb(47,35,74),155);
        renderer.icon(item,24,y+12,24);char name[192];
        c.clipped(44,y+4,244,item_name(item,name,sizeof(name)),at==build_selection?White:Muted);
        if(item.kind==OFFER_MOD&&game.player.mods[item.id]>1){char rank[16]="x";appendNumber(rank,sizeof(rank),game.player.mods[item.id]);c.right(305,y+4,rank,Cyan);}
    }
    divider(12,199,296);char pos[40]={0};appendNumber(pos,sizeof(pos),unsigned(build_selection+1));append(pos,sizeof(pos)," / ");appendNumber(pos,sizeof(pos),unsigned(count));c.text(14,201,pos,Muted);
    footer(tr("UiDetails"));
}
static void render_tree() {
    Canvas& c=renderer.canvas;char name[192];heading(item_name(detail_offer,name,sizeof(name)));
    const ModDef& mod=mods[detail_offer.id];
    int parents[2]={mod.prerequisite_a,mod.prerequisite_b};
    for(int i=0;i<2;++i)if(parents[i]>=0&&parents[i]<MOD_COUNT) {
        int x=parents[1]<0?160:84+i*152;
        c.line(x,92,160,126,rgb(115,129,180));renderer.icon(Offer{OFFER_MOD,uint8_t(parents[i])},x,72,44);
        c.clipped(x-65,97,130,item_name(Offer{OFFER_MOD,uint8_t(parents[i])},name,sizeof(name)),Muted);
    }
    renderer.icon(detail_offer,160,139,47);renderer.selectIcon(160,139,54);
    unsigned children[3],count=0;
    for(unsigned m=0;m<MOD_COUNT&&count<3;++m)if(mods[m].prerequisite_a==detail_offer.id||mods[m].prerequisite_b==detail_offer.id)children[count++]=m;
    for(unsigned i=0;i<count;++i){int x=160-int(count-1)*42+int(i)*84;c.line(160,159,x,180,rgb(115,129,180));renderer.icon(Offer{OFFER_MOD,uint8_t(children[i])},x,191,36);}
    footer(tr("UiDescription"),"7 / Tab");
}
static void render_detail() {
    Canvas& c=renderer.canvas;renderer.menuBackground(clock_ms,false);Canvas::setFont(&font);
    if(detail_mode==-3) {
        heading(enemy_name(unsigned(reference_enemy)));renderer.enemyIcon(unsigned(reference_enemy),160,103,105);
        const EnemyDef& enemy=enemy_types[reference_enemy];
        char value[32];decimal(value,unsigned(enemy.hull));c.text(12,170,tr("GlossaryHull","Hull"),Muted);c.right(308,170,value,Cyan);
        decimal(value,enemy.contact);c.text(12,191,tr("GlossaryCrashDamage","Crash Damage"),Muted);c.right(308,191,value,Cyan);
        footer(tr("UiBack"));return;
    }
    if(detail_tree){render_tree();return;}
    if(detail_mode>=0)renderer.ui(mode_choices[detail_mode].sprite,27,28,37);
    else if(detail_mode==-1)renderer.icon(detail_offer,27,28,37);
    char name[192],description[6144];
    const char* title=detail_mode>=0?tr(mode_choices[detail_mode].title):detail_mode<=-2?reference_title:item_name(detail_offer,name,sizeof(name));
    int titleLines=c.wrapped(detail_mode<=-2?12:55,9,detail_mode<=-2?296:252,3,title,White);
    int y=titleLines>2?63:53;divider(12,y,296);
    const char* text;
    if(detail_mode>=0)text=tr(mode_choices[detail_mode].description);
    else if(detail_mode==-2){Locale::format(description,sizeof(description),tr(reference_key),true);text=description;}
    else if(detail_mode==-4)text=reference_body;
    else if(detail_mode==-5){daily_rules(description,sizeof(description));text=description;}
    else text=item_description(detail_offer,description,sizeof(description),true);
    int capacity=(193-y)/Canvas::lineHeight();if(capacity<1)capacity=1;detail_visible=capacity;
    int tagRows=detail_mode==-1?render_tags(detail_offer,y+7,detail_scroll,capacity):0;
    int visibleTags=tagRows-detail_scroll;if(visibleTags<0)visibleTags=0;if(visibleTags>capacity)visibleTags=capacity;
    int textSkip=detail_scroll>tagRows?detail_scroll-tagRows:0;
    detail_lines=tagRows+c.wrappedPage(13,y+7+visibleTags*Canvas::lineHeight(),292,capacity-visibleTags,textSkip,text,Cyan);
    if(detail_scroll>=detail_lines&&detail_scroll>0)detail_scroll=detail_lines>capacity?detail_lines-capacity:0;
    char position[40]={0};appendNumber(position,sizeof(position),unsigned(detail_scroll/unsigned(capacity)+1));append(position,sizeof(position)," / ");appendNumber(position,sizeof(position),unsigned((detail_lines+capacity-1)/capacity));c.right(308,201,position,Muted);
    if(detail_mode==-1&&detail_offer.kind==OFFER_MOD){char hint[128]={0};append(hint,sizeof(hint),physical_key_name(input_controller.bindings().key[BindAbility]));append(hint,sizeof(hint),": ");append(hint,sizeof(hint),tr("GlossaryModTree","Mod Tree"));c.clipped(12,201,210,hint,White);}
    footer(tr("UiScroll"),"8 / 2");
}
static void render_learn() {
    Canvas& c=renderer.canvas;heading(tr("LearnTitle"));char title[32],text[4096]={0};
    if(learn_tab<3) {
        const char* heads[]={"Learn1","Learn4","Learn7"};c.text(12,41,tr(heads[learn_tab]),Cyan);
        if(learn_tab==0){append(text,sizeof(text),tr("Learn2"));append(text,sizeof(text),"\n\n");append(text,sizeof(text),tr("Learn3"));}
        else if(learn_tab==1){append(text,sizeof(text),tr("Learn5"));append(text,sizeof(text),"\n\n");append(text,sizeof(text),tr("Learn6"));}
        else append(text,sizeof(text),tr("Learn8"));
    }else{c.text(12,41,tr(learn_tab<11?"Learn9":learn_tab<16?"Learn10":"Learn11"),Cyan);title[0]=0;append(title,sizeof(title),"LearnTip");appendNumber(title,sizeof(title),unsigned(learn_tab-2));append(text,sizeof(text),tr(title));}
    detail_visible=7;detail_lines=c.wrappedPage(12,66,296,detail_visible,detail_scroll,text,White);
    if(learn_tab<3){renderer.icon(Offer{OFFER_WEAPON,0},252,194,25);renderer.icon(Offer{OFFER_SHIELD,0},278,194,25);renderer.icon(Offer{OFFER_BODY,0},301,194,25);}
    char pos[64]={0};appendNumber(pos,sizeof(pos),learn_tab+1);append(pos,sizeof(pos)," / 20");c.text(12,200,pos,Muted);
    footer(tr("UiPage"),"4 / 6");
}
static const char* binding_label(unsigned action) {
    const char* keys[]={"DefineKeyboardControlsThrust","UiBrake","DefineKeyboardControlsTurnLeft","DefineKeyboardControlsTurnRight","DefineKeyboardControlsFire","UiAbility","DefineKeyboardControlsUpgrade","DefineKeyboardControlsReroll","TutorialBanish2"};
    return action<BindCount?tr(keys[action]):tr("UiResetKeys");
}
static void render_rebind() {
    Canvas& c=renderer.canvas;heading(tr("SettingsTitleControls"));int first=selection/5*5;
    for(int row=0;row<5&&first+row<=BindCount;++row){unsigned at=unsigned(first+row);int y=44+row*25;
        const char* key=at<BindCount?physical_key_name(input_controller.bindings().key[at]):"";
        button(12,y,296,25,binding_label(at),selection==int(at),Canvas::textWidth(key)+12);c.right(303,y+4,key,Cyan);
    }
    const char* status="";
    if(input_controller.rebindStatus()==RebindStatus::Reserved)status=tr("UiBindingReserved");
    else if(input_controller.rebindStatus()==RebindStatus::Conflict)status=tr("UiBindingConflict");
    else if(input_controller.capturingBinding())status=tr("DefineKeyboardControlsEnterKey");
    if(*status){panel(8,188,304,32);c.wrapped(12,189,296,2,status,White);}
    footer(tr("UiChange"));
}
static const char* reward_title(unsigned rank,char* buffer,unsigned capacity) {
    const char* source=Career::unlockKey(rank);if(source[0]=='s'&&source[1]=='t'&&source[2]=='r')source+=3;
    const char* mp="ModUnlockMessage";const char* gp="GearUnlockMessage";unsigned m=0,g=0;
    while(mp[m]&&source[m]==mp[m])++m;
    while(gp[g]&&source[g]==gp[g])++g;
    char key[96]={0};
    if(!mp[m]){append(key,sizeof(key),"ModTitle");append(key,sizeof(key),source+m);}
    else if(!gp[g]){append(key,sizeof(key),"GearTitle");if(rank==9)append(key,sizeof(key),"BodyHullbreaker");else if(rank==27)append(key,sizeof(key),"WeaponVortex");else append(key,sizeof(key),source+g);}
    else append(key,sizeof(key),rank==2?"UnlocksTitleTrain":source);
    Locale::plain(buffer,capacity,tr(key,tr(source)),true);for(char* p=buffer;*p;++p)if(*p=='\n'){*p=0;break;}return buffer;
}
static unsigned display_unlock_rank(const Offer& offer,unsigned depth=0) {
    unsigned rank=offerUnlockRank(offer.kind,offer.id);
    if(offer.kind!=OFFER_MOD||offer.id>=MOD_COUNT||depth>8)return rank;
    if(mods[offer.id].category==2&&rank<14)rank=14;
    int parents[2]={mods[offer.id].prerequisite_a,mods[offer.id].prerequisite_b};
    for(int p:parents)if(p>=0&&p<MOD_COUNT){unsigned r=display_unlock_rank(Offer{OFFER_MOD,uint8_t(p)},depth+1);if(r>rank)rank=r;}
    return rank;
}
static void render_progress() {
    Canvas& c=renderer.canvas;heading(tr("DrawEndScoreProgress"));
    value_label(12,42,"DrawEndScoreRank",career.rank,Cyan);value_label(160,42,"DrawEndScoreUnlocksRemaining",64-career.rank,Muted);
    unsigned cap=career.nextRankPoints()*1000;unsigned amount=career.unlock_points_milli;
    c.fill(12,67,296,5,rgb(38,39,70));if(cap)c.fill(12,67,int(296ull*amount/cap),5,Cyan);
    char pts[64]={0};appendNumber(pts,sizeof(pts),amount/1000);append(pts,sizeof(pts)," / ");appendNumber(pts,sizeof(pts),cap/1000);c.text(12,78,pts,Muted);
    int first=progress_selection/4*4;
    for(int row=0;row<4;++row){unsigned rank=unsigned(first+row+1);const char* key=Career::unlockKey(rank);if(key[0]=='s'&&key[1]=='t'&&key[2]=='r')key+=3;
        char label[256]={0};appendNumber(label,sizeof(label),rank);append(label,sizeof(label),". ");char plain[220];reward_title(rank,plain,sizeof(plain));append(label,sizeof(label),plain);
        button(12,103+row*25,296,25,label,progress_selection==int(rank-1),rank>career.rank?Canvas::textWidth(tr("UiLocked"))+8:0);
        if(rank>career.rank)c.right(303,107+row*25,tr("UiLocked"),Muted);
    }
    footer(tr("UnlocksTitle"),physical_key_name(input_controller.bindings().key[BindAbility]));
}
static void render_reference_list() {
    Canvas& c=renderer.canvas;
    heading(page==GlossaryPage?tr("MenuGlossary"):page==BestiaryPage?tr("UiBestiary"):page==SuperModsPage?tr("PauseMenuShowSupermods"):tr("UnlocksTitle"));
    int count=page==GlossaryPage?GlossaryCount:page==BestiaryPage?ENEMY_COUNT:page==SuperModsPage?super_count():catalog_count();
    if(page==CatalogPage) {
        const char* titles[]={"DrawEndScoreTitleWeapon","DrawEndScoreTitleShields","DrawEndScoreTitleBody","Unlocks4","LoadoutChallenge"};
        c.text(12,40,tr(titles[catalog_tab]),Cyan);char rank[48]={0};append(rank,sizeof(rank),tr("DrawEndScoreRank"));appendNumber(rank,sizeof(rank),career.rank);append(rank,sizeof(rank)," / 64");c.right(307,40,rank,Muted);
        int first=selection/12*12;
        for(int i=0;i<12&&first+i<count;++i) {int at=first+i,x=50+(i%6)*47,y=88+(i/6)*49;bool locked;
            if(catalog_tab==4){renderer.ui(mode_choices[at].sprite,x,y,39);locked=!career.modeUnlocked(mode_choices[at].flag);}
            else {Offer o=catalog_offer(at);renderer.icon(o,x,y,39);locked=career.rank<display_unlock_rank(o);}
            if(locked)renderer.ui(catalog_tab==4?"sModeLockedFrame":"sHexLockedFrame",x,y,39);
            if(at==selection)renderer.selectIcon(x,y,46);
        }
        divider(12,165,296);char name[192];c.wrapped(12,171,296,2,catalog_tab==4?tr(mode_choices[selection].title):item_name(catalog_offer(selection),name,sizeof(name)),White);
        char hint[128]={0};append(hint,sizeof(hint),physical_key_name(input_controller.bindings().key[BindAbility]));append(hint,sizeof(hint),": ");append(hint,sizeof(hint),tr("DrawEndScoreProgress"));c.clipped(12,204,215,hint,Muted);footer(tr("UiDetails"));return;
    }
    int first=selection/6*6;
    for(int row=0;row<6&&first+row<count;++row){int at=first+row,y=43+row*25;char name[192];
        const char* title=page==GlossaryPage?glossary_title(unsigned(at)):page==BestiaryPage?enemy_name(unsigned(at)):item_name(super_offer(at),name,sizeof(name));
        if(page==GlossaryPage)button(12,y,296,25,title,selection==at);
        else {if(at==selection)c.shade(10,y,300,25,rgb(47,35,74),155);
            if(page==BestiaryPage)renderer.enemyIcon(unsigned(at),26,y+13,23);else renderer.icon(super_offer(at),26,y+13,23);
            c.clipped(47,y+4,258,title,at==selection?White:Muted);
        }
    }
    char pos[40]={0};appendNumber(pos,sizeof(pos),selection+1);append(pos,sizeof(pos)," / ");appendNumber(pos,sizeof(pos),count);c.right(307,201,pos,Muted);
    if(page==GlossaryPage){char hint[128]={0};append(hint,sizeof(hint),physical_key_name(input_controller.bindings().key[BindAbility]));append(hint,sizeof(hint),": ");append(hint,sizeof(hint),tr("UiBestiary"));c.clipped(12,201,215,hint,Muted);}
    footer(tr("UiDetails"));
}
static void render_credits() {
    Canvas& c=renderer.canvas;heading(tr("CreditsTitle"));detail_visible=10;
    detail_lines=c.wrappedPage(12,43,296,detail_visible,detail_scroll,tr("OwnerCredits",tr("ScoreCredits")),White);
    char pos[32]={0};appendNumber(pos,sizeof(pos),unsigned(detail_scroll/detail_visible+1));append(pos,sizeof(pos)," / ");appendNumber(pos,sizeof(pos),unsigned((detail_lines+detail_visible-1)/detail_visible));c.right(307,204,pos,Muted);
    footer(tr("UiScroll"),"8 / 2");
}
static void render_changelog() {
    Canvas& c=renderer.canvas;heading(reference_title);
    detail_visible=10;detail_lines=c.wrappedPage(12,43,296,detail_visible,detail_scroll,tr("OwnerChangelog"),White);
    char position[40]={0};appendNumber(position,sizeof(position),unsigned(detail_scroll/detail_visible+1));append(position,sizeof(position)," / ");appendNumber(position,sizeof(position),unsigned((detail_lines+detail_visible-1)/detail_visible));c.right(307,204,position,Muted);
    footer(tr("UiScroll"),"8 / 2");
}
static void stat_row(int y,const char* key,unsigned value,const char* suffix="") {
    Canvas& c=renderer.canvas;c.clipped(12,y,238,tr(key,key),Muted);
    char number[64];decimal(number,value);append(number,sizeof(number),suffix);c.right(307,y,number,Cyan);
}
static void decimal64(char* out,uint64_t value) {
    char reverse[24];unsigned count=0;do{reverse[count++]=char('0'+value%10);value/=10;}while(value);
    for(unsigned i=0;i<count;i++)out[i]=reverse[count-i-1];
    out[count]=0;
}
static void ratio_text(char* out,unsigned capacity,uint64_t numerator,unsigned denominator,const char* suffix="") {
    if(!denominator)denominator=1;
    uint64_t whole=numerator/denominator;unsigned tenth=unsigned(((numerator%denominator)*10+denominator/2)/denominator);
    if(tenth>=10){++whole;tenth=0;}decimal64(out,whole);
    if(tenth){append(out,capacity,".");appendNumber(out,capacity,tenth);}append(out,capacity,suffix);
}
static void ratio_row(int y,const char* key,uint64_t numerator,unsigned denominator,const char* suffix="") {
    Canvas& c=renderer.canvas;c.clipped(12,y,224,tr(key,key),Muted);
    char number[64];ratio_text(number,sizeof(number),numerator,denominator,suffix);c.right(307,y,number,Cyan);
}
static unsigned stats_rows() {
    static const uint8_t counts[8]={11,7,9,13,4,8,3,4};return counts[stats_tab];
}
static void render_stats() {
    Canvas& c=renderer.canvas;heading(tr("ScoreData","STATISTICS"));
    const char* titles[]={"DrawEndScoreTitleWeapon","DrawEndScoreTitleShields","DrawEndScoreTitleBody","DrawEndScoreTitleConstructs","DrawEndScoreGame","DrawEndScoreTitleRecord","DrawEndScoreTitleBurnDamage","DrawEndScoreTitleGlobal"};
    c.centered(39,tr(titles[stats_tab],titles[stats_tab]),White);divider(12,60,296);
    struct Row {const char* key;char value[128];};Row rows[16];unsigned count=0;
    auto number=[&](const char* key,uint64_t value,unsigned denominator=1,const char* suffix="") {Row& r=rows[count++];r.key=key;ratio_text(r.value,sizeof(r.value),value,denominator,suffix);};
    auto resistance=[&](const char* key,unsigned received) {Row& r=rows[count++];r.key=key;r.value[0]=0;int amount=100-(int)received;if(amount<0)append(r.value,sizeof(r.value),"-");appendNumber(r.value,sizeof(r.value),(unsigned)(amount<0?-amount:amount));append(r.value,sizeof(r.value),"%");};
    const uint8_t* m=game.player.mods;
    if(stats_tab==0) {
        number("DrawEndScoreWeapon1",game.weaponDamage());number("DrawEndScoreWeapon2",game.weaponProjectiles());
        number("DrawEndScoreWeapon3",game.weaponRateMilli(),1000," /s");
        number("DrawEndScoreWeapon4",unsigned(weapons[game.player.weapon].speed*game.stats.velocity*TICK_RATE/100),1," /s");
        number("DrawEndScoreWeapon5",unsigned(game.stats.blast),1,"%");number("DrawEndScoreWeapon6",unsigned(game.stats.size),1,"%");
        char degrees[96]=" ";append(degrees,sizeof(degrees),tr("PauseMenuDegrees"));number("DrawEndScoreWeapon7",game.weaponSpreadArcDegrees(),1,degrees);number("DrawEndScoreWeapon8",unsigned(game.stats.homing));number("DrawEndScoreWeapon9",game.weaponCannons());
        Row& mode=rows[count++];mode.key="DrawEndScoreWeapon10";const char* key=m[M_X_BARRAGE]?"DrawEndScoreWeaponBarrage":m[M_SIEGE]?"DrawEndScoreWeaponSiegeWeaponry":m[M_CHARGED_SHOT]&&!m[M_X_CHARGED_MINES]?"DrawEndScoreWeaponChargedShot":m[M_BURST_FIRE]||m[M_X_DOUBLE_TAP]?"DrawEndScoreWeaponBurstFire":"DrawEndScoreWeaponAutomatic";
        mode.value[0]=0;append(mode.value,sizeof(mode.value),tr(key));
        uint64_t damage=game.weaponDamage(),projectiles=game.weaponProjectiles();
        if(game.player.weapon==W_PULSE)damage=damage*3/2+(damage*45/100)*projectiles;
        else if(game.player.weapon!=W_SWORDS)damage*=projectiles;
        number("DrawEndScoreWeapon11",damage*game.weaponRateMilli(),1000);
        // Charge-dependent vortices and autonomous blades do not have a stable
        // direct-fire DPS. A dash avoids presenting a made-up fixed number.
        if(game.player.weapon==W_VORTEX||game.player.weapon==W_BLADE){rows[count-1].value[0]='-';rows[count-1].value[1]=0;}
    } else if(stats_tab==1) {
        number("DrawEndScoreShields1",unsigned(game.player.max_shield));number("DrawEndScoreShields2",game.shieldRegenerationMilli(),1000," /s");
        resistance("DrawEndScoreShields3",game.incomingDamagePercent(true));
        number("DrawEndScoreShields4",game.activeShieldEffect(),1,"%");number("DrawEndScoreShields5",game.activeShieldRadius()*100/shields[game.player.shield_type].radius,1,"%");
        number("DrawEndScoreShields6",game.activeShieldCooldownTicks(),TICK_RATE," s");resistance("DrawEndScoreShields7",game.incomingDamagePercent(true,true));
    } else if(stats_tab==2) {
        number("DrawEndScoreBody1",unsigned(game.player.max_hull));number("DrawEndScoreBody2",game.hullRegenerationMilli(),1000," /s");
        resistance("DrawEndScoreBody3",game.incomingDamagePercent());resistance("DrawEndScoreBody4",game.crashDamageTakenPercent());
        number("DrawEndScoreBody5",game.knockbackResistancePercent(),1,"%");number("DrawEndScoreBody6",unsigned(game.stats.crash_damage),1,"%");
        number("DrawEndScoreBody7",unsigned(game.stats.thrust));resistance("DrawEndScoreBody9",game.incomingDamagePercent(false,true));number("DrawEndScorePlating",unsigned(game.stats.plating));
    } else if(stats_tab==3) {
        number("DrawEndScoreConstruct1",game.constructLimit(C_MINE));number("DrawEndScoreConstruct2",game.constructLimit(C_TURRET));
        number("DrawEndScoreConstruct3",game.constructLimit(C_DRONE)+game.constructLimit(C_DEFENSE_DRONE));
        number("DrawEndScoreConstruct4",game.constructLimit(C_ALLY)+game.constructLimit(C_INTERCEPTOR)+game.constructLimit(C_GUARDIAN));
        number("DrawEndScoreConstruct5",game.constructLimit(C_BASTION));
        number("GearTitleWeaponSwords",game.constructLimit(C_SWORD)/2);
        number("DrawEndScoreConstructBlade",game.constructLimit(C_BLADE));number("DrawEndScoreConstructSwarm",game.constructLimit(C_SWARM));
        number("DrawEndScoreConstruct6",game.constructHullPercent(C_BLADE+1),1,"%");number("DrawEndScoreConstruct7",unsigned(game.stats.assembly),1,"%");
        number("DrawEndScoreConstruct8",game.constructFireRatePercent(C_BLADE+1),1,"%");number("DrawEndScoreTitleConstructs",game.constructCount());number("DrawEndScoreWeapon1",unsigned(game.stats.construct_damage),1,"%");
    } else if(stats_tab==4) {
        number("DrawEndScoreScore",game.score);number("DrawEndScoreWaveReached",game.wave);number("DrawEndScoreLevelReached",game.level);
        Row& r=rows[count++];r.key="DrawEndScoreTimeElapsed";r.value[0]=0;unsigned seconds=game.frame/TICK_RATE;appendNumber(r.value,sizeof(r.value),seconds/60);append(r.value,sizeof(r.value),":");if(seconds%60<10)append(r.value,sizeof(r.value),"0");appendNumber(r.value,sizeof(r.value),seconds%60);
    } else if(stats_tab==5) {
        number("DrawEndScoreTotalDamage",game.run_stats.damage_dealt);number("DrawEndScoreHighestDamage",game.run_stats.highest_hit);number("DrawEndScoreDamageTaken",game.run_stats.damage_taken);number("UiKills",game.kills);
        number("UpgradeRerolls",game.run_stats.rerolls_used);number("UpgradeUpgrade",game.run_stats.mods_taken);number("GlossaryExperience",game.run_stats.experience_collected);number("DrawEndScoreDistanceReached",game.run_stats.distance_q8,FP);
    } else if(stats_tab==6) {
        number("DrawEndScoreBurnDamage",game.burnDamagePercent(),1,"%");number("DrawEndScoreBurnIgniteDamage",game.igniteDamagePercent(),1,"%");number("DrawEndScoreBurnIgniteDuration",game.igniteDurationTicks()*100/90,1,"%");
    } else {
        number("DrawEndScoreGlobalDamage",game.globalDamagePermille(),10,"%");number("DrawEndScoreGlobalDamageTotal",game.globalDamagePermille(),10,"%");
        number("DrawEndScoreGlobalBlastRadius",game.globalBlastRadiusPercent(),1,"%");number("DrawEndScoreGlobalDamageTaken",game.incomingDamagePercent(),1,"%");
    }
    unsigned pages=(count+3)/4;if((unsigned)stats_subpage>=pages)stats_subpage=0;
    for(unsigned row=0;row<4&&stats_subpage*4+row<count;row++){
        Row& r=rows[stats_subpage*4+row];int y=65+int(row)*35,value_width=Canvas::textWidth(r.value);if(value_width>155)value_width=155;
        int label_width=284-value_width;if(label_width<120)label_width=120;
        c.wrapped(12,y,label_width,2,tr(r.key,r.key),Muted);
        if(Canvas::textWidth(r.value)>155)c.wrapped(164,y,144,2,r.value,Cyan);else c.right(307,y+7,r.value,Cyan);
    }
    char position[32]={0};appendNumber(position,sizeof(position),unsigned(stats_subpage+1));append(position,sizeof(position)," / ");appendNumber(position,sizeof(position),pages);c.right(307,203,position,Muted);
    footer(tr("UiPage"),pages>1?"4/6, 8/2":"4/6");
}
static void render_scores() {
    Canvas& c=renderer.canvas;heading(tr(scores_tab==2?"UiRecords":scores_tab==1?"DrawEndScoreTitleRecord":"ScoreHighScores"));
    if(!scores_tab) {
        int first=selection/5*5;
        for(int row=0;row<5;++row){int at=first+row,y=42+row*24;const RunRecord& r=profile.records[at];
            char rank[24]={0};appendNumber(rank,sizeof(rank),at+1);append(rank,sizeof(rank),".");button(16,y,288,24,rank,selection==at);
            char value[24];decimal(value,r.score);c.right(296,y+4,value,selection==at?Cyan:Muted);
        }
        const RunRecord& r=profile.records[selection];divider(12,164,296);
        renderer.icon(Offer{OFFER_WEAPON,r.weapon},27,187,28);renderer.icon(Offer{OFFER_SHIELD,r.shield},55,187,28);renderer.icon(Offer{OFFER_BODY,r.body},83,187,28);
        value_label(113,174,"GUIWave",r.wave,Cyan);value_label(113,196,"GUILevel",r.level,Muted);
    }else if(scores_tab==1) {
        const RunRecord& r=profile.records[selection];
        c.text(12,48,tr("DrawEndScoreMode"),Muted);c.right(307,48,tr((r.flags&RUN_ENDLESS)?"DrawEndScoreEndless":"DrawEndScoreArcade"),Cyan);
        stat_row(70,"DrawEndScoreScore",r.score);stat_row(92,"DrawEndScoreTotalDamage",r.damage_dealt);
        stat_row(114,"DrawEndScoreHighestDamage",r.highest_hit);stat_row(136,"DrawEndScoreDamageTaken",r.damage_taken);
        stat_row(158,"UiKills",r.kills);ratio_row(180,"DrawEndScoreTimeElapsed",r.ticks,TICK_RATE," s");
        ratio_row(202,"DrawEndScoreDistanceReached",r.distance_q8,FP);
    }else {
        stat_row(48,"UiRuns",career.runs);stat_row(70,"DrawEndScoreLifetimePoints",career.total_score);
        stat_row(92,"UiKills",career.total_kills);stat_row(114,"UiBestWave",career.best_wave);
        ratio_row(136,"DrawEndScoreTimeElapsed",career.total_ticks,TICK_RATE," s");
        stat_row(158,"DrawEndScoreRank",career.rank);stat_row(180,"DrawEndScoreHighestDamage",career.highest_hit);
        ratio_row(202,"DrawEndScoreDistanceReached",career.total_distance_q8,FP);
    }
    footer(tr("UiPage"),"4 / 6");
}
static void render_resume() {
    Canvas& c=renderer.canvas;const Game& saved=snapshot.game;heading(tr("MenuResumeRun"));
    renderer.ui("sStart",42,61,43);button(63,44,245,34,tr("MenuResumeRun"),true);
    for(int i=0;i<ModeCount;++i)renderer.ui(mode_choices[i].sprite,24+i%6*54,100+i/6*37,29,65535,(saved.run_flags&mode_choices[i].flag)?255:70);
    divider(12,161,296);value_label(12,169,"GUIScore",saved.score,Cyan);value_label(184,169,"GUIWave",saved.wave,White);value_label(12,194,"GUILevel",saved.level,Muted);
    char time[40]={0};unsigned seconds=saved.frame/TICK_RATE;appendNumber(time,sizeof(time),seconds/60);append(time,sizeof(time),":");if(seconds%60<10)append(time,sizeof(time),"0");appendNumber(time,sizeof(time),seconds%60);c.right(307,194,time,Muted);
    footer(tr("UiContinue"));
}
static void render_gameover() {
    Canvas& c=renderer.canvas;renderer.menuBackground(clock_ms,false);Canvas::setFont(&font);
    heading(tr("UiRunEnded"));
    if(game.run_flags&RUN_PRACTICE)c.clipped(12,23,296,tr("DrawEndScorePractice"),Muted);
    else if(game.run_flags&RUN_DAILY){char date[11];calendar_text(date,game.daily_day);c.clipped(12,23,208,tr("UiDailyOffline"),Muted);c.right(308,23,date,Cyan);}
    char score[24];decimal(score,game.score);c.centered(40,score,Cyan,2);
    renderer.icon(Offer{OFFER_WEAPON,game.player.weapon},27,108,31);
    renderer.icon(Offer{OFFER_SHIELD,game.player.shield_type},58,108,31);
    renderer.icon(Offer{OFFER_BODY,game.player.body},43,137,31);
    value_label(93,80,"GUIWave",game.wave,White);value_label(93,103,"GUILevel",game.level,Muted);
    value_label(12,183,"DrawEndScoreRank",career.rank,Cyan);
    const char* items[]={tr("UiRetry"),tr("ScoreData","Statistics"),tr("UiMainMenu"),tr("MenuExit","Exit")};
    for(int i=0;i<4;++i)button(93,133+i*23,220,23,items[i],selection==i);
}
#include "app_ending.inc"
}

bool app_init(const char* base_path) {
    quitting=false;clock_ms=autosave_ms=fps_ms=fps_frames=fps_value=0;
    page=return_page=controls_return=credits_return=scores_return=HomePage;reference_return=LearnPage;
    selection=build_selection=stats_tab=stats_subpage=controls_tab=upgrade_selection=0;detail_mode=-1;detail_tree=false;
    pointer_visible=false;toast[0]=0;toast_until=demo_ticks=0;incompatible_save=false;
    input_controller.reset();input_controller.loadBindings();pending_ability=false;load_profile();gained_ranks=0;progress_selection=learn_tab=0;
    settings_tab=scores_tab=0;progress_return=HomePage;pending_run_day=0;load_daily_profile();
    bool profile_ok=load_preferences();
    ready=assets.open(base_path);
    for(unsigned i=0;i<Locale::Count;++i) {
        char name[]="nNova.eng.locale.dat.tns";const char* code=Locale::code(i);for(unsigned j=0;j<3;++j)name[6+j]=code[j];
        platform::File* file=platform::open_data(name);language_available[i]=file!=0;if(file)platform::close(file);
    }
    int desired=profile_ok?int(preferences.language):Locale::fromSystem(platform::system_language());
    bool selected=desired>=0&&language.open(unsigned(desired))&&font.open(unsigned(desired));
    if(selected)preferences.language=unsigned(desired);
    if(!selected&&language.open(0)&&font.open(0)){preferences.language=0;selected=true;}
    if(!selected)for(unsigned i=0;i<Locale::Count&&!selected;++i)
        if(language.open(i)&&font.open(i)){preferences.language=i;selected=true;}
    if(!selected){preferences.language=0;language.open(0);font.open(0);}
    if(preferences.language==9)preferences.flags|=Chinese;else preferences.flags&=~Chinese;
    input_controller.setPointerMode((preferences.flags&PointerAim)!=0);
    game.start(1);game.state=Title;gameover_recorded=false;ending_seen=false;
    renderer.initialize(&assets,&snapshot.game);skins.open();
    if(preferences.skin>skins.count()||!skins.select(preferences.skin))preferences.skin=0;
    apply_settings();ataraxia_prompt_level=0;
    has_save=load_run();
    selection=has_save?0:1;
    if(!has_save) {
        platform::File* saved=platform::open_state("nNova.sav.tns");
        if(saved){uint32_t header[6]={0};if(platform::read(saved,header,sizeof(header))==sizeof(header)&&(header[2]!=2||header[3]!=sizeof(Game)))incompatible_save=true;platform::close(saved);}
        if(incompatible_save)notice(tr("UiSaveUnavailable"));
    }
    renderer.setText(&language,&font);return ready;
}
void app_frame(const Input& raw,uint32_t elapsed_ms) {
    Input in=input_controller.update(raw,(unsigned(page)<<8)|unsigned(game.state),page==HomePage&&game.state==Playing);
    if(input_controller.pointerMode()!=bool(preferences.flags&PointerAim)){
        if(input_controller.pointerMode())preferences.flags|=PointerAim;else preferences.flags&=~PointerAim;
        save_preferences();notice(tr(input_controller.pointerMode()?"UiTouchAim":"UiKeypad"));
    }
    clock_ms+=elapsed_ms;fps_ms+=elapsed_ms;++fps_frames;
    if(fps_ms>=1000){fps_value=fps_frames*1000/fps_ms;fps_ms=0;fps_frames=0;}
    pointer_activity=input_controller.pointerActivity();pointer_visible=in.pointer_present;
    pointer_x=in.pointer_x;pointer_y=in.pointer_y;pointer_down=in.pointer_down;
    if(ready&&!assets.ready())ready=false;
    if(in.quit){if(game.state==Playing||game.state==Pause||game.state==Upgrade)autosave_run();quitting=true;return;}
    if(!ready){if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm))quitting=true;return;}
    if(platform::host_demo()){demo_frame(elapsed_ms);return;}
    if(page==EndingPage){process_ending(in,elapsed_ms);return;}
    if(page==SettingsPage){process_settings(in);return;}
    if(page==AssistPage){move_menu(in,2,145,29,45,230);if(pressed(in,ButtonBack)){page=SettingsPage;selection=2;return;}
        if(activate(in,145,29,45,230)){if(selection==1){preferences.flags|=AssistMode;apply_settings();save_preferences();}page=SettingsPage;selection=2;}return;}
    if(page==LanguagePage){process_languages(in);return;}
    if(page==ModePage){process_mode(in);return;}
    if(page==DailyPage){process_daily(in);return;}
    if(page==DailyScoresPage){process_daily_scores(in);return;}
    if(page==LearnPage){process_learn(in);return;}
    if(page==RebindPage){process_rebind(in);return;}
    if(page==ProgressPage){process_progress(in);return;}
    if(page==ResumePage){if(pressed(in,ButtonBack)){page=HomePage;selection=0;}else if(pressed(in,ButtonConfirm)||(in.pointer_pressed&&hit(12,40,296,42)))resume_saved();return;}
    if(page==ResetPage){
        move_menu(in,2,145,29,45,230);
        if(pressed(in,ButtonBack)){page=SettingsPage;selection=0;return;}
        if(activate(in,145,29,45,230)){if(selection==1){
            if(reset_progress){career.clear();preferences.run_flags=0;preferences.flags&=~Wild;}
            else {for(unsigned i=0;i<10;++i)profile.records[i]=RunRecord{};preferences.high_score=preferences.best_wave=0;career.best_score=career.best_wave=0;}
            save_profile();save_preferences();
        }page=SettingsPage;selection=0;}return;
    }
    if(page==GlossaryPage||page==CatalogPage||page==BestiaryPage||page==SuperModsPage){process_reference_list(in);return;}
    if(page==CreditsPage){
        if(pressed(in,ButtonUp)){detail_scroll-=detail_visible;if(detail_scroll<0)detail_scroll=0;}
        if(pressed(in,ButtonDown)&&detail_scroll+detail_visible<detail_lines)detail_scroll+=detail_visible;
        if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)){page=credits_return;selection=0;}return;
    }
    if(page==ChangelogPage){
        if(pressed(in,ButtonUp)){detail_scroll-=detail_visible;if(detail_scroll<0)detail_scroll=0;}
        if(pressed(in,ButtonDown)&&detail_scroll+detail_visible<detail_lines)detail_scroll+=detail_visible;
        if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)){language.open(preferences.language);page=reference_return;selection=0;}return;
    }
    if(page==ControlsPage){
        if(pressed(in,ButtonLeft)||pressed(in,ButtonRight))controls_tab^=1;
        if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)){page=controls_return;selection=0;}return;
    }
    if(page==BuildPage){process_build(in);return;}
    if(page==DetailPage) {
        if(pressed(in,ButtonTab)&&detail_mode==-1&&detail_offer.kind==OFFER_MOD){detail_tree=!detail_tree;return;}
        if(pressed(in,ButtonUp)&&detail_scroll>0)--detail_scroll;
        if(pressed(in,ButtonDown)&&detail_scroll+detail_visible<detail_lines)++detail_scroll;
        if(pressed(in,ButtonLeft)){detail_scroll-=detail_visible;if(detail_scroll<0)detail_scroll=0;}
        if(pressed(in,ButtonRight)){detail_scroll+=detail_visible;if(detail_scroll+detail_visible>detail_lines)detail_scroll=detail_lines>detail_visible?detail_lines-detail_visible:0;}
        if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)){
            page=detail_return;selection=detail_selection;
        }return;
    }
    if(page==StatsPage){
        if(pressed(in,ButtonLeft)){stats_tab=(stats_tab+7)%8;stats_subpage=0;}
        if(pressed(in,ButtonRight)){stats_tab=(stats_tab+1)%8;stats_subpage=0;}
        int pages=int((stats_rows()+3)/4);
        if(pressed(in,ButtonUp))stats_subpage=(stats_subpage+pages-1)%pages;
        if(pressed(in,ButtonDown))stats_subpage=(stats_subpage+1)%pages;
        if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm))return_from_page();
        return;
    }
    if(page==ScoresPage){
        if(pressed(in,ButtonLeft))scores_tab=(scores_tab+2)%3;
        if(pressed(in,ButtonRight))scores_tab=(scores_tab+1)%3;
        if(!scores_tab)move_menu(in,10,42,24,16,288,selection/5*5,5);
        if(pressed(in,ButtonBack)||pressed(in,ButtonConfirm)){page=scores_return;selection=0;}return;
    }
    if(page==NewRunPage) {
        move_menu(in,2,145,29,45,230);
        if(pressed(in,ButtonBack)){page=pending_run_day?DailyPage:ModePage;selection=0;return;}
        if(activate(in,145,29,45,230)){if(selection==1)start_run(pending_run_day);else{page=pending_run_day?DailyPage:ModePage;selection=0;}}return;
    }
    if(game.state==Title){process_home(in);return;}
    if(game.state==Pause){process_pause(in);return;}
    if(game.state==Upgrade){process_upgrade(in);return;}
    if(game.state==GameOver){record_gameover();if(game.victory&&!ending_seen&&!(game.run_flags&RUN_ENDLESS)){begin_ending();return;}process_gameover(in);return;}
    if(game.banked_upgrades&&pressed(in,ButtonUpgrade)){
        game.reopenUpgrade();pending_ability=false;ataraxia_prompt_level=game.level;autosave_run();return;
    }
    if(pressed(in,ButtonPause)||pressed(in,ButtonBack)||pressed(in,ButtonMenu)){game.state=Pause;selection=0;autosave_run();return;}
    GameInput control={};control.up=(in.held&(ButtonUp|ButtonThrust))!=0;
    control.down=(in.held&ButtonDown)!=0;control.left=(in.held&ButtonLeft)!=0;control.right=(in.held&ButtonRight)!=0;
    control.fire=(in.held&ButtonFire)!=0||in.pointer_down;pending_ability=pending_ability||pressed(in,ButtonTab);
    control.ability=pending_ability;control.pointer_valid=input_controller.pointerSteering()&&in.pointer_present;
    control.pointer_down=in.pointer_down;control.pointer_x=int16_t(in.pointer_x*game.worldWidth()/ScreenWidth);
    control.pointer_y=int16_t(in.pointer_y*game.worldHeight()/ScreenHeight);
    uint32_t previous=game.frame;game.advance(elapsed_ms,control);if(game.frame!=previous)pending_ability=false;
    autosave_ms+=elapsed_ms;
    if(game.state==Upgrade&&game.player.mods[M_X_ATARAXIA]){
        if(preferences.ataraxia_reminders==2)game.bankUpgrade();else ataraxia_prompt_level=game.level;
    }
    if(game.state==GameOver){record_gameover();if(game.victory&&!ending_seen&&!(game.run_flags&RUN_ENDLESS))begin_ending();}
    else if((preferences.autosave==2&&autosave_ms>=10000)||(preferences.autosave&&game.state==Upgrade))save_run();
}
void app_render(uint16_t* framebuffer) {
    renderer.bind(framebuffer);Canvas& c=renderer.canvas;Canvas::setFont(&font);
    if(ready&&!assets.ready())ready=false;
    if(!ready){
        c.clear(Panel);Canvas::setFont(0);c.centered(30,"NOVA DRIFT",White,2);c.centered(76,"DATA FILES NOT FOUND",Red);
        c.wrapped(20,100,280,4,assets.error(),White);c.wrapped(20,152,280,4,"Copy the complete nNova folder. Keep nNova.tns next to data/ and state/.",Muted);
        c.centered(215,"ENTER / ESC: EXIT",Cyan);return;
    }
    if(page==EndingPage){render_ending();return;}
    if(game.state==Playing&&page==HomePage)renderer.world(game,clock_ms,(preferences.flags&FullEffects)!=0,(preferences.flags&BrightAsteroids)!=0,(preferences.flags&EnemyHullBars)!=0,physical_key_name(input_controller.bindings().key[BindUpgrade]),!game.player.mods[M_X_ATARAXIA]||preferences.ataraxia_reminders==0||(preferences.ataraxia_reminders==1&&game.level>ataraxia_prompt_level));
    else renderer.menuBackground(clock_ms,page==HomePage||page==ModePage);
    Canvas::setFont(&font);
    if(page==SettingsPage)render_settings();else if(page==ControlsPage)render_controls();
    else if(page==LanguagePage)render_languages();else if(page==ModePage)render_mode();
    else if(page==DailyPage)render_daily();else if(page==DailyScoresPage)render_daily_scores();
    else if(page==BuildPage)render_build();else if(page==DetailPage)render_detail();
    else if(page==StatsPage)render_stats();else if(page==ScoresPage)render_scores();
    else if(page==LearnPage)render_learn();
    else if(page==GlossaryPage||page==CatalogPage||page==BestiaryPage||page==SuperModsPage)render_reference_list();
    else if(page==CreditsPage)render_credits();else if(page==ChangelogPage)render_changelog();
    else if(page==RebindPage)render_rebind();else if(page==ProgressPage)render_progress();else if(page==ResumePage)render_resume();
    else if(page==ResetPage){heading(tr(reset_progress?"SettingsResetProgress":"SettingsResetScores"));c.wrapped(20,63,280,4,tr(reset_progress?"SettingsResetConfirm":"SettingsScoresResetConfirm"),Red);button(45,145,230,28,tr("UiCancel"),selection==0);button(45,174,230,28,tr("UiContinue"),selection==1);}
    else if(page==AssistPage){heading(tr("SettingsAssistMode"));c.wrapped(20,63,280,4,tr("SettingsAssistModeConfirm"),White);button(45,145,230,28,tr("UiCancel"),selection==0);button(45,174,230,28,tr("UiContinue"),selection==1);}
    else if(page==NewRunPage){
        panel(15,49,290,156);c.centered(58,tr("UiNewRun"),White);
        c.wrapped(27,87,266,3,tr("MenuResumeRunQuery","Really start a new run? Your saved run will be lost."),Muted);
        button(45,145,230,28,tr("UiCancel"),selection==0);button(45,174,230,28,tr("UiNewRun"),selection==1);
    } else if(game.state==Title)render_home();else if(game.state==Pause)render_pause();
    else if(game.state==Upgrade)render_upgrade();else if(game.state==GameOver)render_gameover();
    if((preferences.flags&ShowFps)&&game.state==Playing&&page==HomePage){
        char fps[20]={0};appendNumber(fps,sizeof(fps),fps_value);append(fps,sizeof(fps)," FPS");c.text(242,27,fps,Muted);
    }
    if(toast[0]&&clock_ms<toast_until){c.shade(5,198,310,36,Panel,248);c.wrapped(12,200,296,2,toast,White);}
    if(pointer_visible&&!(preferences.flags&HideCursor))renderer.pointer(pointer_x,pointer_y,pointer_down);
}
bool app_quit(){return quitting;}
#ifdef NOVA_TEST
Game& app_test_game(){return game;}
Career& app_test_career(){return career;}
unsigned app_test_page(){return unsigned(page);}
unsigned app_test_selection(){return unsigned(selection);}
unsigned app_test_flags(){return preferences.flags;}
unsigned app_test_language(){return preferences.language;}
unsigned app_test_ending_ms(){return ending_ms;}
unsigned app_test_stats_tab(){return unsigned(stats_tab);}
unsigned app_test_stats_page(){return unsigned(stats_subpage);}
unsigned app_test_daily_day(){return selected_daily_day();}
unsigned app_test_daily_score(unsigned day){for(const DailyRecord& r:daily_profile.records)if(r.day==day)return r.run.score;return 0;}
void app_test_set_page(unsigned value){page=Page(value);selection=0;if(page==DailyPage)open_daily();if(page==ControlsPage)controls_tab=0;if(page==StatsPage)stats_tab=stats_subpage=0;if(page==LearnPage){learn_tab=0;detail_scroll=0;}if(page==CreditsPage||page==ChangelogPage)detail_scroll=0;if(page==ProgressPage)progress_selection=career.rank<64?career.rank:63;}
bool app_test_set_language(unsigned value){return switch_language(value);}
#endif
}
