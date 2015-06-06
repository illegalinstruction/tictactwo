#include        "splash-screen.h"

/*! \defgroup splash_priv "Module-private functions and data for the splash screen"
 * \{
 */

#define     SPLASH_LOGO_SLIDE_AMT       (11.75f)
#define     SPLASH_LOGO_VPOS            64

#define     SPLASH_LOGO_HALF_WIDTH      300
#define     SPLASH_LOGO_HALF_HEIGHT     150

#define     SPLASH_TEXT_VPOS            480
#define     SPLASH_TEXT_SIZE            36
#define     SPLASH_TEXT_KERN            -6.5
#define     SPLASH_TEXT_ALPHA           (int)((sin(splash_anim_clock / 31.0f) * 127.0f) + 128.0f)

#define     SPLASH_LOGO_TOP_PATH        "./gfx/splash/tictac.tga"
#define     SPLASH_LOGO_BOTTOM_PATH     "./gfx/splash/two.tga"

static TEXTURE_HANDLE splash_logo_tex_top;
static TEXTURE_HANDLE splash_logo_tex_bottom;

static int splash_anim_clock;
/*! \} */

/****************************************************************************************************************/
/*! \brief Set any variables the splash screen uses to sensible starting values.
 */
void SPLASH_load(void)
{
    splash_logo_tex_bottom  = COMMON_load_texture(SPLASH_LOGO_BOTTOM_PATH);
    splash_logo_tex_top     = COMMON_load_texture(SPLASH_LOGO_TOP_PATH);
}

/****************************************************************************************************************/
/*! \brief Set any variables the splash screen uses to sensible starting values.
 */
void SPLASH_init(void)
{
    COMMON_set_bgm(0);
    COMMON_set_bgm(BGM_ID_MAINMENU_LOBBY);
    splash_anim_clock = 0;
    SCRNWIPE_start(TRUE);
}

/****************************************************************************************************************/
/*! \brief Advance all animations and logic by exactly one quantum of time.
 */
void SPLASH_tick(void)
{
    splash_anim_clock++;

    if ((mouse_b & 1) && (SCRNWIPE_check_active() == FALSE))
    {
        // we got clicked - transition to next screen
        SCRNWIPE_start(FALSE);
    }

    // are we at the end of this gamestate?
    if ((SCRNWIPE_check_active() == FALSE) &&
        (SCRNWIPE_check_direction() == FALSE))
    {
        common_next_state       = GAMESTATE_NAME_AND_SERVER_ENTRY;
        common_curr_state_done  = TRUE;
    }
}

/****************************************************************************************************************/
/*! \brief Repaint the splash screen.
 */
void SPLASH_draw(void)
{
    // slide the top and bottom halves of the logo in from the sides
    float top_pos = common_effective_display_width - (splash_anim_clock * SPLASH_LOGO_SLIDE_AMT);
    float bot_pos = (splash_anim_clock * SPLASH_LOGO_SLIDE_AMT);

    if (top_pos < (common_effective_display_width / 2.0f))
        top_pos = (common_effective_display_width / 2.0f);

    if (bot_pos > (common_effective_display_width / 2.0f))
        bot_pos = (common_effective_display_width / 2.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ub(255, 255, 255);
    COMMON_draw_sprite(splash_logo_tex_bottom, bot_pos - (SPLASH_LOGO_HALF_WIDTH / 2.0),
        SPLASH_LOGO_VPOS + (SPLASH_LOGO_HALF_HEIGHT / 3.0),
        0, SPLASH_LOGO_HALF_WIDTH, SPLASH_LOGO_HALF_HEIGHT * 2.0f);

    COMMON_draw_sprite(splash_logo_tex_top, top_pos - (SPLASH_LOGO_HALF_WIDTH / 2.0),
        SPLASH_LOGO_VPOS,
        0, SPLASH_LOGO_HALF_WIDTH, SPLASH_LOGO_HALF_HEIGHT);

    glColor4ub(255,255,255, 255 - (splash_anim_clock % 128) * 2);
    COMMON_glprint(common_gamefont, (common_effective_display_width / 2.0) - ((SPLASH_TEXT_SIZE + SPLASH_TEXT_KERN) * 12.5),
        SPLASH_TEXT_VPOS, 0,
        SPLASH_TEXT_SIZE, SPLASH_TEXT_KERN, "Please click to continue.");
}

/****************************************************************************************************************/
/*! \brief Discard textures and free up memory.
 */
void SPLASH_unload(void)
{
    glDeleteTextures(1, &splash_logo_tex_bottom);
    glDeleteTextures(1, &splash_logo_tex_top);
}
