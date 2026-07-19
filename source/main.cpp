#include "demake/game_app.hpp"

#include <3ds.h>

int main() {
    bool is_new_model = false;
    if (R_SUCCEEDED(APT_CheckNew3DS(&is_new_model)) && is_new_model) {
        // Preserve optional compatibility with newer systems without making
        // their extra CPU mode a requirement for the original-3DS build.
        osSetSpeedupEnable(true);
    }

    demake::GameApp app;
    if (!app.initialize()) {
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
