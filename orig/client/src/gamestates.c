#include "gamestates.h"
#include "splash-screen.h"
#include "name_and_server_entry.h"
#include "connection_failure.h"
#include "lobby.h"
#include "wait_for_opponent.h"
#include "got_challenged.h"
#include "gameplay.h"
#include "you_win.h"

/*! \defgroup gamestates_module The table of states the app can be in, and their attendant function pointers.
 * \{
 */
/*! \brief The actual game-state table. */
GAMESTATE gamestates[NUM_GAMESTATES];

/****************************************************************************************************************/
/*! \brief Configures the gamestate table.  This should only be done once, on startup. */
void GAMESTATE_table_init(void)
{
    gamestates[ GAMESTATE_SPLASH_SCREEN ].tick              =    SPLASH_tick;
    gamestates[ GAMESTATE_SPLASH_SCREEN ].init              =    SPLASH_init;
    gamestates[ GAMESTATE_SPLASH_SCREEN ].draw              =    SPLASH_draw;
    gamestates[ GAMESTATE_SPLASH_SCREEN ].load              =    SPLASH_load;
    gamestates[ GAMESTATE_SPLASH_SCREEN ].unload            =    SPLASH_unload;

    gamestates[ GAMESTATE_CONNECTION_FAILURE ].tick         =    CONNFAIL_tick;
    gamestates[ GAMESTATE_CONNECTION_FAILURE ].init         =    CONNFAIL_init;
    gamestates[ GAMESTATE_CONNECTION_FAILURE ].draw         =    CONNFAIL_draw;
    gamestates[ GAMESTATE_CONNECTION_FAILURE ].load         =    CONNFAIL_load;
    gamestates[ GAMESTATE_CONNECTION_FAILURE ].unload       =    CONNFAIL_unload;

    gamestates[ GAMESTATE_NAME_AND_SERVER_ENTRY ].tick      =    LOGIN_tick;
    gamestates[ GAMESTATE_NAME_AND_SERVER_ENTRY ].init      =    LOGIN_init;
    gamestates[ GAMESTATE_NAME_AND_SERVER_ENTRY ].draw      =    LOGIN_draw;
    gamestates[ GAMESTATE_NAME_AND_SERVER_ENTRY ].load      =    LOGIN_load;
    gamestates[ GAMESTATE_NAME_AND_SERVER_ENTRY ].unload    =    LOGIN_unload;

    gamestates[ GAMESTATE_WAITING_FOR_HANDSHAKE ].tick      =    OPPWAIT_tick;
    gamestates[ GAMESTATE_WAITING_FOR_HANDSHAKE ].init      =    OPPWAIT_init;
    gamestates[ GAMESTATE_WAITING_FOR_HANDSHAKE ].draw      =    OPPWAIT_draw;
    gamestates[ GAMESTATE_WAITING_FOR_HANDSHAKE ].load      =    OPPWAIT_load;
    gamestates[ GAMESTATE_WAITING_FOR_HANDSHAKE ].unload    =    OPPWAIT_unload;

    gamestates[ GAMESTATE_LOBBY ].tick                      =    LOBBY_tick;
    gamestates[ GAMESTATE_LOBBY ].init                      =    LOBBY_init;
    gamestates[ GAMESTATE_LOBBY ].draw                      =    LOBBY_draw;
    gamestates[ GAMESTATE_LOBBY ].load                      =    LOBBY_load;
    gamestates[ GAMESTATE_LOBBY ].unload                    =    LOBBY_unload;

    gamestates[ GAMESTATE_GAMEPLAY ].tick                   =    GAMEPLAY_tick;
    gamestates[ GAMESTATE_GAMEPLAY ].init                   =    GAMEPLAY_init;
    gamestates[ GAMESTATE_GAMEPLAY ].draw                   =    GAMEPLAY_draw;
    gamestates[ GAMESTATE_GAMEPLAY ].load                   =    GAMEPLAY_load;
    gamestates[ GAMESTATE_GAMEPLAY ].unload                 =    GAMEPLAY_unload;

    gamestates[ GAMESTATE_RECEIVED_INVITATION ].tick        =    CHALLRESP_tick;
    gamestates[ GAMESTATE_RECEIVED_INVITATION ].init        =    CHALLRESP_init;
    gamestates[ GAMESTATE_RECEIVED_INVITATION ].draw        =    CHALLRESP_draw;
    gamestates[ GAMESTATE_RECEIVED_INVITATION ].load        =    CHALLRESP_load;
    gamestates[ GAMESTATE_RECEIVED_INVITATION ].unload      =    CHALLRESP_unload;

    gamestates[ GAMESTATE_STAT_SCREEN ].tick                =    YOUWIN_tick;
    gamestates[ GAMESTATE_STAT_SCREEN ].init                =    YOUWIN_init;
    gamestates[ GAMESTATE_STAT_SCREEN ].draw                =    YOUWIN_draw;
    gamestates[ GAMESTATE_STAT_SCREEN ].load                =    YOUWIN_load;
    gamestates[ GAMESTATE_STAT_SCREEN ].unload              =    YOUWIN_unload;


    common_curr_state_done  = FALSE;
    common_curr_state       = GAMESTATE_SPLASH_SCREEN;

    gamestates[common_curr_state].load();
    gamestates[common_curr_state].init();
}

/*! \} */
