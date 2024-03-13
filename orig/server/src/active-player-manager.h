/*! \file active-player-manager.h
 * \brief A module for maintaining an active count of who's logged in, handing them off to the
 * gameroom manager, etc.  Listening for incoming connections, the lobby, chat and web server
 * are all handled here.
 */
#ifndef         ACTIVE_PLAYERS_H
    #define     ACTIVE_PLAYERS_H

    #include    "tictactwo-common.h"
    #include    "server-common.h"
    #include    "player_db.h"

    void                PLYRMNGR_init(void);
    void                PLYRMNGR_tick(void);
    void                PLYRMNGR_handle_lobby_refresh(PLAYER_STRUCT *ps);
    void                PLYRMNGR_handle_chat_msg(PLAYER_STRUCT *ps, const char *msg_text);
    PLAYER_STRUCT *     PLYRMNGR_handle_new_connect(const char *name, uint8_t avatar);
    void                PLYRMNGR_send_invite(PLAYER_STRUCT *inviter, const char *invitee_name);
    void                PLYRMNGR_resp_invite(PLAYER_STRUCT *invitee, const char *inviter_name);
    void                PLYRMNGR_handle_disconnect(PLAYER_STRUCT *ps);

#endif
