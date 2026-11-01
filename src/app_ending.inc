// Source ending: three-second centring, the 60-Hz SEQN and its overlapping
// controller alarms. The resource ends at 1860; the controller ends later.
static constexpr unsigned EndingCentreFrames=180,EndingCollision=898;
static constexpr unsigned EndingStageFrames[]={899,1199,1499,1799,1889,1979,2099};
static constexpr unsigned EndingTotalFrames=EndingCentreFrames+EndingStageFrames[6];
static unsigned ending_ms,ending_hold_ms;
static int ending_start_x,ending_start_y;
static uint8_t ending_start_angle;
static const int16_t ending_samples[][8]={
#include "ending_samples.inc"
};
static int ending_sample(unsigned frame,unsigned field) {
    unsigned at=frame/15,part=frame%15,count=sizeof(ending_samples)/sizeof(ending_samples[0]);
    if(at>=count-1){at=count-2;part=15;}
    const int16_t* a=ending_samples[at];
    const int16_t* b=ending_samples[at+1];
    return a[field]+(b[field]-a[field])*int(part)/15;
}
static void ending_sprite(const char* name,unsigned frame,int x,int y,int extent,uint8_t angle=0,unsigned opacity=255,uint16_t tint=65535,bool additive=false,int vertical=256) {
    if(extent<=0||!opacity)return;
    const Sprite* s=assets.find(name,frame);if(!s)return;
    int edge=s->canvas_width>s->canvas_height?s->canvas_width:s->canvas_height;if(!edge)return;
    int scale=extent*256/edge;if(scale<1)scale=1;if(scale>1024)scale=1024;
    int sy=scale*vertical/256;if(sy<1)sy=1;if(sy>1024)sy=1024;
    const uint16_t* colors=s->pixels;int ox=s->origin_x,oy=s->origin_y;
    if(name[0]=='s'&&name[1]=='p'&&name[2]=='r') {
        // Original kaleidoscope sprites are black alpha masks with top-left
        // pivots. The PC shader supplies their animated colour; preserve the
        // exact mask while supplying colour and centring its original canvas.
        static uint16_t mask[64*64];static bool initialized=false;
        if(s->width*s->height>64*64)return;
        if(!initialized){for(unsigned i=0;i<64*64;++i)mask[i]=65535;initialized=true;}
        colors=mask;ox+=s->canvas_width/2;oy+=s->canvas_height/2;
    }
    renderer.canvas.sprite(x,y,colors,s->alpha,s->width,s->height,ox,oy,
        dirX(angle),dirY(angle),scale,tint,opacity,additive,0,false,sy);
}
static void begin_ending() {
    ending_seen=true;ending_ms=ending_hold_ms=0;
    ending_start_x=pixel(game.player.x)*ScreenW/game.worldWidth();
    ending_start_y=pixel(game.player.y)*ScreenH/game.worldHeight();ending_start_angle=game.player.angle;
    page=EndingPage;selection=0;pending_ability=false;toast[0]=0;
}
static void finish_ending() {page=HomePage;selection=0;ending_hold_ms=0;}
static void process_ending(const Input& in,uint32_t elapsed) {
    // InputController consumes all keys/touches held across either boundary.
    // The source skip bar takes 45/60 seconds; it cannot activate a score row.
    const unsigned duration=(EndingTotalFrames*1000+59)/60;
    if(elapsed>duration-ending_ms)ending_ms=duration;else ending_ms+=elapsed;
    if(in.held&ButtonConfirm){unsigned left=750-ending_hold_ms;ending_hold_ms+=elapsed>left?left:elapsed;}
    else ending_hold_ms=0;
    if(ending_hold_ms>=750||ending_ms>=duration)finish_ending();
}
static void render_ending() {
    Canvas& c=renderer.canvas;unsigned total=ending_ms*60/1000;
    unsigned frame=total>EndingCentreFrames?total-EndingCentreFrames:0;
    int travel=-ending_sample(frame,2);renderer.background(unsigned(travel*3),true);
    // Moving original speed-line sprites, not a blurred framebuffer trail.
    if(frame<954&&frame>=179){
        unsigned speed=frame<300?1:frame<420?2:frame<841?3:5;
        for(unsigned i=0;i<18;++i){int x=int((i*83+17)%320),y=int((i*137+unsigned(travel/8)+frame*speed)%280)-20;
            ending_sprite("sAchiSpeedlineStar2",0,x,y,12+int(speed)*7,0,100+speed*20,65535,true);}
    }
    int px=160,py=120+ending_sample(frame,0)*10/92;
    uint8_t angle=192;
    if(total<EndingCentreFrames){
        int q=int(total)*256/EndingCentreFrames;q=q*q*(768-2*q)/65536;
        px=ending_start_x+(160-ending_start_x)*q/256;py=ending_start_y+(120-ending_start_y)*q/256;
        int delta=int(uint8_t(192-ending_start_angle));if(delta>128)delta-=256;
        angle=uint8_t(int(ending_start_angle)+delta*q/256);
    }
    if(frame<EndingCollision){
        int thrust=ending_sample(frame,3);
        if(thrust>0)ending_sprite("sAchiThruster",frame/3,px-dirX(angle)*16/256,py-dirY(angle)*16/256,12+thrust*14/256,angle,220);
        if(game.player.body<BODY_COUNT)ending_sprite(bodies[game.player.body].sprite,frame/8,px,py,28,angle);
    }
    if(frame>=438&&frame<900){
        unsigned opacity=unsigned(ending_sample(frame,5));
        ending_sprite("sAchiEndingGlow",0,160,-8,220,0,opacity/2,65535,true,ending_sample(frame,4));
    }
    if(frame>=719&&frame<945){
        int sy=120+ending_sample(frame,1)*10/92,scale=ending_sample(frame,6);
        unsigned alpha=frame>911?(945-frame)*255/34:255;
        ending_sprite("sAchiEndingStar2",0,160,sy,scale*158/256,0,alpha,65535,true);
    }
    if(frame>=844&&frame<926)ending_sprite("sAchiEndingStar2",0,px,py,ending_sample(frame,7)*100/256,0,200,65535,true);
    if(frame>=EndingStageFrames[0]){
        unsigned stage=1;while(stage<7&&frame>=EndingStageFrames[stage])++stage;
        unsigned age=frame-EndingStageFrames[stage-1];
        const char* rings[]={"sprite_hexagon3","sprite_hexagon1","sprite_hexagon2","sprite_dotted_lines_1","sprite_dotted_lines_2","sprite_simple_lines"};
        // Original controller uses shuffled source geometry lists. Keep the
        // choices deterministic per run, preserving the live 300/90/120 timers.
        unsigned choice=(game.seed+stage*3)%6;
        unsigned grow=age<90?age:90;int extent=35+int(grow)*120/90;
        uint8_t rotation=uint8_t((frame-899)*256/720);
        uint16_t colors[]={rgb(84,184,255),rgb(180,110,255),rgb(250,85,175),rgb(90,225,220),rgb(230,200,110)};
        uint16_t tint=colors[(game.seed+stage)%5];
        ending_sprite(rings[choice],0,160,120,extent,rotation,210,tint,true);
        ending_sprite(rings[(choice+2)%6],0,160,120,extent*3/4,uint8_t(0-rotation),210,tint,true);
        ending_sprite("sprite_hexagon_rot1",age%80,160,120,35+int(age<80?age:80),0,200,65535,true);
        if(stage<=3)for(unsigned i=0;i<6;++i){uint8_t a=uint8_t(i*256/6+rotation);int radius=20+int(grow)/2;
            ending_sprite(i&1?"sprite_triangles_1":"sprite_triangles_2",0,160+dirX(a)*radius/256,120+dirY(a)*radius/256,22+int(grow)/4,a,180,tint,true);}
        if(stage==6&&age>75)c.shade(0,0,320,240,0,(age-75)*255/45);
    }
    if(!(preferences.flags&DisableFlash)&&frame>=EndingCollision&&frame<EndingCollision+24)
        c.shade(0,0,320,240,White,(EndingCollision+24-frame)*210/24);
    // The visual remains legible when flash/shake options are disabled.
    c.shade(0,219,320,21,Panel,215);char prompt[128]="5 / Enter: ";append(prompt,sizeof(prompt),tr("UiContinue"));
    c.clipped(9,222,302,prompt,Muted);
    if(ending_hold_ms)c.bar(9,215,302,3,int(ending_hold_ms),750,Cyan);
}
