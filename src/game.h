#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

// Buttons held this frame, for game_update
#define BUTTON_FLAP 0x01
#define BUTTON_PAUSE 0x02
#define BUTTON_QUIT 0x04

// Events returned by game_update
#define EVENT_FLAP 0x01
#define EVENT_SCORE 0x02
#define EVENT_HIT 0x04
#define EVENT_OVER 0x08
#define EVENT_BEST 0x10
#define EVENT_QUIT 0x20

// Values of game.state
#define STATE_TITLE 0
#define STATE_PLAY 1
#define STATE_FALL 2
#define STATE_OVER 3

// Canvas in pixels
#define CANVAS_W 320
#define GROUND_Y 216

// Coo sprite box and hitbox in pixels
#define COO_X 64
#define COO_W 52
#define COO_H 36
#define COO_HIT_X 16
#define COO_HIT_Y 8
#define COO_HIT_W 28
#define COO_HIT_H 22
#define COO_START_Y 100

// Coo motion in 1/16 pixel per frame
#define COO_GRAVITY 4
#define COO_FLAP_VY (-72)
#define COO_MAX_VY 96
#define COO_DIVE_VY 48
#define COO_BOB_VY 12

// Coo animation frames, in the order of img/coo.png
#define COO_FRAME_FLAP 0
#define COO_FRAME_RISE 7
#define COO_FRAME_DIVE 10
#define COO_FRAME_IDLE 14
#define COO_FRAME_COUNT 18
#define COO_ANIM_TICKS 4

// World speed in 1/16 pixel per frame, pipes in pixels
#define WORLD_SPEED 24
#define PIPE_COUNT 3
#define PIPE_SPACING 144
#define PIPE_W 36
#define PIPE_GAP 88
#define PIPE_GAP_MIN 40
#define PIPE_GAP_MASK 63

// Frames in STATE_OVER before FLAP starts a new game
#define RESTART_DELAY 40

typedef struct
{
    uint8_t state;
    bool paused;
    uint8_t held;     // buttons held last frame
    uint8_t timer;    // frames in STATE_OVER
    uint16_t score;
    uint16_t best;
    int16_t coo_y;    // top of the sprite box in 1/16 pixel
    int16_t coo_vy;   // 1/16 pixel per frame, negative is up
    uint8_t coo_frame;
    uint8_t anim;     // frame within the current cycle
    uint8_t anim_tick;
    uint8_t world_sub;  // 1/16 pixel
    uint16_t distance;  // pixels, wraps
    int16_t pipe_x[PIPE_COUNT];   // left edge of the cap
    uint8_t pipe_gap[PIPE_COUNT]; // top of the gap
    uint16_t rng;
} game_t;

extern game_t game;

void game_init(uint16_t seed, uint16_t best);
uint8_t game_update(uint8_t buttons);

#endif
