#include "platform.h"
#include "app.h"
#if !defined(NOVA_NDLESS)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace {
alignas(4) uint16_t framebuffer[nova::ScreenWidth*nova::ScreenHeight];
}
extern "C" int main(int argc,char** argv) {
#if !defined(NOVA_NDLESS)
    const char* data=nullptr; const char* screenshot=nullptr;
    uint32_t frames=0; bool headless=false,demo=false;
    for (int i=1;i<argc;++i) {
        if (!strcmp(argv[i],"--data") && i+1<argc) data=argv[++i];
        else if (!strcmp(argv[i],"--screenshot") && i+1<argc) screenshot=argv[++i];
        else if (!strcmp(argv[i],"--frames") && i+1<argc) frames=static_cast<uint32_t>(strtoul(argv[++i],nullptr,10));
        else if (!strcmp(argv[i],"--headless")) headless=true;
        else if (!strcmp(argv[i],"--demo")) demo=true;
        else {
            fprintf(stderr,"Usage: %s [--data DIR] [--headless --frames N] [--demo] [--screenshot FILE.ppm]\n",argv[0]);
            return !strcmp(argv[i],"--help")?0:2;
        }
    }
    if (headless && !frames) frames=1;
    nova::platform::configure_host(data,screenshot,frames,headless,demo);
#endif
    if (!nova::platform::initialize(argc>0&&argv&&argv[0]?argv[0]:"nNovaDrift.tns")) return 1;
    // On a failed data load App renders the actionable error and handles exit.
    (void)nova::app_init(nova::platform::base_path());
    uint32_t previous=nova::platform::ticks_ms();
    while (!nova::app_quit()) {
        nova::Input input=nova::platform::poll();
        if (input.quit) { nova::app_frame(input,0); break; }
        uint32_t now=nova::platform::ticks_ms(), elapsed=now-previous;
#if !defined(NOVA_NDLESS)
        if (headless) elapsed=34;
        else
#endif
        if (elapsed<33) { nova::platform::wait_ms(33-elapsed); now=nova::platform::ticks_ms(); elapsed=now-previous; }
        previous=now;
        // App receives real wall time for clocks and truthful frame-rate
        // accounting. The simulation owns its own bounded catch-up policy.
        nova::app_frame(input,elapsed);
        nova::app_render(framebuffer);
        nova::platform::present(framebuffer);
    }
    nova::platform::shutdown();
    return 0;
}
