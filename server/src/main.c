#include "tictactwo-common.h"
#include "active-player-manager.h"
#include "player_db.h"
#include "gameroom.h"

#define     SAVE_STATS_INTERVAL     120 // every 30 seconds

int main(void)
{
    int save_stats_clock = 0;

    if(!SERVER_init()) return 1;
    PLYRMNGR_init();
    GMRM_init();

    while(TRUE)
    {
        save_stats_clock++;

        if (save_stats_clock >= SAVE_STATS_INTERVAL)
        {
            save_stats_clock = 0;
            PLYRDB_save_to_disk();
        }

        PLYRMNGR_tick();
        GMRM_tick_all();
        usleep(250000);
    }

    return 0;
}
