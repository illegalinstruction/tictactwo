#include "gameplay.h"
#include "client-common.h"
#include "screenwipe.h"
#include "particles.h"
#include "text_entry_widget.h"

#define     GAMEPLAY_GRID_GFX_PATH              "./gfx/gameplay/grid.tga"
#define     GAMEPLAY_X_GFX_PATH                 "./gfx/gameplay/x.tga"
#define     GAMEPLAY_O_GFX_PATH                 "./gfx/gameplay/o.tga"
#define     GAMEPLAY_X_INDICATIOR_GFX_PATH      "./gfx/gameplay/player_x.tga"
#define     GAMEPLAY_O_INDICATIOR_GFX_PATH      "./gfx/gameplay/player_o.tga"
#define     GAMEPLAY_CHATWIDGET_PATH            "./gfx/lobby/chat-widget.tga"


#define     GAMEPLAY_PLAYER_INDICATOR_W         240
#define     GAMEPLAY_PLAYER_INDICATOR_H         240
#define     GAMEPLAY_PLAYER_INDICATOR_X         (common_effective_display_width -  GAMEPLAY_PLAYER_INDICATOR_W)
#define     GAMEPLAY_PLAYER_INDICATOR_Y         16

#define     GAMEPLAY_GRID_X                     16
#define     GAMEPLAY_GRID_Y                     16
#define     GAMEPLAY_GRID_SIZE                  525

#define     GAMEPLAY_CHIT_SIZE                  175

#define     GAMEPLAY_CHATWIDGET_W               768
#define     GAMEPLAY_CHATWIDGET_H               96
#define     GAMEPLAY_CHATWIDGET_X               ((common_effective_display_width - GAMEPLAY_CHATWIDGET_W) / 2.0f)
#define     GAMEPLAY_CHATWIDGET_Y               0
#define     GAMEPLAY_CHATWIDGET_SLIDE_AMT       16

#define     GAMEPLAY_INCOMING_SND_PATH          "./snd/lobby/incoming.wav"


#define     GAMEPLAY_TURN_INDICATOR_X           550
#define     GAMEPLAY_TURN_INDICATOR_Y           400
#define     GAMEPLAY_TURN_INDICATOR_SIZE        26
#define     GAMEPLAY_TURN_INDICATOR_KERN        -5


/****************************************************************************************************************/
/*! \defgroup gameplay_priv Private functions and data for the gameplay module
 * \{ */
static  TEXTURE_HANDLE      gameplay_grid_gfx;
static  TEXTURE_HANDLE      gameplay_x_gfx;
static  TEXTURE_HANDLE      gameplay_o_gfx;
static  TEXTURE_HANDLE      gameplay_x_indicator_gfx;
static  TEXTURE_HANDLE      gameplay_o_indicator_gfx;
static  void                    GAMEPLAY_chat_helper(void);
static  BOOL                    gameplay_chatbox_active = FALSE;
static  int                     gameplay_chatbox_slide;
static  int                     gameplay_chat_timer;
static  TEXTURE_HANDLE       gameplay_chatwidget_gfx;
static char                     gameplay_incoming_chat[OUTGOING_CHAT_MESSAGE_LENGTH];
static SAMPLE *                 gameplay_chat_noise;
static BOOL                     gameplay_my_turn = FALSE;
static int                     gameplay_side = 0;
static int                      gameplay_board[3][3];
/*! \} */

/****************************************************************************************************************/

void GAMEPLAY_load(void)
{
    gameplay_grid_gfx           =   COMMON_load_texture(GAMEPLAY_GRID_GFX_PATH);
    gameplay_x_gfx              =   COMMON_load_texture(GAMEPLAY_X_GFX_PATH);
    gameplay_o_gfx              =   COMMON_load_texture(GAMEPLAY_O_GFX_PATH);
    gameplay_x_indicator_gfx    =   COMMON_load_texture(GAMEPLAY_X_INDICATIOR_GFX_PATH);
    gameplay_o_indicator_gfx    =   COMMON_load_texture(GAMEPLAY_O_INDICATIOR_GFX_PATH);
    gameplay_chatwidget_gfx     =   COMMON_load_texture(GAMEPLAY_CHATWIDGET_PATH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);            // fixes the artifacting along the bottom of the widget
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // the chat sound
    if (exists(GAMEPLAY_INCOMING_SND_PATH))
    {
        gameplay_chat_noise = load_sample(GAMEPLAY_INCOMING_SND_PATH);
    }
    else
    {
        DUH_WHERE_AM_I("missing asset: %s", GAMEPLAY_INCOMING_SND_PATH);
        gameplay_chat_noise = create_sample(8, FALSE, 22050, 1);
    }
}

/****************************************************************************************************************/

void GAMEPLAY_unload(void)
{
    glDeleteTextures(1, &gameplay_grid_gfx       );
    glDeleteTextures(1, &gameplay_x_gfx          );
    glDeleteTextures(1, &gameplay_o_gfx          );
    glDeleteTextures(1, &gameplay_o_indicator_gfx);
    glDeleteTextures(1, &gameplay_o_indicator_gfx);
    glDeleteTextures(1, &gameplay_chatwidget_gfx );
    destroy_sample(gameplay_chat_noise);
}

/****************************************************************************************************************/

void GAMEPLAY_init(void)
{
    SCRNWIPE_start(TRUE);
    gameplay_chatbox_active = FALSE;
    gameplay_chatbox_slide = -GAMEPLAY_CHATWIDGET_H;
    COMMON_set_bgm(BGM_ID_INGAME);
    gameplay_side = 0;

    int x_index;
    int y_index;

    for(y_index = 0; y_index < 3; y_index++)
    {
        for (x_index = 0; x_index < 3; x_index++)
        {
            gameplay_board[y_index][x_index] = 0;
        }
    }
}

/****************************************************************************************************************/

void GAMEPLAY_tick(void)
{
    if ((SCRNWIPE_check_active() == FALSE) && (SCRNWIPE_check_direction() == FALSE))
    {
        common_curr_state_done = TRUE;
    }

    char communication_buffer[OUTGOING_CHAT_MESSAGE_LENGTH];

    GAMEPLAY_chat_helper();

    static BOOL is_mouse_held;

    // handle incoming data first...
    BOOL had_msg_waiting = COMMON_recv(communication_buffer, sizeof(communication_buffer));

    // we need turn order before we do anything else
    if (gameplay_side == 0)
    {
        if (had_msg_waiting == FALSE)
            return;
        else
        {
            DUH_WHERE_AM_I("here 1. %c", communication_buffer[0]);

            if ((communication_buffer[0] == MSGTYPE_YOU_ARE_X) || (communication_buffer[0] == MSGTYPE_YOU_ARE_O))
            {
                gameplay_side = communication_buffer[0];
                had_msg_waiting = FALSE;

                if (gameplay_side == MSGTYPE_YOU_ARE_X)
                    gameplay_my_turn = TRUE;
            }
            else
            {
                DUH_WHERE_AM_I("here 1. %c", communication_buffer[0]);
                // the server is supposed to send which side we're on; if they don't, we kinda can't proceed
                return;
            }
        }
    }

    if (had_msg_waiting)
    {
        switch (communication_buffer[0])
        {
            case MSGTYPE_CHAT:
                strncpy(gameplay_incoming_chat,  &communication_buffer[1], OUTGOING_CHAT_MESSAGE_LENGTH);
                play_sample(gameplay_chat_noise, 255, 128, 1000, 0);
                gameplay_chat_timer = INCOMING_CHAT_TIMEOUT;
            break;

            //--------------------

            case MSGTYPE_ITS_YOUR_TURN:
            {
                gameplay_my_turn = TRUE;
                DUH_WHERE_AM_I("my turn now...");

                gameplay_board[0][0] = communication_buffer[1];
                gameplay_board[0][1] = communication_buffer[2];
                gameplay_board[0][2] = communication_buffer[3];

                gameplay_board[1][0] = communication_buffer[4];
                gameplay_board[1][1] = communication_buffer[5];
                gameplay_board[1][2] = communication_buffer[6];

                gameplay_board[2][0] = communication_buffer[7];
                gameplay_board[2][1] = communication_buffer[8];
                gameplay_board[2][2] = communication_buffer[9];
            }
            break;

            //---------------------

            case MSGTYPE_YOU_WIN:
            case MSGTYPE_YOU_LOSE:
            case MSGTYPE_YOU_TIE:
            {
                common_next_state = GAMESTATE_STAT_SCREEN;

                switch (communication_buffer[0])
                {
                    case MSGTYPE_YOU_WIN:
                        common_next_state_param = 'W';
                    break;

                    case MSGTYPE_YOU_LOSE:
                        common_next_state_param = 'L';
                    break;

                    case MSGTYPE_YOU_TIE:
                        common_next_state_param = 'T';
                    break;
                }

                SCRNWIPE_start(FALSE);
            }
        }
    }

    // are we still animating?
    if (SCRNWIPE_check_active())
    {
        // yeah, don't allow input yet
        return;
    }

    if (mouse_b & 1)
    {
        if (is_mouse_held == FALSE)
        {
            if ((mouse_x > GAMEPLAY_GRID_X) && (mouse_x < (GAMEPLAY_GRID_X + GAMEPLAY_GRID_SIZE)))
            {
                if ((mouse_y > GAMEPLAY_GRID_Y) && (mouse_y < (GAMEPLAY_GRID_Y + GAMEPLAY_GRID_SIZE)))
                {
                    if (gameplay_my_turn)
                    {
                        uint8_t column =  ((mouse_x - GAMEPLAY_GRID_X) / (GAMEPLAY_GRID_SIZE / 3));
                        uint8_t row    =  ((mouse_y - GAMEPLAY_GRID_Y) / (GAMEPLAY_GRID_SIZE / 3));

                        if (gameplay_board[row][column] == 0)
                        {
                            play_sample(gameplay_chat_noise, 255, 128, 750, 0);

                            gameplay_board[row][column] = gameplay_side;

                            int tmp;

                            for (tmp = 0; tmp < 10; tmp++)
                            {
                                PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 1.25f, (rand() % 20 - 10) / 1.25f,
                                    PARTICLE_SPARK, (rand() % 32) + 192, rand() % NUM_PARTICLE_COLOURS);
                                PARTICLE_spawn(mouse_x, mouse_y, (rand() % 20 - 10) / 2.0f, (rand() % 20 - 10) / 2.0f,
                                    PARTICLE_HEART, 255,  rand() % NUM_PARTICLE_COLOURS);
                            }

                            communication_buffer[0] = MSGTYPE_MOVE;
                            communication_buffer[1] = column;
                            communication_buffer[2] = row;

                            COMMON_send(communication_buffer, 3);
                            gameplay_my_turn = FALSE;
                        }
                    }
                }
            }
        }
        is_mouse_held = TRUE;
    }
    else
    {
        is_mouse_held = FALSE;
    }

    // the chat fadeout animation
    if (gameplay_chat_timer > 0) gameplay_chat_timer--;
}

/****************************************************************************************************************/

void GAMEPLAY_chat_helper(void)
{
    static BOOL was_tab_held;

    // did the user press the chat toggle key?
    if (key[KEY_TAB])
    {
        // yes
        if (!was_tab_held)
        {
            gameplay_chatbox_active = ~gameplay_chatbox_active;
            TEWI_enable_or_disable(gameplay_chatbox_active);

            if (gameplay_chatbox_active)
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
    if (key[KEY_ENTER] && gameplay_chatbox_active)
    {
        gameplay_chatbox_active = FALSE;

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
    if (gameplay_chatbox_active)
    {
        gameplay_chatbox_slide += GAMEPLAY_CHATWIDGET_SLIDE_AMT;
        if (gameplay_chatbox_slide > 0) gameplay_chatbox_slide = 0;
    }
    else
    {
        gameplay_chatbox_slide -= GAMEPLAY_CHATWIDGET_SLIDE_AMT;
        if (gameplay_chatbox_slide < -GAMEPLAY_CHATWIDGET_H) gameplay_chatbox_slide = -GAMEPLAY_CHATWIDGET_H;
    }

    // anchor the textfield to the chat widget sliding
    TEWI_set_vpos(gameplay_chatbox_slide + (GAMEPLAY_CHATWIDGET_H / 2.0f));
}

/****************************************************************************************************************/

void GAMEPLAY_draw(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // the player indicator
    if (gameplay_side == 'x')
    {
        COMMON_draw_sprite(gameplay_x_indicator_gfx, GAMEPLAY_PLAYER_INDICATOR_X, GAMEPLAY_PLAYER_INDICATOR_Y, 0,
            GAMEPLAY_PLAYER_INDICATOR_W, GAMEPLAY_PLAYER_INDICATOR_H);
    }
    else if (gameplay_side == 'o')
    {
        COMMON_draw_sprite(gameplay_o_indicator_gfx, GAMEPLAY_PLAYER_INDICATOR_X, GAMEPLAY_PLAYER_INDICATOR_Y, 0,
            GAMEPLAY_PLAYER_INDICATOR_W, GAMEPLAY_PLAYER_INDICATOR_H);
    }

    // the board
    COMMON_draw_sprite(gameplay_grid_gfx, GAMEPLAY_GRID_X, GAMEPLAY_GRID_Y, 0, GAMEPLAY_GRID_SIZE, GAMEPLAY_GRID_SIZE);

    int x;
    int y;

    for (y = 0; y < 3; y++)
    {
        for (x = 0; x < 3; x++)
        {
            if (gameplay_board[y][x] == 'x')
            {
                COMMON_draw_sprite(gameplay_x_gfx, GAMEPLAY_GRID_X + (x * GAMEPLAY_GRID_SIZE / 3),
                    GAMEPLAY_GRID_Y + (y * GAMEPLAY_GRID_SIZE / 3), 0, GAMEPLAY_CHIT_SIZE, GAMEPLAY_CHIT_SIZE);
            }

            if (gameplay_board[y][x] == 'o')
            {
                COMMON_draw_sprite(gameplay_o_gfx, GAMEPLAY_GRID_X + (x * GAMEPLAY_GRID_SIZE / 3),
                    GAMEPLAY_GRID_Y + (y * GAMEPLAY_GRID_SIZE / 3), 0, GAMEPLAY_CHIT_SIZE, GAMEPLAY_CHIT_SIZE);
            }
        }
    }

    // turn indicator
    if (gameplay_my_turn)
    {
        COMMON_glprint(common_gamefont, GAMEPLAY_TURN_INDICATOR_X, GAMEPLAY_TURN_INDICATOR_Y, 0,
            GAMEPLAY_TURN_INDICATOR_SIZE, GAMEPLAY_TURN_INDICATOR_KERN, "Your turn.");
    }

    // the chat widget
    //! \todo Abstract/factor the chat widget out (heavens, how I hated copypasting :^( )
    if (gameplay_chatbox_slide > -GAMEPLAY_CHATWIDGET_H)
    {
        COMMON_draw_sprite(gameplay_chatwidget_gfx, GAMEPLAY_CHATWIDGET_X, gameplay_chatbox_slide, 0,
            GAMEPLAY_CHATWIDGET_W, GAMEPLAY_CHATWIDGET_H);
    }

    // incoming chat messages (plus a little magic to make them fade out cleanly)
    if (gameplay_chat_timer > 0)
    {
        int alpha = gameplay_chat_timer * 4;
        if (alpha > 255) alpha = 255;
        glColor4ub(INCOMING_CHAT_COLOR, alpha);
        COMMON_glprint(common_gamefont, INCOMING_CHAT_X, INCOMING_CHAT_Y + INCOMING_CHAT_SIZE, 0,
            INCOMING_CHAT_SIZE, INCOMING_CHAT_KERN, gameplay_incoming_chat);
    }
}
