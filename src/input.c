#include <rp6502.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"
#include "input.h"
#include "xram.h"

static keyboard_t keyboard;
static uint8_t contact_flags;
static gamepad_player_t pad;

void input_init(void)
{
    xreg_ria_keyboard(XRAM_KEYBOARD);
    xreg_ria_tablet(XRAM_TABLET);
    xreg_ria_gamepad(XRAM_GAMEPAD);
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
    addr = XRAM_TABLET + offsetof(tablet_t, contact) + offsetof(tablet_contact_t, flags);
    contact_flags = 0;
    for (i = 0; i < TABLET_CONTACTS; ++i)
    {
        contact_flags |= xram0_peek8(addr);
        addr += sizeof(tablet_contact_t);
    }
    if (contact_flags & TABLET_FLAG_LEFT)
        buttons |= BUTTON_FLAP;
    if (contact_flags & TABLET_FLAG_RIGHT)
        buttons |= BUTTON_PAUSE;

    addr = XRAM_GAMEPAD;
    for (i = 0; i < GAMEPAD_PLAYERS; ++i)
    {
        xram0_read(&pad, addr, sizeof(pad));
        addr += sizeof(gamepad_player_t);
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
