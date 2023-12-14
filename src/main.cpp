#define SDL_MAIN_HANDLED

#include "input.hpp"
#include "window.hpp"
#include <SDL2/SDL.h>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        handleSdlError("SDL_Init");
    }
    auto window = makeFullScreenWindow(320, 200, "Voxel Landscape");
    SDL_ShowCursor(SDL_DISABLE);
    for (;;) {
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }
        presentWindow(window);
    }
    destroyWindow(window);
    SDL_Quit();
    return 0;
}
