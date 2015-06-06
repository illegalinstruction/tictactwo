#ifndef         CONNECTION_FAILURE_H
    #define     CONNECTION_FAILURE_H

    #include    "client-common.h"
    #include    "screenwipe.h"

    /*! \defgroup connection_failure_iface The public interface to the Connection Failed screen's module.
     * \{ */
    void CONNFAIL_init(void);
    void CONNFAIL_tick(void);
    void CONNFAIL_draw(void);
    void CONNFAIL_load(void);
    void CONNFAIL_unload(void);
    /*! \} */
#endif
