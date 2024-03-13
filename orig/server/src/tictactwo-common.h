#ifndef         TICTACTWO_COMMON_H
    #define     TICTACTWO_COMMON_H
    #include    <arpa/inet.h>
    #include    <sys/types.h>
    #include    <sys/socket.h>
    #include    <netinet/in.h>
    #include    <stdint.h>
    #include    <strings.h> // this is hilarious to me
    #include    <string.h>  // right now - why?
    #include    <stdlib.h>
    #include    <stdio.h>

    #define     DUH_WHERE_AM_I(...) {                                           \
                                        fprintf(stderr,"\n%s, %s(), line %i: ", \
                                        __FILE__,__FUNCTION__,__LINE__);        \
                                        fprintf(stderr,__VA_ARGS__);            \
                                        fprintf(stderr,"\n");                   \
                                    }

    #define     OH_SMEG(...) {                                           \
                                        fprintf(stderr,"\n\x1b[30;41m/!\\\x1b[0m%s, %s(), line %i: ", \
                                        __FILE__,__FUNCTION__,__LINE__);        \
                                        fprintf(stderr,__VA_ARGS__);            \
                                        fprintf(stderr,"\n");                   \
                             }

    #ifndef         BOOL
        #define     BOOL    unsigned
    #endif

    #ifndef         FALSE
        #define     FALSE   0
    #endif

    #ifndef         TRUE
        #define     TRUE    ~0          // some versions of GCC dislike this - in this case, it's intentional...
    #endif

    /*! \defgroup protocol_defs
     * \brief The various message types supported by the game protocol.
     * \note These are all one byte long and go at the beginning of a
     * message; based on the leading byte, the client and server will
     * take action on or interpret the payload bytes after it.
     * \{
     */
    #define     MSGTYPE_LOGIN                   (unsigned char)'>'
    #define     MSGTYPE_LOGIN_SUCCESSFUL        (unsigned char)'<'
    #define     MSGTYPE_DENIED_DUPLICATE_NAME   (unsigned char)'-'
    #define     MSGTYPE_REQUEST_LOBBY           (unsigned char)'R'
    #define     MSGTYPE_INVITE                  (unsigned char)'i'
    #define     MSGTYPE_YOUVE_BEEN_INVITED      (unsigned char)'I'
    #define     MSGTYPE_RESPOND_ACCEPT          (unsigned char)'A'
    #define     MSGTYPE_RESPOND_DECLINE         (unsigned char)'D'
    #define     MSGTYPE_GOT_ACCEPTED            (unsigned char)'a'
    #define     MSGTYPE_GOT_DECLINED            (unsigned char)'d'
    #define     MSGTYPE_CHAT                    (unsigned char)'|'
    #define     MSGTYPE_START_GAME              (unsigned char)'!'
    #define     MSGTYPE_MOVE                    (unsigned char)'m'
    #define     MSGTYPE_ITS_YOUR_TURN           (unsigned char)'T'
    #define     MSGTYPE_WAIT_FOR_OPPONENT       (unsigned char)'t'
    #define     MSGTYPE_YOU_WIN                 (unsigned char)'W'
    #define     MSGTYPE_YOU_LOSE                (unsigned char)'L'
    #define     MSGTYPE_YOU_TIE                 (unsigned char)'c'
    #define     MSGTYPE_DONE_WITH_STAT_SCREEN   (unsigned char)'.'
    #define     MSGTYPE_CLIENT_QUITTING         (unsigned char)'q'
    #define     MSGTYPE_NO_FREE_ROOMS           (unsigned char)'n'
    #define     MSGTYPE_GAMEPLAY_TIMED_OUT      (unsigned char)'O'

    #define     MSGTYPE_YOU_ARE_X               (unsigned char)'x'
    #define     MSGTYPE_YOU_ARE_O               (unsigned char)'o'

    /*! \brief Catch-all for the case that something unrecoverable happened on the server
     * \note Upon receiving this, a client should go directly to the 'connection failure' screen.
     */
    #define     MSGTYPE_FAILURE                 (unsigned char)'F'

    /*! \} */

    /*! \defgroup client_states
     * \brief The various states or stages the client may be in.  Most of them are mirrored on
     * the server, although some, like the splash screen, don't have useful analogues.
     *
     * In the client code, they're also used as indices into an array of GAMESTATE structures
     * that contain callbacks for advancing the logic of a state, making the state repaint itself,
     * etc.
     * \{
     */
    #define GAMESTATE_SPLASH_SCREEN             0
    #define GAMESTATE_NAME_AND_SERVER_ENTRY     1
    #define GAMESTATE_CONNECTING_PLEASE_WAIT    2
    #define GAMESTATE_LOBBY                     3
    #define GAMESTATE_WAITING_ON_OPPONENT       4
    #define GAMESTATE_RECEIVED_INVITATION       5
    #define GAMESTATE_WAITING_FOR_HANDSHAKE     6
    #define GAMESTATE_GAMEPLAY                  7
    #define GAMESTATE_STAT_SCREEN               8
    #define GAMESTATE_CONNECTION_FAILURE        9
    #define GAMESTATE_CONNECTION_TIMEOUT        10

    #define NUM_GAMESTATES                      11

    /*! \brief a 'fake' gamestate for the server to remember this player isn't active right now without
     *  combing the active player table... */
    #define GAMESTATE_NOT_CONNECTED             99
    /*! \} */

    #define     AVATAR_ID_POSITION              32

    #define     LOBBY_LIST_RECORD_SIZE          48

    #define     MAX_NAME_LENGTH                 30
    #define     MAX_CHAT_LENGTH                 30

    #define     MAX_MESSAGE_SIZE                64  // please see doc/feature-list for details. this ONLY applies
                                                    // to messages coming in from the client.

    #define     OUTGOING_CHAT_MESSAGE_LENGTH    (1 + MAX_NAME_LENGTH + 2 + MAX_CHAT_LENGTH + 1 + 1)
                                            //  cmd  plyr name       ": "  what they said  NULL  avatar id

    #define     BOARD_WIDTH                     3
    #define     BOARD_HEIGHT                    3

    #define     MAX_ACTIVE_PLAYERS              64                         // this is already gonna mean some HUGE messages for lobby refresh...
    #define     MAX_ACTIVE_ROOMS                (MAX_ACTIVE_PLAYERS / 2)

    #define     TICTACTWO_GAMEPLAY_PORT         5555
    #define     TICTACTWO_WEBPAGE_PORT          8080

    #define     NUM_AVATARS                     10

#endif          // TICTACTWO_COMMON_H
