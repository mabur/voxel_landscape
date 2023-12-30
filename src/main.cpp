#define SDL_MAIN_HANDLED

#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <Eigen/Core>
#include <iglo/input.hpp>
#include <iglo/window.hpp>
#include <SDL2/SDL.h>

#include "camera.hpp"
#include "graphics.hpp"

CameraExtrinsics moveCamera(CameraExtrinsics extrinsics) {
    auto SPEED = 1.0;
    auto ANGLE_SPEED = 3.14 / 180 * 1;
    auto x = 0.0;
    auto y = 0.0;
    auto z = 0.0;
    if (isKeyDown(SDL_SCANCODE_D)) {
        x += SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_A)) {
        x -= SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_W)) {
        z += SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_S)) {
        z -= SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_RCTRL)) {
        y += SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_RSHIFT)) {
        y -= SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_RIGHT)) {
        extrinsics.yaw += ANGLE_SPEED;
    }
    if (isKeyDown(SDL_SCANCODE_LEFT)) {
        extrinsics.yaw -= ANGLE_SPEED;
    }
    return translateCamera(extrinsics, x, y, z);
}

StepParameters getStepParameters() {
    static auto parameters = StepParameters{};
    if (isKeyReleased(SDL_SCANCODE_1)) {
        parameters.step_size *= 1.1;
        printf("step_size %.4f\n", parameters.step_size);
    }
    if (isKeyReleased(SDL_SCANCODE_2)) {
        parameters.step_size *= 0.9;
        printf("step_size %.4f\n", parameters.step_size);
    }
    if (isKeyReleased(SDL_SCANCODE_3)) {
        parameters.step_count += 8;
        printf("step_count %d\n", parameters.step_count);
    }
    if (isKeyReleased(SDL_SCANCODE_4)) {
        parameters.step_count -= 8;
        printf("step_count %d\n", parameters.step_count);
    }
    return parameters;
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
    auto screen = Image{};
    screen.width = WIDTH;
    screen.height = HEIGHT;
    screen.data = (PixelArgb*)malloc(WIDTH * HEIGHT * sizeof(PixelArgb));
    SDL_ShowCursor(SDL_DISABLE);
    auto texture = readPpm("images/texture.ppm");
    auto height_map = readPpm("images/height_map.ppm");
    
    if (texture.width != height_map.width || texture.height != height_map.height) {
        printf("Texture and height map should have the same size");
        exit(1);
    }
    
    
    auto intrinsics = makeCameraIntrinsics(WIDTH, HEIGHT);
    auto extrinsics = CameraExtrinsics{};
    // extrinsics.x = texture.width() / 2;
    // extrinsics.z = -10;
    extrinsics.x = 110;
    extrinsics.y = 20;
    extrinsics.z = -1;
    extrinsics.yaw = 3.14;

    for (;;) {
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }
        extrinsics = moveCamera(extrinsics);
        auto step_parameters = getStepParameters();
        drawSky(screen);
        drawTexturedGround(
            screen,
            texture,
            height_map,
            intrinsics,
            extrinsics,
            step_parameters
        );
        drawMap(screen, texture, height_map, extrinsics);
        drawPixels(window, screen.data);
        presentWindow(window);
    }
    destroyWindow(window);
    SDL_Quit();
    return 0;
}
