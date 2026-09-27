#include <rp6502.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"
#include "input.h"
#include "xram.h"

// The fields of a gamepad_t player that the game reads, in the same order
typedef struct
{
    uint8_t dpad;
    uint8_t sticks;
    uint8_t btn0;
    uint8_t btn1;
} pad_t;

static keyboard_t keyboard;
static uint8_t contact_flags;
static pad_t pad;

void input_init(void)
{
    xreg_ria_keyboard(XRAM_KEYBOARD);
    xreg_ria_tablet(XRAM_TABLET);
    xreg_ria_gamepad(XRAM_GAMEPAD);
    // Mapping the tablet sets control to TABLET_CURSOR_OFF.
    RIA.addr0 = XRAM_TABLET + offsetof(tablet_t, control);
    RIA.rw0 = TABLET_CURSOR_ARROW;
}

uint8_t input_read(void)
{
    uint8_t buttons = 0;
    uint8_t i;
    uint16_t addr;

    xram0_read(&keyboard, XRAM_KEYBOARD, sizeof(keyboard));
    if (KEYBOARD_PRESSED(keyboard.keys, HID_KEY_SPACE) ||
        KEYBOARD_PRESSED(keyboard.keys, HID_KEY_ENTER))
        buttons |= BUTTON_FLAP;
    if (KEYBOARD_PRESSED(keyboard.keys, HID_KEY_P))
        buttons |= BUTTON_PAUSE;

    // A mouse or a pen is contact 0, and each finger on a touchscreen is a
    // separate contact.
    RIA.addr0 = XRAM_TABLET + offsetof(tablet_t, contact[0].flags);
    RIA.step0 = offsetof(tablet_t, contact[1]) - offsetof(tablet_t, contact[0]);
    contact_flags = 0;
    for (i = 0; i < TABLET_CONTACTS; ++i)
        contact_flags |= RIA.rw0;
    // Everything else writes XRAM with a step of 1.
    RIA.step0 = 1;
    if (contact_flags & TABLET_FLAG_LEFT)
        buttons |= BUTTON_FLAP;
    if (contact_flags & TABLET_FLAG_RIGHT)
        buttons |= BUTTON_PAUSE;

    addr = XRAM_GAMEPAD;
    for (i = 0; i < GAMEPAD_PLAYERS; ++i)
    {
        xram0_read(&pad, addr, sizeof(pad));
        addr += offsetof(gamepad_t, player[1]) - offsetof(gamepad_t, player[0]);
        if (!(pad.dpad & GAMEPAD_FEAT_CONNECTED))
            continue;
        if (pad.btn0 & (GAMEPAD_BTN0_A | GAMEPAD_BTN0_B |
                        GAMEPAD_BTN0_X | GAMEPAD_BTN0_Y))
            buttons |= BUTTON_FLAP;
        if (pad.btn1 & GAMEPAD_BTN1_START)
            buttons |= BUTTON_PAUSE;
    }
    return buttons;
}
