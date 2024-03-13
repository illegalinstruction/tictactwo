/*! \file wait_for_opponent.c
 * \todo Make this cancellable.
 */
#include "wait_for_opponent.h"
#include "client-common.h"

#define     OPPWAIT_HEADER_W        512
#define     OPPWAIT_HEADER_H        128
#define     OPPWAIT_HEADER_X        ((common_effective_display_width - OPPWAIT_HEADER_W) / 2.0)
#define     OPPWAIT_HEADER_Y        100
#define     OPPWAIT_HEADER_GFX_PATH "./gfx/wait/waiting-for-opponent.tga"

/*! \defgroup oppwait_priv Private functions and data for the 'waiting-for-opponent' screen.
 * \{ */
/*! \brief The header graphic. */
static TEXTURE_HANDLE   oppwait_header_gfx;
static BOOL             oppwait_got_response_yet;
/*! \} */

/****************************************************************************************************************/
/*! \brief Load all assets required for this screen. */
void OPPWAIT_load(void)
{
    oppwait_header_gfx = COMMON_load_texture(OPPWAIT_HEADER_GFX_PATH);
}

/****************************************************************************************************************/
/*! \brief Unload all assets and free memory. */
void OPPWAIT_unload(void)
{
    glDeleteTextures(1, &oppwait_header_gfx);
}

/****************************************************************************************************************/
/*! \brief Doesn't actually do anything on this screen... */
void OPPWAIT_init(void)
{
    SCRNWIPE_start(TRUE);
    oppwait_got_response_yet = FALSE;
}

/****************************************************************************************************************/
/*! \brief Repaint.  Doesn't actually do anything on this screen... */
void OPPWAIT_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    COMMON_draw_sprite(oppwait_header_gfx, OPPWAIT_HEADER_X, OPPWAIT_HEADER_Y, 0, OPPWAIT_HEADER_W, OPPWAIT_HEADER_H);
}

/****************************************************************************************************************/
/*! \brief Waits for the response from the opposing player (via the server). */
void OPPWAIT_tick(void)
{
    char communication_buffer[MAX_MESSAGE_SIZE];

    if (oppwait_got_response_yet == FALSE)
    {
        if (COMMON_recv(communication_buffer, MAX_MESSAGE_SIZE) == FALSE)
        {
            return;
        }
        else
        {
            switch (communication_buffer[0])
            {
                case MSGTYPE_GOT_ACCEPTED:
                {
                    common_next_state = GAMESTATE_GAMEPLAY;
                }
                break;

                case MSGTYPE_GOT_DECLINED:
                {
                    common_next_state = GAMESTATE_LOBBY;
                }
                break;

                default:
                {
                    COMMON_disconnect();
                    common_next_state = GAMESTATE_CONNECTION_FAILURE;
                }
            }

            SCRNWIPE_start(FALSE);
        }

        oppwait_got_response_yet = TRUE;
    }
    else
    {
        // is anim done?
        if ((SCRNWIPE_check_active() == FALSE) || (SCRNWIPE_check_direction() == FALSE))
        {
            common_curr_state_done = TRUE;
        }
    }
}
