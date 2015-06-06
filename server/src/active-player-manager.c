#include "active-player-manager.h"
#include <fcntl.h>

/*! \defgroup player_manager_private
 * \brief Data and functions private to the active-player-manager module.
 * \{
 */
static PLAYER_STRUCT *active_players[MAX_ACTIVE_PLAYERS];
static void PLYRMNGR_check_for_new_connections(void);
static BOOL plyrmngr_was_module_inited = FALSE;
static void PLYRMNGR_cleanup(void);
static void PLYRMNGR_build_lobbylist(void);
static char plyrmngr_name_list_buffer[1 + (LOBBY_LIST_RECORD_SIZE * MAX_ACTIVE_PLAYERS)];
/*! \} */

/****************************************************************************************************************/
/*! \brief Readies the module for use.
 */
void PLYRMNGR_init(void)
{
    if (plyrmngr_was_module_inited) return;

    int index;

    for (index = 0; index < MAX_ACTIVE_PLAYERS; index++)
    {
        active_players[index] = NULL;
    }

    plyrmngr_was_module_inited = TRUE;
}

/****************************************************************************************************************/
/*! \brief Handle a newly-connected player by retrieving a PLAYER_STRUCT with their details; if they don't
 * exist in the DB yet, a new PLAYER_STRUCT will be created for them.
 *
 * \note If the server already has too many players, NULL is returned and nothing is added to the DB.
 */
PLAYER_STRUCT *PLYRMNGR_handle_new_connect(const char *name, uint8_t avatar)
{
    static int last_pool_index;
    int strides;

    BOOL success = FALSE;

    // find an empty slot in the pool first
    while (strides < MAX_ACTIVE_PLAYERS)
    {
        if (active_players[last_pool_index] == NULL)
        {
            success = TRUE;
            strides = MAX_ACTIVE_PLAYERS;
        }
        else
        {
            strides++;
            last_pool_index++;
            if (last_pool_index >= MAX_ACTIVE_PLAYERS)
            {
                last_pool_index = 0;
            }
        }
    }

    // is the server full?
    if (success == FALSE)
    {
        // eeyup
        return NULL;
    }

    PLAYER_STRUCT *tmp = PLYRDB_find_by_name(name);

    // new player?
    if (tmp == NULL)
    {
        tmp = PLYRDB_create_new_player(name);
    }

    active_players[last_pool_index] = tmp;
    tmp->avatar = avatar;
    PLYRMNGR_build_lobbylist();

    return tmp;
}

/****************************************************************************************************************/
/*! \brief A helper function that rebuilds the lobby list and caches it, so PLYRMNGR_tick() doesn't have to.
 */
static void PLYRMNGR_build_lobbylist(void)
{
    // structure of individual lobby list item:
    //  name    null  wins    losses     ties     avatar index   null padding
    // 0.....30  31  32...35 36.....39  40....43    44           45.......47

    bzero(plyrmngr_name_list_buffer, (LOBBY_LIST_RECORD_SIZE * MAX_ACTIVE_PLAYERS));

    plyrmngr_name_list_buffer[0] = MSGTYPE_REQUEST_LOBBY;

    int client_index;

    for (client_index = 0; client_index < MAX_ACTIVE_PLAYERS; client_index++)
    {
        if (active_players[client_index] !=  NULL)
        {
            // name
            memcpy(&plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE))],
                active_players[client_index]->name, 31);

            // force the trailing null
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE)) + 31] = 0;

            // wins
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 32] = (active_players[client_index]->games_won >> 24) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 33] = (active_players[client_index]->games_won >> 16) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 34] = (active_players[client_index]->games_won >>  8) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 35] = (active_players[client_index]->games_won      ) & 0xff;

            // losses
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 36] = (active_players[client_index]->games_lost >> 24) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 37] = (active_players[client_index]->games_lost >> 16) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 38] = (active_players[client_index]->games_lost >>  8) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 39] = (active_players[client_index]->games_lost      ) & 0xff;

            // ties
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 40] = (active_players[client_index]->games_tied >> 24) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 41] = (active_players[client_index]->games_tied >> 16) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 42] = (active_players[client_index]->games_tied >>  8) & 0xff;
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE )) + 43] = (active_players[client_index]->games_tied      ) & 0xff;

            // avatar
            plyrmngr_name_list_buffer[1 + (client_index * (LOBBY_LIST_RECORD_SIZE)) + 44] = active_players[client_index]->avatar;
        }
    }
}

/****************************************************************************************************************/
/*! \brief A helper function that checks whether there's a brand-new incoming connection yet.  If
 *  there isn't, it returns without doing anything; otherwise, it tries to add the player to2 the game server if there
 * is room. */
static void PLYRMNGR_check_for_new_connections(void)
{
    char    communication_buffer[MAX_MESSAGE_SIZE];
    struct  sockaddr_in tmp;
    int     tmp_len = sizeof(tmp);
    int     success = accept(server_listenfd_game, (struct sockaddr *) &tmp, &tmp_len);

    // was there anything waiting for us?
    if (success != -1)
    {
        // yep
        DUH_WHERE_AM_I(" --- got new connection, getting player's name.");
        bzero(communication_buffer, MAX_MESSAGE_SIZE);

        // block here and wait for a player name.
        // admittedly, blocking here is kind of lame, and this should be some kind of
        // timeout, but alas.  Maybe version 2.0.
        int fd_opts;

        fd_opts = fcntl(success, F_GETFL);
        fd_opts ^= O_NONBLOCK;
        fcntl(success, F_SETFL, fd_opts);
        recv(success, communication_buffer, 64, 0); // wait here - the client is supposed to do this immediately.

        // what did the client actually send us?
        if (communication_buffer[0] != MSGTYPE_LOGIN)
        {
            // garbage, that's what.
            communication_buffer[0] = MSGTYPE_FAILURE;
            send(success, communication_buffer, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
            close(success);
            return;
        }

        // does the server have room for them?
        PLAYER_STRUCT *tmp_plyr = PLYRMNGR_handle_new_connect(&communication_buffer[1], communication_buffer[AVATAR_ID_POSITION]);
        if (tmp_plyr == NULL)
        {
            // server was full
            communication_buffer[0] = MSGTYPE_FAILURE; // there's no client state for server full (yet) - future enhancement?
            send(success, communication_buffer, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
            close(success);
            return;
        }

        // we had room for them, at this point, they should be in the player pool
        // save their socket descriptor for talking to them later...
        tmp_plyr->connection_fd  = success;

        // they haven't been challenged yet, so they're invitable
        tmp_plyr->challenger_id     = -1;
        tmp_plyr->state             = GAMESTATE_LOBBY;
    }
}

/****************************************************************************************************************/
/*! \brief Walk thorough all the players that are currently connected and update them as needed.
 * \todo This could stand to be broken up a bit more for modularity/readability...
 */
void PLYRMNGR_tick(void)
{
    PLYRMNGR_check_for_new_connections();

    int     index;
    char    communication_buffer[OUTGOING_CHAT_MESSAGE_LENGTH];     // used for receiving messages from client
    ssize_t received;

    for (index = 0; index < MAX_ACTIVE_PLAYERS; index++)
    {
        if (active_players[index] !=  NULL)
        {

            // did this player send something?
            bzero(communication_buffer, MAX_MESSAGE_SIZE);

            if (active_players[index]->state != GAMESTATE_GAMEPLAY)
            {
                received = recv(active_players[index]->connection_fd, communication_buffer,
                    OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
            }

            if (received > 0)
            {
                switch (communication_buffer[0])
                {
                    case MSGTYPE_DONE_WITH_STAT_SCREEN:
                        active_players[index]->state = GAMESTATE_LOBBY;
                        active_players[index]->challenger_id = -1;
                        PLYRMNGR_build_lobbylist();
                    break;

                    //--------------------------

                    case MSGTYPE_CHAT :
                    {
                        char    out_buffer[OUTGOING_CHAT_MESSAGE_LENGTH];
                        int     client_index;

                        // format the chat message into the way it should display on the client
                        bzero(out_buffer, OUTGOING_CHAT_MESSAGE_LENGTH);
                        snprintf(out_buffer, OUTGOING_CHAT_MESSAGE_LENGTH, "%c%s: %s", MSGTYPE_CHAT,
                            active_players[index]->name, &communication_buffer[1]);

                        out_buffer[64] = active_players[index]->avatar;

                        // put it out to the console to ease debugging
                        DUH_WHERE_AM_I("%s", &out_buffer[1]);

                        // ...and propagate it.
                        for (client_index = 0; client_index < MAX_ACTIVE_PLAYERS; client_index++)
                        {
                            if (active_players[client_index] != NULL)
                            {
                                send(active_players[client_index]->connection_fd, out_buffer,
                                    OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
                            }
                        }
                    }
                    break;

                    // ---------------------

                    case MSGTYPE_CLIENT_QUITTING:
                    {
                        close(active_players[index]->connection_fd);
                        active_players[index]->state = GAMESTATE_NOT_CONNECTED;
                        active_players[index] = NULL;
                        PLYRMNGR_build_lobbylist();
                    }
                    break;

                    // ---------------------

                    case MSGTYPE_REQUEST_LOBBY:
                    {
                        int i = 0;

                        /*! \todo This stupidly sends the entire lobby, meaning ~6kbytes, every time - even if only
                         * one player is logged in. */

                        send(active_players[index]->connection_fd, plyrmngr_name_list_buffer, 1 + (LOBBY_LIST_RECORD_SIZE * MAX_ACTIVE_PLAYERS), MSG_DONTWAIT | MSG_NOSIGNAL);
                    }
                    break;

                    // ------------

                    case MSGTYPE_INVITE:
                        if (active_players[index]->state == GAMESTATE_LOBBY)
                        {
                            active_players[index]->state = GAMESTATE_WAITING_FOR_HANDSHAKE;

                            PLAYER_STRUCT *invitee = PLYRDB_find_by_name(&communication_buffer[1]);

                            // did we try to invite ourselves?
                            if (invitee == active_players[index])
                            {
                                // not allowed, for obvious reasons - autodecline
                                communication_buffer[0] = MSGTYPE_GOT_DECLINED;
                                send(active_players[index]->connection_fd, communication_buffer, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
                                active_players[index]->state = GAMESTATE_LOBBY;
                            }

                            // does player even exist and are they in an invitable state?
                            if ((invitee == NULL) || (invitee->state != GAMESTATE_LOBBY))
                            {
                                // player cannot be invited - autodecline
                                communication_buffer[0] = MSGTYPE_GOT_DECLINED;
                                send(active_players[index]->connection_fd, communication_buffer, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
                                active_players[index]->state = GAMESTATE_LOBBY;
                            }
                            else
                            {
                                // they're inviteable - only allow one active invite at a time...
                                invitee->state          = GAMESTATE_RECEIVED_INVITATION;
                                invitee->challenger_id  = index;

                                communication_buffer[0] = MSGTYPE_INVITE;

                                snprintf(&communication_buffer[1], MAX_NAME_LENGTH + 1, "%s",
                                    active_players[index]->name);

                                // tell them they've been invited (insert your own pinkie pie reference here)
                                send(invitee->connection_fd, communication_buffer, 1 + MAX_NAME_LENGTH + 1,
                                    MSG_DONTWAIT | MSG_NOSIGNAL);
                            }
                        }
                    break;

                    // ------------

                    case MSGTYPE_RESPOND_ACCEPT:
                    {
                        int acceptee_id = active_players[index]->challenger_id;

                        // inform the inviter that they've been matched
                        communication_buffer[0] = MSGTYPE_GOT_ACCEPTED;
                        send(active_players[acceptee_id]->connection_fd, communication_buffer, 1,
                            MSG_DONTWAIT | MSG_NOSIGNAL);

                        // remember they're in-game
                        active_players[index]->state            = GAMESTATE_GAMEPLAY;
                        active_players[acceptee_id]->state      = GAMESTATE_GAMEPLAY;

                        // retsuprae
                        GMRM_create_new(active_players[index], active_players[acceptee_id]);
                        DUH_WHERE_AM_I("starting game with %s and %s",active_players[index]->name, active_players[acceptee_id]->name);

                        /*! \todo MORE STUFF GOES HERE. */
                    }
                    break;

                    // ------------

                    case MSGTYPE_RESPOND_DECLINE:
                    {
                        if ((active_players[index]->state == GAMESTATE_RECEIVED_INVITATION) ||
                            (active_players[index]->state == GAMESTATE_WAITING_FOR_HANDSHAKE))
                        {
                            int declinee_id = active_players[index]->challenger_id;

                            // inform the inviter that they've been turned down
                            communication_buffer[0] = MSGTYPE_GOT_DECLINED;
                            if(active_players[declinee_id] != NULL)
                                active_players[declinee_id]->state = GAMESTATE_LOBBY;
                            send(active_players[declinee_id]->connection_fd, communication_buffer, 1,
                                MSG_DONTWAIT | MSG_NOSIGNAL);

                            // remember that the invitee turned them down
                            active_players[index]->state            = GAMESTATE_LOBBY;
                            active_players[index]->challenger_id    = -1;
                        }
                    }
                    break;

                    // ------------

                    case MSGTYPE_MOVE:
                        // handled elsewhere.
                    break;

                    default:
                    {                                                                                                                                                                             DUH_WHERE_AM_I("player %s sent invalid stuff",active_players[index]->name);
                        // client has sent a garbled response - don't attempt to handle it, just toss 'em
                        communication_buffer[0] = MSGTYPE_FAILURE;
                        send(active_players[index]->connection_fd, communication_buffer, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
                        close(active_players[index]->connection_fd);
                        PLYRMNGR_build_lobbylist();
                    }
                }
                received = 0;
            } // end the case where a message was received
        }
    } // end for
}

