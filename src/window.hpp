#pragma once

#include <stdint.h>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Rect;

void handleSdlError(const char* context);

struct SdlWindow {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int width;
    int height;
};

SdlWindow makeFullScreenWindow(int width, int height, const char* window_title);
void destroyWindow(SdlWindow window);
void presentWindow(SdlWindow window);
void drawPixels(SdlWindow window, const uint32_t* pixels_argb);

struct SdlTexture {
    int width;
    int height;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
};

SdlTexture readTextureFromPpm(SdlWindow window, const char* file_path);
void drawTexture(SdlTexture texture);
void drawTextureAt(SdlTexture texture, int x, int y);
void drawTexturePatch(SdlTexture texture, SDL_Rect source);
void drawTexturePatchAt(SdlTexture texture, SDL_Rect source, int x, int y);
