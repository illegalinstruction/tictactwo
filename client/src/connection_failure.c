#include   "connection_failure.h"

/*! \brief The path to and filename of the 'connection problem' graphic. */
#define CONNECTION_FAILURE_LOGOTYPE_PATH    "./gfx/connectfail/connection-problem.tga"

#define CONNECTION_FAILURE_LOGO_W           540
#define CONNECTION_FAILURE_LOGO_H           270
#define CONNECTION_FAILURE_LOGO_X           ((common_effective_display_width - CONNECTION_FAILURE_LOGO_W) / 2.0f)
#define CONNECTION_FAILURE_LOGO_Y           96

#define CONNECTION_FAILURE_TEXT_SIZE        29
#define CONNECTION_FAILURE_TEXT_KERN        -5
#define CONNECTION_FAILURE_TEXT_X           \
    (common_effective_display_width - ((CONNECTION_FAILURE_TEXT_SIZE + CONNECTION_FAILURE_TEXT_KERN) * 25.0f)) / 2.0f

#define CONNECTION_FAILURE_TEXT_Y           400
#define CONNECTION_FAILURE_TEXT_ALPHA       (int)((sin(connfail_animclock / 30.1f) * 127.0f) + 128.0f)

/*! \defgroup connection_failure_priv Private data and functions for the Connection Failure screen.
 * \{ */
/*! \brief the 'connection problem'image. */
static TEXTURE_HANDLE connfail_logotype;
/*! \brief flag to check whether the player clicked to get out of this screen. */
static BOOL connfail_was_clicked_yet = FALSE;
static BOOL connfail_stale_mbuttons  = FALSE;

static int connfail_animclock;
/*! \} */

/****************************************************************************************************************/
/*! \brief Load all assets required to display this screen.
 */
void CONNFAIL_load(void)
{
    connfail_logotype = COMMON_load_texture(CONNECTION_FAILURE_LOGOTYPE_PATH);
}

/****************************************************************************************************************/
/*! \brief Unload assets and free up memory.
 */
void CONNFAIL_unload(void)
{
    glDeleteTextures(1, &connfail_logotype);
}

/****************************************************************************************************************/
/*! \brief Reset our internal state to something sensible and prepare for user interaction...
 */
void CONNFAIL_init(void)
{
    SCRNWIPE_start(TRUE);
    connfail_animclock = 0;

    // make sure we don't actually respond to a stale mouse click...
    if (mouse_b)
        connfail_stale_mbuttons = TRUE;
    connfail_was_clicked_yet = FALSE;
}

/****************************************************************************************************************/
/*! \brief Advance any logic or animations this screen contains.  (Because it's a purely informational one, and
 * one the player isn't supposed to see under ideal conditions, it's kinda barebones...)
 */
void CONNFAIL_tick(void)
{
    connfail_animclock++;

    // are we still transitioning in?
    if (SCRNWIPE_check_active() && SCRNWIPE_check_direction())
    {
        // fixes the bug where we would 'see' clicks from during the screenwipe
        if (mouse_b)
            connfail_stale_mbuttons = TRUE;

        return;
    }

    if (!connfail_was_clicked_yet)
    {
        // do we have a valid click from the user?
        if (mouse_b)
        {
            if (connfail_stale_mbuttons == FALSE)
            {
                // yep, transition out.
                connfail_was_clicked_yet = TRUE;
                SCRNWIPE_start(FALSE);
            }
        }
        else
        {
            // mouse buttons are idle now, so no more stale clicks left
            connfail_stale_mbuttons = FALSE;
        }
    }

    if ((SCRNWIPE_check_active() == FALSE) && (SCRNWIPE_check_direction() == FALSE))
    {
        // animation's done.
        common_curr_state_done  = TRUE;
        common_next_state       = GAMESTATE_SPLASH_SCREEN;
    }
}

/****************************************************************************************************************/
/*! \brief Repaints the Connection Problem screen.
 */
void CONNFAIL_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ub(255,255,255);

    glEnable(GL_TEXTURE_2D);

    COMMON_draw_sprite(connfail_logotype, CONNECTION_FAILURE_LOGO_X, CONNECTION_FAILURE_LOGO_Y, 0,
        CONNECTION_FAILURE_LOGO_W, CONNECTION_FAILURE_LOGO_H);

    glColor4ub(255,255,255,CONNECTION_FAILURE_TEXT_ALPHA);

    COMMON_glprint(common_gamefont, CONNECTION_FAILURE_TEXT_X, CONNECTION_FAILURE_TEXT_Y, 0,
        CONNECTION_FAILURE_TEXT_SIZE, CONNECTION_FAILURE_TEXT_KERN, "Please click to continue.");

    glColor3ub(255,255,255);
}
