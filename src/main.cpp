#define SDL_MAIN_HANDLED

#include <vector>

#include "input.hpp"
#include "window.hpp"
#include <SDL2/SDL.h>

uint32_t packColorRgb(uint32_t r, uint32_t g, uint32_t b) {
    return (255 << 24) | (r << 16) | (g << 8) | (b << 0);
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        handleSdlError("SDL_Init");
    }
    const auto WIDTH = 320;
    const auto HEIGHT = 200;
    const auto NUM_PIXELS = WIDTH * HEIGHT;
    auto window = makeFullScreenWindow(WIDTH, HEIGHT, "Voxel Landscape");
    auto pixels = std::vector<uint32_t>(NUM_PIXELS, packColorRgb(0, 0, 255));
    SDL_ShowCursor(SDL_DISABLE);
    for (;;) {
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }

        drawPixels(window, pixels.data());
        presentWindow(window);
    }
    destroyWindow(window);
    SDL_Quit();
    return 0;
}
