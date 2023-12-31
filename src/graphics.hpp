#pragma once

#include <stdint.h>

#include "vector_space.hpp"

struct CameraIntrinsics;
struct CameraExtrinsics;
    
using PixelArgb = uint32_t;

struct Image {
    PixelArgb* data;
    int width;
    int height;
};

struct StepParameters {
    int step_count = 256;
    double step_size = 0.01;
};

Image readPpm(const char* file_path);

void drawSky(Image screen);

void drawTexturedGround(
    Image screen,
    Image texture,
    Image height_map,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics,
    StepParameters step_parameters
);

void drawBall(
    Image screen,
    Vector4d ball_in_world,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics
);

void drawMap(Image screen, Image texture, Image height_map, CameraExtrinsics extrinsics);

double sampleHeightMap(Image height_map, double x, double y);
