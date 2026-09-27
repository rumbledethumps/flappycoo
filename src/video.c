#include <rp6502.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "game.h"
#include "video.h"
#include "xram.h"

#define CANVAS_H 240

#define LOGO_X ((CANVAS_W - LOGO_W) / 2)
#define LOGO_Y 28

#define PIPE_BODY_X ((PIPE_W - PIPE_BODY_W) / 2)

// What the text layer shows
#define SCREEN_TITLE 0
#define SCREEN_PLAY 1
#define SCREEN_PAUSED 2
#define SCREEN_OVER 3
#define SCREEN_AGAIN 4
#define SCREEN_NONE 0xFF

#define TEXT_ROW(row) (XRAM_TEXT + (row) * TEXT_COLS)

static const uint16_t text_palette[2] = {
    0, COLOR_FROM_RGB8(8, 16, 48) | COLOR_ALPHA_MASK};

static const mode2_config_t sky_config = {
    true, false, 0, 0, SKY_MAP_W, SKY_MAP_H,
    XRAM_SKY_MAP, XRAM_SKY_PALETTE, XRAM_SKY_TILES};

static const mode3_config_t logo_config = {
    false, false, LOGO_X, LOGO_Y, LOGO_W, LOGO_H,
    XRAM_LOGO, XRAM_LOGO_PALETTE};

// A font address of 0xFFFF selects the built-in font.
static const mode1_config_t text_config = {
    false, false, 0, 0, TEXT_COLS, TEXT_ROWS,
    XRAM_TEXT, XRAM_TEXT_PALETTE, 0xFFFF};

static const mode2_config_t ground_config = {
    true, false, 0, GROUND_Y, GROUND_MAP_W, GROUND_MAP_H,
    XRAM_GROUND_MAP, XRAM_GROUND_PALETTE, XRAM_GROUND_TILES};

// Each pipe is four sprites: top body, top cap, bottom cap, bottom body.
// Every row of the body image is the same, so the image drawn at double
// height covers the longest pipe with half the XRAM.
static const mode5_csprite_t pipe_sprites[4] = {
    {0, 0, XRAM_PIPE_BODY, XRAM_PIPE_PALETTE,
     MODE5_SIZE(PIPE_BODY_W, PIPE_BODY_H), MODE5_4BPP | MODE5_VDOUBLE},
    {0, 0, XRAM_PIPE_CAP, XRAM_PIPE_PALETTE,
     MODE5_SIZE(PIPE_W, PIPE_CAP_H), MODE5_4BPP},
    {0, 0, XRAM_PIPE_CAP, XRAM_PIPE_PALETTE,
     MODE5_SIZE(PIPE_W, PIPE_CAP_H), MODE5_4BPP},
    {0, 0, XRAM_PIPE_BODY, XRAM_PIPE_PALETTE,
     MODE5_SIZE(PIPE_BODY_W, PIPE_BODY_H), MODE5_4BPP | MODE5_VDOUBLE}};

static const mode5_csprite_t coo_sprite = {
    COO_X, 0, XRAM_COO, XRAM_COO_PALETTE,
    MODE5_SIZE(COO_W, COO_H), MODE5_8BPP};

static const uint16_t tens[] = {10000, 1000, 100, 10};

static uint16_t coo_images[COO_FRAME_COUNT];
static char line[TEXT_COLS];
static char cells[TEXT_COLS];
static uint8_t line_length;
static uint8_t shown_screen = SCREEN_NONE;
static uint16_t shown_score;

static void word_write(uint16_t addr, uint16_t value)
{
    RIA.addr0 = addr;
    RIA.rw0 = value & 0xFF;
    RIA.rw0 = value >> 8;
}

static void sprite_move(uint16_t sprite, int16_t x, int16_t y)
{
    RIA.addr0 = sprite;
    RIA.rw0 = x & 0xFF;
    RIA.rw0 = x >> 8;
    RIA.rw0 = y & 0xFF;
    RIA.rw0 = y >> 8;
}

static void pipes_draw(void)
{
    uint8_t i;
    uint16_t sprite = XRAM_PIPE_SPRITES;
    int16_t x;
    int16_t gap;
    for (i = 0; i < PIPE_COUNT; ++i)
    {
        x = game.pipe_x[i];
        gap = game.pipe_gap[i];
        sprite_move(sprite, x + PIPE_BODY_X,
                    gap - PIPE_CAP_H - PIPE_BODY_H * 2);
        sprite_move(sprite + sizeof(mode5_csprite_t), x, gap - PIPE_CAP_H);
        sprite_move(sprite + sizeof(mode5_csprite_t) * 2, x, gap + PIPE_GAP);
        sprite_move(sprite + sizeof(mode5_csprite_t) * 3, x + PIPE_BODY_X,
                    gap + PIPE_GAP + PIPE_CAP_H);
        sprite += sizeof(pipe_sprites);
    }
}

static void put_text(const char *text)
{
    while (*text)
        line[line_length++] = *text++;
}

// Repeated subtraction is smaller and faster on the 6502 than division.
static void put_number(uint16_t n)
{
    uint8_t i;
    char digit;
    bool leading = true;
    for (i = 0; i < 4; ++i)
    {
        digit = '0';
        while (n >= tens[i])
        {
            n -= tens[i];
            ++digit;
        }
        if (digit != '0' || !leading)
        {
            line[line_length++] = digit;
            leading = false;
        }
    }
    line[line_length++] = '0' + n;
}

static void show_line(uint16_t row_addr)
{
    memset(cells, ' ', TEXT_COLS);
    memcpy(cells + ((TEXT_COLS - line_length) >> 1), line, line_length);
    xram0_write(row_addr, cells, TEXT_COLS);
    line_length = 0;
}

static void score_draw(void)
{
    shown_score = game.score;
    put_number(game.score);
    show_line(TEXT_ROW(1));
}

static uint8_t screen(void)
{
    if (game.state == STATE_TITLE)
        return SCREEN_TITLE;
    if (game.state == STATE_OVER)
        return game.timer < RESTART_DELAY ? SCREEN_OVER : SCREEN_AGAIN;
    return game.paused ? SCREEN_PAUSED : SCREEN_PLAY;
}

// Every row that any screen uses is written whole on each screen change, so
// no row is blank on the canvas between its old text and its new text.
static void text_draw(void)
{
    uint8_t now = screen();
    if (now == shown_screen)
    {
        if (game.score != shown_score)
            score_draw();
        return;
    }
    shown_screen = now;
    if (now == SCREEN_TITLE)
    {
        put_text("BEST ");
        put_number(game.best);
        show_line(TEXT_ROW(1));
    }
    else if (now == SCREEN_PLAY || now == SCREEN_PAUSED)
        score_draw();
    else
        show_line(TEXT_ROW(1));
    if (now >= SCREEN_OVER)
        put_text("GAME OVER");
    show_line(TEXT_ROW(4));
    if (now == SCREEN_PAUSED)
        put_text("PAUSED");
    if (now >= SCREEN_OVER)
    {
        put_text("SCORE ");
        put_number(game.score);
        put_text("   BEST ");
        put_number(game.best);
    }
    show_line(TEXT_ROW(6));
    if (now == SCREEN_AGAIN)
        put_text("SPACE, CLICK OR A TO PLAY");
    show_line(TEXT_ROW(8));
    if (now == SCREEN_TITLE)
        put_text("SPACE, CLICK OR A TO FLAP");
    show_line(TEXT_ROW(18));
}

// The positions are written first, in the blanking time after VSYNC, so no
// layer or sprite moves partway through the drawing of the canvas.
void video_draw(void)
{
    pipes_draw();
    sprite_move(XRAM_COO_SPRITE, COO_X, game.coo_y >> 4);
    word_write(XRAM_COO_SPRITE + offsetof(mode5_csprite_t, xram_sprite_ptr),
               coo_images[game.coo_frame]);
    word_write(XRAM_SKY_CONFIG + offsetof(mode2_config_t, x_pos_px),
               -((game.distance >> 2) & (SKY_MAP_W * 8 - 1)));
    word_write(XRAM_GROUND_CONFIG + offsetof(mode2_config_t, x_pos_px),
               -(game.distance & (GROUND_MAP_W * 8 - 1)));
    word_write(XRAM_LOGO_CONFIG + offsetof(mode3_config_t, x_pos_px),
               game.state == STATE_TITLE ? LOGO_X : CANVAS_W);
    text_draw();
}

void video_init(void)
{
    uint8_t i;
    uint16_t addr = XRAM_COO;
    for (i = 0; i < COO_FRAME_COUNT; ++i)
    {
        coo_images[i] = addr;
        addr += sizeof(coo_image_t);
    }

    // XRAM starts out random, so every byte the modes read is written
    // before the modes are programmed.
    xram0_write(XRAM_TEXT_PALETTE, text_palette, sizeof(text_palette));
    xram0_set(XRAM_TEXT, ' ', TEXT_ROWS * TEXT_COLS);
    xram0_write(XRAM_SKY_CONFIG, &sky_config, sizeof(sky_config));
    xram0_write(XRAM_LOGO_CONFIG, &logo_config, sizeof(logo_config));
    xram0_write(XRAM_TEXT_CONFIG, &text_config, sizeof(text_config));
    xram0_write(XRAM_GROUND_CONFIG, &ground_config, sizeof(ground_config));
    addr = XRAM_PIPE_SPRITES;
    for (i = 0; i < PIPE_COUNT; ++i)
    {
        xram0_write(addr, pipe_sprites, sizeof(pipe_sprites));
        addr += sizeof(pipe_sprites);
    }
    xram0_write(XRAM_COO_SPRITE, &coo_sprite, sizeof(coo_sprite));
    video_draw();

    xreg_vga_canvas(CANVAS_320X240);
    xreg_vga_mode2(MODE2_4BPP | MODE2_8X8, XRAM_SKY_CONFIG, 0, 0, GROUND_Y);
    xreg_vga_mode3(MODE3_4BPP, XRAM_LOGO_CONFIG, 1);
    xreg_vga_mode5(MODE5_CUSTOM, XRAM_PIPE_SPRITES, PIPE_COUNT * 4, 1, 0, GROUND_Y);
    xreg_vga_mode1(MODE1_1BPP | MODE1_8X8, XRAM_TEXT_CONFIG, 2, 0, GROUND_Y);
    xreg_vga_mode2(MODE2_4BPP | MODE2_8X8, XRAM_GROUND_CONFIG, 2, GROUND_Y, CANVAS_H);
    xreg_vga_mode5(MODE5_CUSTOM, XRAM_COO_SPRITE, 1, 2);
}
