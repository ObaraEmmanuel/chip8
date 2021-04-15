#pragma once

#include <stdint.h>
#include <stdio.h>

#define RAM_SIZE 4096
#define STACK_SIZE 16
#define FONT_SET_SIZE 80
#define OP_SIZE 2

#define NUM_REGISTERS 16
#define VF_IDX 15

#define PROGRAM_START 0x200
#define PROGRAM_END 0xFF

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

#define CPU_CLOCK_DELAY 1000
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
    uint8_t step_cycles;
    uint8_t debug;


} chip8;


void init_emulator(FILE* rom, chip8* chip8_ctx);

void reset_emulator(chip8* chip8_ctx);

void execute(chip8* chip8_ctx);
