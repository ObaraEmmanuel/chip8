#pragma once

#include <stdint.h>
#include <stdio.h>

#define RAM_SIZE 4096
#define STACK_SIZE 16
#define FONT_SET_SIZE 80
#define SUPER_FONT_SET_SIZE 100
#define OP_SIZE 2

#define NUM_REGISTERS 16
#define NUM_FLAGS 8
#define VF_IDX 15

#define PROGRAM_START 0x200
#define PROGRAM_END 0xFF

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512
#define SCROLL_STEP 4

#ifdef WIN32
#define CPU_CLOCK_DELAY 1 //ms
#else
#define CPU_CLOCK_DELAY 1000 //us
#endif

#define CLOCK_DIV 9

#define NUM_KEYS 16

const static uint8_t FONT_SET[FONT_SET_SIZE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


static uint8_t SUPER_FONT_SET[SUPER_FONT_SET_SIZE] = {
    0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, /* 0 */
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, /* 1 */
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, /* 2 */
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, /* 3 */
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, /* 4 */
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, /* 5 */
    0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, /* 6 */
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, /* 7 */
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, /* 8 */
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, /* 9 */
}; /* 8x10 pixel font patterns (only 10) */


typedef enum {
    LOW_RES64 = 0,
    HIGH_RES128 = 1
} SCREEN_MODE;


/*
* opcode anatomy
*
* |<--------------|<-----kk------>|
* |<--op->|<---------addr-------->|
* |<--op->|<--x-->|<--y-->|<--n-->|
*/

typedef struct {
    uint8_t op;
    uint16_t addr;
    uint8_t x;
    uint8_t y;
    uint8_t n;
    uint8_t kk;
    uint16_t full_op;
} opcode;


typedef struct {
    uint8_t mem[RAM_SIZE];
    uint16_t stack[STACK_SIZE];
    uint8_t flags[NUM_FLAGS];

    uint16_t I;                     // index register (only first 12 bits used)
    uint8_t v[NUM_REGISTERS];       // general purpose registers (0 - 14), 15 = VF
    uint16_t pc;                    // instruction pointer
    uint16_t sp;                    // stack pointer
    opcode current_op;              // current opcode

    uint8_t delay_timer;
    uint8_t sound_timer;

    uint8_t screen[SCREEN_HEIGHT * SCREEN_WIDTH];

    uint8_t keyboard[NUM_KEYS];
    uint8_t wait;
    uint8_t exit;
    uint8_t draw;
    SCREEN_MODE screen_mode;
    uint8_t step_cycles;
    uint8_t debug;


} chip8;


void init_emulator(FILE* rom, chip8* chip8_ctx);

void reset_emulator(chip8* chip8_ctx);

void execute(chip8* chip8_ctx);
