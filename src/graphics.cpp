#include "graphics.hpp"

#include <stdio.h>

#include "camera.hpp"

Image makeImage(int width, int height) {
    return Image{
        .data = (PixelArgb*)malloc(width * height * sizeof(PixelArgb)),
        .width = width,
        .height = height,
    };
}

Imaged makeImaged(int width, int height) {
    return Imaged{
        .data = (double*)malloc(width * height * sizeof(double)),
        .width = width,
        .height = height,
    };
}

int clampi(int minimum, int value, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

double clampd(double minimum, double value, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

int mini(int a, int b) {
    return a < b ? a : b;
}

int maxi(int a, int b) {
    return a < b ? b : a;
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
    r = clampi(0, r0 * (1 - t) + r1 * t, 255);
    g = clampi(0, g0 * (1 - t) + g1 * t, 255);
    b = clampi(0, b0 * (1 - t) + b1 * t, 255);
    return packColorRgb(r, g, b);
}

const auto DARK_SKY_COLOR = packColorRgb(0, 145, 212);
const auto LIGHT_SKY_COLOR = packColorRgb(154, 223, 255);

PixelArgb* readPpm(const char* file_path, int* width, int* height) {
    auto file = fopen(file_path, "r");
    if (file == nullptr) {
        return nullptr;
    }
    int max_intensity;
    char line[256];
    fgets(line, sizeof(line), file); // P3
    fgets(line, sizeof(line), file); // # Created by Paint Shop Pro 7 # CREATOR: GIMP PNM Filter Version 1.1
    fscanf(file, "%d %d %d", width, height, &max_intensity);
    auto pixel_count = (*width) * (*height);
    auto pixels = (PixelArgb*)malloc(pixel_count * sizeof(PixelArgb));
    for (int i = 0; i < pixel_count; ++i) {
        uint32_t r, g, b;
        fscanf(file, "%d %d %d", &r, &g, &b);
        pixels[i] = packColorRgb(r, g, b);
    }
    fclose(file);
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

void drawSky(Image screen, Imaged depth_buffer) {
    auto i = 0;
    for (auto y = 0; y < screen.height; ++y) {
        auto t = 2.0 * y / screen.height;
        auto color = interpolateColors(DARK_SKY_COLOR, LIGHT_SKY_COLOR, t);
        for (auto x = 0; x < screen.width; ++x, ++i) {
            screen.data[i] = color;
            depth_buffer.data[i] = INFINITY;
        }
    }
}

PixelArgb sampleTexture(Image texture, double x, double y) {
    auto texture_u = clampi(0, x, texture.width - 1);
    auto texture_v = clampi(0, y, texture.height - 1);
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
    Imaged depth_buffer,
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
                    auto i = screen_y * screen.width + screen_x;
                    if (depth_buffer.data[i] > dz_in_camera * total_length) {
                        depth_buffer.data[i] = dz_in_camera * total_length;
                        screen.data[i] = color;    
                    }
                }
                latest_y = mini(latest_y, next_screen_y);
            }
        }
    }
}

void drawFlag(
    Image screen,
    Imaged depth_buffer,
    Vector4d flag_in_world,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics
) {
    double POLE_HEIGHT = 10.0;
    double FLAG_WIDTH = 3.0;
    double FLAG_HEIGHT = FLAG_WIDTH * 3 / 4;
    Matrix4d image_from_world = imageFromCamera(intrinsics) * cameraFromWorld(extrinsics);
    Vector4d flag_in_image = image_from_world * flag_in_world;
    int u = int(flag_in_image.x() / flag_in_image.w());
    int v = int(flag_in_image.y() / flag_in_image.w());
    double z = flag_in_image.w() / flag_in_image.z();

    int flag_width_in_image = int(FLAG_WIDTH * intrinsics.fx / z);
    int flag_height_in_image = int(FLAG_HEIGHT * intrinsics.fy / z);
    int pole_height_in_image = int(POLE_HEIGHT * intrinsics.fy / z);

    int pole_x = u;
    int pole_ymax = v;
    int pole_ymin = v - pole_height_in_image;

    int flag_xmin = u;
    int flag_xmax = u + flag_width_in_image;
    int flag_ymin = pole_ymin;
    int flag_ymax = pole_ymin + flag_height_in_image;
    
    if (0 <= pole_x && pole_x < screen.width - 1) {
        for (auto y = maxi(pole_ymin, 0); y < mini(pole_ymax, screen.height); ++y) {
            auto i = y * screen.width + pole_x;
            if (z <= depth_buffer.data[i]) {
                depth_buffer.data[i] = z;
                screen.data[i] = packColorRgb(255, 255, 255);    
            }
        }
    }
    
    for (auto y = maxi(flag_ymin, 0); y < mini(flag_ymax, screen.height); ++y) {
        for (auto x = maxi(flag_xmin, 0); x < mini(flag_xmax, screen.width); ++x) {
            auto i = y * screen.width + x;
            if (z <= depth_buffer.data[i]) {
                depth_buffer.data[i] = z;
                screen.data[i] = packColorRgb(255, 0, 0);
            }
        }
    }
}

void drawBall(
    Image screen,
    Imaged depth_buffer,
    Vector4d ball_in_world,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics
) {
    Matrix4d image_from_world = imageFromCamera(intrinsics) * cameraFromWorld(extrinsics);
    Vector4d ball_in_image = image_from_world * ball_in_world;
    auto u = int(ball_in_image.x() / ball_in_image.w());
    auto v = int(ball_in_image.y() / ball_in_image.w());
    if (0 <= u && u < screen.width - 1 && 0 <= v && v < screen.height - 1) {
        screen.data[(v + 0) * screen.width + u + 0] = packColorRgb(255, 255, 255);
        screen.data[(v + 0) * screen.width + u + 1] = packColorRgb(255, 255, 255);
        screen.data[(v + 1) * screen.width + u + 0] = packColorRgb(255, 255, 255);
        screen.data[(v + 1) * screen.width + u + 1] = packColorRgb(255, 255, 255);
    }
}

void drawMap(
    Image screen,
    Image texture,
    Image height_map,
    Vector4d flag_in_world,
    Vector4d ball_in_world
) {
    auto scale = 8;
    for (auto y = 0; y < texture.height; ++y) {
        for (auto x = 0; x < texture.width; ++x) {
            auto target_x = screen.width - x / scale - 1;
            auto target_y = texture.height / scale - y / scale - 1;
            screen.data[target_y * screen.width + target_x] =
                texture.data[y * texture.width + x];
        }
    }
    if (0 <= flag_in_world.x() && flag_in_world.x() < texture.width &&
        0 <= flag_in_world.z() && flag_in_world.z() < texture.height
    ) {
        auto target_x = screen.width - int(flag_in_world.x()) / scale - 1;
        auto target_y = texture.height / scale - int(flag_in_world.z()) / scale - 1;
        screen.data[target_y * screen.width + target_x] = packColorRgb(255, 0, 0);
    }
    if (0 <= ball_in_world.x() && ball_in_world.x() < texture.width &&
        0 <= ball_in_world.z() && ball_in_world.z() < texture.height
    ) {
        auto target_x = screen.width - int(ball_in_world.x()) / scale - 1;
        auto target_y = texture.height / scale - int(ball_in_world.z()) / scale - 1;
        screen.data[target_y * screen.width + target_x] = packColorRgb(255, 255, 255);
    }
}

void draw(
    Image screen,
    Imaged depth_buffer,
    Image texture,
    Image height_map,
    Vector4d flag_in_world,
    Vector4d ball_in_world,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics,
    StepParameters step_parameters
) {
    drawSky(screen, depth_buffer);
    drawTexturedGround(
        screen,
        depth_buffer,
        texture,
        height_map,
        intrinsics,
        extrinsics,
        step_parameters
    );
    drawFlag(
        screen,
        depth_buffer,
        flag_in_world,
        intrinsics,
        extrinsics
    );
    drawBall(
        screen,
        depth_buffer,
        ball_in_world,
        intrinsics,
        extrinsics
    );
    drawMap(screen, texture, height_map, flag_in_world, ball_in_world);
}
