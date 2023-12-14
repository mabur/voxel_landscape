#define SDL_MAIN_HANDLED

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>

#include <Eigen/Core>
#include <SDL2/SDL.h>

#include "algorithm.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "window.hpp"

using PixelArgb = uint32_t;

PixelArgb packColorRgb(uint32_t r, uint32_t g, uint32_t b) {
    return (255 << 24) | (r << 16) | (g << 8) | (b << 0);
}

PixelArgb* readPpm(const char* file_path, int* width, int* height) {
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
        pixels[i] = packColorRgb(r, g, b);
    }
    file.close();

    return pixels;
}

CameraExtrinsics moveCamera(CameraExtrinsics extrinsics) {
    auto speed = 0.1;
    if (isKeyDown(SDL_SCANCODE_LEFT)) {
        extrinsics.x += speed;
    }
    if (isKeyDown(SDL_SCANCODE_RIGHT)) {
        extrinsics.x -= speed;
    }
    if (isKeyDown(SDL_SCANCODE_UP)) {
        extrinsics.z += speed;
    }
    if (isKeyDown(SDL_SCANCODE_DOWN)) {
        extrinsics.z -= speed;
    }
    return extrinsics;
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        handleSdlError("SDL_Init");
    }
    auto WIDTH = 320;
    auto HEIGHT = 200;
    auto NUM_PIXELS = WIDTH * HEIGHT;
    auto window = makeFullScreenWindow(WIDTH, HEIGHT, "Voxel Landscape");
    auto pixels = std::vector<PixelArgb>(NUM_PIXELS);
    SDL_ShowCursor(SDL_DISABLE);
    auto intrinsics = makeCameraIntrinsics(WIDTH, HEIGHT);
    auto extrinsics = CameraExtrinsics{};
    extrinsics.z = -10;

    auto texture_width = 0;
    auto texture_height = 0;
    auto texture_data = readPpm("images/texture.ppm", &texture_width, &texture_height);
    if (texture_data == nullptr) {
        printf("could not read texture\n");
        exit(1);
    } else {
        printf("Read texture width=%i height=%i\n", texture_width, texture_height);
    }

    auto points = Vectors4d{
        {1,1,1,1},{1,-1,1,1},{-1,-1,1,1},{-1,1,1,1},
        {1,1,-1,1},{1,-1,-1,1},{-1,-1,-1,1},{-1,1,-1,1},
    };

    for (;;) {
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }
        extrinsics = moveCamera(extrinsics);

        fill(pixels, packColorRgb(0, 0, 0));

        for (auto y = 0; y < std::min(texture_height, HEIGHT); ++y) {
            for (auto x = 0; x < texture_width; ++x) {
                pixels[y * WIDTH + x] = texture_data[y * texture_width + x];
            }
        }

        auto image_from_world = (imageFromCamera(intrinsics) * cameraFromWorld(extrinsics)).eval();
        for (auto point_in_world : points) {
            auto point_in_image = (image_from_world * point_in_world).eval();
            auto u = int(point_in_image.x() / point_in_image.w());
            auto v = int(point_in_image.y() / point_in_image.w());
            if (0 <= u && u < WIDTH && 0 <= v && v < HEIGHT) {
                pixels[v * WIDTH + u] = packColorRgb(255, 255, 255);
            }
        }

        drawPixels(window, pixels.data());
        presentWindow(window);
    }
    destroyWindow(window);
    SDL_Quit();
    return 0;
}
