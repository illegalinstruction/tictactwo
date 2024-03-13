/*! \file player_db.h
 * \brief Contains stuff for storing and retrieving player stats.
 * In the ideal world, there'd be support for querying won and lost records,
 * etc.; in the current design, the module that manages already-logged-in
 * players probably knows a little more about the internals of the PLAYER_STRUCT
 * than it should, but...eghh.
 */
#ifndef             PLAYER_DB_H
    #define         PLAYER_DB_H

    #include        "tictactwo-common.h"

    /*! \brief Structure that maps to a representation of a player the
     *  server has seen before.
     * \todo Need a password field (and to make sure it's written out to disk and
     *  checked against on connection.
     * \note This would scale better if the players got to live in a tree instead
     * of a list...but then, if scalability is the goal, we should just use SQLite
     * or something like that...
     */
    typedef struct
    {
        unsigned char   name[MAX_NAME_LENGTH];
        /*! \brief This is a convenience to the login manager; it won't contain anything useful if
         * this player isn't logged in.
         */
        int             connection_fd;
        uint32_t        games_won;
        uint32_t        games_lost;
        uint32_t        games_tied;
        /*! \brief It's an index into an array of forty-ish pixmaps the client loads on startup.
         * Used during the lobby and gameplay. (and by 40, I mean 10 (deadlines))
         */
        uint8_t         avatar;
        void            *next;
        void            *prev;
        /*! \brief It's used by the active player manager to track whether we're in a game, the lobby, etc. */
        uint8_t         state;
        /*! \brief Tracks who we were challenged by */
        int             challenger_id;
    } PLAYER_STRUCT;

    PLAYER_STRUCT   *PLYRDB_find_by_name(const char *name);
    PLAYER_STRUCT   *PLYRDB_create_new_player(const char *name);
    void            PLYRDB_load_from_disk(void);
    void            PLYRDB_save_to_disk(void);
#endif
