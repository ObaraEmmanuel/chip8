#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>

#include "gfx.h"
#include "chip8.h"
#include "utils.h"

static void wait_for_event(chip8* ctx);


int main(int argc, char *argv[]){
    if(argc < 2){
        printf("ERROR > Input file not provided \n");
        exit(EXIT_FAILURE);
    }

    FILE* input = fopen(argv[1], "rb");
    if(!input){
        printf("ERROR > Input file not found \n");
        exit(EXIT_FAILURE);
    }

    if(file_size(input) > (PROGRAM_END - PROGRAM_START)){
        printf("ERROR > file too big for emulator \n");
        exit(EXIT_FAILURE);
    }

    chip8 ctx;
    init_emulator(input, &ctx);
    ctx.debug = 0;
    fclose(input);

    GraphicsContext g_ctx;
    g_ctx.width = SCREEN_WIDTH;
    g_ctx.height = SCREEN_HEIGHT;
    g_ctx.scale = WINDOW_WIDTH / SCREEN_WIDTH;
    get_graphics_context(&g_ctx);
    int paused = 1;
    SDL_PauseAudioDevice(g_ctx.audio_device, paused);

    while (!ctx.exit){
        execute(&ctx);
        ctx.step_cycles++;
        if(ctx.draw){
            render_graphics(&g_ctx, ctx.screen);
            ctx.draw = 0;
        }

        do{
            wait_for_event(&ctx);
        } while (ctx.wait && !ctx.exit);

        if(ctx.step_cycles == CLOCK_DIV){
            ctx.delay_timer -= (ctx.delay_timer > 0);
            ctx.sound_timer -= (ctx.sound_timer > 0);
            if(ctx.sound_timer > 0 && paused){
                paused = 0;
                SDL_PauseAudioDevice(g_ctx.audio_device, paused);
            }
            if(ctx.sound_timer == 0) {
                paused = 1;
                SDL_PauseAudioDevice(g_ctx.audio_device, paused);
            }
            ctx.step_cycles = 0;
        }
#ifdef WIN32
        Sleep(CPU_CLOCK_DELAY);
#else
        usleep(CPU_CLOCK_DELAY);
#endif
    }
    free_graphics(&g_ctx);
    return 0;
}

static void wait_for_event(chip8* ctx){
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        switch (e.type) {
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        ctx->exit = 1;
                        break;
                    case SDLK_SPACE:
                        ctx->wait = !ctx->wait;
                        break;
                    case SDLK_F5:
                        reset_emulator(ctx);
                        break;
                    default:
                        break;
                }
                for(size_t i = 0; i < NUM_KEYS; i++){
                    if(e.key.keysym.sym == KEYMAP[i]){
                        ctx->keyboard[i] = 1;
                    }
                }
                break;
            case SDL_KEYUP:
                for (int i = 0; i < NUM_KEYS; i++) {
                    if (e.key.keysym.sym == KEYMAP[i]) {
                        ctx->keyboard[i] = 0;
                    }
                }
                break;
            case SDL_QUIT:
                ctx->exit = 1;
        }
    }
}
