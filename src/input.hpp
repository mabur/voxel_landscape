#pragma once

#include <SDL2/SDL_scancode.h>

struct SDL_Point;
struct SDL_Renderer;

// This function should be called exactly once per frame.
// After that the other functions below can be called to access the input.
void registerFrameInput(SDL_Renderer* renderer);

bool hasReceivedQuitEvent();

SDL_Point getAbsoluteMousePosition();
SDL_Point getRelativeMousePosition();

bool isKeyUp(SDL_Scancode key);
bool isKeyClicked(SDL_Scancode key);
bool isKeyDown(SDL_Scancode key);
bool isKeyReleased(SDL_Scancode key);

bool isLeftMouseButtonUp();
bool isLeftMouseButtonClicked();
bool isLeftMouseButtonDown();
bool isLeftMouseButtonReleased();

bool isMiddleMouseButtonUp();
bool isMiddleMouseButtonClicked();
bool isMiddleMouseButtonDown();
bool isMiddleMouseButtonReleased();

bool isRightMouseButtonUp();
bool isRightMouseButtonClicked();
bool isRightMouseButtonDown();
bool isRightMouseButtonReleased();
