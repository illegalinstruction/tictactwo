#ifndef         WAIT_FOR_OPPONENT_H
    #define     WAIT_FOR_OPPONENT_H

    #include "client-common.h"

    /*! \defgroup oppwait_public_iface Public interface for the 'waiting for opponent' gamestate.
     * \{ */
    void OPPWAIT_init(void);
    void OPPWAIT_tick(void);
    void OPPWAIT_draw(void);
    void OPPWAIT_load(void);
    void OPPWAIT_unload(void);
    /*! \} */

#endif
