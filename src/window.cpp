#include "window.hpp"

#include <vector>
#include <fstream>

#include <SDL2/SDL.h>

typedef uint32_t PixelArgb;

static std::vector<SDL_Texture*> s_textures;

void handleSdlError(const char* context) {
    SDL_Log("Error for %s. %s", context, SDL_GetError());
    exit(EXIT_FAILURE);
}

static SDL_Texture* makeSdlTexture(SDL_Renderer* renderer, int width, int height) {
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    if (!texture) {
        handleSdlError("SDL_CreateTexture");
    }
    s_textures.push_back(texture);
    return texture;
}

SdlWindow makeFullScreenWindow(int width, int height, const char* window_title) {
    const auto window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    if (!window) {
        handleSdlError("SDL_CreateWindow");
    }
    const auto renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        handleSdlError("SDL_CreateRenderer");
    }
    if (SDL_RenderSetLogicalSize(renderer, width, height) != 0) {
        handleSdlError("SDL_RenderSetLogicalSize");
    }
    const auto texture = makeSdlTexture(renderer, width, height);
    return SdlWindow{
        .window = window,
        .renderer = renderer,
        .texture = texture,
        .width = width,
        .height = height,
    };
}

void destroyWindow(SdlWindow window) {
    for (auto texture : s_textures) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(window.renderer);
    SDL_DestroyWindow(window.window);
}

void presentWindow(SdlWindow window) {
    SDL_RenderPresent(window.renderer);
}

void drawPixels(SdlWindow window, const PixelArgb* pixels_argb) {
    SDL_UpdateTexture(window.texture, nullptr, pixels_argb, window.width * sizeof(PixelArgb));
    SDL_RenderCopy(window.renderer, window.texture, nullptr, nullptr);
}

static
bool isTransparent(Uint32 r, Uint32 g, Uint32 b) {
    return r == 0 && g == 255 && b == 255;
}

static
PixelArgb packColorArgb(Uint32 a, Uint32 r, Uint32 g, Uint32 b)
{
    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

static PixelArgb* readPpm(const char* file_path, int* width, int* height) {
    using namespace std;
    ifstream file(file_path);
    if (file.fail()) {
        return 0;
    }
    string line;
    getline(file, line); // P3
    getline(file, line); // # Created by Paint Shop Pro 7 # CREATOR: GIMP PNM Filter Version 1.1
    int max_intensity;
    file >> *width >> *height >> max_intensity;

    const auto pixel_count = (*width) * (*height);

    PixelArgb* pixels = (PixelArgb*)malloc(pixel_count * sizeof(PixelArgb));
    for (auto i = 0; i < pixel_count; ++i) {
        int r, g, b;
        file >> r >> g >> b;
        int a = isTransparent(r, g, b) ? 0 : 255;
        pixels[i] = packColorArgb(a, r, g, b);
    }
    file.close();
    
    return pixels;
}

SdlTexture readTextureFromPpm(SdlWindow window, const char* file_path) {
    int width;
    int height;
    SDL_Log("Loading texture %s ... ", file_path);
    PixelArgb* pixels = readPpm(file_path, &width, &height);
    if (pixels == 0) {
        SDL_Log("Could not read %s", file_path);
        exit(1);
    }
    const auto texture = makeSdlTexture(window.renderer, width, height);
    int error;
    error = SDL_UpdateTexture(
        texture, nullptr, pixels, width * sizeof(PixelArgb)
    );
    if (error) {
        handleSdlError("SDL_UpdateTexture");
    }
    error = SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (error) {
        handleSdlError("SDL_SetTextureBlendMode");
    }
    free(pixels);
    SDL_Log("Done");
    return SdlTexture{ width, height, window.renderer, texture };
}

void drawTexture(SdlTexture texture){
    const auto target = SDL_Rect{ 0, 0, texture.width, texture.height };
    SDL_RenderCopy(texture.renderer, texture.texture, NULL, &target);
}

void drawTextureAt(SdlTexture texture, int x, int y){
    const auto target = SDL_Rect{ x, y, texture.width, texture.height };
    SDL_RenderCopy(texture.renderer, texture.texture, NULL, &target);
}

void drawTexturePatch(SdlTexture texture, SDL_Rect source) {
    const auto target = SDL_Rect{ .x=0, .y=0, .w=source.w, .h=source.h };
    SDL_RenderCopy(texture.renderer, texture.texture, &source, &target);
}

void drawTexturePatchAt(SdlTexture texture, SDL_Rect source, int x, int y) {
    const auto target = SDL_Rect{ .x=x, .y=y, .w=source.w, .h=source.h };
    SDL_RenderCopy(texture.renderer, texture.texture, &source, &target);
}
