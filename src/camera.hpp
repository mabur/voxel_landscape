#pragma once

#include "vector_space.hpp"

// In camera coordinate system:
// x is right
// y is down
// z is forward

struct CameraExtrinsics
{
    double x;
    double y;
    double z;
    double yaw;
    double pitch;
};

struct CameraIntrinsics
{
    double fx;
    double fy;
    double cx;
    double cy;
    size_t width;
    size_t height;
};

CameraIntrinsics makeCameraIntrinsics(size_t width, size_t height);

Vector4d cameraInWorld(CameraExtrinsics coordinates);

Matrix4d imageFromCamera(CameraIntrinsics intrinsics);
Matrix4d cameraFromImage(CameraIntrinsics intrinsics);
Matrix4d worldFromCamera(CameraExtrinsics extrinsics);
Matrix4d cameraFromWorld(CameraExtrinsics extrinsics);

CameraExtrinsics translateCamera(
    CameraExtrinsics extrinsics, double dx_in_cam, double dy_in_cam, double dz_in_cam
);
