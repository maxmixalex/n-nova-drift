// Reproducible UI evidence. Synthetic states are explicitly not a playthrough.
#include "app.h"
#include "game.h"
#include "font.h"
#include "localization.h"
#include "nnova/native_strings.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <cassert>
#include <cstring>
using namespace nova;
static uint16_t framebuf[320*240];
static void shot(const std::filesystem::path& path) {
    app_render(framebuf);FILE* f=std::fopen(path.string().c_str(),"wb");assert(f);
    std::fprintf(f,"P6\n320 240\n255\n");
    for(unsigned p:framebuf){unsigned char b[3]={static_cast<unsigned char>(((p>>11)&31)*255/31),static_cast<unsigned char>(((p>>5)&63)*255/63),static_cast<unsigned char>((p&31)*255/31)};std::fwrite(b,1,3,f);}
    std::fclose(f);
}
static void tap(uint32_t key){Input in={};in.held=key;app_frame(in,34);app_frame({},34);}
int main(int argc,char** argv) {
    assert(argc==3||argc==4);std::filesystem::create_directories(argv[2]);
    platform::configure_host(argv[1],nullptr,0,true,false);
    std::filesystem::path root=std::filesystem::path(argv[2])/"fixture";std::filesystem::create_directories(root);
    assert(platform::initialize((root/"nNova.tns").string().c_str()));assert(app_init(platform::base_path()));
    if(argc==4&&!std::strcmp(argv[3],"ending")) {
        for(unsigned lang:{0u,9u}) {
            assert(app_init(platform::base_path()));app_test_set_language(lang);
            Game& g=app_test_game();g.start(7788);g.configureCareer(64,true);g.wave=120;g.level=45;g.score=987654;
            assert(g.equipBody(B_ASSAULT));g.player.x=75*FP;g.player.y=185*FP;g.player.angle=30;
            g.victory=true;g.state=GameOver;app_test_set_page(0);app_frame({},34);assert(app_test_page()==22);
            unsigned previous=0;for(unsigned time:{0u,2800u,6000u,10000u,16000u,17600u,18000u,22000u,32000u,36500u,38000u}) {
                app_frame({},time-previous);previous=time;shot(std::filesystem::path(argv[2])/(std::string(Locale::code(lang))+"-ending-"+std::to_string(time)+".ppm"));
            }
            assert(app_test_page()==0&&g.state==GameOver);
        }
        platform::shutdown();return 0;
    }
    if(argc==4&&!std::strcmp(argv[3],"advanced")) {
        app_test_set_language(9);app_frame({},3000);Game& g=app_test_game();g.start(719);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=140*FP;g.player.y=120*FP;g.player.angle=0;g.equipWeapon(W_SWORDS);g.equipShield(S_BASTION);
        app_test_set_page(0);for(unsigned n=0;n<120;++n)g.tick(GameInput{});shot(std::filesystem::path(argv[2])/"01-assembled.ppm");
        GameInput fire={};fire.fire=true;g.tick(fire);
        for(unsigned n=0;n<4;++n){for(unsigned t=0;t<9;++t)g.tick(GameInput{});shot(std::filesystem::path(argv[2])/("02-sword-sweep-"+std::to_string(n)+".ppm"));}
        unsigned bastion=MAX_CONSTRUCTS;for(unsigned c=0;c<MAX_CONSTRUCTS;++c)if(g.constructs[c].active&&g.constructs[c].kind==C_BASTION)bastion=c;
        assert(bastion<MAX_CONSTRUCTS);g.damageConstruct(bastion,40);shot(std::filesystem::path(argv[2])/"03-bastion-energy.ppm");
        g.player.invulnerable=0;int enemy=g.spawnEnemy(E_ASTEROID,g.player.x/FP,g.player.y/FP);assert(enemy>=0);g.enemies[enemy].age=25;g.enemies[enemy].hull=g.enemies[enemy].max_hull=100000000;g.enemies[enemy].vx=g.enemies[enemy].vy=0;g.tick(GameInput{});g.enemies[enemy].active=0;
        for(unsigned n=0;n<3;++n){for(unsigned t=0;t<3;++t)g.tick(GameInput{});shot(std::filesystem::path(argv[2])/("04-bastion-wave-"+std::to_string(n)+".ppm"));}
        platform::shutdown();return 0;
    }
    if(argc==4&&!std::strcmp(argv[3],"snapper")) {
        app_test_set_language(9);app_frame({},3000);Game& g=app_test_game();app_test_set_page(0);
        for(unsigned rarity=0;rarity<3;++rarity){g.start(6417);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=235*FP;g.player.y=140*FP;
            int enemy=g.spawnEnemy(E_SNAPPER,150,120,rarity);assert(enemy>=0);g.enemies[enemy].angle=0;
            unsigned previous=0;for(unsigned time:{40u,75u,100u,180u}){for(unsigned t=previous;t<time;++t)g.tick(GameInput{});previous=time;shot(std::filesystem::path(argv[2])/("snapper-"+std::to_string(rarity)+"-"+std::to_string(time)+".ppm"));}
            for(unsigned part=0;part<MAX_BOSS_PARTS;++part)if(g.boss_parts[part].active&&g.boss_parts[part].owner==enemy&&g.boss_parts[part].kind==BP_SNAPPER_BASE){g.boss_parts[part].active=0;g.boss_parts[part].hull=0;break;}
            for(unsigned t=0;t<3;++t){g.tick(GameInput{});}
            shot(std::filesystem::path(argv[2])/("snapper-"+std::to_string(rarity)+"-chain-removed.ppm"));
        }
        platform::shutdown();return 0;
    }
    if(argc==4&&!std::strcmp(argv[3],"celestial")) {
        app_test_set_language(9);app_frame({},3000);Game& g=app_test_game();g.start(6417);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.player.x=140*FP;g.player.y=170*FP;
        int enemy=g.spawnEnemy(E_CELESTIAL,170,110,2);assert(enemy>=0);app_test_set_page(0);
        unsigned previous=0;for(unsigned time:{70u,100u,140u,230u,253u,260u,270u,400u}){for(unsigned t=previous;t<time;++t)g.tick(GameInput{});previous=time;shot(std::filesystem::path(argv[2])/("celestial-"+std::to_string(time)+".ppm"));}
        g.start(6717);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;g.equipWeapon(W_SWORDS);g.player.mods[M_ALLY]=g.player.mods[M_INTERCEPTOR]=g.player.mods[M_X_OVERPOWER]=g.player.mods[M_OVERSEER]=1;g.recalculate();g.player.x=50*FP;g.player.y=200*FP;
        for(unsigned t=0;t<1000;++t){g.tick(GameInput{});}
        enemy=g.spawnEnemy(E_ASTEROID,180,90);assert(enemy>=0);g.enemies[enemy].hull=g.enemies[enemy].max_hull=100000000;g.enemies[enemy].vx=g.enemies[enemy].vy=0;
        for(unsigned time=0;time<4;++time){for(unsigned t=0;t<12;++t)g.tick(GameInput{});shot(std::filesystem::path(argv[2])/("interceptor-swords-"+std::to_string(time)+".ppm"));}
        platform::shutdown();return 0;
    }
    if(argc==4&&!std::strcmp(argv[3],"eclipse")) {
        for(unsigned lang:{0u,9u,4u,7u}) {
            app_test_set_language(lang);app_frame({},3000);Game& g=app_test_game();g.start(123);g.configureCareer(64,true);g.next_wave=60000;g.player.invulnerable=60000;
            int enemy=g.spawnEnemy(E_ECLIPSE,160,100);assert(enemy>=0);g.tick(GameInput{});g.state=Playing;app_test_set_page(0);
            shot(std::filesystem::path(argv[2])/(std::string(Locale::code(lang))+"-eclipse-world.ppm"));
            app_test_set_page(13);app_frame({},34);for(unsigned i=0;i<E_ECLIPSE;++i)tap(ButtonDown);tap(ButtonConfirm);
            shot(std::filesystem::path(argv[2])/(std::string(Locale::code(lang))+"-eclipse-bestiary.ppm"));
        }
        platform::shutdown();return 0;
    }
    if(argc==4) {
        const unsigned bosses[]={E_SCION,E_DREADNOUGHT,E_STATION_OMEGA,E_WARBRINGER,E_SERAPH,E_STAR_EATER,E_DWELLER,E_CERATOTITAN,E_GLAUCUS,E_CRUCIBLE,E_SPITFIRE};
        app_test_set_language(0);app_frame({},3000);
        for(unsigned id:bosses) {
            Game& g=app_test_game();g.start(123);g.configureCareer(64,true);g.configureRun(0);
            for(auto& e:g.enemies)e.active=0;
            for(auto& p:g.boss_parts)p.active=0;
            g.player.x=30*FP;g.player.y=210*FP;g.player.invulnerable=60000;g.next_wave=60000;
            int at=g.spawnEnemy(id,(id==E_STAR_EATER||id==E_GLAUCUS)?270:160,120);assert(at>=0);
            g.enemies[at].angle=0;
            if(id==E_STAR_EATER){g.enemies[at].form=2;g.enemies[at].age=61;}
            g.tick(GameInput{});g.enemies[at].boss_hidden=0;g.state=Playing;app_test_set_page(0);
            std::string prefix=std::string(argv[2])+"/boss-"+std::to_string(id);
            shot(prefix+"-world.ppm");
            for(auto& p:g.boss_parts)if(p.active){p.active=0;shot(prefix+"-part-removed.ppm");break;}
            app_test_set_page(13);app_frame({},34);for(unsigned i=0;i<id;++i)tap(ButtonDown);tap(ButtonConfirm);shot(prefix+"-bestiary.ppm");
        }
        platform::shutdown();return 0;
    }
    for(unsigned lang=0;lang<Locale::Count;++lang) {
        if(!app_test_set_language(lang))continue;
        auto directory=std::filesystem::path(argv[2])/Locale::code(lang);std::filesystem::create_directories(directory);
        app_test_game().state=Title;app_test_set_page(0);app_frame({},3000);shot(directory/"01-title.ppm");
        app_test_set_page(5);shot(directory/"02-loadout.ppm");
        app_test_set_page(1);shot(directory/"03-settings.ppm");
        app_test_set_page(6);shot(directory/"04-languages.ppm");
        app_test_set_page(2);shot(directory/"05-controls.ppm");
        Game& g=app_test_game();g.start(0x334455);g.configureCareer(64,true);g.level=25;g.wave=45;g.score=123456;
        for(unsigned pick=0;pick<3;++pick){g.giveExperience(g.xp_next);g.chooseOffer(0);}
        assert(g.equipWeapon(W_GRENADE));assert(g.equipBody(B_CARRIER));assert(g.equipShield(S_REFLECT));
        for(unsigned i=0;i<MAX_ENEMIES;++i)g.enemies[i].active=0;
        for(unsigned i=0;i<6;++i)g.spawnEnemy(E_ASTEROID,32+int(i)*49,60+int(i%2)*75);
        g.spawnEnemy(E_FRIGATE,250,165);g.spawnEnemy(E_DRONE,105,100);
        app_test_set_page(0);g.state=Playing;shot(directory/"06-combat.ppm");
        g.giveExperience(g.xp_next);g.state=Upgrade;g.banked_upgrades=1;
        g.offer_count=OFFER_COUNT;for(unsigned i=0;i<OFFER_COUNT;++i)g.offers[i]=Offer{OFFER_MOD,uint8_t(M_VOLLEY+i)};
        g.selected_offer=0;app_frame({},34);shot(directory/"07-upgrade.ppm");
        tap(ButtonTab);shot(directory/"08-description.ppm");
        tap(ButtonTab);shot(directory/"08b-upgrade-tree.ppm");
        tap(ButtonTab);
        tap(ButtonBack);g.state=Pause;app_test_set_page(0);shot(directory/"09-pause.ppm");
        app_test_set_page(3);shot(directory/"10-build.ppm");
        app_test_set_page(8);shot(directory/"11-statistics.ppm");tap(ButtonDown);shot(directory/"11a-weapon-page2.ppm");tap(ButtonDown);shot(directory/"11a-weapon-page3.ppm");
        tap(ButtonRight);shot(directory/"11d-shields-page1.ppm");tap(ButtonDown);shot(directory/"11d-shields-page2.ppm");
        tap(ButtonRight);shot(directory/"11e-body-page1.ppm");tap(ButtonDown);shot(directory/"11e-body-page2.ppm");tap(ButtonDown);shot(directory/"11e-body-page3.ppm");
        tap(ButtonRight);shot(directory/"11f-constructs-page1.ppm");tap(ButtonDown);shot(directory/"11f-constructs-page2.ppm");tap(ButtonDown);shot(directory/"11f-constructs-page3.ppm");tap(ButtonDown);shot(directory/"11f-constructs-page4.ppm");
        tap(ButtonRight);tap(ButtonRight);shot(directory/"11g-record-page1.ppm");tap(ButtonDown);shot(directory/"11g-record-page2.ppm");
        tap(ButtonRight);shot(directory/"11b-burn.ppm");tap(ButtonRight);shot(directory/"11c-global.ppm");tap(ButtonRight);
        app_test_set_page(0);g.state=GameOver;shot(directory/"12-results.ppm");
        app_test_set_page(9);shot(directory/"13-scores.ppm");tap(ButtonRight);shot(directory/"13b-record.ppm");tap(ButtonRight);shot(directory/"13c-career-records.ppm");tap(ButtonRight);
        app_test_set_page(10);shot(directory/"14-learn.ppm");
        app_test_set_page(11);shot(directory/"15-glossary.ppm");
        app_frame({},34);tap(ButtonConfirm);shot(directory/"15b-glossary-description.ppm");
        app_test_set_page(12);shot(directory/"16-unlocks.ppm");
        app_test_set_page(13);shot(directory/"17-bestiary.ppm");
        app_frame({},34);tap(ButtonConfirm);shot(directory/"17b-bestiary-description.ppm");
        app_test_set_page(14);shot(directory/"18-credits.ppm");
        g.state=Title;app_test_set_page(0);app_frame({},34);for(unsigned i=0;i<5;++i)tap(ButtonDown);tap(ButtonConfirm);
        shot(directory/"19-changelog.ppm");tap(ButtonBack);
        app_test_set_page(1);app_frame({},34);tap(ButtonRight);shot(directory/"20-controls-settings.ppm");
        tap(ButtonRight);shot(directory/"21-language-settings.ppm");tap(ButtonRight);shot(directory/"22-video-settings.ppm");
        tap(ButtonRight);shot(directory/"23-audio-settings.ppm");tap(ButtonRight);shot(directory/"24-account-settings.ppm");
        tap(ButtonRight);
        app_test_set_page(16);shot(directory/"25-rebind.ppm");tap(ButtonConfirm);shot(directory/"25b-rebind-waiting.ppm");tap(ButtonBack);
        app_test_set_page(17);shot(directory/"26-progress.ppm");
        app_test_set_page(18);shot(directory/"27-supermods.ppm");
        app_test_set_page(10);app_frame({},34);tap(ButtonRight);shot(directory/"28-learn-gear.ppm");
        app_test_set_page(14);app_frame({},34);tap(ButtonDown);shot(directory/"29-credits-page2.ppm");
        g.start(0x334455);g.giveExperience(g.xp_next);app_test_set_page(0);shot(directory/"30-first-gear.ppm");
        tap(ButtonBack);g.state=Pause;tap(ButtonDown);tap(ButtonDown);tap(ButtonDown);tap(ButtonConfirm);tap(ButtonConfirm);shot(directory/"31-resume-preview.ppm");tap(ButtonBack);
        app_test_set_page(23);app_frame({},34);shot(directory/"32-daily-calendar.ppm");
        tap(ButtonTab);shot(directory/"33-daily-rules.ppm");tap(ButtonBack);
        for(unsigned i=0;i<5;++i){tap(ButtonDown);}tap(ButtonConfirm);shot(directory/"34-daily-scores.ppm");tap(ButtonBack);
        app_test_set_page(2);app_frame({},34);tap(ButtonRight);shot(directory/"35-controls-menu.ppm");
        // A deliberately synthetic completed Daily supplies a populated local
        // record view without claiming a naturally achieved score.
        unsigned day=app_test_daily_day();g.start(Game::dailySeedFor(day));g.configureDaily(day);
        g.wave=40;g.level=30;g.score=765432;g.state=GameOver;g.victory=false;
        app_test_set_page(0);app_frame({},34);shot(directory/"36-daily-results.ppm");
        app_test_set_page(24);app_frame({},34);shot(directory/"37-daily-best.ppm");
    }
    platform::shutdown();return 0;
}
