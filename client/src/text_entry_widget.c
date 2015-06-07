/*! \file text_entry_widget.c
 * \todo Add a clickable 'OK' button
 * \todo Hiragana/katakana modes (where the user spells them out phonetically) would be nice (requires better font handling)
 * \todo Maybe make it instantiable? There will be times we need more than one active...
 * \todo Port to Allegro 5 (may take a while)
 * \todo The buttons should be abstracted away into a module of their own; there's a lot of goofy repeated logic.
 */
#include "client-common.h"
#include "text_entry_widget.h"
#include "mouse_effects.h"
#include <string.h>

#define TEXT_ENTRY_WIDGET_CURSOR_BLINK          (128 - (int)(sin(tewi_animclock/9.25f) * 127.0f))

#define TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE      24
#define TEXT_ENTRY_WIDGET_WIDGET_BG             64,  64, 64, 128
#define TEXT_ENTRY_WIDGET_YSTART                32
#define TEXT_ENTRY_WIDGET_SPRITE_SIZE           40

#define TEXT_ENTRY_BLIP_PATH                    "./snd/text_widget/type.wav"
#define TEXT_ENTRY_ERASE_PATH                   "./snd/text_widget/delete.wav"
#define TEXT_ENTRY_ACCEPT_PATH                  "./snd/text_widget/accept.wav"

/*! \defgroup text_entry_module_private Private data and functions for the text entry field widget.
 * \{  */
/*! \brief Buffer to hold the being-typed-in string. */
static char         tewi_priv_string[TEXT_ENTRY_WIDGET_STRING_LENGTH + 1];
/*! \brief Controls the blinking of the cursor (and any other future animations). */
static uint32_t     tewi_animclock;
/*! \brief Used to automatically centre the widget on the display. */
static float        tewi_horz_centre;
/*! \brief Whether the widget is active or not. */
static BOOL         tewi_active;
/*! \brief What input mode we're in. */
static uint8_t      tewi_mode;
/*! \brief Where the cursor currently is. */
static unsigned int tewi_char_index = 0;
/*! \brief A keystroke blip noise. */
static SAMPLE       *tewi_keybeep = NULL;
/*! \brief An erasing noise. */
static SAMPLE       *tewi_delbeep = NULL;
/*! \brief A confirmation/accept noise. */
static SAMPLE       *tewi_retbeep = NULL;

static BOOL         tewi_cleanup_registered = FALSE;
/*! \brief Unloads any assets we had loaded. Designed to be called once, automatically from exit() */
static void         TEWI_cleanup(void);

static float        tewi_ystart = TEXT_ENTRY_WIDGET_YSTART;
/*! \} */

/****************************************************************************************************************/
/*! \brief Set up and configure everything needed to use the widget.  Should only be called <b>once</b>.       */
void TEWI_init(void)
{
    if (tewi_cleanup_registered)
        return;

    atexit(TEWI_cleanup);

    bzero(tewi_priv_string, sizeof(tewi_priv_string));
    tewi_char_index = 0;
    tewi_animclock = 0;
    tewi_horz_centre = common_effective_display_width / 2.0f;
    tewi_active = FALSE;
    tewi_mode = TEXT_ENTRY_WIDGET_MODE_NORMAL;

    if (exists(TEXT_ENTRY_BLIP_PATH))
        tewi_keybeep = load_sample(TEXT_ENTRY_BLIP_PATH);
    else
        tewi_keybeep = create_sample(8, 0, 1000, 1);

    if (exists(TEXT_ENTRY_ERASE_PATH))
        tewi_delbeep = load_sample(TEXT_ENTRY_ERASE_PATH);
    else
        tewi_delbeep = create_sample(8, 0, 1000, 1);

    if (exists(TEXT_ENTRY_ACCEPT_PATH))
        tewi_retbeep = load_sample(TEXT_ENTRY_ACCEPT_PATH);
    else
        tewi_retbeep = create_sample(8, 0, 1000, 1);
}

/****************************************************************************************************************/
/*! \brief Frees up the sounds we loaded in the init function.
 */
void TEWI_cleanup(void)
{
    destroy_sample(tewi_keybeep);
    destroy_sample(tewi_delbeep);
    destroy_sample(tewi_retbeep);
}

/****************************************************************************************************************/
/*! \brief Turn on or off the widget.
 * \param should_be_enabled Pass TRUE for turn it on, or FALSE to turn it off.
 */
void TEWI_enable_or_disable(BOOL should_be_enabled)
{
    tewi_active = should_be_enabled;
    clear_keybuf();
}

/****************************************************************************************************************/
/*! @brief Repaints all the elements in the widget.
 * \note Will malfunction HORRIBLY if used in perspective mode, does not attempt to track or reset OpenGL
 *  state.
 */
void TEWI_draw(void)
{
    if (!tewi_active)
        return;

    int x_index, y_index;

    float x_pos = tewi_horz_centre - (TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE * (TEXT_ENTRY_WIDGET_STRING_LENGTH + 1) / 2.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // draw the text field
    // greyish box in the background
    glColor4ub(TEXT_ENTRY_WIDGET_WIDGET_BG);
    COMMON_draw_sprite(0,
        x_pos,                            // x
        tewi_ystart,   // y
        0,                                                                                      // z
        TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE * TEXT_ENTRY_WIDGET_STRING_LENGTH,
        TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE);

    // the actual text
    glColor4ub(255, 255, 255, 255);
    COMMON_glprint(common_gamefont,
        x_pos,
        tewi_ystart,
        0,
        TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE,
        0,
        "%s",tewi_priv_string);

    // a visual cursor
    if (tewi_char_index < TEXT_ENTRY_WIDGET_STRING_LENGTH)
    {
        glColor4ub(255, 255, 255, TEXT_ENTRY_WIDGET_CURSOR_BLINK);
        COMMON_draw_sprite(0, x_pos + ((tewi_char_index) * TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE),
            tewi_ystart,
            0,
            TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE, TEXT_ENTRY_WIDGET_WIDGET_TEXT_SIZE);
        glColor4ub(255, 255, 255, 255);
    }
}

/****************************************************************************************************************/
/*! \brief Returns whether the widget is currently being drawn and is ready to receive keystrokes. */
BOOL TEWI_check_active()
{
    return tewi_active;
}

/****************************************************************************************************************/
/*! \brief Respond to user input, animate the cursor, etc.
 */
void TEWI_tick(void)
{
    if (!tewi_active)
        return;

    // handle a typed key
    if (keypressed())
    {
        int key_tmp    =   readkey();
        uint8_t char_tmp = key_tmp & 0xFF;

        switch (tewi_mode)
        {
            // ----------------------------------------

            case TEXT_ENTRY_WIDGET_MODE_NORMAL:
            {
                if ((char_tmp >= ' ') && (char_tmp <= '~'))
                {
                    if (tewi_char_index < TEXT_ENTRY_WIDGET_STRING_LENGTH)
                    {
                        tewi_priv_string[tewi_char_index] = char_tmp;
                        tewi_char_index++;
                        play_sample(tewi_keybeep, 255, 128, 1000, 0);
                    }
                }
            }
            break;

            // ----------------------------------------

            case TEXT_ENTRY_WIDGET_MODE_NUMERIC:
            {
                if (((char_tmp >= '0') && (char_tmp <= '9')) || (char_tmp == '.'))
                {
                    if (tewi_char_index < TEXT_ENTRY_WIDGET_STRING_LENGTH)
                    {
                        tewi_priv_string[tewi_char_index] = char_tmp;
                        tewi_char_index++;
                        play_sample(tewi_keybeep, 255, 128, 1000, 0);
                    }
                }
            }
            break;

            // ----------------------------------------

            case TEXT_ENTRY_WIDGET_MODE_NAME:
            {
                if ((char_tmp >= '!') && (char_tmp <= '~'))
                {
                    if (tewi_char_index < TEXT_ENTRY_WIDGET_STRING_LENGTH)
                    {
                        tewi_priv_string[tewi_char_index] = char_tmp;
                        tewi_char_index++;
                        play_sample(tewi_keybeep, 255, 128, 1000, 0);
                    }
                }
            }
            break;
        } // end if the user typed a normal key

        // backspace or delete?
        if ((char_tmp == 8) || (char_tmp == 127))
        {
            tewi_priv_string[tewi_char_index] = 0;

            if (tewi_char_index > 0)
                tewi_char_index--;

            play_sample(tewi_delbeep, 255, 128, 1000, 0);
        }

        // enter?
        if (((char_tmp == 10) || (char_tmp == 13)) && (strlen(tewi_priv_string) > 0))                           //bugfix - don't accept unless there's at least one character. (TODO: make this configurable)
        {
            // no more text may be entered, and the client code may
            // retrieve the string.
            tewi_active = FALSE;
            play_sample(tewi_retbeep, 255, 128, 1000, 0);
        }
    }

    tewi_animclock++;
}

/****************************************************************************************************************/
/*! @brief Gets the string that was entered by the player and returns it (as a char *).
 *  @return A pointer to the string.
 *  @note The returned string must <b>never</b> be free()d or bad things will happen.
 */
char *TEWI_get_string(void)
{
    return tewi_priv_string;
}

/****************************************************************************************************************/
/*! \brief Sets the mode of the widget.
 */
void TEWI_set_mode(uint8_t mode)
{
    // todo: mode checking
    tewi_mode = mode;
}

/****************************************************************************************************************/
/*! \brief Queries and returns the current mode of the widget.
 */
int TEWI_get_mode(void)
{
    return tewi_mode;
}

/****************************************************************************************************************/
/*! \brief Set the starting value of string in the widget.
 * \note If the passed-in string is too long, the widget silently and gleefully truncates it.
 * \param in_str The string to start the widget with; NULL may be passed (in which case, the widget
 * starts out empty).
 */
void TEWI_set_string(const char *in_str)
{
    if (in_str == NULL)
    {
        bzero(tewi_priv_string, sizeof(tewi_priv_string));
        tewi_char_index = 0;
    }
    else
    {
        snprintf(tewi_priv_string, TEXT_ENTRY_WIDGET_STRING_LENGTH, "%s", in_str);
        tewi_char_index = strlen(in_str);
    }
}

/****************************************************************************************************************/
/*! \brief Set the vertical position of the widget in the display.
 */
void TEWI_set_vpos(float vpos)
{
    tewi_ystart = vpos;
}
