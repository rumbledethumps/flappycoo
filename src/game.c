#include "game.h"

// Limits of game.coo_y: one box height above the canvas, and the hitbox on
// the ground
#define COO_MIN_Y (-(COO_H << 4))
#define COO_MAX_Y ((GROUND_Y - COO_HIT_Y - COO_HIT_H) << 4)

// Hitbox edges on the canvas
#define COO_LEFT (COO_X + COO_HIT_X)
#define COO_RIGHT (COO_LEFT + COO_HIT_W)

game_t game;

static uint8_t pressed;
static uint8_t events;
static uint8_t step;
static bool hit;

static uint8_t rng_next(void)
{
    game.rng ^= game.rng << 7;
    game.rng ^= game.rng >> 9;
    game.rng ^= game.rng << 8;
    return (uint8_t)game.rng;
}

static void pipes_reset(void)
{
    uint8_t i;
    int16_t x = CANVAS_W;
    for (i = 0; i < PIPE_COUNT; ++i)
    {
        game.pipe_x[i] = x;
        game.pipe_gap[i] = PIPE_GAP_MIN + (rng_next() & PIPE_GAP_MASK);
        x += PIPE_SPACING;
    }
}

static void world_scroll(void)
{
    game.world_sub += WORLD_SPEED;
    step = game.world_sub >> 4;
    game.world_sub &= 15;
    game.distance += step;
}

static void pipes_move(void)
{
    uint8_t i;
    int16_t x;
    int16_t top = (game.coo_y >> 4) + COO_HIT_Y;
    hit = false;
    for (i = 0; i < PIPE_COUNT; ++i)
    {
        x = game.pipe_x[i] - step;
        if (game.pipe_x[i] > COO_LEFT - PIPE_W && x <= COO_LEFT - PIPE_W)
        {
            if (game.score != UINT16_MAX)
                ++game.score;
            events |= EVENT_SCORE;
        }
        if (x <= -PIPE_W)
        {
            x += PIPE_COUNT * PIPE_SPACING;
            game.pipe_gap[i] = PIPE_GAP_MIN + (rng_next() & PIPE_GAP_MASK);
        }
        game.pipe_x[i] = x;
        if (x < COO_RIGHT && x > COO_LEFT - PIPE_W &&
            (top < game.pipe_gap[i] ||
             top + COO_HIT_H > game.pipe_gap[i] + PIPE_GAP))
            hit = true;
    }
}

static void coo_gravity(void)
{
    game.coo_vy += COO_GRAVITY;
    if (game.coo_vy > COO_MAX_VY)
        game.coo_vy = COO_MAX_VY;
}

static bool coo_landed(void)
{
    if (game.coo_y < COO_MAX_Y)
        return false;
    game.coo_y = COO_MAX_Y;
    return true;
}

static void game_over(void)
{
    game.state = STATE_OVER;
    game.timer = 0;
    events |= EVENT_OVER;
    if (game.score > game.best)
    {
        game.best = game.score;
        events |= EVENT_BEST;
    }
}

static void play(void)
{
    if (pressed & BUTTON_FLAP)
    {
        game.coo_vy = COO_FLAP_VY;
        events |= EVENT_FLAP;
    }
    else
        coo_gravity();
    game.coo_y += game.coo_vy;
    if (game.coo_y < COO_MIN_Y)
    {
        game.coo_y = COO_MIN_Y;
        game.coo_vy = 0;
    }
    world_scroll();
    pipes_move();
    if (coo_landed())
    {
        events |= EVENT_HIT;
        game_over();
    }
    else if (hit)
    {
        events |= EVENT_HIT;
        game.state = STATE_FALL;
        game.coo_vy = 0;
    }
}

// The press that starts a game is also the first flap of that game.
static void play_start(void)
{
    game.state = STATE_PLAY;
    game.score = 0;
    pipes_reset();
    play();
}

static void title(void)
{
    if (pressed & BUTTON_FLAP)
        play_start();
    else
    {
        world_scroll();
        game.coo_vy += game.coo_y < (COO_START_Y << 4) ? 1 : -1;
        game.coo_y += game.coo_vy;
    }
}

static void fall(void)
{
    coo_gravity();
    game.coo_y += game.coo_vy;
    if (coo_landed())
        game_over();
}

static void over(void)
{
    if (game.timer < RESTART_DELAY)
        ++game.timer;
    else if (pressed & BUTTON_FLAP)
    {
        game.coo_y = COO_START_Y << 4;
        play_start();
    }
}

static void coo_cycle(uint8_t first, uint8_t count)
{
    if (++game.anim_tick == COO_ANIM_TICKS)
    {
        game.anim_tick = 0;
        ++game.anim;
    }
    if (game.anim >= count)
        game.anim = 0;
    game.coo_frame = first + game.anim;
}

_Static_assert(COO_FRAME_DIVE - COO_FRAME_RISE == 3,
               "There must be three rise frames.");

static void coo_animate(void)
{
    if (game.state == STATE_TITLE)
        coo_cycle(COO_FRAME_IDLE, COO_FRAME_COUNT - COO_FRAME_IDLE);
    else if (game.state != STATE_PLAY || game.coo_vy >= COO_DIVE_VY)
        coo_cycle(COO_FRAME_DIVE, COO_FRAME_IDLE - COO_FRAME_DIVE);
    else if (game.coo_vy >= 0)
        coo_cycle(COO_FRAME_FLAP, COO_FRAME_RISE - COO_FRAME_FLAP);
    else
    {
        // Each tilt frame is shown for a third of the rise after a flap.
        game.coo_frame = COO_FRAME_RISE;
        if (game.coo_vy < COO_FLAP_VY / 3)
            ++game.coo_frame;
        if (game.coo_vy < COO_FLAP_VY * 2 / 3)
            ++game.coo_frame;
    }
}

void game_init(uint16_t seed, uint16_t best)
{
    game.state = STATE_TITLE;
    game.paused = false;
    game.held = 0;
    game.timer = 0;
    game.score = 0;
    game.best = best;
    game.coo_y = COO_START_Y << 4;
    game.coo_vy = COO_BOB_VY;
    game.coo_frame = COO_FRAME_IDLE;
    game.anim = 0;
    game.anim_tick = 0;
    game.world_sub = 0;
    game.distance = 0;
    game.rng = seed | 1;
    pipes_reset();
}

uint8_t game_update(uint8_t buttons)
{
    pressed = buttons & ~game.held;
    game.held = buttons;
    events = 0;
    if (game.state == STATE_PLAY && (pressed & BUTTON_PAUSE))
        game.paused = !game.paused;
    if (game.paused)
        return 0;
    switch (game.state)
    {
    case STATE_TITLE:
        title();
        break;
    case STATE_PLAY:
        play();
        break;
    case STATE_FALL:
        fall();
        break;
    case STATE_OVER:
        over();
        break;
    }
    coo_animate();
    return events;
}
