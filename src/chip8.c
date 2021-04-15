#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "chip8.h"
#include "utils.h"


static void fetch(chip8* chip8_ctx);
static void adv(chip8* chip8_ctx, size_t steps);
static void wait_key(chip8* chip8_ctx);
static void draw(chip8* chip8_ctx);


void init_emulator(FILE* input, chip8* chip8_ctx){

    memset(chip8_ctx->mem, 0, RAM_SIZE);
    fread(chip8_ctx->mem + PROGRAM_START, 1, file_size(input), input);

    memset(chip8_ctx->stack, 0, STACK_SIZE);
    memset(chip8_ctx->v, 0, NUM_REGISTERS);
    memset(chip8_ctx->screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);

    chip8_ctx->pc = PROGRAM_START;
    chip8_ctx->sp = 0;
    chip8_ctx->I = 0;

    chip8_ctx->delay_timer = 0;
    chip8_ctx->sound_timer = 0;
    chip8_ctx->exit = 0;
    chip8_ctx->step_cycles = 0;
    chip8_ctx->draw = 1;
    chip8_ctx->wait = 0;

    for (int i = 0; i < NUM_KEYS; i++) {
        chip8_ctx->keyboard[i] = 0;
    }

    for(int i = 0; i < FONT_SET_SIZE; i++) {
        chip8_ctx->mem[i] = FONT_SET[i];
    }

    srandom(time(0));
}


void reset_emulator(chip8* chip8_ctx){
    // reset display
    memset(chip8_ctx->screen, 0 , sizeof(chip8_ctx->screen));
    // clear program memory upto
    memset(chip8_ctx->mem + FONT_SET_SIZE, 0, PROGRAM_START - FONT_SET_SIZE);

    memset(chip8_ctx->v, 0 , NUM_REGISTERS);
    memset(chip8_ctx->keyboard, 0, NUM_KEYS);
    memset(chip8_ctx->stack, 0, STACK_SIZE);

    chip8_ctx->pc = PROGRAM_START;
    chip8_ctx->sp = 0;
    chip8_ctx->I = 0;

    chip8_ctx->delay_timer = 0;
    chip8_ctx->sound_timer = 0;
    chip8_ctx->exit = 0;
    chip8_ctx->step_cycles = 0;
    chip8_ctx->draw = 1;
    chip8_ctx->wait = 0;
}


static void fetch(chip8* chip8_ctx){

    uint16_t opcode = chip8_ctx->mem[chip8_ctx->pc] << 8 | chip8_ctx->mem[chip8_ctx->pc + 1];

    chip8_ctx->current_op.op = (opcode & 0xf000) >> 12;
    chip8_ctx->current_op.x = (opcode & 0x0f00) >> 8;
    chip8_ctx->current_op.y = (opcode & 0x00f0) >> 4;
    chip8_ctx->current_op.addr = opcode & 0xfff;
    chip8_ctx->current_op.kk = opcode & 0xff;
    chip8_ctx->current_op.n = opcode & 0xf;
    chip8_ctx->current_op.full_op = opcode;

    if(!chip8_ctx->debug){
        return;
    }
    printf("OP > %4X | op = %1X |  x = %1X | y = %1X | addr = %3X | nn = %2X | n = %1X \n",
           opcode,
           chip8_ctx->current_op.op,
           chip8_ctx->current_op.x,
           chip8_ctx->current_op.y,
           chip8_ctx->current_op.addr,
           chip8_ctx->current_op.kk,
           chip8_ctx->current_op.n
    );
}

void execute(chip8* chip8_ctx){
    fetch(chip8_ctx);
    opcode op = chip8_ctx->current_op;

    uint8_t v_x = chip8_ctx->v[op.x];
    uint8_t v_y = chip8_ctx->v[op.y];
    uint16_t wide_sum;

    switch (op.op) {
        case 0:
            switch (op.kk) {
                case 0xE0:
                    // clear screen
                    memset(chip8_ctx->screen, 0 , sizeof(chip8_ctx->screen));
                    adv(chip8_ctx, 1);
                    chip8_ctx->draw = 1;
                    break;
                case 0xEE:
                    // return
                    chip8_ctx->pc = chip8_ctx->stack[(--chip8_ctx->sp)] + OP_SIZE;
                    break;
                default:
                    // unknown opcode
                    printf("ERROR > Opcode %X not recognized \n", op.full_op);
                    exit(EXIT_FAILURE);
            }
            break;
        case 1:
            // jump
            chip8_ctx->pc = op.addr;
            break;
        case 2:
            // call
            chip8_ctx->stack[(chip8_ctx->sp++)] = chip8_ctx->pc;
            chip8_ctx->pc = op.addr;
            break;
        case 3:
            // skip next op if Vx = kk
            adv(chip8_ctx, 1 + (v_x == op.kk));
            break;
        case 4:
            // skip next op if Vx != kk
            adv(chip8_ctx, 1 + (v_x != op.kk));
            break;
        case 5:
            // skip next op if Vx = Vy
            adv(chip8_ctx, 1 + (v_x == v_y));
            break;
        case 6:
            // set Vx = kk
            chip8_ctx->v[op.x] = op.kk;
            adv(chip8_ctx, 1);
            break;
        case 7:
            // set Vx = Vx + kk
            chip8_ctx->v[op.x] = v_x + op.kk;
            adv(chip8_ctx, 1);
            break;
        case 8:
            switch (op.n) {
                case 0:
                    // set Vx = Vy
                    chip8_ctx->v[op.x] = v_y;
                    break;
                case 1:
                    // set Vx = Vx OR Vy
                    chip8_ctx->v[op.x] = v_x | v_y;
                    break;
                case 2:
                    // set Vx = Vx AND Vy
                    chip8_ctx->v[op.x] = v_x & v_y;
                    break;
                case 3:
                    // set Vx = Vx XOR Vy
                    chip8_ctx->v[op.x] = v_x ^ v_y;
                    break;
                case 4:
                    // set Vx = Vx + Vy, set VF = carry (if 8-bit overflow)
                    wide_sum = (uint16_t)v_x + (uint16_t)v_y;
                    chip8_ctx->v[VF_IDX] = (wide_sum & 0xff00) > 0;
                    // keep first 8 bits
                    chip8_ctx->v[op.x] = wide_sum & 0x00ff;
                    break;
                case 5:
                    // set Vx = Vx - Vy, set VF = NOT borrow
                    chip8_ctx->v[VF_IDX] = v_x > v_y;
                    chip8_ctx->v[op.x] = v_x - v_y;
                    break;
                case 6:
                    // set Vx = Vx SHR 1
                    chip8_ctx->v[VF_IDX] = v_x & 1;
                    chip8_ctx->v[op.x] = v_x >> 1;
                    break;
                case 7:
                    // set Vx = Vy - Vx, set VF - NOT borrow
                    chip8_ctx->v[VF_IDX] = v_y > v_x;
                    chip8_ctx->v[op.x] = v_y - v_x;
                    break;
                case 0xE:
                    // set Vx = Vx SHL 1
                    chip8_ctx->v[VF_IDX] = v_x >> 7;
                    chip8_ctx->v[op.x] = v_x << 1;
                    break;
                default:
                    // unknown opcode
                    printf("ERROR > Opcode %X not recognized \n", op.full_op);
                    exit(EXIT_FAILURE);
            }
            adv(chip8_ctx, 1);
            break;
        case 9:
            // skip next op if Vx != Vy
            adv(chip8_ctx, 1 + (v_x != v_y));
            break;
        case 0xA:
            // set I == nnn
            chip8_ctx->I = op.addr;
            adv(chip8_ctx, 1);
            break;
        case 0xB:
            // jump to nnn + V0
            chip8_ctx->pc = op.addr + chip8_ctx->v[0];
            break;
        case 0xC:
            // Vx = random byte AND kk
            chip8_ctx->v[op.x] = (uint8_t)random() & op.kk;
            adv(chip8_ctx, 1);
            break;
        case 0xD:
            // display n-byte sprite starting at memory location I at (Vx, Vy),
            // set VF = collision if any pixel is unset
            draw(chip8_ctx);
            break;
        case 0xE:
            switch (op.kk) {
                case 0x9E:
                    // skip next op if key with value Vx is pressed
                    adv(chip8_ctx, 1 + chip8_ctx->keyboard[v_x]);
                    break;
                case 0xA1:
                    // skip next op if key with value Vx is not pressed
                    adv(chip8_ctx, 1 + (!chip8_ctx->keyboard[v_x]));
                    break;
                default:
                    // unknown opcode
                    printf("ERROR > Opcode %X not recognized \n", op.full_op);
                    exit(EXIT_FAILURE);
            }
            break;
        case 0xF:
            switch (op.kk) {
                case 7:
                    // Vx = delay timer value
                    chip8_ctx->v[op.x] = chip8_ctx->delay_timer;
                    adv(chip8_ctx, 1);
                    break;
                case 0x0A:
                    // Wait for keypress and store value of the key in Vx
                    wait_key(chip8_ctx);
                    break;
                case 0x15:
                    // delay timer = Vx
                    chip8_ctx->delay_timer = v_x;
                    adv(chip8_ctx, 1);
                    break;
                case 0x18:
                    // set sound timer = Vx
                    chip8_ctx->sound_timer = v_x;
                    adv(chip8_ctx, 1);
                    break;
                case 0x1E:
                    // set I = I +Vx
                    chip8_ctx->I += v_x;
                    adv(chip8_ctx, 1);
                    break;
                case 0x29:
                    //set I = location of sprite for digit Vx
                    chip8_ctx->I = chip8_ctx->v[op.x] * 5;
                    adv(chip8_ctx, 1);
                    break;
                case 0x33:
                    // store BCD representation of Vx in memory location I, I+1 and I+2
                    chip8_ctx->mem[chip8_ctx->I] = v_x / 100;
                    chip8_ctx->mem[chip8_ctx->I + 1] = (v_x / 10) % 10;
                    chip8_ctx->mem[chip8_ctx->I + 2] = v_x % 10;
                    adv(chip8_ctx, 1);
                    break;
                case 0x55:
                    // store registers V0 through Vx in memory starting at location I
                    memcpy(chip8_ctx->mem + chip8_ctx->I, chip8_ctx->v, op.x + 1);
                    adv(chip8_ctx, 1);
                    break;
                case 0x65:
                    // read registers v0 through Vx from memory at location I
                    memcpy(chip8_ctx->v, chip8_ctx->mem + chip8_ctx->I, op.x + 1);
                    adv(chip8_ctx, 1);
                    break;
                default:
                    // unknown opcode
                    printf("ERROR > Opcode %X not recognized \n", op.full_op);
                    exit(EXIT_FAILURE);

            }
            break;
        default:
            // unknown opcode
            printf("ERROR > Opcode %X not recognized \n", op.full_op);
            exit(EXIT_FAILURE);
    }

}


static void adv(chip8* chip8_ctx, size_t steps){
    chip8_ctx->pc  += (OP_SIZE * steps);
}

static void draw(chip8* chip8_ctx){
    opcode op = chip8_ctx->current_op;
    uint8_t v_x = chip8_ctx->v[op.x];
    uint8_t v_y = chip8_ctx->v[op.y];
    uint16_t y, coord;
    uint8_t pixel, x;
    // wrap starting coordinates
    v_x %= SCREEN_WIDTH;
    v_y %= SCREEN_HEIGHT;
    // clear collision register
    chip8_ctx->v[VF_IDX] = 0;

    for(y = 0; y < op.n; y++){
        pixel = chip8_ctx->mem[chip8_ctx->I + y];
        for(x = 0; x < 8; x++){
            if(pixel & (0x80 >> x)){
                coord = (y + v_y) * 64 + x + v_x;
                chip8_ctx->v[VF_IDX] = (chip8_ctx->v[VF_IDX] || chip8_ctx->screen[coord]) ? 1: 0;
                chip8_ctx->screen[coord] ^= 1;
            }
        }
    }
    chip8_ctx->draw = 1;
    adv(chip8_ctx, 1);
}

static void wait_key(chip8* chip8_ctx){
    uint8_t i;
    for (i = 0; i < NUM_KEYS; i++){
        if(chip8_ctx->keyboard[i]){
            chip8_ctx->v[chip8_ctx->current_op.x] = i;
            adv(chip8_ctx, 1);
            break;
        }
    }
}
