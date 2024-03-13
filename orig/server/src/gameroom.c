#include "gameroom.h"

#define GAMEROOM_MAX_IDLE_TICKS     10000

/*! \defgroup gameroom_module_private
 * \brief Functions and data private to the gameroom module.
 * \{
 */

/*! \brief A table of game rooms. */
static GAMEROOM_STRUCT gamerooms[MAX_ACTIVE_ROOMS];

/*! \brief Tracks whether this module was inited already; tries to prevent it from
 * getting inited more than once.
 */
static BOOL gmrm_was_module_inited = FALSE;

/*! \} */

/****************************************************************************************************************/
/*! \brief Marks all the gamerooms as idle and prepares them for use. Needs to be called once, when the server
 * is started.
 */
void GMRM_init(void)
{
    if (gmrm_was_module_inited)
    {
        DUH_WHERE_AM_I("WARNING: Tried to init the gameroom module more than once.");
        return;
    }

    int index;

    for (index = 0; index < MAX_ACTIVE_ROOMS; index++)
    {
        gamerooms[index].occupied   = FALSE;
        gamerooms[index].plyr_1     = NULL;
        gamerooms[index].plyr_2     = NULL;
    }
}

/****************************************************************************************************************/
/*! \brief Checks whether either player has won the specified game
 * \return GAMEROOM_PLAYER_ONE_WON or GAMEROOM_PLAYER_TWO_WON if someone won, GAMEROOM_TIE if nobody won,
 *  or GAMEROOM_STILL_PLAYING if the game is still underway.
 * \param gs The gameroom to check.
 */
uint8_t GMRM_check_if_won(const GAMEROOM_STRUCT *gs)
{
    // verticals
    if ((gs->board[0 + (0 * BOARD_WIDTH)] == gs->board[0 + (1 * BOARD_WIDTH)]) &&
        (gs->board[0 + (0 * BOARD_WIDTH)] == gs->board[0 + (2 * BOARD_WIDTH)]) && ((gs->board[0 + (0 * BOARD_WIDTH)] != 0)))
        return gs->board[0 + (0 * BOARD_WIDTH)];

    if ((gs->board[1 + (0 * BOARD_WIDTH)] == gs->board[1 + (1 * BOARD_WIDTH)]) &&
        (gs->board[1 + (0 * BOARD_WIDTH)] == gs->board[1 + (2 * BOARD_WIDTH)]) && ((gs->board[1 + (0 * BOARD_WIDTH)] != 0)))
        return gs->board[1 + (0 * BOARD_WIDTH)];

    if ((gs->board[2 + (0 * BOARD_WIDTH)] == gs->board[2 + (1 * BOARD_WIDTH)]) &&
        (gs->board[2 + (0 * BOARD_WIDTH)] == gs->board[2 + (2 * BOARD_WIDTH)]) && ((gs->board[2 + (0 * BOARD_WIDTH)] != 0)))
        return gs->board[2 + (0 * BOARD_WIDTH)];

    // horizontals
    if ((gs->board[0 + (0 * BOARD_WIDTH)] == gs->board[1 + (0 * BOARD_WIDTH)]) &&
        (gs->board[0 + (0 * BOARD_WIDTH)] == gs->board[2 + (0 * BOARD_WIDTH)]) && ((gs->board[0 + (0 * BOARD_WIDTH)] != 0)))
        return gs->board[0 + (0 * BOARD_WIDTH)];

    if ((gs->board[0 + (1 * BOARD_WIDTH)] == gs->board[1 + (1 * BOARD_WIDTH)]) &&
        (gs->board[0 + (1 * BOARD_WIDTH)] == gs->board[2 + (1 * BOARD_WIDTH)]) && ((gs->board[0 + (1 * BOARD_WIDTH)] != 0)))
        return gs->board[0 + (1 * BOARD_WIDTH)];

    if ((gs->board[0 + (2 * BOARD_WIDTH)] == gs->board[1 + (2 * BOARD_WIDTH)]) &&
        (gs->board[0 + (2 * BOARD_WIDTH)] == gs->board[2 + (2 * BOARD_WIDTH)]) && ((gs->board[0 + (2 * BOARD_WIDTH)] != 0)))
        return gs->board[0 + (2 * BOARD_WIDTH)];

    // diagonals
    if ((gs->board[0 + (0 * BOARD_WIDTH)] == gs->board[1 + (1 * BOARD_WIDTH)]) &&
        (gs->board[0 + (0 * BOARD_WIDTH)] == gs->board[2 + (2 * BOARD_WIDTH)]) && ((gs->board[0 + (0 * BOARD_WIDTH)] != 0)))
        return gs->board[0 + (0 * BOARD_WIDTH)];

    if ((gs->board[0 + (2 * BOARD_WIDTH)] == gs->board[1 + (1 * BOARD_WIDTH)]) &&
        (gs->board[0 + (2 * BOARD_WIDTH)] == gs->board[2 + (0 * BOARD_WIDTH)]) && ((gs->board[0 + (2 * BOARD_WIDTH)] != 0)))
        return gs->board[0 + (2 * BOARD_WIDTH)];

    // cats game, or still underway?
    int x_index, y_index;

    for (y_index = 0; y_index < 3; y_index++)
    {
        for (x_index = 0; x_index < 3; x_index++)
        {
            // found an empty space, game needs to continue.
            if (gs->board[(y_index * BOARD_WIDTH) +x_index] == 0)
                return GAMEROOM_STILL_PLAYING;
        }
    }

    return GAMEROOM_TIE;
}

/****************************************************************************************************************/
/*! \brief Attempt to start a new game with the specified players.
 * \return FALSE if there were no free gamerooms, or TRUE if it succeeded.
 * \note Notifies both interested clients of success or failure.
 */
BOOL GMRM_create_new(PLAYER_STRUCT *player_1, PLAYER_STRUCT *player_2)
{
    char packet;
    static int pool_index;
    int walk;

    while (walk < MAX_ACTIVE_ROOMS)
    {
        if (gamerooms[pool_index].occupied)
        {
            pool_index++;
            pool_index = pool_index % MAX_ACTIVE_ROOMS;
            walk++;
        }
        else
        {
            // ♥ don't disturb this rooooom... ♫
            gamerooms[pool_index].occupied = TRUE;

            // set up players.
            // we decide randomly who gets to be p1 and p2 so that
            // the same person doesn't always go first.
            if (rand() & 1)
            {
                gamerooms[pool_index].plyr_1 = player_1;
                gamerooms[pool_index].plyr_2 = player_2;
            }
            else
            {
                gamerooms[pool_index].plyr_1 = player_2;
                gamerooms[pool_index].plyr_2 = player_1;
            }

            // clear the board and set us up to start with player 1
            bzero(gamerooms[pool_index].board, BOARD_HEIGHT * BOARD_WIDTH);
            gamerooms[pool_index].whose_turn = 1;

            // notify the clients that the game is ready to start
            packet = MSGTYPE_YOU_ARE_X;
            send(gamerooms[pool_index].plyr_1->connection_fd, &packet, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
            usleep(150000);
            send(gamerooms[pool_index].plyr_1->connection_fd, &packet, 1, MSG_DONTWAIT | MSG_NOSIGNAL);

            packet = MSGTYPE_YOU_ARE_O;
            // I have no idea why the client never sees this, let's force the issue by sending it twice...
            send(gamerooms[pool_index].plyr_2->connection_fd, &packet, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
            usleep(150000);
            send(gamerooms[pool_index].plyr_2->connection_fd, &packet, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
            // maybe the client is in the wrong state when it arrives? ~shrug~

            // don't start searching on this room next time, since we just started using it
            pool_index++;
            pool_index = pool_index % MAX_ACTIVE_ROOMS;

            return TRUE;
        }
    }

    // packet = MSGTYPE_NO_FREE_ROOMS; // not implemented yet
    packet = MSGTYPE_FAILURE;

    send(player_1->connection_fd, &packet, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
    send(player_2->connection_fd, &packet, 1, MSG_DONTWAIT | MSG_NOSIGNAL);

    return FALSE;
}

/****************************************************************************************************************/
/*! \brief Advances all active rooms, handling incoming messages and trying to reap old/unused games.
 *  \bug It's possible to eat up a room by going into gameplay, then sending a chat message once every five
 *  minutes, if done by enough players, it forms a denial-of-service attack. I am not going to fix this right
 *  now, though.
 */
void GMRM_tick_all(void)
{
    char    communication_buffer_1[MAX_MESSAGE_SIZE];
    char    communication_buffer_2[MAX_MESSAGE_SIZE];
    char    out_buffer[OUTGOING_CHAT_MESSAGE_LENGTH];
    int     got_from_1;
    int     got_from_2;
    int     index;

    for (index = 0; index < MAX_ACTIVE_ROOMS; index++)
    {
        if (gamerooms[index].occupied)
        {
            gamerooms[index].ticks_since_last_move++;

            bzero(communication_buffer_1, MAX_MESSAGE_SIZE);
            bzero(communication_buffer_2, MAX_MESSAGE_SIZE);
            bzero(out_buffer, OUTGOING_CHAT_MESSAGE_LENGTH);

            // check to see if either player has communicated with us
            got_from_1 = recv(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                MAX_MESSAGE_SIZE, MSG_DONTWAIT | MSG_NOSIGNAL);

            got_from_2 = recv(gamerooms[index].plyr_2->connection_fd, communication_buffer_2,
                MAX_MESSAGE_SIZE, MSG_DONTWAIT | MSG_NOSIGNAL);

            // got anhything?
            if ((got_from_1 > 0) || (got_from_2 > 0))
            {
                // at least one player did something - room isn't idling anymore
                gamerooms[index].ticks_since_last_move = 0;

                // handle chat messages - these are private to the players in the game room

                if (communication_buffer_1[0] == MSGTYPE_CHAT)
                {
                    char tmp[OUTGOING_CHAT_MESSAGE_LENGTH];
                    tmp[0] = MSGTYPE_CHAT;

                    snprintf(&tmp[1], OUTGOING_CHAT_MESSAGE_LENGTH-1, "%s: %s", gamerooms[index].plyr_1->name, &communication_buffer_1[1]);
                    send(gamerooms[index].plyr_1->connection_fd, tmp,
                        OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
                    send(gamerooms[index].plyr_2->connection_fd, tmp,
                        OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
                }

                if (communication_buffer_2[0] == MSGTYPE_CHAT)
                {
                    char tmp[OUTGOING_CHAT_MESSAGE_LENGTH];
                    tmp[0] = MSGTYPE_CHAT;

                    snprintf(&tmp[1], OUTGOING_CHAT_MESSAGE_LENGTH-1, "%s: %s", gamerooms[index].plyr_2->name, &communication_buffer_2[1]);
                    send(gamerooms[index].plyr_1->connection_fd, tmp,
                        OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
                    send(gamerooms[index].plyr_2->connection_fd, tmp,
                        OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
                }

                // handle gameplay messages
                // these are laid out like so:
                //
                // [0]   [1  2   .......30]  [31]
                // cmd   col row NULL bytes  NULL byte
                switch (gamerooms[index].whose_turn)
                {
                    case 1:
                    {       DUH_WHERE_AM_I("here 1.");
                        if (communication_buffer_1[0] == MSGTYPE_MOVE)
                        {
                            unsigned char x_tmp = communication_buffer_1[1];
                            unsigned char y_tmp = communication_buffer_1[2];

                            if (gamerooms[index].board[x_tmp + (y_tmp * 3)] == 0)
                            {
                                // ONLY do the move if this square is empty.
                                // we shouldn't get illegal moves, but...
                                gamerooms[index].board[x_tmp + (y_tmp * 3)] = 'x';

                                // player 1 has moved, player 2's turn now
                                gamerooms[index].whose_turn = 2;

                                // before we do anything, make sure the game didn't just end...
                                int result = GMRM_check_if_won(&gamerooms[index]);
                                if (result != GAMEROOM_STILL_PLAYING)
                                {
                                    // game just ended

                                    if (result == 'x')
                                    {
                                        // track this in the stats...
                                        gamerooms[index].plyr_1->games_won++;
                                        gamerooms[index].plyr_2->games_lost++;

                                        // ...and notify the clients...
                                        communication_buffer_1[0] = MSGTYPE_YOU_WIN;
                                        communication_buffer_2[0] = MSGTYPE_YOU_LOSE;

                                        send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);

                                        send(gamerooms[index].plyr_2->connection_fd, communication_buffer_2,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);

                                        // ...and free up the room.
                                        gamerooms[index].occupied = FALSE;
                                    }
                                    else
                                    {
                                        // tie - repeat win steps above, but with different values
                                        gamerooms[index].plyr_1->games_tied++;
                                        gamerooms[index].plyr_2->games_tied++;

                                        communication_buffer_1[0] = MSGTYPE_YOU_TIE;

                                        send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);
                                        send(gamerooms[index].plyr_2->connection_fd, communication_buffer_1,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);

                                        gamerooms[index].occupied = FALSE;
                                    }

                                    gamerooms[index].plyr_1->state = GAMESTATE_STAT_SCREEN;
                                    gamerooms[index].plyr_2->state = GAMESTATE_STAT_SCREEN;
                                }
                                else
                                {
                                    gamerooms[index].whose_turn = 2;
                                    // we're still underway - notify players what state they should be in
//                                    communication_buffer_1[0] = MSGTYPE_WAIT_FOR_OPPONENT;
                                    communication_buffer_2[0] = MSGTYPE_ITS_YOUR_TURN;

                                    communication_buffer_2[1] = gamerooms[index].board[0];
                                    communication_buffer_2[2] = gamerooms[index].board[1];
                                    communication_buffer_2[3] = gamerooms[index].board[2];

                                    communication_buffer_2[4] = gamerooms[index].board[3];
                                    communication_buffer_2[5] = gamerooms[index].board[4];
                                    communication_buffer_2[6] = gamerooms[index].board[5];

                                    communication_buffer_2[7] = gamerooms[index].board[6];
                                    communication_buffer_2[8] = gamerooms[index].board[7];
                                    communication_buffer_2[9] = gamerooms[index].board[8];


//                                    send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
//                                        1, MSG_DONTWAIT | MSG_NOSIGNAL);
                                    send(gamerooms[index].plyr_2->connection_fd, communication_buffer_2,
                                        10,  MSG_NOSIGNAL);
                                }
                            }
                            else
                            {
                                // player 1 clicked in an occupied square - let them know we're on to them.
                                snprintf(out_buffer, OUTGOING_CHAT_MESSAGE_LENGTH, "server: %s tried to cheat.",
                                    gamerooms[index].plyr_1->name);

                                send(gamerooms[index].plyr_1->connection_fd, out_buffer,
                                    OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);

                                send(gamerooms[index].plyr_2->connection_fd, out_buffer,
                                    OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);

                                // no need to change gamestates, since it's still p1's turn...
                            }
                        }
                    } // -------- end the case where it's player one's turn.
                    break;

                    /********************************************************************************************/

                    case 2:
                    {
                        // logic is pretty much identical to player one's
                        if (communication_buffer_2[0] == MSGTYPE_MOVE)
                        {
                            unsigned char x_tmp = communication_buffer_2[1];
                            unsigned char y_tmp = communication_buffer_2[2];

                            if (gamerooms[index].board[x_tmp + (y_tmp * 3)] == 0)
                            {
                                gamerooms[index].board[x_tmp + (y_tmp * 3)] = 'o';

                                // player 2 has moved, player 1's turn now
                                gamerooms[index].whose_turn = 1;

                                int result = GMRM_check_if_won(&gamerooms[index]);
                                if (result != GAMEROOM_STILL_PLAYING)
                                {
                                    // game just ended
                                    if (result == 'o')
                                    {
                                        // win
                                        gamerooms[index].plyr_2->games_won++;
                                        gamerooms[index].plyr_1->games_lost++;

                                        communication_buffer_1[0] = MSGTYPE_YOU_LOSE;
                                        communication_buffer_2[0] = MSGTYPE_YOU_WIN;

                                        send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);

                                        send(gamerooms[index].plyr_2->connection_fd, communication_buffer_2,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);

                                        gamerooms[index].occupied = FALSE;


                                    }
                                    else
                                    {
                                        // tie
                                        gamerooms[index].plyr_1->games_tied++;
                                        gamerooms[index].plyr_2->games_tied++;

                                        communication_buffer_1[0] = MSGTYPE_YOU_TIE;

                                        send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);
                                        send(gamerooms[index].plyr_2->connection_fd, communication_buffer_1,
                                            1, MSG_DONTWAIT | MSG_NOSIGNAL);

                                        gamerooms[index].occupied = FALSE;
                                    }

                                    gamerooms[index].plyr_1->state = GAMESTATE_STAT_SCREEN;
                                    gamerooms[index].plyr_2->state = GAMESTATE_STAT_SCREEN;
                                }
                                else
                                {
                                    gamerooms[index].whose_turn = 1;
                                    // we're still underway - notify players what state they should be in
                                    communication_buffer_1[0] = MSGTYPE_ITS_YOUR_TURN;
//                                    communication_buffer_2[0] = MSGTYPE_WAIT_FOR_OPPONENT;

                                    communication_buffer_1[1] = gamerooms[index].board[0];
                                    communication_buffer_1[2] = gamerooms[index].board[1];
                                    communication_buffer_1[3] = gamerooms[index].board[2];

                                    communication_buffer_1[4] = gamerooms[index].board[3];
                                    communication_buffer_1[5] = gamerooms[index].board[4];
                                    communication_buffer_1[6] = gamerooms[index].board[5];

                                    communication_buffer_1[7] = gamerooms[index].board[6];
                                    communication_buffer_1[8] = gamerooms[index].board[7];
                                    communication_buffer_1[9] = gamerooms[index].board[8];


                                    send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                                        10,  MSG_NOSIGNAL);
//                                    send(gamerooms[index].plyr_2->connection_fd, communication_buffer_2,
//                                        1, MSG_DONTWAIT | MSG_NOSIGNAL);
                                }
                            }
                            else
                            {
                                // player 2 clicked in an occupied square - let them know we're on to them.
                                snprintf(out_buffer, OUTGOING_CHAT_MESSAGE_LENGTH, "server: %s tried to cheat.",
                                    gamerooms[index].plyr_2->name);

                                send(gamerooms[index].plyr_1->connection_fd, out_buffer,
                                    OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);

                                send(gamerooms[index].plyr_2->connection_fd, out_buffer,
                                    OUTGOING_CHAT_MESSAGE_LENGTH, MSG_DONTWAIT | MSG_NOSIGNAL);
                            }
                        }
                    } // -------- end the case where it's player two's turn.
                    break;

                    default:
                        DUH_WHERE_AM_I("SHOULD NOT GET HERE.");
                }

                // handle quit/disconnect message.
                // if either player quits during the game, nothing happens to their stats and the other person
                // gets an automatic win for now (there isn't a field for disconnects (yet))
                //
                // todo: make sure this isn't exploitable with both clients quitting to collude and
                // give each other wins...
                if (gamerooms[index].occupied) // check this to handle a race between someone quitting
                {                               // simultaneously with the round ending...

                    if ((communication_buffer_1[0] == MSGTYPE_CLIENT_QUITTING) && (communication_buffer_2[0] != MSGTYPE_CLIENT_QUITTING))
                    {
                        gamerooms[index].plyr_2->games_won++;

                        communication_buffer_2[0] = MSGTYPE_YOU_WIN;
                        send(gamerooms[index].plyr_2->connection_fd, communication_buffer_2,
                            1, MSG_DONTWAIT | MSG_NOSIGNAL);
                    }

                    if ((communication_buffer_2[0] == MSGTYPE_CLIENT_QUITTING) && (communication_buffer_1[0] != MSGTYPE_CLIENT_QUITTING))
                    {
                        gamerooms[index].plyr_1->games_won++;

                        communication_buffer_1[0] = MSGTYPE_YOU_WIN;
                        send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                            1, MSG_DONTWAIT | MSG_NOSIGNAL);
                    }

                    if ((communication_buffer_1[0] == MSGTYPE_CLIENT_QUITTING) || (communication_buffer_2[0] == MSGTYPE_CLIENT_QUITTING))
                    {
                        // room not needed anymore
                        gamerooms[index].occupied = FALSE;
                    }
                }

                // if we get any other kinds of message here, the client's royally hosed,
                // but we certainly don't care about that, now do we?  (⍛‿⍛)
            }
            else
            {
                // yep, no one said anything
                // how long have we been idle?
                if (gamerooms[index].ticks_since_last_move > GAMEROOM_MAX_IDLE_TICKS)
                {
                    // <applejack mood="annoyed">both o' y'all waited too long, get out of mah orchard</applejack>
                    communication_buffer_1[0] = MSGTYPE_GAMEPLAY_TIMED_OUT;

                    send(gamerooms[index].plyr_1->connection_fd, communication_buffer_1,
                        MAX_MESSAGE_SIZE, MSG_DONTWAIT | MSG_NOSIGNAL);

                    send(gamerooms[index].plyr_2->connection_fd, communication_buffer_1,
                        MAX_MESSAGE_SIZE, MSG_DONTWAIT | MSG_NOSIGNAL);

                    // reap the room
                    gamerooms[index].occupied = FALSE;
                }
            }

        } // end the case where this game room was occupied
    } // end for
}
