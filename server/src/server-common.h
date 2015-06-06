/*! \file server-common.h
 * \brief Contains the server-common functionality in a module.
 */
#ifndef         SERVER_COMMON_H
    #define     SERVER_COMMON_H

    #include    "tictactwo-common.h"
    #include    <signal.h>
    #include    <fcntl.h>

    BOOL        SERVER_init(void);

    extern int  server_listenfd_game;
    extern int  server_listenfd_http;

#endif
