#include   "you_win.h"

/*! \brief The path to and filename of the 'connection problem' graphic. */
#define YOU_WIN_LOGOTYPE_PATH    "./gfx/result/win.tga"
#define YOU_LOSE_LOGOTYPE_PATH    "./gfx/result/lose.tga"
#define YOU_DRAW_LOGOTYPE_PATH    "./gfx/result/draw.tga"

#define YOU_DRAW_LOGO_SIZE      550
#define YOU_DRAW_X              ((common_effective_display_width - YOU_DRAW_LOGO_SIZE) / 2.0f)
#define YOU_DRAW_Y              25

#define YOU_WIN_LOGO_W           540
#define YOU_WIN_LOGO_H           270
#define YOU_WIN_LOGO_X           ((common_effective_display_width - YOU_WIN_LOGO_W) / 2.0f)
#define YOU_WIN_LOGO_Y           96

#define YOU_WIN_TEXT_SIZE        32
#define YOU_WIN_TEXT_KERN        -6
#define YOU_WIN_TEXT_X           \
    (common_effective_display_width - ((YOU_WIN_TEXT_SIZE + YOU_WIN_TEXT_KERN) * 25.0f)) / 2.0f

#define YOU_WIN_TEXT_Y           400
#define YOU_WIN_TEXT_ALPHA       (int)((sin(youwin_animclock / 30.1f) * 127.0f) + 128.0f)

/*! \defgroup YOU_WIN_priv Private data and functions for the Connection Failure screen.
 * \{ */
/*! \brief the 'connection problem'image. */
static TEXTURE_HANDLE youwin_logotype;
static TEXTURE_HANDLE youlose_logotype;
static TEXTURE_HANDLE youdraw_logotype;
/*! \brief flag to check whether the player clicked to get out of this screen. */
static BOOL youwin_was_clicked_yet = FALSE;
static BOOL youwin_stale_mbuttons  = FALSE;

static int youwin_animclock;
/*! \} */

/****************************************************************************************************************/
/*! \brief Load all assets required to display this screen.
 */
void YOUWIN_load(void)
{
    youwin_logotype     = COMMON_load_texture(YOU_WIN_LOGOTYPE_PATH);
    youlose_logotype    = COMMON_load_texture(YOU_LOSE_LOGOTYPE_PATH);
    youdraw_logotype    = COMMON_load_texture(YOU_DRAW_LOGOTYPE_PATH);
}

/****************************************************************************************************************/
/*! \brief Unload assets and free up memory.
 */
void YOUWIN_unload(void)
{
    glDeleteTextures(1, &youwin_logotype);
    glDeleteTextures(1, &youlose_logotype);
    glDeleteTextures(1, &youdraw_logotype);
}

/****************************************************************************************************************/
/*! \brief Reset our internal state to something sensible and prepare for user interaction...
 */
void YOUWIN_init(void)
{
    SCRNWIPE_start(TRUE);
    youwin_animclock = 0;

    COMMON_set_bgm(BGM_ID_END_OF_ROUND);

    // make sure we don't actually respond to a stale mouse click...
    if (mouse_b)
        youwin_stale_mbuttons = TRUE;
    youwin_was_clicked_yet = FALSE;
}

/****************************************************************************************************************/
/*! \brief Advance any logic or animations this screen contains.  (Because it's a purely informational one, and
 * one the player isn't supposed to see under ideal conditions, it's kinda barebones...)
 */
void YOUWIN_tick(void)
{
    youwin_animclock++;

    // are we still transitioning in?
    if (SCRNWIPE_check_active() && SCRNWIPE_check_direction())
    {
        // fixes the bug where we would 'see' clicks from during the screenwipe
        if (mouse_b)
            youwin_stale_mbuttons = TRUE;

        return;
    }

    if (!youwin_was_clicked_yet)
    {
        // do we have a valid click from the user?
        if (mouse_b)
        {
            if (youwin_stale_mbuttons == FALSE)
            {
                // yep, transition out.
                youwin_was_clicked_yet = TRUE;
                SCRNWIPE_start(FALSE);
            }
        }
        else
        {
            // mouse buttons are idle now, so no more stale clicks left
            youwin_stale_mbuttons = FALSE;
        }
    }

    if ((SCRNWIPE_check_active() == FALSE) && (SCRNWIPE_check_direction() == FALSE))
    {
        // animation's done.
        common_curr_state_done  = TRUE;
        common_next_state       = GAMESTATE_LOBBY;

        // tell the server we're invitable again.
        char tmp = MSGTYPE_DONE_WITH_STAT_SCREEN;
        COMMON_send(&tmp, 1);
    }
}

/****************************************************************************************************************/
/*! \brief Repaints the Connection Problem screen.
 */
void YOUWIN_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ub(255,255,255);

    glEnable(GL_TEXTURE_2D);

    switch(common_next_state_param)
    {
        case 'W':
        {
            COMMON_draw_sprite(youwin_logotype, YOU_WIN_LOGO_X, YOU_WIN_LOGO_Y, 0,
                YOU_WIN_LOGO_W, YOU_WIN_LOGO_H);
        }
        break;

        case 'L':
        {
            COMMON_draw_sprite(youlose_logotype, YOU_WIN_LOGO_X, YOU_WIN_LOGO_Y, 0,
                YOU_WIN_LOGO_W, YOU_WIN_LOGO_H);
        }
        break;

        case 'T':
        {
            COMMON_draw_sprite(youdraw_logotype, YOU_DRAW_X, YOU_DRAW_Y, 0,
                YOU_DRAW_LOGO_SIZE, YOU_DRAW_LOGO_SIZE);
        }
        break;
    }

    glColor4ub(255,255,255,YOU_WIN_TEXT_ALPHA);

    COMMON_glprint(common_gamefont, YOU_WIN_TEXT_X, YOU_WIN_TEXT_Y, 0,
        YOU_WIN_TEXT_SIZE, YOU_WIN_TEXT_KERN, "Please click to continue.");

    glColor3ub(255,255,255);
}
