#pragma once

#include <SDL2/SDL.h>

const static int KEYMAP[0x10] = {
        SDLK_x, // 0
        SDLK_1, // 1
        SDLK_2, // 2
        SDLK_3, // 3
        SDLK_w, // 4
        SDLK_LEFT, // 5
        SDLK_RIGHT, // 6
        SDLK_a, // 7
        SDLK_s, // 8
        SDLK_d, // 9
        SDLK_z, // A
        SDLK_c, // B
        SDLK_4, // C
        SDLK_r, // D
        SDLK_f, // E
        SDLK_v  // F
};

typedef struct{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int width;
    int height;
    int scale;

} GraphicsContext;

void free_graphics(GraphicsContext* ctx);

void beep();

void get_graphics_context(GraphicsContext* ctx);

void render_graphics(GraphicsContext* g_ctx, const uint8_t* buffer);