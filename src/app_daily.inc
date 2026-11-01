// Calculator daily challenge pages. The source challenge rules live in Game;
// calendar choice and local personal records remain an application concern.
static const char* const daily_variant_keys[]={
    "DailySerpentineDrift","DailyBossRush","Daily20PointDraft","Daily50PointDraft","DailyCosmicEgg",
    "DailyPurity","DailyTurbo","DailyTurboEndless","DailyWayOfTheSword","DailyNovaBrothers"
};
static const char* const daily_modifier_keys[]={
    "","DailyGraceDesc","DailyUnbreakableDesc","DailyUnstoppableDesc","DailyImmovableDesc",
    "DailyHeistDesc","DailyHyperactiveDesc","DailyCompactDesc","DailyHurtMePlentyDesc","DailyUltraViolenceDesc",
    "DailyImTheBossNowDesc","DailyChosenOneDesc","DailyWilderMetamorphosisDesc","DailyPandemoniumDesc",
    "DailyCageMatchDesc","DailyTougherStuffDesc","DailyRogueHarderDesc","DailyVengeanceDesc",
    "DailySlipperyFishDesc","DailyFrenzyDesc","DailyLongShotDesc","DailyCelestialShowerDesc","DailyCosmicBalletDesc","DailyWildernessDesc"
};
static uint32_t selected_daily_day(){return calendar_day(daily_date);}
static const char* daily_title(uint32_t day){return tr(daily_variant_keys[Game::dailyVariantFor(day)%10]);}
static void daily_rules(char* out,unsigned capacity) {
    out[0]=0;uint32_t day=selected_daily_day();
    unsigned variant=Game::dailyVariantFor(day)%10,modifier=Game::dailyModifierFor(day)%24;
    char key[80]={0};append(key,sizeof(key),daily_variant_keys[variant]);append(key,sizeof(key),"Desc");
    append(out,capacity,daily_title(day));append(out,capacity,"\n\n");
    char formatted[4096];Locale::format(formatted,sizeof(formatted),tr(key),true);append(out,capacity,formatted);
    if(modifier){append(out,capacity,"\n\n");Locale::format(formatted,sizeof(formatted),tr(daily_modifier_keys[modifier]),true);append(out,capacity,formatted);}
}
static void open_daily() {
    uint32_t day=platform::utc_day();if(!day)day=daily_profile.last_day;
    daily_date_unset=!day;daily_date=calendar_date(day);selection=0;page=DailyPage;
}
static void process_daily(const Input& in) {
    if(pressed(in,ButtonBack)){page=ModePage;selection=ModeCount;return;}
    move_menu(in,6,43,23,12,296);
    if(pressed(in,ButtonLeft)||pressed(in,ButtonRight)) {
        int delta=pressed(in,ButtonLeft)?-1:1;
        if(selection==0)daily_date.year=unsigned((int(daily_date.year)-2020+delta+80)%80+2020);
        else if(selection==1)daily_date.month=unsigned((int(daily_date.month)-1+delta+12)%12+1);
        else if(selection==2)daily_date.day=unsigned((int(daily_date.day)-1+delta+int(calendar_days(daily_date.year,daily_date.month)))%int(calendar_days(daily_date.year,daily_date.month))+1);
        if(selection<3){unsigned days=calendar_days(daily_date.year,daily_date.month);if(daily_date.day>days)daily_date.day=days;daily_date_unset=false;}
        return;
    }
    if(pressed(in,ButtonTab)||(selection==4&&activate(in,43,23,12,296))) {
        reference_title[0]=0;append(reference_title,sizeof(reference_title),tr("UiDailyRules"));
        detail_mode=-5;detail_return=DailyPage;detail_selection=selection;detail_scroll=detail_lines=0;detail_tree=false;page=DetailPage;return;
    }
    if(!activate(in,43,23,12,296))return;
    if(selection<3){daily_date_unset=false;selection=(selection+1)%6;return;}
    if(selection==3){
        daily_date_unset=false;pending_run_day=selected_daily_day();
        if(has_save||incompatible_save){page=NewRunPage;selection=0;}else start_run(pending_run_day);
    } else if(selection==5){page=DailyScoresPage;selection=0;}
}
static unsigned daily_record_count() {
    unsigned count=0;for(const DailyRecord& r:daily_profile.records)if(r.day)++count;return count;
}
static void process_daily_scores(const Input& in) {
    unsigned count=daily_record_count();
    if(pressed(in,ButtonBack)){page=DailyPage;selection=5;return;}
    if(!count)return;
    int first=selection/5*5;move_menu(in,int(count),43,26,12,296,first,5);first=selection/5*5;
    if(activate(in,43,26,12,296,first)) {
        daily_date=calendar_date(daily_profile.records[selection].day);daily_date_unset=false;page=DailyPage;selection=3;
    }
}
static void render_daily() {
    Canvas& c=renderer.canvas;heading(tr("UiDailyOffline"));
    const char* labels[]={"UiYear","UiMonth","UiDay","LoadoutStart","UiDailyRules","UiLocalScores"};
    const unsigned values[]={daily_date.year,daily_date.month,daily_date.day};
    for(unsigned i=0;i<6;++i){int y=43+int(i)*23;button(12,y,296,22,tr(labels[i]),selection==int(i),i<3?80:0);
        if(i<3){char value[24];decimal(value,values[i]);c.right(303,y+3,value,selection==int(i)?Cyan:Muted);}}
    divider(12,183,296);
    c.clipped(12,187,296,daily_date_unset?tr("UiDailySetDate"):daily_title(selected_daily_day()),Cyan);
    char hint[96]={0};append(hint,sizeof(hint),physical_key_name(input_controller.bindings().key[BindAbility]));append(hint,sizeof(hint),": ");append(hint,sizeof(hint),tr("UiDailyRules"));c.clipped(12,203,296,hint,Muted);
    footer(tr(selection<3?"UiChange":"UiChoose"),selection<3?"4 / 6":"5 / Enter");
}
static void render_daily_scores() {
    Canvas& c=renderer.canvas;heading(tr("UiLocalScores"));unsigned count=daily_record_count();
    if(!count){c.wrapped(16,77,288,3,tr("UiNoRecords"),Muted);footer(tr("UiBack"),"Esc");return;}
    int first=selection/5*5;
    for(unsigned i=unsigned(first);i<count&&i<unsigned(first+5);++i){int y=43+(int(i)-first)*26;char date[11],score[24];calendar_text(date,daily_profile.records[i].day);decimal(score,daily_profile.records[i].run.score);
        button(12,y,296,25,date,selection==int(i),110);c.right(303,y+4,score,selection==int(i)?Cyan:Muted);}
    const DailyRecord& record=daily_profile.records[selection];divider(12,178,296);
    renderer.icon(Offer{OFFER_WEAPON,record.run.weapon},27,199,27);renderer.icon(Offer{OFFER_SHIELD,record.run.shield},56,199,27);renderer.icon(Offer{OFFER_BODY,record.run.body},85,199,27);
    value_label(110,185,"GUIWave",record.run.wave,Cyan);value_label(110,202,"GUILevel",record.run.level,Muted);
    char time[24]={0};appendNumber(time,sizeof(time),record.run.ticks/TICK_RATE);append(time,sizeof(time)," s");c.right(306,202,time,Muted);
    footer(tr("UiChoose"));
}
