#include "test.h"
#include "game.h"

#define COO_REST_Y ((GROUND_Y - COO_HIT_Y - COO_HIT_H) * 16)

static uint16_t ref_rng;
static uint8_t probe_events;

// A second copy of the xorshift in game.c, for checking the pipe gaps.
static uint8_t ref_next(void)
{
    ref_rng ^= ref_rng << 7;
    ref_rng ^= ref_rng >> 9;
    ref_rng ^= ref_rng << 8;
    return (uint8_t)ref_rng;
}

// Leaves the game in STATE_PLAY with FLAP still held.
static void start_play(uint16_t seed)
{
    game_init(seed, 0);
    game_update(BUTTON_FLAP);
}

// Holds the coo still for one frame with the box top at y pixels.
static void hold_coo(int16_t y)
{
    game.coo_y = y * 16;
    game.coo_vy = -COO_GRAVITY;
}

// Runs one PLAY frame with the coo at y and pipe 0 at x after the frame.
// Returns true when the frame ends in STATE_FALL. With world_sub at 15, the
// frame moves the pipe at least 1 px even when WORLD_SPEED is under 16.
static bool probe(int16_t y, int16_t x, uint8_t gap)
{
    game.state = STATE_PLAY;
    game.world_sub = 15;
    hold_coo(y);
    game.pipe_x[0] = x + ((WORLD_SPEED + 15) >> 4);
    game.pipe_gap[0] = gap;
    game.pipe_x[1] = 400;
    game.pipe_x[2] = 544;
    probe_events = game_update(0);
    return game.state == STATE_FALL;
}

static uint8_t run_until_over(void)
{
    uint8_t events = 0;
    uint16_t n;
    for (n = 0; n < 1000 && game.state != STATE_OVER; ++n)
        events = game_update(0);
    return events;
}

static bool frame_in(uint8_t first, uint8_t count)
{
    return game.coo_frame >= first && game.coo_frame < first + count;
}

// Returns one bit per frame of a cycle, and 0x8000 for a frame outside it.
static uint16_t cycle_bit(uint8_t first, uint8_t count)
{
    if (!frame_in(first, count))
        return 0x8000;
    return 1u << (game.coo_frame - first);
}

UTEST(game, initial_state_and_title)
{
    uint16_t n;
    int16_t low = COO_START_Y * 16;
    int16_t high = COO_START_Y * 16;
    game_init(1, 123);
    EXPECT_EQ(game.state, STATE_TITLE);
    EXPECT_EQ(game.best, 123u);
    EXPECT_EQ(game.score, 0u);
    EXPECT_EQ(game.coo_y, COO_START_Y * 16);
    EXPECT_EQ(game.pipe_x[2], CANVAS_W + 2 * PIPE_SPACING);
    for (n = 0; n < 1000; ++n)
    {
        ASSERT_EQ(game_update(0), 0);
        if (game.coo_y < low)
            low = game.coo_y;
        if (game.coo_y > high)
            high = game.coo_y;
    }
    EXPECT_EQ(game.state, STATE_TITLE);
    EXPECT_EQ(game.distance, 1000u * WORLD_SPEED / 16);
    EXPECT_EQ(game.pipe_x[0], CANVAS_W);
    EXPECT_TRUE(low < (COO_START_Y - 2) * 16 && low > (COO_START_Y - 10) * 16);
    EXPECT_TRUE(high > (COO_START_Y + 2) * 16 && high < (COO_START_Y + 10) * 16);
}

UTEST(game, flap_starts_play)
{
    int16_t y;
    game_init(1, 0);
    game_update(BUTTON_PAUSE);
    EXPECT_EQ(game.state, STATE_TITLE);
    EXPECT_FALSE(game.paused);
    y = game.coo_y;
    EXPECT_EQ(game_update(BUTTON_FLAP), EVENT_FLAP);
    EXPECT_EQ(game.state, STATE_PLAY);
    EXPECT_EQ(game.coo_vy, COO_FLAP_VY);
    EXPECT_EQ(game.coo_y, y + COO_FLAP_VY);
}

UTEST(play, gravity_and_max_fall_speed)
{
    uint8_t n;
    int16_t y;
    int16_t vy;
    start_play(1);
    game.coo_y = 0;
    game.coo_vy = 0;
    for (n = 0; n < 30; ++n)
    {
        y = game.coo_y;
        vy = game.coo_vy + COO_GRAVITY;
        if (vy > COO_MAX_VY)
            vy = COO_MAX_VY;
        game_update(0);
        ASSERT_EQ(game.coo_vy, vy);
        ASSERT_EQ(game.coo_y, y + vy);
    }
    EXPECT_EQ(game.coo_vy, COO_MAX_VY);
}

UTEST(play, flap_impulse)
{
    start_play(1);
    game_update(0);
    game.coo_y = 100 * 16;
    game.coo_vy = 40;
    EXPECT_EQ(game_update(BUTTON_FLAP), EVENT_FLAP);
    EXPECT_EQ(game.coo_vy, COO_FLAP_VY);
    EXPECT_EQ(game.coo_y, 100 * 16 + COO_FLAP_VY);
}

UTEST(play, held_button_acts_once)
{
    uint8_t n;
    start_play(1);
    for (n = 0; n < 5; ++n)
        EXPECT_EQ(game_update(BUTTON_FLAP), 0);
    game_update(0);
    EXPECT_EQ(game_update(BUTTON_FLAP), EVENT_FLAP);
}

UTEST(play, ceiling_limit)
{
    uint8_t n;
    int16_t low = 0;
    start_play(1);
    for (n = 0; n < 80; ++n)
    {
        game_update(n & 1 ? BUTTON_FLAP : 0);
        if (game.coo_y < low)
            low = game.coo_y;
    }
    EXPECT_EQ(low, -COO_H * 16);
    EXPECT_EQ(game.state, STATE_PLAY);
}

UTEST(play, ground_ends_play)
{
    uint8_t events = 0;
    uint16_t n;
    start_play(1);
    for (n = 0; n < 1000 && game.state == STATE_PLAY; ++n)
        events = game_update(0);
    EXPECT_EQ(game.state, STATE_OVER);
    EXPECT_EQ(events, EVENT_HIT | EVENT_OVER);
    EXPECT_EQ(game.coo_y, COO_REST_Y);
}

UTEST(play, pipe_hit_falls_then_over)
{
    uint8_t events = 0;
    uint8_t n = 0;
    uint16_t distance;
    start_play(1);
    game_update(0);
    game.score = 1;
    EXPECT_TRUE(probe(50, COO_X, 120));
    EXPECT_EQ(probe_events, EVENT_HIT);
    distance = game.distance;
    while (n < 200 && game.state == STATE_FALL)
    {
        events = game_update(++n & 1 ? BUTTON_FLAP | BUTTON_PAUSE : 0);
        ASSERT_FALSE(events & EVENT_FLAP);
        ASSERT_FALSE(game.paused);
        ASSERT_TRUE(game.pipe_x[0] == COO_X && game.distance == distance);
    }
    EXPECT_EQ(events, EVENT_OVER | EVENT_BEST);
    EXPECT_EQ(game.coo_y, COO_REST_Y);
}

UTEST(play, pipe_collision)
{
    start_play(1);
    EXPECT_TRUE(probe(-COO_H, COO_X, PIPE_GAP_MIN));
    EXPECT_FALSE(probe(100, 60, 100 + COO_HIT_Y));
    EXPECT_TRUE(probe(100, 60, 100 + COO_HIT_Y + 1));
    EXPECT_FALSE(probe(100, 60, 100 + COO_HIT_Y + COO_HIT_H - PIPE_GAP));
    EXPECT_TRUE(probe(100, 60, 99 + COO_HIT_Y + COO_HIT_H - PIPE_GAP));
    EXPECT_FALSE(probe(100, COO_X + COO_HIT_X + COO_HIT_W, 0));
    EXPECT_TRUE(probe(100, COO_X + COO_HIT_X + COO_HIT_W - 1, 0));
    EXPECT_FALSE(probe(100, COO_X + COO_HIT_X - PIPE_W, 0));
    EXPECT_TRUE(probe(100, COO_X + COO_HIT_X - PIPE_W + 1, 0));

    // Only the position after the coo moves up 1 px is inside the top pipe.
    game.state = STATE_PLAY;
    game.world_sub = 15;
    game.coo_y = 100 * 16;
    game.coo_vy = -16 - COO_GRAVITY;
    game.pipe_x[0] = 60 + ((WORLD_SPEED + 15) >> 4);
    game.pipe_gap[0] = 100 + COO_HIT_Y;
    game.pipe_x[1] = 400;
    game.pipe_x[2] = 544;
    game_update(0);
    EXPECT_EQ(game.state, STATE_FALL);
}

// Keeps the coo inside every gap while game.distance is below distance, and
// counts EVENT_SCORE.
static uint8_t fly_through(uint16_t distance)
{
    uint8_t scores = 0;
    uint8_t i;
    while (game.distance < distance)
    {
        for (i = 0; i < PIPE_COUNT; ++i)
            game.pipe_gap[i] = 100;
        hold_coo(100);
        if (game_update(0) & EVENT_SCORE)
            ++scores;
        if (game.state != STATE_PLAY)
            return 0;
    }
    return scores;
}

UTEST(play, score_once_per_pipe)
{
    start_play(1);
    EXPECT_EQ(fly_through(CANVAS_W + 2 * PIPE_SPACING), 3);
    EXPECT_EQ(game.score, 3u);
    // The score goes up on the first frame with the whole pipe left of the
    // hitbox.
    probe(100, COO_X + COO_HIT_X - PIPE_W + 1, 100);
    EXPECT_EQ(game.score, 3u);
    probe(100, COO_X + COO_HIT_X - PIPE_W, 100);
    EXPECT_EQ(game.score, 4u);
}

UTEST(play, score_saturates)
{
    start_play(1);
    game.score = 65534u;
    EXPECT_EQ(fly_through(CANVAS_W + 2 * PIPE_SPACING), 3);
    EXPECT_EQ(game.score, 65535u);
}

UTEST(play, pause_freezes_everything)
{
    uint8_t n;
    game_t before;
    start_play(1);
    // In the FLAP cycle the coo frame changes with time, not with vy.
    for (n = 0; n < 10; ++n)
    {
        hold_coo(100);
        game_update(0);
    }
    EXPECT_EQ(game_update(BUTTON_PAUSE), 0);
    EXPECT_TRUE(game.paused);
    before = game;
    for (n = 0; n < 50; ++n)
        EXPECT_EQ(game_update(n & 1 ? BUTTON_FLAP : 0), 0);
    EXPECT_EQ(game.coo_y, before.coo_y);
    EXPECT_EQ(game.coo_frame, before.coo_frame);
    EXPECT_EQ(game.anim_tick, before.anim_tick);
    EXPECT_EQ(game.distance, before.distance);
    EXPECT_EQ(game.pipe_x[0], before.pipe_x[0]);
    game_update(BUTTON_PAUSE);
    game_update(BUTTON_PAUSE);
    EXPECT_FALSE(game.paused);
    EXPECT_NE(game.distance, before.distance);
}

UTEST(over, best_updates)
{
    game_init(1, 7);
    game_update(BUTTON_FLAP);
    game.score = 8;
    EXPECT_EQ(run_until_over(), EVENT_HIT | EVENT_OVER | EVENT_BEST);
    EXPECT_EQ(game.best, 8u);
    game_init(1, 8);
    game_update(BUTTON_FLAP);
    game.score = 8;
    EXPECT_EQ(run_until_over(), EVENT_HIT | EVENT_OVER);
    EXPECT_EQ(game.best, 8u);
}

// Runs to STATE_OVER, then presses FLAP on the given frame in STATE_OVER.
static uint8_t flap_after_over(uint8_t frame)
{
    run_until_over();
    while (--frame)
        game_update(0);
    game_update(BUTTON_FLAP);
    return game.state;
}

UTEST(over, restart_delay)
{
    uint8_t n;
    uint16_t distance;
    start_play(1);
    game.score = 5;
    EXPECT_EQ(flap_after_over(RESTART_DELAY + 1), STATE_PLAY);
    EXPECT_EQ(game.score, 0u);
    EXPECT_EQ(game.coo_y, COO_START_Y * 16 + COO_FLAP_VY);
    EXPECT_GT(game.pipe_x[0], CANVAS_W - 3);
    EXPECT_EQ(flap_after_over(RESTART_DELAY), STATE_OVER);

    distance = game.distance;
    for (n = 0; n < 100; ++n)
        game_update(BUTTON_FLAP);
    EXPECT_EQ(game.state, STATE_OVER);
    EXPECT_EQ(game.distance, distance);
    game_update(BUTTON_PAUSE);
    EXPECT_FALSE(game.paused);
}

UTEST(rng, deterministic)
{
    uint8_t i;
    game_init(1, 0);
    EXPECT_EQ(game.rng, 0xE999u);
    EXPECT_EQ(game.pipe_gap[0], PIPE_GAP_MIN + (0x81 & PIPE_GAP_MASK));
    game_init(0, 0);
    EXPECT_EQ(game.rng, 0xE999u);
    ref_rng = 0x1234 | 1;
    game_init(0x1234, 0);
    for (i = 0; i < PIPE_COUNT; ++i)
        EXPECT_EQ(game.pipe_gap[i], PIPE_GAP_MIN + (ref_next() & PIPE_GAP_MASK));
}

UTEST(pipes, gap_range)
{
    uint16_t seed;
    uint8_t i;
    uint8_t low = 255;
    uint8_t high = 0;
    for (seed = 1; seed < 512; seed += 2)
    {
        game_init(seed, 0);
        for (i = 0; i < PIPE_COUNT; ++i)
        {
            if (game.pipe_gap[i] < low)
                low = game.pipe_gap[i];
            if (game.pipe_gap[i] > high)
                high = game.pipe_gap[i];
        }
    }
    EXPECT_EQ(low, PIPE_GAP_MIN);
    EXPECT_EQ(high, PIPE_GAP_MIN + PIPE_GAP_MASK);
    EXPECT_LT(high + PIPE_GAP, GROUND_Y);
}

UTEST(pipes, recycle_spacing)
{
    uint8_t i;
    uint8_t recycles = 0;
    int16_t last[PIPE_COUNT];
    int16_t d;
    start_play(7);
    ref_rng = 7;
    for (i = 0; i < 2 * PIPE_COUNT; ++i)
        ref_next();
    // The first recycle is at a distance of CANVAS_W + PIPE_W, and the next
    // ones are PIPE_SPACING apart.
    while (game.distance < CANVAS_W + PIPE_W + 4 * PIPE_COUNT * PIPE_SPACING)
    {
        hold_coo(100);
        for (i = 0; i < PIPE_COUNT; ++i)
        {
            last[i] = game.pipe_x[i];
            if (last[i] > 20 && last[i] < 128)
                hold_coo(game.pipe_gap[i]);
        }
        game_update(0);
        ASSERT_EQ(game.state, STATE_PLAY);
        for (i = 0; i < PIPE_COUNT; ++i)
        {
            if (game.pipe_x[i] > last[i])
            {
                ++recycles;
                ASSERT_EQ(game.pipe_gap[i],
                          PIPE_GAP_MIN + (ref_next() & PIPE_GAP_MASK));
            }
            ASSERT_GT(game.pipe_x[i], -PIPE_W);
            d = game.pipe_x[i == PIPE_COUNT - 1 ? 0 : i + 1] - game.pipe_x[i];
            ASSERT_TRUE(d == PIPE_SPACING || d == PIPE_SPACING - PIPE_COUNT * PIPE_SPACING);
        }
    }
    EXPECT_EQ(recycles, 4 * PIPE_COUNT + 1);

    probe(100, 1 - PIPE_W, 100);
    EXPECT_EQ(game.pipe_x[0], 1 - PIPE_W);
    probe(100, -PIPE_W, 100);
    EXPECT_EQ(game.pipe_x[0], PIPE_COUNT * PIPE_SPACING - PIPE_W);
}

UTEST(anim, frames_per_state)
{
    uint8_t n;
    uint16_t seen = 0;
    uint8_t last;
    uint8_t changes;
    game_init(1, 0);
    for (n = 0; n < (COO_FRAME_COUNT - COO_FRAME_IDLE) * COO_ANIM_TICKS * 2; ++n)
    {
        game_update(0);
        seen |= cycle_bit(COO_FRAME_IDLE, COO_FRAME_COUNT - COO_FRAME_IDLE);
    }
    EXPECT_EQ(seen, (1u << (COO_FRAME_COUNT - COO_FRAME_IDLE)) - 1);

    game_update(BUTTON_FLAP);
    EXPECT_EQ(game.coo_frame, COO_FRAME_DIVE - 1);
    last = game.coo_frame;
    for (n = 0; n < 100 && game.coo_vy < 0; ++n)
    {
        ASSERT_TRUE(frame_in(COO_FRAME_RISE, COO_FRAME_DIVE - COO_FRAME_RISE) &&
                    game.coo_frame <= last);
        last = game.coo_frame;
        game_update(0);
    }
    EXPECT_EQ(last, COO_FRAME_RISE);

    seen = 0;
    changes = 0;
    for (n = 0; n < (COO_FRAME_RISE - COO_FRAME_FLAP) * COO_ANIM_TICKS * 2; ++n)
    {
        last = game.coo_frame;
        hold_coo(100);
        game_update(0);
        seen |= cycle_bit(COO_FRAME_FLAP, COO_FRAME_RISE - COO_FRAME_FLAP);
        changes += game.coo_frame != last;
    }
    EXPECT_EQ(seen, (1u << (COO_FRAME_RISE - COO_FRAME_FLAP)) - 1);
    EXPECT_EQ(changes, (COO_FRAME_RISE - COO_FRAME_FLAP) * 2);

    seen = 0;
    for (n = 0; n < (COO_FRAME_IDLE - COO_FRAME_DIVE) * COO_ANIM_TICKS * 2; ++n)
    {
        game.coo_y = 100 * 16;
        game.coo_vy = COO_MAX_VY;
        game_update(0);
        seen |= cycle_bit(COO_FRAME_DIVE, COO_FRAME_IDLE - COO_FRAME_DIVE);
    }
    EXPECT_EQ(seen, (1u << (COO_FRAME_IDLE - COO_FRAME_DIVE)) - 1);

    probe(100, 60, 0);
    for (n = 0; n < 100; ++n)
    {
        ASSERT_TRUE(frame_in(COO_FRAME_DIVE, COO_FRAME_IDLE - COO_FRAME_DIVE));
        game_update(0);
    }
    EXPECT_EQ(game.state, STATE_OVER);
}

// Presses FLAP while the coo falls with the hitbox bottom below the bottom of
// the next gap minus a margin.
static uint16_t bot(uint16_t seed, uint8_t margin)
{
    uint16_t n;
    uint8_t i;
    uint8_t next;
    int16_t bottom;
    start_play(seed);
    for (n = 0; n < 10000 && game.state == STATE_PLAY && game.score < 50; ++n)
    {
        next = PIPE_COUNT;
        for (i = 0; i < PIPE_COUNT; ++i)
            if (game.pipe_x[i] + PIPE_W > COO_X + COO_HIT_X &&
                (next == PIPE_COUNT || game.pipe_x[i] < game.pipe_x[next]))
                next = i;
        bottom = (game.coo_y >> 4) + COO_HIT_Y + COO_HIT_H;
        game_update(game.coo_vy > 0 &&
                            bottom > game.pipe_gap[next] + PIPE_GAP - margin
                        ? BUTTON_FLAP
                        : 0);
    }
    return game.score;
}

UTEST(bot, passes_50_pipes)
{
    static const uint16_t seeds[] = {1, 0x1234, 0x5678, 0xBEEF, 0xFFFF};
    // Margins 6 and 20 are about three frames of fall apart. A pass with both
    // leaves time for a person to react.
    static const uint8_t margins[] = {6, 20};
    uint8_t i;
    uint8_t m;
    for (i = 0; i < sizeof seeds / sizeof seeds[0]; ++i)
        for (m = 0; m < sizeof margins; ++m)
            EXPECT_EQ(bot(seeds[i], margins[m]), 50u);
}
