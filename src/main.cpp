#define SDL_MAIN_HANDLED

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>

#include <Eigen/Core>
#include <SDL2/SDL.h>

#include "algorithm.hpp"
#include "array2.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "window.hpp"

using PixelArgb = uint32_t;

int clamp(int minimum, int value, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

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

Array2<PixelArgb> readPpm(const char* file_path) {
    auto width = 0;
    auto height = 0;
    auto data = readPpm(file_path, &width, &height);
    auto image = Array2<PixelArgb>(width, height, 0);
    std::copy(data, data + image.size(), image.data());
    return image;
}

CameraExtrinsics moveCamera(CameraExtrinsics extrinsics) {
    auto speed = 1.0;
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

void drawTexturedGround(
    Array2<PixelArgb>& screen,
    const Array2<PixelArgb>& texture,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics
) {
    auto image_from_world = (imageFromCamera(intrinsics) * cameraFromWorld(extrinsics)).eval();
    auto world_from_image = image_from_world.inverse().eval();

    for (auto screen_x = 0; screen_x < screen.width(); ++screen_x) {
        auto step_count = 200;
        auto step_length = 2.0;
        auto point_in_image = Vector4d{double(screen_x), 0, 1, 1};
        auto point_in_world = world_from_image * point_in_image;
        auto x = extrinsics.x;
        auto z = extrinsics.z;
        auto dx = point_in_world.x() / point_in_world.w() - x;
        auto dz = point_in_world.z() / point_in_world.w() - z;
        auto d = sqrt(dx * dx + dz * dz);
        dx *= step_length / d;
        dz *= step_length / d;

        if (screen_x == 0) {
            printf("dx=%.2f dz=%.2f\n", dx, dz);
        }

        auto latest_y = screen.height();
        for (auto step = 0; step < step_count; ++step) {
            x += dx;
            z += dz;
            auto texture_u = clamp(0, x, texture.width() - 1);
            auto texture_v = clamp(0, z, texture.height() - 1);
            auto color = texture(texture_u, texture_v);

            auto ground_height = -20.0;
            auto texture_point_in_world = Vector4d{x, ground_height, z, 1};
            auto texture_point_in_image = (image_from_world * texture_point_in_world).eval();
            auto screen_y = texture_point_in_image.y() / texture_point_in_image.w();

            if (0 <= screen_y && screen_y <= screen.height() - 1) {
                for (auto y = screen_y; y < latest_y; ++y) {
                    screen(screen_x, y) = color;
                }
                latest_y = screen_y;
            }
        }
    }
}

void printCameraCoordinates(CameraExtrinsics extrinsics) {
    auto forward_in_camera = Vector4d{0, 0, 1, 0};
    auto forward_in_world = (worldFromCamera(extrinsics) * forward_in_camera).eval();

    printf("Camera position %.0f %.0f %.0f\n", extrinsics.x, extrinsics.y, extrinsics.z);
    printf("Forward direction %.0f %.0f %.0f\n", forward_in_world.x(), forward_in_world.y(), forward_in_world.z());
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        handleSdlError("SDL_Init");
    }
    auto WIDTH = 320;
    auto HEIGHT = 200;
    auto window = makeFullScreenWindow(WIDTH, HEIGHT, "Voxel Landscape");
    auto pixels = Array2<PixelArgb>(WIDTH, HEIGHT, 0);
    SDL_ShowCursor(SDL_DISABLE);
    auto texture = readPpm("images/texture.ppm");
    auto intrinsics = makeCameraIntrinsics(WIDTH, HEIGHT);
    auto extrinsics = CameraExtrinsics{};
    extrinsics.x = texture.width() / 2;
    extrinsics.z = -10;
    extrinsics.yaw = 3.14;
    printCameraCoordinates(extrinsics);

    for (;;) {
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }
        extrinsics = moveCamera(extrinsics);
        fill(pixels, packColorRgb(0, 0, 0));
        drawTexturedGround(
            pixels,
            texture,
            intrinsics,
            extrinsics
        );
        drawPixels(window, pixels.data());
        presentWindow(window);
    }
    destroyWindow(window);
    SDL_Quit();
    return 0;
}
