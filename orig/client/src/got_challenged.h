#ifndef         GOT_CHALLENGED_H
    #define     GOT_CHALLENGED_H

    #include "client-common.h"

    /*! \defgroup challresp_public_iface Public interface for the 'you have been challenged' gamestate.
     * \{ */
    void CHALLRESP_init(void);
    void CHALLRESP_tick(void);
    void CHALLRESP_draw(void);
    void CHALLRESP_load(void);
    void CHALLRESP_unload(void);
    /*! \} */

#endif
