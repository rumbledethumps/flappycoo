#include <fcntl.h>
#include <rp6502.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "game.h"
#include "input.h"
#include "sound.h"
#include "video.h"

#define HISCORE "SAVE:flappycoo.hiscore"

static uint16_t best;
static uint8_t vsync;
static uint8_t events;

// No file, a short file, or no USB drive on a Pico all give a best of 0.
static uint16_t best_load(void)
{
    int fd = open(HISCORE, O_RDONLY);
    if (fd >= 0)
    {
        if (read(fd, &best, sizeof(best)) != sizeof(best))
            best = 0;
        close(fd);
    }
    return best;
}

// Without O_TRUNC, the old best stays in the file when the write fails, so
// only the new best is lost and failures are ignored.
static void best_save(void)
{
    int fd = open(HISCORE, O_WRONLY | O_CREAT);
    if (fd < 0)
        return;
    write(fd, &game.best, sizeof(game.best));
    // close() alone can return before the data is stored.
    syncfs(fd);
    close(fd);
}

int main(void)
{
    game_init(ria_attr_get(RIA_ATTR_LRAND), best_load());
    sound_init();
    input_init();
    video_init();
    vsync = ria_vsync();
    while (true)
    {
        while (ria_vsync() == vsync)
            ;
        vsync = ria_vsync();
        video_draw();
        events = game_update(input_read());
        sound_update(events);
        if (events & EVENT_BEST)
            best_save();
    }
}
