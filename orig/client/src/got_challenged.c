/*! \file got_challenged.c
 */
#include "got_challenged.h"
#include "client-common.h"
#include "particles.h"

#define     CHALLRESP_HEADER_W        512
#define     CHALLRESP_HEADER_H        256
#define     CHALLRESP_HEADER_X        ((common_effective_display_width - CHALLRESP_HEADER_W) / 2.0)
#define     CHALLRESP_HEADER_Y        32

#define     CHALLRESP_BUTTON_WIDTH      140
#define     CHALLRESP_BUTTON_HEIGHT     70
#define     CHALLRESP_ACCEPT_X          16
#define     CHALLRESP_DECLINE_X         (common_effective_display_width - CHALLRESP_BUTTON_WIDTH - 16)

#define     CHALLRESP_BUTTON_Y          320

#define     CHALLRESP_HEADER_GFX_PATH       "./gfx/challenge/challenged.tga"
#define     CHALLRESP_ACCEPT_GFX_PATH       "./gfx/challenge/accept.tga"
#define     CHALLRESP_DECLINE_GFX_PATH      "./gfx/challenge/decline.tga"

/*! \defgroup challresp_priv Private functions and data for the 'you-have-been-challenged' screen.
 * \{ */
/*! \brief The header graphic. */
static TEXTURE_HANDLE   challresp_header_gfx;

static TEXTURE_HANDLE   challresp_accept_gfx;
static TEXTURE_HANDLE   challresp_decline_gfx;

/*! \} */

/****************************************************************************************************************/
/*! \brief Load all assets required for this screen. */
void CHALLRESP_load(void)
{
    challresp_header_gfx = COMMON_load_texture(CHALLRESP_HEADER_GFX_PATH);

    challresp_accept_gfx    = COMMON_load_texture(CHALLRESP_ACCEPT_GFX_PATH);
    challresp_decline_gfx   = COMMON_load_texture(CHALLRESP_DECLINE_GFX_PATH);
}

/****************************************************************************************************************/
/*! \brief Unload all assets and free memory. */
void CHALLRESP_unload(void)
{
    glDeleteTextures(1, &challresp_header_gfx);
    glDeleteTextures(1, &challresp_accept_gfx);
    glDeleteTextures(1, &challresp_decline_gfx);
}

/****************************************************************************************************************/
/*! \brief Doesn't actually do anything on this screen... */
void CHALLRESP_init(void)
{
    SCRNWIPE_start(TRUE);
}

/****************************************************************************************************************/
/*! \brief Repaint.  Doesn't actually do anything on this screen... */
void CHALLRESP_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // header
    COMMON_draw_sprite(challresp_header_gfx, CHALLRESP_HEADER_X, CHALLRESP_HEADER_Y, 0,
        CHALLRESP_HEADER_W, CHALLRESP_HEADER_H);

    // buttons
    COMMON_draw_sprite(challresp_accept_gfx, CHALLRESP_ACCEPT_X, CHALLRESP_BUTTON_Y, 0,
        CHALLRESP_BUTTON_WIDTH, CHALLRESP_BUTTON_HEIGHT);
    COMMON_draw_sprite(challresp_decline_gfx, CHALLRESP_DECLINE_X, CHALLRESP_BUTTON_Y, 0,
        CHALLRESP_BUTTON_WIDTH, CHALLRESP_BUTTON_HEIGHT);
}

/****************************************************************************************************************/
/*! \brief Waits for the response from the opposing player (via the server). */
void CHALLRESP_tick(void)
{
    char communication_buffer[MAX_MESSAGE_SIZE];
    static BOOL mouse_held;

    if (SCRNWIPE_check_active())
        return;

    // is transition out anim done?
    if ((SCRNWIPE_check_active() == FALSE) && (SCRNWIPE_check_direction() == FALSE))
    {
        common_curr_state_done = TRUE;
        return;
    }

    if (mouse_b & 1)
    {
        if (!mouse_held)
        {
            if ((mouse_y > CHALLRESP_BUTTON_Y) && (mouse_y < (CHALLRESP_BUTTON_Y + CHALLRESP_BUTTON_HEIGHT)))
            {
                if ((mouse_x > CHALLRESP_ACCEPT_X) && (mouse_x < (CHALLRESP_ACCEPT_X + CHALLRESP_BUTTON_WIDTH)))
                {
                    // inside accept button
                    int i;
                    for (i = 0; i < 10; i++)
                    {
                        PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 1.25f, (rand() % 20 - 10) / 1.25f,
                            PARTICLE_SPARK, (rand() % 32) + 192, rand() % NUM_PARTICLE_COLOURS);
                        PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 2.0f, (rand() % 20 - 10) / 2.0f,
                            PARTICLE_HEART, 255,  rand() % NUM_PARTICLE_COLOURS);
                    }

                    communication_buffer[0] = MSGTYPE_RESPOND_ACCEPT;
                    COMMON_send(communication_buffer, 1);

                    common_next_state = GAMESTATE_GAMEPLAY;

                    SCRNWIPE_start(FALSE);
                }

                if ((mouse_x > CHALLRESP_DECLINE_X) && (mouse_x < (CHALLRESP_DECLINE_X + CHALLRESP_BUTTON_WIDTH)))
                {
                    // inside decline button
                    int i;
                    for (i = 0; i < 20; i++)
                    {
                        // fixme - magic number for particle type
                        PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 1.25f, (rand() % 20 - 10) / 1.25f,
                            PARTICLE_SPARK, (rand() % 32) + 192, 0);
                    }

                    communication_buffer[0] = MSGTYPE_RESPOND_DECLINE;
                    COMMON_send(communication_buffer, 1);

                    common_next_state = GAMESTATE_LOBBY;

                    SCRNWIPE_start(FALSE);
                }
            }
        }

        mouse_held = TRUE;
    }
    else
    {
        mouse_held = FALSE;
    }

    if ((SCRNWIPE_check_active() == FALSE) && (SCRNWIPE_check_direction() == FALSE))
    {
        // animation's done.
        common_curr_state_done  = TRUE;
    }
}
