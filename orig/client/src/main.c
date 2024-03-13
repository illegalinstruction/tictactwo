#include "tictactwo-common.h"
#include "client-common.h"
#include "screenwipe.h"
#include "background.h"
#include "mouse_effects.h"
#include "particles.h"
#include "gamestates.h"

void main_game_loop(void);

/****************************************************************************************************************/

int main(void)
{
    COMMON_setup(FALSE);

    // -------------------------------------------------
    // load up the various modules that stay resident for the entire application
    BKGRND_load();
    MEFX_load();
    PARTICLE_load();

    // -------------------------------------------------
    // intialize them all
    PARTICLE_init();
    SCRNWIPE_init();
    TEWI_init();

    // -------------------------------------------------
    // prepare the gamestate table and first gamestate we'll run...
    GAMESTATE_table_init();

    while (!key[KEY_ESC])
    {
        if (common_time_for_logic)
        {

            main_game_loop();
            COMMON_flip_buffer();
            common_time_for_logic = 0;
        }
        else
        {
            rest(0);
        }
    }

    // -------------------------------------------------
    SCRNWIPE_start(FALSE);
    COMMON_fade_bgm();

    // run the game loop a few more times until the animation completes,
    // for visual polish reasons
    while(SCRNWIPE_check_active())
    {
        if (common_time_for_logic)
        {

            main_game_loop();
            COMMON_flip_buffer();
            common_time_for_logic = 0;
        }
        else
        {
            rest(0);
        }
    }

    // free up whatever we were doing
    gamestates[common_curr_state].unload();

    // -------------------------------------------------
    // notify the server we're leaving
    // decline any pending invites

    char quitting;

    if (common_curr_state == GAMESTATE_RECEIVED_INVITATION)
    {
        quitting = MSGTYPE_RESPOND_DECLINE;
        COMMON_send(&quitting, 1);
    }

    // send out the quit message and disconnect.
    quitting = MSGTYPE_CLIENT_QUITTING;
    COMMON_send(&quitting, 1);
    COMMON_disconnect();

    // -------------------------------------------------
    // unload resident modules
    MEFX_unload();
    PARTICLE_unload();

    // -------------------------------------------------
    COMMON_shutdown();
}
END_OF_MAIN();

/****************************************************************************************************************/

void main_game_loop(void)
{
    //-----------------------------------------------------------------
    // collect input
    poll_keyboard();
    poll_mouse();
    // fixup mouse coords for widescreen
    mouse_x = (int)(mouse_x * (common_effective_display_width / (float)DISPLAY_WIDTH));

    //-----------------------------------------------------------------
    //logic
    // these are guaranteed to be called; individual gamestates should NOT call them.
    BKGRND_tick();
    SCRNWIPE_tick();
    MEFX_tick();
    TEWI_tick();
    PARTICLE_tick();

    // now the gamestate gets to run
    gamestates[common_curr_state].tick();

    // gamestate just completed? load next one
    if (common_curr_state_done)
    {
        gamestates[common_curr_state].unload();
        gamestates[common_next_state].load();
        gamestates[common_next_state].init();
        gamestates[common_next_state].tick();
        common_curr_state = common_next_state;

        common_curr_state_done = FALSE;
    }

    //-----------------------------------------------------------------
    // painting
    BKGRND_draw();

    // gamestate is guaranteed to get a context in ortho projection with no depth writes
    COMMON_to_ortho();
    glDepthMask(GL_FALSE);

    // draw the gamestate, then everything else gets drawn over it
    gamestates[common_curr_state].draw();

    // these are guaranteed to be drawn in this order after the gamestate finishes drawing.
    TEWI_draw();
    MEFX_draw();
    PARTICLE_draw();
    SCRNWIPE_draw();
}
