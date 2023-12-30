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

enum BallState {BALL_MOVING, BALL_STILL};

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

struct Player {
    CameraIntrinsics intrinsics;
    CameraExtrinsics extrinsics;
    BallState ball_state;
    Vector4d ball_velocity_in_world;
};

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
    auto player = Player{
        .intrinsics = makeCameraIntrinsics(WIDTH, HEIGHT),
        .extrinsics = CameraExtrinsics{ .x = 110, .y = 20, .z = -1, .yaw = 3.14 },
        .ball_state = BALL_STILL,
        .ball_velocity_in_world = Vector4d{ 0, 0, 0, 0 },
    };
    
    for (;;) {
        registerFrameInput(window.renderer);
        if (hasReceivedQuitEvent() || isKeyDown(SDL_SCANCODE_ESCAPE)) {
            break;
        }
        player.extrinsics = moveCamera(player.extrinsics);
        auto step_parameters = getStepParameters();
        
        if (isKeyReleased(SDL_SCANCODE_SPACE)) {
            player.ball_state = BALL_MOVING;
            auto ball_velocity_in_camera = Vector4d{0, -0.5, 0.5, 0};
            auto world_from_camera = worldFromCamera(player.extrinsics);
            player.ball_velocity_in_world = world_from_camera * ball_velocity_in_camera;
        }
        player.extrinsics.x += player.ball_velocity_in_world.x();
        player.extrinsics.y += player.ball_velocity_in_world.y();
        player.extrinsics.z += player.ball_velocity_in_world.z();
        if (player.ball_state == BALL_MOVING) {
            player.ball_velocity_in_world.y() -= 0.003;
            auto ground_height = sampleHeightMap(height_map, player.extrinsics.x, player.extrinsics.z);
            if (player.extrinsics.y < ground_height) {
                player.extrinsics.y = ground_height;
                player.ball_velocity_in_world.y() *= -1;
                player.ball_velocity_in_world *= 0.5;
                if (player.ball_velocity_in_world.norm() < 0.1) {
                    player.ball_velocity_in_world = {0, 0, 0, 0};
                    player.ball_state = BALL_STILL;
                }
            }
        }
        
        drawSky(screen);
        drawTexturedGround(
            screen,
            texture,
            height_map,
            player.intrinsics,
            player.extrinsics,
            step_parameters
        );
        drawMap(screen, texture, height_map, player.extrinsics);
        drawPixels(window, screen.data);
        presentWindow(window);
    }
    destroyWindow(window);
    SDL_Quit();
    return 0;
}
