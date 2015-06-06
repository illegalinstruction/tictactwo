#ifndef         GAME_ROOM_H
    #define     GAME_ROOM_H

    #include    "tictactwo-common.h"
    #include    "player_db.h"

    /*! \defgroup gameroom_resolutions
     * \brief Various states a game can be in - returned by GMRM_check_if_won()
     * \{
     */
    #define     GAMEROOM_PLAYER_ONE_WON     1
    #define     GAMEROOM_PLAYER_TWO_WON     2
    #define     GAMEROOM_TIE                3
    #define     GAMEROOM_STILL_PLAYING      0
    /*! \} */

    /*! \brief A structure that represents an in-progress game.
     */
    typedef struct
    {
        uint8_t         board[BOARD_WIDTH * BOARD_HEIGHT];
        BOOL            occupied;
        PLAYER_STRUCT   *plyr_1;
        PLAYER_STRUCT   *plyr_2;
        int             whose_turn;
        int             who_went_first;
        /*! \brief Used to time out and reap rooms where one or
         * more players are disconnected or otherwise not playing
         */
        uint32_t        ticks_since_last_move;
    } GAMEROOM_STRUCT;

    void    GMRM_init(void);
    BOOL    GMRM_create_new(PLAYER_STRUCT *player_1, PLAYER_STRUCT *player_2);
    void    GMRM_handle_incoming_move(int row, int col, int plyr);
    uint8_t GMRM_check_if_won(const GAMEROOM_STRUCT *gs);
    void    GMRM_tick_all(void);

#endif
