#include "demake/game_app.hpp"

#include <3ds.h>
#include <cstdio>

namespace {

int showCompatibilityMessage() {
    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);
    std::printf("Ashen Rift requires New Nintendo 3DS hardware.\n\n");
    std::printf("Supported: New 3DS, New 3DS XL, New 2DS XL.\n\n");
    std::printf("Press START to return to Homebrew Launcher.\n");
    while (aptMainLoop()) {
        hidScanInput();
        if ((hidKeysDown() & KEY_START) != 0) {
            break;
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}

} // namespace

int main() {
    bool is_new_model = false;
    if (R_FAILED(APT_CheckNew3DS(&is_new_model)) || !is_new_model) {
        return showCompatibilityMessage();
    }
    osSetSpeedupEnable(true);

    demake::GameApp app;
    if (!app.initialize()) {
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
