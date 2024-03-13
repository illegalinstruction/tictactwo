/*! \file name_and_server_entry.h
 */
#ifndef         NAME_AND_SERVER_ENTRY_H
    #define     NAME_AND_SERVER_ENTRY_H

    #include    "client-common.h"
    #include    "screenwipe.h"
    #include    "text_entry_widget.h"

    /*! \defgroup login_public_iface The public interface to the login screen gamestate.
     * \{ */
    void LOGIN_init(void);
    void LOGIN_tick(void);
    void LOGIN_draw(void);
    void LOGIN_load(void);
    void LOGIN_unload(void);
    /*! \} */
#endif
