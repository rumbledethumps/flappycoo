#include <rp6502.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"
#include "sound.h"
#include "xram.h"

// One PSG channel for each effect
#define FLAP_CHANNEL (XRAM_PSG + offsetof(psg_t, channel[0]))
#define SCORE_CHANNEL (XRAM_PSG + offsetof(psg_t, channel[1]))
#define HIT_CHANNEL (XRAM_PSG + offsetof(psg_t, channel[2]))
#define MOO_CHANNEL (XRAM_PSG + offsetof(psg_t, channel[3]))

// The flap rises and the moo falls by these Hz each frame.
#define FLAP_RISE 40
#define FLAP_FRAMES 6
#define MOO_FALL 1
#define MOO_FRAMES 45

// The score sound steps up to the second note after SCORE_FRAMES.
#define SCORE_HZ 1319
#define SCORE_FRAMES 5

// A sustain of 0xF_ in decay is silent, so the flap, score and hit
// fade out while the channel gates stay on. The moo sustains until the moo
// gate closes.
static const psg_channel_t flap = {
    PSG_FREQ_HZ(330), 128, 0x20, 0xF4, 0x02 | PSG_WAVE_SQUARE};
static const psg_channel_t score = {
    PSG_FREQ_HZ(988), 255, 0x00, 0xF8, 0x04 | PSG_WAVE_TRIANGLE};
static const psg_channel_t hit = {
    PSG_FREQ_HZ(1000), 255, 0x00, 0xF6, 0x04 | PSG_WAVE_NOISE};
static const psg_channel_t moo = {
    PSG_FREQ_HZ(160), 255, 0x06, 0x29, 0x07 | PSG_WAVE_SAWTOOTH};

static uint16_t flap_freq;
static uint8_t flap_frames;
static uint8_t score_frames;
static uint16_t moo_freq;
static uint8_t moo_frames;

// The gate starts a note only when it changes from 0 to 1.
static void start(uint16_t channel, const psg_channel_t *effect)
{
    xram0_poke8(channel + offsetof(psg_channel_t, pan_gate), 0);
    xram0_write(channel, effect, offsetof(psg_channel_t, pan_gate));
    xram0_poke8(channel + offsetof(psg_channel_t, pan_gate), PSG_GATE);
}

void sound_init(void)
{
    xram0_set(XRAM_PSG, 0, sizeof(psg_t));
    xreg_ria_psg(XRAM_PSG);
}

void sound_update(uint8_t events)
{
    if (events & EVENT_FLAP)
    {
        start(FLAP_CHANNEL, &flap);
        flap_freq = flap.freq;
        flap_frames = FLAP_FRAMES;
    }
    if (events & EVENT_SCORE)
    {
        start(SCORE_CHANNEL, &score);
        score_frames = SCORE_FRAMES;
    }
    if (events & EVENT_HIT)
        start(HIT_CHANNEL, &hit);
    if (events & EVENT_OVER)
    {
        start(MOO_CHANNEL, &moo);
        moo_freq = moo.freq;
        moo_frames = MOO_FRAMES;
    }

    if (flap_frames)
    {
        --flap_frames;
        flap_freq += PSG_FREQ_HZ(FLAP_RISE);
        xram0_poke16(FLAP_CHANNEL, flap_freq);
    }
    if (score_frames && !--score_frames)
        xram0_poke16(SCORE_CHANNEL, PSG_FREQ_HZ(SCORE_HZ));
    if (moo_frames)
    {
        if (--moo_frames)
        {
            moo_freq -= PSG_FREQ_HZ(MOO_FALL);
            xram0_poke16(MOO_CHANNEL, moo_freq);
        }
        else
            xram0_poke8(MOO_CHANNEL + offsetof(psg_channel_t, pan_gate), 0);
    }
}
