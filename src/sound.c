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

#define PAN_GATE offsetof(psg_t, channel[0].pan_gate)

// The flap rises and the moo falls by these Hz each frame.
#define FLAP_RISE 40
#define FLAP_FRAMES 6
#define MOO_FALL 1
#define MOO_FRAMES 45

// The score steps up to its second note after SCORE_FRAMES.
#define SCORE_HZ 1319
#define SCORE_FRAMES 5

// The fields of a PSG channel up to pan_gate, in the same order
typedef struct
{
    uint16_t freq;
    uint8_t duty;
    uint8_t vol_attack;
    uint8_t vol_decay;
    uint8_t wave_release;
} effect_t;

// A sustain of 0xF_ in vol_decay is silent, so the flap, score and hit fade
// out while their gates stay on. The moo sustains until its gate closes.
static const effect_t flap = {
    PSG_FREQ_HZ(330), 128, 0x20, 0xF4, PSG_WAVE_SQUARE | 0x02};
static const effect_t score = {
    PSG_FREQ_HZ(988), 255, 0x00, 0xF8, PSG_WAVE_TRIANGLE | 0x04};
static const effect_t hit = {
    PSG_FREQ_HZ(1000), 255, 0x00, 0xF6, PSG_WAVE_NOISE | 0x04};
static const effect_t moo = {
    PSG_FREQ_HZ(160), 255, 0x06, 0x29, PSG_WAVE_SAWTOOTH | 0x07};

static uint16_t flap_freq;
static uint8_t flap_frames;
static uint8_t score_frames;
static uint16_t moo_freq;
static uint8_t moo_frames;

static void gate_write(uint16_t channel, uint8_t gate)
{
    RIA.addr0 = channel + PAN_GATE;
    RIA.rw0 = gate;
}

static void freq_write(uint16_t channel, uint16_t freq)
{
    RIA.addr0 = channel;
    RIA.rw0 = freq & 0xFF;
    RIA.rw0 = freq >> 8;
}

// The gate starts a note only when it changes from 0 to 1.
static void start(uint16_t channel, const effect_t *effect)
{
    gate_write(channel, 0);
    xram0_write(channel, effect, sizeof(effect_t));
    gate_write(channel, PSG_GATE);
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
        freq_write(FLAP_CHANNEL, flap_freq);
    }
    if (score_frames && !--score_frames)
        freq_write(SCORE_CHANNEL, PSG_FREQ_HZ(SCORE_HZ));
    if (moo_frames)
    {
        if (--moo_frames)
        {
            moo_freq -= PSG_FREQ_HZ(MOO_FALL);
            freq_write(MOO_CHANNEL, moo_freq);
        }
        else
            gate_write(MOO_CHANNEL, 0);
    }
}
