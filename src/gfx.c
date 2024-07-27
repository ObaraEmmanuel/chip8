#include "gfx.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>


static void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes);

void get_graphics_context(GraphicsContext* ctx){

    SDL_Init(SDL_INIT_EVERYTHING);
    ctx->window = SDL_CreateWindow(
        "SUPER CHIP 1.1 EMULATOR",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        ctx->width * ctx->scale,
        ctx->height * ctx->scale,
        SDL_WINDOW_SHOWN
        | SDL_WINDOW_OPENGL
        | SDL_WINDOW_RESIZABLE
        | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if(ctx->window == NULL){
        printf(
        "ERROR > Could not create window \n"
            "SDL_ERROR > %s \n",
            SDL_GetError()
        );
        exit(EXIT_FAILURE);
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, 0);

    if(ctx->renderer == NULL){
        printf(
            "ERROR > Could not create renderer \n"
            "SDL_ERROR > %s \n",
            SDL_GetError()
        );
        exit(EXIT_FAILURE);
    }

    ctx->texture = SDL_CreateTexture(
        ctx->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        ctx->width,
        ctx->height
    );

    if(ctx->texture == NULL){
        printf(
            "ERROR > Could not initialize texture \n"
            "SDL_ERROR > %s \n",
            SDL_GetError()
        );
        exit(EXIT_FAILURE);
    }

    int sample_nr = 0;

    SDL_AudioSpec spec;
    SDL_zero(spec);

    spec.freq = 44100; // number of samples per second
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = 2048; // buffer-size
    spec.callback = audio_callback; // function SDL calls periodically to refill the buffer
    spec.userdata = &sample_nr; // counter, keeping track of current sample number

    SDL_AudioSpec get_spec;
    ctx->audio_device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (ctx->audio_device == 0) {
        printf(
                "ERROR > Failed to open audio \n"
                "SDL_ERROR > %s \n",
                SDL_GetError()
        );
    }

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);
    SDL_RenderPresent(ctx->renderer);
}

void render_graphics(GraphicsContext* g_ctx, const uint8_t* buffer){
    SDL_SetRenderDrawColor(g_ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_ctx->renderer);
    SDL_SetRenderDrawColor(g_ctx->renderer, 255, 255, 255, 255);

    for (int y = 0; y < g_ctx->height; y++) {
        for (int x = 0; x < g_ctx->width; x++) {
            if (buffer[x + (y * g_ctx->width)]) {
                SDL_Rect rect;
                rect.x = x * g_ctx->scale;
                rect.y = y * g_ctx->scale;
                rect.w = g_ctx->scale;
                rect.h = g_ctx->scale;
                SDL_RenderFillRect(g_ctx->renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(g_ctx->renderer);
}

static void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes) {
    double sample_rate = 44100.0, time;
    int amplitude = 28000;

    Sint16* buffer = (Sint16*) raw_buffer;
    int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
    int sample_nr = *(int*) user_data;

    for (int i = 0; i < length; i++, sample_nr++) {
        time = sample_nr / sample_rate;
        buffer[i] = (Sint16)(amplitude * sin(2.0f * M_PI * 441.0f * time)); // render 441 HZ sine wave
    }
}

void beep(){
    // SDL_PauseAudio(0); // start playing sound
    // SDL_Delay(25); // wait while sound is playing
    // SDL_PauseAudio(1); // stop playing sound
}

void free_graphics(GraphicsContext* ctx){
    SDL_DestroyWindow(ctx->window);
    SDL_DestroyRenderer(ctx->renderer);
    SDL_DestroyTexture(ctx->texture);
    SDL_Quit();
}
