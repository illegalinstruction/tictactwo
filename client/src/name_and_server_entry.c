#include "name_and_server_entry.h"

#define LOGIN_PHASE_NAME_SLIDING_IN         1
#define LOGIN_PHASE_NAME_WAITING            2
#define LOGIN_PHASE_NAME_SLIDING_OUT        3

#define LOGIN_PHASE_IP_SLIDING_IN           4
#define LOGIN_PHASE_IP_WAITING              5
#define LOGIN_PHASE_IP_SLIDING_OUT          6

#define LOGIN_PHASE_AWAITING_CONNECTION     7

#define LOGIN_AVATAR_W                      64
#define LOGIN_AVATAR_H                      64

#define LOGIN_AVATAR_X                      ((common_effective_display_width - LOGIN_AVATAR_W) / 2.0f)
#define LOGIN_AVATAR_Y                      400

#define LOGIN_HEADER_W                      500
#define LOGIN_HEADER_H                      125

#define LOGIN_HEADER_X                      (int)((common_effective_display_width - LOGIN_HEADER_W) / 2.0f)
#define LOGIN_HEADER_Y                      8

#define LOGIN_HEADER_SLIDE_AMT              (30.0f * (common_effective_display_width / DISPLAY_WIDTH))

#define LOGIN_CFG_FILE_NAME                 "tictac2.rc"

#define LOGIN_NAME_GFX_PATH                 "./gfx/login/enter-name.tga"
#define LOGIN_IP_GFX_PATH                   "./gfx/login/server-ip.tga"
#define LOGIN_AVATAR_PATH                   "./gfx/avatar/"

#define LOGIN_TEXT_SIZE                     22
#define LOGIN_TEXT_KERN                     -6

#define LOGIN_AVATAR_LEGEND_X               ((common_effective_display_width - (40 * (LOGIN_TEXT_SIZE + LOGIN_TEXT_KERN))) / 2.0f)
#define LOGIN_AVATAR_LEGEND_Y               480
#define LOGIN_AVATAR_STRING                 "Click the picture to choose your avatar."

#define LOGIN_TEXT_WIDGET_VPOS              360

/*! \defgroup login_module_priv Private data and functions for the login screen gamestate.
 * \{  */
/*! \brief The 'enter your name' image. */
static TEXTURE_HANDLE   login_name_gfx;
/*! \brief The 'server ip:' image. */
static TEXTURE_HANDLE   login_ip_gfx;
/*! \brief Tracks where we are in the data collection and connecting process. */
static int              login_phase;
/*! \brief Animation variable for the header graphics. */
static float            login_header_slide;
static char             login_str_name[TEXT_ENTRY_WIDGET_STRING_LENGTH];
static char             login_str_ip[TEXT_ENTRY_WIDGET_STRING_LENGTH];
/*! \brief What avatar this user has chosen. */
static uint8_t          login_avatar_id;
/*! \brief The avatar textures. */
static TEXTURE_HANDLE   login_avatars[NUM_AVATARS];
static void             LOGIN_load_prefs(void);
static void             LOGIN_save_prefs(void);
/*! \} */

/****************************************************************************************************************/
/*! \brief Reset all of our variables to something sensible.
 * \todo make it read defaults for some stuff from the file...
 */
void LOGIN_init(void)
{
    login_header_slide = -LOGIN_HEADER_W;
    login_phase = LOGIN_PHASE_NAME_SLIDING_IN;
    SCRNWIPE_start(TRUE);
    TEWI_set_vpos(LOGIN_TEXT_WIDGET_VPOS);
    bzero(login_str_name, TEXT_ENTRY_WIDGET_STRING_LENGTH);
    bzero(login_str_ip, TEXT_ENTRY_WIDGET_STRING_LENGTH);
    login_avatar_id = 0;

    // if this fails, it'll leave them alone...
    LOGIN_load_prefs();
}

/****************************************************************************************************************/
/*! \brief Advance the logic of this gamestate by exactly one tick.
 *
 * All animation happens here, as well as the initial connection attempt with the server and the sending of the
 * player's screen name.
 *
 * \note The initial handshake with the server actually happens here as well.
 */
void LOGIN_tick(void)
{
    static BOOL mouse_down;

    if (SCRNWIPE_check_active() && SCRNWIPE_check_direction())
        return;

    switch (login_phase)
    {
        case LOGIN_PHASE_NAME_SLIDING_IN:
            if (login_header_slide < LOGIN_HEADER_X)
            {
                login_header_slide += LOGIN_HEADER_SLIDE_AMT;
            }
            else
            {
                login_phase = LOGIN_PHASE_NAME_WAITING;
                TEWI_set_string(login_str_name);
                TEWI_set_mode(TEXT_ENTRY_WIDGET_MODE_NAME);
                TEWI_enable_or_disable(TRUE);
            }
        break;

        //----------

        case LOGIN_PHASE_NAME_WAITING:
        {
            if (TEWI_check_active() == FALSE)
            {
                login_phase = LOGIN_PHASE_NAME_SLIDING_OUT;
                strncpy(login_str_name, TEWI_get_string(), TEXT_ENTRY_WIDGET_STRING_LENGTH);
            }

            // check if the avatar image was clicked on
            if (mouse_b & 1)
            {
                // make sure we aren't acting on a stale click
                if (!mouse_down)
                {
                    if ((mouse_x >= LOGIN_AVATAR_X) && (mouse_x <= (LOGIN_AVATAR_X + LOGIN_AVATAR_W)) &&
                        (mouse_y >= LOGIN_AVATAR_Y) && (mouse_y <= (LOGIN_AVATAR_Y + LOGIN_AVATAR_H)) )
                    {
                        MEFX_spawn_shockring();
                        login_avatar_id++;

                        if (login_avatar_id >= NUM_AVATARS)
                            login_avatar_id = 0;
                    }
                    mouse_down = TRUE;
                }
            }
            else
            {
                mouse_down = FALSE;
            }
        }
        break;

        //----------

        case LOGIN_PHASE_NAME_SLIDING_OUT:
            if (login_header_slide < common_effective_display_width)
            {
                login_header_slide += LOGIN_HEADER_SLIDE_AMT;
            }
            else
            {
                // anim done, go to ip entry
                login_phase = LOGIN_PHASE_IP_SLIDING_IN;
                login_header_slide = -LOGIN_HEADER_W;
            }
        break;

        //----------

        case LOGIN_PHASE_IP_SLIDING_IN:
            if (login_header_slide < LOGIN_HEADER_X)
            {
                login_header_slide += LOGIN_HEADER_SLIDE_AMT;
            }
            else
            {
                login_phase = LOGIN_PHASE_IP_WAITING;
                TEWI_set_string(login_str_ip);
                TEWI_set_mode(TEXT_ENTRY_WIDGET_MODE_NUMERIC);
                TEWI_enable_or_disable(TRUE);
            }
        break;

        //----------

        case LOGIN_PHASE_IP_WAITING:
        {
            if (TEWI_check_active() == FALSE)
            {
                login_phase = LOGIN_PHASE_IP_SLIDING_OUT;
                strncpy(login_str_ip, TEWI_get_string(), TEXT_ENTRY_WIDGET_STRING_LENGTH);
            }
        }
        break;

        //----------

        case LOGIN_PHASE_IP_SLIDING_OUT:
            if (login_header_slide < common_effective_display_width)
            {
                login_header_slide += LOGIN_HEADER_SLIDE_AMT;
            }
            else
            {
                SCRNWIPE_start(FALSE);
                login_phase = LOGIN_PHASE_AWAITING_CONNECTION;
            }
        break;

        //----------

        case LOGIN_PHASE_AWAITING_CONNECTION:
        {
            if (SCRNWIPE_check_active() == FALSE)
            {
                struct in_addr tmp;
                if (inet_aton(login_str_ip, &tmp) == FALSE)
                {
                    // not a real ip address, go get it again
                    login_phase = LOGIN_PHASE_IP_SLIDING_IN;
                    SCRNWIPE_start(TRUE);
                    login_header_slide = -LOGIN_HEADER_W;
                }
                else
                {
                    // we've gotten valid values for everything - save them so the
                    // user has sensible defaults for the next session.
                    LOGIN_save_prefs();

                    // let's try connecting!
                    BOOL success = COMMON_connect(tmp.s_addr, TICTACTWO_GAMEPLAY_PORT);

                    common_curr_state_done = TRUE;
                    if (success)
                    {
                        char communication_buffer[MAX_MESSAGE_SIZE];
                        communication_buffer[0] = MSGTYPE_LOGIN;
                        snprintf(&communication_buffer[1], MAX_NAME_LENGTH + 1, "%s", login_str_name);
                        communication_buffer[AVATAR_ID_POSITION] = login_avatar_id;
                        COMMON_send(communication_buffer, MAX_MESSAGE_SIZE);

                        common_next_state = GAMESTATE_LOBBY;
                    }
                    else
                    {
                        common_next_state = GAMESTATE_CONNECTION_FAILURE;
                    }
                }
            }
        }
        break;
    }
}

/****************************************************************************************************************/
/*! \brief Load the texture art we'll need to display this gamestate.
 */
void LOGIN_load(void)
{
    // load header graphics
    login_ip_gfx    = COMMON_load_texture(LOGIN_IP_GFX_PATH);
    login_name_gfx  = COMMON_load_texture(LOGIN_NAME_GFX_PATH);

    char buf[1024];
    int index;

    // load avatars
    for (index = 0; index < NUM_AVATARS; index++)
    {
        snprintf(buf, 1023, "%s%01d.tga", LOGIN_AVATAR_PATH, index);
        login_avatars[index] = COMMON_load_texture(buf);
    }

}


/****************************************************************************************************************/
/*! \brief A helper function to write out the player name, avatar choice and preferred server for next session.
 */
void LOGIN_save_prefs(void)
{
    char *filename_buffer;

    filename_buffer = (char *)malloc(strlen(common_user_home_path) + strlen(LOGIN_CFG_FILE_NAME) + 3);

    if (filename_buffer != NULL)
    {
        snprintf(filename_buffer, strlen(common_user_home_path) + strlen(LOGIN_CFG_FILE_NAME) + 2,
            "%s/%s", common_user_home_path, LOGIN_CFG_FILE_NAME);

        FILE *fout = fopen(filename_buffer, "wb");

        if (fout == NULL)
        {
            OH_SMEG("couldn't open savegame for writing - continuing, but something's wrong...");
            free(filename_buffer);
            return;
        }
        else
        {
            free(filename_buffer);
            fwrite(login_str_name, TEXT_ENTRY_WIDGET_STRING_LENGTH, 1, fout);
            fwrite(login_str_ip, TEXT_ENTRY_WIDGET_STRING_LENGTH, 1, fout);
            fputc(login_avatar_id, fout);
            fclose(fout);
            return; // successful
        }
    }
    else
    {
        OH_SMEG("Couldn't allocate big enough block to hold the savegame path.\nContinuing, but the game may run out of memory soon.");
    }
}


/****************************************************************************************************************/
/*! \brief A helper function to grab the player name, avatar choice and preferred server from a disk file.
 */
void LOGIN_load_prefs(void)
{
    char *filename_buffer;

    filename_buffer = (char *)malloc(strlen(common_user_home_path) + strlen(LOGIN_CFG_FILE_NAME) + 3);

    if (filename_buffer != NULL)
    {
        snprintf(filename_buffer, strlen(common_user_home_path) + strlen(LOGIN_CFG_FILE_NAME) + 2,
            "%s/%s", common_user_home_path, LOGIN_CFG_FILE_NAME);

        if (exists(filename_buffer))
        {
            FILE *fin  = fopen(filename_buffer, "rb");

            if (fin == NULL)
            {
                OH_SMEG("savegame exists, but we can't read it - continuing, but permissions might be screwy...");
                free(filename_buffer);
                return;
            }
            else
            {
                free(filename_buffer); // no longer need, since we've already opened the file.
                fread(login_str_name, TEXT_ENTRY_WIDGET_STRING_LENGTH, 1, fin);
                fread(login_str_ip, TEXT_ENTRY_WIDGET_STRING_LENGTH, 1, fin);
                login_avatar_id = fgetc(fin);
                fclose(fin);
                return; // successful
            }
        }
        else
            free(filename_buffer);
    }
    else
    {
        OH_SMEG("Couldn't allocate big enough block to hold the savegame path.\nContinuing, but the game may run out of memory soon.");
    }
}


/****************************************************************************************************************/
/*! \brief Discard textures and free up memory.
 */
void LOGIN_unload(void)
{
    // delete header graphics
    glDeleteTextures(1,&login_ip_gfx    );
    glDeleteTextures(1,&login_name_gfx  );

    // delete avatars
    glDeleteTextures(NUM_AVATARS, login_avatars);
}

/****************************************************************************************************************/
/*! \brief Draw the sliding header images.
 *
 * Very little else happens in here, since the background, particles, mouse effects and screen wipe are
 * drawn automagically by the main loop.
 */
void LOGIN_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    switch (login_phase)
    {
        case LOGIN_PHASE_NAME_SLIDING_IN:
        case LOGIN_PHASE_NAME_SLIDING_OUT:
            COMMON_draw_sprite(login_name_gfx, login_header_slide, LOGIN_HEADER_Y, 0, LOGIN_HEADER_W, LOGIN_HEADER_H);
        break;

        case LOGIN_PHASE_NAME_WAITING:
            COMMON_draw_sprite(login_name_gfx, login_header_slide, LOGIN_HEADER_Y, 0, LOGIN_HEADER_W, LOGIN_HEADER_H);
            COMMON_draw_sprite(login_avatars[login_avatar_id], LOGIN_AVATAR_X, LOGIN_AVATAR_Y, 0, LOGIN_AVATAR_W, LOGIN_AVATAR_H);
            COMMON_glprint(common_gamefont, LOGIN_AVATAR_LEGEND_X, LOGIN_AVATAR_LEGEND_Y, 0, LOGIN_TEXT_SIZE, LOGIN_TEXT_KERN,
                LOGIN_AVATAR_STRING);
        break;


        case LOGIN_PHASE_IP_SLIDING_IN:
        case LOGIN_PHASE_IP_WAITING:
        case LOGIN_PHASE_IP_SLIDING_OUT:
            COMMON_draw_sprite(login_ip_gfx, login_header_slide, LOGIN_HEADER_Y, 0, LOGIN_HEADER_W, LOGIN_HEADER_H);
            COMMON_glprint(common_gamefont, login_header_slide, LOGIN_HEADER_Y + LOGIN_HEADER_H + LOGIN_TEXT_SIZE, 0,
                LOGIN_TEXT_SIZE, LOGIN_TEXT_KERN, "Using name %s", login_str_name);

            COMMON_draw_sprite(login_avatars[login_avatar_id], login_header_slide,
                LOGIN_HEADER_Y + LOGIN_HEADER_H + LOGIN_TEXT_SIZE + LOGIN_TEXT_SIZE,
                0, LOGIN_AVATAR_W, LOGIN_AVATAR_H);

        break;
    }
}
