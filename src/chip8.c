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
static void wide_draw(chip8* chip8_ctx);
static void scroll_left(chip8 *chip8_ctx);
static void scroll_right(chip8 *chip8_ctx);
static void scroll_down(chip8* chip8_ctx, int n);


void init_emulator(FILE* input, chip8* chip8_ctx){

    memset(chip8_ctx->mem, 0, RAM_SIZE);
    fread(chip8_ctx->mem + PROGRAM_START, 1, file_size(input), input);

    memset(chip8_ctx->stack, 0, STACK_SIZE);
    memset(chip8_ctx->v, 0, NUM_REGISTERS);
    memset(chip8_ctx->screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    memset(chip8_ctx->keyboard, 0, NUM_KEYS);
    memset(chip8_ctx->flags, 0, NUM_FLAGS);

    chip8_ctx->pc = PROGRAM_START;
    chip8_ctx->sp = 0;
    chip8_ctx->I = 0;

    chip8_ctx->delay_timer = 0;
    chip8_ctx->sound_timer = 0;
    chip8_ctx->exit = 0;
    chip8_ctx->step_cycles = 0;
    chip8_ctx->draw = 1;
    chip8_ctx->wait = 0;
    chip8_ctx->screen_mode = LOW_RES64;

    // read font sets
    memcpy(chip8_ctx->mem, FONT_SET, FONT_SET_SIZE);
    memcpy(chip8_ctx->mem + FONT_SET_SIZE, SUPER_FONT_SET, SUPER_FONT_SET_SIZE);

    srand(time(0));
}


void reset_emulator(chip8* chip8_ctx){
    // reset display
    memset(chip8_ctx->screen, 0 , sizeof(chip8_ctx->screen));
    // clear program memory upto the font sets
    memset(chip8_ctx->mem + FONT_SET_SIZE + SUPER_FONT_SET_SIZE, 0, PROGRAM_START - (FONT_SET_SIZE + SUPER_FONT_SET_SIZE));

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

    if(chip8_ctx->debug) {
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
}

void execute(chip8* chip8_ctx){
    fetch(chip8_ctx);
    opcode op = chip8_ctx->current_op;

    uint8_t v_x = chip8_ctx->v[op.x];
    uint8_t v_y = chip8_ctx->v[op.y];
    uint16_t wide_sum;

    switch (op.op) {
        case 0:
            switch (op.y) {
                case 0x0:
                    // ignore old SYS opcode
                    adv(chip8_ctx, 1);
                    break;
                case 0xC:
                    // scroll down N lines
                    scroll_down(chip8_ctx, op.n);
                    adv(chip8_ctx, 1);
                    break;
                case 0xE:
                    switch (op.n) {
                        case 0x0:
                            // clear screen
                            memset(chip8_ctx->screen, 0 , sizeof(chip8_ctx->screen));
                            adv(chip8_ctx, 1);
                            chip8_ctx->draw = 1;
                            break;
                        case 0xE:
                            // return
                            chip8_ctx->pc = chip8_ctx->stack[(--chip8_ctx->sp)] + OP_SIZE;
                            break;
                        default:
                            // unknown opcode
                            printf("ERROR > Opcode %X not recognized \n", op.full_op);
                            exit(EXIT_FAILURE);
                    }
                    break;
                case 0xF:
                    switch (op.n) {
                        case 0xB:
                            // scroll right 4 pixels
                            scroll_right(chip8_ctx);
                            adv(chip8_ctx, 1);
                            break;
                        case 0xC:
                            // scroll left 4 pixels
                            scroll_left(chip8_ctx);
                            adv(chip8_ctx, 1);
                            break;
                        case 0xD:
                            // exit interpreter, we will just reset instead
                            reset_emulator(chip8_ctx);
                            break;
                        case 0xE:
                            // disable 128 x 64 screen mode
                            memset(chip8_ctx->screen, 0 , sizeof(chip8_ctx->screen));
                            chip8_ctx->screen_mode = LOW_RES64;
                            chip8_ctx->draw = 1;
                            adv(chip8_ctx, 1);
                            break;
                        case 0xF:
                            // enable 128 x 64 screen mode
                            memset(chip8_ctx->screen, 0 , sizeof(chip8_ctx->screen));
                            chip8_ctx->screen_mode = HIGH_RES128;
                            chip8_ctx->draw = 1;
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
                    chip8_ctx->v[VF_IDX] = v_x >= v_y;
                    chip8_ctx->v[op.x] = v_x - v_y;
                    break;
                case 6:
                    // set Vx = Vx SHR 1
                    chip8_ctx->v[VF_IDX] = v_x & 1;
                    chip8_ctx->v[op.x] = v_x >> 1;
                    break;
                case 7:
                    // set Vx = Vy - Vx, set VF - NOT borrow
                    chip8_ctx->v[VF_IDX] = v_y >= v_x;
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
            chip8_ctx->v[op.x] = (uint8_t)rand() & op.kk;
            adv(chip8_ctx, 1);
            break;
        case 0xD:
            // display n-byte sprite starting at memory location I at (Vx, Vy),
            // set VF = collision if any pixel is unset
            if(op.n == 0 && chip8_ctx->screen_mode == HIGH_RES128){
                // draw a 16 x 16 sprite
                wide_draw(chip8_ctx);
            }else {
                draw(chip8_ctx);
            }
            adv(chip8_ctx, 1);
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
                    // set I = I + Vx
                    chip8_ctx->v[VF_IDX] = !((0xFFFF - chip8_ctx->I) < v_x);
                    chip8_ctx->I += v_x;
                    adv(chip8_ctx, 1);
                    break;
                case 0x29:
                    //set I = location of sprite for digit Vx
                    chip8_ctx->I = chip8_ctx->v[op.x] * 5;
                    adv(chip8_ctx, 1);
                    break;
                case 0x30:
                    // set I = location of 10 byte sprite
                    chip8_ctx->I = FONT_SET_SIZE + chip8_ctx->v[op.x] * 10;
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
                case 0x75:
                    // super chip save to flags
                    memcpy(chip8_ctx->flags, chip8_ctx->v, NUM_FLAGS);
                    adv(chip8_ctx, 1);
                    break;
                case 0x85:
                    // super chip load from flags
                    memcpy(chip8_ctx->v, chip8_ctx->flags, NUM_FLAGS);
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
    uint16_t y, coord_1, coord_2, w_y;
    uint8_t pixel, x, w_x;
    // clear collision register
    chip8_ctx->v[VF_IDX] = 0;
    y = 0;
    if(chip8_ctx->screen_mode == LOW_RES64) {
        v_x *= 2;
        v_y *= 2;
        // wrap starting coordinates
        v_x %= SCREEN_WIDTH;
        v_y %= SCREEN_HEIGHT;
        for (w_y = 0; y < op.n; w_y += 2, y++) {
            pixel = chip8_ctx->mem[chip8_ctx->I + y];
            x = 0;
            for (w_x = 0; w_x < 16; x++, w_x += 2) {
                if (pixel & (0x80 >> x)) {
                    coord_1 = (w_y + v_y) * SCREEN_WIDTH + w_x + v_x;
                    coord_2 = (w_y + 1 + v_y) * SCREEN_WIDTH + w_x + v_x;
                    // use top left pixel as collision representative of the whole 2 x 2 pixel
                    chip8_ctx->v[VF_IDX] = chip8_ctx->v[VF_IDX] || chip8_ctx->screen[coord_1];
                    chip8_ctx->screen[coord_1] ^= 1;
                    chip8_ctx->screen[coord_2] ^= 1;
                    chip8_ctx->screen[coord_1 + 1] ^= 1;
                    chip8_ctx->screen[coord_2 + 1] ^= 1;
                }
            }
        }
    }else{
        // wrap starting coordinates
        v_x %= SCREEN_WIDTH;
        v_y %= SCREEN_HEIGHT;
        // render high resolution 128 x 64
        for(y = 0; y < op.n; y++){
            pixel = chip8_ctx->mem[chip8_ctx->I + y];
            for(x = 0; x < 8; x++){
                if(pixel & (0x80 >> x)){
                    coord_1 = (y + v_y) * SCREEN_WIDTH + x + v_x;
                    chip8_ctx->v[VF_IDX] = chip8_ctx->v[VF_IDX] || chip8_ctx->screen[coord_1];
                    chip8_ctx->screen[coord_1] ^= 1;
                }
            }
        }
    }
    chip8_ctx->draw = 1;
}

static void wide_draw(chip8* chip8_ctx){
    opcode op = chip8_ctx->current_op;
    uint8_t v_x = chip8_ctx->v[op.x];
    uint8_t v_y = chip8_ctx->v[op.y];
    uint16_t y, coord, pixel;
    uint8_t x;
    // wrap starting coordinates
    v_x %= SCREEN_WIDTH;
    v_y %= SCREEN_HEIGHT;
    // clear collision register
    chip8_ctx->v[VF_IDX] = 0;

    for(y = 0; y < 16; y++){
        pixel = (chip8_ctx->mem[chip8_ctx->I + y * 2] << 8) | chip8_ctx->mem[chip8_ctx->I + y * 2 + 1];
        for(x = 0; x < 16; x++){
            if(pixel & (0x8000 >> x)){
                coord = (y + v_y) * SCREEN_WIDTH + x + v_x;
                chip8_ctx->v[VF_IDX] = chip8_ctx->v[VF_IDX] || chip8_ctx->screen[coord];
                chip8_ctx->screen[coord] ^= 1;
            }
        }
    }
    chip8_ctx->draw = 1;
}

static void scroll_left(chip8 *chip8_ctx) {
    for(size_t y = 0; y < SCREEN_HEIGHT; y++){
        for (size_t x = SCROLL_STEP; x < SCREEN_WIDTH; x++){
            chip8_ctx->screen[y * SCREEN_WIDTH + x - SCROLL_STEP] = chip8_ctx->screen[y * SCREEN_WIDTH + x];
            if(x > SCREEN_WIDTH - 1 - SCROLL_STEP) {
                chip8_ctx->screen[y * SCREEN_WIDTH + x] = 0;
            }
        }
    }
}

static void scroll_right(chip8 *chip8_ctx) {
    for(size_t y = 0; y < SCREEN_HEIGHT; y++){
        for (long long x = SCREEN_WIDTH - SCROLL_STEP - 1; x >= 0; x--){
            chip8_ctx->screen[y * SCREEN_WIDTH + x + SCROLL_STEP] = chip8_ctx->screen[y * SCREEN_WIDTH + x];
            if(x < SCROLL_STEP) {
                chip8_ctx->screen[y * SCREEN_WIDTH + x] = 0;
            }
        }
    }
}

static void scroll_down(chip8* chip8_ctx, int n){
    for(long long y = SCREEN_HEIGHT - n - 1; y >= 0; y--){
        memcpy(chip8_ctx->screen + (y + n)*SCREEN_WIDTH, chip8_ctx->screen + (y * SCREEN_WIDTH), SCREEN_WIDTH);
    }
    memset(chip8_ctx->screen, 0, SCREEN_WIDTH * n);
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
