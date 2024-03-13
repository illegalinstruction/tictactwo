#include "lobby.h"
#include "text_entry_widget.h"
#include "particles.h"

#define LOBBY_NAME_TEXT_SIZE    20
#define LOBBY_NAME_TEXT_KERN    -5
#define LOBBY_STATS_TEXT_SIZE   14
#define LOBBY_STATS_TEXT_KERN   -2

#define LOBBY_AVATAR_SIZE       50
#define LOBBY_LIST_X            23

#define LOBBY_NAMES_PER_PAGE    9

#define LOBBY_HEADER_W          448
#define LOBBY_HEADER_H          56
#define LOBBY_HEADER_X          ((common_effective_display_width - LOBBY_HEADER_W) / 2.0f)
#define LOBBY_HEADER_Y          4

#define LOBBY_TIME_BETWEEN_REFRESHES    (TICKS_PER_SEC * 10)

/* format of a name in the lobby refresh message:
 *  name    null  wins    losses     ties     avatar index   null padding
 * 0.....30  31  32...35 36.....39  40....43    44           45.......47
 */

#define LOBBY_NAME_DATA_SIZE     48

typedef struct
{
    char display_name[31];
    int wins;
    int losses;
    int ties;
    uint8_t av_id;
} LOBBY_DISPLAYABLE_NAME_PRIV;

/*! \defgroup lobby_asset_paths The file and path names of the textures this module needs to draw itself.
 * \{ */
#define LOBBY_AVATAR_PATH       "./gfx/avatar/"
#define LOBBY_HEADER_PATH       "./gfx/lobby/header.tga"
#define LOBBY_NEXTBTN_PATH      "./gfx/lobby/next-page.tga"
#define LOBBY_INVITE_PATH       "./gfx/lobby/invite.tga"
#define LOBBY_CHATWIDGET_PATH   "./gfx/lobby/chat-widget.tga"
#define LOBBY_INCOMING_SND_PATH "./snd/lobby/incoming.wav"
#define LOBBY_INVITE_SND_PATH   "./snd/lobby/invite.wav"
/*! \} */

/*! \defgroup lobby_chatwidget_macros These govern how the chat widget will be rendered.
 * \{ */
#define LOBBY_CHATWIDGET_W          768
#define LOBBY_CHATWIDGET_H          96
#define LOBBY_CHATWIDGET_X          ((common_effective_display_width - LOBBY_CHATWIDGET_W) / 2.0f)
#define LOBBY_CHATWIDGET_Y          0
#define LOBBY_CHATWIDGET_SLIDE_AMT  16
/*! \} */

#define LOBBY_NEXTBTN_W             108
#define LOBBY_NEXTBTN_H             108

#define LOBBY_NEXTBTN_X             (common_effective_display_width - LOBBY_NEXTBTN_W)
#define LOBBY_NEXTBTN_Y             (DISPLAY_HEIGHT - LOBBY_NEXTBTN_H)

#define LOBBY_INVITE_W             108
#define LOBBY_INVITE_H             108

#define LOBBY_INVITE_X             (common_effective_display_width - (LOBBY_INVITE_W * 3))
#define LOBBY_INVITE_Y             (DISPLAY_HEIGHT - LOBBY_NEXTBTN_H)


/*! \defgroup lobby_priv Private data and functions for the lobby gamestate.
 * \{ */
static TEXTURE_HANDLE   lobby_header_gfx;
static TEXTURE_HANDLE   lobby_next_gfx;
static TEXTURE_HANDLE   lobby_invite_gfx;
static TEXTURE_HANDLE   lobby_chatwidget_gfx;
static uint8_t          lobby_curr_page = 0;
/*! \brief The avatar textures. */
static TEXTURE_HANDLE   lobby_avatars[NUM_AVATARS];
/*! \brief A place to hold the incoming text for displaying. */
static char             lobby_incoming_chat[OUTGOING_CHAT_MESSAGE_LENGTH];
/*! \brief Tracks how long we've displayed the last message we got and times it out as needed. */
static int              lobby_incoming_chat_timer;
/*! \brief Used to display the avatar of the person who last spoke. */
static uint8_t          lobby_incoming_chat_avatar;
/*! \brief Plays whenever we get an incoming chat message. */
static SAMPLE *         lobby_chat_noise;
/*! \brief A place to hold incoming data - large enough to handle the names and stats of the max # of clients. */
static char             lobby_msg_buff[LOBBY_NAME_DATA_SIZE * MAX_ACTIVE_PLAYERS];
/*! \brief Tracks whether the chat widget should be rendered and can receive keystrokes */
static BOOL             lobby_chatbox_active = FALSE;
/*! \brief Tracks the slide-in/slide-out animation for the chat widget */
static int              lobby_chatbox_slide = -LOBBY_CHATWIDGET_H;
/*! \brief Helper function to prevent LOBBY_tick() from becoming too much of a big ball of mud... */
static void             LOBBY_chat_helper(void);
static void             LOBBY_name_list_helper(void);
/*! \brief The names we're going to show in the name list. \todo Should we allocate this dynamically? */
static LOBBY_DISPLAYABLE_NAME_PRIV lobby_name_list[MAX_ACTIVE_PLAYERS];
/*! \brief Counter to make sure we re-fetch the lobby every so often. */
static int              lobby_refresh_timer;
/*! \brief Player we're planning to invite. */
static int              lobby_highlighted_player;
/*! \brief Plays when you invite someone. */
static SAMPLE *         lobby_invite_noise;

/*! \} */

/****************************************************************************************************************/
/*! \brief Loads the assets required to display the lobby.
 */
void LOBBY_load(void)
{
    char buf[1024];
    int index;

    // load avatars
    for (index = 0; index < NUM_AVATARS; index++)
    {
        snprintf(buf, 1023, "%s%01d.tga",LOBBY_AVATAR_PATH, index);
        lobby_avatars[index] = COMMON_load_texture(buf);
    }

    // header image
    lobby_header_gfx = COMMON_load_texture(LOBBY_HEADER_PATH);

    // next page button
    lobby_next_gfx  = COMMON_load_texture(LOBBY_NEXTBTN_PATH);

    // invite/challenge button
    lobby_invite_gfx  = COMMON_load_texture(LOBBY_INVITE_PATH);

    // the backdrop of the chat widget
    lobby_chatwidget_gfx = COMMON_load_texture(LOBBY_CHATWIDGET_PATH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);            // fixes the artifacting along the bottom of the widget
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // the chat sound
    if (exists(LOBBY_INCOMING_SND_PATH))
    {
        lobby_chat_noise = load_sample(LOBBY_INCOMING_SND_PATH);
    }
    else
    {
        DUH_WHERE_AM_I("missing asset: %s", LOBBY_INCOMING_SND_PATH);
        lobby_chat_noise = create_sample(8, FALSE, 22050, 1);
    }

    // the invite sound
    if (exists(LOBBY_INVITE_SND_PATH))
    {
        lobby_invite_noise = load_sample(LOBBY_INVITE_SND_PATH);
    }
    else
    {
        DUH_WHERE_AM_I("missing asset: %s", LOBBY_INVITE_SND_PATH);
        lobby_invite_noise = create_sample(8, FALSE, 22050, 1);
    }
}


/****************************************************************************************************************/
/*! \brief Unload any textures that were being used by the lobby and free up memory.
 */
void LOBBY_unload(void)
{
    // avatars
    glDeleteTextures(NUM_AVATARS, lobby_avatars);

    // header image
    glDeleteTextures(1, &lobby_header_gfx);

    // next page button
    glDeleteTextures(1, &lobby_next_gfx);

    // next invite button
    glDeleteTextures(1, &lobby_invite_gfx);

    // the backdrop of the chat widget
    glDeleteTextures(1, &lobby_chatwidget_gfx);

    // the sounds
    destroy_sample(lobby_chat_noise);
    destroy_sample(lobby_invite_noise);
}


/****************************************************************************************************************/
/*! \brief Set all variables to safe values, start the transition in, etc.
 * \note We also make the initial request for the lobby list here, but the response from the
 * server isn't handled until the first time LOBBY_tick() runs.
 */
void LOBBY_init(void)
{
    SCRNWIPE_start(TRUE);

    char out_buffer[MAX_MESSAGE_SIZE];
    out_buffer[0] = MSGTYPE_REQUEST_LOBBY;

    COMMON_send(out_buffer, 1);
    lobby_refresh_timer = LOBBY_TIME_BETWEEN_REFRESHES;

    lobby_chatbox_active = FALSE;
    lobby_chatbox_slide = -LOBBY_CHATWIDGET_H;

    lobby_curr_page = 0;
    lobby_incoming_chat_timer = 0;

    lobby_highlighted_player = -1;

    TEWI_set_string(NULL);

    // ugly hack - restart the music here if it's not playing due to weird state interaction
    if (common_bgm_fade_done)
    {
        COMMON_set_bgm(0);
    }

    COMMON_set_bgm(BGM_ID_MAINMENU_LOBBY);
}


/****************************************************************************************************************/
/*! \brief Gather user input, and handle things like chat, refreshing, and game invites.
 */
void LOBBY_tick(void)
{
    static BOOL mbutton_held = FALSE;

    // are we still transitioning in? don't allow user or network interaction yet
    if (SCRNWIPE_check_active() )
        return;

    // handle incoming data first...
    BOOL had_msg_waiting = COMMON_recv(lobby_msg_buff, sizeof(lobby_msg_buff));

    if (had_msg_waiting)
    {
        switch (lobby_msg_buff[0])
        {
            case MSGTYPE_CHAT:
                strncpy(lobby_incoming_chat,  &lobby_msg_buff[1], OUTGOING_CHAT_MESSAGE_LENGTH);
                play_sample(lobby_chat_noise, 255, 128, 1000, 0);
                lobby_incoming_chat_timer = INCOMING_CHAT_TIMEOUT;
                lobby_incoming_chat_avatar = lobby_msg_buff[64];
            break;

            case MSGTYPE_REQUEST_LOBBY:
                LOBBY_name_list_helper();
            break;

            case MSGTYPE_INVITE:
                common_next_state = GAMESTATE_RECEIVED_INVITATION;
                SCRNWIPE_start(FALSE);
                COMMON_fade_bgm();
            break;

            case MSGTYPE_GOT_DECLINED:                          // ugly hack - decline arrived before we were ready for it - this can only happen if you invite yourself, so don't.
            {
                /*! \todo better way of handling the player attempting to invite themselves -or- the client just shouldn't let you. */
                common_next_state = GAMESTATE_LOBBY;
                COMMON_set_bgm(0);
                COMMON_set_bgm(BGM_ID_MAINMENU_LOBBY);
            }
            break;

            case MSGTYPE_FAILURE:
            {
                COMMON_disconnect();
                common_next_state = GAMESTATE_CONNECTION_FAILURE;
            }
        }
    }

    // handle chat
    LOBBY_chat_helper();

    if (lobby_incoming_chat_timer > 0)
    {
        lobby_incoming_chat_timer--;
    }

    // is it time for a refresh yet?
    lobby_refresh_timer--;
    if (lobby_refresh_timer <= 0)
    {
        lobby_refresh_timer = LOBBY_TIME_BETWEEN_REFRESHES;
        char cmd = MSGTYPE_REQUEST_LOBBY;
        COMMON_send(&cmd, 1);
    }

    // handle mouse input
    if (lobby_chatbox_active == FALSE)
    {
        if (mouse_b & 1)
        {
            if (mbutton_held == FALSE)
            {
                MEFX_spawn_shockring();

                // inside one of the buttons?
                // next page
                if ((mouse_x >= LOBBY_NEXTBTN_X) && (mouse_x <= (LOBBY_NEXTBTN_X + LOBBY_NEXTBTN_W)) &&
                    (mouse_y > LOBBY_NEXTBTN_Y) && (mouse_y <= (LOBBY_NEXTBTN_Y + LOBBY_NEXTBTN_H)))
                {
                    int tmp;

                    for (tmp = 0; tmp < 10; tmp++)
                    {
                        PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 1.25f, (rand() % 20 - 10) / 1.25f,
                            PARTICLE_SPARK, (rand() % 64) + 64, rand() % NUM_PARTICLE_COLOURS);
                    }

                    lobby_curr_page++;
                    lobby_curr_page = lobby_curr_page % (MAX_ACTIVE_PLAYERS / LOBBY_NAMES_PER_PAGE);
                }

                //-------------------------------------------------------

                // invite
                if ((mouse_x >= LOBBY_INVITE_X) && (mouse_x <= (LOBBY_INVITE_X + LOBBY_INVITE_W)) &&
                    (mouse_y >  LOBBY_INVITE_Y) && (mouse_y <= (LOBBY_INVITE_Y + LOBBY_INVITE_H)))
                {
                    if (lobby_highlighted_player != -1)
                    {
                        int tmp;

                        for (tmp = 0; tmp < 10; tmp++)
                        {
                            PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 1.25f, (rand() % 20 - 10) / 1.25f,
                                PARTICLE_SPARK, (rand() % 32) + 192, rand() % NUM_PARTICLE_COLOURS);
                            PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 2.0f, (rand() % 20 - 10) / 2.0f,
                                PARTICLE_HEART, 255,  rand() % NUM_PARTICLE_COLOURS);
                        }

                        char communications_buffer[MAX_MESSAGE_SIZE];
                        communications_buffer[0] = MSGTYPE_INVITE;
                        snprintf(&communications_buffer[1], MAX_NAME_LENGTH + 1, "%s",
                            lobby_name_list[lobby_highlighted_player].display_name);

                        COMMON_send(communications_buffer, MAX_MESSAGE_SIZE);

                        play_sample(lobby_invite_noise, 255, 128, 1000, 0);

                        SCRNWIPE_start(FALSE);
                        COMMON_fade_bgm();

                        common_next_state = GAMESTATE_WAITING_FOR_HANDSHAKE;
                    }
                }

                //-------------------------------------------------------

                // inside one of the player names?
                if (mouse_x > LOBBY_LIST_X)
                {
                    int transformed_y       = mouse_y - (LOBBY_HEADER_Y + LOBBY_HEADER_H);
                    int highlighted_index   = transformed_y / LOBBY_AVATAR_SIZE;

                    if ((highlighted_index >= 0) && (highlighted_index < LOBBY_NAMES_PER_PAGE))
                    {
                        lobby_highlighted_player = highlighted_index + (LOBBY_NAMES_PER_PAGE * lobby_curr_page);
                    }
                    else
                        lobby_highlighted_player = -1;

                    // don;t allow to select empty players
                    if (lobby_name_list[lobby_highlighted_player].display_name[0] == 0)
                        lobby_highlighted_player = -1;
                }
            }

            mbutton_held = TRUE;
        }
        else
            mbutton_held = FALSE;
    }

    if ((SCRNWIPE_check_active() == FALSE) && (SCRNWIPE_check_direction() == FALSE) && (common_bgm_fade_done == TRUE))
    {
        // animation's done.
        common_curr_state_done = TRUE;
    }
}


/****************************************************************************************************************/
/*! \brief Handling of incoming lobby refresh here; LOBBY_tick() calls this.
 */
void LOBBY_name_list_helper(void)
{
    int buffer_index = 1; // we need to skip the command byte
    int player_index = 0;
    int list_index   = 0;

    for(player_index = 0; player_index < MAX_ACTIVE_PLAYERS; player_index++)
    {
        if (&lobby_msg_buff[buffer_index] != 0)
        {
            strncpy(lobby_name_list[list_index].display_name, &lobby_msg_buff[buffer_index],MAX_NAME_LENGTH + 1);
            lobby_name_list[list_index].wins    = htonl(*(int *)(&lobby_msg_buff[buffer_index + 32]));
            lobby_name_list[list_index].losses  = htonl(*(int *)(&lobby_msg_buff[buffer_index + 36]));
            lobby_name_list[list_index].ties    = htonl(*(int *)(&lobby_msg_buff[buffer_index + 40]));
            lobby_name_list[list_index].av_id   = lobby_msg_buff[buffer_index + 44];

            list_index++;
        }

        buffer_index += LOBBY_NAME_DATA_SIZE;
    }

    lobby_msg_buff[0] = 0;
}


/****************************************************************************************************************/
/*! \brief Handling of chat happens here; LOBBY_tick() calls this.
 */
void LOBBY_chat_helper(void)
{
    static BOOL was_tab_held;

    // did the user press the chat toggle key?
    if (key[KEY_TAB])
    {
        // yes
        if (!was_tab_held)
        {
            lobby_chatbox_active = ~lobby_chatbox_active;
            TEWI_enable_or_disable(lobby_chatbox_active);

            if (lobby_chatbox_active)
            {
                TEWI_set_mode(TEXT_ENTRY_WIDGET_MODE_NORMAL);
                TEWI_set_string(NULL);
            }

            was_tab_held = TRUE;
        }
    }
    else
    {
        // no
        was_tab_held = FALSE;
    }

    // did the user just press the chat-entry key?
    if (key[KEY_ENTER] && lobby_chatbox_active)
    {
        lobby_chatbox_active = FALSE;

        // is there actually data there?
        if (TEWI_get_string()[0] != 0)
        {
            // send it to the server as a chat message
            char buf[OUTGOING_CHAT_MESSAGE_LENGTH];

            snprintf(buf, OUTGOING_CHAT_MESSAGE_LENGTH, "%c%s", MSGTYPE_CHAT, TEWI_get_string());
            COMMON_send(buf, OUTGOING_CHAT_MESSAGE_LENGTH);
            TEWI_set_string(NULL);
        }
    }

    // handle animating the chat widget in or out, as needed
    if (lobby_chatbox_active)
    {
        lobby_chatbox_slide += LOBBY_CHATWIDGET_SLIDE_AMT;
        if (lobby_chatbox_slide > 0) lobby_chatbox_slide = 0;
    }
    else
    {
        lobby_chatbox_slide -= LOBBY_CHATWIDGET_SLIDE_AMT;
        if (lobby_chatbox_slide < -LOBBY_CHATWIDGET_H) lobby_chatbox_slide = -LOBBY_CHATWIDGET_H;
    }

    // anchor the textfield to the chat widget sliding
    TEWI_set_vpos(lobby_chatbox_slide + (LOBBY_CHATWIDGET_H / 2.0f));
}


/****************************************************************************************************************/
/*! \brief Repaint this screen's components.
 */
void LOBBY_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // the 'who's logged in?' across the top
    COMMON_draw_sprite(lobby_header_gfx, LOBBY_HEADER_X, LOBBY_HEADER_Y, 0, LOBBY_HEADER_W, LOBBY_HEADER_H);

    // page indicator
    COMMON_glprint(common_gamefont, 10, 6, 0, 18,-1, "Page %d/%d",
        lobby_curr_page + 1, (MAX_ACTIVE_PLAYERS / LOBBY_NAMES_PER_PAGE));

    // the chat widget
    if (lobby_chatbox_slide > -LOBBY_CHATWIDGET_H)
    {
        COMMON_draw_sprite(lobby_chatwidget_gfx, LOBBY_CHATWIDGET_X, lobby_chatbox_slide, 0,
            LOBBY_CHATWIDGET_W, LOBBY_CHATWIDGET_H);
    }

    // painting of avatars and the lobby name list
    int plyr_index;

    for (   plyr_index = (LOBBY_NAMES_PER_PAGE * lobby_curr_page);
            plyr_index < (LOBBY_NAMES_PER_PAGE * (lobby_curr_page + 1));
            plyr_index++)
    {
        if (lobby_name_list[plyr_index].display_name[0] != 0)
        {
            float y_pos = LOBBY_HEADER_Y + LOBBY_HEADER_H + ((plyr_index - (LOBBY_NAMES_PER_PAGE * lobby_curr_page)) * LOBBY_AVATAR_SIZE);

            // this player is selected?
            if (plyr_index == lobby_highlighted_player)
            {
                // yes
                glColor4ub(255, 255, 255, 64);
                COMMON_draw_sprite(0, LOBBY_LIST_X, y_pos, 0, DISPLAY_WIDTH, LOBBY_AVATAR_SIZE);
                glColor3ub(255, 255, 255);
            }

            COMMON_draw_sprite(lobby_avatars[lobby_name_list[plyr_index].av_id], LOBBY_LIST_X,
                y_pos,
                0,
                LOBBY_AVATAR_SIZE, LOBBY_AVATAR_SIZE);

            COMMON_glprint(common_gamefont, LOBBY_LIST_X + LOBBY_AVATAR_SIZE, y_pos, 0, LOBBY_NAME_TEXT_SIZE,
                LOBBY_NAME_TEXT_KERN,  "%s %dW/%dL/%dT",lobby_name_list[plyr_index].display_name,
                lobby_name_list[plyr_index].wins, lobby_name_list[plyr_index].losses, lobby_name_list[plyr_index].ties);
        }
    }

    // the buttons at the bottom
    COMMON_draw_sprite(lobby_next_gfx, LOBBY_NEXTBTN_X, LOBBY_NEXTBTN_Y, 0, LOBBY_NEXTBTN_W, LOBBY_NEXTBTN_H);

    // is there no one selected?
    if (lobby_highlighted_player == -1)
    {
        // make this faded out to indicate it can't be clicked right now.
        glColor4ub(255, 255, 255, 32);
    }
    COMMON_draw_sprite(lobby_invite_gfx, LOBBY_INVITE_X, LOBBY_INVITE_Y, 0, LOBBY_INVITE_W, LOBBY_INVITE_H);

    // incoming chat messages (plus a little magic to make them fade out cleanly)
    if (lobby_incoming_chat_timer > 0)
    {
        int alpha = lobby_incoming_chat_timer * 4;
        if (alpha > 255) alpha = 255;
        glColor4ub(INCOMING_CHAT_COLOR, alpha);
        COMMON_glprint(common_gamefont, INCOMING_CHAT_X, INCOMING_CHAT_Y, 0,
            INCOMING_CHAT_SIZE, INCOMING_CHAT_KERN, lobby_incoming_chat);

        glColor4ub(255, 255, 255, alpha);
        COMMON_draw_sprite(lobby_avatars[lobby_incoming_chat_avatar], INCOMING_CHAT_X - 64, INCOMING_CHAT_Y, 0,
            64, 64);

    }
}
