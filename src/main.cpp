#define SDL_MAIN_HANDLED

#include <algorithm>
#include <cmath>
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

double clampd(double minimum, double value, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

PixelArgb packColorRgb(uint32_t r, uint32_t g, uint32_t b) {
    return (255 << 24) | (r << 16) | (g << 8) | (b << 0);
}

void unpackColorRgb(PixelArgb color, uint32_t* r, uint32_t* g, uint32_t* b) {
    *r = (color >> 16) & 0xFF;
    *g = (color >> 8) & 0xFF;
    *b = (color >> 0) & 0xFF;
}

PixelArgb interpolateColors(PixelArgb color0, PixelArgb color1, double t) {
    uint32_t r0, g0, b0, r1, g1, b1, r, g, b;
    unpackColorRgb(color0, &r0, &g0, &b0);
    unpackColorRgb(color1, &r1, &g1, &b1);
    r = clamp(0, r0 * (1 - t) + r1 * t, 255);
    g = clamp(0, g0 * (1 - t) + g1 * t, 255);
    b = clamp(0, b0 * (1 - t) + b1 * t, 255);
    return packColorRgb(r, g, b);
}

const auto DARK_SKY_COLOR = packColorRgb(0, 145, 212);
const auto LIGHT_SKY_COLOR = packColorRgb(154, 223, 255);

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

void drawSky(Array2<PixelArgb>& screen) {
    for (auto y = 0; y < screen.height(); ++y) {
        auto t = 2.0 * y / screen.height();
        auto color = interpolateColors(DARK_SKY_COLOR, LIGHT_SKY_COLOR, t);
        for (auto x = 0; x < screen.width(); ++x) {
            screen(x, y) = color;
        }
    }
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

        auto dx = point_in_world.x() / point_in_world.w() - extrinsics.x;
        auto dz = point_in_world.z() / point_in_world.w() - extrinsics.z;
        auto d = hypot(dx, dz);
        dx *= step_length / d;
        dz *= step_length / d;
        auto offset_x = 0.0;
        auto offset_z = 0.0;

        auto latest_y = screen.height();
        for (auto step = 0; step < step_count; ++step) {
            offset_x += dx;
            offset_z += dz;
            auto offset = hypot(offset_x, offset_z);
            auto shading = clampd(0, 200.0 / offset, 1);
            shading *= shading * shading * shading;
            //shading = 1.0;

            auto x = extrinsics.x + offset_x;
            auto z = extrinsics.z + offset_z;

            auto texture_u = clamp(0, x, texture.width() - 1);
            auto texture_v = clamp(0, z, texture.height() - 1);
            auto texture_color = texture(texture_u, texture_v);
            auto color = interpolateColors(LIGHT_SKY_COLOR, texture_color, shading);

            auto ground_height = -20.0;
            auto texture_point_in_world = Vector4d{x, ground_height, z, 1};
            auto texture_point_in_image = (image_from_world * texture_point_in_world).eval();
            auto next_screen_y = texture_point_in_image.y() / texture_point_in_image.w();

            if (0 <= next_screen_y && next_screen_y <= screen.height() - 1) {
                for (auto screen_y = next_screen_y; screen_y < latest_y; ++screen_y) {
                    screen(screen_x, screen_y) = color;
                }
                latest_y = next_screen_y;
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
    // extrinsics.x = texture.width() / 2;
    // extrinsics.z = -10;
    extrinsics.x = 110;
    extrinsics.z = -1;
    extrinsics.yaw = 3.14;

    for (;;) {
        printCameraCoordinates(extrinsics);
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }
        extrinsics = moveCamera(extrinsics);
        drawSky(pixels);
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
