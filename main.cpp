#include "app.h"

int main(int argc, char* argv[]) {
    App app;
    if (!app.init()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Init failed",
            "Could not initialize app. Check assets/font.ttf exists.", nullptr);
        return 1;
    }
    app.run();
    app.quit();
    return 0;
}
