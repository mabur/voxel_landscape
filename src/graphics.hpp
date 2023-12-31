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

struct Imaged {
    double* data;
    int width;
    int height;
};

Image makeImage(int width, int height);
Imaged makeImaged(int width, int height);

struct StepParameters {
    int step_count = 256;
    double step_size = 0.01;
};

Image readPpm(const char* file_path);

double sampleHeightMap(Image height_map, double x, double y);

void draw(
    Image screen,
    Imaged depth_buffer,
    Image texture,
    Image height_map,
    Vector4d ball_in_world,
    CameraIntrinsics intrinsics,
    CameraExtrinsics extrinsics,
    StepParameters step_parameters
);
