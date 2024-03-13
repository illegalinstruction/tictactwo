#include "screenwipe.h"

/*! \brief How long the animation should display for, in game heartbeats (50 per second) */
#define SCREENWIPE_TICKS_TO_COMPLETE    65

#define SCREENWIPE_IN_GFX_PATH          "./gfx/transition/in.tga"
#define SCREENWIPE_OUT_GFX_PATH         "./gfx/transition/out.tga"

/****************************************************************************************************************/
/*! \defgroup screenwipe_priv "Screenwipe module private data"
 * \brief Variables and functions private and/or internal to the screenwipe module.
 * \{
 */

/*! \brief Tracks whether this module was inited yet. */
static BOOL             scrnwipe_module_inited = FALSE;

/*! \brief The texture that gets used when the screenwipe is switching to a new screen */
static TEXTURE_HANDLE   scrnwipe_in_tex;

/*! \brief The texture that gets used when the screenwipe is obscuring an old screen */
static TEXTURE_HANDLE   scrnwipe_out_tex;

/*! \brief Are we transitioning in or out? */
static BOOL scrnwipe_direction;

/*! \brief The animation clock that controls how far along we are in the animation */
static int scrnwipe_anim_clock  = SCREENWIPE_TICKS_TO_COMPLETE;

/*! \brief Discard the textures and clean up after ourselves.  Designed to be called from exit() */
static void SCRNWIPE_unload(void);

/*! \} */

/****************************************************************************************************************/
/*! \brief Load everything and prepare this module for use.
 */
void SCRNWIPE_init(void)
{
    if (scrnwipe_module_inited)
    {
        // we were already called once before, nothing to do here.
        return;
    }

    atexit(SCRNWIPE_unload);

    scrnwipe_anim_clock     = SCREENWIPE_TICKS_TO_COMPLETE;

    scrnwipe_in_tex         = COMMON_load_texture(SCREENWIPE_IN_GFX_PATH);
    scrnwipe_out_tex        = COMMON_load_texture(SCREENWIPE_OUT_GFX_PATH);
    scrnwipe_module_inited  = TRUE;
}

/****************************************************************************************************************/
/*! \brief Discard the textures and clean up after ourselves.  Designed to be called from exit().
 */
void SCRNWIPE_unload(void)
{
    glDeleteTextures(1,&scrnwipe_in_tex   );
    glDeleteTextures(1,&scrnwipe_out_tex  );
}

/****************************************************************************************************************/
/*! \brief Returns whether the screenwipe is currently in the middle of an animation or not.
 */
BOOL SCRNWIPE_check_active(void)
{
    if (scrnwipe_anim_clock < SCREENWIPE_TICKS_TO_COMPLETE)
        return TRUE;
    return FALSE;
}

/****************************************************************************************************************/
/*! \brief Returns TRUE if the screen is wiping in, or FALSE otherwise.
 */
BOOL SCRNWIPE_check_direction(void)
{
    return scrnwipe_direction;
}

/****************************************************************************************************************/
/*! \brief Advance the animation and any attendant logic by one tick.
 */
void SCRNWIPE_tick(void)
{
    if (scrnwipe_anim_clock < SCREENWIPE_TICKS_TO_COMPLETE)
        scrnwipe_anim_clock++;
}

/****************************************************************************************************************/
/*! \brief Start the transition animation.
 * \param direction TRUE for in, FALSE for out
 */
void SCRNWIPE_start(BOOL direction)
{
    scrnwipe_anim_clock = 0;
    scrnwipe_direction  = direction;
}

/****************************************************************************************************************/
/*! \brief Repaint the screen transition.  Should be called <b>LAST</b> of all the drawing functions in the
 * main loop.
 */
void SCRNWIPE_draw(void)
{
    int x_tmp;

    if (common_persp_mode)
        COMMON_to_ortho();

    if (SCRNWIPE_check_active() == FALSE)
    {
        // did we just finish an 'out'?
        if (scrnwipe_direction == FALSE)
        {   glDisable(GL_ALPHA_TEST);
            glColor3ub(0, 0, 0);
            COMMON_draw_sprite(0, 0, 0, 0,common_effective_display_width,DISPLAY_HEIGHT);
            glColor3ub(255,255,255);
        }

        return;
    }

    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);

    if (scrnwipe_direction)
    {
        glAlphaFunc(GL_LESS, (SCREENWIPE_TICKS_TO_COMPLETE - scrnwipe_anim_clock)/(float)SCREENWIPE_TICKS_TO_COMPLETE);
    }
    else
    {
        glAlphaFunc(GL_LESS, scrnwipe_anim_clock/(float)SCREENWIPE_TICKS_TO_COMPLETE);
    }

    for (x_tmp = 0; x_tmp < common_effective_display_width; x_tmp += DISPLAY_HEIGHT)
    {
        if (scrnwipe_direction)
        {
            COMMON_draw_sprite(scrnwipe_in_tex, x_tmp, 0, 0, DISPLAY_HEIGHT, DISPLAY_HEIGHT);
        }
        else
        {
            COMMON_draw_sprite(scrnwipe_out_tex, x_tmp, 0, 0, DISPLAY_HEIGHT, DISPLAY_HEIGHT);
        }
    }

    glDisable(GL_ALPHA_TEST);
    glColor3ub(255,255,255);
}
