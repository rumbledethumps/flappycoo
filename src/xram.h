#ifndef XRAM_H
#define XRAM_H

#include <rp6502.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"

#define xreg_vga_canvas(...) xreg(1, 0, 0, __VA_ARGS__)

#define CANVAS_CONSOLE 0
#define CANVAS_320X240 1
#define CANVAS_320X180 2
#define CANVAS_640X480 3
#define CANVAS_640X360 4

#define COLOR_FROM_RGB8(r, g, b) \
    ((((unsigned)(b) >> 3) << 11) | (((unsigned)(g) >> 3) << 6) | ((unsigned)(r) >> 3))
#define COLOR_FROM_RGB5(r, g, b) \
    (((unsigned)(b) << 11) | ((unsigned)(g) << 6) | (unsigned)(r))
#define COLOR_ALPHA_MASK (1u << 5)

#define xreg_vga_mode1(...) xreg(1, 0, 1, 1, __VA_ARGS__)

#define MODE1_1BPP 0x00
#define MODE1_4BPPR 0x01
#define MODE1_4BPP 0x02
#define MODE1_8BPP 0x03
#define MODE1_16BPP 0x04

#define MODE1_8X8 0x00
#define MODE1_8X16 0x08

#define MODE1_FG_BG(fg, bg) ((uint8_t)(((fg) << 4) | (bg)))
#define MODE1_BG_FG(bg, fg) ((uint8_t)(((bg) << 4) | (fg)))

typedef struct
{
    bool x_wrap;
    bool y_wrap;
    int16_t x_pos_px;
    int16_t y_pos_px;
    int16_t width_chars;
    int16_t height_chars;
    uint16_t xram_data_ptr;
    uint16_t xram_palette_ptr;
    uint16_t xram_font_ptr;
} mode1_config_t; /* layout */

typedef struct
{
    uint8_t glyph_code;
} mode1_1bpp_data_t; /* layout */

typedef struct
{
    uint8_t glyph_code;
    uint8_t fg_bg_index;
} mode1_4bppr_data_t; /* layout */

typedef struct
{
    uint8_t glyph_code;
    uint8_t bg_fg_index;
} mode1_4bpp_data_t; /* layout */

typedef struct
{
    uint8_t glyph_code;
    uint8_t fg_index;
    uint8_t bg_index;
} mode1_8bpp_data_t; /* layout */

typedef struct
{
    uint8_t glyph_code;
    uint8_t attributes;
    uint16_t fg_color;
    uint16_t bg_color;
} mode1_16bpp_data_t; /* layout */

#define xreg_vga_mode2(...) xreg(1, 0, 1, 2, __VA_ARGS__)

#define MODE2_1BPP 0x00
#define MODE2_2BPP 0x01
#define MODE2_4BPP 0x02
#define MODE2_8BPP 0x03

#define MODE2_8X8 0x00
#define MODE2_16X16 0x08

#define MODE2_X_TRIM(cols) ((cols) << 4)
#define MODE2_Y_TRIM(rows) ((rows) << 8)

#define MODE2_TILE(bpp, size)                 \
    struct                                    \
    {                                         \
        struct                                \
        {                                     \
            uint8_t cols[(size) * (bpp) / 8]; \
        } rows[size];                         \
    }

typedef struct
{
    bool x_wrap;
    bool y_wrap;
    int16_t x_pos_px;
    int16_t y_pos_px;
    int16_t width_tiles;
    int16_t height_tiles;
    uint16_t xram_data_ptr;
    uint16_t xram_palette_ptr;
    uint16_t xram_tile_ptr;
} mode2_config_t; /* layout */

#define xreg_vga_mode3(...) xreg(1, 0, 1, 3, __VA_ARGS__)

#define MODE3_1BPP 0x00
#define MODE3_2BPP 0x01
#define MODE3_4BPP 0x02
#define MODE3_8BPP 0x03
#define MODE3_16BPP 0x04

#define MODE3_REVERSE_BITS 0x08

typedef struct
{
    bool x_wrap;
    bool y_wrap;
    int16_t x_pos_px;
    int16_t y_pos_px;
    int16_t width_px;
    int16_t height_px;
    uint16_t xram_data_ptr;
    uint16_t xram_palette_ptr;
} mode3_config_t; /* layout */

#define xreg_vga_mode5(...) xreg(1, 0, 1, 5, __VA_ARGS__)

#define MODE5_1BPP 0x00
#define MODE5_2BPP 0x01
#define MODE5_4BPP 0x02
#define MODE5_8BPP 0x03

#define MODE5_8X8 0x00
#define MODE5_16X16 0x08
#define MODE5_32X32 0x10
#define MODE5_64X64 0x18
#define MODE5_128X128 0x20
#define MODE5_256X256 0x28
#define MODE5_512X512 0x30
#define MODE5_CUSTOM 0x38

#define MODE5_HFLIP 0x10
#define MODE5_VFLIP 0x20
#define MODE5_HDOUBLE 0x40
#define MODE5_VDOUBLE 0x80

#define MODE5_IMAGE(bpp, size)                \
    struct                                    \
    {                                         \
        struct                                \
        {                                     \
            uint8_t cols[(size) * (bpp) / 8]; \
        } rows[size];                         \
    }

#define MODE5_SIZE(width, height) \
    ((((height) / 4 - 1) << 4) | ((width) / 4 - 1))

#define MODE5_CUSTOM_IMAGE(bpp, width, height)       \
    struct                                           \
    {                                                \
        struct                                       \
        {                                            \
            uint8_t cols[((width) * (bpp) + 7) / 8]; \
        } rows[height];                              \
    }

typedef struct
{
    int16_t x_pos_px;
    int16_t y_pos_px;
    uint16_t xram_sprite_ptr;
    uint16_t palette_ptr;
} mode5_sprite_t; /* layout */

typedef struct
{
    int16_t x_pos_px;
    int16_t y_pos_px;
    uint16_t xram_sprite_ptr;
    uint16_t palette_ptr;
    uint8_t width_height;
    uint8_t options;
} mode5_csprite_t; /* layout */

#define KEYBOARD_NO_KEY 0
#define KEYBOARD_NUM_LOCK 1
#define KEYBOARD_CAPS_LOCK 2
#define KEYBOARD_SCROLL_LOCK 3

#define KEYBOARD_PRESSED(keys, code) ((keys)[(code) >> 3] & (1 << ((code) & 7)))

#define xreg_ria_keyboard(...) xreg(0, 0, 0, __VA_ARGS__)

typedef struct
{
    uint8_t keys[32];
} keyboard_t; /* layout */

#define TABLET_CONTACTS 8

#define TABLET_STATUS_HOST_CURSOR 0x01

#define TABLET_FLAG_LEFT 0x01
#define TABLET_FLAG_RIGHT 0x02
#define TABLET_FLAG_MIDDLE 0x04
#define TABLET_FLAG_BACKWARD 0x08
#define TABLET_FLAG_FORWARD 0x10
#define TABLET_FLAG_HOVER 0x80

#define TABLET_CURSOR_OFF 0
#define TABLET_CURSOR_ARROW 1
#define TABLET_CURSOR_CROSSHAIR 2
#define TABLET_CURSOR_IBEAM 3
#define TABLET_CURSOR_HAND 4
#define TABLET_CURSOR_RESIZE_EW 5
#define TABLET_CURSOR_RESIZE_NS 6

#define xreg_ria_tablet(...) xreg(0, 0, 3, __VA_ARGS__)

typedef struct
{
    uint8_t flags;
    uint8_t x0, x1, x2;
    uint8_t y0, y1;
} tablet_contact_t;

typedef struct
{
    uint8_t control;
    uint8_t status;
    uint8_t wheel;
    uint8_t pan;
    tablet_contact_t contact[TABLET_CONTACTS];
} tablet_t; /* layout */

#define GAMEPAD_PLAYERS 4

#define GAMEPAD_DPAD_UP 0x01
#define GAMEPAD_DPAD_DOWN 0x02
#define GAMEPAD_DPAD_LEFT 0x04
#define GAMEPAD_DPAD_RIGHT 0x08

#define GAMEPAD_FEAT_TYPE_MASK 0x30
#define GAMEPAD_TYPE_UNKNOWN 0x00
#define GAMEPAD_TYPE_WESTERN 0x10
#define GAMEPAD_TYPE_EASTERN 0x20
#define GAMEPAD_TYPE_PLAYSTATION 0x30
#define GAMEPAD_FEAT_STICKS 0x40
#define GAMEPAD_FEAT_CONNECTED 0x80

#define GAMEPAD_LSTICK_UP 0x01
#define GAMEPAD_LSTICK_DOWN 0x02
#define GAMEPAD_LSTICK_LEFT 0x04
#define GAMEPAD_LSTICK_RIGHT 0x08
#define GAMEPAD_RSTICK_UP 0x10
#define GAMEPAD_RSTICK_DOWN 0x20
#define GAMEPAD_RSTICK_LEFT 0x40
#define GAMEPAD_RSTICK_RIGHT 0x80

#define GAMEPAD_BTN0_A 0x01
#define GAMEPAD_BTN0_B 0x02
#define GAMEPAD_BTN0_C 0x04
#define GAMEPAD_BTN0_X 0x08
#define GAMEPAD_BTN0_Y 0x10
#define GAMEPAD_BTN0_Z 0x20
#define GAMEPAD_BTN0_L1 0x40
#define GAMEPAD_BTN0_R1 0x80

#define GAMEPAD_BTN1_L2 0x01
#define GAMEPAD_BTN1_R2 0x02
#define GAMEPAD_BTN1_SELECT 0x04
#define GAMEPAD_BTN1_START 0x08
#define GAMEPAD_BTN1_HOME 0x10
#define GAMEPAD_BTN1_L3 0x20
#define GAMEPAD_BTN1_R3 0x40

#define xreg_ria_gamepad(...) xreg(0, 0, 2, __VA_ARGS__)

typedef struct
{
    uint8_t dpad;
    uint8_t sticks;
    uint8_t btn0;
    uint8_t btn1;
    int8_t lx;
    int8_t ly;
    int8_t rx;
    int8_t ry;
    uint8_t l2;
    uint8_t r2;
} gamepad_player_t;

typedef struct
{
    gamepad_player_t player[GAMEPAD_PLAYERS];
} gamepad_t; /* layout */

#define PSG_CHANNELS 8

#define PSG_WAVE_SINE 0x00
#define PSG_WAVE_SQUARE 0x10
#define PSG_WAVE_SAWTOOTH 0x20
#define PSG_WAVE_TRIANGLE 0x30
#define PSG_WAVE_NOISE 0x40

#define PSG_GATE 0x01

#define PSG_FREQ_HZ(hz) ((hz) * 3u)
#define PSG_PAN(pan) ((uint8_t)((pan) * 2))

#define xreg_ria_psg(...) xreg(0, 1, 0, __VA_ARGS__)

typedef struct
{
    uint16_t freq;
    uint8_t duty;
    uint8_t attack;
    uint8_t decay;
    uint8_t release_wave;
    uint8_t pan_gate;
    uint8_t reserved;
} psg_channel_t;

typedef struct
{
    psg_channel_t channel[PSG_CHANNELS];
} psg_t; /* layout */

/* After the XRAM_ names: PSG_PAGE_CHECK(XRAM_PSG); */
#define PSG_PAGE_CHECK(addr) \
    _Static_assert((addr) % 256 + sizeof(psg_t) <= 256, #addr " crosses a page.")

// The HID keycodes that the game reads
#define HID_KEY_P 0x13
#define HID_KEY_ENTER 0x28
#define HID_KEY_SPACE 0x2C

// Sizes of the art in img/ and of the text layer
#define LOGO_W 160
#define LOGO_H 82
#define PIPE_BODY_W 32
#define PIPE_BODY_H 64
#define PIPE_CAP_H 12
#define SKY_MAP_W 64
#define SKY_MAP_H 27
#define GROUND_MAP_W 8
#define GROUND_MAP_H 3
#define TEXT_COLS 40
#define TEXT_ROWS 27

// video_draw wraps the scroll positions with a mask, and game.distance wraps
// at 65536.
_Static_assert((SKY_MAP_W & (SKY_MAP_W - 1)) == 0 &&
                   (GROUND_MAP_W & (GROUND_MAP_W - 1)) == 0,
               "The sky and ground map widths must be powers of two.");

typedef MODE2_TILE(4, 8) tile_t;
typedef MODE5_CUSTOM_IMAGE(8, COO_W, COO_H) coo_image_t;

typedef struct
{
    uint16_t coo[256];
    uint16_t sky[16];
    uint16_t ground[16];
    uint16_t pipe[16];
    uint16_t logo[16];
    uint16_t text[2];
} palettes_t;

_Static_assert(sizeof(palettes_t) / sizeof(uint16_t) <= 512,
               "palettes_t holds more than 512 colors.");

typedef struct
{
    psg_t psg;
    palettes_t palettes;
    mode2_config_t sky_config;
    mode3_config_t logo_config;
    mode1_config_t text_config;
    mode2_config_t ground_config;
    mode5_csprite_t pipe_sprites[PIPE_COUNT * 4];
    mode5_csprite_t coo_sprite;
    keyboard_t keyboard;
    tablet_t tablet;
    gamepad_t gamepad;
    mode1_1bpp_data_t text[TEXT_ROWS][TEXT_COLS];
    uint8_t sky_map[SKY_MAP_H][SKY_MAP_W];
    uint8_t ground_map[GROUND_MAP_H][GROUND_MAP_W];
    tile_t sky_tiles[256];
    tile_t ground_tiles[GROUND_MAP_H * GROUND_MAP_W];
    coo_image_t coo[COO_FRAME_COUNT];
    uint8_t logo[LOGO_H][LOGO_W / 2];
    MODE5_CUSTOM_IMAGE(4, PIPE_BODY_W, PIPE_BODY_H) pipe_body;
    MODE5_CUSTOM_IMAGE(4, PIPE_W, PIPE_CAP_H) pipe_cap;
} xram_layout_t;

#define XRAM_PSG offsetof(xram_layout_t, psg)
#define XRAM_COO_PALETTE offsetof(xram_layout_t, palettes.coo)
#define XRAM_SKY_PALETTE offsetof(xram_layout_t, palettes.sky)
#define XRAM_GROUND_PALETTE offsetof(xram_layout_t, palettes.ground)
#define XRAM_PIPE_PALETTE offsetof(xram_layout_t, palettes.pipe)
#define XRAM_LOGO_PALETTE offsetof(xram_layout_t, palettes.logo)
#define XRAM_TEXT_PALETTE offsetof(xram_layout_t, palettes.text)
#define XRAM_SKY_CONFIG offsetof(xram_layout_t, sky_config)
#define XRAM_LOGO_CONFIG offsetof(xram_layout_t, logo_config)
#define XRAM_TEXT_CONFIG offsetof(xram_layout_t, text_config)
#define XRAM_GROUND_CONFIG offsetof(xram_layout_t, ground_config)
#define XRAM_PIPE_SPRITES offsetof(xram_layout_t, pipe_sprites)
#define XRAM_COO_SPRITE offsetof(xram_layout_t, coo_sprite)
#define XRAM_KEYBOARD offsetof(xram_layout_t, keyboard)
#define XRAM_TABLET offsetof(xram_layout_t, tablet)
#define XRAM_GAMEPAD offsetof(xram_layout_t, gamepad)
#define XRAM_TEXT offsetof(xram_layout_t, text)
#define XRAM_SKY_MAP offsetof(xram_layout_t, sky_map)
#define XRAM_GROUND_MAP offsetof(xram_layout_t, ground_map)
#define XRAM_SKY_TILES offsetof(xram_layout_t, sky_tiles)
#define XRAM_GROUND_TILES offsetof(xram_layout_t, ground_tiles)
#define XRAM_COO offsetof(xram_layout_t, coo)
#define XRAM_LOGO offsetof(xram_layout_t, logo)
#define XRAM_PIPE_BODY offsetof(xram_layout_t, pipe_body)
#define XRAM_PIPE_CAP offsetof(xram_layout_t, pipe_cap)

PSG_PAGE_CHECK(XRAM_PSG);

#endif
