#include "server-common.h"

/*! \defgroup server_common_priv
 * \brief Private data and functions for use by the server module.
 * \{
 */
static BOOL server_was_inited_yet = FALSE;
static void SERVER_signal_handler(int signal_num);
static void SERVER_cleanup(void);
/*! \} */

/*! \brief The socket the application listens for incoming player connections on.
 * \note Public because both the active-player manager and the game manager will need it.
 */
int server_listenfd_game = -1;

/*! \brief The socket the application listens for requests for an html list of logged-in players on.
 * \note Public because the active-player manager and player-db modules need it.
 */
int server_listenfd_http = -1;

/****************************************************************************************************************/
/*! \brief Tries to set up the game and http listening socket for use.
 *  \return TRUE if it was successful and we're ready to start accepting players and serving stats, or FALSE if
 *      it failed for any reason.
 *  \note As the port numbers is hard-coded (by design), this will return FALSE if it can't get the port numbers
 *      it wants.  Future versions may relax this restriction and accept ports number on the cmd line.
 */
BOOL SERVER_init(void)
{
    // ensure this only happens ONCE
    if (server_was_inited_yet) return TRUE;

    server_was_inited_yet = TRUE;

    signal(SIGINT, SERVER_signal_handler);
    signal(SIGTERM, SERVER_signal_handler);
    signal(SIGHUP, SERVER_signal_handler);

    // we close all the sockets and such in SERVER_cleanup(),
    // which will get called if SERVER_init() fails, so the
    // multiple return paths below are safe...
    atexit(SERVER_cleanup);

    // On to the actual socket creation.
    struct sockaddr_in my_address;

    //------- gameplay port
    my_address.sin_family       = AF_INET;
    my_address.sin_addr.s_addr  = INADDR_ANY;
    my_address.sin_port         = htons(TICTACTWO_GAMEPLAY_PORT);

    server_listenfd_game = socket(AF_INET, SOCK_STREAM, 0);
    if (server_listenfd_game == -1)
    {
        OH_SMEG("Call to socket() failed.");
        return FALSE;
    }

    fcntl(server_listenfd_game, F_SETFL, O_NONBLOCK);

    if (bind(server_listenfd_game, (struct sockaddr *) &my_address, sizeof(my_address)) == -1)
    {
        OH_SMEG("Unable bind to gameplay port.");
        return FALSE;
    }

    if (listen(server_listenfd_game, 0) == -1)
    {
        OH_SMEG("It won't let me listen on the gameplay port.");
        return FALSE;
    }

/* punted to a future version
    //------- webserver port
    my_address.sin_port         = htons(TICTACTWO_WEBPAGE_PORT);

    server_listenfd_http = socket(AF_INET, SOCK_STREAM, 0)
    if (server_listenfd_http == -1)
    {
        OH_SMEG("Call to socket() failed.");
        return FALSE;
    }

    fcntl(server_listenfd_http, F_SETFL, O_NONBLOCK);

    if (bind(server_listenfd_http, (struct sockaddr *) &my_address, sizeof(my_address)) == -1)
    {
        OH_SMEG("Unable bind to webserver port.");
        return FALSE;
    }

    if (listen(server_listenfd_http, 0) == -1)
    {
        OH_SMEG("It won't let me listen on the webserver port.");
        return FALSE;
    }
*/
    // if we're all the way down here, we should have succeeded at everything and
    // now are listening on 5555 and 8080.
    return TRUE;
}

/****************************************************************************************************************/
/*! \brief Cleans up and closes any ports we were listening on.  Designed to be called automagically on exit.
 */
static void SERVER_signal_handler(int signal_num)
{
    switch (signal_num)
    {
        case SIGINT:
        case SIGHUP:
        case SIGTERM:
        {
            DUH_WHERE_AM_I("We've been asked to shut down...");
            exit(0); // the individual modules' cleanup functions will run automatically at this point.
        }
    }
}

/****************************************************************************************************************/
/*! \brief Cleans up and closes any ports we were listening on.  Designed to be called automagically on exit.
 */
static void SERVER_cleanup(void)
{
    if (server_listenfd_http != -1)
        close(server_listenfd_http);

    if (server_listenfd_game != -1)
        close(server_listenfd_game);
}
