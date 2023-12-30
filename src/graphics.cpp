#include "graphics.hpp"

#include <fstream>

#include "camera.hpp"

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

Image readPpm(const char* file_path) {
    auto image = Image{};
    image.data = readPpm(file_path, &image.width, &image.height);
    if (!image.data || !image.width || !image.height) {
        printf("Error reading %s data:%u width:%u height:%u", file_path, image.width, image.height);
        exit(1);
    }
    return image;
}

void drawSky(Image screen) {
    auto i = 0;
    for (auto y = 0; y < screen.height; ++y) {
        auto t = 2.0 * y / screen.height;
        auto color = interpolateColors(DARK_SKY_COLOR, LIGHT_SKY_COLOR, t);
        for (auto x = 0; x < screen.width; ++x, ++i) {
            screen.data[i] = color;
        }
    }
}

PixelArgb sampleTexture(Image texture, double x, double y) {
    auto texture_u = clamp(0, x, texture.width - 1);
    auto texture_v = clamp(0, y, texture.height - 1);
    return texture.data[texture_v * texture.width + texture_u];
}

uint32_t sampleGrayTexture(Image texture, double x, double y) {
    auto color = sampleTexture(texture, x, y);
    uint32_t gray;
    unpackColorRgb(color, &gray, &gray, &gray);
    return gray;
}

double sampleHeightMap(Image height_map, double x, double y) {
    return 0.05 * sampleGrayTexture(height_map, x, y); 
}

void drawTexturedGround(
    Image screen,
    Image texture,
    Image height_map,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics,
    StepParameters step_parameters
) {
    Matrix4d image_from_world = imageFromCamera(intrinsics) * cameraFromWorld(extrinsics);
    Matrix4d world_from_camera = worldFromCamera(extrinsics);
    Vector4d right_in_camera = {1, 0, 0, 0};
    Vector4d forward_in_camera = {0, 0, 1, 0};
    Vector4d right_in_world = world_from_camera * right_in_camera;
    Vector4d forward_in_world = world_from_camera * forward_in_camera;

    for (int screen_x = 0; screen_x < screen.width; ++screen_x) {
        double dx_in_camera = (screen_x - 0.5 * screen.width) / intrinsics.fx;
        double dz_in_camera = 1;

        Vector4d delta_in_world = dx_in_camera * right_in_world + dz_in_camera * forward_in_world;
        double dx_in_world = delta_in_world.x();
        double dz_in_world = delta_in_world.z();

        int latest_y = int(screen.height);
        for (int step = 0; step < step_parameters.step_count; ++step) {
            double total_length = step * step * step_parameters.step_size;
            double shading = clampd(0.0, 300.0 / (dz_in_camera * total_length), 1.0);
            shading *= shading * shading * shading;

            double x = extrinsics.x + dx_in_world * total_length;
            double z = extrinsics.z + dz_in_world * total_length;

            double y = sampleHeightMap(height_map, x, z);
            PixelArgb texture_color = sampleTexture(texture, x, z);
            PixelArgb color = interpolateColors(LIGHT_SKY_COLOR, texture_color, shading);

            Vector4d texture_point_in_world = {x, y, z, 1};
            Vector4d texture_point_in_image = image_from_world * texture_point_in_world;
            int next_screen_y = int(texture_point_in_image.y() / texture_point_in_image.w());

            if (0 <= next_screen_y && next_screen_y < screen.height) {
                for (int screen_y = next_screen_y; screen_y < latest_y; ++screen_y) {
                    screen.data[screen_y * screen.width + screen_x] = color;
                }
                latest_y = std::min(latest_y, next_screen_y);
            }
        }
    }
}

void drawMap(Image screen, Image texture, Image height_map, CameraExtrinsics extrinsics) {
    auto scale = 8;
    for (auto y = 0; y < texture.height; ++y) {
        for (auto x = 0; x < texture.width; ++x) {
            auto target_x = screen.width - x / scale - 1;
            auto target_y = texture.height / scale - y / scale - 1;
            screen.data[target_y * screen.width + target_x] =
                texture.data[y * texture.width + x];
        }
    }
    if (0 <= extrinsics.x && extrinsics.x < texture.width &&
        0 <= extrinsics.z && extrinsics.z < texture.height
        ) {
        auto target_x = screen.width - int(extrinsics.x) / scale - 1;
        auto target_y = texture.height / scale - int(extrinsics.z) / scale - 1;
        screen.data[target_y * screen.width + target_x] = packColorRgb(255, 255, 255);
    }
}
