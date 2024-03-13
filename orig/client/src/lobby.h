#ifndef         LOBBY_H
    #define     LOBBY_H

    #include "client-common.h"

    /*! \defgroup lobby_public_iface Public interface for the lobby gamestate.
     * \{ */
    void LOBBY_init(void);
    void LOBBY_tick(void);
    void LOBBY_draw(void);
    void LOBBY_load(void);
    void LOBBY_unload(void);
    /*! \} */

#endif
